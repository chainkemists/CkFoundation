#include "CkEcsWorld.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FEcsWorld::
    FEcsWorld()
    {
        _OwnedRegistry = MakeUnique<ck::registry_table::EnttRegistryType>();

        const auto RegistryHandle = ck::registry_table::Allocate(_OwnedRegistry.Get());
        const auto TransientEntityId = FCk_Entity{_OwnedRegistry->create()};

        // Bind view to slot, then push the transient entity into the registry's
        // ctx. Single source of truth — any view resolved from this slot,
        // including via *Handle, sees the same transient entity.
        _Registry = FCk_Registry{RegistryHandle};
        _Registry.SetContext<ck::FCtx_TransientEntity>(ck::FCtx_TransientEntity{TransientEntityId});
    }

    FEcsWorld::
    ~FEcsWorld()
    {
        // Free the slot FIRST so any outstanding handle resolves to nullptr
        // from here on, then destroy the entt registry. Order matters: between
        // these two operations, ghost-handle access fails safe.
        ck::registry_table::Free(_Registry.Get_RegistryHandle());

        _Registry = FCk_Registry{};
        _OwnedRegistry.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
