# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("CoroutineLib")

# Include sub-projects.
add_subdirectory ("CoroutineLib")

if(NOT DEFINED CRLIB_SKIP_TESTS)
add_subdirectory ("CoroutineTest")
ENDIF()