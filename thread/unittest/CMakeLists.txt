set(thread_src ThreadPoolTest.cc ThreadTest.cc WorkerTest.cc)

add_executable(thread_test ${thread_src})
target_include_directories(thread_test PRIVATE ..)
target_include_directories(thread_test PRIVATE ../..)

target_link_libraries(thread_test thread_util sys_util misc_util)
target_link_libraries(thread_test pthread gtest gtest_main boost_system)
