#pragma once

#include <cstdint>
#include "config.hpp"

namespace ipq {
template <typename>
struct SegmentTreeTrait;
extern const char* province_names[];
extern const char** country_names[];

class Location {
  static constexpr uint64_t UnMarkLoc = uint64_t(-1);
  static constexpr uint64_t NonExistLoc = uint64_t(-2);
  uint64_t loc;
  static constexpr unsigned int province_length = 32;
  static constexpr unsigned int country_length = 32;
  static constexpr unsigned int province_shift = country_length;
  static constexpr unsigned int country_shift = 0;
  static constexpr uint64_t province_mask =
      ((uint64_t(1) << province_length) - 1) << province_shift;
  static constexpr uint64_t shifted_province_mask =
      ((uint64_t(1) << country_length) - 1);
  static constexpr uint64_t country_mask = (uint64_t(1) << country_length) - 1;
  static constexpr uint64_t shifted_country_mask = country_mask;

  friend struct SegmentTreeTrait<Location>;

 public:
  Location(uint64_t loc) : loc(loc) {
  }
  Location(uint64_t prov, uint64_t city) {
    loc = (prov & shifted_province_mask) << province_shift |(city & country_mask);
  }
  uint64_t getLoc() { return loc; }
  void setLoc(uint64_t nloc) { loc = nloc; }

  uint64_t getProvinceCode() {
    return (loc >> province_shift) & shifted_province_mask;
  }

  void setProvinceCode(uint64_t prov) {
    IPQ_ASSERT(!(prov ^ shifted_province_mask));
    loc = (prov << province_shift) | (loc & country_mask);
  }

  uint64_t getCountryCode() { return loc & country_mask; }

  void setCountryCOde(uint64_t cou) {
    IPQ_ASSERT(!(cou & country_mask));
    loc = (loc & province_mask) | cou;
  }

  const char* getProcince() { return province_names[getProvinceCode()]; }

  const char* getContryName() {
    return country_names[getProvinceCode()][getCountryCode()];
  }

  bool operator==(Location& rhs) const { return loc == rhs.loc; }
};

template <>
struct SegmentTreeTrait<Location> {
  using ValueTy = Location;
  static void initialNonExist(Location* storage) {
    storage->loc = Location::NonExistLoc;
  }
  static bool isUnmarkValue(const Location& val) {
    return val.loc == Location::UnMarkLoc;
  }
  static bool isNonExistValue(const Location& val) {
    return val.loc == Location::NonExistLoc;
  }
  static void copyUnmarkValue(Location& val) { val.loc = Location::UnMarkLoc; }
  static void copyNonExistValue(Location& val) {
    val.loc = Location::NonExistLoc;
  }
  static Location getNonExitValue() { return Location(Location::NonExistLoc); }
  static Location getUnmarkValue() { return Location(Location::UnMarkLoc); }
};

}  // namespace ipq
