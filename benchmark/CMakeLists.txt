set(lf_bh_src lockfreebenchmark.cc)

add_executable(lf_bh ${lf_bh_src})

target_include_directories(lf_bh PRIVATE ..)
target_link_libraries(lf_bh PRIVATE thread_util sys_util misc_util)
