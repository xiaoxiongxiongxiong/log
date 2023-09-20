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
        for (int j = 0; j < args.mis[i].times; ++j)
        {
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

}

int parse_cmd_args(int argc, char * argv[], cmd_args_t * args)
{
    if (argc <= 1)
    {
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

        }
        else
        {
            release_cmd_args(args);
            return -1;
        }

    }

    if (NULL != lo)

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

}
