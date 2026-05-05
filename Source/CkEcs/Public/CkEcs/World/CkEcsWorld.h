#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Lightweight RAII wrapper that owns an entt registry, allocates a slot for
    // it in ck::registry_table on construction, and releases the slot +
    // destroys the registry on destruction. The exposed Get_Registry() returns
    // a non-owning FCk_Registry view bound to that slot — fragments and
    // entities created through this view stay alive for the FEcsWorld's
    // lifetime, and any outstanding handle resolves safely (slot is freed
    // first so stale resolves return nullptr instead of UAFing).
    //
    // Used by editor-only engine subsystems that need a private ECS world
    // (e.g. UCk_EditorAssetLoader_SubSystem_UE, UCk_EditorToolbar_*). The
    // game-time UCk_EcsWorld_Subsystem_UE / UCk_EditorEcsWorld_Subsystem_UE
    // do not use FEcsWorld — they own the registry directly.
    class CKECS_API FEcsWorld
    {
        CK_GENERATED_BODY(FEcsWorld);

    public:
        using RegistryType = FCk_Registry;
        using HandleType = FCk_Handle;

    public:
        FEcsWorld();
        ~FEcsWorld();

        // FEcsWorld is non-copyable and non-movable: it owns a slot in the
        // global registry table, and copy/move would either alias the slot
        // (double-free risk) or leak it. Callers who need to pass it around
        // should hold an FEcsWorld by reference / pointer.
        FEcsWorld(const FEcsWorld&) = delete;
        auto operator=(const FEcsWorld&) -> FEcsWorld& = delete;
        FEcsWorld(FEcsWorld&&) = delete;
        auto operator=(FEcsWorld&&) -> FEcsWorld& = delete;

    private:
        TUniquePtr<ck::registry_table::EnttRegistryType> _OwnedRegistry;
        RegistryType _Registry;

    public:
        CK_PROPERTY_GET(_Registry);
        CK_PROPERTY_GET_NON_CONST(_Registry);
    };
}

// --------------------------------------------------------------------------------------------------------------------
