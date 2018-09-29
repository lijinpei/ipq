#include "btree.hpp"

#include <memory>

int main() {
  using P = ipq::internal::BTreeParams<5, int, int, std::allocator<int>>;
  ipq::internal::BTreeImpl<P> btree;
}
