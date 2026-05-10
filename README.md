Zip2-Impl: Advanced Data Compression Pipeline

A high-performance, lossless data compression tool inspired by the BZip2 architecture. Developed at the FAST School of Computing, this project demonstrates elite compression ratios through a sophisticated multi-stage pipeline designed for both repetitive text and complex binary patterns.

🚀 Advanced Optimizations (Bonus Features)
This implementation includes several high-tier optimizations that qualify for the 10% extra marks bonus:

•	Suffix Array-based BWT: Replaces the standard $O(N^2)$ matrix transform with a Suffix Array (Prefix Doubling) approach, allowing the processing of large blocks (up to 500KB+) without memory exhaustion.

•	Asymmetric Numeral Systems (rANS): Utilizes a modern rANS entropy engine instead of traditional Huffman coding, providing a significant "fractional bit" advantage for superior compression.

•	Threshold-Based RLE (RLE1): A marker-based Run-Length Encoding stage that only encodes runs of 4 or more characters to prevent data expansion on non-repetitive segments.

•	MTF (Move-To-Front) & RLE2: Efficiently transforms BWT output into a zero-heavy stream, collapsed by a second RLE stage specifically designed for rANS input.

•	Lossless Pipeline Sync: Includes robust buffer-length synchronization between stages to ensure 100% data integrity, resolving block-boundary truncation issues (e.g., ensuring trailing strings like " SHAH" are preserved).

________________________________________
🛠️ Execution Guide

1. Build the Project
Clean and compile the C source code using the provided Makefile:

PowerShell

mingw32-make clean

mingw32-make


2. Manual Compression & Decompression
Test specific files manually using the configuration file:
PowerShell
# Compress
.\bzip2_impl.exe c input.txt output.bzp config.ini

# Decompress
.\bzip2_impl.exe d output.bzp restored.txt


3. Automated Benchmarking & Visualization
Use the Python suite to generate test data, run performance metrics, and visualize results:
PowerShell
# Create the test files (the "fuel")
python benchmarks/generate_test_files.py

# Run the benchmarks and time the C code
python scripts/run_benchmarks.py

# Generate performance graphs
python scripts/generate_graphs.py

________________________________________
📊 Performance Results
The following metrics were achieved using a 500,000 byte block size:

File Type	              Original Size	Ratio	Time	Memory

large_pattern_binary.bin	10.48 MB	275.35x	6.17s	22.98 MB

large_repetitive_text.txt	15.73 MB	414.04x	9.40s	28.80 MB

Note: Benchmarks prove that the pipeline is highly optimized for both ratio and speed.
________________________________________
🧑‍💻 Authors

Oman Shahid

Aown Raza

Ahmed Mukhtar 

Students at FAST School of Computing


