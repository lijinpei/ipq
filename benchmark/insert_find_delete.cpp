#include <limits>
#include <random>
#include <utility>
#include <vector>
#include "benchmark/benchmark.h"
#include "btree_set.hpp"

namespace {

std::random_device rd;

template <typename T>
T to_value(int v) {
  return v;
}

template <>
std::string to_value<std::string>(int v) {
  return std::to_string(v);
}

template <typename T>
class SetRandomOpFixture : public ::benchmark::Fixture {
 public:
  void SetUp(const ::benchmark::State& st) {
    int nmax = st.range(0);
    std::uniform_int_distribution<int> value_dist(
        std::numeric_limits<int>::min());
    std::uniform_int_distribution<int> op_dist(0, 10);
    operations_.reserve(nmax);
    for (int i = 0; i < nmax; ++i) {
      operations_.emplace_back(op_dist(rd), to_value<T>(value_dist(rd)));
    }
  }
  std::vector<std::pair<int, T>> operations_;
};

BENCHMARK_TEMPLATE_DEFINE_F(SetRandomOpFixture, int_btree_set, int)
(benchmark::State& st) {
  ipq::BTreeSet<int> set;
  for (auto _ : st) {
    for (auto& v : operations_) {
    switch (v.first) {
      case 1: {
        set.erase(v.second);
      } break;
      case 2: {
        set.find(v.second);
      } break;
      case 3: {
        set.lower_bound(v.second);
      } break;
      case 4: {
        set.upper_bound(v.second);
      } break;
      case 5: {
        set.find(v.second);
      } break;
      default: {
        set.insert(v.second);
      }
    }
  }
}
}
BENCHMARK_REGISTER_F(SetRandomOpFixture, int_btree_set)
    ->RangeMultiplier(1 << 8)
    ->Range(1 << 8, 1 << 24);

BENCHMARK_TEMPLATE_DEFINE_F(SetRandomOpFixture, int_stl_set, int)
(benchmark::State& st) {
  std::set<int> set;
  for (auto _ : st) {
    for (auto& v : operations_) {
    switch (v.first) {
      case 1: {
        set.erase(v.second);
      } break;
      case 2: {
        set.find(v.second);
      } break;
      case 3: {
        set.lower_bound(v.second);
      } break;
      case 4: {
        set.upper_bound(v.second);
      } break;
      case 5: {
        set.find(v.second);
      } break;
      default: {
        set.insert(v.second);
      }
    }
  }
}
}
BENCHMARK_REGISTER_F(SetRandomOpFixture, int_stl_set)
    ->RangeMultiplier(1 << 8)
    ->Range(1 << 8, 1 << 24);

BENCHMARK_TEMPLATE_DEFINE_F(SetRandomOpFixture, string_btree_set, std::string)
(benchmark::State& st) {
  ipq::BTreeSet<std::string> set;
  for (auto _ : st) {
    for (auto& v : operations_) {
    switch (v.first) {
      case 1: {
        set.erase(v.second);
      } break;
      case 2: {
        set.find(v.second);
      } break;
      case 3: {
        set.lower_bound(v.second);
      } break;
      case 4: {
        set.upper_bound(v.second);
      } break;
      case 5: {
        set.find(v.second);
      } break;
      default: {
        set.insert(v.second);
      }
    }
  }
}
}
BENCHMARK_REGISTER_F(SetRandomOpFixture, string_btree_set)
    ->RangeMultiplier(1 << 8)
    ->Range(1 << 8, 1 << 24);

BENCHMARK_TEMPLATE_DEFINE_F(SetRandomOpFixture, string_stl_set, std::string)
(benchmark::State& st) {
  std::set<std::string> set;
  for (auto _ : st) {
    for (auto& v : operations_) {
    switch (v.first) {
      case 1: {
        set.erase(v.second);
      } break;
      case 2: {
        set.find(v.second);
      } break;
      case 3: {
        set.lower_bound(v.second);
      } break;
      case 4: {
        set.upper_bound(v.second);
      } break;
      case 5: {
        set.find(v.second);
      } break;
      default: {
        set.insert(v.second);
      }
    }
  }
}
}
BENCHMARK_REGISTER_F(SetRandomOpFixture, string_stl_set)
    ->RangeMultiplier(1 << 8)
    ->Range(1 << 8, 1 << 24);
}  // namespace

BENCHMARK_MAIN();

