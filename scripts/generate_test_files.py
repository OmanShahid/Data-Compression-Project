import os

def generate_files():
    print("Generating test files...")

    # 1. Create a 15MB repetitive text file (Highly Compressible)
    text_filename = "large_repetitive_text.txt"
    chunk = "The quick brown fox jumps over the lazy dog. " * 100 + "\n"
    target_size = 15 * 1024 * 1024  # 15 MB

    with open(text_filename, "w") as f:
        written = 0
        while written < target_size:
            f.write(chunk)
            written += len(chunk)
    print(f"Created: {text_filename} (approx 15MB)")

    # 2. Create a 10MB binary file with repeating patterns (Moderately Compressible)
    binary_filename = "large_pattern_binary.bin"
    target_size_bin = 10 * 1024 * 1024 # 10 MB
    
    with open(binary_filename, "wb") as f:
        # Create a repeating 256-byte pattern
        pattern = bytearray([i % 256 for i in range(256)]) * 4096
        written = 0
        while written < target_size_bin:
            f.write(pattern)
            written += len(pattern)
    print(f"Created: {binary_filename} (approx 10MB)")
    print("Done! You can now run your benchmarking script.")

if __name__ == "__main__":
    generate_files()