This is a repository to benchmark integer compression algorithms with **sorted lists of integers**.

## Data sets

These are the data sets that have been tested so far:

* `ts.txt`: a text file with a large list of timestamps. It can be downloaded from [here](https://github.com/zentures/encoding/tree/master/benchmark/data).

* `gov2.sorted`: a binary file containing a sequence of sorted lists of 32-bit integers. This file is part of the "Document identifier data set" created by [D. Lemire](https://lemire.me/en/). It can be downloaded from [here](https://lemire.me/data/integercompression2014.html).

## Results

The following results have been obtained on a laptop Ubuntu Desktop 19.04 with a Core i7-6700HQ CPU @ 2.60GHz x 8.

* `ts.txt`:

| Algorithm          |Encoded Size|Ratio %    |Encoding Speed |Decoding Speed|
|:-------------------|-----------:|----------:|--------------:|-------------:|
| VTEnc              |  **21,686**| **0.0038**|  **60.92 G/s**|    734.54 M/s|
| Delta+FastPFor256  |   1,179,312|       0.20|       2.01 G/s|      4.75 G/s|
| Delta+FastPFor128  |   2,306,544|       0.40|       1.90 G/s|      4.83 G/s|
| Delta+BinaryPacking|   4,552,280|       0.79|       8.59 G/s|      5.77 G/s|
| Delta+VariableByte | 144,285,504|       25.0|       4.86 G/s|      5.09 G/s|
| Delta+VarIntGB     | 180,356,880|      31.25|       6.75 G/s|  **9.26 G/s**|
| Copy               | 577,141,992|      100.0|      10.41 G/s|       -      |

* `gov2.sorted`:

| Algorithm          |Encoded Size     |Ratio %  |Encoding Speed|Decoding Speed|
|:-------------------|----------------:|--------:|-------------:|-------------:|
| VTEnc              |**2,889,599,350**|**12.08**|     77.69 M/s|     69.08 M/s|
| Delta+FastPFor128  |    3,849,161,656|    16.09|    855.60 M/s|    850.43 M/s|
| Delta+FastPFor256  |    3,899,341,376|    16.30|    915.43 M/s|    916.01 M/s|
| Delta+BinaryPacking|    4,329,919,808|    18.10|  **3.03 G/s**|      2.94 G/s|
| Delta+VariableByte |    6,572,084,696|    27.48|      2.05 G/s|      2.09 G/s|
| Delta+VarIntGB     |    7,923,819,720|    33.13|      2.54 G/s|  **3.73 G/s**|
| Copy               |   23,918,861,764|    100.0|      6.27 G/s|       -      |

## Included libraries

* [benchmark](https://github.com/google/benchmark): benchmarking library created by Google.
* [SIMDCompressionAndIntersection](https://github.com/lemire/SIMDCompressionAndIntersection): C++ library to encode/decode sorted lists of integers. It provides a series of integer algorithms optimised to work with sorted lists.
* [VTEnc](https://github.com/vteromero/VTEnc): C library that implements VTEnc algorithm.

## Building

To build this project, clone it in this way:

```bash
$ git clone --recurse-submodules https://github.com/vteromero/integer-compression-benchmarks
```

Then, you need to build the included libraries. It's highly recommended to follow the build instructions for each library in their respective repositories. However, for convenience, here is a quick list of instructions:

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