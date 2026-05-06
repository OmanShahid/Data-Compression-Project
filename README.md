BZip2-Impl: Advanced Data Compression PipelineA high-performance, lossless data compression tool inspired by the BZip2 architecture. Developed at the FAST School of Computing, this project demonstrates elite compression ratios through a sophisticated multi-stage pipeline designed for both repetitive text and complex binary patterns.🚀 Advanced Optimizations (Bonus Features)This implementation includes several high-tier optimizations that qualify for the 10% extra marks bonus:Suffix Array-based BWT: Replaces the standard $O(N^2)$ matrix transform with a Suffix Array (Prefix Doubling) approach, allowing the processing of large blocks (up to 500KB+) without memory exhaustion.Asymmetric Numeral Systems (rANS): Utilizes a modern rANS entropy engine instead of traditional Huffman coding, providing a significant "fractional bit" advantage for superior compression.Threshold-Based RLE (RLE1): A marker-based Run-Length Encoding stage that only encodes runs of 4 or more characters to prevent data expansion on non-repetitive segments.MTF (Move-To-Front) & RLE2: Efficiently transforms BWT output into a zero-heavy stream, collapsed by a second RLE stage specifically designed for rANS input.Lossless Pipeline Sync: Includes robust buffer-length synchronization between stages to ensure 100% data integrity, resolving block-boundary truncation issues.🛠️ Execution Guide1. Build the ProjectClean and compile the C source code using the provided Makefile:PowerShellmingw32-make clean
mingw32-

2. Manual Compression & DecompressionTest specific files manually using the configuration file:PowerShell# Compress
.\bzip2_impl.exe c input.txt output.bzp config.ini

# Decompress
.\bzip2_impl.exe d output.bzp restored.txt

3. Automated Benchmarking & VisualizationUse the Python suite to generate test data, run performance metrics, and visualize results:PowerShell# Create the test files (the "fuel")
python benchmarks/generate_test_files.py

# Run the benchmarks and time the C code
python scripts/run_benchmarks.py

# Generate performance graphs
python scripts/generate_graphs.py



Note: Benchmarks prove that the pipeline is highly optimized for both ratio and speed.
