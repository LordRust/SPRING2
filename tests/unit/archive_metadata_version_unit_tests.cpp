#include "../support/doctest.h"

#include "../../src/common/params.h"
#include "../../src/workflow/workflow_internal.h"

#include <sstream>

namespace {

constexpr char kExpectedLegacyArchiveVersion[] = "1.0.0-rc.1";

spring::compression_params make_sample_params() {
  spring::compression_params cp{};
  cp.encoding.paired_end = true;
  cp.encoding.preserve_order = true;
  cp.encoding.preserve_quality = true;
  cp.encoding.preserve_id = true;
  cp.encoding.long_flag = false;
  cp.encoding.num_thr = 4;
  cp.encoding.compression_level = 6;
  cp.encoding.num_reads_per_block = 123;
  cp.encoding.num_reads_per_block_long = 45;
  cp.encoding.fasta_mode = false;
  cp.encoding.use_crlf = false;
  cp.encoding.use_crlf_by_stream[0] = false;
  cp.encoding.use_crlf_by_stream[1] = true;
  cp.read_info.num_reads = 100;
  cp.read_info.num_reads_clean[0] = 50;
  cp.read_info.num_reads_clean[1] = 50;
  cp.read_info.max_readlen = 151;
  cp.read_info.paired_id_code = 7;
  cp.read_info.paired_id_match = true;
  cp.read_info.input_filename_1 = "reads_1.fastq.gz";
  cp.read_info.input_filename_2 = "reads_2.fastq.gz";
  cp.read_info.note = "note";
  cp.read_info.assay = "rna";
  cp.read_info.assay_confidence = "high";
  cp.read_info.compressor_version = "1.0.0-rc.2";
  cp.read_info.sequence_crc[0] = 11;
  cp.read_info.sequence_crc[1] = 22;
  cp.read_info.quality_crc[0] = 33;
  cp.read_info.quality_crc[1] = 44;
  cp.read_info.id_crc[0] = 55;
  cp.read_info.id_crc[1] = 66;
  cp.read_info.quality_header_has_id_by_stream[0] = false;
  cp.read_info.quality_header_has_id_by_stream[1] = true;
  cp.gzip.streams[0].was_gzipped = true;
  cp.gzip.streams[1].was_gzipped = true;
  cp.gzip.streams[0].name = "reads_1.fastq";
  cp.gzip.streams[1].name = "reads_2.fastq";
  return cp;
}

std::string
serialize_current_archive_metadata(const spring::compression_params &cp) {
  std::ostringstream output(std::ios::binary);
  spring::write_compression_params(output, cp);
  return output.str();
}

std::string
serialize_legacy_archive_metadata(const spring::compression_params &cp) {
  std::string current = serialize_current_archive_metadata(cp);
  return current.substr(sizeof(uint32_t) * 2);
}

} // namespace

TEST_CASE("Current archive metadata stores format header") {
  const spring::compression_params cp = make_sample_params();
  const std::string bytes = serialize_current_archive_metadata(cp);
  std::istringstream input(bytes, std::ios::binary);

  spring::compression_params parsed{};
  spring::read_compression_params(input, parsed);

  CHECK(parsed.read_info.archive_format_version ==
        spring::CURRENT_ARCHIVE_FORMAT_VERSION);
  CHECK(parsed.read_info.compressor_version == cp.read_info.compressor_version);

  const spring::archive_decompression_plan plan =
      spring::build_archive_decompression_plan(parsed);
  CHECK(plan.is_legacy_unversioned == false);
  CHECK(plan.archive_version.valid);
  CHECK(plan.archive_version.major == 1);
  CHECK(plan.archive_version.minor == 0);
}

TEST_CASE("Legacy archive metadata remains readable") {
  const spring::compression_params cp = make_sample_params();
  const std::string bytes = serialize_legacy_archive_metadata(cp);
  std::istringstream input(bytes, std::ios::binary);

  spring::compression_params parsed{};
  spring::read_compression_params(input, parsed);

  CHECK(parsed.read_info.archive_format_version ==
        spring::LEGACY_ARCHIVE_FORMAT_VERSION);
  CHECK(parsed.read_info.compressor_version == kExpectedLegacyArchiveVersion);
  CHECK(parsed.read_info.input_filename_1 == cp.read_info.input_filename_1);
  CHECK(parsed.read_info.input_filename_2 == cp.read_info.input_filename_2);

  const spring::archive_decompression_plan plan =
      spring::build_archive_decompression_plan(parsed);
  CHECK(plan.is_legacy_unversioned);
  CHECK(plan.compressor_version == kExpectedLegacyArchiveVersion);
}

TEST_CASE("Archive decompression plan preserves exact current version") {
  spring::compression_params cp = make_sample_params();
  cp.read_info.compressor_version = "1.0.7";

  const spring::archive_decompression_plan plan =
      spring::build_archive_decompression_plan(cp);
  CHECK(plan.archive_version.valid);
  CHECK(plan.archive_version.major == 1);
  CHECK(plan.archive_version.minor == 0);
  CHECK(plan.archive_version.patch == 7);
  CHECK(spring::archive_decompression_route_name(plan) == "1.0.7");
}

TEST_CASE("Unsupported archive compressor versions fail explicitly") {
  spring::compression_params cp = make_sample_params();
  cp.read_info.compressor_version = "1.1.0";

  CHECK_THROWS_AS(spring::build_archive_decompression_plan(cp),
                  std::runtime_error);
}