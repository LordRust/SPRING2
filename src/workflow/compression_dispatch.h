// Declares the runtime dispatch helpers used by the compression workflow to
// select reorder and encoder implementations sized for the current dataset.

#ifndef SPRING_COMPRESSION_DISPATCH_H_
#define SPRING_COMPRESSION_DISPATCH_H_

#include "read_reordering.h"
#include "stream_reordering.h"

namespace spring {

struct compression_params;

reorder_encoder_artifact call_reorder(const reorder_input_artifact &artifact,
                                      compression_params &cp);

reordered_stream_artifact call_encoder(const reorder_encoder_artifact &artifact,
                                       compression_params &cp);

} // namespace spring

#endif // SPRING_COMPRESSION_DISPATCH_H_