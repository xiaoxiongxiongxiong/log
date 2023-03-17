#include "log.h"
#include "os_directory.h"
#include "os_time.h"
#include <cstdbool>
#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>

#if defined(WIN32) || defined(_WIN32)
#include <Windows.h>
#else
#include <cstring>
#endif

#define OS_LOG_DATE_MAX 32       // 日志时间长度 
#define OS_LOG_MSG_MAX  3072     // 日志信息最大长度
#define OS_LOG_LINE_MAX 4096     // 日志每行打印的最大字节数

typedef struct _log_node_t
{
    int line;                     // 日志所在行数
    long tid;                     // 线程id
    LOG_MSG_LEVEL level;          // 日志级别
    char date[OS_LOG_DATE_MAX];   // 打印日志时间
    char file[OS_UTILS_FILE_MAX]; // 日志消息所在文件
    char func[OS_UTILS_FILE_MAX]; // 日志消息所在函数
    char msg[OS_LOG_MSG_MAX];     // 日志消息
} log_node_t;

typedef struct _os_log_t
{
    char path[OS_UTILS_PATH_MAX]; // 日志路径
    std::deque<log_node_t> nodes; // 日志消息队列
    std::thread thr;              // 线程
    std::mutex mtx;               // 线程锁
    std::condition_variable cond; // 条件变量
    size_t slice_duration;        // 切片时长
    size_t slice_size;            // 切片大小
    FILE * fp;                    // 文件句柄
    LOG_MSG_LEVEL level;          // 保存到文件中的日志级别
    bool inited;                  // 是否初始化
} os_log_t;

// 日志级别字符串
static const char * g_log_level_str[] = { "DEBUG", "INFO", "WARN", "ERROR", "FATAL" };
static os_log_t g_log_info = {};

// 日志消息处理线程
static void log_msg_deal_thr();
// 日志打印及写入文件
static void log_msg_write_file(const log_node_t & node);

int log_msg_init(const char * file, const LOG_MSG_LEVEL level)
{
    if (g_log_info.inited)
        return 0;

    if (level <= LOG_LEVEL_DEBUG)
        g_log_info.level = LOG_LEVEL_DEBUG;
    else if (level >= LOG_LEVEL_FATAL)
        g_log_info.level = LOG_LEVEL_FATAL;
    else
        g_log_info.level = level;

    if (nullptr != file && '\0' != file[0])
    {
        if (0 != os_directory_create(file))
        {
            const int code = errno;
            printf("Create directory failed for %s\n", strerror(code));
            return -1;
        }

        g_log_info.fp = fopen(file, "a");
        if (nullptr == g_log_info.fp)
        {
            const int code = errno;
            printf("Open file failed for %s\n", strerror(code));
            return -1;
        }
        strncpy(g_log_info.path, file, OS_UTILS_PATH_MAX - 1);
    }

    g_log_info.inited = true;
    g_log_info.thr = std::thread{ log_msg_deal_thr };

    return 0;
}

void log_msg_uninit(void)
{
    if (!g_log_info.inited)
        return;

    if (g_log_info.thr.joinable())
    {
        g_log_info.inited = false;
        g_log_info.thr.join();
    }

    if (nullptr != g_log_info.fp)
    {
        fclose(g_log_info.fp);
        g_log_info.fp = nullptr;
        memset(g_log_info.path, 0, sizeof(g_log_info.path));
    }
}

void log_msg_set_slice_duration(const size_t ms)
{
    if (0ULL != ms)
        g_log_info.slice_duration = ms;
}

void log_msg_set_slice_size(const size_t kb)
{
    if (0ULL != kb)
        g_log_info.slice_size = kb;
}

int log_msg(const LOG_MSG_LEVEL level, const char * file, const int line, const char * func, const char * fmt, ...)
{
    if (!g_log_info.inited)
        return 0;

    va_list vl;

    va_start(vl, fmt);
    int len = log_msg_doit(level, file, line, func, fmt, vl);
    va_end(vl);

    return len;
}

int log_msg_doit(const LOG_MSG_LEVEL level, const char * file, const int line, const char * func, const char * fmt, va_list vl)
{
    if (!g_log_info.inited)
        return 0;

    os_time_t tv = os_time_by_ms(os_time_ms());

    log_node_t node = { 0 };
    // 级别修正
    if (level >= LOG_LEVEL_INFO && level <= LOG_LEVEL_FATAL)
        node.level = level;
    else
        node.level = LOG_LEVEL_FATAL;
    node.line = line;
    snprintf(node.date, OS_LOG_DATE_MAX, "%04d/%02d/%02d %02d:%02d:%02d.%03d",
             tv.year, tv.month, tv.day, tv.hour, tv.minute, tv.second, tv.millisecond);

    node.tid = os_utils_tid();
    if (NULL != file)
        os_utils_file_name(file, node.file, OS_UTILS_FILE_MAX);

    // 所在函数
    if (func)
        strncpy(node.func, func, OS_UTILS_FILE_MAX - 1);

    // 格式化日志
    int len = vsnprintf(node.msg, sizeof(node.msg), fmt, vl);
    std::unique_lock<std::mutex> lck(g_log_info.mtx);
    g_log_info.nodes.emplace_back(node);
    g_log_info.cond.notify_one();

    // 根据级别判定是否退出
    if (level == LOG_LEVEL_FATAL)
    {
        lck.unlock();
        log_msg_uninit();  //日志反初始化并杀掉当前进程
        exit(EXIT_FAILURE);
    }

    return len;
}

void log_msg_deal_thr()
{
    while (g_log_info.inited)
    {
        std::unique_lock<std::mutex> lck(g_log_info.mtx);
        g_log_info.cond.wait(lck);

        while (!g_log_info.nodes.empty())
        {
            auto & node = g_log_info.nodes.front();
            log_msg_write_file(node);
            g_log_info.nodes.pop_front();
        }
    }

    std::unique_lock<std::mutex> lck(g_log_info.mtx);
    while (!g_log_info.nodes.empty())
    {
        auto & node = g_log_info.nodes.front();
        log_msg_write_file(node);
        g_log_info.nodes.pop_front();
    }
}

void log_msg_write_file(const log_node_t & node)
{
    char log_buf[OS_LOG_LINE_MAX] = { 0 };
    snprintf(log_buf, OS_LOG_LINE_MAX, "[%s][%s][%ld][%s:%d][%s] %s\n",
             node.date, g_log_level_str[node.level], node.tid, node.file,
             node.line, node.func, node.msg);

#ifndef NDEBUG
    fflush(stdout);
    fputs(log_buf, stderr);
    fflush(stderr);
#endif

#if defined(WIN32) || defined(_WIN32)
    OutputDebugStringA(log_buf);
#endif

    if (node.level >= g_log_info.level)
    {
        if (!os_directory_exist(g_log_info.path))
        {
            os_directory_create(g_log_info.path);
            g_log_info.fp = freopen(g_log_info.path, "a", g_log_info.fp);
        }

        if (nullptr != g_log_info.fp)
        {
            if (EOF == fputs(log_buf, g_log_info.fp))  // 写入文件失败时打印到控制台
            {
                fflush(stdout);
                fputs(log_buf, stderr);
                fflush(stderr);
            }
            else
                fflush(g_log_info.fp);
        }
    }
}
