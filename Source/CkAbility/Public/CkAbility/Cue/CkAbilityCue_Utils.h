#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkAbilityCue_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKABILITY_API UCk_Utils_AbilityCue_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AbilityCue_UE);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AbilityCue",
        DisplayName="Request Spawn Ability Cue")
    static FCk_Handle
    Request_Spawn_AbilityCue(
        FCk_Handle InHandle,
        const FCk_Request_AbilityCue_Spawn& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|AbilityCue",
        DisplayName="Request Replicate Location")
    static void
    Request_ReplicateLocation(
        FCk_Handle InAbilityCueRequestEntity,
        FVector InLocation);

};

// --------------------------------------------------------------------------------------------------------------------
