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

        _Registry = FCk_Registry{RegistryHandle, TransientEntityId};
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
