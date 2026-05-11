#include "../support/doctest.h"

#include "../../src/workflow/workflow_internal.h"

namespace {

double bytes_to_gib(const uint64_t bytes) {
  return static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
}

} // namespace

TEST_CASE("Compression storage plan uses peak intermediate memory margin") {
  const spring::string_list input_paths = {"data/samples/test_1.fastq"};
  const spring::compression_storage_plan baseline_plan =
      spring::build_compression_storage_plan(input_paths, 1024.0);

  CHECK(baseline_plan.estimated_input_bytes > 0);
  CHECK(baseline_plan.estimated_peak_intermediate_bytes >
        baseline_plan.estimated_input_bytes);
  CHECK(baseline_plan.safety_margin_bytes > 0);
  CHECK(baseline_plan.required_peak_memory_bytes >
        baseline_plan.estimated_peak_intermediate_bytes);

  const uint64_t one_mebibyte = 1024ULL * 1024ULL;
  REQUIRE(baseline_plan.required_peak_memory_bytes > one_mebibyte);

  const spring::compression_storage_plan below_threshold_plan =
      spring::build_compression_storage_plan(
          input_paths, bytes_to_gib(baseline_plan.required_peak_memory_bytes -
                                    one_mebibyte));
  CHECK(below_threshold_plan.selected_path ==
        spring::compression_storage_path::disk_path);

  const spring::compression_storage_plan above_threshold_plan =
      spring::build_compression_storage_plan(
          input_paths, bytes_to_gib(baseline_plan.required_peak_memory_bytes +
                                    one_mebibyte));
  CHECK(above_threshold_plan.selected_path ==
        spring::compression_storage_path::memory_path);
}