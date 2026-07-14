#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkCrowd_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Algorithm-mode enums for the Phase 1 + Phase 2 hybrid avoidance system. Every mode is an enum
// (never a bool) so the per-game-mode customisation story stays consistent and extensible.
// See Gate_03_Separation_Addendum.md for design rationale and dtCrowd / Ant citations.
// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AccelClampMode : uint8
{
    Enabled,    // velocity-delta clamp active (default — kills snap-flips)
    Disabled,   // for A/B comparison and edge-case opt-out
};

UENUM(BlueprintType)
enum class ECk_AvoidanceSidePreference : uint8
{
    Disabled,   // skip wSide computation entirely (saves a cross + dot per sample × neighbor)
    PassLeft,   // wSide active, agents prefer to pass neighbors on the left (dtCrowd default)
    PassRight,  // wSide active, opposite sign — for mirrored / region-specific conventions
};

UENUM(BlueprintType)
enum class ECk_AvoidanceSampleTrigger : uint8
{
    Disabled,                // never sample — force solver only
    NeighborCountOnly,       // numeric: NeighborCache.Num() >= threshold
    ZoneTagOnly,             // designer-tagged zones / agents only
    NeighborCountAndZoneTag, // either condition fires (default)
};

UENUM(BlueprintType)
enum class ECk_PushApartMode : uint8
{
    Disabled,    // no post-hoc resolution
    Single,      // 1 iteration — cheapest, ~80% as effective as Standard
    Standard,    // 4 iterations (dtCrowd default; resolves cascaded interactions in one frame)
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Crowd"))
class CKCROWD_API UCk_Crowd_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Crowd_ProjectSettings_UE);

private:
    // ---- AccelClamp (Phase 1.2) ----
    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|AccelClamp",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Velocity-delta clamp on FFragment_CrowdAgent_DesiredVelocity output. Disabling reverts to the Gate-2 scalar clamp; for A/B testing only — production should leave Enabled."))
    ECk_AccelClampMode _AccelClampMode = ECk_AccelClampMode::Enabled;

    // ---- Sampling Avoidance (Phase 2) ----
    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling",
        meta = (AllowPrivateAccess = true,
            ToolTip = "What gates the sampling override running for an agent. Tag-based modes consult TAG_CrowdAvoidance_AlwaysSample / NeverSample on the agent or its lifetime owner."))
    ECk_AvoidanceSampleTrigger _AvoidanceSampleTrigger = ECk_AvoidanceSampleTrigger::NeighborCountAndZoneTag;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling",
        meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1, ClampMax = 12, UIMax = 12,
            ToolTip = "Minimum NeighborCache size that triggers sampling under NeighborCountOnly / NeighborCountAndZoneTag. Default 1 means: as soon as an agent has any neighbor at all, use the predictive sampler. Force-only behaviour at higher thresholds is reactive (kicks in within _SeparationRadius cm) and tends to look snap-and-touch rather than smooth-arc; the sampler's perf cost is negligible at our 130-agent target so threshold>=2 is rarely worth it."))
    int32 _AvoidanceSampleNeighborThreshold = 1;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling",
        meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1, ClampMax = 8, UIMax = 8,
            ToolTip = "Round-robin stride. 1=every triggered agent every frame (default — best correctness). >=2 saves perf at the cost of vibration: on off-frames Steering's path-follow velocity overwrites the sampler's choice and AccelClamp drags the velocity back, undoing lateral deflection. Use >=2 only if you've added persistent-cache plumbing or perf is provably the bottleneck."))
    int32 _AvoidanceSampleStride = 1;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling",
        meta = (AllowPrivateAccess = true, ClampMin = 4, UIMin = 4, ClampMax = 16, UIMax = 16))
    int32 _AvoidanceSampleAngularDivs = 8;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling",
        meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1, ClampMax = 4, UIMax = 4))
    int32 _AvoidanceSampleRings = 2;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0, ClampMax = 0.9, UIMax = 0.9,
            ToolTip = "Velocity bias: the sample cloud is centred on DesiredVelocity * Bias with ring radius MaxSpeed * (1 - Bias), mirroring dtCrowd (DetourObstacleAvoidance.cpp:570-572). Detour ships 0.5 at every quality level (CrowdManager.cpp:182-208), which makes the most conservative candidate half-speed-FORWARD rather than a dead stop. 0 centres the cloud on the origin, putting a full stop at the centre of the search — that is freeze-prone, because a stopped pair predicts no collision and so pays no time-to-impact penalty. Values near 1 stop exploring alternatives."))
    float _AvoidanceVelBias = 0.5f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Side-preference behaviour. Disabled skips the wSide cross product entirely. PassLeft mirrors dtCrowd's default convention."))
    ECk_AvoidanceSidePreference _AvoidanceSidePreference = ECk_AvoidanceSidePreference::PassLeft;

    // Penalty weights — see DetourObstacleAvoidance.cpp:471-475 for the dtCrowd defaults we mirror.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling|Penalty",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0))
    float _AvoidanceWeightDesVel = 2.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling|Penalty",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0))
    float _AvoidanceWeightCurVel = 0.75f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling|Penalty",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0))
    float _AvoidanceWeightSide = 0.75f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling|Penalty",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0))
    float _AvoidanceWeightToi = 2.5f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling|Penalty",
        meta = (AllowPrivateAccess = true, ClampMin = 0.1, UIMin = 0.1, ClampMax = 10.0, UIMax = 10.0,
            ToolTip = "Time horizon (seconds) for the time-to-collision penalty. Mirrors dtCrowd's horizTime default."))
    float _AvoidanceHorizonTime = 2.5f;

    // ---- Push-Apart (Phase 2) ----
    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|PushApart",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Post-integration physical resolution of overlapping agents. Standard = 4 iterations per dtCrowd. Disabled allows brief overlap during sampling latency."))
    ECk_PushApartMode _PushApartMode = ECk_PushApartMode::Standard;

public:
    CK_PROPERTY_GET(_AccelClampMode);
    CK_PROPERTY_GET(_AvoidanceSampleTrigger);
    CK_PROPERTY_GET(_AvoidanceSampleNeighborThreshold);
    CK_PROPERTY_GET(_AvoidanceSampleStride);
    CK_PROPERTY_GET(_AvoidanceSampleAngularDivs);
    CK_PROPERTY_GET(_AvoidanceSampleRings);
    CK_PROPERTY_GET(_AvoidanceVelBias);
    CK_PROPERTY_GET(_AvoidanceSidePreference);
    CK_PROPERTY_GET(_AvoidanceWeightDesVel);
    CK_PROPERTY_GET(_AvoidanceWeightCurVel);
    CK_PROPERTY_GET(_AvoidanceWeightSide);
    CK_PROPERTY_GET(_AvoidanceWeightToi);
    CK_PROPERTY_GET(_AvoidanceHorizonTime);
    CK_PROPERTY_GET(_PushApartMode);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCROWD_API UCk_Utils_Crowd_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Crowd_Settings_UE);

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_AccelClampMode Get_AccelClampMode();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_AvoidanceSampleTrigger Get_AvoidanceSampleTrigger();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static int32 Get_AvoidanceSampleNeighborThreshold();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static int32 Get_AvoidanceSampleStride();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_PushApartMode Get_PushApartMode();

    // Internal C++ accessor avoiding repeated GetMutableDefault calls in hot paths.
    static const UCk_Crowd_ProjectSettings_UE* Get();
};

// --------------------------------------------------------------------------------------------------------------------
