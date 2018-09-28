#pragma once

#include "config.hpp"
#include <cstdint>

namespace ipq {
  class Ip {
    uint32_t ip_host_order;
    template <int P>
    uint32_t getByteHostOrder() {
      IPQ_STATIC_ASSERT(P < 4);
    }
    template <int P>
    void setByteHostOrder(uint8_t v) {
      IPQ_STATIC_ASSERT(P < 4);
    }
  public:
    uint32_t getHostOrder() {
    }
    void setHostOrder(uint32_t ip) {
    }
    uint32_t getNetworkOrder() {
    }
    void setNetworkOrder(uint32_t ip) {
    }
    uint32_t getLittileEndian() {
    }
    void setLittleEndian(uint32_t ip) {
    }
    uint32_t getBigEndian() {
    }
    void setBigEndian(uint32_t ip) {
    }
    uint8_t getByByte(int p) {
      IPQ_ASSERT(p < 4);
    }
    void setByByte(int p, uint8_t v) {
      IPQ_ASSERT(p < 4);
    }
    template <int P>
    uint8_t geByByte() {
      IPQ_STATIC_ASSERT(P < 4);
    }
    template <int P>
    void setByByte(uint8_t v) {
      IPQ_STATIC_ASSERT(P < 4);
    }
  };
}

