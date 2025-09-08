#pragma once

#include <CoreMinimal.h>

#include "CkBuild_Macros.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Build_Type : uint8
{
    TestOrDevelopment,
    Release
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Build_Specific_Type : uint8
{
    Test,
    Development,
    Release
};

// --------------------------------------------------------------------------------------------------------------------