// Copyright (c) 2019 Vicente Romero Calero. All rights reserved.
// Licensed under the MIT License.
// See LICENSE file in the project root for full license information.

#ifndef INTCOMPBENCH_COMMON_H_
#define INTCOMPBENCH_COMMON_H_

#include <string>
#include <vector>

#include "benchmark/include/benchmark/benchmark.h"
#include "SIMDCompressionAndIntersection/include/codecs.h"
#include "SIMDCompressionAndIntersection/include/variablebyte.h"

struct GlobalState {
  static std::string dataDirectory;
};

class CompressionStats {
private:
  benchmark::State& _state;

public:
  CompressionStats(benchmark::State& state);

  void SetInputLengthInBytes(size_t len);
  void UpdateInputLengthInBytes(size_t len);
  void ResetInputLengthInBytes();
  void SetEncodedLengthInBytes(size_t len);
  void UpdateEncodedLengthInBytes(size_t len);
  void ResetEncodedLengthInBytes();
  void SetFinalStats();
  void Reset();
};

class SIMDCompressionUtil {
private:
  SIMDCompressionLib::VByte<true> _fallbackCodec;
  SIMDCompressionLib::IntegerCODEC& _codec;
  std::vector<uint32_t>& _data;
  size_t _inputLength;
  size_t _inputLength1;
  size_t _inputLength2;
  std::vector<uint32_t> _copyOfData;
  std::vector<uint32_t> _encoded;
  size_t _encodedLength1;
  size_t _encodedLength2;
  std::vector<uint32_t> _decoded;

public:
  SIMDCompressionUtil(
    SIMDCompressionLib::IntegerCODEC& codec,
    size_t blockSize,
    std::vector<uint32_t>& data);

  void Reset();
  void Encode();
  void Decode();
  size_t InputLength();
  size_t EncodedLength();
  void EqualityCheck();
};

#endif // INTCOMPBENCH_COMMON_H_
