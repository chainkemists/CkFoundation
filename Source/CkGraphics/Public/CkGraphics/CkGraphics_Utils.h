#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkEcs/Delegates/CkDelegates.h"

#include "CkGraphics_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_OnAllRenderedActorsGathered_MC,
    const TArray<AActor*>&, InRenderedActors);

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_OnAllRenderedActorsGathered,
    const TArray<AActor*>&, InRenderedActors);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKGRAPHICS_API UCk_Utils_Graphics_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Graphics_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName  = "[Ck] Get Modified Color Intensity",
              Category = "Ck|Utils|Graphics")
    static FColor
    Get_ModifiedColorIntensity(
        FColor InColor,
        float InIntensity = 1.0f);

    UFUNCTION(BlueprintPure,
              DisplayName  = "[Ck] Get Was Actor Recently Rendered",
              Category = "Ck|Utils|Graphics")
    static bool
    Get_WasActorRecentlyRendered(
        AActor* InActor,
        FCk_Time InTimeTolerance);
};

// --------------------------------------------------------------------------------------------------------------------
