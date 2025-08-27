#pragma once

#include "CkCue_Fragment.h"
#include "CkCueBase_EntityScript.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"

#include "CkCue_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Cue"))
class CKCUE_API UCk_Utils_Cue_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Cue_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Cue);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Cue",
        DisplayName="[Ck][Cue] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Cue",
        DisplayName="[Ck][Cue] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Cue
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Cue",
        DisplayName="[Ck][Cue] Handle -> Cue Handle",
        meta = (CompactNodeTitle = "<AsCue>", BlueprintAutocast))
    static FCk_Handle_Cue
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Cue",
        DisplayName = "[Ck][Cue] Get Invalid Handle",
        meta = (CompactNodeTitle = "INVALID_CueHandle", Keywords = "make"))
    static FCk_Handle_Cue
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Cue",
        DisplayName="[Ck][Cue] Request Execute")
    static FCk_Handle_PendingEntityScript
    Request_Execute(
        const FCk_Handle& InOwnerEntity,
        const FGameplayTag& InCueName,
        FInstancedStruct InSpawnParams);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Cue",
        DisplayName="[Ck][Cue] Request Execute Local")
    static FCk_Handle_PendingEntityScript
    Request_Execute_Local(
        const FCk_Handle& InOwnerEntity,
        const FGameplayTag& InCueName,
        FInstancedStruct InSpawnParams);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Cue",
        DisplayName="[Ck][Cue] Get Name")
    static FGameplayTag
    Get_Name(
        const FCk_Handle_Cue& InCueEntity);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Cue",
        DisplayName="[Ck][Cue] Get Cue Class")
    static TSubclassOf<UCk_CueBase_EntityScript>
    Get_CueClass(
        const FCk_Handle_Cue& InCueEntity);
};

// --------------------------------------------------------------------------------------------------------------------
