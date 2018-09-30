#pragma once

#include "btree_impl.hpp"

namespace ipq {
template <typename KeyTy, typename ValueTy,
          typename ThreeWayCompTy = ThreeWayCompAdaptor<
              std::pair<KeyTy, ValueTy>, std::less<std::pair<KeyTy, ValueTy>>>,
          typename AllocTy = std::allocator<std::pair<KeyTy, ValueTy>>,
          int MinChildDegree = 4>
class BTreeMap {};

template <typename KeyTy, typename ValueTy,
          typename ThreeWayCompTy = ThreeWayCompAdaptor<
              std::pair<KeyTy, ValueTy>, std::less<std::pair<KeyTy, ValueTy>>>,
          typename AllocTy = std::allocator<std::pair<KeyTy, ValueTy>>,
          int MinChildDegree = 4>
class BTreeMultiMap {};
}
