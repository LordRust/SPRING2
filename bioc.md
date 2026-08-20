Add spring2 1.3.4 — a cross-platform FASTQ/FASTA compressor for genomic sequencing data.

SPRING2 is a C++20 rewrite and substantial modernization of [SPRING](https://github.com/shubhamchandak94/Spring), which is already packaged in Bioconda. It adds grouped-lane support (R1/R2/R3/I1/I2), assay-aware preprocessing (sc-ATAC, sc-RNA, bisulfite, RNA-seq), archive integrity auditing, preview without decompression, and backward-compatible decompression of legacy SPRING1 `.spring` archives.

**Platforms:** linux-64, linux-aarch64, osx-arm64

**Build notes:**

- Uses CMake + make; requires `cmake >=3.31`
- `LIBRAPIDARCHIVE_WITH_ISAL=OFF` on aarch64 (ISA-L requires NASM, which is x86-only); ISA-L enabled on linux-64 via `nasm` build dependency
- aarch64 also sets `-march=armv8-a+crc` and disables IPO, consistent with upstream CI
- `OpenMP_ROOT=${PREFIX}` directs FindOpenMP to conda's `libgomp`/`llvm-openmp` rather than Homebrew

**run_exports:** `pin_subpackage('spring2', max_pin="x")` — required by Bioconda linter for all compiled packages (semantic versioning, case 1).
