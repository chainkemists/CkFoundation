#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkEntityLifetime_Fragment_Data.generated.h"

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
enum class ECk_EntityLifetime_OwnerType : uint8
{
    UseTransientEntity,
    UseCustomEntity
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EntityLifetime_OwnerType);

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

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Lifetime_OnDestroy,
    FCk_Handle, InHandle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_Lifetime_OnDestroy_MC,
    FCk_Handle, InHandle);

// --------------------------------------------------------------------------------------------------------------------
