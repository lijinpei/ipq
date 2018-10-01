cmake_minimum_required (VERSION 3.12)
project (ipq)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_CXX_FLAGS "-W -Wall -Wextra -Wpedantic")

include_directories (include)
add_subdirectory (test)
