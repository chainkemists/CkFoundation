#pragma once

#include "CkVfxCue_Fragment_Data.h"
#include "CkVfxCue_EntityScript.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkVfxCue_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKVFX_API UCk_Utils_VfxCue_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VfxCue_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VfxCue);

public:
    static FCk_Handle_VfxCue
    Add(
        FCk_Handle& InHandle,
        const UCk_VfxCue_EntityScript& InVfxCueScript);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VfxCue",
              DisplayName="[Ck][VfxCue] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|VfxCue",
        DisplayName="[Ck][VfxCue] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_VfxCue
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VfxCue",
        DisplayName="[Ck][VfxCue] Handle -> VfxCue Handle",
        meta = (CompactNodeTitle = "<AsVfxCue>", BlueprintAutocast))
    static FCk_Handle_VfxCue
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid VfxCue Handle",
        Category = "Ck|Utils|VfxCue",
        meta = (CompactNodeTitle = "INVALID_VfxCueHandle", Keywords = "make"))
    static FCk_Handle_VfxCue
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VfxCue",
              DisplayName="[Ck][VfxCue] Request Play")
    static FCk_Handle_VfxCue
    Request_Play(
        UPARAM(ref) FCk_Handle_VfxCue& InVfxCue,
        const FCk_Request_VfxCue_Play& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VfxCue",
              DisplayName="[Ck][VfxCue] Request Stop")
    static FCk_Handle_VfxCue
    Request_Stop(
        UPARAM(ref) FCk_Handle_VfxCue& InVfxCue);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VfxCue",
              DisplayName = "[Ck][VfxCue] Bind To OnStarted")
    static FCk_Handle_VfxCue
    BindTo_OnStarted(
        UPARAM(ref) FCk_Handle_VfxCue& InVfxCue,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VfxCue_OnStarted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VfxCue",
              DisplayName = "[Ck][VfxCue] Bind To OnFinished")
    static FCk_Handle_VfxCue
    BindTo_OnFinished(
        UPARAM(ref) FCk_Handle_VfxCue& InVfxCue,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VfxCue_OnFinished& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VfxCue",
              DisplayName = "[Ck][VfxCue] Unbind From OnStarted")
    static FCk_Handle_VfxCue
    UnbindFrom_OnStarted(
        UPARAM(ref) FCk_Handle_VfxCue& InVfxCue,
        const FCk_Delegate_VfxCue_OnStarted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VfxCue",
              DisplayName = "[Ck][VfxCue] Unbind From OnFinished")
    static FCk_Handle_VfxCue
    UnbindFrom_OnFinished(
        UPARAM(ref) FCk_Handle_VfxCue& InVfxCue,
        const FCk_Delegate_VfxCue_OnFinished& InDelegate);

private:
    friend class UCk_VfxCue_EntityScript;
};

// --------------------------------------------------------------------------------------------------------------------
