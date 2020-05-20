// Copyright (c) 2020 Vicente Romero Calero. All rights reserved.
// Licensed under the MIT License.
// See LICENSE file in the project root for full license information.

#include <algorithm>
#include <iostream>
#include <limits>
#include <random>
#include <unordered_map>
#include <vector>

#include "common.h"

#include "benchmark/include/benchmark/benchmark.h"
#include "SIMDCompressionAndIntersection/include/binarypacking.h"
#include "SIMDCompressionAndIntersection/include/fastpfor.h"
#include "SIMDCompressionAndIntersection/include/variablebyte.h"
#include "SIMDCompressionAndIntersection/include/varintgb.h"
#include "VTEnc/vtenc.h"

class RandomUniform32 : public benchmark::Fixture {
private:
  void makeSortedSet(std::vector<uint32_t>& v) {
    std::sort(v.begin(), v.end());

    auto last = std::unique(v.begin(), v.end());
    v.erase(last, v.end());
  }

  void generateRandomDistribution(size_t len) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<uint32_t> dist(0, std::numeric_limits<uint32_t>::max());
    std::vector<uint32_t> v = std::vector<uint32_t>(len);

    for (size_t i = 0; i < len; ++i) {
      v[i] = dist(mt);
    }

    makeSortedSet(v);

    dist_map[len] = v;
  }

public:
  static std::unordered_map<size_t, std::vector<uint32_t> > dist_map;

  void SetUp(const ::benchmark::State& state) {
    size_t len = state.range(0);

    if (dist_map.find(len) == dist_map.end()) {
      generateRandomDistribution(len);
    }
  }

  void TearDown(const ::benchmark::State& state) {}
};

std::unordered_map<size_t, std::vector<uint32_t> > RandomUniform32::dist_map = std::unordered_map<size_t, std::vector<uint32_t> >();

static void benchmarkEncode(SIMDCompressionUtil& comp, benchmark::State& state) {
  CompressionStats stats(state);

  for (auto _ : state) {
    state.PauseTiming();
    comp.Reset();
    state.ResumeTiming();

    comp.Encode();
  }

  stats.SetInputLengthInBytes(comp.InputLength() * sizeof(uint32_t));
  stats.SetEncodedLengthInBytes(comp.EncodedLength() * sizeof(uint32_t));
  stats.SetFinalStats();
}

static void benchmarkDecode(SIMDCompressionUtil& comp, benchmark::State& state) {
  CompressionStats stats(state);

  comp.Encode();

  for (auto _ : state) {
    comp.Decode();
  }

  comp.EqualityCheck();

  stats.SetInputLengthInBytes(comp.InputLength() * sizeof(uint32_t));
  stats.SetEncodedLengthInBytes(comp.EncodedLength() * sizeof(uint32_t));
  stats.SetFinalStats();
}

BENCHMARK_DEFINE_F(RandomUniform32, Copy)(benchmark::State& state) {
  CompressionStats stats(state);
  size_t len = state.range(0);
  std::vector<uint32_t> &data = dist_map[len];
  std::vector<uint32_t> copyTo(data.size());

  for (auto _ : state)
    std::copy(data.begin(), data.end(), copyTo.begin());

  stats.SetInputLengthInBytes(data.size() * sizeof(uint32_t));
  stats.SetEncodedLengthInBytes(data.size() * sizeof(uint32_t));
  stats.SetFinalStats();
}

BENCHMARK_DEFINE_F(RandomUniform32, VTEncEncode)(benchmark::State& state) {
  CompressionStats stats(state);
  size_t len = state.range(0);
  std::vector<uint32_t> &data = dist_map[len];
  size_t encodedLength = 0;
  std::vector<uint8_t> encoded(vtenc_max_encoded_size32(data.size()));
  VtencEncoder encoder = { .allow_repeated_values = 0, .skip_full_subtrees = 1, .min_cluster_length = 1 };

  for (auto _ : state)
    encodedLength = vtenc_encode32(&encoder, data.data(), data.size(), encoded.data(), encoded.size());

  stats.SetInputLengthInBytes(data.size() * sizeof(uint32_t));
  stats.SetEncodedLengthInBytes(encodedLength);
  stats.SetFinalStats();
}

BENCHMARK_DEFINE_F(RandomUniform32, VTEncDecode)(benchmark::State& state) {
  CompressionStats stats(state);
  size_t len = state.range(0);
  std::vector<uint32_t> &data = dist_map[len];

  std::vector<uint8_t> encoded(vtenc_max_encoded_size32(data.size()));
  VtencEncoder encoder = { .allow_repeated_values = 0, .skip_full_subtrees = 1, .min_cluster_length = 1 };
  size_t encodedLength = vtenc_encode32(&encoder, data.data(), data.size(), encoded.data(), encoded.size());

  std::vector<uint32_t> decoded(data.size());
  VtencDecoder decoder = { .allow_repeated_values = 0, .skip_full_subtrees = 1, .min_cluster_length = 1 };

  for (auto _ : state)
    vtenc_decode32(&decoder, encoded.data(), encodedLength, decoded.data(), decoded.size());

  if (data != decoded) {
    throw std::logic_error("equality check failed");
  }

  stats.SetInputLengthInBytes(data.size() * sizeof(uint32_t));
  stats.SetEncodedLengthInBytes(encodedLength);
  stats.SetFinalStats();
}

BENCHMARK_DEFINE_F(RandomUniform32, DeltaVariableByteEncode)(benchmark::State& state) {
  size_t len = state.range(0);
  std::vector<uint32_t> &data = dist_map[len];
  SIMDCompressionLib::VByte<true> codec;
  SIMDCompressionUtil comp(codec, 1, data);
  benchmarkEncode(comp, state);
}

BENCHMARK_DEFINE_F(RandomUniform32, DeltaVariableByteDecode)(benchmark::State& state) {
  size_t len = state.range(0);
  std::vector<uint32_t> &data = dist_map[len];
  SIMDCompressionLib::VByte<true> codec;
  SIMDCompressionUtil comp(codec, 1, data);
  benchmarkDecode(comp, state);
}

BENCHMARK_DEFINE_F(RandomUniform32, DeltaVarIntGBEncode)(benchmark::State& state) {
  size_t len = state.range(0);
  std::vector<uint32_t> &data = dist_map[len];
  SIMDCompressionLib::VarIntGB<true> codec;
  SIMDCompressionUtil comp(codec, 1, data);
  benchmarkEncode(comp, state);
}

BENCHMARK_DEFINE_F(RandomUniform32, DeltaVarIntGBDecode)(benchmark::State& state) {
  size_t len = state.range(0);
  std::vector<uint32_t> &data = dist_map[len];
  SIMDCompressionLib::VarIntGB<true> codec;
  SIMDCompressionUtil comp(codec, 1, data);
  benchmarkDecode(comp, state);
}

BENCHMARK_DEFINE_F(RandomUniform32, DeltaBinaryPackingEncode)(benchmark::State& state) {
  size_t len = state.range(0);
  std::vector<uint32_t> &data = dist_map[len];
  SIMDCompressionLib::BinaryPacking<SIMDCompressionLib::BasicBlockPacker> codec;
  SIMDCompressionUtil comp(codec, codec.BlockSize, data);
  benchmarkEncode(comp, state);
}

BENCHMARK_DEFINE_F(RandomUniform32, DeltaBinaryPackingDecode)(benchmark::State& state) {
  size_t len = state.range(0);
  std::vector<uint32_t> &data = dist_map[len];
  SIMDCompressionLib::BinaryPacking<SIMDCompressionLib::BasicBlockPacker> codec;
  SIMDCompressionUtil comp(codec, codec.BlockSize, data);
  benchmarkDecode(comp, state);
}

BENCHMARK_DEFINE_F(RandomUniform32, DeltaFastPFor128Encode)(benchmark::State& state) {
  size_t len = state.range(0);
  std::vector<uint32_t> &data = dist_map[len];
  SIMDCompressionLib::FastPFor<4, true> codec;
  SIMDCompressionUtil comp(codec, codec.BlockSize, data);
  benchmarkEncode(comp, state);
}

BENCHMARK_DEFINE_F(RandomUniform32, DeltaFastPFor128Decode)(benchmark::State& state) {
  size_t len = state.range(0);
  std::vector<uint32_t> &data = dist_map[len];
  SIMDCompressionLib::FastPFor<4, true> codec;
  SIMDCompressionUtil comp(codec, codec.BlockSize, data);
  benchmarkDecode(comp, state);
}

BENCHMARK_DEFINE_F(RandomUniform32, DeltaFastPFor256Encode)(benchmark::State& state) {
  size_t len = state.range(0);
  std::vector<uint32_t> &data = dist_map[len];
  SIMDCompressionLib::FastPFor<8, true> codec;
  SIMDCompressionUtil comp(codec, codec.BlockSize, data);
  benchmarkEncode(comp, state);
}

BENCHMARK_DEFINE_F(RandomUniform32, DeltaFastPFor256Decode)(benchmark::State& state) {
  size_t len = state.range(0);
  std::vector<uint32_t> &data = dist_map[len];
  SIMDCompressionLib::FastPFor<8, true> codec;
  SIMDCompressionUtil comp(codec, codec.BlockSize, data);
  benchmarkDecode(comp, state);
}

BENCHMARK_REGISTER_F(RandomUniform32, Copy)
  ->RangeMultiplier(10)->Range(100, 10000000);

BENCHMARK_REGISTER_F(RandomUniform32, VTEncEncode)
  ->RangeMultiplier(10)->Range(100, 10000000);

BENCHMARK_REGISTER_F(RandomUniform32, VTEncDecode)
  ->RangeMultiplier(10)->Range(100, 10000000);

BENCHMARK_REGISTER_F(RandomUniform32, DeltaVariableByteEncode)
  ->RangeMultiplier(10)->Range(100, 10000000);

BENCHMARK_REGISTER_F(RandomUniform32, DeltaVariableByteDecode)
  ->RangeMultiplier(10)->Range(100, 10000000);

BENCHMARK_REGISTER_F(RandomUniform32, DeltaVarIntGBEncode)
  ->RangeMultiplier(10)->Range(100, 10000000);

BENCHMARK_REGISTER_F(RandomUniform32, DeltaVarIntGBDecode)
  ->RangeMultiplier(10)->Range(100, 10000000);

BENCHMARK_REGISTER_F(RandomUniform32, DeltaBinaryPackingEncode)
  ->RangeMultiplier(10)->Range(100, 10000000);

BENCHMARK_REGISTER_F(RandomUniform32, DeltaBinaryPackingDecode)
  ->RangeMultiplier(10)->Range(100, 10000000);

BENCHMARK_REGISTER_F(RandomUniform32, DeltaFastPFor128Encode)
  ->RangeMultiplier(10)->Range(100, 10000000);

BENCHMARK_REGISTER_F(RandomUniform32, DeltaFastPFor128Decode)
  ->RangeMultiplier(10)->Range(100, 10000000);

BENCHMARK_REGISTER_F(RandomUniform32, DeltaFastPFor256Encode)
  ->RangeMultiplier(10)->Range(100, 10000000);

BENCHMARK_REGISTER_F(RandomUniform32, DeltaFastPFor256Decode)
  ->RangeMultiplier(10)->Range(100, 10000000);
