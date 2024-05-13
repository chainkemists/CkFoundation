#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkRecord_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// NOTE: this _should_ be in CkEntityExtensions but then we have a circular dependency
USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKRECORD_API FCk_Handle_EntityExtension : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_EntityExtension); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_EntityExtension);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_FragmentQuery_Policy : uint8
{
    CurrentEntity,
    EntityInRecord
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_FragmentQuery_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Record_ForEachIterationResult : uint8
{
    Continue,
    Break
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Record_ForEachIterationResult);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Record_EntryHandlingPolicy : uint8
{
    Default,

    // Cannot add multiple Record Entries that have the same name (GameplayLabel)
    DisallowDuplicateNames
};

ENUM_CLASS_FLAGS(ECk_Record_EntryHandlingPolicy)
ENABLE_ENUM_BITWISE_OPERATORS(ECk_Record_EntryHandlingPolicy);
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Record_EntryHandlingPolicy);

// --------------------------------------------------------------------------------------------------------------------