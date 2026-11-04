# cache_simulator
# Cache Memory Simulator (C++)

A configurable **Cache Memory Simulator** developed in C++ to demonstrate and analyse cache memory organization, mapping techniques, replacement algorithms, and write policies.

The simulator processes memory access traces and evaluates cache performance through hit/miss statistics, miss classification, and Average Memory Access Time (AMAT).

## Features

### 1. Cache Mapping Techniques
- Direct-Mapped Cache
- Fully Associative Cache
- Set-Associative Cache

### 2. Replacement Policies
- FIFO (First In First Out)
- LRU (Least Recently Used)
- MRU (Most Recently Used)
- Random Replacement

### 3. Write Policies
- Write-Through
- Write-Back
- Write-Allocate assumption

### 4. Trace File Support

Supports multiple memory access trace generation methods:
- File-based trace input
- Manual memory access input
- Sequential address generation
- Looping address patterns
- Random memory access generation

### 5. Performance Analysis

Calculates:
- Cache hits and misses
- Hit ratio and miss ratio
- Compulsory misses
- Capacity misses
- Conflict misses
- Average Memory Access Time (AMAT)
- Write-through memory writes
- Write-back dirty evictions and final flush writes

## Technologies Used

- C++
- Standard Template Library (STL)
- File Handling (ifstream)
- Data Structures (Vectors)
- Cache Memory Organization and Architecture

## Getting Started

### Prerequisites
- C++ compiler (GCC/G++)
- C++11 or later

### Compilation

Clone the repository:

```bash
git clone <repository-url>
cd <repository-folder>
```

Compile the program:

```bash
g++ -std=c++11 main.cpp -o cache_simulator
```

Run:

```bash
./cache_simulator
```

On Windows:

```bash
cache_simulator.exe
```

## Configuration

The simulator allows users to configure:

- Main memory size (bytes)
- Cache size (bytes)
- Block size (bytes)
- Default associativity (ways)

Alternatively, entering a non-zero roll number generates a configuration automatically based on the roll number.

All memory and cache parameters must satisfy the power-of-two and size constraints implemented in the program.

## Using a Trace File

The simulator supports external memory access trace files.

Each line must contain:
- Operation: `R` for Read or `W` for Write
- Memory address: Decimal address

Example (`sample_cache_trace.txt`):

```text
R 0
R 4
W 8
R 16
R 20
W 64
R 0
R 64
W 128
R 132
```

Select the integrated simulator (option 4), choose file-based trace input (option 1), and enter the trace filename.

The file must be accessible from the program's working directory.

## Average Memory Access Time (AMAT)

The simulator calculates AMAT using:

\[
AMAT = Hit\ Time + (Miss\ Ratio \times Miss\ Penalty)
\]

Where:
- Hit Time: Time required to access the cache.
- Miss Ratio: Fraction of memory accesses resulting in cache misses.
- Miss Penalty: Additional time required for a cache miss.

## Output

The simulator displays:

1. Memory access sequence and operation.
2. Block number, tag, set index, and offset.
3. HIT/MISS result for every access.
4. Classification of cache misses.
5. Overall cache performance statistics.
6. Write-policy memory write statistics.
7. Final cache contents, including dirty blocks.

## Project Structure

```text
Cache-Memory-Simulator/
│
├── main.cpp
├── sample_cache_trace.txt
└── README.md
```

## Learning Objectives

This project explores fundamental concepts of computer architecture and memory hierarchy, including:

1. Cache mapping and organization
2. Address decomposition (Tag, Index, Offset)
3. Cache replacement strategies
4. Write policies and dirty blocks
5. Cache miss classification
6. Cache performance evaluation

## Future Improvements

- Graphical visualization of cache operations
- Comparison of multiple cache configurations
- Multilevel cache simulation (L1, L2, L3)
- Performance graphs for different replacement policies
- More realistic workload trace generation

