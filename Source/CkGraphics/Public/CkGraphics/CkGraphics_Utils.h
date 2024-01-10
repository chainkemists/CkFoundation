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
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Graphics")
    static FColor
    Get_ModifiedColorIntensity(
        FColor InColor,
        float InIntensity = 1.0f);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Graphics",
              meta = (DefaultToSelf = "InWorldContextObject"))
    static void
    Request_GatherAllRenderedActors(
        UObject* InWorldContextObject,
        FCk_Time InTimeTolerance,
        FCk_Delegate_OnAllRenderedActorsGathered InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
