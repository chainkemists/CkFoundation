#include "CkSnapshot_Archive_Writer.h"

namespace ck
{
    auto FSnapshotArchive_Writer::operator()(entt::entity InEntity) -> void
    {
        auto Raw = static_cast<std::underlying_type_t<entt::entity>>(InEntity);
        _Proxy << Raw;
    }

    auto FSnapshotArchive_Writer::operator()(std::underlying_type_t<entt::entity> InSize) -> void
    {
        _Proxy << InSize;
    }
}
