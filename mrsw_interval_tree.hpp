#pragma once

#include <map>
#include <optional>
#include <shared_mutex>
#include <utility>

namespace ipq {
namespace inverval_tree {
template <typename KeyTy, typename ValTy> class IntervalTree {
  using NodeTy = std::pair<KeyTy, ValTy>;
  std::map<KeyTy, NodeTy> keys;

public:
  std::optional<ValTy> find(KeyTy key) {
    auto iter = keys.upper_bound(key);
    if (iter == keys.begin()) {
      return {};
    }
    --iter;
    if (iter->first <= key && key < iter->second.first) {
      return iter->second.second;
    } else {
      return {};
    }
  }
  void rangeUpdate(KeyTy key1, KeyTy key2, ValTy val) {
    auto iter = keys.lower_bound(key1);
    while (iter != keys.end() && iter->second.first <= key2) {
      auto iter1 = iter;
      ++iter;
      keys.erase(iter1);
    }
    if (iter != keys.end() && iter->first < key2) {
      auto iter1 = iter;
      auto old_val = iter1->second;
      ++iter;
      keys.erase(iter1);
      iter = keys.emplace_hint(iter, key2, old_val);
    }
    iter = keys.emplace_hint(iter, key1, std::make_pair(key2, val));
    if (iter != keys.begin() && (--iter)->second.first > key1) {
      iter->second.first = key1;
    }
  }
};

template <typename KeyTy, typename ValTy, typename LockTy = std::shared_mutex> class IntervalTreeMRSW {
  LockTy mutex;
  IntervalTree<KeyTy, ValTy> int_tree;
public:
  void rangeUpdate(KeyTy key1, KeyTy key2, ValTy val) {
    mutex.lock();
    int_tree.rangeUpdate(key1, key2, val);
    mutex.unlock();
  }
  std::optional<ValTy> find(KeyTy key) {
    mutex.lock_shared();
    auto ret = int_tree.find(key);
    mutex.unlock_shared();
    return ret;
  }
};

} // namespace inverval_tree
} // namespace ipq
