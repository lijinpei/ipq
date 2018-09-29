#pragma once

#include "config.hpp"

#include <cstdint>
#include <limits>
#include <llvm/ADT/SmallVector.h>
#include <type_traits>
#include <utility>
#include <functional>

namespace ipq {

template <typename ElementTy, typename CompTy>
struct ThreeWayCompAdaptor {
  constexpr int operator()(const ElementTy & e1, const ElementTy & e2) const {
    auto comp1 = CompTy{}(e1, e2);
    if (comp1) {
      return -1;
    }
    auto comp2 = CompTy{}(e2, e1);
    return comp2 ? 1 : 0;
  }
};

template <typename T>
class DummyVector {
public:
  void push_back(const T&) {}
  template <typename ...A>
  void emplace_back(A...) {}
};

namespace internal {
template <typename P> struct LeafNode;

template <typename P> struct InternalNode;
template <typename ElementTy, typename ThreeWayCompTy, typename AllocTy, int MinNodeDegree>
class BTreeSet;

template <typename KeyTy, typename ValueTy, typename ThreeWayCompTy, typename AllocTy, int MinNodeDegree>
class BTreeMap;

/* Something interesting https://godbolt.org/z/WI3pGh
 */
template <int MinChildDeg, typename Value, typename ThreeWayComp,
          typename AllocTy>
struct BTreeParams {
  static_assert(MinChildDeg >= 2, "minimal degree of a b-tree should be 2");
  using LeafNodeTy = LeafNode<BTreeParams>;
  using InternalNodeTy = InternalNode<BTreeParams>;
  using LeafNodeAllocTy = typename AllocTy::template rebind<LeafNodeTy>::other;
  using InternalNodeAllocTy =
      typename AllocTy::template rebind<InternalNodeTy>::other;
  enum {
    MinChildDegree = MinChildDeg,
    MaxChildDegree = 2 * MinChildDegree,
    MinNodeDegree = MinChildDegree - 1,
    MaxNodeDegree = MaxChildDegree - 1,
    SplitIndex = (MaxNodeDegree - 1) / 2,
  };
  using DegreeCountTy = typename std::conditional<
      MaxChildDegree <= std::numeric_limits<uint8_t>::max(), uint8_t,
      typename std::conditional<MaxChildDegree <=
                                    std::numeric_limits<uint16_t>::max(),
                                uint16_t, uint32_t>::type>::type;
  using ValueTy = Value;
  using ThreeWayCompTy = ThreeWayComp;
};

template <typename P> struct LeafNode {
  typename P::DegreeCountTy node_degree_;
  typename P::ValueTy values_[P::MaxNodeDegree];
  bool isFull() { return node_degree_ == P::MaxNodeDegree; }
};

template <typename P> struct InternalNode : LeafNode<P> {
private:
  using ValueTy = typename P::ValueTy;
  using DegreeCountTy = typename P::DegreeCountTy;
  enum { SplitIndex = P::SplitIndex, MaxNodeDegree = P::MaxNodeDegree };
  /* Move 'from' to uninitialized memory 'to'.
   */
  static void transter(ValueTy *from, ValueTy *to) {
    new (to) ValueTy(std::move(*from));
    from->~ValueTy();
  }

public:
  InternalNode *children_[P::MaxChildDegree];
  LeafNode<P> asLeaf() { return *this; }
  /* split this node as index idx, children[idx] is guaranteed to be full, this
   * node is guaranteed to be not full.
   */
  template <bool ChildisLeaf>
  void split(typename P::DegreeCountTy idx, InternalNode *new_child) {
    IPQ_ASSERT(!isFull());
    InternalNode *child = children_[idx];
    IPQ_ASSERT(child && child->isFull());
    for (int i = SplitIndex + 1; i < MaxNodeDegree; ++i) {
      child->transfer<false, !ChildisLeaf>(i, new_child, i - SplitIndex - 1);
    }
    if (!ChildisLeaf) {
      new_child->children_[0] = child->children_[SplitIndex + 1];
    }
    new_child->node_degree_ = MaxNodeDegree - SplitIndex - 1;
    for (int i = this->node_degree_ - 1; i >= idx; --i) {
      transfer<false, true>(i, this, i + 1);
    }
    child->transfer(SplitIndex, this, idx);
    child->node_degree_ = SplitIndex;
    children_[idx + 1] = new_child;
  }

  /* Predicate: this is full. this is a root node;
   * new_root, new_child are uninitialzed.
   * Post-condition: new_root is the new root. this and new_child is new_root's
   * two children
   */
  InternalNode *split_root(InternalNode *new_root, InternalNode *new_child) {
    transfer(SplitIndex, new_root, 0);
    new_root->node_degree_ = 1;
    new_root->children_[0] = this;
    new_root->children_[1] = new_child;
    for (int i = SplitIndex + 1; i < MaxNodeDegree; ++i) {
      transfer<false, true>(i, new_child, i - SplitIndex - 1);
    }
    new_child->children_[0] = children_[SplitIndex + 1];
    new_child->node_degree_ = MaxNodeDegree - SplitIndex - 1;
    return new_root;
  }

  template <bool WithChildren>
  void transferToRight(InternalNode &other, InternalNode &parent,
                       DegreeCountTy idx) {
    for (int i = other.node_degree_ - 1; i >= 0; --i) {
      other.transfer<false, WithChildren>(i, other, i + 1);
    }
    transfer(parent.values_[idx], other.values_[0]);
    if (WithChildren) {
      other.children_[1] = other.children_[0];
      other.children_[0] = children_[this->node_degree_];
    }
    transfer(this->node_degree_ - 1, parent, idx);
    ++other->node_degree_;
    --this->node_degree;
  }

  template <bool WithChildren>
  void transferToRight(InternalNode *other, InternalNode *parent,
                       DegreeCountTy idx) {
    transferToRight<WithChildren>(*other, *parent, idx);
  }

  template <bool WithChildren>
  void transferToLeft(InternalNode &other, InternalNode &parent,
                      DegreeCountTy idx) {
    transfer(parent.values_[idx], other.values_[other.node_degree_]);
    if (WithChildren) {
      other.children_[other.node_degree_] = children_[0];
    }
    for (int i = 1; i < this->node_degree_; ++i) {
      transfer<WithChildren, false>(i, this, i - 1);
    }
    if (WithChildren) {
      children_[this->node_degree_ - 1] = children_[this->node_degree_];
    }
    ++other->node_degree_;
    --this->node_degree;
  }
  template <bool WithChildren>
  void transferToLeft(InternalNode *other, InternalNode *parent,
                      DegreeCountTy idx) {
    transferToLeft<WithChildren>(*other, *parent, idx);
  }

  static void transfer(ValueTy &from, ValueTy &to) { transfer(&from, &to); }
  template <bool WithLeftPointer = false, bool WithRightPointer = false>
  void transfer(DegreeCountTy from, InternalNode &to, DegreeCountTy to_idx) {
    transfer(this->values_[from], to.values[to_idx]);
    if (WithRightPointer) {
      to.children_[to_idx + 1] = children_[from + 1];
    }
    if (WithLeftPointer) {
      to.children_[to_idx] = children_[to_idx];
    }
  }
  template <bool WithRightPointer = false, bool WithLeftPointer = false>
  void transfer(DegreeCountTy from, InternalNode *to, DegreeCountTy to_idx) {
    transfer(from, *to, to_idx);
  }
};

template <typename P>
class BTreeImpl : P::LeafNodeAllocTy, P::InternalNodeAllocTy {
  using LeafNodeAllocTy = P::LeafNodeAllocTy;
  using InternalNodeAllocTy = P::InternalNodeAllocTy;
  using LeafNodeTy = typename P::LeafNodeTy;
  using InternalNodeTy = typename P::InternalNodeTy;
  using ThreeWayCompTy = typename P::ThreeWayCompTy;
  using ValueTy = typename P::ValueTy;
  enum {
    MinNodeDegree = P::MinNodeDegree,
    MaxNodeDegree = P::MaxNodeDegree,
    MinChildDegree = P::MinChildDegree,
    MaxChildDegree = P::MaxChildDegree,
    BSearchThreshold = 8
  };
  using DegreeCountTy = typename P::DegreeCountTy;
  std::size_t internal_height_; // mininal value of height is 1 for an empty or
                                // one-level tree
  InternalNodeTy *root_;
  /* if ret.second == true, then ret.first >= 0 and ret.first <
   * node.node_degree_, ret.first is the index of one of the elements in
   * node.values_ that is equal to target if ret.second == false, then ret.first
   * >= 0 and ret.first <= node.node_degree_, ret.first is then index of the
   * first element that is larger than (or equal to) target, if no such element,
   * ret.first == node.node_degree_
   */
  std::pair<DegreeCountTy, bool> lower_bound(const ValueTy target,
                                             const LeafNodeTy &node) {
    DegreeCountTy l = 0, r = node.node_degree_;
    while (r - l > BSearchThreshold) {
      int m = (r - l) / 2 + l;
      int res = ThreeWayCompTy{}(target, node.values_[m]);
      if (res == 0) {
        return {m, true};
      } else if (res < 0) {
        r = m;
      } else {
        l = m + 1;
      }
    }
    while (l < r) {
      int res = ThreeWayCompTy{}(target, node.values_[l]);
      if (!res) {
        return {l, true};
      }
      if (res > 0) {
        break;
      }
    }
    return {l, false};
  }

  template <typename ContTy>
  ValueTy *find(const ValueTy &target, ContTy &path) {
    InternalNodeTy *p = &root_;
    for (int i = 0; i <= internal_height_; ++i) {
      auto res = lower_bound(target, p->asLeaf());
      path.emplace_back(p, res.first);
      if (res.second) {
        return &p->values_[res.first];
      } else {
        p = p->children_[res.first];
      }
    }
    return nullptr;
  }

  template <typename ContTy>
  std::pair<ValueTy *, bool> addNonFull(const ValueTy &target, ContTy &path) {
    InternalNodeTy *node = root_;
    for (int height = 0; height < internal_height_; ++height) {
      auto res = lower_bound(target, node->asLeaf());
      path.emplace_back(node, res.first);
      if (res.second) {
        return {&node->values_[res.first], false};
      } else {
        InternalNodeTy *node = node->children_[res.first];
        if (node->isFull()) {
          node->split(res.first);
          int cmp = ThreeWayCompTy{}(target, node->values_[res.first]);
          if (!cmp) {
            return {&node->values[res.first], false};
          } else if (cmp < 0) {
            node = node->children_[res.first];
          } else {
            node = node->children[res.first + 1];
          }
        }
      }
    }
    auto res = node->insert(target);
    return res;
  }

  template <typename ContTy>
  std::pair<ValueTy *, bool> add(const ValueTy &target, ContTy &path) {
    if (root_->isFull()) {
      InternalNodeTy *new_node1 = this->InternalNodeAllocTy::allocate(1);
      InternalNodeTy *new_node2 = this->InternalNodeAllocTy::allocate(1);
      root_ = root_->split_root(new_node1, new_node2);
      ++internal_height_;
    }
    return addNonFull(target, path);
  }

  bool remove(const ValueTy &target) {
    InternalNodeTy *node = root_;
    if (!node) {
      return false;
    }
    for (int height = 0; height < internal_height_; ++height) {
      auto res = lower_bound(target, node->asLeaf());
      auto idx = res.first;
      if (res.second) {
        InternalNodeTy *left_node = node->children_[idx];
        if (left_node->node_degree_ >= MinChildDegree) {
          node->values[idx].~ValueTy();
          removePrec(left_node, height, &node->values[idx]);
          return true;
        } else {
          InternalNodeTy *right_node = node->children_[idx + 1];
          if (right_node->node_degree_ >= MinChildDegree) {
            node->values[idx].~ValueTy();
            removeSucc(right_node, height, &node->values[idx]);
            return true;
          } else {
            if (height + 1 == internal_height_) {
              auto res = node->mergeAt<false>(idx);
              node = res.first;
              this->LeafNodeAllocTy::deallocate(res.second, 1);
            } else {
              auto res = node->mergeAt<true>(idx);
              node = res.first;
              this->InternalNodeAllocTy::deallocate(res.second, 1);
            }
          }
        }
      } else {
        InternalNodeTy *child = node->children_[idx];
        if (child->node_degree_ == MinNodeDegree) {
          if (idx && node->children_[idx - 1]->node_degree_ != MinNodeDegree) {
            if (height + 1 == internal_height_) {
              node->children_[idx - 1]->transferToRight<false>(child, node,
                                                               idx - 1);
            } else {
              node->children_[idx - 1]->transferToRight<true>(child, node,
                                                              idx - 1);
            }
            node = child;
          } else if (idx != node->node_degree_ &&
                     node->children_[idx + 1]->node_degree_ != MinChildDegree) {
            if (height + 1 == internal_height_) {
              node->children_[idx + 1]->transferToLeft<false>(child, node, idx);
            } else {
              node->children_[idx + 1]->transferToLeft<true>(child, node, idx);
            }
            node = child;
          } else {
            if (height + 1 == internal_height_) {
              node = node->mergeAt<false>(idx);
            } else {
              node = node->mergeAt<true>(idx);
            }
          }
        }
      }
    }
    auto res = lower_bound(target, node->asLeaf());
    if (res.second) {
      node->remove(res.first);
      return true;
    } else {
      return false;
    }
  }
  void removePrec(InternalNodeTy *node, int height, ValueTy *pos) {
    llvm::SmallVector<InternalNodeTy *, 32> nodes;
    for (; height < internal_height_; ++height) {
      nodes.push_back(node);
      nodes = nodes->children_[nodes->node_degree_];
    }
    InternalNodeTy::transter(&nodes->values_[nodes->node_degree_ - 1], pos);
    --node->node_degree_;
    bool is_leaf = true;
    while (!nodes.empty()) {
      if (node->node_degree_ >= MinNodeDegree) {
        return;
      }
      InternalNodeTy *parent = nodes.back();
      InternalNodeTy *left_sibling = parent->children_[nodes->node_degree_ - 1];
      if (left_sibling->node_degree_ > MinNodeDegree) {
        if (is_leaf) {
          left_sibling->transferToRight<true>(node, parent,
                                              parent->node_degree_ - 1);
        } else {
          left_sibling->transferToRight<false>(node, parent,
                                               parent->node_degree_ - 1);
        }
        return;
      } else {
        parent->mergeAt(parent->node_degree_ - 1);
        node = parent;
        nodes.pop_back();
      }
      is_leaf = false;
    }
  }

  void removeSucc(InternalNodeTy *node, int height, ValueTy *pos) {
    llvm::SmallVector<InternalNodeTy *, 32> nodes;
    for (; height < internal_height_; ++height) {
      nodes.push_back(node);
      nodes = nodes->children_[0];
    }
    InternalNodeTy::transter(&nodes->values_[0], pos);
    for (int i = 1; i < node->node_degree_; ++i) {
      InternalNodeTy::transter(nodes->values_[i], nodes->values_[i - 1]);
    }
    --node->node_degree_;
    bool is_leaf = true;
    while (!nodes.empty()) {
      if (node->node_degree_ >= MinNodeDegree) {
        return;
      }
      InternalNodeTy *parent = nodes.back();
      InternalNodeTy *right_sibling = parent->children_[1];
      if (right_sibling->node_degree_ > MinNodeDegree) {
        if (is_leaf) {
          right_sibling->transferToLeft<true>(node, parent, 0);
        } else {
          right_sibling->transferToLeft<false>(node, parent, 0);
        }
        return;
      } else {
        parent->mergeAt(0);
        node = parent;
        nodes.pop_back();
      }
      is_leaf = false;
    }
  }
  template <typename ElementTy, typename AllocTy, typename ThreeWayCompTy,
            int MinNodeDegree>
  friend class BTreeSet;

  template <typename KeyTy, typename ValueTy, typename ThreeWayCompTy,
            typename AllocTy,
            int MinNodeDegree>
  friend class BTreeMap;
};

template <typename ElementTy, typename ThreeWayCompTy = ThreeWayCompAdaptor<ElementTy, std::less<ElementTy>>, typename AllocTy = std::allocator<ElementTy>,
          int MinNodeDegree = 4>
class BTreeSet {
  using Param = BTreeParams<MinNodeDegree, ElementTy, ThreeWayCompTy, AllocTy>;
  BTreeImpl<Param> btree;
public:
  bool add(ElementTy & e) {
  }
  bool remove(ElementTy & e) {
  }
  bool contains(ElementTy& e) {
  }
};

template <typename KeyTy, typename ValueTy, typename ThreeWayCompTy = ThreeWayCompAdaptor<std::pair<KeyTy, ValueTy>, std::less<std::pair<KeyTy, ValueTy>>>,
          typename AllocTy = std::allocator<std::pair<KeyTy, ValueTy>>,
          int MinNodeDegree = 4>
class BTreeMap {};

} // namespace internal
} // namespace ipq

