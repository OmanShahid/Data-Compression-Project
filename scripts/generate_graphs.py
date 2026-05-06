#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt


def main():
    df = pd.read_csv("results/results.csv")

    plt.figure(figsize=(10, 5))
    plt.bar(df["File"], df["CompressionRatio"])
    plt.xticks(rotation=45, ha="right")
    plt.ylabel("Compression Ratio")
    plt.tight_layout()
    plt.savefig("results/compression_ratio.png")

    plt.figure(figsize=(10, 5))
    plt.bar(df["File"], df["Time"])
    plt.xticks(rotation=45, ha="right")
    plt.ylabel("Time (s)")
    plt.tight_layout()
    plt.savefig("results/time.png")


if __name__ == "__main__":
    main()
