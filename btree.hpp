#pragma once

#include "config.hpp"

#include <cstdint>
#include <functional>
#include <limits>
#include <llvm/ADT/SmallVector.h>
#include <type_traits>
#include <utility>
#include <iostream>

namespace ipq {

template <typename ElementTy, typename CompTy> struct ThreeWayCompAdaptor {
  constexpr int operator()(const ElementTy &e1, const ElementTy &e2) const {
    auto comp1 = CompTy{}(e1, e2);
    if (comp1) {
      return -1;
    }
    auto comp2 = CompTy{}(e2, e1);
    return comp2 ? 1 : 0;
  }
};

template <typename T> class DummyVector {
public:
  void push_back(const T &) {}
  template <typename... A> void emplace_back(A...) {}
};

template <typename ElementTy, typename ThreeWayCompTy, typename AllocTy,
          int MinChildDegree>
class BTreeSet;

template <typename KeyTy, typename ValueTy, typename ThreeWayCompTy,
          typename AllocTy, int MinChildDegree>
class BTreeMap;
namespace internal {
template <typename P> struct LeafNode;

template <typename P> struct InternalNode;

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
    BSearchThreshold = 8
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
  using DegreeCountTy = typename P::DegreeCountTy;
  using ThreeWayCompTy = typename P::ThreeWayCompTy;
  using ValueTy = typename P::ValueTy;
  enum {
    MaxNodeDegree = P::MaxNodeDegree,
    BSearchThreshold = P::BSearchThreshold
  };
  DegreeCountTy node_degree_;
  ValueTy values_[MaxNodeDegree];
  bool isFull() { 
    return node_degree_ == MaxNodeDegree; }

  /* if ret.second == true, then ret.first >= 0 and ret.first <
   * node.node_degree_, ret.first is the index of one of the elements in
   * node.values_ that is equal to target if ret.second == false, then ret.first
   * >= 0 and ret.first <= node.node_degree_, ret.first is then index of the
   * first element that is larger than (or equal to) target, if no such element,
   * ret.first == node.node_degree_
   */
  std::pair<DegreeCountTy, bool> lower_bound(const ValueTy &target) const {
    DegreeCountTy l = 0, r = node_degree_;
    while (r - l > BSearchThreshold) {
      int m = (r - l) / 2 + l;
      int res = ThreeWayCompTy{}(target, values_[m]);
      if (res == 0) {
        return {m, true};
      } else if (res < 0) {
        r = m;
      } else {
        l = m + 1;
      }
    }
    while (l < r) {
      int res = ThreeWayCompTy{}(target, values_[l]);
      if (!res) {
        return {l, true};
      }
      if (res < 0) {
        break;
      }
      ++l;
    }
    return {l, false};
  }
  ValueTy *insert(const ValueTy &value) {
    auto res = lower_bound(value);
    DegreeCountTy idx = res.first;
    for (auto i = idx; i < node_degree_; ++i) {
      transfer(values_[i], values_[i + 1]);
    }
    ++node_degree_;
    new (values_ + idx) ValueTy(value);
    return values_ + idx;
  }

  static void transfer(ValueTy *from, ValueTy *to) {
    new (to) ValueTy(std::move(*from));
    from->~ValueTy();
  }
  void remove(DegreeCountTy idx) {
    for (DegreeCountTy i = idx + 1; i < node_degree_; ++i) {
      transfer(values_[i], values_[i - 1]);
    }
    --node_degree_;
  }
  static void transfer(ValueTy &from, ValueTy &to) { transfer(&from, &to); }
  void dump(std::ostream& os, int h) {
    os << "height: " << h << " elements: ";
    for (DegreeCountTy i = 0; i < node_degree_; ++i) {
      os << values_[i] << ' ';
    }
    os << std::endl;
  }
};

template <typename P> struct InternalNode : LeafNode<P> {
private:
  using ValueTy = typename P::ValueTy;
  using LeafNodeTy = LeafNode<P>;
  using DegreeCountTy = typename P::DegreeCountTy;
  enum { SplitIndex = P::SplitIndex, MaxNodeDegree = P::MaxNodeDegree, MinNodeDegree = P::MinNodeDegree};
  /* Move 'from' to uninitialized memory 'to'.
   */
public:
  InternalNode *children_[P::MaxChildDegree];
  LeafNode<P> asLeaf() { return *this; }
  /* split this node as index idx, children[idx] is guaranteed to be full, this
   * node is guaranteed to be not full.
   */
  template <bool WithChildren>
  void split(typename P::DegreeCountTy idx, InternalNode *new_child) {
    IPQ_ASSERT(!this->isFull());
    InternalNode *child = children_[idx];
    IPQ_ASSERT(child && child->isFull());
    for (int i = SplitIndex + 1; i < MaxNodeDegree; ++i) {
      child->transfer<false, WithChildren>(i, new_child, i - SplitIndex - 1);
    }
    if (WithChildren) {
      new_child->children_[0] = child->children_[SplitIndex + 1];
    }
    new_child->node_degree_ = MaxNodeDegree - SplitIndex - 1;
    for (int i = this->node_degree_ - 1; i >= idx; --i) {
      transfer<false, true>(i, this, i + 1);
    }
    child->transfer(SplitIndex, this, idx);
    child->node_degree_ = SplitIndex;
    children_[idx + 1] = new_child;
    ++this->node_degree_;
  }

  /* Predicate: this is full. this is a root node;
   * new_root, new_child are uninitialzed.
   * Post-condition: new_root is the new root. this and new_child is new_root's
   * two children
   */
  template <bool WithChildren>
  InternalNode *split_root(InternalNode *new_root, InternalNode *new_child) {
    transfer(SplitIndex, new_root, 0);
    new_root->node_degree_ = 1;
    new_root->children_[0] = this;
    new_root->children_[1] = new_child;
    for (int i = SplitIndex + 1; i < MaxNodeDegree; ++i) {
      transfer<false, WithChildren>(i, new_child, i - SplitIndex - 1);
    }
    if (WithChildren) {
      new_child->children_[0] = children_[SplitIndex + 1];
    }
    new_child->node_degree_ = MaxNodeDegree - SplitIndex - 1;
    this->node_degree_ = SplitIndex;
    return new_root;
  }

  template <bool WithChildren>
  void transferToRight(InternalNode &other, InternalNode &parent,
                       DegreeCountTy idx) {
    for (int i = other.node_degree_ - 1; i >= 0; --i) {
      other.transfer<false, WithChildren>(i, other, i + 1);
    }
    LeafNodeTy::transfer(parent.values_[idx], other.values_[0]);
    if (WithChildren) {
      other.children_[1] = other.children_[0];
      other.children_[0] = children_[this->node_degree_];
    }
    transfer(this->node_degree_ - 1, parent, idx);
    ++other.node_degree_;
    --this->node_degree_;
  }

  template <bool WithChildren>
  void transferToRight(InternalNode *other, InternalNode *parent,
                       DegreeCountTy idx) {
    transferToRight<WithChildren>(*other, *parent, idx);
  }

  template <bool WithChildren>
  void transferToLeft(InternalNode &other, InternalNode &parent,
                      DegreeCountTy idx) {
    LeafNodeTy::transfer(parent.values_[idx], other.values_[other.node_degree_]);
    LeafNodeTy::transfer(this->values_, parent.values_ + idx);
    if (WithChildren) {
      other.children_[other.node_degree_ + 1] = children_[0];
    }
    for (int i = 1; i < this->node_degree_; ++i) {
      transfer<WithChildren, false>(i, this, i - 1);
    }
    if (WithChildren) {
      children_[this->node_degree_ - 1] = children_[this->node_degree_];
    }
    ++other.node_degree_;
    --this->node_degree_;
  }
  template <bool WithChildren>
  void transferToLeft(InternalNode *other, InternalNode *parent,
                      DegreeCountTy idx) {
    transferToLeft<WithChildren>(*other, *parent, idx);
  }
  template <bool WithLeftPointer = false, bool WithRightPointer = false>
  void transfer(DegreeCountTy from, InternalNode &to, DegreeCountTy to_idx) {
    LeafNode<P>::transfer(this->values_[from], to.values_[to_idx]);
    if (WithRightPointer) {
      to.children_[to_idx + 1] = children_[from + 1];
    }
    if (WithLeftPointer) {
      to.children_[to_idx] = children_[from];
    }
  }
  template <bool WithLeftPointer = false, bool WithRightPointer= false>
  void transfer(DegreeCountTy from, InternalNode *to, DegreeCountTy to_idx) {
    transfer<WithLeftPointer, WithRightPointer>(from, *to, to_idx);
  }
  void dump(int h, int internal_height_, std::ostream & os) {
    LeafNode<P>::dump(os, h);
    if (h != internal_height_) {
      for (DegreeCountTy i = 0; i <= this->node_degree_; ++i) {
        children_[i]->dump(h + 1, internal_height_, os);
      }
    }
  }
  template <bool WithChildren>
  std::pair<InternalNode *, InternalNode *> mergeAt(DegreeCountTy idx) {
    InternalNode *left_child = children_[idx],
                 *right_child = children_[idx + 1];
    IPQ_ASSERT(left_child->node_degree_ == MinNodeDegree);
    IPQ_ASSERT(right_child->node_degree_ == MinNodeDegree);
    transfer(idx, left_child, MinNodeDegree);
    for (DegreeCountTy i = 0; i < MinNodeDegree; ++i) {
      right_child->transfer<WithChildren, false>(
          i, left_child, MinNodeDegree + 1 + i);
    }
    left_child->node_degree_ = MaxNodeDegree;
    if (WithChildren) {
      left_child->children_[MaxNodeDegree] =
          right_child->children_[MinNodeDegree];
    }
    for (DegreeCountTy i = idx; i + 1 < this->node_degree_; ++i) {
      transfer<false, true>(i + 1, this, i);
    }
    --this->node_degree_;
    return {left_child, right_child};
  }
};

template <typename P>
class BTreeImpl : P::LeafNodeAllocTy, P::InternalNodeAllocTy {
  using LeafNodeAllocTy = typename P::LeafNodeAllocTy;
  using InternalNodeAllocTy = typename P::InternalNodeAllocTy;
  using LeafNodeTy = typename P::LeafNodeTy;
  using InternalNodeTy = typename P::InternalNodeTy;
  using ThreeWayCompTy = typename P::ThreeWayCompTy;
  using ValueTy = typename P::ValueTy;
  enum {
    MinNodeDegree = P::MinNodeDegree,
    MaxNodeDegree = P::MaxNodeDegree,
    MinChildDegree = P::MinChildDegree,
    MaxChildDegree = P::MaxChildDegree,
  };
  using DegreeCountTy = typename P::DegreeCountTy;
  std::size_t internal_height_; // mininal value of height is 1 for an empty or
                                // one-level tree
  InternalNodeTy *root_;

  template <typename ContTy>
  ValueTy *find(const ValueTy &target, ContTy & path) {
    //dump(std::cout);
    InternalNodeTy *p = root_;
    for (size_t i = 0; i < internal_height_; ++i) {
      auto res = p->lower_bound(target);
      auto idx = res.first;
      path.emplace_back(p, idx);
      if (res.second) {
        return &p->values_[idx];
      } else {
        p = p->children_[idx];
      }
    }
    auto res = p->lower_bound(target);
    auto idx = res.first;
    path.emplace_back(p, idx);
    if (res.second) {
      return p->values_ + idx;
    } else {
      return nullptr;
    }
  }

  template <typename ContTy>
  std::pair<ValueTy *, bool> addNonFull(const ValueTy &target, ContTy &path) {
    InternalNodeTy *node = root_;
    for (size_t height = 0; height < internal_height_; ++height) {
      auto res = node->lower_bound(target);
      path.emplace_back(node, res.first);
      auto idx = res.first;
      if (res.second) {
        return {&node->values_[idx], false};
      } else {
        InternalNodeTy* child = node->children_[idx];
        if (child->isFull()) {
          if (height + 1 == internal_height_) {
            LeafNodeTy *new_node = this->LeafNodeAllocTy::allocate(1);
            node->template split<false>(idx, static_cast<InternalNodeTy *>(new_node));
          } else {
            InternalNodeTy *new_node = this->InternalNodeAllocTy::allocate(1);
            node->template split<true>(idx, new_node);
          }
          int cmp = ThreeWayCompTy{}(target, node->values_[idx]);
          if (!cmp) {
            return {&node->values_[idx], false};
          } else if (cmp < 0) {
            node = node->children_[idx];
          } else {
            node = node->children_[idx + 1];
          }
        } else {
          node = child;
        }
      }
    }
    return {node->insert(target), true};
  }

  template <typename ContTy>
  std::pair<ValueTy *, bool> add(const ValueTy &target, ContTy &path) {
    if (root_->isFull()) {
      if (internal_height_) {
        InternalNodeTy *new_node1 = this->InternalNodeAllocTy::allocate(1);
        InternalNodeTy *new_node2 = this->InternalNodeAllocTy::allocate(1);
        root_ = root_->template split_root<true>(new_node1, new_node2);
      } else {
        InternalNodeTy *new_node1 = this->InternalNodeAllocTy::allocate(1);
        LeafNodeTy *new_node2 = this->LeafNodeAllocTy::allocate(1);
        root_ = root_->template split_root<false>(
            new_node1, static_cast<InternalNodeTy *>(new_node2));
      }
      ++internal_height_;
    }
    return addNonFull(target, path);
  }

  bool remove(const ValueTy &target) {
    InternalNodeTy *node = root_;
    IPQ_ASSERT(node);
    for (size_t height = 0; height < internal_height_; ++height) {
      auto res = node->lower_bound(target);
      auto idx = res.first;
      if (res.second) {
        InternalNodeTy *left_node = node->children_[idx];
        if (left_node->node_degree_ >= MinChildDegree) {
          node->values_[idx].~ValueTy();
          removePrec(left_node, height, &node->values_[idx]);
          return true;
        } else {
          InternalNodeTy *right_node = node->children_[idx + 1];
          if (right_node->node_degree_ >= MinChildDegree) {
            node->values_[idx].~ValueTy();
            removeSucc(right_node, height, &node->values_[idx]);
            return true;
          } else {
            std::pair<InternalNodeTy*, InternalNodeTy*> res;
            if (height + 1 == internal_height_) {
              res = node->template mergeAt<false>(idx);
              this->LeafNodeAllocTy::deallocate(res.second, 1);
            } else {
              res = node->template mergeAt<true>(idx);
              this->InternalNodeAllocTy::deallocate(res.second, 1);
            }
            if (node == root_ && !node->node_degree_) {
              --internal_height_;
              --height;
              this->InternalNodeAllocTy::deallocate(node, 1);
              root_ = res.first;
            }
            node = res.first;
          }
        }
      } else {
        InternalNodeTy *child = node->children_[idx];
        if (child->node_degree_ == MinNodeDegree) {
          if (idx && node->children_[idx - 1]->node_degree_ != MinNodeDegree) {
            if (height + 1 == internal_height_) {
              node->children_[idx - 1]->template transferToRight<false>(
                  child, node, idx - 1);
            } else {
              node->children_[idx - 1]->template transferToRight<true>(
                  child, node, idx - 1);
            }
            node = child;
          } else if (idx != node->node_degree_ &&
                     node->children_[idx + 1]->node_degree_ != MinNodeDegree) {
            if (height + 1 == internal_height_) {
              node->children_[idx + 1]->template transferToLeft<false>(
                  child, node, idx);
            } else {
              node->children_[idx + 1]->template transferToLeft<true>(
                  child, node, idx);
            }
            node = child;
          } else {
            std::pair<InternalNodeTy*, InternalNodeTy*> res;
            if (height + 1 == internal_height_) {
              res = node->template mergeAt<false>(idx);
              this->LeafNodeAllocTy::deallocate(res.second, 1);
            } else {
              res = node->template mergeAt<true>(idx);
              this->InternalNodeAllocTy::deallocate(res.second, 1);
            }
            if (node == root_ && !root_->node_degree_) {
              --internal_height_;
              --height;
              this->InternalNodeAllocTy::deallocate(root_, 1);
              root_ = res.first;
            }
            node = res.first;
          }
        } else {
          node = child;
        }
      }
    }
    auto res = node->lower_bound(target);
    auto idx = res.first;
    if (res.second) {
      (node->values_ + idx)->~ValueTy();
      node->remove(idx);
      return true;
    } else {
      return false;
    }
  }

  void removePrec(InternalNodeTy *node, size_t height, ValueTy *pos) {
    llvm::SmallVector<InternalNodeTy *, 32> nodes;
    for (; height < internal_height_; ++height) {
      nodes.push_back(node);
      node = node->children_[node->node_degree_];
    }
    LeafNodeTy::transfer(node->values_ + node->node_degree_ - 1, pos);
    --node->node_degree_;
    bool is_leaf = true;
    while (!nodes.empty()) {
      if (node->node_degree_ >= MinNodeDegree) {
        return;
      }
      InternalNodeTy *parent = nodes.back();
      InternalNodeTy *left_sibling = parent->children_[node->node_degree_ - 1];
      if (left_sibling->node_degree_ > MinNodeDegree) {
        if (is_leaf) {
          left_sibling->template transferToRight<true>(
              node, parent, parent->node_degree_ - 1);
        } else {
          left_sibling->template transferToRight<false>(
              node, parent, parent->node_degree_ - 1);
        }
        return;
      } else {
        if (is_leaf) {
          parent->template mergeAt<false>(parent->node_degree_ - 1);
        } else {
          parent->template mergeAt<true>(parent->node_degree_ - 1);
        }
        node = parent;
        nodes.pop_back();
      }
      is_leaf = false;
    }
  }

  void removeSucc(InternalNodeTy *node, size_t height, ValueTy *pos) {
    llvm::SmallVector<InternalNodeTy *, 32> nodes;
    for (; height < internal_height_; ++height) {
      nodes.push_back(node);
      node = node->children_[0];
    }
    LeafNodeTy::transfer(&node->values_[0], pos);
    for (int i = 1; i < node->node_degree_; ++i) {
      LeafNodeTy::transfer(node->values_[i], node->values_[i - 1]);
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
          right_sibling->template transferToLeft<true>(node, parent, 0);
        } else {
          right_sibling->template transferToLeft<false>(node, parent, 0);
        }
        return;
      } else {
        if (is_leaf) {
          parent->template mergeAt<false>(0);
        } else {
          parent->template mergeAt<true>(0);
        }
        node = parent;
        nodes.pop_back();
      }
      is_leaf = false;
    }
  }
  template <typename ElementTy, typename AllocTy, typename ThreeWayCompTy,
            int MinNodeDegree>
  friend class ipq::BTreeSet;

  template <typename KeyTy, typename ValueTy, typename ThreeWayCompTy,
            typename AllocTy, int MinNodeDegree>
  friend class ipq::BTreeMap;

  void dump(std::ostream& os) {
    os << "height: " << internal_height_ << std::endl;
    root_->dump(0, internal_height_, os);
  }

public:
  BTreeImpl()
      : internal_height_(0),
        root_(static_cast<InternalNodeTy *>(new LeafNodeTy)) {
    root_->node_degree_ = 0;
  }
};
} // namespace internal

template <typename ElementTy,
          typename ThreeWayCompTy =
              ThreeWayCompAdaptor<ElementTy, std::less<ElementTy>>,
          typename AllocTy = std::allocator<ElementTy>, int MinChildDegree = 4>
class BTreeSet {
  using Param =
      internal::BTreeParams<MinChildDegree, ElementTy, ThreeWayCompTy, AllocTy>;
  internal::BTreeImpl<Param> btree;

public:
  bool add(ElementTy &e) {
    DummyVector<ElementTy> dummy_vector;
    return btree.add(e, dummy_vector).second;
  }
  bool remove(ElementTy &e) {
    return btree.remove(e);
  }
  bool find(ElementTy &e) {
    DummyVector<ElementTy> dummy_vector;
    return btree.find(e, dummy_vector);
  }
  void dump(std::ostream & os) {
    btree.dump(os);
  }
};

template <typename KeyTy, typename ValueTy,
          typename ThreeWayCompTy = ThreeWayCompAdaptor<
              std::pair<KeyTy, ValueTy>, std::less<std::pair<KeyTy, ValueTy>>>,
          typename AllocTy = std::allocator<std::pair<KeyTy, ValueTy>>,
          int MinChildDegree = 4>
class BTreeMap {};

} // namespace ipq

