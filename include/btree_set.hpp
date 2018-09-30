#pragma once

#include "btree_impl.hpp"
#include <iostream>

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

template <typename ElementTy,
          typename ThreeWayCompTy =
              ThreeWayCompAdaptor<ElementTy, std::less<ElementTy>>,
          typename AllocTy = std::allocator<ElementTy>, int MinChildDegree = 4>
class BTreeSet {
  using Param =
      internal::BTreeParams<MinChildDegree, ElementTy, ThreeWayCompTy, AllocTy>;
  internal::BTreeImpl<Param> btree;

 public:
  using key_type = ElementTy;
  using value_type = ElementTy;
  using size_type = std::size_t;
  using three_way_key_compare = ThreeWayCompTy;
  using three_way_value_compare = ThreeWayCompTy;
  using allocator_type = AllocTy;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = typename std::allocator_traits<allocator_type>::pointer;
  using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;
  using iterator = internal::BTreeIteratorImpl<Param, false, false>;
  using const_iterator = internal::BTreeIteratorImpl<Param, true, false>;
  using reverse_iterator = internal::BTreeIteratorImpl<Param, false, true>;
  using const_reverse_iterator = internal::BTreeIteratorImpl<Param, true, true>;

  /*
  using node_type = ;
  using inter_return_type = ;
   */

  BTreeSet() : BTreeSet(ThreeWayCompTy(), AllocTy()) {}
  explicit BTreeSet(const ThreeWayCompTy &comp,
                    const AllocTy &alloc = AllocTy())
      : btree(comp, alloc){};
  explicit BTreeSet(const AllocTy &alloc) : BTreeSet(ThreeWayCompTy(), alloc) {}

  iterator begin() {
  }
  const_iterator cbegin() {
  }
  iterator end() {
  }
  const_iterator cend() {
  }
  reverse_iterator rbegin() {
  }
  const_reverse_iterator crbegin() {
  }
  reverse_iterator rend() {
  }
  const_reverse_iterator crend() {
  }

  bool empty() {
    return !size();
  }

  size_type size() {
    return btree.size();
  }

  void clear() {
    btree.clear();
  }

  void erase(const key_type & key) {
    DummyVector path;
    btree.remove(key, path);
  }

  std::pair<iterator, bool> insert(const value_type &  value) {
    iterator iter;
    auto res = btree.add(value, iter.path_);
    iter.internal_height_ = btree.internal_height_;
    return {iter, res};
  }

  void swap(BTreeSet & other) {
    btree.swap(other.btree);
  }

  iterator find(const key_type & key) {
    iterator ret;
    auto res = btree.find(key, ret.path_);
    ret.internal_height_ = btree.internal_height_;
    if (!res) {
      ret.clear();
    }
    return ret;
  }
/*
  const_iterator find(const key_type & key) const {
    iterator ret;
    auto res = btree.find(key, ret.path_);
    ret.internal_height_ = btree.internal_height_;
    if (!res) {
      ret.clear();
    }
    return ret;
  }
*/

  iterator lower_bound(const key_type & key) {
    iterator ret;
    btree.find(key, ret.path_);
    ret.internal_height_ = btree.internal_height_;
    return ret;
  }
/*
  const_iterator lower_bound(const key_type & key) const {
  }
*/

  void dump(std::ostream &os) { btree.dump(os); }
};

}  // namespace ipq
