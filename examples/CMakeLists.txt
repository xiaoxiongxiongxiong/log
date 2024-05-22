cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

# 源码
set(OS_LOG_TEST_SRC
    test.c)

# os_log_test
add_executable(os_log_test ${OS_LOG_TEST_SRC})
target_include_directories(os_log_test PRIVATE 
                           ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(os_log_test PUBLIC 
                      os_log
                      ${OS_LOG_DEP_LIBS})

# install
install(TARGETS os_log_test DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)