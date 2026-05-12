// Internal declarations shared across the split compression-dispatch
// translation units.

#ifndef SPRING_COMPRESSION_DISPATCH_INTERNAL_H_
#define SPRING_COMPRESSION_DISPATCH_INTERNAL_H_

#include "compression_dispatch.h"
#include <array>
#include <cstddef>

namespace spring::dispatch_detail {

using reorder_template_main_fn = reorder_encoder_artifact (*)(
    const reorder_input_artifact &, const compression_params &);
using encoder_template_main_fn = reordered_stream_artifact (*)(
    const reorder_encoder_artifact &, compression_params &);

constexpr size_t kBitsetStep = 64;
constexpr size_t kMaxReorderBitsetSize = 1024;
constexpr size_t kMaxEncoderBitsetSize = 1536;

extern const std::array<reorder_template_main_fn, 16> reorder_dispatchers;
extern const std::array<encoder_template_main_fn, 24> encoder_dispatchers;

} // namespace spring::dispatch_detail

#endif // SPRING_COMPRESSION_DISPATCH_INTERNAL_H_