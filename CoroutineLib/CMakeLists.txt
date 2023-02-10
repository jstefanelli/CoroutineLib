# CMakeList.txt : CMake project for CoroutineTest, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.20)

# Add source to this project's executable.
add_library (CoroutineLib SHARED "include/crlib/cc_api.h" "include/crlib/cc_task.h" "include/crlib/cc_task_scheduler.h" "include/crlib/cc_thread_pool.h" "cc_thread_pool.cpp"   "cc_task_scheduler.cpp" include/crlib/cc_queue_2.h include/crlib/cc_dictionary.h include/crlib/cc_generic_queue.h include/crlib/cc_generator_task.h include/crlib/cc_task_locks.h include/crlib/cc_task_types.h include/crlib/cc_awaitables.h include/crlib/cc_base_promise.h include/crlib/cc_sync_utils.h include/crlib/cc_boundless_queue.h include/crlib/cc_queue_config.h include/crlib/cc_base_queue.h include/crlib/cc_value_task.h)
target_compile_definitions(CoroutineLib PRIVATE CRLIB_EXPORTS)
target_include_directories(CoroutineLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(CoroutineLib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/crlib)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

else()
target_compile_options(CoroutineLib PRIVATE -Wall -Wpedantic)
target_compile_options(CoroutineLib PUBLIC -fconcepts-diagnostics-depth=4)
endif()

set_property (TARGET CoroutineLib PROPERTY CXX_STANDARD 20)
set_property (TARGET CoroutineLib PROPERTY CXX_STANDARD_REQUIRED TRUE)
# TODO: Add tests and install targets if needed.
