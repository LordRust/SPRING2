// Explicit reorder-template instantiations kept in a dedicated translation
// unit so they do not force encoder code into the same compile.

#include "params.h"
#include "read_reordering_impl.h"
#include "workflow/compression_dispatch_internal.h"

namespace spring::dispatch_detail {

template <size_t bitset_size>
reorder_encoder_artifact call_reorder_main(reorder_input_artifact artifact,
                                           const compression_params &params) {
  return reorder_main<bitset_size>(std::move(artifact), params);
}

const std::array<reorder_template_main_fn, 16> reorder_dispatchers = {
    &call_reorder_main<64>,   &call_reorder_main<128>, &call_reorder_main<192>,
    &call_reorder_main<256>,  &call_reorder_main<320>, &call_reorder_main<384>,
    &call_reorder_main<448>,  &call_reorder_main<512>, &call_reorder_main<576>,
    &call_reorder_main<640>,  &call_reorder_main<704>, &call_reorder_main<768>,
    &call_reorder_main<832>,  &call_reorder_main<896>, &call_reorder_main<960>,
    &call_reorder_main<1024>,
};

} // namespace spring::dispatch_detail