set(misc_src src/LockFreeBuffer.cc src/LockFreeList.cc src/PerThreadMemory.cc)

add_library(misc_util ${misc_src})

target_compile_options(misc_util PRIVATE -mcx16)

target_link_libraries(misc_util PUBLIC rt)
target_include_directories(misc_util PRIVATE .)
target_include_directories(misc_util PRIVATE ..)

#IF(test)
    add_subdirectory(unittest)
#ENDIF()
