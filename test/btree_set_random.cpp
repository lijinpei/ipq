#include <cassert>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <string>
#include "gtest/gtest.h"

#include "btree_set.hpp"

const int NMAX = 10000000;

std::random_device rd;

TEST(RandomInsertDelete, int) {
  ipq::BTreeSet<int> btree_set;
  std::set<int> set;
  std::uniform_int_distribution<int> op_dist(1, 10);
  std::uniform_int_distribution<int> value_dist(
      std::numeric_limits<int>::min());
  auto check_iter_equal = [&](decltype(btree_set)::iterator &iter1,
                              decltype(set)::iterator &iter2) {
    if (iter2 == set.end()) {
      EXPECT_EQ(iter1, btree_set.end());
    } else {
      EXPECT_NE(iter1, btree_set.end());
      EXPECT_EQ(*iter1, *iter2);
    }
  };
  for (int i = 0; i < NMAX; ++i) {
    int op = op_dist(rd);
    int val = value_dist(rd);
    switch (op) {
      case 1: {
        auto res1 = btree_set.erase(val);
        auto res2 = set.erase(val);
        EXPECT_EQ(res1, res2);
      } break;
      case 2: {
        auto iter1 = btree_set.find(val);
        auto iter2 = set.find(val);
        check_iter_equal(iter1, iter2);
      } break;
      case 3: {
        auto iter1 = btree_set.lower_bound(val);
        auto iter2 = set.lower_bound(val);
        check_iter_equal(iter1, iter2);
      } break;
      case 4: {
        auto iter1 = btree_set.upper_bound(val);
        auto iter2 = set.upper_bound(val);
        check_iter_equal(iter1, iter2);
      } break;
      case 5: {
        auto iter1 = btree_set.find(val);
        auto iter2 = set.find(val);
        check_iter_equal(iter1, iter2);
        if (iter2 != set.end()) {
          iter2 = set.erase(iter2);
          iter1 = btree_set.erase(iter1);
          check_iter_equal(iter1, iter2);
        }
      } break;
      default: {
        auto res1 = btree_set.insert(val);
        auto res2 = set.insert(val);
        EXPECT_EQ(res1.second, res2.second);
        check_iter_equal(res1.first, res2.first);
      }
    }
    {
      if (!set.empty()) {
        int val = *set.rbegin() + 1;
        auto iter1 = btree_set.lower_bound(val);
        auto iter2 = set.lower_bound(val);
        check_iter_equal(iter1, iter2);
      }
    }
  }
  std::cout << "size after test: " << btree_set.size() << ' ' << set.size()
            << std::endl;
  {
    auto iter1 = btree_set.begin(), iter2 = btree_set.end();
    auto iter3 = set.begin(), iter4 = set.end();
    while (iter3 != iter4) {
      EXPECT_EQ(*iter1, *iter3);
      ++iter1;
      ++iter3;
    }
    EXPECT_EQ(iter1, iter2);
  }

  {
    auto iter1 = btree_set.rbegin(), iter2 = btree_set.rend();
    auto iter3 = set.rbegin(), iter4 = set.rend();
    while (iter3 != iter4) {
      EXPECT_EQ(*iter1, *iter3);
      ++iter1;
      ++iter3;
    }
    EXPECT_EQ(iter1, iter2);
  }
}

TEST(RandomInsertDelete, string) {
  return;
  ipq::BTreeSet<std::string> btree_set;
  std::set<std::string> set;
  std::uniform_int_distribution<int> op_dist(1, 10);
  std::uniform_int_distribution<int> value_dist(
      std::numeric_limits<int>::min());
  auto check_iter_equal = [&](decltype(btree_set)::iterator &iter1,
                              decltype(set)::iterator &iter2) {
    if (iter2 == set.end()) {
      EXPECT_EQ(iter1, btree_set.end());
    } else {
      EXPECT_NE(iter1, btree_set.end());
      EXPECT_EQ(*iter1, *iter2);
    }
  };
  for (int i = 0; i < NMAX; ++i) {
    int op = op_dist(rd);
    std::string val = std::to_string(value_dist(rd));
    switch (op) {
      case 1: {
        auto res1 = btree_set.erase(val);
        auto res2 = set.erase(val);
        EXPECT_EQ(res1, res2);
      } break;
      case 2: {
        auto iter1 = btree_set.find(val);
        auto iter2 = set.find(val);
        check_iter_equal(iter1, iter2);
      } break;
      case 3: {
        auto iter1 = btree_set.lower_bound(val);
        auto iter2 = set.lower_bound(val);
        check_iter_equal(iter1, iter2);
      } break;
      case 4: {
        auto iter1 = btree_set.upper_bound(val);
        auto iter2 = set.upper_bound(val);
        check_iter_equal(iter1, iter2);
      } break;
      case 5: {
        auto iter1 = btree_set.find(val);
        auto iter2 = set.find(val);
        check_iter_equal(iter1, iter2);
        if (iter2 != set.end()) {
          iter2 = set.erase(iter2);
          iter1 = btree_set.erase(iter1);
          check_iter_equal(iter1, iter2);
        }
      } break;
      default:
        auto res1 = btree_set.insert(val);
        auto res2 = set.insert(val);
        EXPECT_EQ(res1.second, res2.second);
        check_iter_equal(res1.first, res2.first);
    }
  }
  std::cout << "size after test: " << btree_set.size() << ' ' << set.size()
            << std::endl;
  {
    auto iter1 = btree_set.begin(), iter2 = btree_set.end();
    auto iter3 = set.begin(), iter4 = set.end();
    while (iter3 != iter4) {
      EXPECT_EQ(*iter1, *iter3);
      ++iter1;
      ++iter3;
    }
    EXPECT_EQ(iter1, iter2);
  }

  {
    auto iter1 = btree_set.rbegin(), iter2 = btree_set.rend();
    auto iter3 = set.rbegin(), iter4 = set.rend();
    while (iter3 != iter4) {
      EXPECT_EQ(*iter1, *iter3);
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
