#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkRecord_Fragment_Data.generated.h"

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