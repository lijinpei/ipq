#pragma once

#include <iostream>
#include "btree_impl.hpp"

namespace ipq {


template <typename ElementTy,
          typename ThreeWayCompTy =
              ThreeWayCompAdaptor<ElementTy, std::less<ElementTy>>,
          typename AllocTy = std::allocator<ElementTy>, int MinChildDegree = 4>
class BTreeSet {
  using Param =
      internal::BTreeParams<MinChildDegree, ElementTy, ThreeWayCompTy, AllocTy>;
  internal::BTreeImpl<Param> btree_;

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
  using const_pointer =
      typename std::allocator_traits<allocator_type>::const_pointer;
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
      : btree_(comp, alloc){};
  explicit BTreeSet(const AllocTy &alloc) : BTreeSet(ThreeWayCompTy(), alloc) {}

  iterator begin() {
    iterator iter(btree_);
    btree_.begin(iter.path_);
    return iter;
  }
  const_iterator cbegin() {}
  iterator end() {
    iterator iter(btree_);
    btree_.end(iter.path_);
    return iter;
  }
  /*
  const_iterator cend() {
  }
  */
  reverse_iterator rbegin() {
    reverse_iterator iter(btree_);
    btree_.rbegin(iter.path_);
    return iter;
  }
  /*
  const_reverse_iterator crbegin() {
  }
  */
  reverse_iterator rend() {
    reverse_iterator iter(btree_);
    btree_.rend(iter.path_);
    return iter;
  }
  /*
  const_reverse_iterator crend() {
  }
  */

  bool empty() { return !size(); }

  size_type size() { return btree_.size(); }

  void clear() { btree_.clear(); }

  size_t erase(const key_type &key) {
    DummyVector path;
    if (btree_.remove(key, path)) {
      return 1;
    } else {
      return 0;
    }
  }

  iterator erase(iterator pos) {
    btree_.remove(pos.path_);
    return pos;
  }

  std::pair<iterator, bool> insert(const value_type &value) {
    iterator iter(btree_);
    auto res = btree_.add(value, iter.path_);
    return {iter, res};
  }

  void swap(BTreeSet &other) { btree_.swap(other.btree_); }

  iterator find(const key_type &key) {
    iterator ret(btree_);
    auto res = btree_.lowerBound(key, ret.path_);
    if (!res) {
      ret.clear();
    }
    return ret;
  }
  /*
    const_iterator find(const key_type & key) const {
    }
  */

  iterator lower_bound(const key_type &key) {
    iterator ret(btree_);
    btree_.lowerBound(key, ret.path_);
    return ret;
  }
  /*
    const_iterator lower_bound(const key_type & key) const {
    }
  */
  iterator upper_bound(const key_type &key) {
    iterator ret(btree_);
    btree_.upperBound(key, ret.path_);
    return ret;
  }

  void dump(std::ostream &os) { btree_.dump(os); }
};

}  // namespace ipq
