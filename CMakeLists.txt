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
    fix_client_md_f1
    fix_client_md_f2
    fix_client_md_f3
    fix_client_md_f4
    fix_client_md_f5
    fix_client_md_t1
    fix_client_md_t2
    fix_client_md_t3
    fix_client_trade1
    fix_client_trade2
    fix_om_1
    fix_om_2
    fix_om_3
    fix_om_4
    fix_om_5
    fix_om_6
    fix_om_7
    fix_om_8
    fix_om_9
    fix_om_10
    fix_om_11
    fix_om_12
    fix_om_13
    fix_om_14
    fix_om_15
    fix_om_16
    fix_om_17
    fix_sm_1a
    fix_sm_1b
    fix_sm_3a
    fix_sm_3b
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

foreach(test ${practice})
  add_executable(${test} ${test}.cpp)
  target_link_libraries(${test} quickfix)
endforeach()
