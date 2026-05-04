#include "frame_executor_internal.h"
#include "tachyon/runtime/execution/property_sampling.h"

namespace tachyon {

std::uint64_t build_node_key(std::uint64_t global_key, const CompiledNode& node) {
    CacheKeyBuilder builder;
    builder.add_u64(global_key);
    builder.add_u32(node.node_id);
    builder.add_u32(node.topo_index);
    builder.add_u32(node.version);
    return builder.finish();
}

} // namespace tachyon
