#pragma once

#include "CkCore/Format/CkFormat.h"

#include "CkAttributeEnums.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/*
 * All common enums go in this file
*/

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Attribute_ValueCategory : uint8
{
    Base,
    Final
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Attribute_ValueCategory);

// --------------------------------------------------------------------------------------------------------------------

