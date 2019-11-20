// Copyright (c) 2019 Vicente Romero Calero. All rights reserved.
// Licensed under the MIT License.
// See LICENSE file in the project root for full license information.

#include <iostream>
#include <string>

#include "common.h"

#include "benchmark/include/benchmark/benchmark.h"

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
