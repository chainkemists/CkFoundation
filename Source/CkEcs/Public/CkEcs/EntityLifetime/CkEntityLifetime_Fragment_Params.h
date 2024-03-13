#pragma once

#include "CkEntityLifetime_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EntityLifetime_DestructionBehavior : uint8
{
    ForceDestroy,
    DestroyOnlyIfOrphan
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EntityLifetime_DestructionBehavior);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EntityLifetime_DestructionPhase : uint8
{
    // Entity is still Valid for the remainder of THIS frame
    Initiated,
    // Entity has been invalidated and is no longer safe to mutate
    Confirmed,
    // Entity may be in any destruction phase
    InitiatedOrConfirmed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EntityLifetime_DestructionPhase);

// --------------------------------------------------------------------------------------------------------------------
