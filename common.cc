// Copyright (c) 2019 Vicente Romero Calero. All rights reserved.
// Licensed under the MIT License.
// See LICENSE file in the project root for full license information.

#include "common.h"

#include <algorithm>
#include <string>
#include <vector>

#include "benchmark/include/benchmark/benchmark.h"
#include "SIMDCompressionAndIntersection/include/codecs.h"
#include "SIMDCompressionAndIntersection/include/variablebyte.h"

std::string GlobalState::dataDirectory = std::string();

CompressionStats::CompressionStats(benchmark::State& state): _state(state) {
  Reset();
}

void CompressionStats::SetInputLengthInBytes(size_t len) {
  _state.counters["inputLength"] = len;
}

void CompressionStats::UpdateInputLengthInBytes(size_t len) {
  _state.counters["inputLength"] += len;
}

void CompressionStats::ResetInputLengthInBytes() {
  _state.counters["inputLength"] = 0;
}

void CompressionStats::SetEncodedLengthInBytes(size_t len) {
  _state.counters["encodedLength"] = len;
}

void CompressionStats::UpdateEncodedLengthInBytes(size_t len) {
  _state.counters["encodedLength"] += len;
}

void CompressionStats::ResetEncodedLengthInBytes() {
  _state.counters["encodedLength"] = 0;
}

void CompressionStats::SetFinalStats() {
  _state.counters["compressionRatio"] = _state.counters["encodedLength"] / _state.counters["inputLength"];
  _state.SetBytesProcessed(int64_t(_state.iterations()) * int64_t(_state.counters["inputLength"]));
}

void CompressionStats::Reset() {
  ResetInputLengthInBytes();
  ResetEncodedLengthInBytes();
}

SIMDCompressionUtil::SIMDCompressionUtil(
  SIMDCompressionLib::IntegerCODEC& codec,
  size_t blockSize,
  std::vector<uint32_t>& data): _codec(codec), _data(data)
{
  _inputLength = data.size();
  _inputLength2 = _inputLength % blockSize;
  _inputLength1 = _inputLength - _inputLength2;
  _copyOfData.resize(_inputLength);
  _encoded.resize(_inputLength + 1024);
  _encodedLength1 = _encoded.size();
  _encodedLength2 = _encoded.size();
  _decoded.resize(_inputLength);

  std::copy(_data.begin(), _data.end(), _copyOfData.begin());
}

void SIMDCompressionUtil::Reset(){
  std::copy(_data.begin(), _data.end(), _copyOfData.begin());
}

void SIMDCompressionUtil::Encode() {
  _codec.encodeArray(_copyOfData.data(), _inputLength1, _encoded.data(), _encodedLength1);
  if (_inputLength2) {
    _fallbackCodec.encodeArray(_copyOfData.data() + _inputLength1, _inputLength2, _encoded.data() + _encodedLength1, _encodedLength2);
  }
}

void SIMDCompressionUtil::Decode() {
  _codec.decodeArray(_encoded.data(), _encodedLength1, _decoded.data(), _inputLength1);
  if (_inputLength2) {
    _fallbackCodec.decodeArray(_encoded.data() + _encodedLength1, _encodedLength2, _decoded.data() + _inputLength1, _inputLength2);
  }
}

size_t SIMDCompressionUtil::InputLength() {
  return _inputLength;
}

size_t SIMDCompressionUtil::EncodedLength() {
  return (_inputLength2) ? (_encodedLength1 + _encodedLength2) : _encodedLength1;
}

void SIMDCompressionUtil::EqualityCheck() {
  if (_data != _decoded) {
    throw std::logic_error("equality check failed");
  }
}
