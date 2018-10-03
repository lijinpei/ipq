add_executable (btree_ipq ipq.cpp)
add_executable (stl_ipq ipq.cpp)
target_compile_definitions(btree_ipq PRIVATE BTREE)
