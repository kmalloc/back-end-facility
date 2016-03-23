set(misc_src LockFreeBufferTest.cc PerThreadMemoryTest.cc SpinlockQueueTest.cc testFunctor.cc)

add_executable(misc_test ${misc_src})
target_include_directories(misc_test PRIVATE ..)
target_include_directories(misc_test PRIVATE ../..)

target_link_libraries(misc_test misc_util thread_util)
target_link_libraries(misc_test pthread gtest gtest_main boost_system)
