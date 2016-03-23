set(sys_src src/Daemon.cc src/Log.cc src/Logger.cc)

add_library(sys_util ${sys_src})

target_link_libraries(sys_util PRIVATE pthread misc_util thread_util)

target_include_directories(sys_util PRIVATE .)
target_include_directories(sys_util PRIVATE ..)

#IF(test)
add_subdirectory(unittest)
#ENDIF()
