#pragma once

#include "config.hpp"

#include <llvm/ADT/SmallVector.h>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <type_traits>
#include <utility>

namespace ipq {

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

template <typename T>
class DummyVector {
 public:
  void push_back(const T &) {}
  template <typename... A>
  void emplace_back(A...) {}
};

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
  ValueTy *leafInsert(const ValueTy &value) {
    auto res = lower_bound(value);
    DegreeCountTy idx = res.first;
    for (auto i = node_degree_ - 1; i >= idx; --i) {
      transfer(values_[i], values_[i + 1]);
    }
    ++node_degree_;
    new (values_ + idx) ValueTy(value);
    return values_ + idx;
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
  std::size_t internal_height_;
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
    path.emplace_back(p, idx);
    if (res.second) {
      return p->values_ + idx;
    } else {
      return nullptr;
    }
  }

  /* Merge parent's two children at idx and idx + 1.
   * Predicate: parent->node_degree_ > MinNodeDegree
   *            left_child->node_degree_  == MinNodeDegree
   *            right_child->node_degree_ == MinNodeDegree
   */
  InternalNodeTy *mergeChildrenAt(InternalNodeTy *parent, DegreeCountTy idx,
                                  size_t &parent_height) {
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
    }
    return left_child;
  }

  /* Predicate: parent == root_ || !parent->isMinmal()
   * This function makes parent's idx-th not minimal by stealing a node from
   * child' sibling or transfer a node from parent
   */
  InternalNodeTy *tryMakeChildNonMinimal(InternalNodeTy *parent,
                                         size_t &parent_height,
                                         DegreeCountTy idx) {
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
    return mergeChildrenAt(parent, idx ? idx - 1 : idx, parent_height);
  }

  template <typename ContTy>
  std::pair<ValueTy *, bool> addNonFull(const ValueTy &target, ContTy &path) {
    IPQ_ASSERT(!root_->isFull());
    InternalNodeTy *node = root_;
    for (size_t height = 0; height < internal_height_; ++height) {
      IPQ_ASSERT(!node->isFull());
      auto res = node->lower_bound(target);
      auto idx = res.first;
      path.emplace_back(node, idx);
      if (res.second) {
        return {&node->values_[idx], false};
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
    return {node->leafInsert(target), true};
  }

  template <typename ContTy>
  std::pair<ValueTy *, bool> add(const ValueTy &target, ContTy &path) {
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

  bool remove(const ValueTy &target) {
    InternalNodeTy *node = root_;
    IPQ_ASSERT(node);
    for (size_t height = 0; height < internal_height_; ++height) {
      auto res = node->lower_bound(target);
      auto idx = res.first;
      InternalNodeTy *left_node = node->children_[idx];
      if (res.second) {
        if (left_node->node_degree_ > MinNodeDegree) {
          node->values_[idx].~ValueTy();
          removePrec(left_node, height + 1, &node->values_[idx]);
          return true;
        } else if (InternalNodeTy *right_node = node->children_[idx + 1];
                   right_node->node_degree_ > MinNodeDegree) {
          node->values_[idx].~ValueTy();
          removeSucc(right_node, height + 1, &node->values_[idx]);
          return true;
        } else {
          node = mergeChildrenAt(node, idx, height);
        }
      } else {
        node = tryMakeChildNonMinimal(node, height, idx);
      }
      IPQ_ASSERT(!node->isMinimal());
    }
    auto res = node->lower_bound(target);
    if (res.second) {
      node->leafRemove(res.first);
      return true;
    } else {
      return false;
    }
  }

  void removePrec(InternalNodeTy *node, size_t height, ValueTy *pos) {
    IPQ_ASSERT(!node->isMinimal());
    for (; height < internal_height_; ++height) {
      node = tryMakeChildNonMinimal(node, height, node->node_degree_ - 1);
    }
    LeafNodeTy::transfer(node->values_ + node->node_degree_ - 1, pos);
    --node->node_degree_;
  }

  void removeSucc(InternalNodeTy *node, size_t height, ValueTy *pos) {
    IPQ_ASSERT(!node->isMinimal());
    for (; height < internal_height_; ++height) {
      node = tryMakeChildNonMinimal(node, height, 0);
    }
    LeafNodeTy::transfer(&node->values_[0], pos);
    node->rangeTransferLeft(1, node->node_degree_, node, 0);
    --node->node_degree_;
  }

  template <typename ElementTy, typename AllocTy, typename ThreeWayCompTy,
            int MinNodeDegree>
  friend class ipq::BTreeSet;

  template <typename KeyTy, typename ValueTy, typename ThreeWayCompTy,
            typename AllocTy, int MinNodeDegree>
  friend class ipq::BTreeMap;

  void dump(std::ostream &os) {
    os << "height: " << internal_height_ << std::endl;
    root_->dump(0, internal_height_, os);
  }

 public:
  BTreeImpl()
      : internal_height_(0),
        root_(static_cast<InternalNodeTy *>(new LeafNodeTy)) {
    root_->node_degree_ = 0;
  }
};  // namespace internal
}  // namespace internal

template <typename ElementTy,
          typename ThreeWayCompTy =
              ThreeWayCompAdaptor<ElementTy, std::less<ElementTy>>,
          typename AllocTy = std::allocator<ElementTy>, int MinChildDegree = 4>
class BTreeSet {
  using Param =
      internal::BTreeParams<MinChildDegree, ElementTy, ThreeWayCompTy, AllocTy>;
  internal::BTreeImpl<Param> btree;

 public:
  bool add(const ElementTy &e) {
    DummyVector<ElementTy> dummy_vector;
    return btree.add(e, dummy_vector).second;
  }
  bool remove(const ElementTy &e) { return btree.remove(e); }
  bool find(const ElementTy &e) {
    DummyVector<ElementTy> dummy_vector;
    return btree.find(e, dummy_vector);
  }
  void dump(std::ostream &os) { btree.dump(os); }
};

template <typename KeyTy, typename ValueTy,
          typename ThreeWayCompTy = ThreeWayCompAdaptor<
              std::pair<KeyTy, ValueTy>, std::less<std::pair<KeyTy, ValueTy>>>,
          typename AllocTy = std::allocator<std::pair<KeyTy, ValueTy>>,
          int MinChildDegree = 4>
class BTreeMap {};

}  // namespace ipq

