set(thread_src src/Thread.cc src/ThreadPool.cc src/Worker.cc src/WorkerBodyBase.cc)

add_library(thread_util ${thread_src})

target_link_libraries(thread_util PRIVATE sys_util)
target_include_directories(thread_util PRIVATE .)
target_include_directories(thread_util PRIVATE ..)

IF(test)
    add_subdirectory(unittest)
ENDIF()
