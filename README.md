# ipq: a ip location query toy
## how to build
```
mkdir build
cd build
cmake ..
make
```
## how to use
first, download the csv file here https://pan.baidu.com/s/1DMxqf8GvzP7G7x9k4P5zrA (originally from: https://lite.ip2location.com/download?id=3), then
```
cd build
src/stl_ipq path/to/your/IP2LOCATION-LITE-DB3.CSV
```


ipq has three commands, type the commands at stdin, one command per line.
```
query ip
delete ip1 ip2
update ip1 ip2 country_code country_name province city
```
Ip could be a decimal number, or something like 127.0.0.1(no verification of the ip address given is performed, so...). Query command queries the location of the ip address. Delete command deletes the information of the ip range, both ip1 and ip2 included. Update command updates data base for the ip range, both ip1 and ip2 included

## implementation details
To support range update and point query, ipq uses three kinds of datastructures:
```
interval-tree: include/interval_tree.hpp
segment-tree: include/segment_tree.hpp
btree: include/btree_{map, set, impl}.hpp
```

The interval tree counld be implemented on std::map or ipq::BTreeMap. The segment-tree is simple in that it only supports non-incremental range update and point-query, also it need two special value in the value space to represents non-existing value and that range not marked as the same. The btree algorithm is from chapter 18 of CLRS, it supports insert/delte/find and find next/prev node of a iterator. **One important fact about btree is that , in contrast to rb-tree, any operations that modifies the btree (for example, insert/delete, or reform the btree during find) will invalidate all existing iterator.**

You can find the test cases for btree/interval-tree/segment-tree at test/ directories, run them with:
```
cd build
ninja test
```

You can benchmark the btree and stl rbtree with files in the benchmark directory. You need to compile it yourself with google benchmark. On my laptop pc, btree starts to out-perform std rb-tree when there are around 65536 node in the tree.

## TODO
1. more corner-case test for btree
2. implement empalce for btree_map
3. more comments
