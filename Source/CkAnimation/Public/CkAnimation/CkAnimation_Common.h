#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkAnimation_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PlayMontageFailureReason : uint8
{
    InvalidPlayRate,
    InvalidMeshComponent,
    InvalidMontage,
    MissingAnimInstanceOnMeshComponent,
    SkeletonMismatch,
    MeshTickDisabled,
    MeshComponentCannotTickOnServer
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PlayMontageFailureReason);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AnimNotify_PlayMontagePolicy : uint8
{
    Compatible UMETA(DisplayName="Triggers output in Play Montage Task"),
    Ignored UMETA(DisplayName="Ignored by Play Montage Task")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AnimNotify_PlayMontagePolicy);

// --------------------------------------------------------------------------------------------------------------------