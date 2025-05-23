#pragma once

#include "CkSpatialQuery/Probe/CkProbe_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkProbe_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Probe_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKSPATIALQUERY_API UCk_Utils_Probe_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Probe_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Probe);

public:
    friend class ck::FProcessor_Probe_HandleRequests;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName="[Ck][Probe] Add Feature")
    static FCk_Handle_Probe
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Probe_ParamsData& InParams,
        const FCk_Probe_DebugInfo& InDebugInfo);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Probe",
              DisplayName="[Ck][Probe] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Probe
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Handle -> Probe Handle",
        meta = (CompactNodeTitle = "<AsProbe>", BlueprintAutocast))
    static FCk_Handle_Probe
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Probe Handle",
        Category = "Ck|Utils|Probe",
        meta = (CompactNodeTitle = "INVALID_ProbeHandle", Keywords = "make"))
    static FCk_Handle_Probe
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Get Name")
    static FGameplayTag
    Get_Name(
            const FCk_Handle_Probe& InProbe);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Get Response Policy")
    static ECk_ProbeResponse_Policy
    Get_ResponsePolicy(
            const FCk_Handle_Probe& InProbe);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Get Filter")
    static FGameplayTagContainer
    Get_Filter(
            const FCk_Handle_Probe& InProbe);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Get Surface Info")
    static FCk_Probe_SurfaceInfo
    Get_SurfaceInfo(
            const FCk_Handle_Probe& InProbe);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Get Enabled/Disabled")
    static ECk_EnableDisable
    Get_IsEnabledDisabled(
        const FCk_Handle_Probe& InProbe);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Get IsOverlapping")
    static bool
    Get_IsOverlapping(
        const FCk_Handle_Probe& InProbe);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Get IsOverlapping With")
    static bool
    Get_IsOverlappingWith(
        const FCk_Handle_Probe& InProbe,
        const FCk_Handle& InOtherEntity);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Get Can Overlap With")
    static bool
    Get_CanOverlapWith(
        const FCk_Handle_Probe& InA,
        const FCk_Handle_Probe& InB);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Request Begin Overlap",
        meta=(DevelopmentOnly))
    static FCk_Handle_Probe
    Request_BeginOverlap(
        UPARAM(ref) FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_BeginOverlap& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Request Overlap Updated",
        meta=(DevelopmentOnly))
    static FCk_Handle_Probe
    Request_OverlapUpdated(
        UPARAM(ref) FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_OverlapUpdated& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Request End Overlap",
        meta=(DevelopmentOnly))
    static FCk_Handle_Probe
    Request_EndOverlap(
        UPARAM(ref) FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_EndOverlap& InRequest);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Request Enable/Disable")
    static FCk_Handle_Probe
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_EnableDisable& InRequest);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Request LineTrace")
    static TArray<FCk_Probe_RayCast_Result>
    Request_LineTrace(
        const FCk_Handle& InAnyHandle,
        FVector InStartPos,
        FVector InEndPos,
        const FCk_Probe_RayCast_Settings& InSettings);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Request LineTrace (Single)")
    static FCk_Probe_RayCast_Result
    Request_SingleLineTrace(
        FCk_Handle InAnyHandle,
        FVector InStartPos,
        FVector InEndPos,
        const FCk_Probe_RayCast_Settings& InSettings);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName = "[Ck][Probe] Bind To OnBeginOverlap")
    static FCk_Handle_Probe
    BindTo_OnBeginOverlap(
        UPARAM(ref) FCk_Handle_Probe& InProbeEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Probe_OnBeginOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName = "[Ck][Probe] Unbind From OnBeginOverlap")
    static FCk_Handle_Probe
    UnbindFrom_OnBeginOverlap(
        UPARAM(ref) FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnBeginOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName = "[Ck][Probe] Bind To OnOverlapUpdated")
    static FCk_Handle_Probe
    BindTo_OnOverlapUpdated(
        UPARAM(ref) FCk_Handle_Probe& InProbeEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Probe_OnOverlapUpdated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName = "[Ck][Probe] Unbind From OnOverlapUpdated")
    static FCk_Handle_Probe
    UnbindFrom_OnOverlapUpdated(
        UPARAM(ref) FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnOverlapUpdated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName = "[Ck][Probe] Bind To OnEndOverlap")
    static FCk_Handle_Probe
    BindTo_OnEndOverlap(
        UPARAM(ref) FCk_Handle_Probe& InProbeEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Probe_OnEndOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName = "[Ck][Probe] Unbind From OnEndOverlap")
    static FCk_Handle_Probe
    UnbindFrom_OnEndOverlap(
        UPARAM(ref) FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnEndOverlap& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName = "[Ck][Probe] Bind To OnEnabledDisable")
    static FCk_Handle_Probe
    BindTo_OnEnableDisable(
        UPARAM(ref) FCk_Handle_Probe& InProbeEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Probe_OnEnableDisable& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName = "[Ck][Probe] Unbind From OnEnabledDisable")
    static FCk_Handle_Probe
    UnbindFrom_OnEnableDisable(
        UPARAM(ref) FCk_Handle_Probe& InProbeEntity,
        const FCk_Delegate_Probe_OnEnableDisable& InDelegate);

private:
    static auto
    Request_MarkProbe_AsOverlapping(
        FCk_Handle_Probe& InProbeEntity) -> void;

    static auto
    Request_MarkProbe_AsNotOverlapping(
        FCk_Handle_Probe& InProbeEntity) -> void;
};

// --------------------------------------------------------------------------------------------------------------------