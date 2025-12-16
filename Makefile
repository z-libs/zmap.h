
BUNDLER = z-core/zbundler.py
SRC     = src/zmap.c
DIST    = zmap.h

CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -std=c11 -O2 -I.
CXXFLAGS = -Wall -Wextra -std=c++11 -O2 -I.

BENCH_DIR_C  = benchmarks/c
UTHASH_URL = https://raw.githubusercontent.com/troydhanson/uthash/master/src

all: bundle get_zerror_h

bundle:
	@echo "Bundling $(DIST)..."
	python3 $(BUNDLER) $(SRC) $(DIST)

get_zerror_h:
	@echo "Using wget to add 'zerror.h'..."
	wget -q "https://raw.githubusercontent.com/z-libs/zerror.h/main/zerror.h" -O "zerror.h"

download_uthash:
	@mkdir -p $(BENCH_DIR_C)
	@if [ ! -f $(BENCH_DIR_C)/uthash.h ]; then \
		echo "Downloading uthash for benchmarks..."; \
		wget -q $(UTHASH_URL)/uthash.h -O $(BENCH_DIR_C)/uthash.h; \
	fi

bench_int: bundle download_uthash
	@echo "=> Compiling Integer Benchmark..."
	gcc -O3 -o $(BENCH_DIR_C)/bench_uthash_int $(BENCH_DIR_C)/bench_uthash.c -I. -I$(BENCH_DIR_C)
	@echo "Running..."
	./$(BENCH_DIR_C)/bench_uthash_int

bench_str: bundle download_uthash
	@echo "=> Compiling String Benchmark..."
	gcc -O3 -o $(BENCH_DIR_C)/bench_uthash_str $(BENCH_DIR_C)/bench_uthash_str.c -I. -I$(BENCH_DIR_C)
	@echo "Running..."
	./$(BENCH_DIR_C)/bench_uthash_str

bench_btc: bundle download_uthash
	@echo "=> Compiling Bitcoin Benchmark..."
	gcc -O3 -o $(BENCH_DIR_C)/bench_uthash_btc $(BENCH_DIR_C)/bench_uthash_btc.c -I. -I$(BENCH_DIR_C)
	@echo "Running..."
	./$(BENCH_DIR_C)/bench_uthash_btc

bench_btc_large: bundle download_uthash
	@echo "=> Compiling Bitcoin (Large) Benchmark..."
	gcc -O3 -o $(BENCH_DIR_C)/bench_uthash_btc_large $(BENCH_DIR_C)/bench_uthash_btc_large.c -I. -I$(BENCH_DIR_C)
	@echo "Running..."
	./$(BENCH_DIR_C)/bench_uthash_btc_large


bench: bench_int bench_str bench_btc bench_btc_large clean_bench

clean: clean_bench clean_zerror

clean_bench:
	rm -f $(BENCH_DIR_C)/bench_uthash_int $(BENCH_DIR_C)/bench_uthash_str $(BENCH_DIR_C)/bench_uthash_btc $(BENCH_DIR_C)/bench_uthash_btc_large
	rm -f $(BENCH_DIR_C)/uthash.h
clean_zerror:
	@echo "Removing 'zerror.h'..."
	@rm zerror.h

init:
	git submodule update --init --recursive

test: bundle get_zerror_h test_c test_cpp clean_zerror

test_c:
	@echo "----------------------------------------"
	@echo "Building C Tests..."
	@$(CC) $(CFLAGS) tests/test_main.c -o tests/runner_c
	@./tests/runner_c
	@rm tests/runner_c

test_cpp:
	@echo "----------------------------------------"
	@echo "Building C++ Tests..."
	@$(CXX) $(CXXFLAGS) tests/test_cpp.cpp -o tests/runner_cpp
	@./tests/runner_cpp
	@rm tests/runner_cpp

.PHONY: all get_zerror_h bundle download_uthash bench bench_int bench_str bench_btc clean clean_bench clean_zerror init test test_c test_cpp
