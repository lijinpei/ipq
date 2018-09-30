#pragma once

// wrapper class for pthread

namespace ipq {
class PthreadSpinLock {
public:
  void lock() {}
  void unlock() {}
};
} // namespace ipq
