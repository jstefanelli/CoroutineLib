# CMakeList.txt : CMake project for CoroutineTest, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.20)

# Add source to this project's executable.
add_executable (CoroutineTest "CoroutineTest.cpp" "CoroutineTest.h" "cc_queue.h" "cc_task.h" "cc_task_scheduler.h" "cc_thread_pool.h" "cc_thread_pool.cpp")


set_property (TARGET CoroutineTest PROPERTY CXX_STANDARD 20)
set_property (TARGET CoroutineTest PROPERTY CXX_STANDARD_REQUIRED TRUE)
# TODO: Add tests and install targets if needed.
