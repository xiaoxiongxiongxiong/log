#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

// [repeat times][level][msg]
typedef struct _msg_info_t
{
    int times;     // 重复次数
    int level;     // 日志级别
    char * msg;    // 日志内容
} msg_info_t;

typedef struct _cmd_args_t // 命令行参数
{
    int level;             // 日志级别
    int mis_cnt;           // 日志消息个数
    size_t slice_size;     // 切片大小
    size_t slice_duration; // 切片时长
    char * path;           // 日志路径
    msg_info_t * mis;      // 日志消息实例
} cmd_args_t;

// 打印用法
static void show_usage();
// 解析命令行
static int parse_cmd_args(int argc, char * argv[], cmd_args_t * args);
// 释放命令行参数
static void release_cmd_args(cmd_args_t * args);
// 任务是否进行中
static int g_is_running = 1;

int main(int argc, char * argv[])
{
    cmd_args_t args = { 0 };
    if (0 != parse_cmd_args(argc, argv, &args))
    {
        show_usage();
        return EXIT_SUCCESS;
    }

    if (0 != log_msg_init(args.path, args.level))
    {
        fprintf(stderr, "log_msg_init failed\n");
        release_cmd_args(&args);
        return EXIT_FAILURE;
    }
    atexit(log_msg_uninit);

    for (int i = 0; i < args.mis_cnt; ++i)
    {
        if (0 == g_is_running)
            break;
        for (int j = 0; j < args.mis[i].times; ++j)
        {
            if (0 == g_is_running)
                break;
            if (LOG_LEVEL_DEBUG == args.mis[i].level)
                log_msg_debug(args.mis[i].msg);
            else if (LOG_LEVEL_INFO == args.mis[i].level)
                log_msg_info(args.mis[i].msg);
            else if (LOG_LEVEL_WARN == args.mis[i].level)
                log_msg_warn(args.mis[i].msg);
            else if (LOG_LEVEL_ERROR == args.mis[i].level)
                log_msg_error(args.mis[i].msg);
            else
                log_msg_fatal(args.mis[i].msg);
        }
    }

    release_cmd_args(&args);

    return EXIT_SUCCESS;
}

void show_usage()
{
    printf("log v1.0.0, C language, 2023/09/20\n");
    printf("Usage: test <options>\n");
    printf("  test --log_path=a.log --log_level=2\n");
    printf("  test --log_path=a.log --log_level=1 --log_msg='[2][1][Hello World!]'\n");
    printf("\n");
    printf("Options: \n");
    printf("  %-18s Show usage and exit\n", "--help | -h");
    printf("  %-18s = Log path, default is log_test.log\n", "--log_path=");
    printf("  %-18s = Log level, write to file level, default is 1, range is [0-4]\n", "--log_level=");
    printf("  %-18s = Log slice size, default 0, not splice\n", "--slice_size=");
    printf("  %-18s = Log slice duration, default 0, not splice\n", "--slice_duration=");
    printf("  %-18s = Log msg info, fmt is [repeat times][log level][msg]\n", "--log_msg");
}

static int parse_msg_info(int index, int argc, char * argv[], cmd_args_t * args)
{
    int ret = index;
    char * buff = argv[index] + 10;
    if (NULL == buff || '[' != buff[0])
    {
        fprintf(stderr, "Unknown msg fmt %s\n", buff);
        return -1;
    }

    int got = 0;
    int flag = 0;
    char * msg = NULL;
    char log_level[4] = { 0 };
    char repeat_times[16] = { 0 };
    const size_t len = strlen(buff);
    size_t pos = 0ul;

    args->mis = (msg_info_t *)realloc(args->mis, sizeof(msg_info_t) * (args->mis_cnt + 1ul));

    for (size_t i = 0ul; i < len; ++i)
    {
        if ('[' == buff[i])
        {
            pos = i;
            got = 1;
        }
        else if (']' == buff[i] && 1 == got)
        {
            if (0 == flag)
                strncpy(repeat_times, buff + pos + 1, i - pos - 1);
            else if (1 == flag)
                strncpy(log_level, buff + pos + 1, i - pos - 1);
            else
            {
                args->mis[args->mis_cnt].msg = (char *)calloc(i - pos, sizeof(char));
                if (NULL == args->mis[args->mis_cnt].msg)
                {
                    fprintf(stderr, "No enough memory\n");
                    return -1;
                }
                strncpy(args->mis[args->mis_cnt].msg, buff + pos + 1, i - pos - 1);
            }
            ++flag;
            got = 0;
        }
    }

    if (3 == flag || 0 == got)
    {
        args->mis[args->mis_cnt].level = atoi(log_level);
        args->mis[args->mis_cnt].times = atoi(repeat_times);
        ++args->mis_cnt;
        return 0;
    }

    return -1;
}

int parse_cmd_args(int argc, char * argv[], cmd_args_t * args)
{
    if (argc <= 1)
    {
        fprintf(stderr, "Input params is empty\n");
        return -1;
    }

    char * log_level = NULL;
    char * slice_size = NULL;
    char * slice_duration = NULL;
    for (int i = 1; i < argc; ++i)
    {
        if (0 == strncmp(argv[i], "--log_path=", 11))
            args->path = _strdup(argv[i] + 11);
        else if (0 == strncmp(argv[i], "--log_level=", 12))
            log_level = argv[i] + 12;
        else if (0 == strncmp(argv[i], "--slice_size=", 13))
            slice_size = argv[i] + 13;
        else if (0 == strncmp(argv[i], "--slice_duration=", 17))
            slice_duration = argv[i] + 17;
        else if (0 == strncmp(argv[i], "--log_msg=", 10))
        {
            if (0 != parse_msg_info(i, argc, argv, args))
            {
                release_cmd_args(args);
                return -1;
            }
        }
        else
        {
            release_cmd_args(args);
            return -1;
        }
    }

    if (NULL != log_level)
    {
        char * end_str = NULL;
        long val = strtol(log_level, &end_str, 10);
        if (NULL != end_str || val < LOG_LEVEL_DEBUG || val > LOG_LEVEL_FATAL)
            fprintf(stderr, "Log level %s is invalid\n", log_level);
        else
            args->level = (LOG_MSG_LEVEL)val;
    }

    if (NULL != slice_size)
    {
        char * end_str = NULL;
        long val = strtol(slice_size, &end_str, 10);
        if (NULL != end_str || val < 0)
            fprintf(stderr, "Slice size %s is invalid, set to 0\n", slice_size);
        else
            args->slice_size = (size_t)val;
    }

    if (NULL != slice_duration)
    {
        char * end_str = NULL;
        long val = strtol(slice_duration, &end_str, 10);
        if (NULL != end_str || val < 0)
            fprintf(stderr, "Slice duration %s is invalid, set to 0\n", slice_duration);
        else
            args->slice_duration = (size_t)val;
    }

    return 0;
}

void release_cmd_args(cmd_args_t * args)
{
    if (NULL == args)
        return;

    if (NULL != args->path)
    {
        free(args->path);
        args->path = NULL;
    }

    for (int i = 0; i < args->mis_cnt; ++i)
    {
        free(args->mis[i].msg);
        args->mis[i].msg = NULL;
    }

    if (NULL != args->mis)
    {
        free(args->mis);
        args->mis = NULL;
    }

    args->mis_cnt = 0ul;
    args->level = LOG_LEVEL_INFO;
    args->slice_size = 0ul;
    args->slice_duration = 0ul;
}
