#pragma once

#include <memory>

namespace ipq {

template <typename ValueTy>
struct SegmentTreeTrait {
  /*
  static void initialNonExist(ValueTy* storage);
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

  void downMark(size_t pos) {
    if (!Trait::isUnmarkValue(storage_[pos])) {
      size_t lc = leftChild(pos), rc = rightChild(pos);
      storage_[lc] = storage_[rc] = storage_[pos];
    }
  }

 public:
  SegmentTree(size_t left, size_t right, const AllocTy& alloc = std::allocator<ValueTy>())
      : AllocTy(alloc), left_edge_(left), right_edge_(right) {
    size_t s = 2 * size();
    storage_ = this->AllocTy::allocate(s);
    Trait::initialNonExist(storage_);
  }

  size_t size() { return right_edge_ - left_edge_ + 1; }

  /* use const ValueTy* as a work-around for std::optional
   */
  const ValueTy* find(size_t point) {
    size_t pos = 0, left = left_edge_, right = right_edge_;
    while (left < right) {
      if (!Trait::isUnmarkValue(storage_[pos])) {
        break;
      } else {
        downMark(pos);
        size_t middle = leftMiddle(left, right);
        if (point <= middle) {
          pos = leftChild(pos);
          right = middle;
        } else {
          pos = rightChild(pos);
          left = middle + 1;
        }
      }
    }
    if (Trait::isNonExistValue(storage_[pos])) {
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
    while (left_edge < right_edge) {
      if (left == left_edge && right == right_edge) {
        storage_[pos] = val;
        return;
      }
      size_t middle = leftMiddle(left_edge, right_edge);
      if (left > middle) {
        downMark(pos);
        Trait::copyUnmarkValue(storage_[pos]);
        pos = rightChild(pos);
        left_edge = middle + 1;
      } else if (right <= middle) {
        downMark(pos);
        Trait::copyUnmarkValue(storage_[pos]);
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
    downMark(pos);
    Trait::copyUnmarkValue(storage_[pos]);
    size_t middle = leftMiddle(left_edge, right_edge);
    auto leftPartialUpdate = [&](size_t pos, size_t le, size_t re) {
      while (le < re) {
        if (left == le) {
          storage_[pos] = val;
          return;
        }
        downMark(pos);
        Trait::copyUnmarkValue(storage_[pos]);
        size_t middle = leftMiddle(le, re);
        if (left <= middle + 1) {
          storage_[rightChild(pos)] = val;
          re = middle;
          pos = leftChild(pos);
        } else {
          le = middle + 1;
          pos = rightChild(pos);
        }
        if (left > re) {
          return;
        }
      }
      assert(le == re);
      if (le == re) {
        storage_[pos] = val;
      }
    };

    auto rightPartialUpdate = [&](size_t pos, size_t le, size_t re) {
      while (le < re) {
        if (right == re) {
          storage_[pos] = val;
          return;
        }
        downMark(pos);
        Trait::copyUnmarkValue(storage_[pos]);
        size_t middle = leftMiddle(le, re);
        if (right >= middle) {
          storage_[leftChild(pos)] = val;
          le = middle + 1;
          pos = rightChild(pos);
        } else {
          re = middle;
          pos = leftChild(pos);
        }
        if (right < le) {
          return;
        }
      }
      assert(le == re);
      if (le == re) {
        storage_[pos] = val;
      }
    };
    leftPartialUpdate(leftChild(pos), left_edge, middle);
    rightPartialUpdate(rightChild(pos), middle + 1, right_edge);
  }
};
}  // namespace ipq
