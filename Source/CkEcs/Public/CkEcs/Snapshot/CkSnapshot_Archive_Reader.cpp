#include "CkSnapshot_Archive_Reader.h"

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
