# SPU C++ Reference Implementation Makefile

CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra -fopenmp
LDFLAGS = -fopenmp

# Targets
all: test_merge

test_merge: test_merge.cpp merge_reference.cpp sha256.h
	$(CXX) $(CXXFLAGS) -o test_merge test_merge.cpp $(LDFLAGS)

# Run tests
test: test_merge
	./test_merge

# Benchmark with different optimization levels
benchmark: test_merge
	@echo "=== O0 (no optimization) ==="
	$(CXX) -O0 -std=c++17 -o test_merge_O0 test_merge.cpp -fopenmp
	./test_merge_O0 | grep "Average latency"
	@echo ""
	@echo "=== O1 ==="
	$(CXX) -O1 -std=c++17 -o test_merge_O1 test_merge.cpp -fopenmp
	./test_merge_O1 | grep "Average latency"
	@echo ""
	@echo "=== O2 ==="
	$(CXX) -O2 -std=c++17 -o test_merge_O2 test_merge.cpp -fopenmp
	./test_merge_O2 | grep "Average latency"
	@echo ""
	@echo "=== O3 (default) ==="
	./test_merge | grep "Average latency"

# Clean build artifacts
clean:
	rm -f test_merge test_merge_O0 test_merge_O1 test_merge_O2 test_merge_O3
	rm -f *.o

# HLS synthesis (requires Vivado HLS)
hls: merge_hls.cpp
	@echo "HLS synthesis requires Vivado HLS"
	@echo "Run: vivado_hls -f synthesis_script.tcl"
	@echo "See README.md for details"

.PHONY: all test benchmark clean hls
