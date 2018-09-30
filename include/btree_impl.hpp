#pragma once

#include "config.hpp"

#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace ipq {

class DummyVector {
 public:
  // FIXME: template parmater pack
  template <typename... A>
  void push_back(A...) {}
  template <typename... A>
  void emplace_back(A...) {}
  template <typename... A>
  void pop_back(A...) {}
};

template <typename ElementTy, typename CompTy>
struct ThreeWayCompAdaptor : CompTy {
  constexpr int operator()(const ElementTy &e1, const ElementTy &e2) const {
    auto comp1 = this->CompTy::operator()(e1, e2);
    if (comp1) {
      return -1;
    }
    auto comp2 = this->CompTy::operator()(e2, e1);
    return comp2 ? 1 : 0;
  }
};

class DummyVector;
template <typename ElementTy, typename ThreeWayCompTy, typename AllocTy,
          int MinChildDegree>
class BTreeSet;

template <typename ElementTy, typename ThreeWayCompTy, typename AllocTy,
          int MinChildDegree>
class BTreeMultiSet;

template <typename KeyTy, typename ValueTy, typename ThreeWayCompTy,
          typename AllocTy, int MinChildDegree>
class BTreeMap;

template <typename KeyTy, typename ValueTy, typename ThreeWayCompTy,
          typename AllocTy, int MinChildDegree>
class BTreeMultiMap;

namespace internal {

template <typename P>
struct LeafNode;

template <typename P>
struct InternalNode;

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
  using ReferenceTy = ValueTy &;
  using ConstReferenceTy = const ValueTy &;
  using PointerTy = ValueTy *;
  using ConstPointerTy = const ValueTy *;
};

template <typename P>
struct LeafNode {
  using DegreeCountTy = typename P::DegreeCountTy;
  using ThreeWayCompTy = typename P::ThreeWayCompTy;
  using ValueTy = typename P::ValueTy;
  enum {
    MaxNodeDegree = P::MaxNodeDegree,
    MinNodeDegree = P::MinNodeDegree,
    BSearchThreshold = P::BSearchThreshold
  };
  DegreeCountTy node_degree_;
  ValueTy values_[MaxNodeDegree];
  bool isFull() { return node_degree_ == MaxNodeDegree; }
  bool isMinimal() { return node_degree_ == MinNodeDegree; }

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
  DegreeCountTy leafInsert(const ValueTy &value) {
    auto res = lower_bound(value);
    DegreeCountTy idx = res.first;
    for (auto i = node_degree_ - 1; i >= idx; --i) {
      transfer(values_[i], values_[i + 1]);
    }
    ++node_degree_;
    new (values_ + idx) ValueTy(value);
    return idx;
  }

  /* transfer() functions move construct value from from to to, and destruct
   * value at form
   */
  static void transfer(ValueTy *from, ValueTy *to) {
    new (to) ValueTy(std::move(*from));
    from->~ValueTy();
  }
  static void transfer(ValueTy &from, ValueTy &to) { transfer(&from, &to); }
  void transfer(DegreeCountTy from, LeafNode &to, DegreeCountTy to_idx) {
    transfer(this->values_[from], to.values_[to_idx]);
  }
  void transfer(DegreeCountTy from, LeafNode *to, DegreeCountTy to_idx) {
    transfer(from, *to, to_idx);
  }
  void leafRemove(DegreeCountTy idx) {
    for (DegreeCountTy i = idx + 1; i < node_degree_; ++i) {
      transfer(values_[i], values_[i - 1]);
    }
    --node_degree_;
  }
  void dump(std::ostream &os, int h) {
    os << "height: " << h << " elements: " << int(node_degree_) << ' '
       << std::flush;
    for (DegreeCountTy i = 0; i < node_degree_; ++i) {
      os << ' ' << values_[i] << std::flush;
    }
    os << std::endl;
  }
  void clear() {
    for (int i = 0; i < node_degree_; ++i) {
      this->values_[i].~ValueTy();
    }
  }
};

template <typename P>
struct InternalNode : LeafNode<P> {
 private:
  using ValueTy = typename P::ValueTy;
  using LeafNodeTy = LeafNode<P>;
  using DegreeCountTy = typename P::DegreeCountTy;
  enum {
    SplitIndex = P::SplitIndex,
    MaxNodeDegree = P::MaxNodeDegree,
    MinNodeDegree = P::MinNodeDegree,
    MaxChildDegree = P::MaxChildDegree,
    MinChildDegree = P::MinChildDegree
  };

 public:
  InternalNode *children_[MaxChildDegree];
  /* Those four functions move this->values[start..end) to the uninitialized
   * memory start at to_node->values[to_idx].
   * the Right/Left suffix of the function indicates whether the transfer should
   * run from right to left, or left to right.
   * WithLeftChildPtr/WithRightChildPtr indicates for values_[idx], whether
   * children_[idx]/children_[idx + 1] should also be copied
   */
  template <bool WithLeftChildPtr = false, bool WithRightChildPtr = false>
  void rangeTransferRight(DegreeCountTy start, DegreeCountTy end,
                          InternalNode *to_node, DegreeCountTy to_idx) {
    int number = end - start;
    for (int i = number - 1; i >= 0; --i) {
      new (to_node->values_ + to_idx + i)
          ValueTy(std::move(this->values_[start + i]));
      this->values_[start + i].~ValueTy();
      if (WithLeftChildPtr) {
        to_node->children_[to_idx + i] = children_[start + i];
      }
      if (WithRightChildPtr) {
        to_node->children_[to_idx + i + 1] = children_[start + i + 1];
      }
    }
  }

  template <bool WithLeftChildPtr = false, bool WithRightChildPtr = false>
  void rangeTransferRight(DegreeCountTy start, DegreeCountTy end,
                          InternalNode &to_node, DegreeCountTy to_idx) {
    rangeTransferRight<WithLeftChildPtr, WithRightChildPtr>(start, end,
                                                            &to_node, to_idx);
  }

  template <bool WithLeftChildPtr = false, bool WithRightChildPtr = false>
  void rangeTransferLeft(DegreeCountTy start, DegreeCountTy end,
                         InternalNode *to_node, DegreeCountTy to_idx) {
    int number = end - start;
    for (int i = 0; i < number; ++i) {
      new (to_node->values_ + to_idx + i)
          ValueTy(std::move(this->values_[start + i]));
      this->values_[start + i].~ValueTy();
      if (WithLeftChildPtr) {
        to_node->children_[to_idx + i] = children_[start + i];
      }
      if (WithRightChildPtr) {
        to_node->children_[to_idx + i + 1] = children_[start + i + 1];
      }
    }
  }

  template <bool WithLeftChildPtr = false, bool WithRightChildPtr = false>
  void rangeTransferLeft(DegreeCountTy start, DegreeCountTy end,
                         InternalNode &to_node, DegreeCountTy to_idx) {
    rangeTransferLeft<WithLeftChildPtr, WithRightChildPtr>(start, end, &to_node,
                                                           to_idx);
  }

  /* Get a node form the child at index idx.
   * Predicate: see the asserts
   * new_child is uninitialized.
   * WithChildren indicates whether child's a leaf node.
   */
  template <bool WithChildren>
  void splitFromChild(typename P::DegreeCountTy idx, InternalNode *new_child) {
    IPQ_ASSERT(!this->isFull());
    IPQ_ASSERT(idx <= this->node_degree_);
    InternalNode *child = children_[idx];
    IPQ_ASSERT(child && child->isFull());
    child->rangeTransferRight<false, WithChildren>(SplitIndex + 1,
                                                   MaxNodeDegree, new_child, 0);
    if (WithChildren) {
      new_child->children_[0] = child->children_[SplitIndex + 1];
    }
    new_child->node_degree_ = MaxNodeDegree - SplitIndex - 1;
    rangeTransferRight<false, true>(idx, this->node_degree_, this, idx + 1);
    child->transfer(SplitIndex, this, idx);
    children_[idx + 1] = new_child;
    child->node_degree_ = SplitIndex;
    ++this->node_degree_;
  }

  /* Predicate: this is full. this is a root node;
   * new_root, new_child are uninitialzed.
   * if (!WithChildren) this and new_child are LeafNodeTy, new_root is
   * InternalNodeTy. else this, new_child and new_root are InternalNodeTy
   * Post-condition: new_root is the new root. this and new_child is
   * new_root's two children
   */
  template <bool WithChildren>
  InternalNode *splitAsRoot(InternalNode *new_root, InternalNode *new_child) {
    this->transfer(SplitIndex, new_root, 0);
    new_root->node_degree_ = 1;
    new_root->children_[0] = this;
    new_root->children_[1] = new_child;
    rangeTransferLeft<false, WithChildren>(SplitIndex + 1, this->node_degree_,
                                           new_child, 0);
    if (WithChildren) {
      new_child->children_[0] = children_[SplitIndex + 1];
    }
    new_child->node_degree_ = MaxNodeDegree - SplitIndex - 1;
    this->node_degree_ = SplitIndex;
    return new_root;
  }

  /* Those four function transfer one rightmost/leftmost node to its
   * right/left sibling's leftmost/rightmost position
   * this node should not be isMinimal(), sibling should not be isFull();
   * parent is the parent of these two nodes. self_idx is the index of this
   * node.
   */
  template <bool WithChildren>
  void transferOneToRightSibling(InternalNode *sibling, InternalNode *parent,
                                 DegreeCountTy self_idx) {
    IPQ_ASSERT(!this->isMinimal());
    IPQ_ASSERT(sibling && !sibling->isFull());
    IPQ_ASSERT(parent && self_idx < parent->node_degree_);
    IPQ_ASSERT(parent->children_[self_idx] == this &&
               parent->children_[self_idx + 1] == sibling);
    sibling->rangeTransferRight<false, WithChildren>(0, sibling->node_degree_,
                                                     sibling, 1);
    LeafNodeTy::transfer(parent->values_[self_idx], sibling->values_[0]);
    LeafNodeTy::transfer(this->node_degree_ - 1, parent, self_idx);
    if (WithChildren) {
      sibling->children_[1] = sibling->children_[0];
      sibling->children_[0] = children_[this->node_degree_];
    }
    ++sibling->node_degree_;
    --this->node_degree_;
  }
  template <bool WithChildren>
  void transferOneToRightSibling(InternalNode &sibling, InternalNode &parent,
                                 DegreeCountTy self_idx) {
    transferOneToRightSibling<WithChildren>(&sibling, &parent, self_idx);
  }

  template <bool WithChildren>
  void transferOneToLeftSibling(InternalNode &sibling, InternalNode &parent,
                                DegreeCountTy sibling_idx) {
    transferOneToLeftSibling<WithChildren>(&sibling, &parent, sibling_idx);
  }
  template <bool WithChildren>
  void transferOneToLeftSibling(InternalNode *sibling, InternalNode *parent,
                                DegreeCountTy sibling_idx) {
    IPQ_ASSERT(!this->isMinimal());
    IPQ_ASSERT(sibling && !sibling->isFull());
    IPQ_ASSERT(parent && sibling_idx < parent->node_degree_);
    IPQ_ASSERT(parent->children_[sibling_idx + 1] == this &&
               parent->children_[sibling_idx] == sibling);
    LeafNodeTy::transfer(parent->values_[sibling_idx],
                         sibling->values_[sibling->node_degree_]);
    LeafNodeTy::transfer(this->values_, parent->values_ + sibling_idx);
    if (WithChildren) {
      sibling->children_[sibling->node_degree_ + 1] = children_[0];
      children_[0] = children_[1];
    }
    rangeTransferLeft<false, WithChildren>(1, this->node_degree_, this, 0);
    ++sibling->node_degree_;
    --this->node_degree_;
  }

  void dump(int h, int internal_height_, std::ostream &os) {
    LeafNode<P>::dump(os, h);
    if (h != internal_height_) {
      for (DegreeCountTy i = 0; i <= this->node_degree_; ++i) {
        children_[i]->dump(h + 1, internal_height_, os);
      }
    }
  }

  /* Merge the two children at idx, idx - 1 with values_[idx]
   * see asserts for the predicates.
   * WithChildren indicates children have children_ fileds.
   * Postcondition: left_child is the new child, right_child should be
   * deallocated.
   */
  template <bool WithChildren>
  void mergeChildrenAt(DegreeCountTy idx) {
    assert(idx < this->node_degree_);
    InternalNode *left_child = children_[idx],
                 *right_child = children_[idx + 1];
    IPQ_ASSERT(left_child->node_degree_ == MinNodeDegree);
    IPQ_ASSERT(right_child->node_degree_ == MinNodeDegree);
    this->transfer(idx, left_child, MinNodeDegree);
    right_child->rangeTransferLeft<WithChildren, false>(
        0, MinNodeDegree, left_child, MinNodeDegree + 1);
    left_child->node_degree_ = MaxNodeDegree;
    if (WithChildren) {
      left_child->children_[MaxNodeDegree] =
          right_child->children_[MinNodeDegree];
    }
    rangeTransferLeft<false, true>(idx + 1, this->node_degree_, this, idx);
    --this->node_degree_;
  }
};

template <typename P, typename ContTy>
void prev_path(ContTy &path, size_t height) {
  if (path.empty()) {
    return;
  }
  if (path.size() == height + 1) {
    if (path.back().second) {
      --path.back().second;
      return;
    }
    path.pop_back();
    while (!path.empty() && !path.back().second) {
      path.pop_back();
    }
    if (!path.empty()) {
      --path.back().second;
    }
    return;
  }
  typename P::InternalNodeTy *node =
      path.back().first->children_[path.back().second];
  while (path.size() < height) {
    typename P::DegreeCountTy idx = node->node_degree_;
    path.emplace_back(node, idx);
    node = node->children_[idx];
  }
  typename P::DegreeCountTy idx = node->node_degree_ - 1;
  path.emplace_back(node, idx);
}

template <typename P>
void prev_path(DummyVector &, size_t) {}

template <typename P, typename ContTy>
void next_path(ContTy &path, size_t height) {
  if (path.empty()) {
    return;
  }
  if (path.size() == height + 1) {
    if (path.back().second == path.back().first->node_degree_ - 1) {
      path.pop_back();
      while (!path.empty() &&
             path.back().second == path.back().first->node_degree_) {
        path.pop_back();
      }
      return;
    } else {
      ++path.back().second;
      return;
    }
  } else {
    ++path.back().second;
    typename P::InternalNodeTy *node =
        path.back().first->children_[path.back().second];
    for (size_t h = path.size(); h <= height; ++h) {
      path.emplace_back(node, typename P::DegreeCountTy(0));
      node = node->children_[0];
    }
    return;
  }
}

template <typename P>
void next_path(DummyVector &, size_t) {}

template <typename P>
class BTreeImpl;

template <typename P, bool IsConst, bool IsReverse>
class BTreeIteratorImpl {
  using InternalNodeTy = typename P::InternalNodeTy;
  using DegreeCountTy = typename P::DegreeCountTy;
  using BTreeImplTy = BTreeImpl<P>;
  enum { MaxNodeDegree = P::MaxNodeDegree, MinNodeDegree = P::MinNodeDegree };
  std::vector<std::pair<InternalNodeTy *, DegreeCountTy>> path_;
  BTreeImplTy *btree_;
  template <typename ElementTy, typename AllocTy, typename ThreeWayCompTy,
            int MinNodeDegree>
  friend class ipq::BTreeSet;

  template <typename ElementTy, typename AllocTy, typename ThreeWayCompTy,
            int MinNodeDegree>
  friend class ipq::BTreeMultiSet;

  template <typename KeyTy, typename ValueTy, typename ThreeWayCompTy,
            typename AllocTy, int MinNodeDegree>
  friend class ipq::BTreeMap;

  template <typename KeyTy, typename ValueTy, typename ThreeWayCompTy,
            typename AllocTy, int MinNodeDegree>
  friend class ipq::BTreeMultiMap;

  void clear() {
    path_.clear();
  }
 public:
  BTreeIteratorImpl() = default;
  BTreeIteratorImpl(const BTreeIteratorImpl &other) = default;
  BTreeIteratorImpl &operator=(const BTreeIteratorImpl &other) = default;
  explicit BTreeIteratorImpl(BTreeImplTy* btree) : btree_(btree) {}
  explicit BTreeIteratorImpl(BTreeImplTy& btree) : btree_(&btree) {}
  void swap(BTreeIteratorImpl &other) {
    path_.swap(other.path_);
    std::swap(btree_, other.btree_);
  }
  using value_type = typename P::ValueTy;
  using reference = typename P::ReferenceTy;
  using const_reference = typename P::ConstReferenceTy;
  using pointer = typename P::PointerTy;
  using iterator_category = std::bidirectional_iterator_tag;
  typename std::conditional<IsConst, const_reference, reference>::type operator
      *() {
    return path_.back().first->values_[path_.back().second];
  }
  typename std::conditional<IsConst, const value_type *, value_type *>::type
  operator->() {
    return path_.back().first->values_ + path_.back().second;
  }
  BTreeIteratorImpl &operator++() {
    if constexpr (IsReverse) {
      prev_path<P>(path_, btree_->internal_height_);
    } else {
      next_path<P>(path_, btree_->internal_height_);
    }
    return *this;
  }
  const BTreeIteratorImpl &operator++(int) {
    BTreeIteratorImpl tmp(*this);
    ++*this;
    return tmp;
  }
  BTreeIteratorImpl &operator--() {
    if constexpr (IsReverse) {
      next_path<P>(path_);
    } else {
      prev_path<P>(path_);
    }
    return *this;
  }
  const BTreeIteratorImpl &operator--(int) {
    BTreeIteratorImpl tmp(*this);
    --*this;
    return tmp;
  }
  bool operator==(const BTreeIteratorImpl &other) const {
    return btree_ == other.btree_ && path_ == other.path_;
  }
  bool operator!=(const BTreeIteratorImpl &other) const {
    return !(*this == other);
  }
};

template <typename P>
class BTreeImpl : P::LeafNodeAllocTy,
                  P::InternalNodeAllocTy,
                  P::ThreeWayCompTy {
  using LeafNodeAllocTy = typename P::LeafNodeAllocTy;
  using InternalNodeAllocTy = typename P::InternalNodeAllocTy;
  using LeafNodeTy = typename P::LeafNodeTy;
  using InternalNodeTy = typename P::InternalNodeTy;
  using ThreeWayCompTy = typename P::ThreeWayCompTy;
  using ValueTy = typename P::ValueTy;
  using DegreeCountTy = typename P::DegreeCountTy;
  enum {
    MinNodeDegree = P::MinNodeDegree,
    MaxNodeDegree = P::MaxNodeDegree,
    MinChildDegree = P::MinChildDegree,
    MaxChildDegree = P::MaxChildDegree,
  };

  // height of internal nodes
  std::size_t internal_height_, size_;
  InternalNodeTy *root_;

  template <typename ContTy>
  ValueTy *find(const ValueTy &target, ContTy &path) {
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
    if (res.second) {
      path.emplace_back(p, idx);
      return p->values_ + idx;
    } else {
      if (idx == p->node_degree_) {
        path.emplace_back(p, DegreeCountTy(idx - 1));
        next_path<P>(path, internal_height_);
      } else {
        path.emplace_back(p, idx);
      }
      return nullptr;
    }
  }

  /* Merge parent's two children at idx and idx + 1.
   * Predicate: parent->node_degree_ > MinNodeDegree
   *            left_child->node_degree_  == MinNodeDegree
   *            right_child->node_degree_ == MinNodeDegree
   * return the new child
   */
  template <typename ConTy>
  InternalNodeTy *mergeChildrenAt(InternalNodeTy *parent, DegreeCountTy idx,
                                  size_t &parent_height, ConTy &path) {
    IPQ_ASSERT(parent == root_ || parent->node_degree_ > MinNodeDegree);
    IPQ_ASSERT(idx < parent->node_degree_);
    InternalNodeTy *left_child = parent->children_[idx],
                   *right_child = parent->children_[idx + 1];
    IPQ_ASSERT(left_child->node_degree_ == MinNodeDegree);
    IPQ_ASSERT(right_child->node_degree_ == MinNodeDegree);
    if (parent_height + 1 == internal_height_) {
      parent->template mergeChildrenAt<false>(idx);
      this->LeafNodeAllocTy::deallocate(right_child, 1);
    } else {
      parent->template mergeChildrenAt<true>(idx);
      this->InternalNodeAllocTy::deallocate(right_child, 1);
    }
    if (root_->node_degree_ == 0) {
      this->InternalNodeAllocTy::deallocate(root_, 1);
      root_ = left_child;
      --internal_height_;
      --parent_height;
      path.pop_back();
    }
    return left_child;
  }

  /* Predicate: parent == root_ || !parent->isMinmal()
   * This function makes parent's idx-th not minimal by stealing a node from
   * child' sibling or transfer a node from parent
   * return: ret.first: whether internal height has been decreased
   *         ret.second: idx of the new child
   */
  template <typename ConTy>
  InternalNodeTy *tryMakeChildNonMinimal(InternalNodeTy *parent,
                                         size_t &parent_height,
                                         DegreeCountTy idx, ConTy &path) {
    IPQ_ASSERT(parent == root_ || !parent->isMinimal());
    InternalNodeTy *child = parent->children_[idx];
    if (!child->isMinimal()) {
      return child;
    }
    if (idx) {
      if (InternalNodeTy *left_child = parent->children_[idx - 1];
          !left_child->isMinimal()) {
        if (parent_height + 1 < internal_height_) {
          left_child->template transferOneToRightSibling<true>(child, parent,
                                                               idx - 1);
        } else {
          left_child->template transferOneToRightSibling<false>(child, parent,
                                                                idx - 1);
        }
        return child;
      }
    }

    if (idx + 1 <= parent->node_degree_) {
      if (InternalNodeTy *right_child = parent->children_[idx + 1];
          right_child->node_degree_ > MinNodeDegree) {
        if (parent_height + 1 < internal_height_) {
          right_child->template transferOneToLeftSibling<true>(child, parent,
                                                               idx);
        } else {
          right_child->template transferOneToLeftSibling<false>(child, parent,
                                                                idx);
        }
        return child;
      }
    }
    DegreeCountTy idx1 = idx ? idx - 1 : idx;
    return mergeChildrenAt(parent, idx1, parent_height, path);
  }

  template <typename ContTy>
  bool addNonFull(const ValueTy &target, ContTy &path) {
    IPQ_ASSERT(!root_->isFull());
    InternalNodeTy *node = root_;
    for (size_t height = 0; height < internal_height_; ++height) {
      IPQ_ASSERT(!node->isFull());
      auto res = node->lower_bound(target);
      auto idx = res.first;
      path.emplace_back(node, idx);
      if (res.second) {
        path.emplace_back(node, idx);
        return false;
      } else {
        InternalNodeTy *child = node->children_[idx];
        if (child->isFull()) {
          if (height + 1 == internal_height_) {
            LeafNodeTy *new_node = this->LeafNodeAllocTy::allocate(1);
            node->template splitFromChild<false>(
                idx, static_cast<InternalNodeTy *>(new_node));
          } else {
            InternalNodeTy *new_node = this->InternalNodeAllocTy::allocate(1);
            node->template splitFromChild<true>(idx, new_node);
          }
          int cmp =
              this->ThreeWayCompTy::operator()(target, node->values_[idx]);
          if (!cmp) {
            path.emplace_back(node, idx);
            return false;
          } else if (cmp < 0) {
            path.emplace_back(node, idx);
            node = node->children_[idx];
          } else {
            path.emplace_back(node, idx + 1);
            node = node->children_[idx + 1];
          }
        } else {
          path.emplace_back(node, idx);
          node = child;
        }
      }
    }
    DegreeCountTy idx = node->leafInsert(target);
    path.emplace_back(node, idx);
    ++size_;
    return true;
  }

  template <typename ContTy>
  bool add(const ValueTy &target, ContTy &path) {
    if (root_->isFull()) {
      if (internal_height_) {
        InternalNodeTy *new_node1 = this->InternalNodeAllocTy::allocate(1);
        InternalNodeTy *new_node2 = this->InternalNodeAllocTy::allocate(1);
        root_ = root_->template splitAsRoot<true>(new_node1, new_node2);
      } else {
        InternalNodeTy *new_node1 = this->InternalNodeAllocTy::allocate(1);
        LeafNodeTy *new_node2 = this->LeafNodeAllocTy::allocate(1);
        root_ = root_->template splitAsRoot<false>(
            new_node1, static_cast<InternalNodeTy *>(new_node2));
      }
      ++internal_height_;
    }
    return addNonFull(target, path);
  }

  template <typename ContTy>
  bool remove(const ValueTy &target, ContTy &path) {
    InternalNodeTy *node = root_;
    IPQ_ASSERT(node);
    for (size_t height = 0; height < internal_height_; ++height) {
      auto res = node->lower_bound(target);
      auto idx = res.first;
      InternalNodeTy *left_node = node->children_[idx];
      if (res.second) {
        if (left_node->node_degree_ > MinNodeDegree) {
          node->values_[idx].~ValueTy();
          path.emplace_back(node, idx);
          removePrec(left_node, height + 1, &node->values_[idx], path);
          next_path<P>(path, internal_height_);
          --size_;
          return true;
        } else if (InternalNodeTy *right_node = node->children_[idx + 1];
                   right_node->node_degree_ > MinNodeDegree) {
          node->values_[idx].~ValueTy();
          path.emplace_back(node, idx);
          removeSucc(right_node, height + 1, &node->values_[idx], path);
          --size_;
          return true;
        } else {
          InternalNodeTy *new_child = mergeChildrenAt(node, idx, height, path);
          path.emplace_back(node, idx);
          node = new_child;
        }
      } else {
        auto new_child = tryMakeChildNonMinimal(node, height, idx, path);
        path.emplace_back(node, res.second);
        node = new_child;
      }
      IPQ_ASSERT(!node->isMinimal());
    }
    auto res = node->lower_bound(target);
    if (res.second) {
      node->leafRemove(res.first);
      --size_;
      return true;
    } else {
      return false;
    }
  }

  template <typename ContTy>
  void removePrec(InternalNodeTy *node, size_t height, ValueTy *pos,
                  ContTy &path) {
    IPQ_ASSERT(node == root_ || !node->isMinimal());
    for (; height < internal_height_; ++height) {
      node = tryMakeChildNonMinimal(node, height, node->node_degree_, path);
    }
    LeafNodeTy::transfer(node->values_ + node->node_degree_ - 1, pos);
    --node->node_degree_;
  }

  template <typename ContTy>
  void removeSucc(InternalNodeTy *node, size_t height, ValueTy *pos,
                  ContTy &path) {
    IPQ_ASSERT(!node->isMinimal());
    for (; height < internal_height_; ++height) {
      node = tryMakeChildNonMinimal(node, height, 0, path);
    }
    LeafNodeTy::transfer(&node->values_[0], pos);
    node->rangeTransferLeft(1, node->node_degree_, node, 0);
    --node->node_degree_;
  }

  template <typename, bool, bool>
  friend class BTreeIteratorImpl;

  template <typename ElementTy, typename AllocTy, typename ThreeWayCompTy,
            int MinNodeDegree>
  friend class ipq::BTreeSet;

  template <typename ElementTy, typename AllocTy, typename ThreeWayCompTy,
            int MinNodeDegree>
  friend class ipq::BTreeMultiSet;

  template <typename KeyTy, typename ValueTy, typename ThreeWayCompTy,
            typename AllocTy, int MinNodeDegree>
  friend class ipq::BTreeMap;

  template <typename KeyTy, typename ValueTy, typename ThreeWayCompTy,
            typename AllocTy, int MinNodeDegree>
  friend class ipq::BTreeMultiMap;

  void dump(std::ostream &os) {
    os << "height: " << internal_height_ << " size: " << size_ << std::endl;
    root_->dump(0, internal_height_, os);
  }

  void clear(InternalNodeTy *node, size_t height) {
    if (height == internal_height_) {
      static_cast<LeafNodeTy *>(node)->clear();
      this->LeafNodeAllocTy::deallocate(node, 1);
      return;
    }
    for (int i = 0, is = node->node_degree_; i <= is; ++i) {
      clear(node->children_[i], height + 1);
    }
    static_cast<LeafNodeTy *>(node)->clear();
    this->InternalNodeAllocTy::deallocate(node, 1);
  }

  template <typename ContTy>
  void begin(ContTy &path) {
    if (!size()) {
      return;
    }
    InternalNodeTy *node = root_;
    for (size_t h = 0; h < internal_height_; ++h) {
      path.emplace_back(node, DegreeCountTy(0));
      node = node->children_[0];
    }
    path.emplace_back(node, DegreeCountTy(0));
    return;
  }

  template <typename ContTy>
  void end(ContTy &) {
    return;
  }

  template <typename ContTy>
  void rbegin(ContTy &path) {
    if (!size()) {
      return;
    }
    InternalNodeTy *node = root_;
    for (size_t h = 0; h < internal_height_; ++h) {
      DegreeCountTy idx = node->node_degree_;
      path.emplace_back(node, idx);
      node = node->children_[idx];
    }
    path.emplace_back(node, node->node_degree_ - 1);
    return;
  }

  template <typename ContTy>
  void rend(ContTy &) {
    return;
  }

 public:
  template <typename Alloc>
  explicit BTreeImpl(const ThreeWayCompTy &comp, const Alloc &alloc)
      : LeafNodeAllocTy(alloc),
        InternalNodeAllocTy(alloc),
        ThreeWayCompTy(comp),
        internal_height_(0),
        size_(0),
        root_(static_cast<InternalNodeTy *>(new LeafNodeTy)) {
    root_->node_degree_ = 0;
  }
  ~BTreeImpl() {
    clear();
  }
  void clear() {
    clear(root_, 0);
    internal_height_ = 0;
    root_ = static_cast<InternalNodeTy*>(this->LeafNodeAllocTy::allocate(1));
    size_ = 0;
  }

  size_t size() { return size_; }
};

}  // namespace internal
}  // namespace ipq

