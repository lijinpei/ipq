#include <set>
#include <string>
#include "btree_set.hpp"
#include "gtest/gtest.h"

const int NMAX = 20000;

TEST(SequentialInsertDelete, int) {
  ipq::BTreeSet<int> btree_set;
  for (int i = 0; i < NMAX; ++i) {
    btree_set.insert(i);
    EXPECT_EQ(btree_set.size(), i + 1);
  }
  {
    int i = 0;
    for (auto iter1 = btree_set.begin(), iter2 = btree_set.end();
         iter1 != iter2; ++iter1) {
      EXPECT_EQ(*iter1, i++);
    }
    i = NMAX - 1;
    for (auto iter1 = btree_set.rbegin(), iter2 = btree_set.rend();
         iter1 != iter2; ++iter1) {
      EXPECT_EQ(*iter1, i--);
      if (i < 0) {
        break;
      }
    }
  }
  for (int i = 0; i < NMAX; ++i) {
    btree_set.erase(i);
    EXPECT_EQ(btree_set.size(), NMAX - i - 1);
  }
}

TEST(SequentialInsertDelete, string) {
  ipq::BTreeSet<std::string> btree_set;
  std::set<std::string> set;
  for (int i = 0; i < NMAX; ++i) {
    std::string str = std::to_string(i);
    btree_set.insert(str);
    EXPECT_EQ(btree_set.size(), i + 1);
    set.insert(str);
  }
  {
    auto iter1 = btree_set.begin(), iter2 = btree_set.end();
    auto iter3 = set.begin(), iter4 = set.end();
    for (; iter3 != iter4; ++iter3, ++iter1) {
      EXPECT_EQ(*iter1, *iter3);
    }
    EXPECT_EQ(iter1, iter2);
  }
  {
    auto iter1 = btree_set.rbegin(), iter2 = btree_set.rend();
    auto iter3 = set.rbegin(), iter4 = set.rend();
    for (; iter3 != iter4; ++iter3, ++iter1) {
      EXPECT_EQ(*iter1, *iter3);
    }
    EXPECT_EQ(iter1, iter2);
  }
  for (int i = 0; i < NMAX; ++i) {
    std::string str = std::to_string(i);
    btree_set.erase(str);
    set.erase(str);
    if (i * i > 0 && i * i < NMAX) {
      auto iter1 = btree_set.begin(), iter2 = btree_set.end();
      auto iter3 = set.begin(), iter4 = set.end();
      for (; iter3 != iter4; ++iter3, ++iter1) {
        EXPECT_EQ(*iter1, *iter3);
      }
      EXPECT_EQ(iter1, iter2);
    }
    if (i * i > 0 && i * i < NMAX) {
      auto iter1 = btree_set.rbegin(), iter2 = btree_set.rend();
      auto iter3 = set.rbegin(), iter4 = set.rend();
      for (; iter3 != iter4; ++iter3, ++iter1) {
        EXPECT_EQ(*iter1, *iter3);
      }
      EXPECT_EQ(iter1, iter2);
    }
    EXPECT_EQ(btree_set.size(), set.size());
  }
}

TEST(lower_bound, int) {
  ipq::BTreeSet<int> btree_set;
  for (int i = 0; i < NMAX; ++i) {
    btree_set.insert(i);
  }
  for (int i = 0; i < NMAX; ++i) {
    auto iter = btree_set.lower_bound(i);
    EXPECT_NE(iter, btree_set.end());
    EXPECT_EQ(*iter, i);
  }
  for (int i = NMAX; i < NMAX + 10; ++i) {
    auto iter = btree_set.lower_bound(i);
    EXPECT_EQ(iter, btree_set.end());
  }
}

TEST(lower_bound, string) {
  ipq::BTreeSet<std::string> btree_set;
  std::set<std::string> set;
  for (int i = 0; i < NMAX; ++i) {
    std::string str = std::to_string(i);
    btree_set.insert(str);
    set.insert(str);
  }
  for (int i = -10 ; i < NMAX + 10; ++i) {
    std::string str = std::to_string(i);
    auto iter1 = btree_set.lower_bound(str);
    auto iter2 = set.lower_bound(str);
    if (iter2 == set.end()) {
      EXPECT_EQ(iter1, btree_set.end());
    } else {
      EXPECT_EQ(*iter1, *iter2);
    }
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
