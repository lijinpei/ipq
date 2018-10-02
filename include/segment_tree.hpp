#pragma once

namespace ipq {

template <typename ValueTy>
struct SegmentTreeTrait {
  /*
  stiatc void initialNonExist(ValueTy* storage, size_t size);
  static bool isUnmarkValue(ValueTy* storage);
  static bool isNonExistValue(ValueTy* storage);
  static bool copyUnmarkValue(ValueTy* val);
  static bool copyNonExistValue(ValueTy* val);
  static ValueTy getNonExitValue();
   */
};

template <typename ValueTy, typename AllocTy = std::allocator<ValueTy>>
class SegmentTree : AllocTy::rebind(ValueTy)::type, SegmentTreeTrait<ValueTy>{
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

  static void populateMark(ValueTy& parent_mark, ValueTy& child_mark) {
    if (isUnmarkValue(parent) && !isUnmarkValue(child_mark)) {
      parent_mark = child_mark;
    } else if (!isUnmarkValue(parent)) {
      child_mark = parent_mark;
    }
  }

  bool populateMarkToChildren(ValueTy & parent_mark, size_t pos) {
    if (isUnmarkValue(parent_mark)) {
      return false;
    }
    storage_[leftChild(pos)] = parent_mark;
    storage_[leftChild(pos)] = parent_mark;
    return true;
  }

 public:
  SegmentTree(size_t left, size_t right, AllocTy& alloc)
      : AllocTy(alloc), left_edge(left), right_edge(right) {
    size_t s = 2 * size();
    storage_ = this->AllocTy::allocate(s);
    Trait::initialNonExist(storage_, s);
  }

  size_t size() { return right_edge_ - left_edge_ + 1; }

  /* use const ValueTy* as a work-around for std::optional
   */
  const ValueTy* find(size_t point) {
    size_t pos = 0, left = left_edge_, right = right_edge_;
    ValueTy mark = getUnmarkValue();
    while (left < right) {
      populateMark(mark, storage_[pos]);
      if (!isUnmarkValue(mark)) {
        break;
      } else {
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
    if (isNonExistValue(mark)) {
      return {};
    } else {
      return mark;
    }
  }

  const ValueTy* rangeFind(size_t left, size_t right) {
    size_t pos = 0, left = left_edge_, right = right_edge_;
    ValueTy mark = getUnmarkValue();
    while (left < right) {
      populateMark(mark, storage_[pos]);
      if (!isUnmarkValue(mark)) {
        if (isNonExistValue(mark)) {
          return nullptr;
        } else {
          return &storage_[pos];
        }
      } else {
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
  }

  void remove(size_t left, size_t right) {
    update(left, right, getNonExitValue());
  }

  void update(size_t left, size_t right, const ValueTy& val) {
    size_t left_edge = left_edge, right_edge = right_edge_, pos = 0;
    if (left == left_edge && right == right_edge) {
      storage_[0] = val;
      return;
    }
    ValueTy mark = storage_[0];
    while (left_edge < right_edge) {
      size_t middle = leftMiddle(left_edge, right_edge);
      if (left > middle) {
        size_t rc = rightChild(pos);
        if (populateMarkToChildren(mark, pos)) {
          mark = storage_[rc];
        }
        pos = rc;
        left_edge = middle + 1;
      } else if (right <= middle) {
        size_t lc = leftChild(pos);
        if (populateMarkToChildren(mark, pos)) {
          mark = storage_[lc];
        }
        pos = lc;
        right_edge = middle;
      } else {
        break;
      }
    }
    if (left_edge == right_edge) {
      storage_[pos] = val;
      return;
    }
    populateMarkToChildren(mark, pos);
    {
      size_t middle = leftMiddle(left_edge, right_edge), lpos = leftChild(pos);
      size_t le = left_edge, re = middle;
      ValueTy mark = storage_[lpos];
      while (le < re) {
        middle = leftMiddle(le, re);
        if (left == middle + 1) {
          storage_[rightChild(lpos] = val;
          break;
        } else if (left <= middle) {
          size_t lc = leftChild(lpos);
          if (populateMarkToChildren(mark, lpos)) {
            mark = storage_[lc];
          }
          storage_[rightChild(lpos)] = val;
          re = middle;
          lpos = lc;
        } else {
          size_t rc = rightChild(lpos);
          if (populateMarkToChildren(mark, lpos)) {
            mark = storage_[rc];
          }
          le = middle + 1;
          lpos = rc;
        }
      }
      if (le == re) {
        storage_[lpos] = val;
      }
    }
    {
      size_t middle = rightMiddle(left_edge, right_edge), rpos = rightChild(pos);
      size_t le = middle, re = right;
      ValueTy mark = storage_[rpos];
      while (le < re) {
        middle = leftMiddle(le, re);
        if (right == middle) {
          storage_[rightChild(rpos)] = val;
          break;
        } else if (right < middle) {
          size_t lc = leftChild(rpos);
          if (populateMarkToChildren(mark, rpos)) {
            mark = storage_[lc];
          }
          re = middle;
          rpos = lc;
        } else {
          size_t rc = rightChild(rpos);
          if (populateMarkToChildren(mark, rpos)) {
            mark = storage_[rc];
          }
          storage_[leftChild(rpos)] = val;
          le = middle + 1;
          rpos = rc;
        }
      }
      if (le == re) {
        storage_[rpos] = val;
      }
    }
  }
};
}  // namespace ipq
