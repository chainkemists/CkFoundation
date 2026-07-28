#pragma once

#include "CkAggro/CkAggro_Fragment.h"
#include "CkAggro/CkAggro_Fragment_Data.h"
#include "CkAggro/CkAggroTarget_Fragment_Data.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkAggro_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Aggro"))
class CKAGGRO_API UCk_Utils_Aggro_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Aggro_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Aggro);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    using RecordOfAggroTargets_Utils = ck::FUtils_RecordOfAggroTargets;

public:
    // Direct-attach the Aggro brain onto InHandle (the AI entity). Requires a Transform feature (spatial scoring).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Add Feature")
    static FCk_Handle_Aggro
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Aggro_ParamsData& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Aggro",
        DisplayName="[Ck][Aggro] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Aggro
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Aggro",
        DisplayName="[Ck][Aggro] Handle -> Aggro Handle",
        meta = (CompactNodeTitle = "<AsAggro>", BlueprintAutocast))
    static FCk_Handle_Aggro
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Aggro Handle",
        Category = "Ck|Utils|Aggro",
        meta = (CompactNodeTitle = "INVALID_AggroHandle", Keywords = "make"))
    static FCk_Handle_Aggro
    Get_InvalidHandle() { return {}; }

public:
    // One-shot: dedupe by tracked entity (returns the existing target), else create a child target under InOwner from
    // the owner's DefaultTargetParams (cap/evict + record-connect + map). Aggro is the accelerant on top of AggroTarget.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Create Target")
    static FCk_Handle_AggroTarget
    CreateTarget(
        UPARAM(ref) FCk_Handle_Aggro& InOwner,
        FCk_Handle InTracked);

    // As CreateTarget, but each param section whose override toggle is on replaces the owner default for this target.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Create Target With Params")
    static FCk_Handle_AggroTarget
    CreateTarget_WithParams(
        UPARAM(ref) FCk_Handle_Aggro& InOwner,
        FCk_Handle InTracked,
        const FCk_AggroTarget_ParamOverrides& InOverrides);

private:
    // InParams must already carry the tracked entity.
    static FCk_Handle_AggroTarget
    DoCreateTarget(
        FCk_Handle_Aggro& InOwner,
        const FCk_Fragment_AggroTarget_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Get Is Enabled")
    static bool
    Get_IsEnabled(
        const FCk_Handle_Aggro& InAggro);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Get Active Target")
    static FCk_Handle_AggroTarget
    Get_ActiveTarget(
        const FCk_Handle_Aggro& InAggro);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Try Get Active Tracked Entity")
    static FCk_Handle
    TryGet_ActiveTrackedEntity(
        const FCk_Handle_Aggro& InAggro);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Try Get Target By Tracked Entity")
    static FCk_Handle_AggroTarget
    TryGet_Target_ByTrackedEntity(
        const FCk_Handle_Aggro& InAggro,
        const FCk_Handle& InTrackedEntity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Get Num Tracked Targets")
    static int32
    Get_NumTrackedTargets(
        const FCk_Handle_Aggro& InAggro);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Get Debug Evaluation Count")
    static int64
    Get_Debug_EvaluationCount(
        const FCk_Handle_Aggro& InAggro);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] For Each Target",
              meta=(AutoCreateRefTerm="InDelegate, InOptionalPayload"))
    static TArray<FCk_Handle_AggroTarget>
    ForEach_Target(
        const FCk_Handle_Aggro& InAggro,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);
    static auto
    ForEach_Target(
        const FCk_Handle_Aggro& InAggro,
        const TFunction<void(FCk_Handle_AggroTarget)>& InFunc) -> void;

public:
    // The DamageResolution one-liner — see FCk_Request_Aggro_AddThreat.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Request Add Threat",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Aggro
    Request_AddThreat(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        FCk_Request_Aggro_AddThreat InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Request Remove Target",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Aggro
    Request_RemoveTarget(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        const FCk_Handle& InTrackedEntity,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Request Clear All Targets",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Aggro
    Request_ClearAllTargets(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Request Set Active Target",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Aggro
    Request_SetActiveTarget(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        const FCk_Handle& InTrackedEntity,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Request Clear Active Target",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Aggro
    Request_ClearActiveTarget(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Request Enable/Disable",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Aggro
    Request_EnableDisable(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        ECk_EnableDisable InEnableDisable,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Convenience for perception feeders that know the tracked entity, not the child target.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Request Mark Perceived By Tracked Entity",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Aggro
    Request_MarkPerceived_ByTrackedEntity(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        const FCk_Handle& InTrackedEntity,
        FCk_Request_AggroTarget_MarkPerceived InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Perceived is a COUNTED tag, so a feeder that can only mark perceived pins its targets
    // perceived forever — they never decay at the unperceived rate nor reach the lost-sight
    // grace. A feeder addressing targets by tracked entity needs the balancing half here.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Request Mark Unperceived By Tracked Entity")
    static FCk_Handle_Aggro
    Request_MarkUnperceived_ByTrackedEntity(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        const FCk_Handle& InTrackedEntity);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Aggro",
              DisplayName="[Ck][Aggro] Request Reset Perception By Tracked Entity")
    static FCk_Handle_Aggro
    Request_ResetPerception_ByTrackedEntity(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        const FCk_Handle& InTrackedEntity);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Aggro",
        DisplayName = "[Ck][Aggro] Bind To OnTargetAcquired")
    static FCk_Handle_Aggro
    BindTo_OnTargetAcquired(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Aggro_OnTargetAcquired& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Aggro",
        DisplayName = "[Ck][Aggro] Unbind From OnTargetAcquired")
    static FCk_Handle_Aggro
    UnbindFrom_OnTargetAcquired(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Aggro_OnTargetAcquired& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Aggro",
        DisplayName = "[Ck][Aggro] Bind To OnActiveTargetChanged")
    static FCk_Handle_Aggro
    BindTo_OnActiveTargetChanged(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Aggro_OnActiveTargetChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Aggro",
        DisplayName = "[Ck][Aggro] Unbind From OnActiveTargetChanged")
    static FCk_Handle_Aggro
    UnbindFrom_OnActiveTargetChanged(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Aggro_OnActiveTargetChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Aggro",
        DisplayName = "[Ck][Aggro] Bind To OnTargetForgotten")
    static FCk_Handle_Aggro
    BindTo_OnTargetForgotten(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Aggro_OnTargetForgotten& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Aggro",
        DisplayName = "[Ck][Aggro] Unbind From OnTargetForgotten")
    static FCk_Handle_Aggro
    UnbindFrom_OnTargetForgotten(
        UPARAM(ref) FCk_Handle_Aggro& InAggro,
        const FCk_Delegate_Aggro_OnTargetForgotten& InDelegate);

public:
    // ---- Pure scoring + analytic-decay model ----
    // Primitive-in / primitive-out — no registry access, so these are safe to call from the parallel Evaluate worker
    // body, and are defined inline below so the hot per-target path keeps its inlining. Model of record: CkAggro/CLAUDE.md.

    static auto
    Compute_PerceptionDecayMultiplier(bool InIsPerceived, double InSecondsSinceLastPerceived, double InLostSightGraceSeconds, float InUnperceivedMultiplier) -> float;

    static auto
    Compute_RangeDecayMultiplier(bool InIsWithinRetention, float InOutOfRangeMultiplier) -> float;

    static auto
    Compute_DecayedThreat(float InCurrentThreat, double InElapsedSeconds, double InDecayRatePerSecond, float InPerceptionMultiplier, float InRangeMultiplier, double InClampMin, double InClampMax) -> float;

    static auto
    Compute_DistanceFactor(double InDistance, double InHalfDistance, double InExponent) -> double;

    static auto
    Compute_NearbyFactor(bool InNearbyEnabled, double InDistance, double InNearbyDistance, double InNearbyMultiplier) -> double;

    static auto
    Compute_Score(float InThreat, double InDistanceFactor, double InNearbyFactor, double InScoreMultiplier, double InScoreBias) -> double;

    static auto
    Is_EligibleTarget(bool InCannotBecomeActive, bool InPendingForget, bool InTrackedIsValid, double InScore, double InMinimumScore, bool InIsWithinRetention, double InDistance, double InAcquisitionDistance) -> bool;

    static auto
    Should_SwitchTarget(double InChallengerScore, double InIncumbentScore, double InCurrentTargetBias, double InSwitchThreshold, double InSecondsSinceLastSwitch, double InSwitchCooldownSeconds, double InSecondsSinceActiveStart, double InMinimumAggroDurationSeconds) -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

inline auto
    UCk_Utils_Aggro_UE::
    Compute_PerceptionDecayMultiplier(bool InIsPerceived, double InSecondsSinceLastPerceived, double InLostSightGraceSeconds, float InUnperceivedMultiplier)
    -> float
{
    const auto WithinGrace = InSecondsSinceLastPerceived <= InLostSightGraceSeconds;
    return (InIsPerceived || WithinGrace) ? 1.0f : InUnperceivedMultiplier;
}

inline auto
    UCk_Utils_Aggro_UE::
    Compute_RangeDecayMultiplier(bool InIsWithinRetention, float InOutOfRangeMultiplier)
    -> float
{
    return InIsWithinRetention ? 1.0f : InOutOfRangeMultiplier;
}

inline auto
    UCk_Utils_Aggro_UE::
    Compute_DecayedThreat(float InCurrentThreat, double InElapsedSeconds, double InDecayRatePerSecond, float InPerceptionMultiplier, float InRangeMultiplier, double InClampMin, double InClampMax)
    -> float
{
    const auto Loss    = InDecayRatePerSecond * InPerceptionMultiplier * InRangeMultiplier * FMath::Max(InElapsedSeconds, 0.0);
    const auto Decayed = static_cast<double>(InCurrentThreat) - Loss;
    return static_cast<float>(FMath::Clamp(Decayed, InClampMin, InClampMax));
}

inline auto
    UCk_Utils_Aggro_UE::
    Compute_DistanceFactor(double InDistance, double InHalfDistance, double InExponent)
    -> double
{
    // Continuous falloff in [0, 1]: 1 at zero distance, 0.5 at the half-distance, -> 0 far out.
    if (InHalfDistance <= 0.0)
    { return 1.0; }

    const auto Ratio = FMath::Max(InDistance, 0.0) / InHalfDistance;
    return 1.0 / (1.0 + FMath::Pow(Ratio, InExponent));
}

inline auto
    UCk_Utils_Aggro_UE::
    Compute_NearbyFactor(bool InNearbyEnabled, double InDistance, double InNearbyDistance, double InNearbyMultiplier)
    -> double
{
    const auto InBand = InNearbyEnabled && InDistance <= InNearbyDistance;
    return InBand ? InNearbyMultiplier : 1.0;
}

inline auto
    UCk_Utils_Aggro_UE::
    Compute_Score(float InThreat, double InDistanceFactor, double InNearbyFactor, double InScoreMultiplier, double InScoreBias)
    -> double
{
    // Raw — no incumbent bias baked in; selection applies that.
    return static_cast<double>(InThreat) * InDistanceFactor * InNearbyFactor * InScoreMultiplier + InScoreBias;
}

inline auto
    UCk_Utils_Aggro_UE::
    Is_EligibleTarget(bool InCannotBecomeActive, bool InPendingForget, bool InTrackedIsValid, double InScore, double InMinimumScore, bool InIsWithinRetention, double InDistance, double InAcquisitionDistance)
    -> bool
{
    if (InCannotBecomeActive || InPendingForget || (NOT InTrackedIsValid))
    { return false; }

    if (InScore < InMinimumScore)
    { return false; }

    // Spatial hysteresis: already-retained targets stay eligible; new ones must be inside acquisition.
    return InIsWithinRetention || InDistance <= InAcquisitionDistance;
}

inline auto
    UCk_Utils_Aggro_UE::
    Should_SwitchTarget(double InChallengerScore, double InIncumbentScore, double InCurrentTargetBias, double InSwitchThreshold, double InSecondsSinceLastSwitch, double InSwitchCooldownSeconds, double InSecondsSinceActiveStart, double InMinimumAggroDurationSeconds)
    -> bool
{
    const auto ScoreGate    = InChallengerScore > InIncumbentScore * InCurrentTargetBias * InSwitchThreshold;
    const auto CooldownGate = InSecondsSinceLastSwitch >= InSwitchCooldownSeconds;
    const auto DurationGate = InSecondsSinceActiveStart >= InMinimumAggroDurationSeconds;
    return ScoreGate && CooldownGate && DurationGate;
}

// --------------------------------------------------------------------------------------------------------------------
