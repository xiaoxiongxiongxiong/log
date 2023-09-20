#include "utils.h"

#ifdef __linux__
#include <unistd.h>
#else
#define F_OK 0
#include <io.h>
#endif

bool os_utils_work_path(char * path, size_t len)
{
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
    return true;
}

os_time_t * os_utils_time(os_time_t * tv)
{
    return tv;
}

int64_t os_utils_time_ms()
{
    return 0;
}
