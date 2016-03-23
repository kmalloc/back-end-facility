set(slog_src AsynLogAppender.cc CacheTimeStamp.cc LogFileAppender.cc Logger.cc LogStream.cc)

add_library(slog ${slog_src})
target_include_directories(slog PRIVATE .)
target_include_directories(slog PRIVATE ..)

target_link_libraries(slog PRIVATE boost_thread)

add_subdirectory(test)
add_subdirectory(samples)
