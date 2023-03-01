# CMakeList.txt : CMake project for CoroutineTest, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.20)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/contrib/picobench)

add_executable (QueueTest "QueueTest.cpp")
target_link_libraries(QueueTest "CoroutineLib")
set_property (TARGET QueueTest PROPERTY CXX_STANDARD 20)
set_property (TARGET QueueTest PROPERTY CXX_STANDARD_REQUIRED TRUE)


add_custom_command(TARGET QueueTest
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:CoroutineLib> "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Copy dll file to ${CMAKE_CURRENT_BINARY_DIR} directory" VERBATIM
)

# Add source to this project's executable.
add_executable (CoroutineTest "CoroutineTest.cpp" "CoroutineTest.h")
target_link_libraries(CoroutineTest "CoroutineLib")

set_property (TARGET CoroutineTest PROPERTY CXX_STANDARD 20)
set_property (TARGET CoroutineTest PROPERTY CXX_STANDARD_REQUIRED TRUE)


add_custom_command(TARGET CoroutineTest
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:CoroutineLib> "${CMAKE_CURRENT_BINARY_DIR}"
		COMMENT "Copy dll file to ${CMAKE_CURRENT_BINARY_DIR} directory" VERBATIM
		)

add_executable (SchedulerTest "SchedulerTest.cpp")
target_link_libraries(SchedulerTest "CoroutineLib")

set_property (TARGET SchedulerTest PROPERTY CXX_STANDARD 20)
set_property (TARGET SchedulerTest PROPERTY CXX_STANDARD_REQUIRED TRUE)


add_custom_command(TARGET SchedulerTest
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:CoroutineLib> "${CMAKE_CURRENT_BINARY_DIR}"
		COMMENT "Copy dll file to ${CMAKE_CURRENT_BINARY_DIR} directory" VERBATIM
		)

include(CTest)

add_executable(Benchmark "benchmark.cpp")
target_link_libraries(Benchmark "CoroutineLib" "picobench")
set_property (TARGET Benchmark PROPERTY CXX_STANDARD 20)
set_property (TARGET Benchmark PROPERTY CXX_STANDARD_REQUIRED TRUE)


add_test(NAME QueueTest COMMAND QueueTest)

add_test(NAME CoroutineTest COMMAND CoroutineTest)
add_test(NAME CoroutineTest_AsyncMutex COMMAND CoroutineTest --test-async-mutex)

add_test(NAME SchedulerTest COMMAND SchedulerTest)