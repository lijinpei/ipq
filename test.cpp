#include "btree.hpp"
#include <iostream>

#include <memory>

int main() {
  ipq::BTreeSet<int> btree;
  int N = 400000;
  for (int i = 0; i < N; ++i) {
    btree.add(i);
  }
  for (int i = 0; i < N + 10; ++i) {
    std::cout << btree.find(i) << ' ';
  }
  std::cout << std::endl;
  for (int i = 0; i < N; ++i) {
    //btree.dump(std::cout);
    std::cout << "remove: " << i << std::endl;
    btree.remove(i);
  }
  btree.dump(std::cout);
}
