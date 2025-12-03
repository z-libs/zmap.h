# Configuration.
BUNDLER = z-core/zbundler.py
SRC     = src/zmap.c
DIST    = zmap.h

# Benchmark settings.
BENCH_DIR_C  = benchmarks/c
UTHASH_URL = https://raw.githubusercontent.com/troydhanson/uthash/master/src

all: bundle

bundle:
	@echo "Bundling $(DIST)..."
	python3 $(BUNDLER) $(SRC) $(DIST)

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

bench: bench_int bench_str bench_btc

clean:
	rm -f $(BENCH_DIR_C)/bench_uthash_int $(BENCH_DIR_C)/bench_uthash_str $(BENCH_DIR_C)/bench_uthash_btc
	# Optional: remove downloaded headers to force fresh fetch
	# rm -f $(BENCH_DIR_C)/uthash.h

init:
	git submodule update --init --recursive

.PHONY: all bundle download_uthash bench bench_int bench_str bench_btc clean init