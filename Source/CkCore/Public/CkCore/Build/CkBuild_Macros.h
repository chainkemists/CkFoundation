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

// TODO: Move to build.cs files

namespace ck
{

#if UE_BUILD_SHIPPING
    #define CK_BUILD_RELEASE 1
#else
    #define CK_BUILD_RELEASE 0
#endif

#if UE_BUILD_SHIPPING
    #define CK_BUILD_TEST 0
#else
    #define CK_BUILD_TEST 1
#endif

#if UE_BUILD_DEVELOPMENT
    #define CK_BUILD_DEVELOPMENT 1
#else
    #define CK_BUILD_DEVELOPMENT 0
#endif

#if NO_LOGGING
    #define CK_BUILD_LOGGING 0
#else
    #define CK_BUILD_LOGGING 1
#endif

}