cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 11)

project(os_log LANGUAGES C)

# library
SET(OS_LOG_SRC
    ./src/utils.c
    ./src/queue.c
	./src/log.c)

IF (CMAKE_HOST_WIN32)
    ADD_COMPILE_DEFINITIONS(HAVE_STRUCT_TIMESPEC=1)
    ADD_COMPILE_DEFINITIONS(LOG_API_EXPORT=1)
    SET(OS_LOG_INC
        ./deps/include
	    ./include)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4) # for 32-bit systems
	    SET(OS_LOG_DEP_LIBS_PATH "./deps/lib/x86")
    else() # for 64-bit systems
        SET(OS_LOG_DEP_LIBS_PATH "./deps/lib/x64")
    endif()
    link_directories(${OS_LOG_DEP_LIBS_PATH})
    SET(OS_LOG_DEP_LIBS pthreadVC2)
ELSE()
    SET(OS_LOG_INC ./include)
    SET(OS_LOG_DEP_LIBS pthread)
ENDIF()

INCLUDE_DIRECTORIES (${OS_LOG_INC})

ADD_LIBRARY (os_log SHARED ${OS_LOG_SRC})

TARGET_LINK_LIBRARIES(os_log ${OS_LOG_DEP_LIBS})

OPTION (OS_LOG_WITH_TEST_PROGRAMS "build with test programs" OFF)
IF(OS_LOG_WITH_TEST_PROGRAMS)
    ADD_EXECUTABLE (os_log_test "examples/test.c")
    TARGET_LINK_LIBRARIES (os_log_test
                           os_log
                           ${OS_LOG_DEP_LIBS})
ENDIF ()

# Add tests and install targets if needed.
INSTALL (FILES "include/log.h" DESTINATION include)
INSTALL (TARGETS os_log DESTINATION lib)
IF (OS_LOG_WITH_TEST_PROGRAMS)
    INSTALL (TARGETS os_log_test DESTINATION bin)
ENDIF ()
