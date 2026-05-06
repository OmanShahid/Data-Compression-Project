# BZip2 Course Project — Implementation Plan

**Source:** Data Compression Course Project (Implementation of BZip2 Compression Algorithm), Dr. Faisal Aslam, April 14, 2026.

**Team size:** 3 students · **Total duration:** 3 weeks

This document turns the project specification into a sequenced plan with deliverables, ownership hints, and verification steps.

---

## 1. Goals and Constraints

| Goal | Detail |
|------|--------|
| **Pipeline** | Block division → RLE-1 → BWT → MTF → RLE-2 → Canonical Huffman |
| **Grading** | Stage 1: 50% · Stage 2: 25% · Stage 3: 25% · Extras: +10% (features) · +10% (beat reference bzip2, max 2 groups) |
| **Rules** | Implement all logic yourself; only provided structs/prototypes are given; no copied implementation code |

**Non-negotiables:** encode *and* decode must work for each stage; `config.ini` drives behavior; cross-platform `Makefile` (Linux + Windows / cross-compile target).

---

## 2. Repository and Scaffold (Week 0 — before Stage 1)

Lay out the required tree early so integration and CI-style builds stay simple:

```
project-bzip2/
├── src/          (main.c, rle.c, bwt.c, mtf.c, huffman.c, config.c)
├── include/      (*.h matching modules)
├── benchmarks/   (Canterbury, Calgary, Silesia, large text/binary samples)
├── results/      (results.csv, plots)
├── Makefile
├── config.ini
├── README.md
└── report.pdf    (final)
```

**Tasks**

1. Create headers with the **exact** typedefs and prototypes from the spec (`Block`, `BlockManager`, `Rotation`, `HuffmanCode`, `HuffmanNode`, and all listed functions).
2. Stub `main.c` to: load config → read file → per-block pipeline → write output; wire **no-ops** or pass-throughs until each stage exists.
3. Implement `config.c` to parse `config.ini` sections `[General]`, `[Performance]`, `[Paths]` (block size 100KB–900KB, flags, `bwt_type` = `matrix` or `suffix_array`).

**Exit criteria:** Project compiles; config loads; block read/write round-trips without compression.

---

## 3. Stage 1 (1.5 weeks) — 50% of grade

### 3.1 Block division and file handling

| Item | Action |
|------|--------|
| **API** | `divide_into_blocks`, `reassemble_blocks`, `free_block_manager` |
| **Behavior** | Stream or map large files; split into fixed-size blocks (last block may be shorter); preserve total byte count on reassembly |

**Verification:** Compress nothing — split a 50MB+ file and `memcmp` with original after reassembly.

### 3.2 RLE-1

| Item | Action |
|------|--------|
| **Spec example** | `ABBBCCCCD` → `A3B4C1D` (count + byte per run) |
| **API** | `rle1_encode`, `rle1_decode` |

**Edge cases:** Empty input; single byte; max run length and output buffer sizing (either document max expansion or allocate conservatively).

**Verification:** Property tests: random buffers → encode → decode → identity.

### 3.3 BWT (matrix method first)

| Item | Action |
|------|--------|
| **Method** | All cyclic rotations → sort lexicographically → last column + **primary index** (row index of original string in sorted matrix) |
| **API** | `compare_rotations`, `bwt_encode`, `bwt_decode` |

**Implementation notes:** Use `unsigned char` throughout for binary safety; `compare_rotations` for `qsort`. Inverse BWT is the standard LF-mapping / iteration — implement from a clear reference (poly paper or standard lecture notes), not by guessing.

**Verification:** Random short strings (including repeated bytes, 0x00) → BWT → inverse → match.

### 3.4 Stage 1 integration

- Wire: **optional** `rle1_enabled` from config (if false, pass raw block through to BWT per spec intent).
- End-to-end for Stage 1: `divide → RLE-1? → BWT → inverse BWT → RLE-1 decode? → reassemble` must match input.

**Stage 1 demo checklist:** RLE-1 correctness; BWT forward/inverse; blocks + config parsing; team can explain memory complexity of matrix BWT (O(n²) sort with O(n) rotations — acceptable for coursework block sizes).

---

## 4. Stage 2 (6 days) — 25% of grade

### 4.1 Move-to-Front (MTF)

| Item | Action |
|------|--------|
| **API** | `mtf_encode`, `mtf_decode` |
| **Model** | 256-symbol list (typically 0…255 init); on encode output index of symbol, move symbol to front; decode inverts |

**Verification:** Round-trip after BWT: `BWT → MTF → inv MTF → inv BWT` on known strings.

### 4.2 RLE-2

| Item | Action |
|------|--------|
| **Role** | Specialized RLE for MTF output (many small values and zeros — follow bzip2-style run encoding as taught or as specified in lectures; align with your `rle2_encode`/`rle2_decode` contract) |
| **API** | `rle2_encode`, `rle2_decode` |

**Verification:** MTF output → RLE-2 → decode → exact MTF recovery.

### 4.3 Full pipeline to end of Stage 2

**Required chain (encode):** RLE-1 → BWT → MTF → RLE-2  
**Decode:** reverse order with flags from config.

**Exit criteria:** Deterministic round-trip on benchmark snippets; no memory leaks on multi-block files (valgrind/Dr. Memory on Linux, or Windows equivalents).

---

## 5. Stage 3 (1 week) — 25% of grade

### 5.1 Canonical Huffman coding

| Item | Action |
|------|--------|
| **Structures** | `HuffmanCode` (code + length), `HuffmanNode` (symbol, freq, children) |
| **API** | `build_huffman_tree`, `generate_canonical_codes`, `huffman_encode`, `huffman_decode`, `write_header`, `encode_data` |

**Implementation sequence**

1. Count byte frequencies on **RLE-2 output** (or whatever is the last stage before Huffman per your main wiring).
2. Build tree (min-heap / two-queue method).
3. Derive code **lengths**; assign **canonical** codes (sorted by length/symbol rules).
4. `write_header`: persist code lengths (and any required symbol set metadata) so decode can rebuild the same codebook.
5. Bit packing: `encode_data` writes bitstream; decoder reads header then symbols until block length is known (you must define **block boundary** in your file format: e.g. header + bit length + payload per block).

**Verification:** Single-block compress/decompress; then multi-block file with correct block framing.

---

## 6. Build system and platforms

| Target | Purpose |
|--------|---------|
| `all` | Default build |
| `clean` | Remove objects and binary |
| `windows` | Cross-compile for Windows (MinGW-w64 or documented toolchain) |

Use `CC`, `CFLAGS` (`-Wall -O2` baseline), and detect OS in the Makefile. Document any extra libs in README.

---

## 7. Performance evaluation and reporting

### 7.1 Datasets (required categories)

- Canterbury Corpus  
- Calgary Corpus  
- Silesia Corpus  
- Large text: 10–100 MB  
- Binary: 10–100 MB  

### 7.2 Metrics

- **Compression ratio** vs reference **bzip2**  
- **Speed (MB/s)** vs reference **bzip2**  
- Weighted score: \( \text{Score} = w_1 \cdot \frac{C_{\text{ref}}}{C} + w_2 \cdot \frac{S}{S_{\text{ref}}} \) (weights from instructor)

### 7.3 Automation

- Script produces `results.csv` with columns: **File, Size, BlockSize, CompressionRatio, Time, Memory**  
- Python (or similar) reads CSV and generates **performance graphs** (matplotlib/pandas suggested in spec)

---

## 8. Optional extras (+10% features)

| Feature | Notes |
|---------|--------|
| Enhanced RLE | Threshold-based runs, adaptive RLE, or RLE + small entropy stage |
| Suffix-array BWT | `build_suffix_array` + BWT from SA for speed on large blocks |
| Alternative entropy | ANS or range coding instead of/in addition to Huffman |

Integrate via `config.ini` (`bwt_type`, future flags) so baseline submission stays gradable.

---

## 9. Suggested 3-person work split

| Area | Owner focus |
|------|-------------|
| **A** | `config.c`, `Makefile`, file I/O, `BlockManager`, benchmarking scripts + CSV + plots |
| **B** | `rle.c` (RLE-1 & RLE-2), `mtf.c`, pipeline glue in `main.c` |
| **C** | `bwt.c` (matrix then optional SA), `huffman.c`, compressed file format + headers |

Rotate code review: each person reviews another’s inverse transforms (BWT, MTF, Huffman decode).

---

## 10. Milestone timeline (example)

| When | Milestone |
|------|-----------|
| **End week 1** | Config + blocks + RLE-1 + BWT + Stage 1 tests |
| **Mid week 2** | MTF + RLE-2 + full Stage 2 pipeline |
| **End week 2** | Huffman + on-disk format + first full-file round-trip |
| **Week 3** | Benchmarks, graphs, README, report.pdf, optional extras, polish |

---

## 11. Risks and mitigations

| Risk | Mitigation |
|------|------------|
| Inverse BWT bugs | Small test harness with fixed examples from literature; compare with known BWT column |
| Bit-level Huffman off-by-one | Unit test one symbol repeated; test odd bit lengths; assert byte alignment rules in format doc |
| Buffer overflows | Centralize max output size helpers for RLE stages; fuzz short inputs |
| Slow matrix BWT | Keep block size configurable; plan SA path for extra marks + speed |
| Reference comparison unfair | Pin bzip2 version and machine spec in README |

---

## 12. Submission checklist

- [ ] GitHub structure matches spec  
- [ ] README: features, per-stage implementation notes, Linux + Windows build, usage, results + graphs, **team names and contributions**  
- [ ] `config.ini` + working Makefile targets  
- [ ] `results.csv` + graph generation script  
- [ ] `report.pdf`  
- [ ] No plagiarism; discuss algorithms, write your own code  

---

## References (from project document)

1. Seward, J. (1996). bzip2 / libbzip2.  
2. Burrows & Wheeler (1994). Block-sorting compression.  
3. Huffman (1952). Minimum-redundancy codes.  
4. Manber & Myers (1993). Suffix arrays.
