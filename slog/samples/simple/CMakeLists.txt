set(slog_simple main.cc)

add_executable(slog_simple ${slog_simple})
target_include_directories(slog_simple PRIVATE ../..)

target_link_libraries(slog_simple slog)
