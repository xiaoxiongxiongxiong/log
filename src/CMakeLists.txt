cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

set(OS_LOG_SRC
    log.c
    queue.c)

if(CMAKE_HOST_WIN32)
    add_compile_definitions(HAVE_STRUCT_TIMESPEC=1)
    add_compile_definitions(OS_LOG_API_EXPORT=1)
endif()

set(OS_LOG_INC
    ${OS_LOG_DEP_INC}
    ${PROJECT_SOURCE_DIR}/include)

include_directories(${OS_LOG_INC}
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/utils/include
)
add_library(os_log SHARED ${OS_LOG_SRC})
target_link_libraries(os_log ${OS_LOG_DEP_LIBS}
    os_utils
)

install(TARGETS os_log
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)
