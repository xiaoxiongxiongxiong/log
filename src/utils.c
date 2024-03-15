#include "utils.h"
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __linux__
#include <unistd.h>
#include <sys/time.h>
#include <libgen.h>
#include <sys/syscall.h>
#else
#define F_OK 0
#include <windows.h>
#include <time.h>
#include <io.h>
#endif

bool os_utils_work_path(char * path, size_t len)
{
    char tmp[OS_UTILS_PATH_MAX] = { 0 };
#ifdef __linux__
    ssize_t bytes = readlink("/proc/self/exe", tmp, PATH_MAX);
    if (-1 == bytes)
    {
        const int code = errno;
        printf("readlink failed for %s.\n", strerror(code));
        return NULL;
}

    if (NULL != path && len > 0)
    {
        memcpy(path, dirname(tmp), len);
        path[len - 1] = '\0';
    }
#else
    GetModuleFileNameA(NULL, tmp, OS_UTILS_PATH_MAX);
    char drive[16] = { 0 };
    char dir[OS_UTILS_PATH_MAX] = { 0 };
    _splitpath(tmp, drive, dir, NULL, NULL);
    snprintf(path, len, "%s%s", drive, dir);
#endif
    return true;
}

bool os_utils_create_directory(const char * path)
{
    return true;
}

bool os_utils_file_exist(const char * path)
{
    int ret = -1;
#ifdef __linux__
    ret = access(path, F_OK);
#else
    ret = _access(path, F_OK);
#endif

    return 0 == ret;
}

bool os_utils_file_name(const char * path, char * file, size_t len)
{
#ifdef __linux__
    char tmp[OS_UTILS_PATH_MAX] = { 0 };
    strncpy(tmp, path, OS_UTILS_PATH_MAX);
    strncpy(file, basename(tmp), len);
#else
    char ext[16] = { 0 };
    char name[OS_UTILS_PATH_MAX] = { 0 };
    _splitpath(path, NULL, NULL, name, ext);
    snprintf(file, len, "%s%s", name, ext);
#endif
    return true;
}

int64_t os_utils_time_ms()
{
#ifdef __linux__
    struct timeval tv = { 0 };
    gettimeofday(&tv, NULL);
    return (int64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
#else
    SYSTEMTIME st = { 0 };
    GetSystemTime(&st);
    time_t tv = time(NULL);
    return (int64_t)tv * 1000 + st.wMilliseconds;
#endif
}

void os_utils_time(os_time_t * tv)
{
#ifdef __linux__
    int64_t ts = os_utils_time_ms();
    time_t val = (time_t)(ts / 1000);
    struct tm * tmp = localtime(&val);
    tv->year = tmp->tm_year + 1900;
    tv->month = tmp->tm_mon + 1;
    tv->day = tmp->tm_mday;
    tv->hour = tmp->tm_hour;
    tv->minute = tmp->tm_min;
    tv->second = tmp->tm_sec;
    tv->millisecond = ts % 1000;
#else
    SYSTEMTIME st = { 0 };
    GetSystemTime(&st);
    tv->year = st.wYear;
    tv->month = st.wMonth;
    tv->day = st.wDay;
    tv->hour = st.wHour;
    tv->minute = st.wMinute;
    tv->second = st.wSecond;
    tv->millisecond = st.wMilliseconds;
#endif
}

int64_t os_utils_tid()
{
#ifdef __linux__
    return (int64_t)syscall(SYS_gettid);
#else
    return (int64_t)GetCurrentThreadId();
#endif
}
