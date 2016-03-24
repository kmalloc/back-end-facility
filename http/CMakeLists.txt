set(net_src HttpBuffer.cc HttpClient.cc HttpServer.cc SocketPoll.cc SocketServer.cc)

add_library(net_util ${net_src})
add_executable(http main.cc)

target_include_directories(http PRIVATE ..)
target_include_directories(net_util PRIVATE ..)

target_link_libraries(http PRIVATE net_util)
target_link_libraries(net_util PRIVATE thread_util sys_util misc_util)

IF(test)
    add_subdirectory(unittest)
ENDIF()
