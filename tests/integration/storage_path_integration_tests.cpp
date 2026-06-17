#include "integration_test_support.h"

#include <filesystem>

namespace fs = std::filesystem;
using namespace integration_test_support;

namespace {

std::string sample_asset_path(const std::string &name) {
#ifdef INTEGRATION_TEST_ASSET_DIR
  return (fs::path(INTEGRATION_TEST_ASSET_DIR) / name).generic_string();
#else
  return (fs::path("..") / ".." / "data" / "samples" / name).generic_string();
#endif
}

TEST_CASE("Single-end sample matches for memory_path and disk_path") {
  const std::string test_dir = "storage_path_single_test_tmp";
  const std::string input_fastq = sample_asset_path("test_1.fastq");
  const std::string memory_archive = test_dir + "/memory.sp";
  const std::string disk_archive = test_dir + "/disk.sp";
  const std::string memory_output = test_dir + "/memory.fastq";
  const std::string disk_output = test_dir + "/disk.fastq";
  const std::string memory_log = test_dir + "/memory.log";
  const std::string disk_log = test_dir + "/disk.log";

  REQUIRE(fs::exists(input_fastq));
  fs::create_directories(test_dir);

  const std::string memory_compress_cmd =
      std::string(SPRING2_EXECUTABLE) + " -c --R1 " + input_fastq + " -o " +
      memory_archive + " -t 1 -m 1024 > " + memory_log + " 2>&1";
  const std::string disk_compress_cmd =
      std::string(SPRING2_EXECUTABLE) + " -c --R1 " + input_fastq + " -o " +
      disk_archive + " -t 1 -m 0.00001 > " + disk_log + " 2>&1";
  const std::string memory_decompress_cmd = std::string(SPRING2_EXECUTABLE) +
                                            " -d -i " + memory_archive +
                                            " -o " + memory_output + " -t 1";
  const std::string disk_decompress_cmd = std::string(SPRING2_EXECUTABLE) +
                                          " -d -i " + disk_archive + " -o " +
                                          disk_output + " -t 1";

  REQUIRE(std::system(memory_compress_cmd.c_str()) == 0);
  REQUIRE(std::system(disk_compress_cmd.c_str()) == 0);
  REQUIRE(std::system(memory_decompress_cmd.c_str()) == 0);
  REQUIRE(std::system(disk_decompress_cmd.c_str()) == 0);

  const std::string original_bytes = read_file_binary(input_fastq);
  check_bytes_equal(read_file_binary(memory_output), original_bytes,
                    "memory-path output");
  check_bytes_equal(read_file_binary(disk_output), original_bytes,
                    "disk-path output");
  check_bytes_equal(read_file_binary(memory_output),
                    read_file_binary(disk_output), "memory vs disk output");

  const std::string memory_log_output = read_file_binary(memory_log);
  const std::string disk_log_output = read_file_binary(disk_log);
  CHECK(memory_log_output.find(
            "Disk-backed compression path selected based on estimated peak "
            "working memory and available memory.") == std::string::npos);
  CHECK(disk_log_output.find(
            "Disk-backed compression path selected based on estimated peak "
            "working memory and available memory.") != std::string::npos);
  CHECK_FALSE(fs::exists(disk_archive + ".work-tmp"));

  fs::remove_all(test_dir);
}

TEST_CASE("Paired sample matches for memory_path and disk_path") {
  const std::string test_dir = "storage_path_paired_test_tmp";
  const std::string input_r1 = sample_asset_path("test_1.fastq");
  const std::string input_r2 = sample_asset_path("test_2.fastq");
  const std::string memory_archive = test_dir + "/memory_paired.sp";
  const std::string disk_archive = test_dir + "/disk_paired.sp";
  const std::string memory_r1 = test_dir + "/memory_R1.fastq";
  const std::string memory_r2 = test_dir + "/memory_R2.fastq";
  const std::string disk_r1 = test_dir + "/disk_R1.fastq";
  const std::string disk_r2 = test_dir + "/disk_R2.fastq";
  const std::string memory_log = test_dir + "/memory_paired.log";
  const std::string disk_log = test_dir + "/disk_paired.log";

  REQUIRE(fs::exists(input_r1));
  REQUIRE(fs::exists(input_r2));
  fs::create_directories(test_dir);

  const std::string memory_compress_cmd =
      std::string(SPRING2_EXECUTABLE) + " -c --R1 " + input_r1 + " --R2 " +
      input_r2 + " -o " + memory_archive + " -t 1 -m 1024 > " + memory_log +
      " 2>&1";
  const std::string disk_compress_cmd =
      std::string(SPRING2_EXECUTABLE) + " -c --R1 " + input_r1 + " --R2 " +
      input_r2 + " -o " + disk_archive + " -t 1 -m 0.00001 > " + disk_log +
      " 2>&1";
  const std::string memory_decompress_cmd =
      std::string(SPRING2_EXECUTABLE) + " -d -i " + memory_archive + " -o " +
      memory_r1 + " " + memory_r2 + " -t 1";
  const std::string disk_decompress_cmd = std::string(SPRING2_EXECUTABLE) +
                                          " -d -i " + disk_archive + " -o " +
                                          disk_r1 + " " + disk_r2 + " -t 1";

  REQUIRE(std::system(memory_compress_cmd.c_str()) == 0);
  REQUIRE(std::system(disk_compress_cmd.c_str()) == 0);
  REQUIRE(std::system(memory_decompress_cmd.c_str()) == 0);
  REQUIRE(std::system(disk_decompress_cmd.c_str()) == 0);

  const std::string original_r1 = read_file_binary(input_r1);
  const std::string original_r2 = read_file_binary(input_r2);
  check_bytes_equal(read_file_binary(memory_r1), original_r1, "memory-path R1");
  check_bytes_equal(read_file_binary(memory_r2), original_r2, "memory-path R2");
  check_bytes_equal(read_file_binary(disk_r1), original_r1, "disk-path R1");
  check_bytes_equal(read_file_binary(disk_r2), original_r2, "disk-path R2");

  const std::string memory_log_output = read_file_binary(memory_log);
  const std::string disk_log_output = read_file_binary(disk_log);
  CHECK(memory_log_output.find(
            "Disk-backed compression path selected based on estimated peak "
            "working memory and available memory.") == std::string::npos);
  CHECK(disk_log_output.find(
            "Disk-backed compression path selected based on estimated peak "
            "working memory and available memory.") != std::string::npos);
  CHECK_FALSE(fs::exists(disk_archive + ".work-tmp"));

  fs::remove_all(test_dir);
}

} // namespace