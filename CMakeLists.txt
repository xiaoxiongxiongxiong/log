cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 11)

project(os_log LANGUAGES C)

if(CMAKE_HOST_WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    set(OS_LOG_DEP_INC ${PROJECT_SOURCE_DIR}/deps/include)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4) # for 32-bit systems
	    set(OS_LOG_DEP_LIBS_PATH "${PROJECT_SOURCE_DIR}/deps/lib/x86")
    else() # for 64-bit systems
        set(OS_LOG_DEP_LIBS_PATH "${PROJECT_SOURCE_DIR}/deps/lib/x64")
    endif()
    link_directories(${OS_LOG_DEP_LIBS_PATH})
    set(OS_LOG_DEP_LIBS pthreadVC2)
else()
    set(OS_LOG_DEP_LIBS pthread)
endif()

add_subdirectory(src)

option(OS_LOG_WITH_TEST_PROGRAMS "build with test programs" OFF)
if(OS_LOG_WITH_TEST_PROGRAMS)
    message("build with example")
    add_subdirectory(examples)
endif()
