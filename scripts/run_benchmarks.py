#!/usr/bin/env python3
import os
import time
import csv
import subprocess
import platform

try:
    import psutil
    PSUTIL_AVAILABLE = True
except ImportError:
    PSUTIL_AVAILABLE = False
    print("Warning: 'psutil' is not installed. Memory tracking will output 'N/A'.")
    print("Please install it using: pip install psutil\n")

def run_benchmarks():
    # Paths based on your folder structure
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    benchmarks_dir = os.path.join(project_root, "benchmarks")
    results_dir = os.path.join(project_root, "results")
    config_path = os.path.join(project_root, "config.ini")
    results_csv = os.path.join(results_dir, "results.csv")
    
    # OS detection for the executable name
    exe_name = "bzip2_impl.exe" if platform.system() == "Windows" else "bzip2_impl"
    exe_path = os.path.join(project_root, exe_name)

    # Ensure directories exist
    if not os.path.exists(results_dir):
        os.makedirs(results_dir)

    if not os.path.exists(exe_path):
        print(f"Error: Could not find executable at {exe_path}")
        print("Please compile the project using 'make' first.")
        return

    
    block_size = 500000 

    print("Starting Benchmarks...")
    print("-" * 50)

    with open(results_csv, mode='w', newline='') as csv_file:
        writer = csv.writer(csv_file)
        # Required format from project document
        writer.writerow(["File", "Size", "BlockSize", "CompressionRatio", "Time", "Memory"])

        # Loop through all files in the benchmarks directory
        for filename in os.listdir(benchmarks_dir):
            input_file = os.path.join(benchmarks_dir, filename)
            
            # Skip directories
            if not os.path.isfile(input_file):
                continue

            output_file = os.path.join(results_dir, f"{filename}.bzp")
            original_size = os.path.getsize(input_file)

            print(f"Benchmarking: {filename} ({original_size} bytes)")

            # Prepare the command: ./bzip2_impl c <input> <output> <config>
            cmd = [exe_path, "c", input_file, output_file, config_path]

            start_time = time.time()
            process = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            
            peak_memory_mb = 0.0

            # Track peak memory while the process is running
            if PSUTIL_AVAILABLE:
                try:
                    p_obj = psutil.Process(process.pid)
                    while process.poll() is None:
                        try:
                            # Get Resident Set Size (RSS) in Megabytes
                            mem = p_obj.memory_info().rss / (1024 * 1024)
                            if mem > peak_memory_mb:
                                peak_memory_mb = mem
                        except (psutil.NoSuchProcess, psutil.AccessDenied):
                            break
                        time.sleep(0.01) # Poll every 10ms
                except psutil.NoSuchProcess:
                    pass
            
            # Ensure process is finished
            process.wait()
            end_time = time.time()
            
            elapsed_time = end_time - start_time

            # Calculate Compression Ratio
            if os.path.exists(output_file):
                compressed_size = os.path.getsize(output_file)
                if compressed_size > 0:
                    compression_ratio = original_size / compressed_size
                else:
                    compression_ratio = 0.0
            else:
                print(f"  -> Error: Compression failed for {filename}")
                continue

            # Format memory string
            memory_str = f"{peak_memory_mb:.2f}" if PSUTIL_AVAILABLE else "N/A"

            # Write to CSV
            writer.writerow([
                filename, 
                original_size, 
                block_size, 
                f"{compression_ratio:.3f}", 
                f"{elapsed_time:.3f}", 
                memory_str
            ])
            
            print(f"  -> Ratio: {compression_ratio:.2f}x | Time: {elapsed_time:.2f}s | Mem: {memory_str} MB")

    print("-" * 50)
    print(f"Benchmarking complete! Results saved to {results_csv}")

if __name__ == "__main__":
    run_benchmarks()