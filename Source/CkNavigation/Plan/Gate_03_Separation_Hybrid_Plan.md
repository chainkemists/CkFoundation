# Gate 3 Separation Hybrid — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the hybrid force + sampling avoidance system specified in [Gate_03_Separation_Addendum.md](Gate_03_Separation_Addendum.md), using dtCrowd-style penalty-scored velocity sampling on top of the tuned force solver, with project-wide enum-gated algorithm modes.

**Architecture:** Phase 1 fixes vibration in the existing force solver via inertia weighting + a new `FProcessor_CrowdAgent_AccelClamp` that clamps velocity delta in vector space (mirroring `DetourCrowd.cpp:integrate()`). Phase 2 adds a sampling override processor (16 candidates, 4-weight penalty mirroring `DetourObstacleAvoidance.cpp:processSample()`), zone-tag opt-in, and a 4-iteration push-apart pass (mirroring `DetourCrowd.cpp:updateStepMove() 1601-1662`). Every algorithm mode is project-tunable via enums on a new `UCk_Crowd_ProjectSettings_UE`.

**Tech Stack:** Unreal 5.6, C++20, EnTT-backed ECS, CkFoundation framework conventions per `Plugins/CkFoundation/Source/CLAUDE.md`. Tests are AngelScript AutoStation gyms in `Plugins/CkTests/Script/CkCrowd/`.

**Reference codebases (cited extensively):**
- `D:\Repos\UnrealEngineAngelscript\Engine\Source\Runtime\Navmesh\Private\DetourCrowd\` — primary penalty/sampling/integrate/push-apart reference
- `E:\UE_5.6\Engine\Plugins\Marketplace\AntRTSCr15a91b355b0dV6\Source\Ant\` — primary per-agent solver-dispatch architecture reference

---

## Pre-flight checklist

Before starting Task 1, verify:

- [ ] CkFoundation is on `feature/navigation` branch with HEAD at `6227d8cdb` (the spec commit)
- [ ] CkTests is on `feature/navigation` branch with HEAD at `d67b3f8` (Separation gym registry commit)
- [ ] Editor closes cleanly; full IDE rebuild produces no errors on the current state
- [ ] `Ck_GymCrowd_Sep_HeadOnNS` in PIE produces the existing vibration behavior (so we have a baseline to measure improvement against)

---

## Task 1: Crowd Project Settings + Algorithm-Mode Enums

Lays foundation for everything downstream. Defines every enum and exposes the project-wide tunables. No behavior change yet — purely scaffolding.

**Files:**
- Create: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Settings/CkCrowd_ProjectSettings.h`
- Create: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Settings/CkCrowd_ProjectSettings.cpp`
- Modify: `Plugins/CkFoundation/Source/CkCrowd/CkCrowd.Build.cs` — confirm `CkSettings` is a public dep

- [ ] **Step 1.1: Verify Build.cs has CkSettings dep**

Read `Plugins/CkFoundation/Source/CkCrowd/CkCrowd.Build.cs`. If `CkSettings` is not in `PublicDependencyModuleNames`, add it. Existing deps include `CkShapes`, `CkSpatialQuery`, `CkNavigation` from prior commits.

- [ ] **Step 1.2: Create the settings header**

Path: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Settings/CkCrowd_ProjectSettings.h`

```cpp
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
            ToolTip = "NeighborCache size that triggers sampling under NeighborCountOnly / NeighborCountAndZoneTag. Default 3 fires for most queue scenarios; bump to 5+ to limit sampling to dense pile-ups only."))
    int32 _AvoidanceSampleNeighborThreshold = 3;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling",
        meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1, ClampMax = 8, UIMax = 8,
            ToolTip = "Round-robin stride. 1=every triggered agent every frame, 3=20Hz per agent (default), higher=cheaper but laggier."))
    int32 _AvoidanceSampleStride = 3;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling",
        meta = (AllowPrivateAccess = true, ClampMin = 4, UIMin = 4, ClampMax = 16, UIMax = 16))
    int32 _AvoidanceSampleAngularDivs = 8;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Avoidance|Sampling",
        meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1, ClampMax = 4, UIMax = 4))
    int32 _AvoidanceSampleRings = 2;

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
    float _AvoidanceWeightCurVel = 1.0f;

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
```

- [ ] **Step 1.3: Create the settings cpp**

Path: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Settings/CkCrowd_ProjectSettings.cpp`

```cpp
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_Crowd_Settings_UE::Get() -> const UCk_Crowd_ProjectSettings_UE*
{
    return GetDefault<UCk_Crowd_ProjectSettings_UE>();
}

auto UCk_Utils_Crowd_Settings_UE::Get_AccelClampMode() -> ECk_AccelClampMode
{
    return Get()->Get_AccelClampMode();
}

auto UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleTrigger() -> ECk_AvoidanceSampleTrigger
{
    return Get()->Get_AvoidanceSampleTrigger();
}

auto UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleNeighborThreshold() -> int32
{
    return Get()->Get_AvoidanceSampleNeighborThreshold();
}

auto UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleStride() -> int32
{
    return Get()->Get_AvoidanceSampleStride();
}

auto UCk_Utils_Crowd_Settings_UE::Get_PushApartMode() -> ECk_PushApartMode
{
    return Get()->Get_PushApartMode();
}

// --------------------------------------------------------------------------------------------------------------------
```

- [ ] **Step 1.4: Build verification (full editor close + IDE build, not Live Coding)**

Live Coding does not register UCLASS additions cleanly. Close Unreal editor fully, build CkFoundation from IDE, relaunch.

Expected: editor opens with Project Settings → Plugins → "CkFoundation > Crowd" category visible with all enum / numeric fields populated.

- [ ] **Step 1.5: Commit**

```bash
cd /d/Repos/CkPlugins/Plugins/CkFoundation
wait_for_git && git add Source/CkCrowd/Public/CkCrowd/Settings/CkCrowd_ProjectSettings.h Source/CkCrowd/Public/CkCrowd/Settings/CkCrowd_ProjectSettings.cpp Source/CkCrowd/CkCrowd.Build.cs
git commit -m "feat(Crowd): UCk_Crowd_ProjectSettings_UE with algorithm-mode enums

Lays the foundation for the Gate 3 hybrid avoidance system. No behavior
change yet — every algorithm mode (AccelClamp, AvoidanceSampleTrigger,
AvoidanceSidePreference, PushApart) plus penalty weights and sampling
parameters live as project settings. All modes are explicit enums per
the addendum's enum-over-bool convention."
```

---

## Task 2: Phase 1.1 — Separation Force Inertia Weighting

Add the `_SeparationInertia` tunable on params and apply the lerp in the existing solver. Mirrors dtCrowd's `weightCurVel` (`DetourObstacleAvoidance.cpp:472`).

**Files:**
- Modify: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h`
- Modify: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Separation_Processor.cpp`

- [ ] **Step 2.1: Add `_SeparationInertia` to params**

In `CkCrowdAgent_Fragment_Data.h`, inside `FCk_Fragment_CrowdAgent_ParamsData`, add the tunable below the existing `_SeparationWeight`:

```cpp
    // Inertia coefficient on separation force. 0=instant changes (vibrate-prone), 1=fully sticky
    // (force never changes). Mirrors dtCrowd's weightCurVel concept (DetourObstacleAvoidance.cpp:472)
    // applied as a force-blend factor — see Gate_03_Separation_Addendum.md §4.1.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess=true, ClampMin="0.0", ClampMax="1.0"))
    float _SeparationInertia = 0.5f;
```

Add `CK_PROPERTY(_SeparationInertia);` to the public block alongside the other property macros.

- [ ] **Step 2.2: Apply inertia lerp in the Separation processor**

In `CkCrowdAgent_Separation_Processor.cpp`, locate the `ForEachEntity` body. Replace the existing final assignment:

```cpp
        InSeparationForce._Force = (Force + Jitter * 0.05f) * SeparationWeight * MaxSpeed;
```

With:

```cpp
        const auto NewForce = (Force + Jitter * 0.05f) * SeparationWeight * MaxSpeed;
        const auto LastForce = InSeparationForce.Get_Force();
        const auto Inertia = FMath::Clamp(InParams.Get_SeparationInertia(), 0.0f, 1.0f);
        // Inertia=0 → NewForce (today's behaviour), Inertia=1 → LastForce never changes.
        // 0.5 default mirrors dtCrowd's wCurVel/wDesVel ratio (~0.375).
        InSeparationForce._Force = FMath::Lerp(NewForce, LastForce, Inertia);
```

- [ ] **Step 2.3: Build (Live Coding OK for this change — no UCLASS additions)**

Build. Expected: clean compile.

- [ ] **Step 2.4: PIE smoke test**

In the Crowd Separation gym:
1. `ck.Crowd.Debug 1`
2. `Ck_GymCrowd_Sep_HeadOnNS`
3. Watch the orange force arrows on each agent.

Expected: arrows no longer flicker chaotically frame-to-frame. They smoothly rotate as agents close. Agents may still clip on the closest pass — that's Phase 2's job.

- [ ] **Step 2.5: Commit**

```bash
git add Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Separation_Processor.cpp
git commit -m "feat(Crowd): Phase 1.1 — separation force inertia weighting

Force solver now lerps each frame's computed force toward the previous
frame's force by _SeparationInertia (default 0.5). Eliminates frame-to-
frame force flicker that drove Phase 1's vibration mode in head-on
encounters. Mirrors dtCrowd's weightCurVel penalty concept
(DetourObstacleAvoidance.cpp:472), applied here as a force-blend factor
since Phase 1 has no sample-and-score solver."
```

---

## Task 3: Phase 1.2 — AccelClamp Processor

Add a dedicated processor that clamps the velocity *delta* in vector space. Replaces the redundant scalar clamp inside Steering. Mirrors `DetourCrowd.cpp:integrate() 53-69`.

**Files:**
- Modify: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h` — add `_LastVelocity` to `FCk_Fragment_CrowdAgent_DesiredVelocityData`
- Create: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AccelClamp_Processor.h`
- Create: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AccelClamp_Processor.cpp`
- Modify: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Steering_Processor.cpp` — remove the inline scalar clamp; just write `Direction * TargetSpeed + SeparationVec`

- [ ] **Step 3.1: Add `_LastVelocity` field to DesiredVelocity fragment**

In `CkCrowdAgent_Fragment_Data.h`, inside `FCk_Fragment_CrowdAgent_DesiredVelocityData`, add:

```cpp
    UPROPERTY()
    FVector _LastVelocity = FVector::ZeroVector;
```

Add the friend declaration for the new processor at the top of the struct:

```cpp
    friend class ck::FProcessor_CrowdAgent_AccelClamp;
```

And the forward declaration in the namespace at the file's head:

```cpp
namespace ck
{
    // ... existing ...
    class FProcessor_CrowdAgent_AccelClamp;
}
```

- [ ] **Step 3.2: Create the AccelClamp processor header**

Path: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AccelClamp_Processor.h`

```cpp
#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPhysics/EulerIntegrator/CkEulerIntegrator_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Phase 1.2 — clamps the per-frame VELOCITY DELTA on FFragment_CrowdAgent_DesiredVelocity.
    // Mirrors DetourCrowd.cpp:integrate() 53-69. Critical because Steering writes a fresh
    // `Direction * TargetSpeed` each frame — direction can flip arbitrarily, which is the root
    // cause of the head-on vibration mode. Capping |dv| ≤ MaxAccel × dt forces direction changes
    // to ramp instead of snap.
    //
    // Group: FGroup_Physics. RunAfter Steering (and, in Phase 2, AvoidanceSample) so this processor
    // sees whichever solver wrote last. RunBefore VelocityBridge so the bridge ships the clamped
    // value into the physics layer.
    //
    // Disable for A/B comparison via UCk_Crowd_ProjectSettings_UE._AccelClampMode = Disabled.
    class CKCROWD_API FProcessor_CrowdAgent_AccelClamp : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_AccelClamp,
            FCk_Handle_CrowdAgent,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_Steering>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
```

- [ ] **Step 3.3: Create the AccelClamp processor body**

Path: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AccelClamp_Processor.cpp`

```cpp
#include "CkCrowdAgent_AccelClamp_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_AccelClamp);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_AccelClamp::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            FFragment_CrowdAgent_DesiredVelocity& InDesired)
        -> void
    {
        // Project-wide enable. Disabled mode is for A/B comparison only — production should leave
        // this on to kill snap-flips that drive Gate-3 vibration.
        if (UCk_Utils_Crowd_Settings_UE::Get_AccelClampMode() == ECk_AccelClampMode::Disabled)
        {
            InDesired._LastVelocity = InDesired._Velocity;
            return;
        }

        const auto MaxSpeed = InParams.Get_MaxSpeed();
        const auto MaxAccel = InParams.Get_MaxAcceleration();
        const auto MaxDelta = MaxAccel * static_cast<float>(InDeltaT.Get_Seconds());

        const auto LastVel = InDesired.Get_LastVelocity();
        const auto NewVel  = InDesired.Get_Velocity().GetClampedToMaxSize(MaxSpeed);
        const auto Dv      = NewVel - LastVel;
        const auto DvLen   = static_cast<float>(Dv.Size());

        if (DvLen > MaxDelta && DvLen > KINDA_SMALL_NUMBER)
        {
            InDesired._Velocity = LastVel + Dv * (MaxDelta / DvLen);
        }
        else
        {
            InDesired._Velocity = NewVel;
        }

        InDesired._LastVelocity = InDesired._Velocity;
    }
}

// --------------------------------------------------------------------------------------------------------------------
```

- [ ] **Step 3.4: Simplify Steering — remove the redundant scalar clamp**

In `CkCrowdAgent_Steering_Processor.cpp`, find the block:

```cpp
        const auto MaxSpeed = InParams.Get_MaxSpeed();
        const auto MaxAccel = InParams.Get_MaxAcceleration();

        // Braking ramp ...
        auto BrakingSpeedCap = MaxSpeed;
        if (MaxAccel > 0.0f)
        {
            BrakingSpeedCap = FMath::Sqrt(2.0f * MaxAccel * DistanceToFinal);
        }

        const auto TargetSpeed = FMath::Min(MaxSpeed, BrakingSpeedCap);

        // Acceleration clamp from last frame's desired speed. ...
        const auto PreviousSpeed = InDesired._Velocity.Size();
        const auto SpeedDelta = MaxAccel * InDeltaT.Get_Seconds();
        const auto NewSpeed = FMath::Clamp(
            TargetSpeed,
            FMath::Max(0.0f, PreviousSpeed - SpeedDelta),
            PreviousSpeed + SpeedDelta);
```

Replace with (drop the scalar clamp; AccelClamp now handles it in vector space):

```cpp
        const auto MaxSpeed = InParams.Get_MaxSpeed();
        const auto MaxAccel = InParams.Get_MaxAcceleration();

        // Braking ramp: stopping distance for speed v at deceleration a is v² / (2a). Solving for v
        // given the remaining distance gives the max speed at which we can still stop in time.
        auto BrakingSpeedCap = MaxSpeed;
        if (MaxAccel > 0.0f)
        {
            BrakingSpeedCap = FMath::Sqrt(2.0f * MaxAccel * DistanceToFinal);
        }

        const auto NewSpeed = FMath::Min(MaxSpeed, BrakingSpeedCap);
```

The downstream combination logic (`PathFollowDamp`, `Combined`, `GetClampedToMaxSize(MaxSpeed)`) stays — it produces the desired velocity that AccelClamp then ramps.

- [ ] **Step 3.5: Full IDE rebuild + smoke test**

Close editor, IDE build, relaunch (new processor needs `CK_REGISTER_PROCESSOR` static-init).

PIE: Crowd Separation gym, `ck.Crowd.Debug 1`, `Ck_GymCrowd_Sep_HeadOnNS`. Compare against Task 2 baseline:

Expected: agents now physically arc apart instead of vibrating. Direction changes are smooth (no snap-flips). Force arrows rotate gradually as agents close. Some clipping at closest pass may still occur — Phase 2 fixes that.

Toggle `_AccelClampMode = Disabled` in Project Settings and re-run — expect the old vibration to return. Toggle back to `Enabled`.

- [ ] **Step 3.6: Commit**

```bash
git add Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AccelClamp_Processor.h Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AccelClamp_Processor.cpp Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Fragment_Data.h Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Steering_Processor.cpp
git commit -m "feat(Crowd): Phase 1.2 — AccelClamp processor (vector-delta velocity ramp)

New FProcessor_CrowdAgent_AccelClamp clamps the per-frame velocity delta
in vector space — |new - last| <= MaxAccel*dt. Replaces the scalar
PreviousSpeed clamp inside Steering, which only limited magnitude and
allowed direction to snap-flip frame-to-frame (the root cause of Phase-1
vibration). Mirrors DetourCrowd.cpp:integrate() 53-69 directly.

DesiredVelocity fragment grows a _LastVelocity field as the per-frame
baseline. Project setting _AccelClampMode = Disabled reverts to the
pre-fix scalar clamp for A/B comparison."
```

---

## Task 4: Per-Agent Avoidance Policy + Zone Tags

Defines the per-agent override fragment and the gameplay tags that designers use to opt zones in/out of sampling. No processor consumes these yet — that's Task 5.

**Files:**
- Create: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Avoidance_Fragment.h`
- Create: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Avoidance_Fragment.cpp`

- [ ] **Step 4.1: Create the avoidance-fragment header**

Path: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Avoidance_Fragment.h`

```cpp
#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include <GameplayTagContainer.h>
#include <NativeGameplayTags.h>

#include "CkCrowdAgent_Avoidance_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Per-agent avoidance-policy override + zone-tag opt-in/out for Phase 2 sampling.
// See Gate_03_Separation_Addendum.md §5.7 for design rationale.
// --------------------------------------------------------------------------------------------------------------------

// Designer-side opt-in: any agent (or any entity in the agent's lifetime-owner chain) carrying
// this tag forces sampling on regardless of NeighborCache size.
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_CrowdAvoidance_AlwaysSample);

// Designer-side opt-out: forces force-only behaviour even when neighbor count would normally
// trigger sampling. Use case: tutorial NPCs, scripted set-pieces.
CKCROWD_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_CrowdAvoidance_NeverSample);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AvoidancePolicy : uint8
{
    UseProjectDefault,  // honour _AvoidanceSampleTrigger + tags
    ForceOnly,          // never sample (overrides project + zone tag)
    SamplingAlways,     // always sample (overrides everything)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_AvoidancePolicy
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_AvoidancePolicy);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess=true))
    ECk_AvoidancePolicy _Policy = ECk_AvoidancePolicy::UseProjectDefault;

public:
    CK_PROPERTY(_Policy);
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_CrowdAgent_AvoidancePolicy, _Policy);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Alias so processors include this fragment alongside the others on an agent.
    using FFragment_CrowdAgent_AvoidancePolicy = FCk_Fragment_CrowdAgent_AvoidancePolicy;
}
```

- [ ] **Step 4.2: Create the avoidance-fragment cpp**

Path: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Avoidance_Fragment.cpp`

```cpp
#include "CkCrowdAgent_Avoidance_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_CrowdAvoidance_AlwaysSample, "CrowdAvoidance.AlwaysSample");
UE_DEFINE_GAMEPLAY_TAG(TAG_CrowdAvoidance_NeverSample,  "CrowdAvoidance.NeverSample");

// --------------------------------------------------------------------------------------------------------------------
```

- [ ] **Step 4.3: Build verification**

Live Coding works for this — no UCLASS additions, only a USTRUCT and tag definitions. Build, expect clean compile. The tags will appear in the editor's GameplayTag pickers under "CrowdAvoidance".

- [ ] **Step 4.4: Commit**

```bash
git add Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Avoidance_Fragment.h Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Avoidance_Fragment.cpp
git commit -m "feat(Crowd): per-agent AvoidancePolicy fragment + CrowdAvoidance gameplay tags

Phase 2 prep — defines the ECk_AvoidancePolicy enum
(UseProjectDefault / ForceOnly / SamplingAlways) and the optional
FCk_Fragment_CrowdAgent_AvoidancePolicy fragment that any agent can
carry to override the project-wide sampling trigger.

Plus two gameplay tags (TAG_CrowdAvoidance_AlwaysSample /
TAG_CrowdAvoidance_NeverSample) for designer-controlled zone opt-in/out.
The sampling processor (Task 5) consults these via a lifetime-owner
walk so applying a tag to a station / volume / parent entity propagates
to its child agents."
```

---

## Task 5: Phase 2 — Sampling Avoidance Override Processor

The big task — the core dtCrowd-style sampler. Mirrors `DetourObstacleAvoidance.cpp:processSample() 323-426` and `sampleVelocityAdaptive() 520-608`, with simplifications spelled out in the addendum §5.

**Files:**
- Create: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AvoidanceSample_Processor.h`
- Create: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AvoidanceSample_Processor.cpp`
- Modify: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AccelClamp_Processor.h` — add `RunAfter` for the new processor

- [ ] **Step 5.1: Create the AvoidanceSample processor header**

Path: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AvoidanceSample_Processor.h`

```cpp
#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Steering_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Phase 2 — dtCrowd-style penalty-scored velocity sampling. Runs only when triggered
    // (project-wide trigger mode + per-agent override + zone tags), only on 1 in N frames per
    // agent (round-robin), only on agents with at least 1 neighbor in cache. When it runs,
    // overwrites _DesiredVelocity with the lowest-penalty candidate from a 16-sample pattern.
    //
    // See Gate_03_Separation_Addendum.md §5 for design rationale and dtCrowd citations.
    //
    // Group: FGroup_Physics. RunAfter Steering (we override its output) + NeighborSync (we read
    // the cache it just wrote). RunBefore AccelClamp + VelocityBridge (so the override gets ramped
    // and shipped). RunBefore is enforced by AccelClamp's RunAfter on this processor (Step 5.6).
    class CKCROWD_API FProcessor_CrowdAgent_AvoidanceSample : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_AvoidanceSample,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_Walking,
            FTag_CrowdAgent_HasProbe,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            ck::TReadWrite<FFragment_CrowdAgent_DesiredVelocity>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_Steering, FProcessor_CrowdAgent_NeighborSync>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_DesiredVelocity& InDesired) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
```

- [ ] **Step 5.2: Create the AvoidanceSample processor body — trigger logic**

Path: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AvoidanceSample_Processor.cpp`

Start with includes, registration, and the trigger evaluator:

```cpp
#include "CkCrowdAgent_AvoidanceSample_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "CkCrowd/Agent/CkCrowdAgent_Avoidance_Fragment.h"
#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

#include "GameplayTagAssetInterface.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_AvoidanceSample);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace
    {
        // Tag check — agent itself plus its lifetime owner (typical: gym station / zone volume).
        // Walks one hop only; designer puts the tag on the immediate parent that defines the zone.
        auto HasAvoidanceTag(const FCk_Handle& InAgent, const FGameplayTag& InTag) -> bool
        {
            // Direct check — the agent may carry it on its Params._Tags.
            const auto& Params = InAgent.Get<FFragment_CrowdAgent_Params>();
            if (Params.Get_Tags().HasTagExact(InTag))
            { return true; }

            const auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAgent);
            if (ck::Is_NOT_Valid(Owner))
            { return false; }

            // Owner-side tag — extend if Owner has its own tag-bearing fragment. For Gate 3 the
            // Params._Tags check above is the canonical path; this is the lifetime-owner fallback
            // for cases where designers tag the parent station instead.
            if (Owner.Has<FFragment_CrowdAgent_Params>())
            {
                return Owner.Get<FFragment_CrowdAgent_Params>().Get_Tags().HasTagExact(InTag);
            }
            return false;
        }

        // Resolve "should this agent sample this frame?" — combines project mode, per-agent
        // override, zone tags, neighbor count, and round-robin stride.
        auto ShouldSample(
            const FCk_Handle_CrowdAgent& InAgent,
            const FFragment_CrowdAgent_NeighborCache& InCache) -> bool
        {
            // Per-agent override has the highest priority (and short-circuits the rest).
            if (InAgent.Has<FFragment_CrowdAgent_AvoidancePolicy>())
            {
                switch (InAgent.Get<FFragment_CrowdAgent_AvoidancePolicy>().Get_Policy())
                {
                    case ECk_AvoidancePolicy::ForceOnly:       return false;
                    case ECk_AvoidancePolicy::SamplingAlways:  return true;
                    case ECk_AvoidancePolicy::UseProjectDefault: break;  // fall through
                }
            }

            // Zone-tag NeverSample wins over project default.
            if (HasAvoidanceTag(InAgent, TAG_CrowdAvoidance_NeverSample))
            { return false; }

            const auto Trigger = UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleTrigger();
            if (Trigger == ECk_AvoidanceSampleTrigger::Disabled)
            { return false; }

            const auto AlwaysTagSet = HasAvoidanceTag(InAgent, TAG_CrowdAvoidance_AlwaysSample);
            const auto Threshold = UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleNeighborThreshold();
            const auto NeighborGate = InCache.Get_Neighbors().Num() >= Threshold;

            switch (Trigger)
            {
                case ECk_AvoidanceSampleTrigger::NeighborCountOnly:        return NeighborGate;
                case ECk_AvoidanceSampleTrigger::ZoneTagOnly:              return AlwaysTagSet;
                case ECk_AvoidanceSampleTrigger::NeighborCountAndZoneTag:  return NeighborGate || AlwaysTagSet;
                default:                                                   return false;
            }
        }

        // Round-robin: agent fires only on frames where (Frame + EntityHash) % Stride == 0.
        // Distributes cost evenly regardless of which agents are clumped.
        auto IsSamplingFrame(const FCk_Handle_CrowdAgent& InAgent) -> bool
        {
            const auto Stride = FMath::Max(1, UCk_Utils_Crowd_Settings_UE::Get_AvoidanceSampleStride());
            const auto FrameIdx = static_cast<int32>(GFrameCounter);
            const auto AgentIdx = static_cast<int32>(GetTypeHash(InAgent));
            return ((FrameIdx + AgentIdx) % Stride) == 0;
        }
    }
}
```

- [ ] **Step 5.3: Add the sample-pattern + penalty-function helpers (still inside the anonymous namespace)**

Append below the existing helpers:

```cpp
namespace ck
{
    namespace
    {
        // Build the candidate-velocity set. Pattern mirrors DetourObstacleAvoidance.cpp:548-567
        // but stripped to a single depth iteration with N×R samples on concentric rings.
        // Returns up to AngularDivs * Rings candidates.
        auto BuildSamplePattern(
            const FVector& InDesiredVelocity,
            float InMaxSpeed,
            int32 InAngularDivs,
            int32 InRings) -> TArray<FVector>
        {
            TArray<FVector> Samples;
            Samples.Reserve(InAngularDivs * InRings + 1);

            // Ring 0: the do-nothing candidate (zero velocity). Always present so "stop" is on
            // the table when a strong neighbor force says we should yield entirely.
            Samples.Add(FVector::ZeroVector);

            // Use the desired velocity's heading as the pattern center. If it's near-zero (agent
            // braked at goal), use world +X as a fallback.
            const auto DesiredDir = InDesiredVelocity.IsNearlyZero()
                ? FVector::ForwardVector
                : InDesiredVelocity.GetSafeNormal();
            const auto BaseAngle = FMath::Atan2(DesiredDir.Y, DesiredDir.X);

            const auto AngularStep = (2.0f * PI) / static_cast<float>(InAngularDivs);
            for (int32 Ring = 1; Ring <= InRings; ++Ring)
            {
                const auto Radius = (static_cast<float>(Ring) / static_cast<float>(InRings)) * InMaxSpeed;
                // Stagger alternating rings by half-step for better angular coverage (mirrors dtCrowd).
                const auto AngleOffset = (Ring % 2 == 0) ? 0.5f * AngularStep : 0.0f;
                for (int32 Div = 0; Div < InAngularDivs; ++Div)
                {
                    const auto Angle = BaseAngle + AngleOffset + AngularStep * static_cast<float>(Div);
                    Samples.Emplace(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0);
                }
            }

            return Samples;
        }

        // Time-to-collision between self at velocity vCand and neighbor at relative position relPos
        // with relative velocity relVel (both in 2D, Z dropped). Returns FLT_MAX if no collision in
        // the time horizon, else seconds until first contact. Math mirrors DetourObstacleAvoidance.cpp
        // sweepCircleCircle: solves for t such that |relPos + (vCand-relVel)*t| == combinedRadius.
        auto TimeToCollision(
            const FVector& InCandidateVel,
            const FVector& InNeighborRelPos,
            const FVector& InNeighborRelVel,
            float InCombinedRadius,
            float InHorizon) -> float
        {
            // Translate to neighbor's frame: dv = candidate moving relative to neighbor.
            const auto Dv = FVector2D(InCandidateVel - InNeighborRelVel);
            const auto P  = FVector2D(-InNeighborRelPos.X, -InNeighborRelPos.Y);
            // (P + Dv*t)·(P + Dv*t) = R² → quadratic in t.
            const auto A = static_cast<float>(FVector2D::DotProduct(Dv, Dv));
            const auto B = static_cast<float>(FVector2D::DotProduct(P, Dv));
            const auto C = static_cast<float>(FVector2D::DotProduct(P, P)) - InCombinedRadius * InCombinedRadius;

            // Already inside the radius? Treat as immediate collision.
            if (C < 0.0f) { return 0.0f; }
            // Moving in a direction with no component toward neighbor? No collision.
            if (A < KINDA_SMALL_NUMBER || B >= 0.0f) { return FLT_MAX; }

            const auto Disc = B * B - A * C;
            if (Disc < 0.0f) { return FLT_MAX; }
            const auto T = (-B - FMath::Sqrt(Disc)) / A;
            return T > InHorizon ? FLT_MAX : T;
        }

        // Side score per neighbor — mirrors dtCrowd's wSide concept. >0 = candidate passes neighbor
        // on the right side relative to current motion; <0 = on the left. PassLeft pref penalises
        // positive (right-pass) results.
        auto SideScore(
            const FVector& InCandidateVel,
            const FVector& InCurrentVel,
            const FVector& InNeighborRelPos) -> float
        {
            // 2D cross of current-velocity and neighbor-relpos tells us which side neighbor is on.
            const auto CrossNeighbor = static_cast<float>(InCurrentVel.X * InNeighborRelPos.Y - InCurrentVel.Y * InNeighborRelPos.X);
            // 2D cross of current-velocity and (candidate-velocity) tells us which side the candidate
            // is taking us toward.
            const auto CrossCand = static_cast<float>(InCurrentVel.X * InCandidateVel.Y - InCurrentVel.Y * InCandidateVel.X);
            // Same sign = candidate goes toward neighbor's side = bad.
            return (CrossNeighbor * CrossCand >= 0.0f) ? FMath::Abs(CrossCand) : 0.0f;
        }
    }
}
```

- [ ] **Step 5.4: Add the main `ForEachEntity` body**

Append below the helpers:

```cpp
namespace ck
{
    auto
        FProcessor_CrowdAgent_AvoidanceSample::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache,
            FFragment_CrowdAgent_DesiredVelocity& InDesired)
        -> void
    {
        // Trigger gate. Off-path leaves the force solver's _Velocity in place (which will then be
        // ramped by AccelClamp downstream).
        if (NOT ShouldSample(InHandle, InNeighborCache))
        { return; }

        if (NOT IsSamplingFrame(InHandle))
        { return; }

        const auto& Neighbors = InNeighborCache.Get_Neighbors();
        if (Neighbors.Num() == 0)
        { return; }

        // Read project settings once per agent (cheap; settings are CDO-backed).
        const auto* Settings = UCk_Utils_Crowd_Settings_UE::Get();
        if (NOT IsValid(Settings))
        { return; }

        const auto MaxSpeed   = InParams.Get_MaxSpeed();
        const auto AgentRad   = InParams.Get_Radius();
        const auto Horizon    = Settings->Get_AvoidanceHorizonTime();
        const auto WDes       = Settings->Get_AvoidanceWeightDesVel();
        const auto WCur       = Settings->Get_AvoidanceWeightCurVel();
        const auto WSide      = Settings->Get_AvoidanceWeightSide();
        const auto WToi       = Settings->Get_AvoidanceWeightToi();
        const auto SidePref   = Settings->Get_AvoidanceSidePreference();
        const auto SideEnabled = (SidePref != ECk_AvoidanceSidePreference::Disabled);
        const auto SideSign   = (SidePref == ECk_AvoidanceSidePreference::PassRight) ? -1.0f : 1.0f;
        const auto AngularDivs = Settings->Get_AvoidanceSampleAngularDivs();
        const auto Rings       = Settings->Get_AvoidanceSampleRings();
        const auto InvVMax     = (MaxSpeed > KINDA_SMALL_NUMBER) ? 1.0f / MaxSpeed : 0.0f;
        const auto InvHorizon  = (Horizon  > KINDA_SMALL_NUMBER) ? 1.0f / Horizon  : 0.0f;

        // Cache the path-follow desired velocity (what the force solver wrote) BEFORE we overwrite.
        const auto DesiredVel = InDesired.Get_Velocity();
        // Current velocity = last frame's _LastVelocity (set by AccelClamp). Falls back to current
        // _Velocity for the first frame an agent samples (LastVelocity may still be zero).
        const auto CurrentVel = InDesired.Get_LastVelocity().IsNearlyZero()
            ? DesiredVel
            : InDesired.Get_LastVelocity();

        const auto Samples = BuildSamplePattern(DesiredVel, MaxSpeed, AngularDivs, Rings);

        // Score each candidate, track best.
        auto BestPenalty = TNumericLimits<float>::Max();
        auto BestVel     = DesiredVel;

        for (const auto& Cand : Samples)
        {
            // wDesVel: deviation from the path-follow heading (normalised by MaxSpeed).
            const auto DesPen = WDes * static_cast<float>(FVector::Dist2D(Cand, DesiredVel)) * InvVMax;
            // wCurVel: deviation from current velocity — the inertia bias.
            const auto CurPen = WCur * static_cast<float>(FVector::Dist2D(Cand, CurrentVel)) * InvVMax;

            // wToi: minimum time-to-collision across neighbors. Reciprocal-scaled so short TTCs
            // dominate (the formula is tpen = wToi * 1/(0.1 + tmin*invHorizTime), per dtCrowd).
            auto TMin = FLT_MAX;
            auto SideAccum = 0.0f;
            for (const auto& Nbr : Neighbors)
            {
                const auto NbrRad = AgentRad + AgentRad;  // approximation: neighbors share radius
                const auto Ttc = TimeToCollision(Cand, Nbr.Get_RelativeOffset(), Nbr.Get_RelativeVelocity(), NbrRad, Horizon);
                if (Ttc < TMin) { TMin = Ttc; }

                if (SideEnabled)
                {
                    SideAccum += SideScore(Cand, CurrentVel, Nbr.Get_RelativeOffset());
                }
            }
            const auto ToiPen = (TMin >= FLT_MAX)
                ? 0.0f
                : (WToi * (1.0f / (0.1f + TMin * InvHorizon)));
            const auto SidePen = SideEnabled
                ? (WSide * SideAccum * SideSign)
                : 0.0f;

            const auto Penalty = DesPen + CurPen + ToiPen + SidePen;
            if (Penalty < BestPenalty)
            {
                BestPenalty = Penalty;
                BestVel     = Cand;
            }
        }

        InDesired._Velocity = BestVel;
    }
}

// --------------------------------------------------------------------------------------------------------------------
```

- [ ] **Step 5.5: Update AccelClamp's `RunAfter` to include AvoidanceSample**

In `CkCrowdAgent_AccelClamp_Processor.h`, add the include and update the dep list:

```cpp
#include "CkCrowd/Agent/CkCrowdAgent_AvoidanceSample_Processor.h"

// ... in the class:
        using RunAfter = TDepList<
            FProcessor_CrowdAgent_Steering,
            FProcessor_CrowdAgent_AvoidanceSample>;
```

- [ ] **Step 5.6: Full IDE rebuild**

Close editor; IDE build; relaunch. Two new processors with `CK_REGISTER_PROCESSOR`.

- [ ] **Step 5.7: PIE smoke tests**

Multiple scenarios — observe with `ck.Crowd.Debug 1`:

1. **Baseline force-only:** in Project Settings, set `_AvoidanceSampleTrigger = Disabled`. Run `Ck_GymCrowd_Sep_HeadOnNS`. Should match Phase 1 behaviour (smooth arc, may clip).
2. **Sampling on (default):** set `_AvoidanceSampleTrigger = NeighborCountAndZoneTag`. Run same. Agents should now choose visibly different lateral velocities — orange arrows snap to the chosen sample direction. No clipping at closest pass.
3. **Cluster:** `Ck_GymCrowd_Sep_Cluster5`. 5 agents converge with different colours. Watch for graceful slot-finding rather than pile-up at the centre.
4. **Side preference:** toggle `_AvoidanceSidePreference` between PassLeft, PassRight, Disabled in Project Settings. Re-run `Sep_HeadOnNS`. Observe agents consistently take the same side or whichever is convenient.

- [ ] **Step 5.8: Commit**

```bash
git add Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AvoidanceSample_Processor.h Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AvoidanceSample_Processor.cpp Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_AccelClamp_Processor.h
git commit -m "feat(Crowd): Phase 2 — AvoidanceSample override processor (4-weight penalty)

Port of dtCrowd's penalty-scored velocity sampling stripped from 71 to
~17 candidates, with the four-term penalty function (wDesVel, wCurVel,
wSide, wToi) per DetourObstacleAvoidance.cpp:414-419. Inertia bias
(wCurVel) eliminates the head-on vibration; time-to-collision penalty
(wToi) provides predictive avoidance.

Trigger logic combines project-wide ECk_AvoidanceSampleTrigger,
per-agent FFragment_CrowdAgent_AvoidancePolicy override, and zone tags
TAG_CrowdAvoidance_AlwaysSample / NeverSample. Round-robin scheduling
keys off (FrameCounter + EntityHash) % Stride for even per-frame cost
distribution. Off-frames fall through to the force solver's velocity
(which AccelClamp then ramps).

Side-preference is enum-gated (Disabled / PassLeft / PassRight) so its
~25% per-sample cost is opt-in. PassLeft default mirrors dtCrowd
convention.

AccelClamp now declares RunAfter on AvoidanceSample so the override
gets the same vector-delta ramp the force-solver output does."
```

---

## Task 6: Phase 2 — PushApart Processor

Post-integration physical resolution. Mirrors `DetourCrowd.cpp:updateStepMove() 1601-1662`.

**Files:**
- Create: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_PushApart_Processor.h`
- Create: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_PushApart_Processor.cpp`

- [ ] **Step 6.1: Create the PushApart processor header**

Path: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_PushApart_Processor.h`

```cpp
#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkCrowd/Agent/CkCrowdAgent_ApplyOffset_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_Neighbors_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Phase 2 — post-hoc physical-resolution pass. Direct port of DetourCrowd.cpp:updateStepMove
    // 1601-1662. Iterates 1 or 4 times (project-controlled) over each agent's neighbor cache,
    // accumulating a displacement vector, and enqueues a single Request_AddLocationOffset per
    // agent. Resolves residual overlap that the sampler couldn't prevent (e.g., during the 50ms
    // sampling-latency window).
    //
    // Group: FGroup_Physics. RunAfter ApplyOffset (we read post-integration positions). The
    // resulting AddLocationOffset request is drained the same frame by Transform_HandleRequests.
    class CKCROWD_API FProcessor_CrowdAgent_PushApart : public ck_exp::TProcessor<
            FProcessor_CrowdAgent_PushApart,
            FCk_Handle_CrowdAgent,
            FTag_CrowdAgent_HasProbe,
            ck::TReadOnly<FFragment_CrowdAgent_Params>,
            ck::TReadOnly<FFragment_CrowdAgent_NeighborCache>,
            TExclude<FTag_CrowdAgent_Asleep>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Physics;
        using RunAfter = TDepList<FProcessor_CrowdAgent_ApplyOffset>;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
```

- [ ] **Step 6.2: Create the PushApart processor body**

Path: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_PushApart_Processor.cpp`

```cpp
#include "CkCrowdAgent_PushApart_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkCrowd/Settings/CkCrowd_ProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_PushApart);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace
    {
        // Per dtCrowd's COLLISION_RESOLVE_FACTOR (DetourCrowd.cpp:1599). Sub-1 so we don't
        // overshoot the resolution; iterations converge on the right answer.
        constexpr auto COLLISION_RESOLVE_FACTOR = 0.7f;

        auto Get_IterationCount(ECk_PushApartMode InMode) -> int32
        {
            switch (InMode)
            {
                case ECk_PushApartMode::Disabled: return 0;
                case ECk_PushApartMode::Single:   return 1;
                case ECk_PushApartMode::Standard: return 4;
                default:                          return 4;
            }
        }
    }

    auto
        FProcessor_CrowdAgent_PushApart::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_Params& InParams,
            const FFragment_CrowdAgent_NeighborCache& InNeighborCache)
        -> void
    {
        const auto Iterations = Get_IterationCount(UCk_Utils_Crowd_Settings_UE::Get_PushApartMode());
        if (Iterations <= 0)
        { return; }

        const auto& Neighbors = InNeighborCache.Get_Neighbors();
        if (Neighbors.Num() == 0)
        { return; }

        auto SelfTransform = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(SelfTransform))
        { return; }

        const auto SelfRadius = InParams.Get_Radius();
        const auto SelfCenter = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(SelfTransform);

        auto Displacement = FVector::ZeroVector;

        for (auto Iter = 0; Iter < Iterations; ++Iter)
        {
            for (const auto& Nbr : Neighbors)
            {
                // Use the cache's RelativeOffset directly — it's NbrLoc - SelfLoc as of NeighborSync.
                // Within a frame this is fresh-enough; for cross-iteration accuracy we'd refresh,
                // but dtCrowd's reference uses the same start-of-frame snapshot across iterations.
                const auto Diff = -Nbr.Get_RelativeOffset() - Displacement;  // self - nbr after our displacement
                const auto Dist = static_cast<float>(Diff.Size());
                const auto NeighborRadius = SelfRadius;  // approximation — neighbors share radius
                const auto CombinedRadius = SelfRadius + NeighborRadius;

                if (Dist >= CombinedRadius)
                { continue; }

                if (Dist < KINDA_SMALL_NUMBER)
                {
                    // Degenerate overlap (exact center match). Push along an arbitrary axis to
                    // unstick. Mirrors dtCrowd's degenerate-case handling.
                    Displacement += FVector(0.5f * CombinedRadius, 0.0f, 0.0f);
                    continue;
                }

                const auto Penetration = CombinedRadius - Dist;
                const auto Pen = (Penetration * 0.5f) * COLLISION_RESOLVE_FACTOR / Dist;
                Displacement += Diff * Pen;
            }
        }

        if (NOT Displacement.IsNearlyZero())
        {
            UCk_Utils_Transform_UE::Request_AddLocationOffset(
                SelfTransform,
                FCk_Request_Transform_AddLocationOffset{Displacement});
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
```

- [ ] **Step 6.3: Full IDE rebuild + smoke test**

Close editor, IDE build, relaunch.

PIE testing matrix (in Project Settings, toggle `_PushApartMode`):
1. **`Standard` (default):** `Ck_GymCrowd_Sep_HeadOnNS` — agents pass cleanly; if they briefly overlap during sampling latency, you see them snap apart by the next frame.
2. **`Single`:** same scenario; resolution slightly less complete (one iteration, may need a couple of frames for full separation).
3. **`Disabled`:** brief overlap during sampling latency persists until sampling re-resolves it. Useful baseline for comparison.

- [ ] **Step 6.4: Commit**

```bash
git add Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_PushApart_Processor.h Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_PushApart_Processor.cpp
git commit -m "feat(Crowd): Phase 2 — PushApart processor (post-hoc physical resolution)

Direct port of DetourCrowd.cpp:updateStepMove() 1601-1662. Iterates 1
or 4 times (per ECk_PushApartMode) over each agent's neighbor cache,
accumulating displacement and enqueueing a single AddLocationOffset
request per frame. Cleans up residual overlap left by the 50ms
sampling-latency window when two agents close faster than the sampler
can react.

Math: pen = (combinedRadius - dist) * 0.5 * 0.7 / dist, mirroring
dtCrowd's COLLISION_RESOLVE_FACTOR. Standard (4 iter) handles cascaded
chains where pushing A off B then runs A into C; Single is the cheap
80%-quality fallback."
```

---

## Task 7: Cleanup — Remove the Old Entity-Index Jitter

The `sin(handle.id * 0.123)` jitter we added in Gate 3 sub-task 3B was a stalemate-breaker for force-only behaviour. Phase 2's `wCurVel` does this job better and the jitter now adds noise. Remove.

**Files:**
- Modify: `Plugins/CkFoundation/Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Separation_Processor.cpp`

- [ ] **Step 7.1: Remove the jitter logic from the Separation processor**

In `CkCrowdAgent_Separation_Processor.cpp`, locate the post-`Force.IsNearlyZero()` block:

```cpp
        // Stalemate-breaking jitter: ...
        const auto EntityIndex = static_cast<float>(GetTypeHash(InHandle));
        const auto JitterPhase = EntityIndex * 0.123f;
        const auto Jitter = FVector{
            FMath::Sin(JitterPhase),
            FMath::Cos(JitterPhase),
            0.0};

        InSeparationForce._Force = (Force + Jitter * 0.05f) * SeparationWeight * MaxSpeed;
```

Replace with:

```cpp
        // Force scaling (Gate 3B unchanged). Jitter removed in Gate 3 hybrid: Phase 2's wCurVel
        // inertia bias breaks head-on stalemates more cleanly than per-agent sin/cos noise.
        InSeparationForce._Force = Force * SeparationWeight * MaxSpeed;
```

The Phase 1.1 inertia lerp (Task 2) wraps this output, so the early-out for `Force.IsNearlyZero()` still applies before the lerp runs.

- [ ] **Step 7.2: Build + verify behaviour unchanged**

Live Coding fine. PIE: `Ck_GymCrowd_Sep_HeadOnNS` should look identical to Task 5/6 result. Single-agent spawn should produce identical results.

- [ ] **Step 7.3: Commit**

```bash
git add Source/CkCrowd/Public/CkCrowd/Agent/CkCrowdAgent_Separation_Processor.cpp
git commit -m "feat(Crowd): remove entity-index jitter from force solver

Gate 3 Phase 1.1's inertia lerp + Phase 2's wCurVel inertia bias have
both eclipsed the original sin(entity_id * 0.123) stalemate-breaker.
Removing it cleans up the force vector for designers reading the
debugger and eliminates the 5% noise term that diluted the actual
separation contribution."
```

---

## Task 8: AutoTest Cases (UCk_AutoTest_Base pattern)

Three AutoTests — HeadOnPass, Convergence, Vibration. The codebase's autotest pattern is **NOT** the gym `Request_StartGym`/`Tick` pattern — it's the `UCk_AutoTest_Base` UObject pattern with `DoBeginPlay` + signal-driven `FinishSuccess`/`FinishFailure`. Reference pattern: `Plugins/CkTests/Script/CkCrowd/CkAutoTest_Crowd_Pathfinding_Success.as`.

Each test is **two classes per .as file**:
1. `UCk_AutoTest_X : UCk_AutoTest_Base` — the test entity script with `DoBeginPlay`, signal handlers, `FinishSuccess()` / `FinishFailure(msg)`.
2. `ACk_AutoTest_X_Actor : ACk_AutoTestRunner` — actor wrapper with `default _TestEntityScriptClass = UCk_AutoTest_X;`.

Tests are auto-discovered by the autotest runner from the `AutoTests_CkTests_Level.umap` map — no GymRegistry registration. Run via `Window → Test Automation → Run Tests` in the editor (UE's session frontend), filtered to "CkTests".

For continuous sampling (Vibration test) we need a per-frame callback. The pattern uses `utils_timer::Add` with `BindTo_OnUpdate` — a recurring timer signal. The Pathfinding test uses signal-driven completion (no timer); we'll use timers for the polled tests.

**Files:**
- Create: `Plugins/CkTests/Script/CkCrowd/CkAutoTest_Crowd_Separation_HeadOnPass.as`
- Create: `Plugins/CkTests/Script/CkCrowd/CkAutoTest_Crowd_Separation_Convergence.as`
- Create: `Plugins/CkTests/Script/CkCrowd/CkAutoTest_Crowd_Separation_Vibration.as`

- [ ] **Step 8.1: Read the reference pattern**

Read `Plugins/CkTests/Script/CkCrowd/CkAutoTest_Crowd_Pathfinding_Success.as` end-to-end. Note:
- `UCk_AutoTest_Base` provides `Assert_True`, `FinishSuccess()`, `FinishFailure(msg)`, `IsFinished()`.
- `DoBeginPlay(FCk_Handle InHandle)` — `InHandle` is the test's own entity. Spawn child entities (agents) via `utils_crowd_agent::Add(InHandle, params)` so they cascade-destroy with the test.
- World-origin transform on the test entity itself; spawn children at world coordinates.

Also read `Plugins/CkTests/Script/CkAudio/.../CkAutoTest_Audio_*.as` if present — usually one of those uses timer-driven polling and provides the canonical timer pattern.

- [ ] **Step 8.2: HeadOnPass autotest**

Path: `Plugins/CkTests/Script/CkCrowd/CkAutoTest_Crowd_Separation_HeadOnPass.as`

```as
// Language=angelscript
//============================================================================
// CK CROWD — AUTOMATION TEST: SEPARATION HEAD-ON PASS
//
// 2 agents 1500cm apart on a head-on course. Polls min-separation every 50ms.
// Asserts min-separation never drops below _Radius * 1.5 (= 63cm at default).
// Per Gate_03_Separation_Addendum.md §8.
//
// REQUIREMENT: test map needs a NavMeshBoundsVolume covering at least
// (-1000, -1000, 0) to (1000, 1000, 0). AutoTests_CkTests_Level satisfies this.
//============================================================================

class UCk_AutoTest_Crowd_Separation_HeadOnPass : UCk_AutoTest_Base
{
    private FCk_Handle_CrowdAgent _AgentA;
    private FCk_Handle_CrowdAgent _AgentB;
    private float _MinSeparation = 999999.0;
    private float _ElapsedSec = 0.0;
    private const float TimeoutSec = 8.0;
    private const float SampleIntervalSec = 0.05;
    private const float MinSepRequirement = 63.0;  // _Radius(42) * 1.5

    UFUNCTION(BlueprintOverride)
    void DoBeginPlay(FCk_Handle InHandle)
    {
        auto LocalHandle = InHandle;

        utils_transform::Add(LocalHandle,
            FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::OneVector),
            ECk_Replication::DoesNotReplicate);

        const auto SpawnA = FVector(-750.0, 0.0, 100.0);
        const auto SpawnB = FVector( 750.0, 0.0, 100.0);
        _AgentA = SpawnAgent(LocalHandle, SpawnA, SpawnB);
        _AgentB = SpawnAgent(LocalHandle, SpawnB, SpawnA);

        // Recurring sample timer — fires every SampleIntervalSec until we hit timeout.
        auto TimerParams = FCk_Fragment_Timer_ParamsData(FCk_Time(SampleIntervalSec));
        TimerParams.Set_StartingState(ECk_Timer_State::Running)
                   .Set_Behavior(ECk_Timer_Behavior::ResetOnDone);
        auto Timer = utils_timer::Add(LocalHandle, TimerParams);
        Timer.BindTo_OnDone(
            ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
            FCk_Delegate_Timer(this, n"OnSample"));
    }

    UFUNCTION()
    private void OnSample(FCk_Handle_Timer InTimer, FCk_Chrono InChrono, FCk_Time InDeltaT)
    {
        if (IsFinished()) { return; }
        if (ck::Is_NOT_Valid(_AgentA) || ck::Is_NOT_Valid(_AgentB)) { return; }

        _ElapsedSec += SampleIntervalSec;

        const auto LocA = utils_transform::Get_EntityCurrentLocation(utils_transform::DoCastChecked(FCk_Handle(_AgentA)));
        const auto LocB = utils_transform::Get_EntityCurrentLocation(utils_transform::DoCastChecked(FCk_Handle(_AgentB)));
        const auto Sep = FVector::Distance(LocA, LocB);
        if (Sep < _MinSeparation) { _MinSeparation = Sep; }

        if (_ElapsedSec > TimeoutSec)
        {
            Assert_True(_MinSeparation >= MinSepRequirement,
                f"min-separation {_MinSeparation} < required {MinSepRequirement}cm — agents clipped during head-on pass");
            FinishSuccess();
        }
    }

    private FCk_Handle_CrowdAgent SpawnAgent(FCk_Handle& InOwner, FVector InSpawn, FVector InTarget)
    {
        auto Params = FCk_Fragment_CrowdAgent_ParamsData(42.0f, 192.0f);
        auto Agent = utils_crowd_agent::Add(InOwner, Params);

        FCk_Handle Generic = Agent;
        const auto Rot = (InTarget - InSpawn).Rotation();
        utils_transform::Add(Generic, FTransform(Rot, InSpawn, FVector::OneVector), ECk_Replication::DoesNotReplicate);
        utils_velocity::Add(Generic, FCk_Fragment_Velocity_ParamsData(ECk_LocalWorld::World, FVector::ZeroVector), ECk_Replication::DoesNotReplicate);
        utils_acceleration::Add(Generic, FCk_Fragment_Acceleration_ParamsData(ECk_LocalWorld::World, FVector::ZeroVector), ECk_Replication::DoesNotReplicate);
        utils_euler_integrator::Request_Start(Generic);
        utils_crowd_agent::Request_MoveTo(Agent, FCk_Request_CrowdAgent_MoveTo(InTarget));
        return Agent;
    }
}

class ACk_AutoTest_Crowd_Separation_HeadOnPass_Actor : ACk_AutoTestRunner
{
    default _TestEntityScriptClass = UCk_AutoTest_Crowd_Separation_HeadOnPass;
}
```

- [ ] **Step 8.3: Convergence autotest**

Path: `Plugins/CkTests/Script/CkCrowd/CkAutoTest_Crowd_Separation_Convergence.as`

```as
// Language=angelscript
//============================================================================
// CK CROWD — AUTOMATION TEST: SEPARATION CONVERGENCE
//
// 5 agents around a 600cm circle, all targeting the centre. After 6s asserts
// all 5 are within 200cm of target and no pair of final positions is within
// _Radius * 2 (= 84cm). Per Gate_03_Separation_Addendum.md §8.
//============================================================================

class UCk_AutoTest_Crowd_Separation_Convergence : UCk_AutoTest_Base
{
    private TArray<FCk_Handle_CrowdAgent> _Agents;
    private const float TimeoutSec = 6.0;
    private const float ArrivalWindow = 200.0;
    private const float MinPairwiseSep = 84.0;
    private const FVector Centre = FVector(0.0, 0.0, 100.0);

    UFUNCTION(BlueprintOverride)
    void DoBeginPlay(FCk_Handle InHandle)
    {
        auto LocalHandle = InHandle;

        utils_transform::Add(LocalHandle,
            FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::OneVector),
            ECk_Replication::DoesNotReplicate);

        const auto Radius = 600.0;
        const auto Count = 5;
        const auto Step = (2.0 * Math::PI) / float(Count);
        for (int32 i = 0; i < Count; ++i)
        {
            const auto Angle = Step * float(i);
            const auto Spawn = Centre + FVector(Radius * Math::Cos(Angle), Radius * Math::Sin(Angle), 0.0);
            _Agents.Add(SpawnAgent(LocalHandle, Spawn, Centre));
        }

        // One-shot timer fires after TimeoutSec — then we evaluate.
        auto TimerParams = FCk_Fragment_Timer_ParamsData(FCk_Time(TimeoutSec));
        TimerParams.Set_StartingState(ECk_Timer_State::Running)
                   .Set_Behavior(ECk_Timer_Behavior::DestroyOnDone);
        auto Timer = utils_timer::Add(LocalHandle, TimerParams);
        Timer.BindTo_OnDone(
            ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
            FCk_Delegate_Timer(this, n"OnTimeout"));
    }

    UFUNCTION()
    private void OnTimeout(FCk_Handle_Timer InTimer, FCk_Chrono InChrono, FCk_Time InDeltaT)
    {
        if (IsFinished()) { return; }

        TArray<FVector> Finals;
        for (auto Agent : _Agents)
        {
            if (ck::Is_NOT_Valid(Agent))
            {
                FinishFailure("an agent went invalid before timeout — possible early destroy / replication issue");
                return;
            }
            const auto Loc = utils_transform::Get_EntityCurrentLocation(utils_transform::DoCastChecked(FCk_Handle(Agent)));
            Assert_True(FVector::Distance(Loc, Centre) <= ArrivalWindow,
                f"agent failed to reach within {ArrivalWindow}cm of centre — final {FVector::Distance(Loc, Centre)}cm");
            Finals.Add(Loc);
        }
        for (int32 i = 0; i < Finals.Num(); ++i)
        {
            for (int32 j = i + 1; j < Finals.Num(); ++j)
            {
                const auto PairSep = FVector::Distance(Finals[i], Finals[j]);
                Assert_True(PairSep >= MinPairwiseSep,
                    f"agents {i} and {j} ended within {MinPairwiseSep}cm — pile-up at goal ({PairSep}cm)");
            }
        }
        FinishSuccess();
    }

    private FCk_Handle_CrowdAgent SpawnAgent(FCk_Handle& InOwner, FVector InSpawn, FVector InTarget)
    {
        auto Params = FCk_Fragment_CrowdAgent_ParamsData(42.0f, 192.0f);
        auto Agent = utils_crowd_agent::Add(InOwner, Params);
        FCk_Handle Generic = Agent;
        const auto Rot = (InTarget - InSpawn).Rotation();
        utils_transform::Add(Generic, FTransform(Rot, InSpawn, FVector::OneVector), ECk_Replication::DoesNotReplicate);
        utils_velocity::Add(Generic, FCk_Fragment_Velocity_ParamsData(ECk_LocalWorld::World, FVector::ZeroVector), ECk_Replication::DoesNotReplicate);
        utils_acceleration::Add(Generic, FCk_Fragment_Acceleration_ParamsData(ECk_LocalWorld::World, FVector::ZeroVector), ECk_Replication::DoesNotReplicate);
        utils_euler_integrator::Request_Start(Generic);
        utils_crowd_agent::Request_MoveTo(Agent, FCk_Request_CrowdAgent_MoveTo(InTarget));
        return Agent;
    }
}

class ACk_AutoTest_Crowd_Separation_Convergence_Actor : ACk_AutoTestRunner
{
    default _TestEntityScriptClass = UCk_AutoTest_Crowd_Separation_Convergence;
}
```

- [ ] **Step 8.4: Vibration autotest**

Path: `Plugins/CkTests/Script/CkCrowd/CkAutoTest_Crowd_Separation_Vibration.as`

```as
// Language=angelscript
//============================================================================
// CK CROWD — AUTOMATION TEST: SEPARATION VIBRATION
//
// 2 agents head-on. Samples agent A's _DesiredVelocity direction every 50ms
// for 3s. Asserts max angular delta between consecutive samples < 30°.
// Catches re-introduction of the Phase 1 vibration bug (force/path-follow
// fighting at full strength).
//============================================================================

class UCk_AutoTest_Crowd_Separation_Vibration : UCk_AutoTest_Base
{
    private FCk_Handle_CrowdAgent _AgentA;
    private FCk_Handle_CrowdAgent _AgentB;
    private FVector _LastDir = FVector::ZeroVector;
    private float _MaxDeltaDeg = 0.0;
    private float _ElapsedSec = 0.0;
    private const float SampleIntervalSec = 0.05;
    private const float TestDurationSec = 3.0;
    private const float MaxAllowedDeltaDeg = 30.0;

    UFUNCTION(BlueprintOverride)
    void DoBeginPlay(FCk_Handle InHandle)
    {
        auto LocalHandle = InHandle;

        utils_transform::Add(LocalHandle,
            FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::OneVector),
            ECk_Replication::DoesNotReplicate);

        const auto SpawnA = FVector(-750.0, 0.0, 100.0);
        const auto SpawnB = FVector( 750.0, 0.0, 100.0);
        _AgentA = SpawnAgent(LocalHandle, SpawnA, SpawnB);
        _AgentB = SpawnAgent(LocalHandle, SpawnB, SpawnA);

        auto TimerParams = FCk_Fragment_Timer_ParamsData(FCk_Time(SampleIntervalSec));
        TimerParams.Set_StartingState(ECk_Timer_State::Running)
                   .Set_Behavior(ECk_Timer_Behavior::ResetOnDone);
        auto Timer = utils_timer::Add(LocalHandle, TimerParams);
        Timer.BindTo_OnDone(
            ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
            FCk_Delegate_Timer(this, n"OnSample"));
    }

    UFUNCTION()
    private void OnSample(FCk_Handle_Timer InTimer, FCk_Chrono InChrono, FCk_Time InDeltaT)
    {
        if (IsFinished()) { return; }
        if (ck::Is_NOT_Valid(_AgentA)) { return; }

        _ElapsedSec += SampleIntervalSec;

        const auto Vel = utils_crowd_agent::Get_DesiredVelocity(_AgentA);
        if (Vel.IsNearlyZero() == false)
        {
            const auto Dir = Vel.GetSafeNormal();
            if (_LastDir.IsNearlyZero() == false)
            {
                const auto DotProduct = FVector::DotProduct(_LastDir, Dir);
                const auto AngleRad = Math::Acos(FMath::Clamp(DotProduct, -1.0, 1.0));
                const auto AngleDeg = AngleRad * (180.0 / Math::PI);
                if (AngleDeg > _MaxDeltaDeg) { _MaxDeltaDeg = AngleDeg; }
            }
            _LastDir = Dir;
        }

        if (_ElapsedSec > TestDurationSec)
        {
            Assert_True(_MaxDeltaDeg <= MaxAllowedDeltaDeg,
                f"max angular delta {_MaxDeltaDeg}° exceeds {MaxAllowedDeltaDeg}° — vibration regression");
            FinishSuccess();
        }
    }

    private FCk_Handle_CrowdAgent SpawnAgent(FCk_Handle& InOwner, FVector InSpawn, FVector InTarget)
    {
        auto Params = FCk_Fragment_CrowdAgent_ParamsData(42.0f, 192.0f);
        auto Agent = utils_crowd_agent::Add(InOwner, Params);
        FCk_Handle Generic = Agent;
        const auto Rot = (InTarget - InSpawn).Rotation();
        utils_transform::Add(Generic, FTransform(Rot, InSpawn, FVector::OneVector), ECk_Replication::DoesNotReplicate);
        utils_velocity::Add(Generic, FCk_Fragment_Velocity_ParamsData(ECk_LocalWorld::World, FVector::ZeroVector), ECk_Replication::DoesNotReplicate);
        utils_acceleration::Add(Generic, FCk_Fragment_Acceleration_ParamsData(ECk_LocalWorld::World, FVector::ZeroVector), ECk_Replication::DoesNotReplicate);
        utils_euler_integrator::Request_Start(Generic);
        utils_crowd_agent::Request_MoveTo(Agent, FCk_Request_CrowdAgent_MoveTo(InTarget));
        return Agent;
    }
}

class ACk_AutoTest_Crowd_Separation_Vibration_Actor : ACk_AutoTestRunner
{
    default _TestEntityScriptClass = UCk_AutoTest_Crowd_Separation_Vibration;
}
```

- [ ] **Step 8.5: Run all three autotests via the Session Frontend**

AS hot-reloads on PIE entry. In the editor:
1. **Window → Test Automation** (or **Window → Developer Tools → Session Frontend** in older UE versions)
2. Filter: `CkTests.CkAutoTest_Crowd_Separation_*`
3. Run all three. Each launches into `AutoTests_CkTests_Level`, runs the test, reports pass/fail.

Expected:
- HeadOnPass passes within 8s with min-separation in the 60-100cm range.
- Convergence passes within 6s with all final positions inside the arrival window and no pair within 84cm.
- Vibration passes within 3s with max angular delta well under 30°.

If HeadOnPass fails: confirm `_AvoidanceSampleTrigger != Disabled` in Project Settings and `_PushApartMode = Standard`.
If Vibration fails: regression in Phase 1 — review Tasks 2-3.
If any test errors with "NoNavData" or similar: the test map's NavMeshBoundsVolume needs to cover at least (-1000, -1000, 0) to (1000, 1000, 0). Check `AutoTests_CkTests_Level.umap` in the editor; expand the volume if needed.

- [ ] **Step 8.6: Commit**

```bash
cd /d/Repos/CkPlugins/Plugins/CkTests
git add Script/CkCrowd/CkAutoTest_Crowd_Separation_HeadOnPass.as Script/CkCrowd/CkAutoTest_Crowd_Separation_Convergence.as Script/CkCrowd/CkAutoTest_Crowd_Separation_Vibration.as
git commit -m "feat(CrowdAutoTest): HeadOnPass + Convergence + Vibration tests

Three UCk_AutoTest_Base autotests cover the Gate 3 acceptance criteria:

- HeadOnPass: 2 agents at 1500cm head-on; min-separation must never
  drop below _Radius*1.5 (=63cm at default).
- Convergence: 5 agents around a 600cm circle, all targeting centre;
  must arrive within 200cm of target by 6s with no pairwise final
  position closer than _Radius*2 (=84cm).
- Vibration: 2 agents head-on; samples _DesiredVelocity direction every
  50ms via a recurring timer; asserts max angular delta <30deg over 3s.
  Catches Phase 1 vibration regressions directly.

Run via Window > Test Automation, filtered to
CkTests.CkAutoTest_Crowd_Separation_*. Tests share AutoTests_CkTests_
Level and require its NavMeshBoundsVolume to cover at least
(-1000,-1000,0) to (1000,1000,0)."
```

---

## Task 9: Update PLAN.md + Bump Submodule

Final wiring: mark Gate 3 done, bump parent.

**Files:**
- Modify: `Plugins/CkFoundation/Source/CkNavigation/PLAN.md` — mark Gate 3 row ✅ Done

- [ ] **Step 9.1: Read the current PLAN.md to understand the status table format**

Read `Plugins/CkFoundation/Source/CkNavigation/PLAN.md`. Locate the Gate 3 row.

- [ ] **Step 9.2: Update Gate 3 row to ✅ Done**

In place, change the status indicator and any "TBD" / "⏳" markers to ✅ Done. Add a one-line note pointing to the addendum:

> Gate 3 Done — Hybrid avoidance per `Plan/Gate_03_Separation_Addendum.md`.

- [ ] **Step 9.3: Commit PLAN.md update inside CkFoundation**

```bash
cd /d/Repos/CkPlugins/Plugins/CkFoundation
git add -f Source/CkNavigation/PLAN.md
git commit -m "docs(Navigation): Gate 3 hybrid avoidance done — mark in PLAN.md"
```

- [ ] **Step 9.4: Bump submodules in the parent repo**

```bash
cd /d/Repos/CkPlugins
wait_for_git && git add Plugins/CkFoundation Plugins/CkTests
git commit -m "chore: bump submodules — Gate 3 hybrid done"
```

(Note: CkGameplayDebugger doesn't need a bump — Gate 3's debugger work landed under `d4b1bf6` and was bumped previously. Confirm with `git -C Plugins/CkGameplayDebugger log --oneline -3` showing nothing newer than `d4b1bf6`.)

- [ ] **Step 9.5: Final verification**

```bash
cd /d/Repos/CkPlugins
git status      # expected: clean
git -C Plugins/CkFoundation log --oneline -12   # expected: tasks 1-9 visible
```

Gate 3 hybrid is shipped.

---

## Notes for the executing engineer

- **Each Task ends with a commit.** Don't batch multiple tasks into one commit — they're sized so one task = one logical change.
- **Live Coding vs IDE rebuild:** any task that introduces a `UCLASS` (Tasks 1, 4) or a new `CK_REGISTER_PROCESSOR` (Tasks 3, 5, 6) requires a full editor restart. Tasks that only modify .cpp body or non-USTRUCT data (Tasks 2, 7) can use Live Coding.
- **AS reload:** Task 8 is .as-only and reloads in PIE without an editor restart — but a fresh PIE session is needed for a fresh AutoStation run.
- **Project settings test path:** every algorithm enum has both an "on" and "off" position. After completing a task that introduces a new enum, toggle it once at default and once at Disabled to confirm the path works in both states.
- **PIE smoke order:** Foundation gym → Locomotion gym → Separation gym. Verify Gates 0-2 still work before celebrating Gate 3.

---

## Self-review

This plan covers every section of the addendum:

- §3 reference tables — cited inline in Tasks 3, 5, 6 (formula sources)
- §4 Phase 1 — Tasks 2 (inertia) + 3 (AccelClamp)
- §5 Phase 2 — Tasks 4 (policy + tags) + 5 (sampler) + 6 (push-apart)
- §6 Phase 3 — explicitly out of scope; no task
- §7 disposition table — Task 7 (jitter cleanup) handles the explicit cleanup item
- §8 AutoStations — Task 8
- §9 perf budget — implicit; testable via PIE if profiling matters
- §10 deferred items — none in this plan (correct — they're deferred)
- §11 commit boundaries — match the order of Tasks 1-9

Every project setting and per-agent override has a designer-visible path. Every behaviour decision the addendum defends is exercised by either an AutoStation (HeadOnPass / Convergence / Vibration) or a manual gym scenario referenced in PIE smoke steps.
