#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "CkThirdParty/entt-3.16.0/src/entt/entity/registry.hpp"
#include "CkThirdParty/entt-3.16.0/src/entt/entity/snapshot.hpp"

#include <type_traits>

class FArchive;

namespace ck
{
    // The entt registry type CkFoundation actually instantiates. ck::registry_table::EnttRegistryType
    // is basic_registry<FCk_Entity::IdType, std::allocator<FCk_Entity::IdType>>, and FCk_Entity::IdType
    // is entt::entity — so this is the SAME type as entt::registry (basic_registry<>). Aliasing it here
    // (rather than hard-coding entt::registry) keeps the snapshot machinery pinned to the registry the
    // ECs world owns: if the project ever swaps its entity id type, every snapshot template follows.
    using SnapshotRegistryType = ck::registry_table::EnttRegistryType;
    using SnapshotEntityType   = SnapshotRegistryType::entity_type;

    // ----------------------------------------------------------------------------------------------------------------

    // Threaded through every SerializeSnapshot(FArchive&, FSnapshotContext&) call.
    // On save: _Saver non-null, _Loader null. Snapshot_Handle writes the raw entity ID.
    // On load: _Loader non-null, _Saver null.  Snapshot_Handle consults continuous_loader's
    //          save-time-to-load-time remap and rewrites the handle to the live ID.
    //
    // Why this matters: after registry.clear() on load, saved entity IDs no longer point at
    // anything meaningful. Without continuous_loader-mediated remap, an FCk_Handle_Item field
    // saved as raw bytes restores as a stale ID -- ck::IsValid might even still pass (registry
    // may coincidentally have an entity at that ID), but it would be the WRONG entity.
    //
    // NOTE on FCk_Entity vs entt::entity:
    // FCk_Handle::Get_Entity() returns FCk_Entity (the project's wrapper around entt::entity).
    // FCk_Entity::Get_ID() exposes the raw entt::entity. Snapshot_Handle bridges these layers
    // so call sites use FCk_Handle-family handles naturally.
    class CKSNAPSHOT_API FSnapshotContext
    {
    public:
        FSnapshotContext() = default;

        explicit FSnapshotContext(entt::basic_snapshot<SnapshotRegistryType>& InSaver)
            : _Saver(&InSaver) {}

        explicit FSnapshotContext(entt::basic_continuous_loader<SnapshotRegistryType>& InLoader)
            : _Loader(&InLoader) {}

    public:
        // Serialize a raw entt entity (used by archive writer/reader).
        auto Snapshot_EnttEntity(FArchive& InAr, entt::entity& InOutEntity) -> void;

        // Serialize an FCk_Entity (project wrapper). Applies continuous_loader remap on load.
        auto Snapshot_Entity(FArchive& InAr, FCk_Entity& InOutEntity) -> void;

        // Serialize any FCk_Handle-family handle (FCk_Handle, FCk_Handle_TypeSafe subclasses).
        // Detected via Get_Entity()->FCk_Entity + Set_Entity(FCk_Entity).
        template <typename T_Handle>
            requires requires (T_Handle& H) {
                { H.Get_Entity() } -> std::convertible_to<FCk_Entity>;
                H.Set_Entity(FCk_Entity{});
            }
        auto Snapshot_Handle(FArchive& InAr, T_Handle& InOutHandle) -> void
        {
            auto Entity = InOutHandle.Get_Entity();
            Snapshot_Entity(InAr, Entity);
            InOutHandle.Set_Entity(Entity);
        }

        auto IsLoading() const -> bool { return _Loader != nullptr; }
        auto IsSaving()  const -> bool { return _Saver  != nullptr; }

    private:
        entt::basic_snapshot<SnapshotRegistryType>*           _Saver  = nullptr;
        entt::basic_continuous_loader<SnapshotRegistryType>*  _Loader = nullptr;
    };
}
