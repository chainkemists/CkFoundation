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
