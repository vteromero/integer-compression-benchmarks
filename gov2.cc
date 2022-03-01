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
#include "VTEnc/vtenc.h"

class Gov2SortedDataSet : public benchmark::Fixture {
private:
  std::ifstream _inFile;
  char _buffer[4096];
  size_t _bufferCount;
  size_t _bufferOffset;

public:
  void SetUp(const ::benchmark::State& state) {}

  void TearDown(const ::benchmark::State& state) {}

  void openFile() {
    const std::string fileName = GlobalState::dataDirectory + std::string("/gov2.sorted");

    _inFile.open(fileName, std::ios::in | std::ios::binary);

    if (_inFile.fail()) {
      throw std::logic_error("Failed to open '" + fileName + "'");
    }

    _bufferCount = 0;
    _bufferOffset = 0;
  }

  void closeFile() {
    _inFile.close();
  }

  void refreshBuffer() {
    // Here, it is assumed that the file has a size multiple of 4, so there is no
    // left bytes to copy over when refreshing the buffer.
    _inFile.read(_buffer, sizeof(_buffer));
    if (_inFile.eof()) {
      throw "End of file";
    }
    _bufferCount = _inFile.gcount();
    _bufferOffset = 0;
  }

  uint32_t readNextUint32() {
    if ((_bufferCount - _bufferOffset) < 4) {
      refreshBuffer();
    }
    uint32_t value;
    std::memcpy(&value, &(_buffer[_bufferOffset]), sizeof(value));
    _bufferOffset += 4;
    return value;
  }

  bool loadNextSet(std::vector<uint32_t>& data) {
    data.clear();

    try {
      uint32_t len = readNextUint32();
      for (size_t i = 0; i < len; ++i) {
        data.push_back(readNextUint32());
      }
    } catch (...) {
      return false;
    }

    return true;
  }
};

static void benchmarkGov2SortedDataSet(
  Gov2SortedDataSet* obj,
  benchmark::State& state,
  void (*func)(
    std::vector<uint32_t>& data,
    benchmark::State& state,
    CompressionStats& stats
  ))
{
  CompressionStats stats(state);
  std::vector<uint32_t> data;

  for (auto _ : state) {
    state.PauseTiming();
    stats.Reset();
    obj->openFile();
    state.ResumeTiming();

    while (1) {
      state.PauseTiming();
      if (!obj->loadNextSet(data)) break;
      state.ResumeTiming();

      func(data, state, stats);
    }

    state.PauseTiming();
    obj->closeFile();
    state.ResumeTiming();
  }

  stats.SetFinalStats();
}

static void encodeWithCopy(
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  state.PauseTiming();
  std::vector<uint32_t> copyTo(data.size());
  state.ResumeTiming();

  std::copy(data.begin(), data.end(), copyTo.begin());

  state.PauseTiming();
  stats.UpdateInputLengthInBytes(data.size() * sizeof(uint32_t));
  stats.UpdateEncodedLengthInBytes(data.size() * sizeof(uint32_t));
  state.ResumeTiming();
}

static void encodeWithVTEnc(
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  state.PauseTiming();
  std::vector<uint8_t> encoded(vtenc_max_encoded_size32(data.size()));
  vtenc *handler = vtenc_create();
  assert(handler != NULL);
  vtenc_config(handler, VTENC_CONFIG_ALLOW_REPEATED_VALUES, 0);
  vtenc_config(handler, VTENC_CONFIG_SKIP_FULL_SUBTREES, 1);
  vtenc_config(handler, VTENC_CONFIG_MIN_CLUSTER_LENGTH, static_cast<size_t>(state.range(0)));
  state.ResumeTiming();

  vtenc_encode32(handler, data.data(), data.size(), encoded.data(), encoded.size());

  state.PauseTiming();
  stats.UpdateInputLengthInBytes(data.size() * sizeof(uint32_t));
  stats.UpdateEncodedLengthInBytes(vtenc_encoded_size(handler));
  vtenc_destroy(handler);
  state.ResumeTiming();
}

static void decodeWithVTEnc(
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  state.PauseTiming();
  std::vector<uint8_t> encoded(vtenc_max_encoded_size32(data.size()));
  vtenc *handler = vtenc_create();
  assert(handler != NULL);
  vtenc_config(handler, VTENC_CONFIG_ALLOW_REPEATED_VALUES, 0);
  vtenc_config(handler, VTENC_CONFIG_SKIP_FULL_SUBTREES, 1);
  vtenc_config(handler, VTENC_CONFIG_MIN_CLUSTER_LENGTH, static_cast<size_t>(state.range(0)));
  vtenc_encode32(handler, data.data(), data.size(), encoded.data(), encoded.size());
  size_t encodedLength = vtenc_encoded_size(handler);
  std::vector<uint32_t> decoded(data.size());
  state.ResumeTiming();

  vtenc_decode32(handler, encoded.data(), encodedLength, decoded.data(), decoded.size());

  state.PauseTiming();
  if (data != decoded) {
    throw std::logic_error("equality check failed");
  }
  stats.UpdateInputLengthInBytes(data.size() * sizeof(uint32_t));
  stats.UpdateEncodedLengthInBytes(encodedLength);
  vtenc_destroy(handler);
  state.ResumeTiming();
}

static void encodeWithSIMDCompressionCodec(
  SIMDCompressionLib::IntegerCODEC& codec,
  size_t blockSize,
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  state.PauseTiming();
  SIMDCompressionUtil comp(codec, blockSize, data);
  state.ResumeTiming();

  comp.Encode();

  state.PauseTiming();
  stats.UpdateInputLengthInBytes(comp.InputLength() * sizeof(uint32_t));
  stats.UpdateEncodedLengthInBytes(comp.EncodedLength() * sizeof(uint32_t));
  state.ResumeTiming();
}

static void decodeWithSIMDCompressionCodec(
  SIMDCompressionLib::IntegerCODEC& codec,
  size_t blockSize,
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  state.PauseTiming();
  SIMDCompressionUtil comp(codec, blockSize, data);
  comp.Encode();
  state.ResumeTiming();

  comp.Decode();

  state.PauseTiming();
  comp.EqualityCheck();
  stats.UpdateInputLengthInBytes(comp.InputLength() * sizeof(uint32_t));
  stats.UpdateEncodedLengthInBytes(comp.EncodedLength() * sizeof(uint32_t));
  state.ResumeTiming();
}

static void encodeWithDeltaVariableByte(
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  SIMDCompressionLib::VByte<true> codec;
  encodeWithSIMDCompressionCodec(codec, 1, data, state, stats);
}

static void decodeWithDeltaVariableByte(
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  SIMDCompressionLib::VByte<true> codec;
  decodeWithSIMDCompressionCodec(codec, 1, data, state, stats);
}

static void encodeWithDeltaVarIntGB(
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  SIMDCompressionLib::VarIntGB<true> codec;
  encodeWithSIMDCompressionCodec(codec, 1, data, state, stats);
}

static void decodeWithDeltaVarIntGB(
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  SIMDCompressionLib::VarIntGB<true> codec;
  decodeWithSIMDCompressionCodec(codec, 1, data, state, stats);
}

static void encodeWithDeltaBinaryPacking(
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  SIMDCompressionLib::BinaryPacking<SIMDCompressionLib::BasicBlockPacker> codec;
  encodeWithSIMDCompressionCodec(codec, codec.BlockSize, data, state, stats);
}

static void decodeWithDeltaBinaryPacking(
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  SIMDCompressionLib::BinaryPacking<SIMDCompressionLib::BasicBlockPacker> codec;
  decodeWithSIMDCompressionCodec(codec, codec.BlockSize, data, state, stats);
}

static void encodeWithDeltaFastPFor128(
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  SIMDCompressionLib::FastPFor<4, true> codec;
  encodeWithSIMDCompressionCodec(codec, codec.BlockSize, data, state, stats);
}

static void decodeWithDeltaFastPFor128(
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  SIMDCompressionLib::FastPFor<4, true> codec;
  encodeWithSIMDCompressionCodec(codec, codec.BlockSize, data, state, stats);
}

static void encodeWithDeltaFastPFor256(
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  SIMDCompressionLib::FastPFor<8, true> codec;
  encodeWithSIMDCompressionCodec(codec, codec.BlockSize, data, state, stats);
}

static void decodeWithDeltaFastPFor256(
  std::vector<uint32_t>& data,
  benchmark::State& state,
  CompressionStats& stats)
{
  SIMDCompressionLib::FastPFor<8, true> codec;
  encodeWithSIMDCompressionCodec(codec, codec.BlockSize, data, state, stats);
}

BENCHMARK_F(Gov2SortedDataSet, Copy)(benchmark::State& state) {
  benchmarkGov2SortedDataSet(this, state, encodeWithCopy);
}

BENCHMARK_DEFINE_F(Gov2SortedDataSet, VTEncEncode)(benchmark::State& state) {
  benchmarkGov2SortedDataSet(this, state, encodeWithVTEnc);
}

BENCHMARK_REGISTER_F(Gov2SortedDataSet, VTEncEncode)
  ->RangeMultiplier(2)->Range(1, 1<<8);

BENCHMARK_DEFINE_F(Gov2SortedDataSet, VTEncDecode)(benchmark::State& state) {
  benchmarkGov2SortedDataSet(this, state, decodeWithVTEnc);
}

BENCHMARK_REGISTER_F(Gov2SortedDataSet, VTEncDecode)
  ->RangeMultiplier(2)->Range(1, 1<<8);

BENCHMARK_F(Gov2SortedDataSet, DeltaVariableByteEncode)(benchmark::State& state) {
  benchmarkGov2SortedDataSet(this, state, encodeWithDeltaVariableByte);
}

BENCHMARK_F(Gov2SortedDataSet, DeltaVariableByteDecode)(benchmark::State& state) {
  benchmarkGov2SortedDataSet(this, state, decodeWithDeltaVariableByte);
}

BENCHMARK_F(Gov2SortedDataSet, DeltaVarIntGBEncode)(benchmark::State& state) {
  benchmarkGov2SortedDataSet(this, state, encodeWithDeltaVarIntGB);
}

BENCHMARK_F(Gov2SortedDataSet, DeltaVarIntGBDecode)(benchmark::State& state) {
  benchmarkGov2SortedDataSet(this, state, decodeWithDeltaVarIntGB);
}

BENCHMARK_F(Gov2SortedDataSet, DeltaBinaryPackingEncode)(benchmark::State& state) {
  benchmarkGov2SortedDataSet(this, state, encodeWithDeltaBinaryPacking);
}

BENCHMARK_F(Gov2SortedDataSet, DeltaBinaryPackingDecode)(benchmark::State& state) {
  benchmarkGov2SortedDataSet(this, state, decodeWithDeltaBinaryPacking);
}

BENCHMARK_F(Gov2SortedDataSet, DeltaFastPFor128Encode)(benchmark::State& state) {
  benchmarkGov2SortedDataSet(this, state, encodeWithDeltaFastPFor128);
}

BENCHMARK_F(Gov2SortedDataSet, DeltaFastPFor128Decode)(benchmark::State& state) {
  benchmarkGov2SortedDataSet(this, state, decodeWithDeltaFastPFor128);
}

BENCHMARK_F(Gov2SortedDataSet, DeltaFastPFor256Encode)(benchmark::State& state) {
  benchmarkGov2SortedDataSet(this, state, encodeWithDeltaFastPFor256);
}

BENCHMARK_F(Gov2SortedDataSet, DeltaFastPFor256Decode)(benchmark::State& state) {
  benchmarkGov2SortedDataSet(this, state, decodeWithDeltaFastPFor256);
}
