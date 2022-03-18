This is a repository to benchmark integer compression algorithms with **sorted lists of integers**.

## Data sets

These are the data sets that have been tested so far:

* `ts.txt`: a text file with a large list of timestamps. It can be downloaded from [here](https://github.com/zentures/encoding/tree/master/benchmark/data).

* `gov2.sorted`: a binary file containing a sequence of sorted lists of 32-bit integers. This file is part of the "Document identifier data set" created by [D. Lemire](https://lemire.me/en/). It can be downloaded from [here](https://lemire.me/data/integercompression2014.html).

## Results

The following results have been obtained on a laptop Ubuntu Desktop 21.10 with a Core i7-6700HQ CPU @ 2.60GHz x 8.

* `ts.txt`:

 | Algorithm          |Encoded Size|Compression Ratio|Encoding Speed |Decoding Speed|
 |:-------------------|-----------:|----------------:|--------------:|-------------:|
 | VTEnc/1 (\*)       |  **21,679**|       **26,622**| **102.28 G/s**|  **7.32 G/s**|
 | Delta+FastPFor256  |   1,179,312|            489.4|       1.45 G/s|      3.61 G/s|
 | Delta+FastPFor128  |   2,306,544|            250.2|       1.58 G/s|      3.88 G/s|
 | Delta+BinaryPacking|   4,552,280|            126.8|       6.76 G/s|      4.31 G/s|
 | Delta+VariableByte | 144,285,504|              4.0|       3.89 G/s|      3.43 G/s|
 | Delta+VarIntGB     | 180,356,880|              3.2|       5.05 G/s|      6.73 G/s|
 | Copy               | 577,141,992|              1.0|      11.83 G/s|       -      |

* `gov2.sorted`:

 | Algorithm          |Encoded Size     |Compression Ratio|Encoding Speed|Decoding Speed|
 |:-------------------|----------------:|----------------:|-------------:|-------------:|
 | VTEnc/1            |**2,885,170,285**|         **8.29**|    154.76 M/s|    210.99 M/s|
 | VTEnc/2            |    2,930,842,147|             8.16|    213.90 M/s|    282.39 M/s|
 | VTEnc/4            |    3,144,764,347|             7.61|    306.40 M/s|    411.75 M/s|
 | VTEnc/8            |    3,482,164,190|             6.87|    443.06 M/s|    594.89 M/s|
 | Delta+FastPFor128  |    3,849,161,656|             6.21|    615.66 M/s|    615.64 M/s|
 | Delta+FastPFor256  |    3,899,341,376|             6.13|    621.15 M/s|    621.17 M/s|
 | VTEnc/16           |    3,922,307,294|             6.10|    639.35 M/s|    829.11 M/s|
 | Delta+BinaryPacking|    4,329,919,808|             5.52|  **2.30 G/s**|      2.33 G/s|
 | VTEnc/32           |    4,447,792,753|             5.38|    901.11 M/s|  1,124.46 M/s|
 | VTEnc/64           |    5,044,828,934|             4.74|      1.15 G/s|      1.42 G/s|
 | VTEnc/128          |    5,719,368,620|             4.18|      1.42 G/s|      1.68 G/s|
 | VTEnc/256          |    6,561,323,112|             3.65|      1.60 G/s|      1.83 G/s|
 | Delta+VariableByte |    6,572,084,696|             3.64|      1.62 G/s|      1.52 G/s|
 | Delta+VarIntGB     |    7,923,819,720|             3.01|      2.03 G/s|  **2.97 G/s**|
 | Copy               |   23,918,860,964|             1.00|      5.10 G/s|       -      |

 (\*) Number alongside VTEnc indicates value of `min_cluster_length` encoding parameter.

## Included libraries

* [benchmark v1.6.1](https://github.com/google/benchmark/tree/v1.6.1): benchmarking library created by Google.
* [SIMDCompressionAndIntersection](https://github.com/lemire/SIMDCompressionAndIntersection): C++ library to encode/decode sorted lists of integers. It provides a series of integer algorithms optimised to work with sorted lists.
* [VTEnc v0.3.0](https://github.com/vteromero/VTEnc/tree/v0.3.0): C library that implements VTEnc algorithm.

## Building

To build this project, clone it in this way:

```bash
git clone --recurse-submodules https://github.com/vteromero/integer-compression-benchmarks
```

Then, build the included libraries. It's highly recommended to follow the build instructions for each library in their respective repositories. However, for convenience, here is a quick list of instructions:

#### Build benchmark

```bash
# Go to the library root directory
$ cd benchmark
# Make a build directory to place the build output.
$ cmake -E make_directory "build"
# Generate build system files with cmake, and download any dependencies.
$ cmake -E chdir "build" cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on -DCMAKE_BUILD_TYPE=Release ../
# or, starting with CMake 3.13, use a simpler form:
# cmake -DCMAKE_BUILD_TYPE=Release -S . -B "build"
# Build the library.
$ cmake --build "build" --config Release
```

#### Build SIMDCompressionAndIntersection

```bash
cd SIMDCompressionAndIntersection
make
```

#### Build VTEnc

```bash
cd VTEnc
make
```

Once the included libraries have been built, run `make` in the root directory. That will generate the executable `intbench`.

## Running

You can invoke `intbench` in a variety of ways. Here are some examples:

```bash
# '--data-dir' option is required and indicates the path to the directory that
# contains the data sets.
./intbench --data-dir=/home/user/data

# With '--benchmark_filter' you can choose which benchmarks to run.
./intbench --data-dir=/home/user/data --benchmark_filter=TimestampsDataSet

# '--benchmark_list_tests' lists all the available benchmarks.
./intbench --data-dir=/home/user/data --benchmark_list_tests

# Use '--help' to get more information about the available options.
./intbench --data-dir=/home/user/data --help
```

## License

This code is licensed under MIT license.
