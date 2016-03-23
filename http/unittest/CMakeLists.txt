set(http_test_src SocketPollTest.cc)

add_executable(http_test ${http_test_src})
target_include_directories(http_test PRIVATE ..)
target_include_directories(http_test PRIVATE ../..)

target_link_libraries(http_test net_util)
target_link_libraries(http_test pthread gtest gtest_main boost_system)
