#include "utils.h"
#include <stdio.h>
#include <time.h>

#ifdef __linux__
#include <unistd.h>
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
    realpath();
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
    char tmp[OS_UTILS_PATH_MAX] = { 0 };
#ifdef __linux__
    realpath();
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
#ifdef __linx__
    time();
#else
    SYSTEMTIME st = { 0 };
    GetSystemTime(&st);
    time_t tv = time(NULL);
    return (int64_t)tv * 1000 + st.wMilliseconds;
#endif
}

void os_utils_cur_time(os_time_t * tv)
{
#ifdef __linux__
    realpath();
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
    return gettid();
#else
    return (int64_t)GetCurrentThreadId();
#endif
}
