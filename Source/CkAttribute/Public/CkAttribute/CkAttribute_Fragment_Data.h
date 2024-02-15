#pragma once
#include "CkCore/Format/CkFormat.h"

#include "CkAttribute_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AttributeModifier_Remove_Result : uint8
{
    Success,
    Failed_MissingAttribute,
    Failed_MissingModifier,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AttributeModifier_Remove_Result);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Attribute_ExistMissing : uint8
{
    HasAttribute,
    MissingAttribute
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Attribute_ExistMissing);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AttributeModifier_ExistMissing : uint8
{
    HasModifier,
    MissingModifier
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AttributeModifier_ExistMissing);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Attribute_BaseBonusFinal : uint8
{
    Base,
    Bonus,
    Final
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Attribute_BaseBonusFinal);

// --------------------------------------------------------------------------------------------------------------------
