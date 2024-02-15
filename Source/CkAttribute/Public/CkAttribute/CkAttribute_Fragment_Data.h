#pragma once
#include "CkCore/Format/CkFormat.h"

#include "CkAttribute_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Attribute_RemoveModifier_Result
{
    Success,
    Failed_MissingAttribute,
    Failed_MissingModifier,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Attribute_RemoveModifier_Result);

// --------------------------------------------------------------------------------------------------------------------
