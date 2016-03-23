set(slog_test TestAsynLogging.cc TestCacheTimeStamp.cc TestLogger.cc TestLogStream.cc)

add_executable(slog_test ${slog_test})
target_include_directories(slog_test PRIVATE ..)

target_link_libraries(slog_test slog)
target_link_libraries(slog_test pthread gtest gtest_main boost_system)
