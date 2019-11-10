This is a repository to benchmark integer compression algorithms with **sorted lists of integers**.

## Data sets

These are the data sets that have been tested so far:

* `ts.txt`: a text file with a large list of timestamps. It can be downloaded from [here](https://github.com/zentures/encoding/tree/master/benchmark/data).

## Results

The following results have been obtained on a laptop Ubuntu Desktop 19.04 with a Core i7-6700HQ CPU @ 2.60GHz x 8.

* `ts.txt`:

| Algorithm          |Encoded Size|Ratio %|Encoding Speed|Decoding Speed|
|:-------------------|-----------:|------:|-------------:|-------------:|
| VTEnc              |  **21,686**| **0.0038**|**60.9155 G/s**|   734.54 M/s |
| Delta+FastPFor256  |   1,179,312|   0.20|  2.00714 G/s |  4.75146 G/s |
| Delta+FastPFor128  |   2,306,544|   0.40|   1.9029 G/s |  4.82925 G/s |
| Delta+BinaryPacking|   4,552,280|   0.79|   8.5867 G/s |  5.77439 G/s |
| Delta+VariableByte | 144,285,504|   25.0|  4.86063 G/s |  5.09461 G/s |
| Delta+VarIntGB     | 180,356,880|  31.25|  6.75428 G/s |**9.2638 G/s**|
| Copy               | 577,141,992|  100.0|  10.4087 G/s | - |

## Included libraries

* [benchmark](https://github.com/google/benchmark): benchmarking library created by Google.
* [SIMDCompressionAndIntersection](https://github.com/lemire/SIMDCompressionAndIntersection): C++ library to encode/decode sorted lists of integers. It provides a series of integer algorithms optimised to work with sorted lists.
* [VTEnc](https://github.com/vteromero/VTEnc): C library that implements VTEnc algorithm.

## Building

To build this project, you first need to build the included libraries. It's highly recommended to follow the build instructions for each library in their respective repositories. However, for convenience, here is a quick list of instructions:

#### Build benchmark

```bash
# Benchmark requires Google Test as a dependency. Add the source tree as a subdirectory.
$ git clone https://github.com/google/googletest.git benchmark/googletest
# Go to the library root directory
$ cd benchmark
# Make a build directory to place the build output.
$ mkdir build && cd build
# Generate a Makefile with cmake.
# Use cmake -G <generator> to generate a different file type.
$ cmake -DCMAKE_BUILD_TYPE=Release ../
# Build the library.
# Use make -j<number_of_parallel_jobs> to speed up the build process, e.g. make -j8 .
$ make
```

#### Build SIMDCompressionAndIntersection

```bash
$ cd SIMDCompressionAndIntersection
$ make
```

#### Build VTEnc

```bash
$ cd VTEnc
$ make
```

Once the included libraries have been built, run `make` in the root directory. That will generate the executable `intbench`.

## Running

You can invoke `intbench` in a variety of ways. Here are some examples:

```bash
# '--data-dir' option is required and indicates the path to the directory that
# contains the data sets.
$ ./intbench --data-dir=/home/user/data

# With '--benchmark_filter' you can choose which benchmarks to run.
$ ./intbench --data-dir=/home/user/data --benchmark_filter=TimestampsDataSet

# '--benchmark_list_tests' lists all the available benchmarks.
$ ./intbench --data-dir=/home/user/data --benchmark_list_tests

# Use '--help' to get more information about the available options.
$ ./intbench --data-dir=/home/user/data --help
```

## License

This code is licensed under MIT license.
