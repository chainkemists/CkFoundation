#pragma once

#include "CkCore/Format/CkFormat.h"

#include "CkUsf_Outline_Types.generated.h"

UENUM(BlueprintType)
enum class ECk_Usf_OutlineScope : uint8
{
    EntityOnly,
    EntityAndDependents
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Usf_OutlineScope);
