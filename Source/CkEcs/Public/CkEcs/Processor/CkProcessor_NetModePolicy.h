#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkProcessor_NetModePolicy.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ProcessorNetModeRequirement : uint8
{
    // Runs everywhere (explicit no-op)
    All,
    // Skip if dedicated server (Host tag present, Client tag absent)
    CosmeticOnly,
    // Only run if has authority (DedicatedServer or ListenServer — Host tag present)
    AuthorityOnly,
    // Only run on pure clients (Client tag present, Host tag absent — excludes ListenServer)
    ClientOnly,
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CKECS_API auto
    ShouldCreateProcessorForNetMode(
        ECk_ProcessorNetModeRequirement InRequirement,
        const FCk_Handle& InTransientEntity) -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
