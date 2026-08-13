#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // RAII owner of an entt registry and its ck::registry_table slot. Get_Registry() hands out a
    // NON-owning view bound to that slot — it must not outlive the FEcsWorld that allocated it.
    class CKECS_API FEcsWorld
    {
        CK_GENERATED_BODY(FEcsWorld);

    public:
        using RegistryType = FCk_Registry;
        using HandleType = FCk_Handle;

    public:
        FEcsWorld();
        ~FEcsWorld();

        // Owns a registry_table slot: a copy would alias it (double free), a move would leak it.
        FEcsWorld(const FEcsWorld&) = delete;
        auto operator=(const FEcsWorld&) -> FEcsWorld& = delete;
        FEcsWorld(FEcsWorld&&) = delete;
        auto operator=(FEcsWorld&&) -> FEcsWorld& = delete;

    private:
        ck::registry_table::TGuardedRegistryPtr _OwnedRegistry;
        RegistryType _Registry;

    public:
        CK_PROPERTY_GET(_Registry);
        CK_PROPERTY_GET_NON_CONST(_Registry);
    };
}

// --------------------------------------------------------------------------------------------------------------------
