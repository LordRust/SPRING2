// Chooses concrete template instantiations for reorder and encoder bitset sizes
// based on the dataset's read-length requirements.

#include "compression_dispatch_internal.h"
#include "params.h"
#include <stdexcept>

namespace spring {

size_t rounded_bitset_size(const size_t encoded_bits_per_read) {
  return (encoded_bits_per_read - 1) / dispatch_detail::kBitsetStep *
             dispatch_detail::kBitsetStep +
         dispatch_detail::kBitsetStep;
}

size_t dispatch_index(const size_t requested_bitset_size,
                      const size_t max_supported_bitset_size) {
  // Template entry points are instantiated in 64-bit increments only.
  if (requested_bitset_size < dispatch_detail::kBitsetStep ||
      requested_bitset_size > max_supported_bitset_size ||
      (requested_bitset_size % dispatch_detail::kBitsetStep) != 0) {
    throw std::runtime_error("Wrong bitset size.");
  }

  return requested_bitset_size / dispatch_detail::kBitsetStep - 1;
}

reorder_encoder_artifact call_reorder(reorder_input_artifact artifact,
                                      compression_params &params) {
  const size_t reorder_bitset_size = rounded_bitset_size(
      2 * static_cast<size_t>(params.read_info.max_readlen));
  return dispatch_detail::reorder_dispatchers[dispatch_index(
      reorder_bitset_size, dispatch_detail::kMaxReorderBitsetSize)](
      std::move(artifact), params);
}

reordered_stream_artifact call_encoder(const reorder_encoder_artifact &artifact,
                                       compression_params &params) {
  const size_t encoder_bitset_size = rounded_bitset_size(
      3 * static_cast<size_t>(params.read_info.max_readlen));
  return dispatch_detail::encoder_dispatchers[dispatch_index(
      encoder_bitset_size, dispatch_detail::kMaxEncoderBitsetSize)](artifact,
                                                                    params);
}

} // namespace spring