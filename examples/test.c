#include "log.h"
#include <stdlib.h>

int main()
{
    const char * path = "os_log.log";
    if (0 != log_msg_init(path, LOG_LEVEL_INFO))
    {
        printf("log_msg_init failed\n");
        return 1;
    }
    atexit(log_msg_uninit);

    for (int i = 0; i < 100; ++i)
        log_msg_info("%" PRId32 " Hello World!", i);

    return 0;
}
