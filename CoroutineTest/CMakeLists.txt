# CMakeList.txt : CMake project for CoroutineTest, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.20)

add_executable (QueueTest "QueueTest.cpp")
target_link_libraries(QueueTest "CoroutineLib")
set_property (TARGET QueueTest PROPERTY CXX_STANDARD 20)
set_property (TARGET QueueTest PROPERTY CXX_STANDARD_REQUIRED TRUE)

# Add source to this project's executable.
add_executable (CoroutineTest "CoroutineTest.cpp" "CoroutineTest.h")
target_link_libraries(CoroutineTest "CoroutineLib")

set_property (TARGET CoroutineTest PROPERTY CXX_STANDARD 20)
set_property (TARGET CoroutineTest PROPERTY CXX_STANDARD_REQUIRED TRUE)

include(CTest)

add_test(NAME QueueTest_Generic COMMAND QueueTest)
add_test(NAME QueueTest_NoMultiRead COMMAND QueueTest --test-no-multi-read)

add_test(NAME CoroutineTest COMMAND CoroutineTest)

