# CMakeList.txt : CMake project for CoroutineTest, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.20)

# Add source to this project's executable.
add_library (CoroutineLib SHARED "include/crlib/cc_api.h" "include/crlib/cc_queue.h" "include/crlib/cc_task.h" "include/crlib/cc_task_scheduler.h" "include/crlib/cc_thread_pool.h" "cc_thread_pool.cpp"   "cc_task_scheduler.cpp")
target_compile_definitions(CoroutineLib PRIVATE CRLIB_EXPORTS)
target_include_directories(CoroutineLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(CoroutineLib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/crlib)

set_property (TARGET CoroutineLib PROPERTY CXX_STANDARD 20)
set_property (TARGET CoroutineLib PROPERTY CXX_STANDARD_REQUIRED TRUE)
# TODO: Add tests and install targets if needed.
