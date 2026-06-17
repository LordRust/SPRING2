#pragma once

#include <fstream>
#include <string>

#include "../support/doctest.h"

#ifndef SPRING2_EXECUTABLE
#define SPRING2_EXECUTABLE "spring2"
#endif

namespace integration_test_support {

std::string read_file_binary(const std::string &path);
std::string read_gzip_file_binary(const std::string &path);

void write_fastq_record(std::ofstream &output, const std::string &id,
                        const std::string &sequence, const std::string &quality,
                        bool quality_header_has_id, bool use_crlf);
void create_custom_fastq(const std::string &path, int num_records,
                         bool quality_header_has_id, bool use_crlf,
                         int read_len = 80);
void create_late_long_fastq(const std::string &path, int short_records,
                            int total_records, int short_len, int long_len);
void create_delayed_crlf_fastq(const std::string &path, int total_records,
                               int lf_records, int read_len = 80);
void create_custom_paired_fastqs(const std::string &r1_path,
                                 const std::string &r2_path, int num_records,
                                 bool r1_quality_header_has_id,
                                 bool r2_quality_header_has_id,
                                 bool r1_use_crlf, bool r2_use_crlf);
void create_gzip_copy(const std::string &input_path,
                      const std::string &output_path, int level);
void create_dummy_fastq(const std::string &path, int num_records);
void create_atac_like_fastq(const std::string &path, int num_records);
void create_sparse_atac_like_fastq(const std::string &path, int num_records);
void create_grouped_sc_rna_like_fastqs(const std::string &r1_path,
                                       const std::string &r2_path,
                                       const std::string &i1_path,
                                       const std::string &i2_path,
                                       int num_records);
std::string read_manifest_value(const std::string &manifest_path,
                                const std::string &key);
void create_tar_with_entry(const std::string &archive_path,
                           const std::string &entry_path,
                           const std::string &contents);
void replace_exact_in_file(const std::string &path, const std::string &from,
                           const std::string &to);

struct ScopedCurrentPath {
  explicit ScopedCurrentPath(const std::string &path);
  ~ScopedCurrentPath();

  std::string original;
};

// Run a spring2 subprocess, suppressing its stdout/stderr unless the command
// fails.  On failure the captured output is printed via INFO() and the test
// is aborted with REQUIRE.  Do NOT use this for commands that already redirect
// their own output (> file 2>&1) or for non-spring2 system commands (e.g. tar).
void run_spring(const std::string &cmd);

// Compare binary content without dumping file bytes to the test log on
// failure.  Use this instead of CHECK(a == b) when a and b are large strings
// (e.g. file contents), so doctest's expression decomposer does not print
// megabytes of data when the assertion fails.
inline void check_bytes_equal(const std::string &actual,
                              const std::string &expected,
                              const char *description = "") {
  const bool match = (actual == expected);
  CHECK_MESSAGE(match, "Content mismatch ["
                           << description << "]: actual=" << actual.size()
                           << " bytes, expected=" << expected.size()
                           << " bytes");
}

} // namespace integration_test_support