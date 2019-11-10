# Copyright (c) 2019 Vicente Romero Calero. All rights reserved.
# Licensed under the MIT License.
# See LICENSE file in the project root for full license information.

CC = g++
CPPFLAGS = -Wall -O3 -std=c++11
LDFLAGS = -lpthread

BENCHMARKDIR = benchmark
VTENCDIR = VTEnc
SIMDCOMPDIR = SIMDCompressionAndIntersection

BENCHMARKLIB = $(BENCHMARKDIR)/build/src/libbenchmark.a
VTENCLIB = $(VTENCDIR)/libvtenc.a
SIMDCOMPLIB = $(SIMDCOMPDIR)/libSIMDCompressionAndIntersection.a

.PHONY: default
default: all

.PHONY: all
all: intbench

%.o: %.cc
	${CC} -c $(CPPFLAGS) $<

intbench: intbench.o
	$(CC) $(CPPFLAGS) $^ $(BENCHMARKLIB) $(VTENCLIB) $(SIMDCOMPLIB) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -f *.o intbench
