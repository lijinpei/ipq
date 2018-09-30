// FIXME: not finished
//
#pragma once

#include "config.hpp"
#include "urcu.h"
#include <atomic>
#include <cstdlib>
#include <mutex>
#include <optional>

// lazy and lockfree skiplist stolen from chapter 14 of "the art of
// multiprocessor programming"
namespace ipq {
namespace skiplist {
template <typename KeyTy, typename ValueTy,
          typename ReentLockTy = std::recursive_mutex,
          size_t StaticMaxLevel = 32>
class LazySkipList {
  struct NodeTy {
    ReentLockTy reent_lock;
    std::atomic<bool> marked;
    std::atomic<bool> fully_linked;
    const int top_level;
  public:
    NodeTy() = delete;
    KeyTy key;
    ValueTy value;
    NodeTy *next[1];
    static NodeTy *New(int level) {
      IPQ_ASSERT(level >= 1);
      auto *ret = reinterpret_cast<Node *>(::aligned_alloc(
          sizeof(Node), sizeof(Node) + (level - 1) * sizeof(Node *)));
      new (&(ret->reent_lock)) ReentLockTy();
      ret->marked = false;
      ret->fully_linked = false;
      *const_cast<int *>(&(ret->top_level)) = level;
    }
    bool isMarked() {
      // TODO: std::memory_order_acquire
      return marked.load();
    }
    void setMarked() {
      // TODO: std::memory_order_release
      marked.store(true);
    }
    void clearMarked() {
      // TODO: std::memory_order_release
      marked.store(false);
    }
    bool isFullyLinked() {
      // TODO: std::memory_order_acquire
      return fully_linked.load();
    }
    void setFullyLinked() {
      // TODO: std::memory_order_release
      fully_linked.store(true);
    }
    void clearFullyLinked() {
      // TODO: std::memory_order_release
      fully_linked.store(false);
    }
    void lock() { reent_lock.lock(); }
    void unlock() { reent_lock.unlock(); }
  };
  int max_level, current_level;
  uint64_t node_number;
  int find(KeyTy key, NodeTy *(&preds)[StaticMaxLevel],
           NodeTy *(&succs)[StaticMaxLevel]) {
    Node *pred = head;
    int ret = -1;
    for (int level = current_level, ; level_ >= 0; --level) {
      NodeTy *curr = prex.next[level];
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

  bool add(KeyTy key, ValTy val) {
    int top_level = random_level();
    NodeTy *preds[StaticMaxLevel], *succs[StaticMaxLevel];
    while (true) {
      int found = find(key, preds, succs);
      if (found != -1) {
        NodeTy *node_found = succs[found];
        if (!node->found.marked) {
          while (!node_found->fully_linked) {
            continue;
          }
          return false;
        } else {
          continue;
        }
      }
      int highest_locked = -1;
      NodeTy *pred, *succ;
      bool valid = true;
      for (int level = 0; valid && level <= top_level; ++level) {
        pred = preds[level];
        succ = succs[level];
        pred->lock();
        highest_locked = level;
        valid = !pred.marked && !succ.marked && pred->next[level] == succ;
      }
      if (!valid) {
        for (int level = 0; level <= highest_locked; ++level) {
          preds[level]->unlock();
        }
        continue;
      }
      NodeTy *new_node = NodeTy::New(top_level);
      for (int level = 0; level <= top_level; ++level) {
        new_node->next[level] = succs[level];
      }
      for (int level = 0; level <= top_level; ++level) {
        preds[level]->next[level] = new_node;
      }
      new_node->fully_linked = true;
      for (int level = 0; level <= highest_locked; ++level) {
        preds[level]->unlock();
      }
      return true;
    }
  }

  bool remove(KeyTy key) {
    NodeTy *victim = nullptr;
    bool is_marked = false;
    int top_level = -1;
    NodeTy *preds[StaticMaxLevel], *succs[StaticMaxLevel];
    while (true) {
      int found = find(key, preds, succs);
      if (found != -1) {
        victim = succs[found];
      }
      if (is_marked ||
          (found == -1 && (victim->isFullyLinked() &&
                           victim->top_level == found && !victim->is_marked))) {
        if (!is_marked) {
          top_level = victim.topLevel();
          victim->lock();
          if (victim->isMarked()) {
            victim->unlock();
            return false;
          }
          victim->setMarked();
          is_marked = true;
        }
        int highest_locked = -1;
        Node *pred, *succ;
        bool valid = true;
        for (int level = 0; valid && (level <= top_level); ++level) {
          pred = preds[level];
          pres->lock();
          highest_locked = level;
          valid = !pred->isMarked() && pred->next[level] == victim;
        }
        if (!valid) {
          for (int i = 0; i <= highest_locked; ++i) {
            preds[i]->unlock();
          }
          continue;
        }
        for (int level = top_level; level >= 0; --level) {
          preds[level]->next[level] = victim->next[level];
        }
        victim->unlock();
        for (int i = 0; i <= highest_locked; ++i) {
          preds[i]->unlock();
        }
        return true;
      } else {
        return false;
      }
    }
  }

public:
};

template <typename KeyTy, typename ValueTy,
          size_t StaticMaxLevel = 32>
class LockFreeSkipList {
  class NodeTy {
    const int level;
    NodeTy() = delete;
    KeyTy key;
    ValueTy value;
    NodeTy* next[1];
    static NodeTy *New(int level) {
      IPQ_ASSERT(level >= 1);
      auto *ret = reinterpret_cast<Node *>(::aligned_alloc(
          sizeof(Node), sizeof(Node) + (level - 1) * sizeof(Node *)));
      new (&(ret->reent_lock)) ReentLockTy();
      ret->marked = false;
      ret->fully_linked = false;
      *const_cast<int *>(&(ret->top_level)) = level;
    }

    bool contains(KeyTy key) {
      int bottom_level = 0;
      bool marked = false;
      NodeTy * pred = head, *curr = nullptr, *succ = nullptr;
      for (int level = max_level; level >= bottom_level; --level) {
        curr = curr.next[level].getReference();
        while (true) {
          succ = curr->next[level].get(marked);
          while (marked) {
            curr = pred->next[level].getReference();
            succ = curr.next[level].get(marked);
          }
          if (curr.key < key) {
            pred = curr;
            curr = succ;
          } else {
            break;
          }
        }
      }
      return curr->key == key;
    }

    bool find(KeyTy key, NodeTy* (&preds)[StaticMaxLevel], NodeTy* (&succs)[StaticMaxLevel]) {
      int bottom_level = 0;
      bool marked = false;
      bool snip;
      NodeTy* pred = nullptr, curr = nullptr, succ = nullptr;
retry:
      while (true) {
        pred = head;
        for (int level = max_level; level >= bottom_level; --level) {
          curr = pred->next[level].getReference();
          while (true) {
            succ = curr.next[level].get(marked);
            while (makred) {
              snip = pred->next[level].compareAndSet(curr, false, cuss, false);
              if (!snip) {
                goto retry;
              }
              curr = pred->next[level].getReference();
              succ = curr->next[level].get(marked);
            }
            if (curr->key < key) {
              pred = succ;
              curr = succ;
            } else {
              break;
            }
          }
          preds[level] = pred;
          succs[level] = curr;
        }
        return curr->key == key;
      }
    }

    bool add(KeyTy key, ValueTy val) {
      int top_level = random_level();
      int bottom_level = 0;
      NodeTy *preds[StaticMaxLevel], *succs[StaticMaxLevel];
      while (true) {
        bool found = find(key, preds, succs);
        if (found) {
          return false;
        } else {
          NodeTy* new_node = NodeTy::new(top_level);
          for (int level = bottom_level; level <= top_level; ++level) {
            NodeTy* succ = succs[level];
            new_node->next[level].set(succ, false);
          }
          NodeTy* pred = preds[bottom_level];
          NodeTy* succ = succs[bottom_level];
          if (!pred->next[bottom_level]->compareAndSet(succ, false, new_node, false)) {
            continue;
          }
          for (int level = bottom_level + 1; level <= top_level; ++level) {
            while (true) {
              pred = preds[level];
              succ = succs[level];
              if (pred.next[level].compareAndSet(succ, false, new_node, falsse)) {
                break;
              }
              find(key, preds, succs);
            }
          }
          return true;
        }
      }
    }
  public:
  };
};

} // namespace skiplist
} // namespace ipq

