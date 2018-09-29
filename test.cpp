#include "btree.hpp"
#include <iostream>

#include <memory>

int main() {
  ipq::BTreeSet<int> btree;
  for (int i = 0; i < 200; ++i) {
    btree.add(i);
  }
  for (int i = 0; i < 200 + 10; ++i) {
    std::cout << btree.find(i) << ' ';
  }
  std::cout << std::endl;
  for (int i = 0; i < 300; ++i) {
    btree.dump(std::cout);
    std::cout << "remove: " << i << std::endl;
    btree.remove(i);
  }
}
