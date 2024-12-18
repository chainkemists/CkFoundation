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
enum class ECk_AttributeModifier_Operation : uint8
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Override
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AttributeModifier_Operation);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AttributeValueChange_SyncPolicy : uint8
{
    // If this is a replicated attribute, sync the new attribute value from server to clients
    TrySyncToClients,
    DoNotSync
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AttributeValueChange_SyncPolicy);

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

UENUM(BlueprintType)
enum class ECk_Attribute_Refill_Policy : uint8
{
    // Refill rate can be negative or positive
    Variable UMETA(DisplayName = "Variable Rate"),

    // Refill rate is absolute and ALWAYS goes to 0
    AlwaysReturnToZero UMETA(DisplayName = "Always Returns to Zero")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Attribute_Refill_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Attribute_RefillState : uint8
{
    Running,
    Paused
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Attribute_RefillState);

// --------------------------------------------------------------------------------------------------------------------
