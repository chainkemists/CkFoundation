#include "CkSnapshot_Archive_Reader.h"

#if CK_WITH_FIDELITY_ORACLE // Phase 5: Model-A serialization archive is oracle/test-only (v3 uses MemoryReader + proxy)

namespace ck
{
    auto FSnapshotArchive_Reader::operator()(entt::entity& OutEntity) -> void
    {
        std::underlying_type_t<entt::entity> Raw = 0;
        _Proxy << Raw;
        OutEntity = static_cast<entt::entity>(Raw);
    }

    auto FSnapshotArchive_Reader::operator()(std::underlying_type_t<entt::entity>& OutSize) -> void
    {
        _Proxy << OutSize;
    }
}

#endif // CK_WITH_FIDELITY_ORACLE
