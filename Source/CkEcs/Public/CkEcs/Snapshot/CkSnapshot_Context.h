#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "CkThirdParty/entt-3.16.0/src/entt/entity/registry.hpp"

#include <type_traits>

class FArchive;

namespace ck
{
    // Aliased rather than hard-coded to entt::registry so that swapping the project's entity id type
    // carries every snapshot template along with it.
    using SnapshotRegistryType = ck::registry_table::EnttRegistryType;
    using SnapshotEntityType   = SnapshotRegistryType::entity_type;

    // ----------------------------------------------------------------------------------------------------------------

    // Threaded through every SerializeSnapshot(FArchive&, FSnapshotContext&) call. On save it is
    // default-constructed and each handle's raw entity ID is written. On load the map-backed ctor
    // supplies the saved-id -> live-handle map: without that remap a saved ID would restore as a
    // stale ID that ck::IsValid may still accept while naming the WRONG entity.
    class CKECS_API FSnapshotContext
    {
    public:
        FSnapshotContext() = default;

        // Rebuild+hydrate load mode. A raw saved id absent from the map (the writer's 0xFFFFFFFF k_NoEntity
        // sentinel, or a ref to a non-persisted entity) rewrites to entt::null so ck::IsValid fails —
        // correct dangling-ref semantics. InLoadRegistryHandle re-homes the handle onto the live world.
        explicit FSnapshotContext(const TMap<uint32, FCk_Handle>* InSavedIdMap,
                                  FCk_RegistryHandle InLoadRegistryHandle)
            : _SavedIdMap(InSavedIdMap), _LoadRegistryHandle(InLoadRegistryHandle) {}

    public:
        auto Snapshot_EnttEntity(FArchive& InAr, entt::entity& InOutEntity) -> void;

        // Applies the saved-id -> live-handle remap on load.
        auto Snapshot_Entity(FArchive& InAr, FCk_Entity& InOutEntity) -> void;

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

            // The deserialized handle's _RegistryHandle still points at the SAVING world's slot — after
            // seamless travel that slot belongs to the destroyed world and the handle resolves to
            // [INVALID REGISTRY]. The registry-only path passes Unset and deliberately skips the re-home.
            if (IsLoading() && _LoadRegistryHandle.IsSet())
            { InOutHandle.Set_Registry(_LoadRegistryHandle); }
        }

        auto IsLoading() const -> bool { return _SavedIdMap != nullptr; }

    private:
        // Not owning — the loader owns the map for the duration of the restore.
        const TMap<uint32, FCk_Handle>*                       _SavedIdMap = nullptr;

        // Unset on save and on the registry-only restore path.
        FCk_RegistryHandle _LoadRegistryHandle = FCk_RegistryHandle::Unset();
    };
}
