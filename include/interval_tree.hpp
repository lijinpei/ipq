#pragma once

#include <map>
#include <utility>

namespace ipq {
template <typename KeyTy, typename ValTy, typename MapTy>
struct IntervalTree {
  MapTy keys;

 public:
  ValTy* find(KeyTy key) {
    auto iter = keys.upper_bound(key);
    if (iter == keys.begin()) {
      return nullptr;
    }
    --iter;
    if (key <= iter->second.first) {
      return &iter->second.second;
    } else {
      return nullptr;
    }
  }

  void update(KeyTy key1, KeyTy key2, ValTy val) {
    auto iter = keys.lower_bound(key1);
    if (iter != keys.end()) {
    }
    while (iter != keys.end() && iter->second.first <= key2) {
      iter = keys.erase(iter);
    }
    if (iter != keys.end() && iter->first <= key2) {
      auto old_val = iter->second;
      keys.erase(iter);
      iter = keys.emplace(key2 + 1, old_val).first;
    }
    iter = keys.emplace(key1, std::make_pair(key2, val)).first;
    if (iter != keys.begin() && (--iter)->second.first >= key1) {
      auto old_val = iter->second;
      iter->second.first = key1 - 1;
      if (old_val.first > key2) {
        keys.emplace(key2 + 1, old_val);
      }
    }
  }

  void remove(KeyTy key1, KeyTy key2) {
    auto iter = keys.lower_bound(key1);
    while (iter != keys.end() && iter->second.first <= key2) {
      iter = keys.erase(iter);
    }
    if (iter != keys.end() && iter->first <= key2) {
      auto old_val = iter->second;
      keys.erase(iter);
      iter = keys.emplace(key2 + 1, old_val).first;
    }
    if (iter != keys.begin() && (--iter)->second.first >= key1) {
      auto old_val = iter->second;
      iter->second.first = key1 - 1;
      if (old_val.first > key2) {
        keys.emplace(key2 + 1, old_val);
      }
    }
  }

  size_t size() {
    return keys.size();
  }
};

}  // namespace ipq
