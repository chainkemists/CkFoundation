#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkCore/Macros/CkMacros.h"

#include "CkStats_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKPROFILE_API UCk_Utils_Stats_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Stats_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get FPS",
              Category = "Ck|Utils|Profile|Stats")
    static float
    Get_FPS();

    UFUNCTION(BlueprintPure,
              DisplayName="[Ck] Get Is Stat Enabled",
              Category = "Ck|Utils|Profile|Stats")
    static bool
    Get_IsStatEnabled(
        const FString& InStatName);
};

// --------------------------------------------------------------------------------------------------------------------
