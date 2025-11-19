cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

set(OS_LOG_SRC
    log.c)

if(CMAKE_HOST_WIN32)
    add_compile_definitions(HAVE_STRUCT_TIMESPEC=1)
    add_compile_definitions(OS_LOG_API_EXPORT=1)
endif()

set(OS_LOG_INC
    ${OS_LOG_DEP_INC}
    ${PROJECT_SOURCE_DIR}/include)

include_directories(${OS_LOG_INC}
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/utils/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libos/include
)
add_library(os_log SHARED ${OS_LOG_SRC})
target_link_libraries(os_log ${OS_LOG_DEP_LIBS}
    os_utils
    libos_queue
)

# install
install(FILES "${PROJECT_SOURCE_DIR}/include/log.h" DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
install(TARGETS os_log DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)