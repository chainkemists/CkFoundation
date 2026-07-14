#include "CkSnapshot_Context.h"

#include "CkCore/Validation/CkIsValid.h" // ck::IsValid (v3 map-backed remap)

#include "Serialization/Archive.h"

namespace ck
{
    auto FSnapshotContext::Snapshot_EnttEntity(FArchive& InAr, entt::entity& InOutEntity) -> void
    {
        auto RawId = static_cast<std::underlying_type_t<entt::entity>>(InOutEntity);
        InAr << RawId;

        if (InAr.IsLoading() && _SavedIdMap != nullptr)
        {
            // v3 rebuild+hydrate: remap the raw saved id through the loader-built saved-id -> live-handle map.
            // Absent (incl. the 0xFFFFFFFF k_NoEntity sentinel or a non-persisted ref) -> entt::null -> invalid handle.
            const auto* Mapped = _SavedIdMap->Find(static_cast<uint32>(RawId));
            InOutEntity = (Mapped != nullptr && ck::IsValid(*Mapped))
                ? Mapped->Get_Entity().Get_ID()
                : entt::null;
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
