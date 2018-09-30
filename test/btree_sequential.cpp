#include "btree_set.hpp"
#include "gtest/gtest.h"
#include <string>

const int NMAX = 400000;

TEST (SequentialInsertDelete, int) {
  ipq::BTreeSet<int> btree_set;
  for (int i = 0; i < NMAX; ++i) {
    btree_set.insert(i);
  }
  std::cout << std::endl << "add finish" << std::endl;
  for (int i = 0; i < NMAX; ++i) {
    btree_set.erase(i);
  }
}

TEST (SequentialInsertDelete, string) {
  ipq::BTreeSet<std::string> btree_set;
  for (int i = 0; i < NMAX; ++i) {
    btree_set.insert(std::to_string(i));
  }
  std::cout << std::endl << "add finish" << std::endl;
  for (int i = 0; i < NMAX; ++i) {
    btree_set.erase(std::to_string(i));
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
