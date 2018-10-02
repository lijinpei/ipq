#include "btree_map.hpp"
#include "segment_tree.hpp"
#include "segment_tree_traits.hpp"
#include "interval_tree.hpp"

#include "gtest/gtest.h"
#include <cstdint>
#include <limits>
#include <map>
#include <random>

int NMAX = 100000;

std::random_device rd;

TEST(IntervalOperations, SegmentTree) {
  using T = uint16_t;
  ipq::SegmentTree<T> seg_tree(std::numeric_limits<T>::min(), std::numeric_limits<T>::max());
  seg_tree.update(43730, 63500, 32742);
  seg_tree.update(41059, 64411, 46513);
  seg_tree.find(40960);
  ipq::IntervalTree<T, T, std::map<T, std::pair<T, T>>> stl_int_tree;
  ipq::IntervalTree<T, T, ipq::BTreeMap<T, std::pair<T, T>>> btree_int_tree;
  std::uniform_int_distribution<int> op_dist(1, 10);
  std::uniform_int_distribution<T> value_dist(
      std::numeric_limits<T>::min(), std::numeric_limits<T>::max() - T(2));
  for (int i = 0; i < NMAX; ++i) {
    int op  = op_dist(rd);
    switch(op) {
    case 1: {
      break;
      T key1 = value_dist(rd), key2 = value_dist(rd);
      std::cout << "remove: " << key1 << ' ' << key2 << std::endl;
      if (key1 > key2) {
        std::swap(key1, key2);
      }
      seg_tree.remove(key1, key2);
      stl_int_tree.remove(key1, key2);
      btree_int_tree.remove(key1, key2);
    }break;
    case 2: {
      //T key = value_dist(rd);
      for (int key_ = std::numeric_limits<T>::min(); key_ <= std::numeric_limits<T>::max(); ++key_) {
        T key = key_;
      auto* res1 = seg_tree.find(key);
      auto* res2 = stl_int_tree.find(key);
      auto* res3 = btree_int_tree.find(key);
      if (!res2) {
        EXPECT_EQ(res1, nullptr);
        if (res1) {
      std::cout << "all intervals:" << std::endl;
      for (auto kv : stl_int_tree.keys) {
        std::cout << kv.first << ' ' << kv.second.first << std::endl;
      }
          std::cout << int(*res1) << std::endl;
          std::cout << "find: " << key << std::endl;
          abort();
        }
        EXPECT_EQ(res3, nullptr);
      } else {
        EXPECT_NE(res1, nullptr);
        if (res1 == nullptr) {
          std::cout << "find: " << key << std::endl;
          abort();
        }
        EXPECT_NE(res3, nullptr);
        EXPECT_EQ(*res2, *res1);
        if (*res1 != *res2) {
          std::cout << "find: " << key << std::endl;
          abort();
        }
        EXPECT_EQ(*res2, *res3);
      }
      }
    }break;
    default: {
      T key1 = value_dist(rd), key2 = value_dist(rd);
      if (key1 > key2) {
        std::swap(key1, key2);
      }
      T val = value_dist(rd);
      std::cout << "insert: " << key1 << ' ' << key2 << ' ' << val << std::endl;
      seg_tree.update(key1, key2, val);
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
