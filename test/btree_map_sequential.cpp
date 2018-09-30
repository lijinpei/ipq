#include <map>
#include <string>
#include "btree_map.hpp"
#include "gtest/gtest.h"

const int NMAX = 20000;

TEST(SequentialInsertDelete, int) {
  ipq::BTreeMap<int, int> btree_map;
  for (int i = 0; i < NMAX; ++i) {
    btree_map.insert(std::make_pair(i, i));
    EXPECT_EQ(btree_map.size(), i + 1);
  }
  {
    int i = 0;
    for (auto iter1 = btree_map.begin(), iter2 = btree_map.end();
         iter1 != iter2; ++iter1) {
      EXPECT_EQ(iter1->second, i++);
    }
    i = NMAX - 1;
    for (auto iter1 = btree_map.rbegin(), iter2 = btree_map.rend();
         iter1 != iter2; ++iter1) {
      EXPECT_EQ(iter1->second, i--);
      if (i < 0) {
        break;
      }
    }
  }
  for (int i = 0; i < NMAX; ++i) {
    btree_map.erase(i);
    EXPECT_EQ(btree_map.size(), NMAX - i - 1);
  }
}

TEST(SequentialInsertDelete, string) {
  ipq::BTreeMap<std::string, int> btree_map;
  std::map<std::string, int> map;
  for (int i = 0; i < NMAX; ++i) {
    std::string str = std::to_string(i);
    auto value = std::make_pair(str, i);
    btree_map.insert(value);
    EXPECT_EQ(btree_map.size(), i + 1);
    map.insert(value);
  }
  {
    auto iter1 = btree_map.begin(), iter2 = btree_map.end();
    auto iter3 = map.begin(), iter4 = map.end();
    for (; iter3 != iter4; ++iter3, ++iter1) {
      EXPECT_EQ(iter1->first, iter3->first);
      EXPECT_EQ(iter1->second, iter3->second);
    }
    EXPECT_EQ(iter1, iter2);
  }
  {
    auto iter1 = btree_map.rbegin(), iter2 = btree_map.rend();
    auto iter3 = map.rbegin(), iter4 = map.rend();
    for (; iter3 != iter4; ++iter3, ++iter1) {
      EXPECT_EQ(iter1->first, iter3->first);
      EXPECT_EQ(iter1->second, iter3->second);
    }
    EXPECT_EQ(iter1, iter2);
  }
  for (int i = 0; i < NMAX; ++i) {
    std::string str = std::to_string(i);
    btree_map.erase(str);
    map.erase(str);
    if (i * i > 0 && i * i < NMAX) {
      auto iter1 = btree_map.begin(), iter2 = btree_map.end();
      auto iter3 = map.begin(), iter4 = map.end();
      for (; iter3 != iter4; ++iter3, ++iter1) {
        EXPECT_EQ(iter1->first, iter3->first);
        EXPECT_EQ(iter1->second, iter3->second);
      }
      EXPECT_EQ(iter1, iter2);
    }
    if (i * i > 0 && i * i < NMAX) {
      auto iter1 = btree_map.rbegin(), iter2 = btree_map.rend();
      auto iter3 = map.rbegin(), iter4 = map.rend();
      for (; iter3 != iter4; ++iter3, ++iter1) {
        EXPECT_EQ(iter1->first, iter3->first);
        EXPECT_EQ(iter1->second, iter3->second);
      }
      EXPECT_EQ(iter1, iter2);
    }
    EXPECT_EQ(btree_map.size(), map.size());
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
