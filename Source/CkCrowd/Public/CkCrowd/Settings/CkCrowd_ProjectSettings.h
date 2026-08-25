#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkCrowd_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Algorithm-mode enums for the hybrid (force + sampling) avoidance system.
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
    Disabled,   // skip the neighbor-relative wSide computation entirely
    PassLeft,   // wSide favors candidates left of each neighbor-relative offset; not a Detour port
    PassRight,  // wSide mirrors PassLeft for mirrored / region-specific conventions
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
enum class ECk_AvoidanceWallSegmentsMode : uint8
{
    Disabled,   // candidates are scored against neighbouring AGENTS only
    Enabled,    // navmesh boundary walls are obstacles too (dtCrowd's dtLocalBoundary)
};

// --------------------------------------------------------------------------------------------------------------------

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
enum class ECk_CrowdCrowdedGoalBlockMode : uint8
{
    Disabled,    // only an agent in contact with the goal ITSELF ever blocks; everyone behind it presses inward
    Enabled,     // contact with a settled neighbour parked on the agent's own goal region blocks too, so settling propagates outward
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

UENUM(BlueprintType)
enum class ECk_CrowdCorridorStandDownMode : uint8
{
    Disabled,    // the sampler keeps scoring inside tight corridors; opposing walls make the winner oscillate and the agent jitters through gaps it geometrically fits
    Enabled,     // between opposing walls closer than the agent needs to also avoid them, the sampler stands down and path-follow + the navmesh clamp carry the agent through
};

UENUM(BlueprintType)
enum class ECk_CrowdPlanAroundStandingCrowdsMode : uint8
{
    Disabled,    // single-phase toll planning: for a close destination behind a crowd, crossing the toll beats any detour and the agent presses into bodies it cannot pass
    Enabled,     // plan strict first (markup impassable — always detour around standing crowds); fall back to the toll filter once when no crowd-free route exists
};

UENUM(BlueprintType)
enum class ECk_CrowdWaypointRetirementLineOfSightMode : uint8
{
    Disabled,    // a corner retires on proximity/plane alone, blind to what the chord to the next waypoint crosses
    Enabled,     // a corner is given up only once the chord from the agent to the next waypoint is navigable
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
        meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1, ClampMax = 8, UIMax = 8,
            ToolTip = "Adaptive sampling depth. Depth 1 evaluates the base cloud only; later depths recenter on the prior winner and halve the search radius. Detour's current default is 5."))
    int32 _AvoidanceSampleDepth = 5;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0, ClampMax = 0.9, UIMax = 0.9,
            ToolTip = "Velocity bias: the sample cloud is centred on DesiredVelocity * Bias with ring radius MaxSpeed * (1 - Bias), mirroring dtCrowd (DetourObstacleAvoidance.cpp:570-572). Detour ships 0.5 at every quality level (CrowdManager.cpp:182-208), which makes the most conservative candidate half-speed-FORWARD rather than a dead stop. 0 centres the cloud on the origin, putting a full stop at the centre of the search — that is freeze-prone, because a stopped pair predicts no collision and so pays no time-to-impact penalty. Values near 1 stop exploring alternatives."))
    float _AvoidanceVelBias = 0.5f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Neighbor-relative side-preference behaviour. Disabled skips wSide. PassLeft favors candidates left of each neighbor-relative offset; PassRight mirrors it. This is not claimed as Detour parity."))
    ECk_AvoidanceSidePreference _AvoidanceSidePreference = ECk_AvoidanceSidePreference::PassLeft;

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

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0,
            ToolTip = "Extra distance (cm) beyond the arrival radius within which an agent on its FINAL path leg stops running the velocity-obstacle sampler and simply follows Steering's path-follow velocity. It exists to break a hard lock-out: ringed by neighbours at contact distance, every inward candidate closes on a near-overlapping body, the overlap branch of the time-to-collision term floods the collision penalty across the whole sample cloud, and the sampler parks the agent at a standoff WIDER than its arrival radius, where it hovers indefinitely without ever arriving. Trades away predictive avoidance for the last (ArrivalRadius + this) of an approach; separation and push-apart still run, so contact in the envelope is resolved by de-penetration, which is the right authority that close to a destination. 0 disables the suppression. An explicit per-agent override (SamplingAlways policy, or the AlwaysSample tag) outranks it."))
    float _AvoidanceFinalApproachSuppressionCm = 90.0f;

    // ---- Sampling Avoidance | Walls ----
    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling|Walls",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Master switch for scoring candidate velocities against navmesh boundary walls, which dtCrowd feeds its obstacle query from a per-agent dtLocalBoundary cache (DetourCrowd.cpp:1557-1564). Disabled restores agent-only scoring: with neighbours pressing, the cheapest candidate is routinely one aimed straight at a wall or a UNavArea_Null fixture hole, the navmesh constraint then eats the whole displacement, and the agent walks on the spot. For A/B comparison only - production leaves this Enabled."))
    ECk_AvoidanceWallSegmentsMode _AvoidanceWallSegments = ECk_AvoidanceWallSegmentsMode::Enabled;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling|Walls",
        meta = (AllowPrivateAccess = true, ClampMin = 1.0, UIMin = 1.0, ClampMax = 32.0, UIMax = 32.0,
            ToolTip = "Wall query range as a multiple of the agent radius. Mirrors dtCrowd's collisionQueryRange, whose default is radius * 12 (CrowdFollowingComponent.cpp:43). It sizes both the navmesh wall search and the cache refresh distance, which is a quarter of it (DetourCrowd.cpp:1284), so raising it widens the search AND refreshes less often."))
    float _AvoidanceWallQueryRangeMultiplier = 12.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling|Walls",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Stand the sampler down between OPPOSING navmesh walls whose corridor width is under (2 x agent radius + CorridorStandDownSlackCm). The navmesh bakes such a corridor walkable — the agent geometrically fits — but the sampler scores both walls as obstacles to keep a margin from, penalising inward candidates from both sides at once; with any neighbour pressure the winner oscillates and the agent jitters through a gap it could simply walk. Standing down hands the corridor to path-follow + the navmesh clamp, the authorities that cannot leave the mesh anyway. Separation (lateral-clamped) and push-apart still run. The explicit per-agent overrides (SamplingAlways policy, AlwaysSample tag) outrank it, and it only engages when wall segments are Enabled — it reads the same cached local boundary."))
    ECk_CrowdCorridorStandDownMode _CorridorStandDown = ECk_CrowdCorridorStandDownMode::Enabled;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling|Walls",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0, ClampMax = 200.0, UIMax = 200.0,
            ToolTip = "Slack (cm) added to the agent's diameter to decide what counts as a TIGHT corridor for the stand-down above. At the default 40 a standard 42cm-radius agent stands down in corridors narrower than 124cm — wide enough to fit, too narrow to also keep an avoidance margin from both walls."))
    float _CorridorStandDownSlackCm = 40.0f;

    // ---- Block detection ----
    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Master switch for goal-blocked detection. Disabled restores the old behaviour: an agent that cannot reach its goal presses against the obstruction indefinitely and nothing notices."))
    ECk_CrowdBlockDetectionMode _BlockDetectionMode = ECk_CrowdBlockDetectionMode::Enabled;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 0.1, UIMin = 0.1,
            ToolTip = "Seconds between progress samples for the no-progress detector. Mirrors UE's BlockDetectionInterval (0.5s)."))
    float _BlockDetectionInterval = 0.5f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 0.5, UIMin = 0.5,
            ToolTip = "Seconds the agent may go without improving its best REMAINING path distance before it counts as stalled. Replaces the feet-sample centroid ring, which an agent sliding laterally along a wall (exactly what the navmesh constraint's surface walk produces) evaded forever while its velocity stayed nonzero. Progress along the path cannot be faked by sliding, and the same measure also catches corner orbiting."))
    float _BlockDetectionNoProgressWindowSeconds = 3.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "PathEpisode",
        meta = (AllowPrivateAccess = true, ClampMin = 1.0, UIMin = 1.0,
            ToolTip = "Seconds a path episode may sit Pending before it is failed with PendingTimeout. The two legitimate long waits are both shorter and bounded: CkNavigation force-fails its deferral queue at ck.Nav.MaxDeferralSeconds (5s), and CkPathNetwork re-enqueues a route in the same breath as it re-parks one. CkPathNetwork and CkVoxelNav carry no timeout of their own, so without this a provider that never answers wedges the agent silently and forever."))
    float _PathPendingTimeoutSeconds = 10.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 1.0, UIMin = 1.0,
            ToolTip = "How much (cm) the best remaining path distance must improve within the no-progress window to count as progress. Too small and push-apart jitter reads as forward motion; too large and a legitimately slow agent (heavy crowd, tight corridor) is declared stalled."))
    float _BlockDetectionProgressEpsilonCm = 30.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0, ClampMax = 8, UIMax = 8,
            ToolTip = "Internal re-paths a stalled agent may spend before the stall is escalated to a block. The frozen Recast polyline is the usual culprit — the world changed after the path was planned — so re-planning at the same goal is the cheap fix. 0 escalates on the first stall."))
    int32 _BlockDetectionMaxStallRepaths = 2;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0,
            ToolTip = "How far (cm, XY) the agent may drift from its current path segment before it is re-pathed. Heals teleports, saves restores, and external shoves, which otherwise leave steering chasing a waypoint the agent can no longer reach along the installed corridor. 0 disables the check."))
    float _BlockDetectionOffPathRepathThresholdCm = 300.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0,
            ToolTip = "Speed (cm/s) below which a neighbour counts as STATIONARY for the occupied-goal test. A neighbour merely passing through the goal is not blocking it — only one that has settled there is."))
    float _BlockedStationarySpeedThreshold = 10.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 0.1, UIMin = 0.1,
            ToolTip = "Seconds between re-checks for a HoldAndRetry agent waiting on a blocked goal. When the goal clears it re-paths and resumes."))
    float _BlockedRecheckInterval = 1.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0, ClampMax = 20, UIMax = 20,
            ToolTip = "How many HoldAndRetry re-check cycles a NoProgress block may spend re-pathing before the move is failed outright. Applies ONLY to NoProgress: a goal occupied by another AGENT keeps the unbounded hold (a queue wait is not a failure), while static geometry never clears and retrying it forever is a silent hang no caller can observe."))
    int32 _BlockedMaxRetries = 3;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Master switch for cluster-aware goal-blocked detection. The occupied-goal test only ever answers for the agent in contact with the GOAL, so in a crowd commanded to one point only the innermost ring learns anything and everyone behind it presses inward until the slow no-progress ladder stops them one by one. Enabled also blocks a walking agent that is in contact with a SETTLED neighbour parked inside its own goal region, standing nearer the goal, and standing in its way (a +/-60 degree cone around the direction of travel). Why the neighbour stopped there is not asked, so a stranger obstructs exactly as much as a rival for the same destination. Disabled restores the occupied-goal-only behaviour, for A/B comparison only."))
    ECk_CrowdCrowdedGoalBlockMode _CrowdedGoalBlockMode = ECk_CrowdCrowdedGoalBlockMode::Enabled;

    UPROPERTY(Config, EditDefaultsOnly, Category = "BlockDetection",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0,
            ToolTip = "Slack (cm) that widens BOTH cluster-blocking distance tests: the radius sum a pair must be within to count as being in CONTACT, and the region around the agent's own goal a settled neighbour must be parked inside to count as standing on the destination. Trades settle latency and reach against false stops: push-apart holds a settled pair at exactly the radius sum, so zero pad leaves the contact test no margin and the 0.5s sampling cadence can miss the frame outright; too large and agents stop with a visible gap, and strangers parked further and further from the destination start halting passers-by."))
    float _CrowdedGoalContactPadCm = 10.0f;

    // ---- Navmesh constraint ----
    UPROPERTY(Config, EditDefaultsOnly, Category = "NavmeshConstraint",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Master switch for the navmesh movement constraint. Enabled walks every per-frame agent displacement along the navmesh surface (dtCrowd movePosition semantics) so no force can push an agent off the mesh. Disabled restores free-space integration — for A/B comparison only. Worlds without nav data are unaffected either way."))
    ECk_CrowdNavmeshConstraintMode _NavmeshConstraintMode = ECk_CrowdNavmeshConstraintMode::Enabled;

    // The grounding lease. The constraint's grounding used to run only when the frame staged
    // displacement, which meant a STATIONARY agent's Z was never reconciled — for years the only
    // reason settled agents stayed grounded was the push-apart solver's never-terminating
    // sub-millimetre corrections keeping the constraint alive by accident. _PushApartSlopCm ended
    // that, and stationary agents froze at whatever Z they had (the floating-NPC regression:
    // permanent NoRouteFound for the rest of the session). The lease is the deliberate
    // replacement: cost is roughly StationaryPopulation / (Interval x FPS) navmesh projections
    // per frame — at 300 stationary agents and 1s, ~5/frame, versus the 300/frame the old
    // accident silently paid.
    UPROPERTY(Config, EditDefaultsOnly, Category = "NavmeshConstraint",
        meta = (AllowPrivateAccess = true, ClampMin = "0.0",
            ToolTip = "Max seconds a stationary (zero-displacement) agent may go without being reconciled against the navmesh. Every ordinary displacing frame already reconciles and resets the clock, so only resting agents pay the lease — one navmesh projection each, once per interval, phase-spread across agents. The idle correction is Z-only past the dead-band, so a settled formation cannot creep. 0 disables the idle verify entirely, restoring the pre-lease behaviour where a stationary agent's elevation error is permanent — for A/B comparison only."))
    float _GroundingVerifyIntervalSeconds = 1.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "NavmeshConstraint",
        meta = (AllowPrivateAccess = true, ClampMin = "0.0",
            ToolTip = "Vertical drift below this (cm) is not corrected by an idle grounding verify, so a correctly-grounded resting crowd issues ZERO transform requests. Keep it above the navmesh projection's own jitter and below anything a player could see."))
    float _GroundingVerifyMinCorrectionCm = 0.5f;

    // ---- Stationary markup ----
    UPROPERTY(Config, EditDefaultsOnly, Category = "StationaryMarkup",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Master switch for stationary-agent nav markup. Enabled paints a cost area under agents that have held still past the delay, so pathfinding detours around standing crowds when a detour exists (a fully-plugged corridor still paths through — cost, not exclusion). Disabled restores agent-blind pathfinding."))
    ECk_CrowdStationaryMarkupMode _StationaryMarkupMode = ECk_CrowdStationaryMarkupMode::Enabled;

    UPROPERTY(Config, EditDefaultsOnly, Category = "StationaryMarkup",
        meta = (AllowPrivateAccess = true, ClampMin = 0.1, UIMin = 0.1,
            ToolTip = "Seconds an agent must be PHYSICALLY stationary (windowed displacement, regardless of Idle/Walking state — a blocked walker plugs a corridor like anyone standing) before its cost disc is painted. Hysteresis against paint/unpaint churn from brief stops; the disc is removed the moment the agent genuinely moves again."))
    float _StationaryMarkupDelaySeconds = 1.5f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "StationaryMarkup",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0, Units = "cm/s",
            ToolTip = "Maximum windowed physical speed (cm/s) that counts as stationary for nav markup. Measured from XY displacement over a 0.25-second window rather than instantaneous velocity, so one-frame push-apart spikes do not churn the disc. Above this speed the stationary timer resets and any painted disc is removed. The 84cm/s default preserves the former half-radius-per-window rule for a standard 42cm agent."))
    float _StationaryMarkupSpeedThreshold = 84.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "StationaryMarkup",
        meta = (AllowPrivateAccess = true, ClampMin = 1.0, UIMin = 1.0, ClampMax = 4.0, UIMax = 4.0,
            ToolTip = "Painted half-extent as a multiple of the agent radius. Sized so neighbouring discs in a queue OVERLAP DEEPLY: the planner crosses a crowd at the cheapest seam between two agents, and shallow overlap (1.5x at 120uu queue pitch) made threading the seam cheaper than detouring around the line. 2x makes the seam ~120uu of marked ground, so 'around' wins at store scale."))
    float _StationaryMarkupExtentMultiplier = 2.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "StationaryMarkup",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Master switch for mid-walk path refresh. StationaryMarkup only bends paths computed AFTER a disc is painted; this re-paths a WALKING agent whose remaining path crosses a disc newer than its path, so already-issued moves detour around a crowd that formed after they planned. No-op while StationaryMarkup is Disabled (no discs exist)."))
    ECk_CrowdPathRefreshMode _PathRefreshMode = ECk_CrowdPathRefreshMode::Enabled;

    UPROPERTY(Config, EditDefaultsOnly, Category = "StationaryMarkup",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Two-phase planning over stationary-crowd markup. Enabled: every agent path plans FIRST with a strict filter that treats painted markup as IMPASSABLE — passers-by and queue-crossers always route around standing crowds instead of discovering with their bodies that a toll-priced route cannot physically be walked; only when no crowd-free route exists does the episode re-dispatch once with the permissive toll filter (how queue-joiners still reach a slot beside standing bodies). The strict filter is the agent's NavQueryFilterStrict tag, or the framework's UCk_NavQueryFilter_AvoidStandingCrowds when unset. Disabled restores single-phase toll planning: for a destination close behind a standing crowd, crossing the toll is cheaper than any detour, and the agent presses into bodies it can never pass."))
    ECk_CrowdPlanAroundStandingCrowdsMode _PlanAroundStandingCrowds = ECk_CrowdPlanAroundStandingCrowdsMode::Enabled;

    UPROPERTY(Config, EditDefaultsOnly, Category = "StationaryMarkup",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0,
            ToolTip = "Seconds after a disc is painted before PathRefresh even checks it against the navmesh. A cheap pre-filter only — actual eligibility is ground truth (the rebuilt mesh must report the cost area at the disc's location), so this just keeps the poly query off discs that cannot possibly have rebaked yet."))
    float _PathRefreshMarkupSettleSeconds = 0.5f;

    // ---- Path follow ----
    UPROPERTY(Config, EditDefaultsOnly, Category = "PathFollow",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Master switch for line-of-sight-gated waypoint retirement. Both retirement tests (proximity and the passed-plane crossing) are laterally blind: an agent standing a few cm inside a corner satisfies them while the straight chord to the FOLLOWING waypoint cuts across a UNavArea_Null hole or a wall. Retiring there aims steering through geometry it cannot enter, the navmesh constraint eats the whole displacement, and the agent walks on the spot until block detection notices. Enabled keeps the agent aimed at the corner (which is on-mesh by construction) until the next chord is navigable, so it walks to the corner and then turns. Disabled restores the old behaviour, for A/B comparison only. Worlds without nav data behave identically either way."))
    ECk_CrowdWaypointRetirementLineOfSightMode _WaypointRetirementLineOfSight = ECk_CrowdWaypointRetirementLineOfSightMode::Enabled;

    // ---- Facing ----
    UPROPERTY(Config, EditDefaultsOnly, Category = "Facing",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0, Units = "cm/s",
            ToolTip = "Desired speed (cm/s) below which an agent stops tracking its desired-velocity heading and simply holds the facing it has. The heading is the avoidance sampler's per-frame winner and may legally slew at the full turn rate at ANY speed, so a jammed agent going nowhere transcribes every flip and whips on the spot. The 10cm/s default matches _BlockedStationarySpeedThreshold, the module's definition of not moving, so facing and block detection agree on which agents are standing still. Re-engagement needs twice this speed, because a bare threshold is a cliff an oscillating speed toggles across. 0 disables the floor. NOT a rotation smoother: raising it just freezes facing on every slow-moving agent — for residual rotational chatter, tune _FacingDeadBandDeg instead."))
    float _FacingSpeedFloorCm = 10.0f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Facing",
        meta = (AllowPrivateAccess = true, ClampMin = 0.0, UIMin = 0.0, UIMax = 14.0, Units = "deg",
            ToolTip = "Heading changes smaller than this never move the committed facing target at all, so the body converges on its facing and STAYS instead of transcribing every sub-degree wobble at the full turn rate — the residual micro-jitter a moving crowd shows once the large sampler flips are already persistence-filtered. A genuine slow arc accumulates past the band within a few frames and tracks normally, at the cost of the body trailing the true heading by at most this many degrees. Keep it below the 15-degree track tolerance: at or above it, every turn pays the persistence filter's commit latency instead. 0 disables the band."))
    float _FacingDeadBandDeg = 6.0f;

    // ---- Push-Apart ----
    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|PushApart",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Post-integration physical resolution of overlapping agents. Standard = 4 iterations per dtCrowd. Disabled allows brief overlap during sampling latency."))
    ECk_PushApartMode _PushApartMode = ECk_PushApartMode::Standard;

    // Overlap this small is left alone. Without it the resolver has no fixed point: it removes
    // COLLISION_RESOLVE_FACTOR of the penetration per iteration, which decays geometrically and
    // never reaches zero, so a SETTLED pile keeps issuing sub-millimetre corrections forever. Each
    // agent reads a one-frame-old neighbour cache, so in a dense pile those corrections chase each
    // other and the formation creeps indefinitely -- measured 0.55 cm/s per agent across 15 agents
    // still drifting 9.6s after they had all dropped below the settled speed. The slop gives the
    // solver a fixed point of finite width to land in, which is why every physics solver has one.
    // Agents come to rest at (radius sum - slop), so the slop is bounded on BOTH sides and the
    // corridor is narrow. Floor: it must exceed the residual per-frame correction, measured at
    // ~0.27mm (0.55 cm/s of creep per agent at 20Hz, 15 agents). Ceiling: Crowd_BunchUp asserts
    // at-rest separation >= 84.0cm with a 0.1cm tolerance, so anything at or above 0.1 rests the
    // pile below that floor and trades this defect for a separation failure. 0.05 sits between
    // the two with roughly a factor of two either way -- raising it needs that assertion re-derived
    // first. 0 restores the old never-terminating behaviour.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|PushApart",
        meta = (AllowPrivateAccess = true, ClampMin = "0.0",
            ToolTip = "Penetration below this (cm) is not corrected. Gives the push-apart solver a resting point instead of an asymptote it chases forever. 0 = correct any overlap, however small."))
    float _PushApartSlopCm = 0.05f;

public:
    CK_PROPERTY_GET(_AccelClampMode);
    CK_PROPERTY_GET(_AvoidanceSampleTrigger);
    CK_PROPERTY_GET(_AvoidanceSampleNeighborThreshold);
    CK_PROPERTY_GET(_AvoidanceSampleStride);
    CK_PROPERTY_GET(_AvoidanceSampleAngularDivs);
    CK_PROPERTY_GET(_AvoidanceSampleRings);
    CK_PROPERTY_GET(_AvoidanceSampleDepth);
    CK_PROPERTY_GET(_AvoidanceVelBias);
    CK_PROPERTY_GET(_AvoidanceSidePreference);
    CK_PROPERTY_GET(_AvoidanceWeightDesVel);
    CK_PROPERTY_GET(_AvoidanceWeightCurVel);
    CK_PROPERTY_GET(_AvoidanceWeightSide);
    CK_PROPERTY_GET(_AvoidanceWeightToi);
    CK_PROPERTY_GET(_AvoidanceHorizonTime);
    CK_PROPERTY_GET(_AvoidanceFinalApproachSuppressionCm);
    CK_PROPERTY_GET(_AvoidanceWallSegments);
    CK_PROPERTY_GET(_AvoidanceWallQueryRangeMultiplier);
    CK_PROPERTY_GET(_CorridorStandDown);
    CK_PROPERTY_GET(_CorridorStandDownSlackCm);
    CK_PROPERTY_GET(_PushApartMode);
    CK_PROPERTY_GET(_PushApartSlopCm);
    CK_PROPERTY_GET(_NavmeshConstraintMode);
    CK_PROPERTY_GET(_GroundingVerifyIntervalSeconds);
    CK_PROPERTY_GET(_GroundingVerifyMinCorrectionCm);
    CK_PROPERTY_GET(_StationaryMarkupMode);
    CK_PROPERTY_GET(_StationaryMarkupDelaySeconds);
    CK_PROPERTY_GET(_StationaryMarkupSpeedThreshold);
    CK_PROPERTY_GET(_StationaryMarkupExtentMultiplier);
    CK_PROPERTY_GET(_PathRefreshMode);
    CK_PROPERTY_GET(_PlanAroundStandingCrowds);
    CK_PROPERTY_GET(_PathRefreshMarkupSettleSeconds);
    CK_PROPERTY_GET(_WaypointRetirementLineOfSight);
    CK_PROPERTY_GET(_FacingSpeedFloorCm);
    CK_PROPERTY_GET(_FacingDeadBandDeg);
    CK_PROPERTY_GET(_BlockDetectionMode);
    CK_PROPERTY_GET(_BlockDetectionInterval);
    CK_PROPERTY_GET(_BlockDetectionNoProgressWindowSeconds);
    CK_PROPERTY_GET(_PathPendingTimeoutSeconds);
    CK_PROPERTY_GET(_BlockDetectionProgressEpsilonCm);
    CK_PROPERTY_GET(_BlockDetectionMaxStallRepaths);
    CK_PROPERTY_GET(_BlockDetectionOffPathRepathThresholdCm);
    CK_PROPERTY_GET(_BlockedStationarySpeedThreshold);
    CK_PROPERTY_GET(_BlockedRecheckInterval);
    CK_PROPERTY_GET(_BlockedMaxRetries);
    CK_PROPERTY_GET(_CrowdedGoalBlockMode);
    CK_PROPERTY_GET(_CrowdedGoalContactPadCm);
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
    static int32 Get_AvoidanceSampleDepth();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_AvoidanceWallSegmentsMode Get_AvoidanceWallSegments();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_CrowdCorridorStandDownMode Get_CorridorStandDown();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static float Get_CorridorStandDownSlackCm();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_PushApartMode Get_PushApartMode();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static float Get_PushApartSlopCm();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_CrowdNavmeshConstraintMode Get_NavmeshConstraintMode();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static float Get_GroundingVerifyIntervalSeconds();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static float Get_GroundingVerifyMinCorrectionCm();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_CrowdStationaryMarkupMode Get_StationaryMarkupMode();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_CrowdPathRefreshMode Get_PathRefreshMode();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_CrowdPlanAroundStandingCrowdsMode Get_PlanAroundStandingCrowds();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_CrowdBlockDetectionMode Get_BlockDetectionMode();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_CrowdCrowdedGoalBlockMode Get_CrowdedGoalBlockMode();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Crowd|Settings")
    static ECk_CrowdWaypointRetirementLineOfSightMode Get_WaypointRetirementLineOfSight();

    // Internal C++ accessor avoiding repeated GetMutableDefault calls in hot paths.
    static const UCk_Crowd_ProjectSettings_UE* Get();
};

// --------------------------------------------------------------------------------------------------------------------
