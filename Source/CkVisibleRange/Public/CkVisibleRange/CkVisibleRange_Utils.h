#pragma once

#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkVisibleRange/CkVisibleRange_Fragment.h"

#include "CkVisibleRange/CkVisibleRange_Fragment_Data.h"

#include "CkVisibleRange_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_VisibleRange"))
class CKVISIBLERANGE_API UCk_Utils_VisibleRange_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VisibleRange_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_VisibleRange);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisibleRange",
              DisplayName="[Ck][VisibleRange] Add New Visible Range")
    static FCk_Handle_VisibleRange
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_VisibleRange_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|VisibleRange",
        DisplayName="[Ck][VisibleRange] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_VisibleRange
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|VisibleRange",
        DisplayName="[Ck][VisibleRange] Handle -> VisibleRange Handle",
        meta = (CompactNodeTitle = "<AsVisibleRange>", BlueprintAutocast))
    static FCk_Handle_VisibleRange
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid VisibleRange Handle",
        Category = "Ck|Utils|VisibleRange",
        meta = (CompactNodeTitle = "INVALID_VisibleRangeHandle", Keywords = "make"))
    static FCk_Handle_VisibleRange
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisibleRange",
              DisplayName="[Ck][VisibleRange] Get Min Range")
    static float
    Get_MinRange(
        const FCk_Handle_VisibleRange& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisibleRange",
              DisplayName="[Ck][VisibleRange] Get Max Range")
    static float
    Get_MaxRange(
        const FCk_Handle_VisibleRange& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisibleRange",
              DisplayName="[Ck][VisibleRange] Get Fade Band")
    static float
    Get_FadeBandCm(
        const FCk_Handle_VisibleRange& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisibleRange",
              DisplayName="[Ck][VisibleRange] Get Fade Alpha")
    static float
    Get_FadeAlpha(
        const FCk_Handle_VisibleRange& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisibleRange",
              DisplayName="[Ck][VisibleRange] Get Is Hidden")
    static bool
    Get_IsHidden(
        const FCk_Handle_VisibleRange& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisibleRange",
              DisplayName="[Ck][VisibleRange] Update Distance")
    static FCk_Handle_VisibleRange
    Update_Distance(
        UPARAM(ref) FCk_Handle_VisibleRange& InHandle,
        float InDistance);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisibleRange",
              DisplayName="[Ck][VisibleRange] Request Set Visibility",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_VisibleRange
    Request_SetVisibility(
        UPARAM(ref) FCk_Handle_VisibleRange& InHandle,
        ECk_VisibleRange_ShowHide InShowHide,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisibleRange",
              DisplayName="[Ck][VisibleRange] Compute Fade Alpha")
    static float
    Compute_FadeAlpha(
        float InMinRange,
        float InMaxRange,
        float InFadeBandCm,
        float InDistance);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VisibleRange",
              DisplayName="[Ck][VisibleRange] Compute Is In Range")
    static bool
    Compute_IsInRange(
        float InMinRange,
        float InMaxRange,
        float InDistance);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisibleRange",
              DisplayName = "[Ck][VisibleRange] Bind To OnHiddenChanged")
    static FCk_Handle_VisibleRange
    BindTo_OnHiddenChanged(
        UPARAM(ref) FCk_Handle_VisibleRange& InHandle,
        const FCk_Delegate_VisibleRange_HiddenChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|VisibleRange",
              DisplayName = "[Ck][VisibleRange] Unbind From OnHiddenChanged")
    static FCk_Handle_VisibleRange
    UnbindFrom_OnHiddenChanged(
        UPARAM(ref) FCk_Handle_VisibleRange& InHandle,
        const FCk_Delegate_VisibleRange_HiddenChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
