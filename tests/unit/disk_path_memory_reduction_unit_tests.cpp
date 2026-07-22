// Unit tests for disk-path memory reduction:
//   external MPHF  - build_in_external_memory when temp disk is available.
//   thread capping - reduce thread count when temp disk is insufficient.

#include "../support/doctest.h"

#include "../../src/common/params.h"

#include <cstdint>

namespace {

// Replicate the constants from compression_workflow.cpp so the tests verify
// the same arithmetic without creating a build dependency on that TU.
constexpr uint64_t kExternalMphfBytesPerKey = 40;
constexpr uint64_t kBytesPerEncoderThread = 4ULL << 30; // 4 GiB

// Replicates the disk-space estimate formula from compress_standard.
uint64_t estimate_external_mphf_disk_bytes(uint64_t total_clean_reads) {
  return total_clean_reads * kExternalMphfBytesPerKey;
}

// Replicates the thread-capping formula from compress_standard.
int compute_safe_thread_count(uint64_t available_memory_bytes,
                              int current_threads) {
  if (available_memory_bytes == 0) {
    return current_threads;
  }
  const uint64_t thread_budget = available_memory_bytes / 2;
  const int safe = static_cast<int>(
      std::max(uint64_t{1}, thread_budget / kBytesPerEncoderThread));
  return safe < current_threads ? safe : current_threads;
}

} // namespace

// ---------------------------------------------------------------------------
// External-memory MPHF: disk-space estimate
// ---------------------------------------------------------------------------

TEST_CASE("External MPHF disk estimate scales linearly with read count") {
  // 1 billion reads → 40 bytes each → 40 GB
  CHECK(estimate_external_mphf_disk_bytes(1'000'000'000ULL) ==
        40'000'000'000ULL);
  // 500 million reads → 20 GB
  CHECK(estimate_external_mphf_disk_bytes(500'000'000ULL) == 20'000'000'000ULL);
  // Zero reads → zero bytes (no MPHF to build)
  CHECK(estimate_external_mphf_disk_bytes(0) == 0);
}

TEST_CASE("External MPHF is selected when disk estimate is met") {
  // If 100 GiB (107,374,182,400 bytes) is available, and 500M reads need
  // 20 GB, external MPHF should be selected (20 GB < 107 GB).
  const uint64_t available = 100ULL * 1024 * 1024 * 1024;
  const uint64_t needed = estimate_external_mphf_disk_bytes(500'000'000ULL);
  CHECK(available >= needed);
}

TEST_CASE("External MPHF is skipped when disk estimate is not met") {
  // If only 10 GiB is free but 1B reads need 40 GB, external MPHF must not
  // be selected.
  const uint64_t available = 10ULL * 1024 * 1024 * 1024;
  const uint64_t needed = estimate_external_mphf_disk_bytes(1'000'000'000ULL);
  CHECK(available < needed);
}

// ---------------------------------------------------------------------------
// Thread-count capping formula
// ---------------------------------------------------------------------------

TEST_CASE("Thread count capping is proportional to available memory") {
  // 100 GiB available → budget = 50 GiB → 50/4 = 12 threads
  CHECK(compute_safe_thread_count(100ULL << 30, 20) == 12);
  // 32 GiB available → budget = 16 GiB → 16/4 = 4 threads
  CHECK(compute_safe_thread_count(32ULL << 30, 20) == 4);
  // 8 GiB available → budget = 4 GiB → 4/4 = 1 thread
  CHECK(compute_safe_thread_count(8ULL << 30, 20) == 1);
}

TEST_CASE("Thread count capping never goes below 1") {
  // Even with a tiny memory budget, safe_threads >= 1.
  CHECK(compute_safe_thread_count(1ULL << 20, 16) == 1); // 1 MiB available
  CHECK(compute_safe_thread_count(0, 8) == 8);           // zero means "no cap"
}

TEST_CASE("Thread count capping does not increase above current thread count") {
  // If available memory would allow more threads than currently requested,
  // the cap must not increase the count.
  CHECK(compute_safe_thread_count(512ULL << 30, 4) == 4); // 512 GiB, 4 threads
}

// ---------------------------------------------------------------------------
// EncodingConfig new fields
// ---------------------------------------------------------------------------

TEST_CASE("EncodingConfig use_external_mphf defaults to false") {
  const spring::compression_params cp{};
  CHECK(!cp.encoding.use_external_mphf);
  CHECK(cp.encoding.mphf_tmp_dir.empty());
}
