// Copyright (c) 2019 Vicente Romero Calero. All rights reserved.
// Licensed under the MIT License.
// See LICENSE file in the project root for full license information.

#include <algorithm>
#include <fstream>
#include <string>

#include "benchmark/include/benchmark/benchmark.h"
#include "SIMDCompressionAndIntersection/include/binarypacking.h"
#include "SIMDCompressionAndIntersection/include/fastpfor.h"
#include "SIMDCompressionAndIntersection/include/variablebyte.h"
#include "SIMDCompressionAndIntersection/include/varintgb.h"
#include "VTEnc/vtenc.h"

struct GlobalState {
  static std::string dataDirectory;
};

class TimestampsDataSet : public benchmark::Fixture {
private:
  void readTimestampsFromFile(std::string fileName) {
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
      readTimestampsFromFile(GlobalState::dataDirectory + std::string("/ts.txt"));
    }
  }

  void TearDown(const ::benchmark::State& state) {}

  void setStats(size_t inputLengthInBytes, size_t encodedLengthInBytes, benchmark::State& state) {
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(inputLengthInBytes));

    state.counters["inputLength"] = inputLengthInBytes;
    state.counters["encodedLength"] = encodedLengthInBytes;
    state.counters["compressionRatio"] = encodedLengthInBytes / double(inputLengthInBytes);
  }

  void encodeWithCodecs(
    SIMDCompressionLib::IntegerCODEC& codec1,
    SIMDCompressionLib::IntegerCODEC& codec2,
    size_t blockSize,
    benchmark::State& state)
  {
    size_t inputLength = timestamps.size();
    size_t inputLength2 = inputLength % blockSize;
    size_t inputLength1 = inputLength - inputLength2;
    std::vector<uint32_t> copyOfTimestamps(inputLength);
    std::vector<uint32_t> encoded(inputLength + 1024);
    size_t encodedLength1 = encoded.size();
    size_t encodedLength2 = encoded.size();

    for (auto _ : state) {
      state.PauseTiming();
      std::copy(timestamps.begin(), timestamps.end(), copyOfTimestamps.begin());
      state.ResumeTiming();

      codec1.encodeArray(copyOfTimestamps.data(), inputLength1, encoded.data(), encodedLength1);
      if (inputLength2) {
        codec2.encodeArray(copyOfTimestamps.data() + inputLength1, inputLength2, encoded.data() + encodedLength1, encodedLength2);
      }
    }

    size_t encodedLength = (inputLength2) ? (encodedLength1 + encodedLength2) : encodedLength1;
    setStats(inputLength * sizeof(uint32_t), encodedLength * sizeof(uint32_t), state);
  }

  void decodeWithCodecs(
    SIMDCompressionLib::IntegerCODEC& codec1,
    SIMDCompressionLib::IntegerCODEC& codec2,
    size_t blockSize,
    benchmark::State& state)
  {
    size_t inputLength = timestamps.size();
    size_t inputLength2 = inputLength % blockSize;
    size_t inputLength1 = inputLength - inputLength2;
    std::vector<uint32_t> copyOfTimestamps(inputLength);
    std::vector<uint32_t> encoded(inputLength + 1024);
    size_t encodedLength1 = encoded.size();
    size_t encodedLength2 = encoded.size();
    std::vector<uint32_t> decoded(inputLength);

    std::copy(timestamps.begin(), timestamps.end(), copyOfTimestamps.begin());

    codec1.encodeArray(copyOfTimestamps.data(), inputLength1, encoded.data(), encodedLength1);
    if (inputLength2) {
      codec2.encodeArray(copyOfTimestamps.data() + inputLength1, inputLength2, encoded.data() + encodedLength1, encodedLength2);
    }

    for (auto _ : state) {
      codec1.decodeArray(encoded.data(), encodedLength1, decoded.data(), inputLength1);
      if (inputLength2) {
        codec2.decodeArray(encoded.data() + encodedLength1, encodedLength2, decoded.data() + inputLength1, inputLength2);
      }
    }

    if (timestamps != decoded) {
      throw std::logic_error("equality check failed");
    }

    size_t encodedLength = (inputLength2) ? (encodedLength1 + encodedLength2) : encodedLength1;
    setStats(inputLength * sizeof(uint32_t), encodedLength * sizeof(uint32_t), state);
  }
};

BENCHMARK_F(TimestampsDataSet, Copy)(benchmark::State& state) {
  std::vector<uint32_t> copyTo(timestamps.size());

  for (auto _ : state)
    std::copy(timestamps.begin(), timestamps.end(), copyTo.begin());

  setStats(timestamps.size() * sizeof(uint32_t), timestamps.size() * sizeof(uint32_t), state);
}

BENCHMARK_F(TimestampsDataSet, VTEncEncode)(benchmark::State& state) {
  size_t encodedLength;
  std::vector<uint8_t> encoded(vtenc_list_max_encoded_size_u32(timestamps.size()));

  for (auto _ : state)
    vtenc_list_encode_u32(timestamps.data(), timestamps.size(), encoded.data(), encoded.size(), &encodedLength);

  setStats(timestamps.size() * sizeof(uint32_t), encodedLength, state);
}

BENCHMARK_F(TimestampsDataSet, VTEncDecode)(benchmark::State& state) {
  size_t encodedLength;
  std::vector<uint8_t> encoded(vtenc_list_max_encoded_size_u32(timestamps.size()));
  vtenc_list_encode_u32(timestamps.data(), timestamps.size(), encoded.data(), encoded.size(), &encodedLength);

  std::vector<uint32_t> decoded(vtenc_list_decoded_size_u32(encoded.data(), encodedLength));

  for (auto _ : state)
    vtenc_list_decode_u32(encoded.data(), encodedLength, decoded.data(), decoded.size());

  if (timestamps != decoded) {
    throw std::logic_error("equality check failed");
  }

  setStats(timestamps.size() * sizeof(uint32_t), encodedLength, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaVariableByteEncode)(benchmark::State& state) {
  SIMDCompressionLib::VByte<true> codec1;
  SIMDCompressionLib::VByte<true> codec2;
  encodeWithCodecs(codec1, codec2, 1, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaVariableByteDecode)(benchmark::State& state) {
  SIMDCompressionLib::VByte<true> codec1;
  SIMDCompressionLib::VByte<true> codec2;
  decodeWithCodecs(codec1, codec2, 1, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaVarIntGBEncode)(benchmark::State& state) {
  SIMDCompressionLib::VarIntGB<true> codec1;
  SIMDCompressionLib::VByte<true> codec2;
  encodeWithCodecs(codec1, codec2, 1, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaVarIntGBDecode)(benchmark::State& state) {
  SIMDCompressionLib::VarIntGB<true> codec1;
  SIMDCompressionLib::VByte<true> codec2;
  decodeWithCodecs(codec1, codec2, 1, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaBinaryPackingEncode)(benchmark::State& state) {
  SIMDCompressionLib::BinaryPacking<SIMDCompressionLib::BasicBlockPacker> codec1;
  SIMDCompressionLib::VByte<true> codec2;
  encodeWithCodecs(codec1, codec2, codec1.BlockSize, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaBinaryPackingDecode)(benchmark::State& state) {
  SIMDCompressionLib::BinaryPacking<SIMDCompressionLib::BasicBlockPacker> codec1;
  SIMDCompressionLib::VByte<true> codec2;
  decodeWithCodecs(codec1, codec2, codec1.BlockSize, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaFastPFor128Encode)(benchmark::State& state) {
  SIMDCompressionLib::FastPFor<4, true> codec1;
  SIMDCompressionLib::VByte<true> codec2;
  encodeWithCodecs(codec1, codec2, codec1.BlockSize, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaFastPFor128Decode)(benchmark::State& state) {
  SIMDCompressionLib::FastPFor<4, true> codec1;
  SIMDCompressionLib::VByte<true> codec2;
  decodeWithCodecs(codec1, codec2, codec1.BlockSize, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaFastPFor256Encode)(benchmark::State& state) {
  SIMDCompressionLib::FastPFor<8, true> codec1;
  SIMDCompressionLib::VByte<true> codec2;
  encodeWithCodecs(codec1, codec2, codec1.BlockSize, state);
}

BENCHMARK_F(TimestampsDataSet, DeltaFastPFor256Decode)(benchmark::State& state) {
  SIMDCompressionLib::FastPFor<8, true> codec1;
  SIMDCompressionLib::VByte<true> codec2;
  decodeWithCodecs(codec1, codec2, codec1.BlockSize, state);
}

void ParseArguments(int argc, char** argv) {
  const std::string dataDirFlag = std::string("--data-dir=");

  for (int i = 1; i < argc; ++i) {
    std::string opt(argv[i]);

    if (dataDirFlag.compare(opt.substr(0, dataDirFlag.length())) == 0) {
      GlobalState::dataDirectory = std::string(opt.substr(dataDirFlag.length()));
    }
  }
}

void Usage(const std::string& programName) {
  std::cerr << "Usage: " << programName << " --data-dir=DIRPATH [BENCHMARK_OPTIONS]" << std::endl;
}

std::string GlobalState::dataDirectory = std::string();
std::vector<uint32_t> TimestampsDataSet::timestamps = std::vector<uint32_t>();

int main(int argc, char** argv) {
  if (argc < 2) {
    Usage(std::string(argv[0]));
    return 1;
  }

  ParseArguments(argc, argv);

  if (GlobalState::dataDirectory.empty()) {
    std::cerr << "--data-dir argument not provided" << std::endl;
    Usage(std::string(argv[0]));
    return 1;
  }

  std::cout << "Data directory: " << GlobalState::dataDirectory << std::endl;

  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();

  return 0;
}
