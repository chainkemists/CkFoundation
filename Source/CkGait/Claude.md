# CkGait

Two features in one module. **Gait** (`FCk_Handle_Gait`) is the locomotion rhythm of a mover: a stride
clock (phase, amount, breath phase) and a landing counter, sampled every frame from the `UNavMovementComponent`
its spec names. **Bob** (`FCk_Handle_Bob`) is a scene node whose offset is a procedural
locomotion bob shaped by a Gait (head bob, hand bob). One Gait can drive any number of Bobs, so every bob of a
character stays in phase. **Bob owns its node's offset.**

## Composition and requests

### Gait

`UCk_Utils_Gait_UE::Add(Handle, FCk_Gait_Spec)` puts the Gait on `Handle`, which may be any entity: the motion
source is the spec's `TWeakObjectPtr<UNavMovementComponent> _MovementComponent` (the `FCk_Camera_Spec::_OutputComponent`
shape), never an actor found on or above the entity. Any `UNavMovementComponent` is a valid source (character,
floating pawn, spectator: every pawn movement; projectile and rotating movement have no footing and are excluded
by type). The spec lives in `ck::FFragment_Gait_Tunables`, so `Request_UpdateSpec` re-points the source. There is
no Setup processor. `_StartingState = Disable` adds `FTag_Gait_Disabled` at Add.

`Add` ensures and returns an invalid handle when the handle is invalid, when it is already a gait
(`already a gait`), when the spec's tunables fail `ck::gait::Get_AreTunablesValid` (`invalid spec`), or when the
spec's movement component is not valid (`the spec's movement component is not valid`).
`ck::gait::Get_IsSpecValid` is both checks together; `Request_UpdateSpec` validates with it at enqueue and at
drain.

Reads: `Get_Spec`, `Get_IsEnabled`, `Get_Phase` (radians `[0, 2pi)`), `Get_Amount`, `Get_SpeedRatio`,
`Get_BreathPhase`, `Get_LastMotion` (`FCk_Gait_Motion { _Velocity (world cm/s), _Footing, _Stance }`),
`Get_LandingCount`, `Get_LastLandImpactSpeed`.

### Bob

`UCk_Utils_Bob_UE::Add(SceneNode, FCk_Bob_Spec)` adds Bob onto an EXISTING scene node and takes ownership
of its offset: the node's current offset becomes the **rest offset** (`Get_RestOffset`), and from then on Bob
writes `node offset = BobOffset * Rest` (`ck::bob::Compose_NodeOffset`; the bob is applied in the rest frame,
then the rest). `UCk_Utils_Bob_UE::Create(ParentTransform, LocalRest, FCk_Bob_Spec)` is
`UCk_Utils_SceneNode_UE::Create(Parent, LocalRest)` + `Add` (lifetime child of the parent, debug name
`Bob(<parent>)`). Attach content under the bob node's Transform. The gait is the spec's `FCk_Handle_Gait _Gait`
(the movement-component shape of the Gait spec): a config-authored spec leaves it unset and receives it at
composition, e.g. `auto Spec = Config.HeadBob; Spec.Set_Gait(Gait); utils_bob::Create(Parent, Rest, Spec);`.

`Add` ensures and returns an invalid handle when the node is invalid or not a SceneNode, when it is already a
bob node (`already a bob node`), when the spec's gait is invalid or not a Gait, or when the spec fails
`ck::bob::Get_IsSpecValid`. `Create` ensures and returns an invalid handle when the parent is invalid or has no
Transform, when the spec is invalid, when the spec's gait is invalid or not a Gait, when the rest offset is non-finite,
when the parent cannot create children, or when the scene node could not be created. Every rejection happens
before the scene node exists, so a rejected `Create` leaves nothing under the parent.

Reads: `Get_Spec`, `Get_Gait`, `Get_IsEnabled`, `Get_RestOffset`, `Get_BobOffset` (the bob-only offset: the
current smoothed, clamped target composed as a transform, in the rest frame), `Get_SpringOffset` (cm, before
intensity).

### Requests

Both `ck::FFragment_Gait_Tunables` and `ck::FFragment_Bob_Tunables` alias their Spec wholesale. They are
request-mutable, not `_Params`. Every request completes its delegate once: boundary rejection
`Failed_NotEnqueued`, drain failure `Failed`, done `Succeeded`, owner teardown `Failed_Cancelled`.

- `Request_UpdateSpec` (Gait, Bob) replaces the tunables. An invalid spec is rejected with an ensure at enqueue.
  On a Bob it also re-points the gait: the spec's gait is validated like `Add` does (at enqueue and at drain), and a
  changed gait re-seeds the consumed landing count from the new gait, so its past landings do not kick the spring.
- `Request_EnableDisable` (Gait, Bob) toggles `FTag_Gait_Disabled` / `FTag_Bob_Disabled`. A disabled Gait
  samples rest motion (zero velocity, grounded, standing): Amount relaxes, the phase idles, no landings are
  counted. A disabled Bob reads its gait as rest: it relaxes to its rest offset; it does not freeze. Enabling a
  Bob re-seeds its consumed landing count from the gait, so a landing the gait counted while the bob was
  disabled does not kick the spring on re-enable.
- `Request_Reset` on a Gait zeroes the clock (phase, amount, speed ratio, breath phase) and the previous motion
  sample; the landing counter and the last impact speed are NOT reset. On a Bob it zeroes the spring and the
  smoothed target and re-seeds the consumed landing count from the gait (0 when the gait is gone); the node
  returns to rest through Update's normal publish path.
- `Request_SetRestOffset` (Bob) replaces the rest offset (a non-finite rest is rejected with an ensure at
  enqueue). This is the only legal way to move the rest pose of a bob node: Update composes
  `BobOffset * NewRest`, sees it differ from the node's actual offset and republishes, so no foreign-write
  ensure fires.

## The model

### Gait (`ck::gait`, `CkGait_Kernel.h`)

Each frame `FProcessor_Gait_Update` samples one motion from the spec's movement component: `Velocity`,
`IsFalling()` → `Airborne` else `Grounded`, `IsCrouching()` → `Crouched` else `Standing` (the last two are
`INavMovementInterface` members, which is why the source is a nav movement component). Then
`ck::gait::Step_Clock`:

- `SpeedRatio = GroundSpeed / _Stride._ReferenceSpeed`, with `GroundSpeed = Velocity.Size2D()` while grounded and 0
  while airborne (`Compute_GroundSpeed`)
- `TargetAmount = min(SpeedRatio, _MaxAmountScale) x (Crouched ? _CrouchScale : 1)` (all `_Stride`)
- `Amount += (TargetAmount - Amount) x (1 - exp(-_Stride._AmountInterpSpeed x dt))`
- `Phase = wrap2pi(Phase + 2pi x _StridesPerSecond x max(SpeedRatio, _MinCadenceScale) x dt)` (all `_Stride`): the clock keeps
  turning slowly at rest so the next step does not restart at a fixed phase
- `BreathPhase = wrap2pi(BreathPhase + 2pi x dt / _BreathPeriodSeconds)`
- no-op when dt is not positive or non-finite

Landing edge (`ck::gait::Detect_Landing`): previous sample Airborne and current Grounded →
`_LandingCount += 1`, `_LastLandImpactSpeed = max(-PrevVelocity.Z, 0)`. A monotonic counter, not a one-frame
flag, so consumers and tests are immune to processor ordering: each consumer diffs it against its own copy.

### Bob (`ck::bob`, `CkBob_Kernel.h`)

With `A = Amount`, `Step = |sin phase|`, `Sway = sin phase` from the gait clock:

- stride location (rest frame, X fwd / Y right / Z up, cm), `Compute_StrideTarget` (amplitudes from `_Stride`):
  `(_ForwardCm x A, _LateralCm x Sway x A, -_VerticalCm x Step x A + _BreathCm x sin(breathPhase) x (1 - A))`
- stride rotation (Roll, Pitch, Yaw deg): `(_RollDeg x Sway x A, -_PitchDeg x Step x A, 0)` (`_Stride`)
- vertical spring (`_Air._Spring`, an `FCk_Bob_SpringResponse { _FrequencyHz, _DampingRatio }`, `Step_Spring`): target =
  airborne ? `clamp(-VerticalSpeed x _Air._LiftCmPerFallSpeed, +-_Air._MaxLiftCm)` : 0 (`Compute_AirLiftTarget`); on
  a new landing (the gait's landing count moved past `_ConsumedLandingCount`) the spring velocity drops by
  `min(Impact x _Air._LandKickPerImpactSpeed, _Air._MaxLandKick)` (`Compute_LandKick`); semi-implicit Euler in fixed
  1/120 s substeps (`kSpringStepSeconds`), the frame capped at 0.1 s (`kMaxSpringFrameSeconds`); a NaN state or
  `|offset| > 100 cm` (`kSpringRunawayCm`) reseeds at rest
- target = (stride location + (0, 0, spring), stride rotation) x `_Intensity` (`Compute_Target`)
- lag (`Smooth`): `smoothed += (target - smoothed) x (1 - exp(-_LagRate x dt))`; `_LagRate <= 0`
  snaps
- location clamped to `_MaxOffsetCm` in magnitude (`Clamp_Location`); node offset =
  `Compose_Offset(location, rotation) * Rest`, with `Compose_Offset` = `FTransform{FRotator{Pitch, Yaw, Roll},
  Location}`

The offset is bounded by construction: every stride term is bounded by its amplitude and Amount, the spring is
reseeded on runaway, and the location is clamped last.

### Spec shape

Specs past roughly seven fields nest related fields into sub-structs (the `FCk_CameraProfile` shape). Defaults
in parentheses.

| `FCk_Gait_Spec` | Holds |
|---|---|
| `_MovementComponent` (Source) | `TWeakObjectPtr<UNavMovementComponent>`, constructor-essential |
| `_Stride` (`FCk_Gait_StrideParams`) | `_ReferenceSpeed` (420), `_StridesPerSecond` (1.6), `_MaxAmountScale` (1.5), `_AmountInterpSpeed` (7), `_CrouchScale` (0.6), `_MinCadenceScale` (0.35) |
| `_BreathPeriodSeconds` | 3.6 |
| `_StartingState` | Enable |

Constructors: `FCk_Gait_Spec(MovementComponent, Stride)`, `FCk_Gait_StrideParams(ReferenceSpeed, StridesPerSecond)`.
Validation: `ck::gait::Get_IsStrideValid` + the breath period (`Get_AreTunablesValid`).

| `FCk_Bob_Spec` | Holds |
|---|---|
| `_Gait` (Source) | `FCk_Handle_Gait`, set at composition |
| `_Stride` (`FCk_Bob_StrideParams`) | `_VerticalCm` (1.6), `_LateralCm` (1.2), `_ForwardCm` (0), `_RollDeg` (2.5), `_PitchDeg` (1.5) |
| `_Air` (`FCk_Bob_AirParams`) | `_LiftCmPerFallSpeed` (0.006), `_MaxLiftCm` (5), `_LandKickPerImpactSpeed` (0.09), `_MaxLandKick` (70), `_Spring` (3 Hz, 0.35) |
| `_LagRate` | 0 |
| `_BreathCm` | 0.4 |
| `_Intensity`, `_MaxOffsetCm` | 1, 20 |
| `_StartingState` | Enable |

Constructors: `FCk_Bob_Spec(Gait, Stride, Air)`, `FCk_Bob_StrideParams(VerticalCm, LateralCm)`,
`FCk_Bob_AirParams(LiftCmPerFallSpeed, MaxLiftCm)`. Validation (`ck::bob::Get_IsSpecValid`, tunables only, not the
gait): `Get_IsStrideValid` (roll/pitch capped at 45 deg), `Get_IsAirValid` (spring frequency > 0) and the rest.
From AngelScript, edit a sub-struct by copy: `auto Stride = Spec.Get_Stride(); Stride.Set_VerticalCm(6.0f);
Spec.Set_Stride(Stride);`.

## Ordering

`FProcessor_Gait_HandleRequests` → `FProcessor_Gait_Update` → `FProcessor_Bob_HandleRequests` →
`FProcessor_Bob_Update`, all in **`FGroup_Gameplay`** (`FProcessor_Bob_Update` `RunAfter` both
`FProcessor_Gait_Update` and `FProcessor_Bob_HandleRequests`). Bob reads no transform, only its gait's state, so
it does not need the Derived band: running before `FGroup_Transform` means its
`UCk_Utils_SceneNode_UE::Request_UpdateOffset` is drained by `FProcessor_SceneNode_HandleRequests`
(`FGroup_Transform`) in the SAME frame, the node composes with the new offset in that pass, and a consumer in
`FGroup_Transform_Derived` (e.g. `FProcessor_Camera_UpdatePOV` for a camera director on a bob node) sees it that
frame. Bob publishes only when the new node offset differs from the node's actual offset.

Neither Update processor has a `MarkedDirtyBy`: a disabled bob must still relax and a gait must idle its phase.
Gait samples the movement component during the ECS tick (TG_PrePhysics); `UCharacterMovementComponent` ticks in
the same group with no guaranteed order, so the sampled velocity may be this frame's or last frame's. For a
smoothed clock this is imperceptible; documented, not fixed. Cancel processors run in `FGroup_EndPlay`. Never
write a Transform from Gait or Bob; Bob mutates only its own node's offset.

## Robustness

- **Two teardown artifacts relax to rest silently, by design.** A movement component that dies after a valid
  Add (its owner torn down before the entity) makes the Gait sample rest motion. A destroyed or stripped Gait
  makes its Bobs read rest (Amount 0, grounded, no landings). Neither ensures: the player entity, its actor and
  its bob nodes die in the same pass, so an ensure would fire at every pawn death.
- A foreign `Request_UpdateOffset` on a bob node is detected by Update (actual offset differs from the last
  offset Bob wrote): it ensures once per node, latched by `FTag_Bob_ForeignOffsetReported`, and Bob overwrites
  the offset.
- A bob node whose SceneNode fragments were removed (e.g. `Request_Detach`) ensures in Update and stops
  publishing.
- Not replicated (the SceneNode Transform is `DoesNotReplicate`) and not snapshotable: gait and bob are
  cosmetic and local; composition rebuilds them. No snapshot registrations belong in this module.

## Diagnostics and tests

No signals in v1.

- **Debug draw:** `UCk_Gait_DebugSettings_UE::_DrawBobs` (Project Settings, "Gait Debug") or cvar
  `ck.Gait.DrawBobs` (cheat) turns on `ck::FProcessor_Gait_DebugDraw` (`FGroup_Gameplay_Camera`). Per bob: the
  node's world axes (10 cm), a 3 cm point at the node, a line from the rest pose (parent world x rest) to the
  node, and the readout `Gait phase .. A .. | Bob loc (..) rot (..) spring .. land#..` (`gait -` when the gait
  is gone; `land#` is the landing count the bob has consumed). Red = disabled, yellow = foreign offset write
  reported, green otherwise.
- **C++ unit rows:** `Ck.Gait.Kernel.*` (10 rows) and `Ck.Gait.BobKernel.*` (11 rows), world-free
  (`CkTests/Private/UnitTests/CkGait/Test_Gait_Kernel.cpp`, `Test_Bob_Kernel.cpp`).
- **AS autotests:** `Ck_AutoTest_Gait_*` (7 rows) and `Ck_AutoTest_Gait_Bob_*` (9 rows) in `CkTests/Script/CkGait/`,
  on `ACk_GaitAutoTest_Character` (`CkGaitAutoTest_Fixture.as`, whose `AddGait` puts the character's movement
  component on the spec): an unpossessed character, so its movement component runs no physics and a scripted velocity / movement mode is sampled exactly (Flying = grounded and
  moving, Falling = airborne, Falling -> Flying = a landing).
- **Gym:** "Gait" (`CkTests/Script/CkGait/CkGaitGym_*.as`), one station `Gym.CkGait.Walker`: a treadmill walker
  scripted +X / -X at 420 cm/s with a hop every 7 s, a raw bob cube and its rigid twin at head height, and a
  lagged bob cube (`_LagRate` 14) above them. O toggles `ck.Gait.DrawBobs`.

## Boundaries

The kernels (`CkGait_Kernel.h`, `CkBob_Kernel.h`) are world-free and reusable without an entity; the Gait kernel
takes a motion sample, so a second producer (a gait source that is not a movement component) is additive. Gait
reads ONLY the spec's movement component (`Velocity`, `IsFalling()`, `IsCrouching()`); it has no Character or
OwningActor dependency and never reads the camera, controller, input or a transform.
Bob reads ONLY its Gait and its own node. This module depends on CkEcsExt (SceneNode/Transform) but never
modifies it, and has no dependency on CkCamera, CkSway, CkTween or CkGameplayDebugger.
