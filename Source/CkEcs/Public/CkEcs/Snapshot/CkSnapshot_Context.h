#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "CkThirdParty/entt-3.16.0/src/entt/entity/registry.hpp"

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
    // On save: default-constructed. Snapshot_Handle writes each handle's raw entity ID.
    // On load (v3 rebuild+hydrate): the map-backed ctor supplies a saved-id -> live-handle map.
    //          Snapshot_Handle remaps each raw saved ID through it and rewrites the handle to the
    //          live ID (a raw ID absent from the map rewrites to entt::null -> invalid handle).
    //
    // Why this matters: after the world is rebuilt on load, saved entity IDs no longer point at
    // anything meaningful. Without the map-mediated remap, an FCk_Handle_Item field saved as raw
    // bytes restores as a stale ID -- ck::IsValid might even still pass (registry may coincidentally
    // have an entity at that ID), but it would be the WRONG entity.
    //
    // NOTE on FCk_Entity vs entt::entity:
    // FCk_Handle::Get_Entity() returns FCk_Entity (the project's wrapper around entt::entity).
    // FCk_Entity::Get_ID() exposes the raw entt::entity. Snapshot_Handle bridges these layers
    // so call sites use FCk_Handle-family handles naturally.
    class CKECS_API FSnapshotContext
    {
    public:
        FSnapshotContext() = default;

        // v3 rebuild+hydrate load mode (spec §4.2, Phase 3B). The v3 save wrote each handle's RAW saved id
        // (packed entt id, the same value as FCk_Snapshot_V3_EntityEntry::_SavedId). On load, remap each raw id
        // through the loader-built saved-id -> live-handle map. A raw id absent from
        // the map (incl. the writer's 0xFFFFFFFF k_NoEntity sentinel, or a ref to a non-persisted entity) rewrites
        // to entt::null so ck::IsValid fails -- correct dangling-ref semantics. InLoadRegistryHandle re-homes the
        // handle onto the live world (Snapshot_Handle's tail).
        explicit FSnapshotContext(const TMap<uint32, FCk_Handle>* InSavedIdMap,
                                  FCk_RegistryHandle InLoadRegistryHandle)
            : _SavedIdMap(InSavedIdMap), _LoadRegistryHandle(InLoadRegistryHandle) {}

    public:
        // Serialize a raw entt entity (used by archive writer/reader).
        auto Snapshot_EnttEntity(FArchive& InAr, entt::entity& InOutEntity) -> void;

        // Serialize an FCk_Entity (project wrapper). Applies the saved-id -> live-handle remap on load.
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

            // Re-home the restored handle onto the live load-target registry. The deserialized handle's
            // _RegistryHandle still points at the SAVING world's slot — harmless on a same-world (OpenLevel)
            // load, but after seamless travel that slot belongs to the destroyed world, so the handle resolves
            // to [INVALID REGISTRY] (breaking Get_LifetimeOwner, Records, every handle-storing fragment). Only
            // re-home when we actually have a load target (the registry-only path passes Unset and skips this).
            if (IsLoading() && _LoadRegistryHandle.IsSet())
            { InOutHandle.Set_Registry(_LoadRegistryHandle); }
        }

        auto IsLoading() const -> bool { return _SavedIdMap != nullptr; }

    private:
        // v3 load remap (set only by the map-backed ctor). Consulted by Snapshot_EnttEntity ahead of the raw-cast
        // fallback. Not owning -- the loader owns the map for the duration of the restore.
        const TMap<uint32, FCk_Handle>*                       _SavedIdMap = nullptr;

        // The live registry restored handles are re-homed onto (load path only). Unset on save and on the
        // registry-only restore path.
        FCk_RegistryHandle _LoadRegistryHandle = FCk_RegistryHandle::Unset();
    };
}
