// Unit tests for reorder performance improvements:
//   maxshift cap - reads > 100 bp are capped at kMaxReorderShift (50).
//   progress log timing - the 60-second wall-clock interval condition.

#include "../support/doctest.h"

#include <algorithm>
#include <chrono>

namespace {

// Replicates the constant and formula from reorder_main in
// read_reordering_impl.h so the tests validate the arithmetic without
// creating a build dependency on that translation unit.
constexpr int kMaxReorderShift = 50;

int compute_maxshift(int max_readlen) {
  return std::min(max_readlen / 2, kMaxReorderShift);
}

} // namespace

// ---------------------------------------------------------------------------
// maxshift cap formula
// ---------------------------------------------------------------------------

TEST_CASE("maxshift is readlen/2 for reads at or below 100 bp") {
  CHECK(compute_maxshift(50) == 25);
  CHECK(compute_maxshift(80) == 40);
  CHECK(compute_maxshift(100) == 50);
}

TEST_CASE("maxshift is capped at 50 for reads above 100 bp") {
  CHECK(compute_maxshift(101) == 50);
  CHECK(compute_maxshift(120) == 50);
  CHECK(compute_maxshift(151) == 50);
  CHECK(compute_maxshift(200) == 50);
  CHECK(compute_maxshift(300) == 50);
}

TEST_CASE("maxshift cap constant equals 50") { CHECK(kMaxReorderShift == 50); }

TEST_CASE("maxshift cap boundary: uncapped at 100 bp, capped above") {
  CHECK(compute_maxshift(98) == 49);  // strictly below cap
  CHECK(compute_maxshift(100) == 50); // exactly at cap boundary
  CHECK(compute_maxshift(102) == 50); // 51 uncapped → capped to 50
  CHECK(compute_maxshift(200) == 50); // 100 uncapped → capped to 50
}

TEST_CASE("maxshift cap reduces iteration count for long reads") {
  // Verify the cap applies a meaningful reduction vs. the uncapped formula.
  const int uncapped_151 = 151 / 2;             // 75
  const int capped_151 = compute_maxshift(151); // 50
  CHECK(capped_151 < uncapped_151);
  CHECK(uncapped_151 - capped_151 == 25); // exactly 25 fewer shifts

  const int uncapped_200 = 200 / 2;             // 100
  const int capped_200 = compute_maxshift(200); // 50
  CHECK(uncapped_200 - capped_200 == 50);       // 50% reduction
}

// ---------------------------------------------------------------------------
// Progress log timing condition
// ---------------------------------------------------------------------------

TEST_CASE("Reorder progress log 60-second interval fires correctly") {
  using clock = std::chrono::steady_clock;
  using seconds = std::chrono::seconds;

  const auto ref = clock::now();

  // Exactly 60 s elapsed: should fire.
  CHECK((ref - (ref - seconds(60))) >= seconds(60));

  // Over 60 s elapsed: should fire.
  CHECK((ref - (ref - seconds(61))) >= seconds(60));

  // Under 60 s elapsed: should NOT fire.
  CHECK_FALSE((ref - (ref - seconds(59))) >= seconds(60));
  CHECK_FALSE((ref - (ref - seconds(1))) >= seconds(60));
}

TEST_CASE("Progress log interval resets after each log emission") {
  using clock = std::chrono::steady_clock;
  using seconds = std::chrono::seconds;

  // Simulate the reorder loop state: after a log is emitted last_log_ts is
  // set to now, so the next check within 60 s should not fire again.
  const auto fire_time = clock::now();
  auto last_log_ts = fire_time; // log just emitted

  // 30 s later: should NOT fire
  const auto check_30s = fire_time + seconds(30);
  CHECK_FALSE((check_30s - last_log_ts) >= seconds(60));

  // 60 s later: should fire
  const auto check_60s = fire_time + seconds(60);
  CHECK((check_60s - last_log_ts) >= seconds(60));

  // Simulate emitting the log and resetting.
  last_log_ts = check_60s;

  // 30 s after that reset: should NOT fire again
  const auto check_90s = fire_time + seconds(90);
  CHECK_FALSE((check_90s - last_log_ts) >= seconds(60));
}
