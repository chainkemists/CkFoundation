#pragma once

#include "CkSpatialQuery/Probe/CkProbe_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkProbe_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_Probe_HandleRequests; }
// ReSharper disable once CppInconsistentNaming
namespace JPH { class PhysicsSystem; }
struct FCk_Handle_Transform;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Probe"))
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
        UPARAM(ref) FCk_Handle_Transform& InHandle,
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
        DisplayName="[Ck][Probe] Get Motion Type")
    static ECk_MotionType
    Get_MotionType(
            const FCk_Handle_Probe& InProbe);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Get Motion Quality")
    static ECk_MotionQuality
    Get_MotionQuality(
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

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Get Current Overlaps")
    static TSet<FCk_Probe_OverlapInfo>
    Get_CurrentOverlaps(
        const FCk_Handle_Probe& InProbe);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Request Enable/Disable DebugDraw",
        meta=(DevelopmentOnly))
    static FCk_Handle_Probe
    Request_EnableDisableDebugDraw(
        UPARAM(ref) FCk_Handle_Probe& InProbe,
        ECk_EnableDisable InEnableDisable = ECk_EnableDisable::Enable);

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
        DisplayName="[Ck][Probe] Request LineTrace (Multi)")
    static TArray<FCk_Probe_RayCast_Result>
    Request_MultiLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Request LineTrace (Single)")
    static FCk_Probe_RayCast_Result
    Request_SingleLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Request LineTrace (Persistent)")
    static FCk_Handle_ProbeTrace
    Request_LineTrace_Persistent(
        const FCk_Probe_RayCastPersistent_Settings& InSettings);

public:
    public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Request ShapeTrace (Multi)")
    static TArray<FCk_ShapeCast_Result>
    Request_MultiShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Probe",
        DisplayName="[Ck][Probe] Request ShapeTrace (Single)")
    static FCk_ShapeCast_Result
    Request_SingleShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings);

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
              DisplayName = "[Ck][ProbeTrace] Bind To OnBeginOverlap")
    static FCk_Handle_ProbeTrace
    BindTo_OnBeginOverlap_ProbeTrace(
        UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ProbeTrace_OnBeginOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName = "[Ck][ProbeTrace] Unbind From OnBeginOverlap")
    static FCk_Handle_ProbeTrace
    UnbindFrom_OnBeginOverlap_ProbeTrace(
        UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnBeginOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName = "[Ck][ProbeTrace] Bind To OnOverlapUpdated")
    static FCk_Handle_ProbeTrace
    BindTo_OnOverlapUpdated_ProbeTrace(
        UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ProbeTrace_OnOverlapUpdated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName = "[Ck][ProbeTrace] Unbind From OnOverlapUpdated")
    static FCk_Handle_ProbeTrace
    UnbindFrom_OnOverlapUpdated_ProbeTrace(
        UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnOverlapUpdated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName = "[Ck][ProbeTrace] Bind To OnEndOverlap")
    static FCk_Handle_ProbeTrace
    BindTo_OnEndOverlap_ProbeTrace(
        UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ProbeTrace_OnEndOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Probe",
              DisplayName = "[Ck][ProbeTrace] Unbind From OnEndOverlap")
    static FCk_Handle_ProbeTrace
    UnbindFrom_OnEndOverlap_ProbeTrace(
        UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnEndOverlap& InDelegate);

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

public:
    static auto
    Request_MultiLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const JPH::PhysicsSystem& InPhysicsSystem) -> TArray<FCk_Probe_RayCast_Result>;

    static auto
    Request_SingleLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const JPH::PhysicsSystem& InPhysicsSystem) -> TOptional<FCk_Probe_RayCast_Result>;

    static auto
    Request_DrawLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings,
        TOptional<FCk_Probe_RayCast_Result> InResult) -> void;

    static auto
    Request_MultiShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const JPH::PhysicsSystem& InPhysicsSystem) -> TArray<FCk_ShapeCast_Result>;

    static auto
    Request_SingleShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings,
        bool InFireOverlaps,
        bool InTryDrawDebug,
        const JPH::PhysicsSystem& InPhysicsSystem) -> TOptional<FCk_ShapeCast_Result>;

    static auto
    Request_DrawShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings,
        TOptional<FCk_ShapeCast_Result> InResult) -> void;

private:
    static auto
    DrawShapeAtLocation(
        UWorld* InWorld,
        const FCk_ShapeCast_Settings& InSettings,
        const FVector& InLocation,
        const FRotator& InRotation,
        const FLinearColor& InColor,
        float InDuration,
        float InThickness) -> void;

private:
    static auto
    Request_MarkProbe_AsOverlapping(
        FCk_Handle_Probe& InProbeEntity) -> void;

    static auto
    Request_MarkProbe_AsNotOverlapping(
        FCk_Handle_Probe& InProbeEntity) -> void;
};

// --------------------------------------------------------------------------------------------------------------------