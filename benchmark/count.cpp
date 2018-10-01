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
    values_to_insert_.reserve(nmax);
    values_to_find_.reserve(nmax);
    for (int i = 0; i < nmax; ++i) {
      values_to_insert_.push_back(to_value<T>(value_dist(rd)));
    }
    for (int i = 0; i < nmax; ++i) {
      values_to_find_.push_back(to_value<T>(value_dist(rd)));
    }
  }
  std::vector<T> values_to_insert_, values_to_find_;
};

BENCHMARK_TEMPLATE_DEFINE_F(SetRandomOpFixture, int_btree_set, int)
(benchmark::State& st) {
  ipq::BTreeSet<int> btree_set;
  for (auto v : values_to_insert_) {
    btree_set.insert(v);
  }
  for (auto _ : st) {
    for (auto v : values_to_find_) {
      btree_set.count(v);
    }
  }
}
BENCHMARK_REGISTER_F(SetRandomOpFixture, int_btree_set)
    ->RangeMultiplier(1 << 8)
    ->Range(1 << 8, 1 << 24);

BENCHMARK_TEMPLATE_DEFINE_F(SetRandomOpFixture, int_stl_set, int)
(benchmark::State& st) {
  std::set<int> set;
  for (auto v : values_to_insert_) {
    set.insert(v);
  }
  for (auto _ : st) {
    for (auto v : values_to_find_) {
      set.count(v);
    }
  }
}
BENCHMARK_REGISTER_F(SetRandomOpFixture, int_stl_set)
    ->RangeMultiplier(1 << 8)
    ->Range(1 << 8, 1 << 24);

BENCHMARK_TEMPLATE_DEFINE_F(SetRandomOpFixture, string_btree_set, std::string)
(benchmark::State& st) {
  ipq::BTreeSet<std::string> btree_set;
  for (auto v : values_to_insert_) {
    btree_set.insert(v);
  }
  for (auto _ : st) {
    for (auto v : values_to_find_) {
      btree_set.count(v);
    }
  }
}
BENCHMARK_REGISTER_F(SetRandomOpFixture, string_btree_set)
    ->RangeMultiplier(1 << 8)
    ->Range(1 << 8, 1 << 24);

BENCHMARK_TEMPLATE_DEFINE_F(SetRandomOpFixture, string_stl_set, std::string)
(benchmark::State& st) {
  std::set<std::string> set;
  for (auto v : values_to_insert_) {
    set.insert(v);
  }
  for (auto _ : st) {
    for (auto v : values_to_find_) {
      set.count(v);
    }
  }
}
BENCHMARK_REGISTER_F(SetRandomOpFixture, string_stl_set)
    ->RangeMultiplier(1 << 8)
    ->Range(1 << 8, 1 << 24);
}  // namespace

BENCHMARK_MAIN();

