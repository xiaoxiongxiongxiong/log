﻿#include "log.h"
#include "utils.h"
#include "queue.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#if defined(WIN32) || defined(_WIN32)
#include <Windows.h>
#endif

#define OS_LOG_DATE_MAX 32       // 日志时间长度 
#define OS_LOG_MSG_MAX  3072     // 日志信息最大长度
#define OS_LOG_LINE_MAX 4096     // 日志每行打印的最大字节数

typedef struct _log_node_t
{
    int line;                     // 日志所在行数
    int len;                      // 日志消息长度
    pthread_t tid;                // 线程id
    LOG_MSG_LEVEL level;          // 日志级别
    int64_t ts;                   // 日志打印时间戳
    char * buff;                  // 日志消息，长度超出OS_LOG_MSG_MAX时启用
    char file[OS_UTILS_FILE_MAX]; // 日志消息所在文件
    char func[OS_UTILS_FILE_MAX]; // 日志消息所在函数
    char msg[OS_LOG_MSG_MAX];     // 日志消息
} log_node_t;

typedef struct _os_log_context_t
{
    char path[OS_UTILS_PATH_MAX]; // 日志路径
    size_t slice_duration;        // 切片时长
    size_t slice_size;            // 切片大小
    size_t slice_count;           // 切片个数
    FILE * fp;                    // 文件句柄
    LOG_MSG_LEVEL level;          // 保存到文件中的日志级别
    pthread_t thr;
    pthread_cond_t cond;
    pthread_mutex_t mtx;
    os_queue_t * oq;              // 队列实例
    bool inited;                  // 是否初始化
} os_log_context_t;

// 日志级别字符串
static const char * g_level_str[] = { "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };
static os_log_context_t g_log_ctx = { 0 };

// 日志消息处理线程
static void log_msg_deal_thr(void * arg);
// 日志打印及写入文件
static void log_msg_write_file(const log_node_t * node);
// 日志初始化
static int os_log_startup();
// 日志销毁
static void os_log_cleanup();

int log_msg_init(const char * file, const LOG_MSG_LEVEL level)
{
    if (g_log_ctx.inited)
        return 0;

    if (level <= LOG_LEVEL_DEBUG)
        g_log_ctx.level = LOG_LEVEL_DEBUG;
    else if (level >= LOG_LEVEL_FATAL)
        g_log_ctx.level = LOG_LEVEL_FATAL;
    else
        g_log_ctx.level = level;

    if (NULL != file && '\0' != file[0])
    {
        if (!os_utils_create_directory(file))
        {
            const int code = errno;
            fprintf(stderr, "Create directory failed for %s\n", strerror(code));
            return -1;
        }
        g_log_ctx.fp = fopen(file, "a");
        if (NULL == g_log_ctx.fp)
        {
            const int code = errno;
            fprintf(stderr, "Open file failed for %s\n", strerror(code));
            return -1;
        }
        strncpy(g_log_ctx.path, file, OS_UTILS_PATH_MAX - 1);
    }

    g_log_ctx.oq = os_queue_create(sizeof(log_node_t));
    if (NULL == g_log_ctx.oq)
    {
        log_msg_uninit();
        fprintf(stderr, "Create log queue failed\n");
        return -1;
    }

    if (0 != os_log_startup())
    {
        log_msg_uninit();
        fprintf(stderr, "Create thread failed\n");
        return -1;
    }

    return 0;
}

void log_msg_uninit(void)
{
    os_log_cleanup();

    if (NULL != g_log_ctx.fp)
    {
        fclose(g_log_ctx.fp);
        g_log_ctx.fp = NULL;
    }

    os_queue_destroy(&g_log_ctx.oq);

    g_log_ctx.slice_duration = 0;
    g_log_ctx.slice_size = 0;
    g_log_ctx.slice_count = 0;
}

void log_msg_set_slice_duration(const size_t ms)
{
    if (0ULL != ms)
    {
        g_log_ctx.slice_duration = ms;
    }
}

void log_msg_set_slice_size(const size_t kb)
{
    if (0ULL != kb)
    {
        g_log_ctx.slice_size = kb;
    }
}

int log_msg(const LOG_MSG_LEVEL level, const char * file, const int line, const char * func, const char * fmt, ...)
{
    if (!g_log_ctx.inited)
        return 0;

    va_list vl;

    va_start(vl, fmt);
    int len = log_msg_doit(level, file, line, func, fmt, vl);
    va_end(vl);

    return len;
}

int log_msg_doit(const LOG_MSG_LEVEL level, const char * file, const int line, const char * func, const char * fmt, va_list vl)
{
    if (!g_log_ctx.inited)
        return 0;

    log_node_t node = { 0 };
    node.ts = os_utils_time_ms();
    // 级别修正
    if (level >= LOG_LEVEL_DEBUG && level <= LOG_LEVEL_FATAL)
        node.level = level;
    else
        node.level = LOG_LEVEL_FATAL;
    node.line = line;

    node.tid = pthread_self();
    if (NULL != file)
        os_utils_file_name(file, node.file, OS_UTILS_FILE_MAX);

    // 所在函数
    if (func)
        strncpy(node.func, func, OS_UTILS_FILE_MAX - 1);

    // 格式化日志
    int len = vsnprintf(node.msg, sizeof(node.msg), fmt, vl);
    if (len > sizeof(node.msg) - 1)
    {
        node.len = len + 1;
        node.buff = (char *)malloc((size_t)node.len);
        if (NULL == node.buff)
        {
            fprintf(stderr, "No enough memory\n");
            return -1;
        }
        vsnprintf(node.buff, len, fmt, vl);
        node.buff[len] = '\0';
    }
    pthread_mutex_lock(&g_log_ctx.mtx);
    os_queue_push(g_log_ctx.oq, &node);
    pthread_cond_signal(&g_log_ctx.cond);
    pthread_mutex_unlock(&g_log_ctx.mtx);

    // 根据级别判定是否退出
    if (level == LOG_LEVEL_FATAL)
    {
        log_msg_uninit();  //日志反初始化并杀掉当前进程
        exit(EXIT_FAILURE);
    }

    return len;
}

int os_log_startup()
{
    g_log_ctx.inited = true;
    int ret = pthread_cond_init(&g_log_ctx.cond, NULL);
    if (0 != ret)
    {
        fprintf(stderr, "pthread_cond_init failed\n");
        g_log_ctx.inited = false;
        return -1;
    }

    ret = pthread_mutex_init(&g_log_ctx.mtx, NULL);
    if (0 != ret)
    {
        fprintf(stderr, "pthread_mutex_init failed\n");
        pthread_cond_destroy(&g_log_ctx.cond);
        g_log_ctx.inited = false;
        return -1;
    }

    ret = pthread_create(&g_log_ctx.thr, NULL, (void *)log_msg_deal_thr, (void *)&g_log_ctx);
    if (0 != ret)
    {
        fprintf(stderr, "pthread_create failed\n");
        pthread_cond_destroy(&g_log_ctx.cond);
        pthread_mutex_destroy(&g_log_ctx.mtx);
        g_log_ctx.inited = false;
        return -1;
    }

    return 0;
}

void os_log_cleanup()
{
    if (!g_log_ctx.inited)
        return;

    g_log_ctx.inited = false;
    pthread_cond_signal(&g_log_ctx.cond);
    pthread_join(g_log_ctx.thr, NULL);
    pthread_cond_destroy(&g_log_ctx.cond);
    pthread_mutex_destroy(&g_log_ctx.mtx);
}

void log_msg_deal_thr(void * arg)
{
    os_log_context_t * ctx = (os_log_context_t *)arg;
    if (NULL == ctx)
        return;

    while (&ctx->inited)
    {
        pthread_mutex_lock(&ctx->mtx);
        while (os_queue_empty(ctx->oq) && &ctx->inited)
        {
            pthread_cond_wait(&ctx->cond, &ctx->mtx);
        }

        while (!os_queue_empty(ctx->oq))
        {
            os_queue_node_t * node = os_queue_front(ctx->oq);
            log_node_t * data = (log_node_t *)os_queue_getdata(node);
            log_msg_write_file(data);
            os_queue_pop(ctx->oq);
        }
        pthread_mutex_unlock(&ctx->mtx);
    }

    while (!os_queue_empty(ctx->oq))
    {
        os_queue_node_t * node = os_queue_front(ctx->oq);
        log_node_t * data = (log_node_t *)os_queue_getdata(node);
        log_msg_write_file(data);
        os_queue_pop(ctx->oq);
    }
}

void log_msg_write_file(const log_node_t * node)
{
    char log_buf[OS_LOG_LINE_MAX] = { 0 };
    //snprintf(log_buf, OS_LOG_LINE_MAX, "[%s][%s][%ld][%s:%d][%s] %s\n",
    //         node.date, g_level_str[node.level], node.tid, node.file,
    //         node.line, node.func, node.msg);

#ifndef NDEBUG
    fflush(stdout);
    fputs(log_buf, stderr);
    fflush(stderr);
#endif

#if defined(WIN32) || defined(_WIN32)
    OutputDebugStringA(log_buf);
#endif

    if (node->level >= g_log_ctx.level)
    {
        if (!os_utils_file_exist(g_log_ctx.path))
        {
            os_utils_create_directory(g_log_ctx.path);
            g_log_ctx.fp = freopen(g_log_ctx.path, "a", g_log_ctx.fp);
        }

        if (NULL != g_log_ctx.fp)
        {
            if (EOF == fputs(log_buf, g_log_ctx.fp))  // 写入文件失败时打印到控制台
            {
                fflush(stdout);
                fputs(log_buf, stderr);
                fflush(stderr);
            }
            else
                fflush(g_log_ctx.fp);
        }
    }
}
