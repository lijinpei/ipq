#pragma once

#include "config.hpp"
#include "urcu.h"
#include <cstdlib>
#include <atomic>
#include <optional>

namespace ipq {
namespace skiplist {
  template <typename KeyTy, typename ValueTy, size_t StaticMaxLevel = 32>
  struct Node {
    KeyTy key;
    ValueTy value;
    Node *next[1];
    Node *New(int level) {
      IPQ_ASSERT(level >= 1);
      return reinterpret_cast<Node *>(::aligned_alloc(
          sizeof(Node), sizeof(Node) + (level - 1) * sizeof(Node *)));
    }
  };
  template <typename KeyTy, typename ValueTy>
  class LazySkipList {
    using NodeTu = Node<KeyTy, ValueTy>;
    int max_level, current_level;
    uint64_t node_number;
    int find_impl(KeyTy key, NodeTy* (&preds)[StaticMaxLevel], NodeTy* (&succs)[StaticMaxLevel]) {
      Node* pred = head;
      int ret = -1;
      for (int level = current_level,; level_>= 0; --level) {
        NodeTy* curr = prex.next[level];
        while (key > curr->key) {
          pred = curr;
          curr = curr->next[level];
        }
        if (ret == -1 && key == curr->key) {
          ret = level;
        }
        preds[level] = pred;
        succs[level] = curr;
      }
      return ret;
    }
  public:
  std::optional<ValueTy> find(KeyTy key) {
    rcu_read_lock();
    NodeTy *preds[StaticMaxLevel], *succs[StaticMaxLevel];
    NodeTy *result = find_impl(key, preds, succs);
    if (result && result->fully_linked && !result->marked) {
      ValueTy val(result->value);
      rcu_read_unlock();
      return val;
    } else {
      rcu_read_unlock();
      return {};
    }
  }
  bool update(KeyTy key, ValueTy val) {
    int top_level = randomLevel();
    NodeTy *preds[StaticMaxLevel], *succs[StaticMaxLevel];
    while (true) {
      rcu_read_lock();
      int found = find(key, preds, succs);
      if (found != -1) {
        NodeTy* node_found = succs[found];
        if (!node_found->marked) {
          while (!node_found->fully_linked) {}
            return false;
        }
      }
      rcu_read_unlock();
    }
  }
  bool remove(KeyTy key) {
  }
  };
}
}

