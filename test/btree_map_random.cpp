#include <cassert>
#include <iostream>
#include <limits>
#include <map>
#include <random>
#include <string>
#include "gtest/gtest.h"

#include "btree_map.hpp"

const int NMAX = 10000000;

std::random_device rd;

TEST(RandomInsertDelete, int) {
  ipq::BTreeMap<int, int> btree_map;
  std::map<int, int> map;
  std::uniform_int_distribution<int> op_dist(1, 10);
  std::uniform_int_distribution<int> value_dist(
      std::numeric_limits<int>::min());
  auto check_iter_equal = [&](decltype(btree_map)::iterator &iter1,
                              decltype(map)::iterator &iter2) {
    if (iter2 == map.end()) {
      EXPECT_EQ(iter1, btree_map.end());
    } else {
      EXPECT_NE(iter1, btree_map.end());
      EXPECT_EQ(iter1->first, iter2->first);
      EXPECT_EQ(iter1->second, iter2->second);
    }
  };
  for (int i = 0; i < NMAX; ++i) {
    int op = op_dist(rd);
    int val = value_dist(rd);
    switch (op) {
      case 1: {
        auto res1 = btree_map.erase(val);
        auto res2 = map.erase(val);
        EXPECT_EQ(res1, res2);
      } break;
      case 2: {
        auto iter1 = btree_map.find(val);
        auto iter2 = map.find(val);
        check_iter_equal(iter1, iter2);
      } break;
      case 3: {
        auto iter1 = btree_map.lower_bound(val);
        auto iter2 = map.lower_bound(val);
        check_iter_equal(iter1, iter2);
      } break;
      case 4: {
        break;
        auto iter1 = btree_map.upper_bound(val);
        auto iter2 = map.upper_bound(val);
        check_iter_equal(iter1, iter2);
      } break;
      case 5: {
        auto iter1 = btree_map.find(val);
        auto iter2 = map.find(val);
        check_iter_equal(iter1, iter2);
        if (iter2 != map.end()) {
          iter2 = map.erase(iter2);
          iter1 = btree_map.erase(iter1);
          check_iter_equal(iter1, iter2);
        }
      } break;
      default: {
        int val1 = value_dist(rd);
        std::pair<int, int> ins_val{val, val1};
        auto res1 = btree_map.insert(ins_val);
        auto res2 = map.insert(ins_val);
        EXPECT_EQ(res1.second, res2.second);
        check_iter_equal(res1.first, res2.first);
      }
    }
  }
  {
    if (!map.empty()) {
      int val = map.rbegin()->first + 1;
      auto iter1 = btree_map.lower_bound(val);
      auto iter2 = map.lower_bound(val);
      check_iter_equal(iter1, iter2);
    }
  }
  std::cout << "size after test: " << btree_map.size() << ' ' << map.size()
            << std::endl;
  {
    auto iter1 = btree_map.begin(), iter2 = btree_map.end();
    auto iter3 = map.begin(), iter4 = map.end();
    while (iter3 != iter4) {
      EXPECT_EQ(iter1->first, iter3->first);
      EXPECT_EQ(iter1->second, iter3->second);
      ++iter1;
      ++iter3;
    }
    EXPECT_EQ(iter1, iter2);
  }

  {
    auto iter1 = btree_map.rbegin(), iter2 = btree_map.rend();
    auto iter3 = map.rbegin(), iter4 = map.rend();
    while (iter3 != iter4) {
      EXPECT_EQ(iter1->first, iter3->first);
      EXPECT_EQ(iter1->second, iter3->second);
      ++iter1;
      ++iter3;
    }
    EXPECT_EQ(iter1, iter2);
  }
}

TEST(RandomInsertDelete, string) {
  ipq::BTreeMap<std::string, std::string> btree_map;
  std::map<std::string, std::string> map;
  std::uniform_int_distribution<int> op_dist(1, 10);
  std::uniform_int_distribution<int> value_dist(
      std::numeric_limits<int>::min());
  auto check_iter_equal = [&](decltype(btree_map)::iterator &iter1,
                              decltype(map)::iterator &iter2) {
    if (iter2 == map.end()) {
      EXPECT_EQ(iter1, btree_map.end());
    } else {
      EXPECT_NE(iter1, btree_map.end());
      EXPECT_EQ(iter1->first, iter2->first);
      EXPECT_EQ(iter1->second, iter2->second);
    }
  };
  for (int i = 0; i < NMAX; ++i) {
    int op = op_dist(rd);
    std::string val = std::to_string(value_dist(rd));
    switch (op) {
      case 1: {
        auto res1 = btree_map.erase(val);
        auto res2 = map.erase(val);
        EXPECT_EQ(res1, res2);
      } break;
      case 2: {
        auto iter1 = btree_map.find(val);
        auto iter2 = map.find(val);
        check_iter_equal(iter1, iter2);
      } break;
      case 3: {
        auto iter1 = btree_map.lower_bound(val);
        auto iter2 = map.lower_bound(val);
        check_iter_equal(iter1, iter2);
      } break;
      case 4: {
        auto iter1 = btree_map.upper_bound(val);
        auto iter2 = map.upper_bound(val);
        check_iter_equal(iter1, iter2);
      } break;
      case 5: {
        auto iter1 = btree_map.find(val);
        auto iter2 = map.find(val);
        check_iter_equal(iter1, iter2);
        if (iter2 != map.end()) {
          iter2 = map.erase(iter2);
          iter1 = btree_map.erase(iter1);
          check_iter_equal(iter1, iter2);
        }
      } break;
      default: {
        std::string val1 = std::to_string(value_dist(rd));
        std::pair<std::string, std::string> ins_val(val, val1);
        auto res1 = btree_map.insert(ins_val);
        auto res2 = map.insert(ins_val);
        EXPECT_EQ(res1.second, res2.second);
        check_iter_equal(res1.first, res2.first);
      }
    }
  }
  std::cout << "size after test: " << btree_map.size() << ' ' << map.size()
            << std::endl;
  {
    auto iter1 = btree_map.begin(), iter2 = btree_map.end();
    auto iter3 = map.begin(), iter4 = map.end();
    while (iter3 != iter4) {
      EXPECT_EQ(iter1->first, iter3->first);
      EXPECT_EQ(iter1->second, iter3->second);
      ++iter1;
      ++iter3;
    }
    EXPECT_EQ(iter1, iter2);
  }

  {
    auto iter1 = btree_map.rbegin(), iter2 = btree_map.rend();
    auto iter3 = map.rbegin(), iter4 = map.rend();
    while (iter3 != iter4) {
      EXPECT_EQ(iter1->first, iter3->first);
      EXPECT_EQ(iter1->second, iter3->second);
      ++iter1;
      ++iter3;
    }
    EXPECT_EQ(iter1, iter2);
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
