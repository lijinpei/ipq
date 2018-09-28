#pragma once

#include "config.hpp"
#include "ip.hpp"
#include "location.hpp"
#include "member_detecter.hpp"

#include <utility>

#include <vector>

namespace ipq {
class DataStorateConcept {
public:
  using AsyncTokenTy = uint64_t;
  virtual Location query(Ip ip) const = 0;
  virtual void updateBlocking(Ip, Location) = 0;
  virtual void batchUpdateBlocking(ArrayRef<std::pair<Ip, Location>>) = 0;
  virtual AsyncTokenTy updateNonblockingRequire(Ip, Location) = 0;
  virtual updateNonBlockingFinish(AsyncTokenTy) = 0;
  virtual AsyncTokenTy
      batchUpdateNonBlockingRequire(ArrayRef<std::pair<Ip, Location>>) = 0;
  virtual batchUpdateNonBlockingFinish(AsyncTokenTy) = 0;
};

template <class ImplT>
class DataStorageModel : public DataStorateConcept, private ImplT {
public:
  Location query(Ip ip) const override;
  void updateBlocking(Ip ip, Location loc) override {
    if constexpr (IPQ_HAS_MEMBER(ImplT, updateBlocking)) {
      this->ImplT::updateBlocking(loc, ip);
    } else if
      constepxr(IPQ_HAS_MEMBER(ImplT, batchUpdateBlocking)) {
        this->ImplT::batchUpdateBlocking({std::make_pair(ip, loc)});
      }
    else {
      IPQ_ASSERT(false && "method not implemented");
    }
  }
  void batchUpdateBlocking(ArrayRef<std::pair<Ip, Location>> updates) override {
    if constexpr (IPQ_HAS_MEMBER(ImplT, batchUpdateBlocking)) {
      this->ImplT::batchUpdateBlocking(updates);
    } else if
      constepxr(IPQ_HAS_MEMBER(ImplT, updateBlocking)) {
        for (auto u : updates) {
          this->ImplT::updateBlocking(u.first, u.second);
        }
      }
    else {
      IPQ_ASSERT(false && "method not implemented");
    }
  }
  AsyncTokenTy updateNonblockingRequire(Location loc, Ip ip) override {
    if constexpr (IPQ_HAS_MEMBER(ImplT, updateNonblockingRequire)) {
      return this->ImplT::updateNonblockingRequire(loc, ip);
    } else if
      constepxr(IPQ_HAS_MEMBER(ImplT, batchUpdateNonBlockingRequire)) {
        return this->ImplT::batchUpdateNonBlockingRequire(
            {std::make_pair(ip, loc)});
      }
    else {
      IPQ_ASSERT(false && "method not implemented");
    }
  }
  updateNonBlockingFinish(AsyncTokenTy tok) override {
    if constexpr (IPQ_HAS_MEMBER(ImplT, updateNonblockingRequire)) {
      return this->ImplT::updateNonblockingFinish(tok);
    } else if
      constepxr(IPQ_HAS_MEMBER(ImplT, batchUpdateNonBlockingRequire)) {
        return this->ImplT::batchUpdateNonBlockingFinish(tok);
      }
    else {
      IPQ_ASSERT(false && "method not implemented");
    }
  }
  AsyncTokenTy batchUpdateNonBlockingRequire(
      ArrayRef<std::pair<Ip, Location>> updates) override {
    if constexpr (IPQ_HAS_MEMBER(ImplT, batchUpdateNonblockingRequire)) {
      return this->ImplT::batchUpdateNonblockingRequire(updates);
    } else {
      IPQ_ASSERT(false && "method not implemented");
    }
  }
  void batchUpdateNonBlockingFinish(AsyncTokenTy tok) override {
    if constexpr (IPQ_HAS_MEMBER(ImplT, batchUpdateNonblockingRequire)) {
      return this->ImplT::batchUpdateNonblockingFinish(tok);
    } else {
      IPQ_ASSERT(false && "method not implemented");
    }
  }
};
}; // namespace ipq

