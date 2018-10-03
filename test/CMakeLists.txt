enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

set (test_list "")

macro(my_add_test name)
add_executable(${name} "${name}.cpp")
list(APPEND test_list ${name})
target_link_libraries(${name} ${GTEST_BOTH_LIBRARIES})
SET_TARGET_PROPERTIES(${name} PROPERTIES COMPILE_FLAGS "-O3")
add_test(${name} ${name})
endmacro()

my_add_test(btree_set_sequential)
my_add_test(btree_map_sequential)
my_add_test(btree_set_random)
my_add_test(btree_map_random)
my_add_test(segment_tree_interval_tree_random)

add_custom_target(test ctest -j 4 DEPENDS ${test_list})
