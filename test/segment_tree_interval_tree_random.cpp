#include "btree_map.hpp"
#include "segment_tree.hpp"
#include "segment_tree_traits.hpp"
#include "interval_tree.hpp"

#include "gtest/gtest.h"
#include <cstdint>
#include <limits>
#include <map>
#include <random>

int NMAX = 10000;

std::random_device rd;

TEST(IntervalOperations, SegmentTree) {
  using T = uint16_t;
//  ipq::SegmentTree<T> seg_tree(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
  ipq::IntervalTree<T, T, std::map<T, std::pair<T, T>>> stl_int_tree;
  ipq::IntervalTree<T, T, ipq::BTreeMap<T, std::pair<T, T>>> btree_int_tree;
  std::uniform_int_distribution<int> op_dist(1, 10);
  std::uniform_int_distribution<T> value_dist(
      std::numeric_limits<T>::min());
  for (int i = 0; i < NMAX; ++i) {
    int op  = op_dist(rd);
    std::cout << "op: " << op << std::endl;
    switch(op) {
    case 1: {
      T key1 = value_dist(rd), key2 = value_dist(rd);
      if (key1 > key2) {
        std::swap(key1, key2);
      }
//      seg_tree.remove(key1, key2);
      stl_int_tree.remove(key1, key2);
      btree_int_tree.remove(key1, key2);
    }break;
    case 2: {
      T key = value_dist(rd);
//      auto* res1 = seg_tree.find(key);
      auto* res2 = stl_int_tree.find(key);
      auto* res3 = btree_int_tree.find(key);
      if (!res3) {
        EXPECT_EQ(res2, nullptr);
        EXPECT_EQ(res3, nullptr);
      } else {
        EXPECT_EQ(*res2, *res3);
      }
    }break;
    default: {
      T key1 = value_dist(rd), key2 = value_dist(rd);
      if (key1 > key2) {
        std::swap(key1, key2);
      }
      T val = value_dist(rd);
//      seg_tree.update(key1, key2, val);
      stl_int_tree.update(key1, key2, val);
      btree_int_tree.update(key1, key2, val);
    }
    }
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
