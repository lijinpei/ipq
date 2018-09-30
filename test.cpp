#include "btree.hpp"
#include <iostream>

#include <memory>
#include <string>
#include <set>

int main() {
  ipq::BTreeSet<std::string> btree;
  int N = 4000;
  std::cout << "start btree" << std::endl;
  for (int i = 0; i < N; ++i) {
    //std::cout << "add: " << i << std::endl;
    btree.add(std::to_string(i));
    //btree.dump(std::cout);
  }
  btree.dump(std::cout);
  std::cout << "add finish" << std::endl;
  for (int i = 0; i < N + 10; ++i) {
    //std::cout << btree.find(i) << ' ';
  }
  std::cout << std::endl;
  for (int i = 0; i < N; ++i) {
    btree.dump(std::cout);
    std::cout << "remove: " << i << std::endl;
    btree.remove(std::to_string(i));
  }
  std::cout << "Finish btree" << std::endl;
  /*
  std::set<int> set;
  std::cout << "start stl set" << std::endl;
  for (int i = 0; i < N; ++i) {
    set.insert(i);
  }
  for (int i = 0; i < N + 10; ++i) {
    //std::cout << btree.find(i) << ' ';
  }
  std::cout << std::endl;
  for (int i = 0; i < N; ++i) {
    //btree.dump(std::cout);
    //std::cout << "remove: " << i << std::endl;
    set.erase(i);
  }
  std::cout << "finish stl set" << std::endl;
  //btree.dump(std::cout);
  */
}
