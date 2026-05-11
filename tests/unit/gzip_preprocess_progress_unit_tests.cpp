#include "../support/doctest.h"
#include "input_preparation.h"
#include "io_utils.h"
#include "progress.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

using namespace spring;

namespace {

class scoped_current_path {
public:
  explicit scoped_current_path(const std::filesystem::path &path)
      : original_path_(std::filesystem::current_path()) {
    std::filesystem::current_path(path);
  }

  ~scoped_current_path() { std::filesystem::current_path(original_path_); }

private:
  std::filesystem::path original_path_;
};

class stream_redirect {
public:
  explicit stream_redirect(std::ostream &stream) : stream_(stream) {
    original_buffer_ = stream_.rdbuf(buffer_.rdbuf());
  }

  ~stream_redirect() { stream_.rdbuf(original_buffer_); }

  [[nodiscard]] std::string str() const { return buffer_.str(); }

private:
  std::ostream &stream_;
  std::ostringstream buffer_;
  std::streambuf *original_buffer_ = nullptr;
};

std::filesystem::path make_test_directory() {
  const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto path = std::filesystem::temp_directory_path() /
                    ("spring2-gzip-progress-" + std::to_string(tick));
  std::filesystem::create_directories(path);
  return path;
}

void write_gzip_fastq(const std::filesystem::path &path, int read_count) {
  gzip_ostream output(path.string(), 1);
  REQUIRE(output.is_open());

  std::string fastq;
  fastq.reserve(static_cast<size_t>(read_count) * 32);
  for (int i = 0; i < read_count; ++i) {
    fastq.append("@read_");
    fastq.append(std::to_string(i));
    fastq.push_back('\n');
    fastq.append("ACGTACGT\n");
    fastq.append("+\n");
    fastq.append("IIIIIIII\n");
  }

  output.write(fastq.data(), static_cast<std::streamsize>(fastq.size()));
  output.close();
}

compression_params make_test_params() {
  compression_params cp{};
  cp.encoding.paired_end = false;
  cp.encoding.preserve_order = false;
  cp.encoding.preserve_quality = true;
  cp.encoding.preserve_id = true;
  cp.encoding.long_flag = false;
  cp.encoding.num_thr = 1;
  cp.encoding.compression_level = 1;
  cp.encoding.num_reads_per_block = 1;
  cp.encoding.num_reads_per_block_long = 1;
  cp.encoding.fasta_mode = false;
  cp.encoding.use_crlf = false;

  cp.quality.qvz_flag = false;
  cp.quality.qvz_ratio = 1.0;
  cp.quality.ill_bin_flag = false;
  cp.quality.bin_thr_flag = false;
  cp.quality.bin_thr_thr = 0;
  cp.quality.bin_thr_high = 0;
  cp.quality.bin_thr_low = 0;

  cp.read_info.assay = "dna";
  cp.read_info.assay_confidence = "low";
  cp.read_info.note.clear();

  return cp;
}

std::vector<int> extract_percentages(const std::string &progress_output) {
  std::vector<int> percentages;
  size_t pos = 0;
  while ((pos = progress_output.find('%', pos)) != std::string::npos) {
    size_t start = pos;
    while (start > 0 && ((progress_output[start - 1] >= '0' &&
                          progress_output[start - 1] <= '9') ||
                         progress_output[start - 1] == '-')) {
      --start;
    }

    if (start < pos) {
      percentages.push_back(
          std::stoi(progress_output.substr(start, pos - start)));
    }
    ++pos;
  }
  return percentages;
}

TEST_CASE("Preprocess gzip progress stays within bounds") {
  const std::filesystem::path test_dir = make_test_directory();
  const std::filesystem::path gzip_fastq = test_dir / "reads.fastq.gz";
  write_gzip_fastq(gzip_fastq, 3);

  std::vector<int> percentages;
  {
    const scoped_current_path cwd_guard(test_dir);
    stream_redirect captured_stdout(std::cout);

    compression_params cp = make_test_params();
    ProgressBar progress(true);
    progress.set_stage("Preprocessing", 0.0F, 1.0F);

    preprocess(gzip_fastq.string(), "", cp, false, &progress, nullptr);
    progress.finalize();

    percentages = extract_percentages(captured_stdout.str());
  }

  CHECK_FALSE(percentages.empty());
  for (const int percent : percentages) {
    CHECK(percent >= 0);
    CHECK(percent <= 100);
  }

  std::filesystem::remove_all(test_dir);
}

} // namespace