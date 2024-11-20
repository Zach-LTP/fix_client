cmake_minimum_required(VERSION 3.9)
project(practice VERSION 1.0.0)
enable_testing()

set(CMAKE_CXX_STANDARD 17)
message("using C++17 standard...")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -pthread")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

include_directories(/usr/local/include/)
link_directories(/usr/local/lib/)

set(practice
    fix_client_1
    fix_client_2
    fix_client_3
    fix_client_4
    fix_client_5
    fix_client_t1
    fix_client_t2
    fix_client_t3
    fix_client_t4
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

foreach(test ${practice})
  add_executable(${test} ${test}.cpp)
  target_link_libraries(${test} quickfix)
endforeach()
