#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkAnimation_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PlayMontageFailureReason : uint8
{
    SkeletonMismatch,
    MeshTickDisabled,
    MeshComponentCannotTickOnServer
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PlayMontageFailureReason);

// --------------------------------------------------------------------------------------------------------------------