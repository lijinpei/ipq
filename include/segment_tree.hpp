#pragma once

#include <memory>

namespace ipq {

template <typename ValueTy>
struct SegmentTreeTrait {
  /*
  static void initialNonExist(ValueTy* storage, size_t size);
  static bool isUnmarkValue(const ValueTy& storage);
  static bool isNonExistValue(const ValueTy& storage);
  static void copyUnmarkValue(ValueTy& val);
  static void copyNonExistValue(ValueTy& val);
  static ValueTy getNonExitValue();
   */
};

template <typename ValueTy, typename AllocTy = std::allocator<ValueTy>>
class SegmentTree : AllocTy::template rebind<ValueTy>::other {
  using Trait = SegmentTreeTrait<ValueTy>;
  size_t left_edge_, right_edge_;
  ValueTy* storage_;

  static size_t leftChild(size_t pos) {
    return (pos << 1) + 1;
  }

  static size_t rightChild(size_t pos) {
    return (pos << 1) + 2;
  }

  static size_t leftMiddle(size_t left, size_t right) {
    return (right - left - 1) / 2 + left;
  }

  static size_t rightMiddle(size_t left, size_t right) {
    return leftMiddle(left, right) + 1;
  }

  void downMarkLeft(ValueTy & parent_mark, size_t pos) {
    if (Trait::isUnmarkValue(parent_mark)) {
      parent_mark = storage_[leftChild(pos)];
    } else {
      size_t lc = leftChild(pos), rc = rightChild(pos);
      storage_[lc] = storage_[rc] = parent_mark;
    }
  }

  void downMarkRight(ValueTy & parent_mark, size_t pos) {
    if (Trait::isUnmarkValue(parent_mark)) {
      parent_mark = storage_[rightChild(pos)];
    } else {
      size_t lc = leftChild(pos), rc = rightChild(pos);
      storage_[lc] = storage_[rc] = parent_mark;
    }
  }

  void downMark(const ValueTy &parent_mark, size_t pos) {
    if (!Trait::isUnmarkValue(parent_mark)) {
      size_t lc = leftChild(pos), rc = rightChild(pos);
      storage_[lc] = storage_[rc] = parent_mark;
    }
  }

 public:
  SegmentTree(size_t left, size_t right, const AllocTy& alloc = std::allocator<ValueTy>())
      : AllocTy(alloc), left_edge_(left), right_edge_(right) {
    size_t s = 2 * size();
    storage_ = this->AllocTy::allocate(s);
    Trait::initialNonExist(storage_, s);
  }

  size_t size() { return right_edge_ - left_edge_ + 1; }

  /* use const ValueTy* as a work-around for std::optional
   */
  const ValueTy* find(size_t point) {
    size_t pos = 0, left = left_edge_, right = right_edge_;
    ValueTy mark = Trait::getUnmarkValue();
    while (left < right) {
      if (!Trait::isUnmarkValue(mark)) {
        break;
      } else {
        size_t middle = leftMiddle(left, right);
        if (point <= middle) {
          downMarkLeft(mark, pos);
          pos = leftChild(pos);
          right = middle;
        } else {
          downMarkRight(mark, pos);
          pos = rightChild(pos);
          left = middle + 1;
        }
      }
    }
    if (Trait::isNonExistValue(mark)) {
      return nullptr;
    } else {
      return storage_ + pos;
    }
  }

  void remove(size_t left, size_t right) {
    update(left, right, Trait::getNonExitValue());
  }

  void update(size_t left, size_t right, const ValueTy& val) {
    size_t left_edge = left_edge_, right_edge = right_edge_, pos = 0;
    if (left == left_edge && right == right_edge) {
      storage_[0] = val;
      return;
    }
    ValueTy mark = storage_[0];
    while (left_edge < right_edge) {
      size_t middle = leftMiddle(left_edge, right_edge);
      if (left > middle) {
        downMarkRight(mark, pos);
        pos = rightChild(pos);
        left_edge = middle + 1;
      } else if (right <= middle) {
        downMarkRight(mark, pos);
        pos = leftChild(pos);
        right_edge = middle;
      } else {
        break;
      }
    }
    if (left_edge == right_edge) {
      storage_[pos] = val;
      return;
    }
    downMark(mark, pos);
    size_t middle = leftMiddle(left_edge, right_edge);
    auto leftPartialUpdate = [&](size_t pos, size_t le, size_t re) {
      ValueTy mark = storage_[pos];
      while (le < re) {
        downMark(mark, pos);
        size_t middle = leftMiddle(le, re);
        if (left <= middle) {
          storage_[rightChild(pos)] = val;
          re = middle;
          pos = leftChild(pos);
        } else {
          le = middle + 1;
          pos = rightChild(pos);
        }
      }
      if (le == re) {
        storage_[pos] = val;
      }
    };

    auto rightPartialUpdate = [&](size_t pos, size_t le, size_t re) {
      ValueTy mark = storage_[pos];
      while (le < re) {
        downMark(mark, pos);
        size_t middle = leftMiddle(le, re);
        if (right > middle) {
          storage_[leftChild(pos)] = val;
          le = middle + 1;
          pos = rightChild(pos);
        } else {
          re = middle;
          pos = leftChild(pos);
        }
      }
      if (le == re) {
        storage_[pos] = val;
      }
    };
    leftPartialUpdate(leftChild(pos), left_edge, middle);
    rightPartialUpdate(leftChild(pos), middle + 1, right_edge);
  }
};
}  // namespace ipq
