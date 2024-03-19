#pragma once

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkEcsBasics/CkEcsBasics_Utils.h"

#include "CkSubstep/CkSubstep_Fragment_Data.h"

#include "CkSubstep_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKSUBSTEP_API UCk_Utils_Substep_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Substep_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Substep);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Substep",
              DisplayName="[Ck][Substep] Add Substep Feature")
    static FCk_Handle_Substep
    Add(
        UPARAM(ref) FCk_Handle InHandle,
        const FCk_Substep_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Substep",
              DisplayName="[Ck][Substep] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Substep",
        DisplayName="[Ck][Substep] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Substep
    DoCast(
        FCk_Handle InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Substep",
        DisplayName="[Ck][Substep] Handle -> Substep Handle",
        meta = (CompactNodeTitle = "<AsSubstep>", BlueprintAutocast))
    static FCk_Handle_Substep
    DoCastChecked(
        FCk_Handle InHandle);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Substep",
        DisplayName="[Ck][Substep] Request Pause")
    static FCk_Handle_Substep
    Request_Pause(
        UPARAM(ref) FCk_Handle_Substep& InHandle);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Substep",
        DisplayName="[Ck][Substep] Request Resume")
    static FCk_Handle_Substep
    Request_Resume(
        UPARAM(ref) FCk_Handle_Substep& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Substep",
              DisplayName = "[Ck][Substep] Bind To OnUpdate")
    static FCk_Handle_Substep
    BindTo_OnUpdate(
        UPARAM(ref) FCk_Handle_Substep& InSubstepHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Substep_OnUpdate& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Substep",
              DisplayName = "[Ck][Substep] Unbind From OnUpdate")
    static FCk_Handle_Substep
    UnbindFrom_OnUpdate(
        UPARAM(ref) FCk_Handle_Substep& InSubstepHandle,
        const FCk_Delegate_Substep_OnUpdate& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
