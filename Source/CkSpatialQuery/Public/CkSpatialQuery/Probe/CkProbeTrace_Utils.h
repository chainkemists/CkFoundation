#pragma once

#include "CkSpatialQuery/Probe/CkProbe_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkProbeTrace_Utils.generated.h"

namespace JPH { class PhysicsSystem; }

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_ProbeTrace"))
class CKSPATIALQUERY_API UCk_Utils_ProbeTrace_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ProbeTrace_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_ProbeTrace);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProbeTrace",
              DisplayName="[Ck][ProbeTrace] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ProbeTrace",
        DisplayName="[Ck][ProbeTrace] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_ProbeTrace
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|ProbeTrace",
        DisplayName="[Ck][ProbeTrace] Handle -> ProbeTrace Handle",
        meta = (CompactNodeTitle = "<AsProbeTrace>", BlueprintAutocast))
    static FCk_Handle_ProbeTrace
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid ProbeTrace Handle",
        Category = "Ck|Utils|ProbeTrace",
        meta = (CompactNodeTitle = "INVALID_ProbeTraceHandle", Keywords = "make"))
    static FCk_Handle_ProbeTrace
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ProbeTrace",
        DisplayName="[Ck][ProbeTrace] Request LineTrace (Multi)")
    static TArray<FCk_Probe_RayCast_Result>
    Request_MultiLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ProbeTrace",
        DisplayName="[Ck][ProbeTrace] Request LineTrace (Single)")
    static FCk_Probe_RayCast_Result
    Request_SingleLineTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_Probe_RayCast_Settings& InSettings);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProbeTrace",
              DisplayName="[Ck][ProbeTrace] Create LineTrace (Persistent)")
    static FCk_Handle_ProbeTrace
    Create_LineTrace_Persistent(
        const FCk_Probe_RayCastPersistent_Settings& InSettings);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProbeTrace",
              DisplayName="[Ck][ProbeTrace] Create ShapeTrace (Persistent)",
              meta = (Keywords = "box, sphere, capsule, cylinder"))
    static FCk_Handle_ProbeTrace
    Create_ShapeTrace_Persistent(
        const FCk_Probe_ShapeCastPersistent_Settings& InSettings);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ProbeTrace",
        DisplayName="[Ck][ProbeTrace] Request ShapeTrace (Multi)",
        meta = (Keywords = "box, sphere, capsule, cylinder"))
    static TArray<FCk_ShapeCast_Result>
    Request_MultiShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|ProbeTrace",
        DisplayName="[Ck][ProbeTrace] Request ShapeTrace (Single)",
        meta = (Keywords = "box, sphere, capsule, cylinder"))
    static FCk_ShapeCast_Result
    Request_SingleShapeTrace(
        const FCk_Handle& InAnyHandle,
        const FCk_ShapeCast_Settings& InSettings);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProbeTrace",
              DisplayName = "[Ck][ProbeTrace] Bind To OnBeginOverlap")
    static FCk_Handle_ProbeTrace
    BindTo_OnBeginOverlap(
        UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnBeginOverlap& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProbeTrace",
              DisplayName = "[Ck][ProbeTrace] Unbind From OnBeginOverlap")
    static FCk_Handle_ProbeTrace
    UnbindFrom_OnBeginOverlap(
        UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnBeginOverlap& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProbeTrace",
              DisplayName = "[Ck][ProbeTrace] Bind To OnOverlapUpdated")
    static FCk_Handle_ProbeTrace
    BindTo_OnOverlapUpdated(
        UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnOverlapUpdated& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProbeTrace",
              DisplayName = "[Ck][ProbeTrace] Unbind From OnOverlapUpdated")
    static FCk_Handle_ProbeTrace
    UnbindFrom_OnOverlapUpdated(
        UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnOverlapUpdated& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProbeTrace",
              DisplayName = "[Ck][ProbeTrace] Bind To OnEndOverlap")
    static FCk_Handle_ProbeTrace
    BindTo_OnEndOverlap(
        UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnEndOverlap& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ProbeTrace",
              DisplayName = "[Ck][ProbeTrace] Unbind From OnEndOverlap")
    static FCk_Handle_ProbeTrace
    UnbindFrom_OnEndOverlap(
        UPARAM(ref) FCk_Handle_ProbeTrace& InProbeTraceEntity,
        const FCk_Delegate_ProbeTrace_OnEndOverlap& InDelegate);

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

    static auto
    DrawShapeConnector(
        UWorld* InWorld,
        const FVector& InStartPos,
        const FVector& InEndPos,
        const FCk_AnyShape& InShape,
        const FLinearColor& InColor,
        float InDuration,
        float InThickness) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
