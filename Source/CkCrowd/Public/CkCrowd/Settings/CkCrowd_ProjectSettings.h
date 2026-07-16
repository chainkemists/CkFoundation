#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkCrowd_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Algorithm-mode enums for the hybrid (force + sampling) avoidance system. Every mode is an enum
// (never a bool) so the per-game-mode customisation story stays consistent and extensible.
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

UENUM(BlueprintType)
enum class ECk_CrowdBlockDetectionMode : uint8
{
    Disabled,    // an agent that cannot reach its goal presses forever; nothing notices
    Enabled,     // detect an unreachable goal, stop, and report OnGoalBlocked
};

UENUM(BlueprintType)
enum class ECk_CrowdNavmeshConstraintMode : uint8
{
    Disabled,    // agents integrate in free space; separation/avoidance/push-apart can displace them off the navmesh
    Enabled,     // every per-frame displacement is walked along the navmesh surface (dtCrowd's corridor movePosition)
};

UENUM(BlueprintType)
enum class ECk_CrowdStationaryMarkupMode : uint8
{
    Disabled,    // pathfinding is blind to standing agents; paths go straight through crowds
    Enabled,     // stationary agents paint a cost area so fresh paths route AROUND standing crowds
};

UENUM(BlueprintType)
enum class ECk_CrowdPathRefreshMode : uint8
{
    Disabled,    // a path computed before a crowd formed is followed forever — the agent presses into standing agents
    Enabled,     // a walking agent whose remaining path crosses a freshly painted cost disc re-paths and detours
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Crowd"))
class CKCROWD_API UCk_Crowd_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Crowd_ProjectSettings_UE);

private:
    // ---- AccelClamp ----
    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|AccelClamp",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Velocity-delta clamp on FFragment_CrowdAgent_DesiredVelocity output. Disabling reverts to the scalar clamp; for A/B testing only — production should leave Enabled."))
    ECk_AccelClampMode _AccelClampMode = ECk_AccelClampMode::Enabled;

    // ---- Sampling Avoidance ----
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

    // ---- Block detection ----
    // The tier CkCrowd was missing entirely: noticing that an agent cannot reach its goal. Without it
    // an agent grinds against an obstruction forever and nothing in the system is aware. Stock UE puts
    // this ABOVE the solver (UPathFollowingComponent block detection => abort the move => behaviour
    // tree decides); we put it in the module so gameplay gets a signal instead of a frozen NPC.
    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Master switch for goal-blocked detection. Disabled restores the old behaviour: an agent that cannot reach its goal presses against the obstruction indefinitely and nothing notices."))
    ECk_CrowdBlockDetectionMode _BlockDetectionMode = ECk_CrowdBlockDetectionMode::Enabled;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 0.1, UIMin = 0.1,
            ToolTip = "Seconds between position samples for the no-progress detector. Mirrors UE's BlockDetectionInterval (0.5s)."))
    float _BlockDetectionInterval = 0.5f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 3, UIMin = 3, ClampMax = 10, UIMax = 10,
            ToolTip = "How many samples must all sit within BlockDetectionDistance of their centroid before the agent counts as going nowhere. 6 samples x 0.5s = a 3s window. UE uses 10 (5s); a crowd-tier detector can afford to be quicker."))
    int32 _BlockDetectionSampleCount = 6;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 1.0, UIMin = 1.0,
            ToolTip = "Radius (cm) around the sample centroid within which every sample must fall for the agent to count as making no progress. Mirrors UE's BlockDetectionDistance (10cm)."))
    float _BlockDetectionDistance = 15.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0,
            ToolTip = "Speed (cm/s) below which a neighbour counts as STATIONARY for the occupied-goal test. A neighbour merely passing through the goal is not blocking it — only one that has settled there is."))
    float _BlockedStationarySpeedThreshold = 10.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 0.1, UIMin = 0.1,
            ToolTip = "Seconds between re-checks for a HoldAndRetry agent waiting on a blocked goal. When the goal clears it re-paths and resumes."))
    float _BlockedRecheckInterval = 1.0f;

    // ---- Navmesh constraint ----
    // The stage the dtCrowd port originally dropped: Detour passes every integrated agent position
    // through dtPathCorridor::movePosition (moveAlongSurface), so a dtCrowd agent CANNOT leave the
    // navmesh — walls stop it and it slides. Without this, any lateral force (separation redirect,
    // avoidance velocity, push-apart shove) moves the transform straight through a navmesh boundary.
    UPROPERTY(Config, EditDefaultsOnly, Category = "NavmeshConstraint",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Master switch for the navmesh movement constraint. Enabled walks every per-frame agent displacement along the navmesh surface (dtCrowd movePosition semantics) so no force can push an agent off the mesh. Disabled restores free-space integration — for A/B comparison only. Worlds without nav data are unaffected either way."))
    ECk_CrowdNavmeshConstraintMode _NavmeshConstraintMode = ECk_CrowdNavmeshConstraintMode::Enabled;

    // ---- Stationary markup ----
    // The path planner only sees static geometry: without this, a path for an agent headed past a
    // standing crowd goes straight THROUGH it, and the local avoidance sampler (short-horizon,
    // greedy) cannot escape a line-shaped local minimum — the agent presses into the crowd
    // forever. Stationary agents therefore paint a UCk_NavArea_CrowdAgent cost disc so fresh
    // paths (including BlockedRecheck re-paths) genuinely route around.
    UPROPERTY(Config, EditDefaultsOnly, Category = "StationaryMarkup",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Master switch for stationary-agent nav markup. Enabled paints a cost area under agents that have held still past the delay, so pathfinding detours around standing crowds when a detour exists (a fully-plugged corridor still paths through — cost, not exclusion). Disabled restores agent-blind pathfinding."))
    ECk_CrowdStationaryMarkupMode _StationaryMarkupMode = ECk_CrowdStationaryMarkupMode::Enabled;

    UPROPERTY(Config, EditDefaultsOnly, Category = "StationaryMarkup",
        meta = (AllowPrivateAccess = true, ClampMin = 0.1, UIMin = 0.1,
            ToolTip = "Seconds an agent must be PHYSICALLY stationary (windowed displacement, regardless of Idle/Walking state — a blocked walker plugs a corridor like anyone standing) before its cost disc is painted. Hysteresis against paint/unpaint churn from brief stops; the disc is removed the moment the agent genuinely moves again."))
    float _StationaryMarkupDelaySeconds = 1.5f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "StationaryMarkup",
        meta = (AllowPrivateAccess = true, ClampMin = 1.0, UIMin = 1.0, ClampMax = 4.0, UIMax = 4.0,
            ToolTip = "Painted half-extent as a multiple of the agent radius. Sized so neighbouring discs in a queue OVERLAP DEEPLY: the planner crosses a crowd at the cheapest seam between two agents, and shallow overlap (1.5x at 120uu queue pitch) made threading the seam cheaper than detouring around the line. 2x makes the seam ~120uu of marked ground, so 'around' wins at store scale."))
    float _StationaryMarkupExtentMultiplier = 2.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "StationaryMarkup",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Master switch for mid-walk path refresh. StationaryMarkup only bends paths computed AFTER a disc is painted; this re-paths a WALKING agent whose remaining path crosses a disc newer than its path, so already-issued moves detour around a crowd that formed after they planned. No-op while StationaryMarkup is Disabled (no discs exist)."))
    ECk_CrowdPathRefreshMode _PathRefreshMode = ECk_CrowdPathRefreshMode::Enabled;

    UPROPERTY(Config, EditDefaultsOnly, Category = "StationaryMarkup",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0,
            ToolTip = "Seconds after a disc is painted before PathRefresh even checks it against the navmesh. A cheap pre-filter only — actual eligibility is ground truth (the rebuilt mesh must report the cost area at the disc's location), so this just keeps the poly query off discs that cannot possibly have rebaked yet."))
    float _PathRefreshMarkupSettleSeconds = 0.5f;

    // ---- Push-Apart ----
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
    CK_PROPERTY_GET(_NavmeshConstraintMode);
    CK_PROPERTY_GET(_StationaryMarkupMode);
    CK_PROPERTY_GET(_StationaryMarkupDelaySeconds);
    CK_PROPERTY_GET(_StationaryMarkupExtentMultiplier);
    CK_PROPERTY_GET(_PathRefreshMode);
    CK_PROPERTY_GET(_PathRefreshMarkupSettleSeconds);
    CK_PROPERTY_GET(_BlockDetectionMode);
    CK_PROPERTY_GET(_BlockDetectionInterval);
    CK_PROPERTY_GET(_BlockDetectionSampleCount);
    CK_PROPERTY_GET(_BlockDetectionDistance);
    CK_PROPERTY_GET(_BlockedStationarySpeedThreshold);
    CK_PROPERTY_GET(_BlockedRecheckInterval);
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

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_CrowdNavmeshConstraintMode Get_NavmeshConstraintMode();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_CrowdStationaryMarkupMode Get_StationaryMarkupMode();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_CrowdPathRefreshMode Get_PathRefreshMode();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_CrowdBlockDetectionMode Get_BlockDetectionMode();

    // Internal C++ accessor avoiding repeated GetMutableDefault calls in hot paths.
    static const UCk_Crowd_ProjectSettings_UE* Get();
};

// --------------------------------------------------------------------------------------------------------------------
