#include "CkEcsWorld.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FEcsWorld::
    FEcsWorld()
    {
        _OwnedRegistry = ck::registry_table::Make_GuardedRegistry();

        const auto RegistryHandle = ck::registry_table::Allocate(_OwnedRegistry.Get());
        const auto TransientEntityId = FCk_Entity{_OwnedRegistry->create()};

        // The transient entity lives in the registry's ctx so every view resolved from this slot,
        // including via *Handle, sees the same one.
        _Registry = FCk_Registry{RegistryHandle};
        _Registry.SetContext<ck::FCtx_TransientEntity>(ck::FCtx_TransientEntity{TransientEntityId});
    }

    FEcsWorld::
    ~FEcsWorld()
    {
        // Free the slot BEFORE destroying the registry: in between, a ghost handle resolves to null
        // rather than reaching freed memory.
        ck::registry_table::Free(_Registry.Get_RegistryHandle());

        _Registry = FCk_Registry{};

        _OwnedRegistry.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
