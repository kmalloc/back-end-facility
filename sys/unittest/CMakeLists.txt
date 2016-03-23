set(sys_src LoggerTest.cc)

add_executable(sys_test ${sys_src})
target_include_directories(sys_test PRIVATE ..)
target_include_directories(sys_test PRIVATE ../..)

target_link_libraries(sys_test sys_util thread_util misc_util)
target_link_libraries(sys_test pthread gtest gtest_main boost_system)
