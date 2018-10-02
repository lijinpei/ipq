#include <cstdint>
#include "segment_tree.hpp"

namespace ipq {
template <>
struct SegmentTreeTrait<uint32_t> {
  using ValueTy = uint32_t;
  constexpr static uint32_t NonExistValue = uint32_t(-1);
  constexpr static uint32_t UnmarkValue = uint32_t(-2);
  static void initialNonExist(uint32_t* storage) {
      *storage = NonExistValue;
  }
  static bool isUnmarkValue(const uint32_t& val) {
    return val == UnmarkValue;
  }
  static bool isNonExistValue(const uint32_t& val) {
    return val == NonExistValue;
  }
  static void copyUnmarkValue(uint32_t& val) { val = UnmarkValue; }
  static void copyNonExistValue(uint32_t& val) { val = NonExistValue; }
  static uint32_t getNonExitValue() { return NonExistValue; }
  static uint32_t getUnmarkValue() { return UnmarkValue; }
};
template <>
struct SegmentTreeTrait<uint16_t> {
  using ValueTy = uint16_t;
  constexpr static uint16_t NonExistValue = uint16_t(-1);
  constexpr static uint16_t UnmarkValue = uint16_t(-2);
  static void initialNonExist(uint16_t* storage) {
      *storage = NonExistValue;
  }
  static bool isUnmarkValue(const uint16_t& val) {
    return val == UnmarkValue;
  }
  static bool isNonExistValue(const uint16_t& val) {
    return val == NonExistValue;
  }
  static void copyUnmarkValue(uint16_t& val) { val = UnmarkValue; }
  static void copyNonExistValue(uint16_t& val) { val = NonExistValue; }
  static uint16_t getNonExitValue() { return NonExistValue; }
  static uint16_t getUnmarkValue() { return UnmarkValue; }
};
}  // namespace ipq
