// Explicit encoder-template instantiations kept in a dedicated translation
// unit so they do not force reorder code into the same compile.

#include "encoder_impl.h"
#include "params.h"
#include "workflow/compression_dispatch_internal.h"

namespace spring::dispatch_detail {

template <size_t bitset_size>
reordered_stream_artifact
call_encoder_main(const reorder_encoder_artifact &artifact,
                  compression_params &params) {
  return encoder_main<bitset_size>(artifact, params);
}

const std::array<encoder_template_main_fn, 24> encoder_dispatchers = {
    &call_encoder_main<64>,   &call_encoder_main<128>,
    &call_encoder_main<192>,  &call_encoder_main<256>,
    &call_encoder_main<320>,  &call_encoder_main<384>,
    &call_encoder_main<448>,  &call_encoder_main<512>,
    &call_encoder_main<576>,  &call_encoder_main<640>,
    &call_encoder_main<704>,  &call_encoder_main<768>,
    &call_encoder_main<832>,  &call_encoder_main<896>,
    &call_encoder_main<960>,  &call_encoder_main<1024>,
    &call_encoder_main<1088>, &call_encoder_main<1152>,
    &call_encoder_main<1216>, &call_encoder_main<1280>,
    &call_encoder_main<1344>, &call_encoder_main<1408>,
    &call_encoder_main<1472>, &call_encoder_main<1536>,
};

} // namespace spring::dispatch_detail