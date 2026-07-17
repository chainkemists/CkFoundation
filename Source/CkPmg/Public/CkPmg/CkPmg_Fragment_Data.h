#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkPmg_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Pmg_Override : uint8
{
    UseExisting,
    Override
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pmg_Override);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Pmg_RenderMode : uint8
{
    SingleSided,
    DoubleSided,
    Hidden
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pmg_RenderMode);

// --------------------------------------------------------------------------------------------------------------------
