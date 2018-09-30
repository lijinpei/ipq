#pragma once

#include "config.hpp"

#include <cstdint>

namespace ipq {

extern const char *province_names[];
extern const char **country_names[];

class Location {
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

public:
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

  const char *getProcince() { return province_names[getProvinceCode()]; }

  const char *getContryName() {
    return country_names[getProvinceCode()][getCountryCode()];
  }
};

} // namespace ipq
