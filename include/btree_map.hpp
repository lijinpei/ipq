#pragma once

#include <utility>
#include "btree_impl.hpp"

namespace ipq {

namespace internal {

template <typename KeyTy, typename ValueTy, typename ThreeWayCompTy>
struct KeyValueThreeWayCompareAdaptor : ThreeWayCompTy {
 private:
  using ElementTy = std::pair<KeyTy, ValueTy>;

 public:
  KeyValueThreeWayCompareAdaptor()
      : KeyValueThreeWayCompareAdaptor(ThreeWayCompTy()) {}
  KeyValueThreeWayCompareAdaptor(const ThreeWayCompTy &base_comp)
      : ThreeWayCompTy(base_comp) {}
  constexpr int operator()(const ElementTy &e1, const ElementTy &e2) const {
    return ThreeWayCompTy::operator()(e1.first, e2.first);
  }
};

}  // namespace internal

template <typename KeyTy, typename ValueTy,
          typename ThreeWayCompTy =
              ThreeWayCompAdaptor<KeyTy, std::less<KeyTy>>,
          typename AllocTy = std::allocator<std::pair<const KeyTy, ValueTy>>,
          int MinChildDegree = 4>
class BTreeMap {
  using RealThreeWayComparatorTy =
      internal::KeyValueThreeWayCompareAdaptor<KeyTy, ValueTy, ThreeWayCompTy>;
  using Param = internal::BTreeParams<MinChildDegree, std::pair<KeyTy, ValueTy>,
                                      RealThreeWayComparatorTy, AllocTy>;
  internal::BTreeImpl<Param> btree_;

 public:
  using key_type = KeyTy;
  using mapped_type = ValueTy;
  using value_type = std::pair<key_type, mapped_type>;
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

  BTreeMap() : BTreeMap(ThreeWayCompTy(), AllocTy()) {}
  explicit BTreeMap(const ThreeWayCompTy &comp,
                    const AllocTy &alloc = AllocTy())
      : btree_(comp, alloc) {}
  explicit BTreeMap(const AllocTy &alloc) : BTreeMap(ThreeWayCompTy(), alloc) {}
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

  size_t size() { return btree_.size(); }

  void clear() { btree_.clear(); }

  std::pair<iterator, bool> insert(const value_type &value) {
    iterator iter(btree_);
    auto res = btree_.add(value, iter.path_);
    return {iter, res};
  }

  size_type erase(const key_type &key) {
    value_type value{key, ValueTy()};
    DummyVector path;
    if (btree_.remove(value, path)) {
      return 1;
    } else {
      return 0;
    }
  }

  void erase( iterator pos ) {
    erase(pos->first);
  }

  template< class... Args >
  std::pair<iterator,bool> emplace( Args&&... args ) {
    value_type value(std::forward(arg)...);
    insert(value);
  }

  template <class... Args>
  iterator emplace_hint(const_iterator, Args&&... args ) {
    emplace(std::forward(args)...);
  }

  iterator find(const key_type &key) {
    value_type value{key, ValueTy()};
    iterator ret(btree_);
    auto res = btree_.find(value, ret.path_);
    if (!res) {
      ret.clear();
    }
    return ret;
  }
  iterator lower_bound( const Key& key );
  iterator upper_bound( const Key& key );
};

template <typename KeyTy, typename ValueTy,
          typename ThreeWayCompTy = ThreeWayCompAdaptor<
              std::pair<KeyTy, ValueTy>, std::less<std::pair<KeyTy, ValueTy>>>,
          typename AllocTy = std::allocator<std::pair<KeyTy, ValueTy>>,
          int MinChildDegree = 4>
class BTreeMultiMap {
  // TODO:
};
}  // namespace ipq
