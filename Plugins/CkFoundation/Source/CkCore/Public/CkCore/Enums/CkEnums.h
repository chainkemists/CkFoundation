#pragma once

#include "CkFormat/CkFormat.h"

#include "CkEnums.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/*
 * All common enums go in this file
*/

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Replication : uint8
{
    Replicates,
    DoesNotReplicate
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Replication);

// --------------------------------------------------------------------------------------------------------------------
