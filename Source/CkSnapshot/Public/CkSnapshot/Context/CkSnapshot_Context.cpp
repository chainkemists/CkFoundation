#include "CkSnapshot_Context.h"

#include "Serialization/Archive.h"

namespace ck
{
    auto FSnapshotContext::Snapshot_EnttEntity(FArchive& InAr, entt::entity& InOutEntity) -> void
    {
        auto RawId = static_cast<std::underlying_type_t<entt::entity>>(InOutEntity);
        InAr << RawId;

        if (InAr.IsLoading() && _Loader != nullptr)
        {
            InOutEntity = _Loader->map(static_cast<entt::entity>(RawId));
        }
        else if (InAr.IsLoading())
        {
            InOutEntity = static_cast<entt::entity>(RawId);
        }
    }

    auto FSnapshotContext::Snapshot_Entity(FArchive& InAr, FCk_Entity& InOutEntity) -> void
    {
        auto RawEnttEntity = InOutEntity.Get_ID();
        Snapshot_EnttEntity(InAr, RawEnttEntity);
        InOutEntity = FCk_Entity{ RawEnttEntity };
    }
}
