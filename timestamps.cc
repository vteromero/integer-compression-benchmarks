// Copyright (c) 2019 Vicente Romero Calero. All rights reserved.
// Licensed under the MIT License.
// See LICENSE file in the project root for full license information.

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include "common.h"

#include "benchmark/include/benchmark/benchmark.h"
#include "SIMDCompressionAndIntersection/include/binarypacking.h"
#include "SIMDCompressionAndIntersection/include/fastpfor.h"
#include "SIMDCompressionAndIntersection/include/variablebyte.h"
#include "SIMDCompressionAndIntersection/include/varintgb.h"
#include "../VTEnc/vtenc.h"

class TimestampsDataSet : public benchmark::Fixture {
private:
  void loadTimestamps(std::string fileName) {
    std::ifstream inFile(fileName);
    uint32_t t;

    if (inFile.fail()) {
      throw std::logic_error("Failed to open '" + fileName + "'");
    }

    while (inFile >> t) {
      timestamps.push_back(t);
    }
  }

public:
  static std::vector<uint32_t> timestamps;

  void SetUp(const ::benchmark::State& state) {
    if (timestamps.empty()) {
      loadTimestamps(GlobalState::dataDirectory + std::string("/ts.txt"));
    }
  }

  void TearDown(const ::benchmark::State& state) {}
};

std::vector<uint32_t> TimestampsDataSet::timestamps = std::vector<uint32_t>();

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

BENCHMARK_F(TimestampsDataSet, Copy)(benchmark::State& state) {
  CompressionStats stats(state);
  std::vector<uint32_t> copyTo(timestamps.size());

  for (auto _ : state)
    std::copy(timestamps.begin(), timestamps.end(), copyTo.begin());

  stats.SetInputLengthInBytes(timestamps.size() * sizeof(uint32_t));
  stats.SetEncodedLengthInBytes(timestamps.size() * sizeof(uint32_t));
  stats.SetFinalStats();
}

BENCHMARK_F(TimestampsDataSet, VTEncEncode)(benchmark::State& state) {
  CompressionStats stats(state);
  std::vector<uint8_t> encoded(vtenc_max_encoded_size32(timestamps.size()));
  vtenc *handler = vtenc_create();

  for (auto _ : state)
    vtenc_encode32(handler, timestamps.data(), timestamps.size(), encoded.data(), encoded.size());

  stats.SetInputLengthInBytes(timestamps.size() * sizeof(uint32_t));
  stats.SetEncodedLengthInBytes(vtenc_encoded_size(handler));
  stats.SetFinalStats();

  vtenc_destroy(handler);
}

BENCHMARK_F(TimestampsDataSet, VTEncDecode)(benchmark::State& state) {
  CompressionStats stats(state);
  std::vector<uint8_t> encoded(vtenc_max_encoded_size32(timestamps.size()));
  vtenc *handler = vtenc_create();
  
  vtenc_encode32(handler, timestamps.data(), timestamps.size(), encoded.data(), encoded.size());
  size_t encodedLength = vtenc_encoded_size(handler);

  std::vector<uint32_t> decoded(timestamps.size());

  for (auto _ : state)
    vtenc_decode32(handler, encoded.data(), encodedLength, decoded.data(), decoded.size());

  if (timestamps != decoded) {
    throw std::logic_error("equality check failed");
  }

  stats.SetInputLengthInBytes(timestamps.size() * sizeof(uint32_t));
  stats.SetEncodedLengthInBytes(encodedLength);
  stats.SetFinalStats();

  vtenc_destroy(handler);
}

BENCHMARK_F(TimestampsDataSet, DeltaVariableByteEncode)(benchmark::State& state) {
  SIMDCompressionLib::VByte<true> codec;
  SIMDCompressionUtil comp(codec, 1, timestamps);
  benchmarkEncode(comp, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaVariableByteDecode)(benchmark::State& state) {
  SIMDCompressionLib::VByte<true> codec;
  SIMDCompressionUtil comp(codec, 1, timestamps);
  benchmarkDecode(comp, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaVarIntGBEncode)(benchmark::State& state) {
  SIMDCompressionLib::VarIntGB<true> codec;
  SIMDCompressionUtil comp(codec, 1, timestamps);
  benchmarkEncode(comp, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaVarIntGBDecode)(benchmark::State& state) {
  SIMDCompressionLib::VarIntGB<true> codec;
  SIMDCompressionUtil comp(codec, 1, timestamps);
  benchmarkDecode(comp, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaBinaryPackingEncode)(benchmark::State& state) {
  SIMDCompressionLib::BinaryPacking<SIMDCompressionLib::BasicBlockPacker> codec;
  SIMDCompressionUtil comp(codec, codec.BlockSize, timestamps);
  benchmarkEncode(comp, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaBinaryPackingDecode)(benchmark::State& state) {
  SIMDCompressionLib::BinaryPacking<SIMDCompressionLib::BasicBlockPacker> codec;
  SIMDCompressionUtil comp(codec, codec.BlockSize, timestamps);
  benchmarkDecode(comp, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaFastPFor128Encode)(benchmark::State& state) {
  SIMDCompressionLib::FastPFor<4, true> codec;
  SIMDCompressionUtil comp(codec, codec.BlockSize, timestamps);
  benchmarkEncode(comp, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaFastPFor128Decode)(benchmark::State& state) {
  SIMDCompressionLib::FastPFor<4, true> codec;
  SIMDCompressionUtil comp(codec, codec.BlockSize, timestamps);
  benchmarkDecode(comp, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaFastPFor256Encode)(benchmark::State& state) {
  SIMDCompressionLib::FastPFor<8, true> codec;
  SIMDCompressionUtil comp(codec, codec.BlockSize, timestamps);
  benchmarkEncode(comp, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaFastPFor256Decode)(benchmark::State& state) {
  SIMDCompressionLib::FastPFor<8, true> codec;
  SIMDCompressionUtil comp(codec, codec.BlockSize, timestamps);
  benchmarkDecode(comp, state);
}
