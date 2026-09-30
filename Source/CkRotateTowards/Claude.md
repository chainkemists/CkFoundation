# CkRotateTowards

A transform entity turns to face a target transform entity, per Euler axis (Pitch, Yaw, Roll), at a bounded
rate or instantly, with optional per-axis locks and optional per-axis angular ranges measured from a rest
rotation. Typical use: turrets, heads, gimbals, NPC facing. **RotateTowards writes rotation only through
Transform/SceneNode requests** — never a Pawn, a Controller, a component, or a fragment write.

## Composition and requests

`UCk_Utils_RotateTowards_UE::Add(Transform, FCk_RotateTowards_Spec)` adds the feature ONTO an existing
transform entity; the returned `FCk_Handle_RotateTowards` is the same entity. `Has(Handle)` is the query;
there is no record and no child entity. `Add` composes everything synchronously (there is no Setup
processor): the spec's tunables become `ck::FFragment_RotateTowards_Tunables` (an alias of
`FCk_RotateTowards_Tunables`, request-mutable), its target becomes `ck::FFragment_RotateTowards::_Target`
(an invalid target means "no target yet"), its range clamp becomes `ck::FFragment_RotateTowards_RangeClamp`
only when at least one axis range is enabled, and `_StartingState == Disable` becomes
`FTag_RotateTowards_Disabled`. `ck::FFragment_RotateTowards_SolveResult` (last desired rotation, remaining per-axis
delta, last applied rotation) exists exactly while `FTag_RotateTowards_HasTarget` does: both are added
together when a target is set and removed together when it clears or is lost. Absence means "no solve".

`Add` ensures and returns an invalid handle when the handle is invalid or has no Transform, when the entity
already has the feature, when the tunables fail `ck::rotate_towards::Get_IsTunablesValid`, when the range
clamp fails `Get_IsRangeClampValid`, when an enabled range clamp's rest reference point is invalid or is the
entity itself, and when the target is the entity itself.

Requests (struct payload + completion delegate; each completes once: boundary rejection
`Failed_NotEnqueued`, drain failure `Failed`, done `Succeeded`, owner teardown `Failed_Cancelled`):

- `Request_SetTarget` — rejected at enqueue for an invalid target (use `Request_ClearTarget`) or the entity
  itself. At drain: the same target succeeds with no signal; a target destroyed since enqueue fails quietly
  (VeryVerbose log); otherwise the target is replaced, `HasTarget` is added, `AtTarget` is removed, the
  SolveResult fragment is added (or reset) and `OnTargetChanged(Handle, New, Previous)` fires.
- `Request_ClearTarget` — idempotent. Clears the target and removes both tags and the SolveResult fragment, then fires
  `OnTargetCleared(Handle, Previous, Requested)`.
- `Request_UpdateTunables` — replaces the tunables; invalid tunables are rejected at enqueue and re-checked
  at drain.
- `Request_SetRangeClamp` — replaces the whole clamp. Rejected at enqueue when no axis is enabled (use
  `Request_ClearRangeClamp`), when invalid, or when the rest reference point is invalid or the entity itself;
  enabled-axis, validity and rest-validity are re-checked at drain.
- `Request_ClearRangeClamp` — removes the clamp fragment; idempotent.
- `Request_EnableDisable` — toggles `FTag_RotateTowards_Disabled`. A disabled entity holds its rotation and
  keeps its target; requests still drain and target changes still signal.

Signals (`BindTo_`/`UnbindFrom_`, binding policy + post-fire behavior): `OnTargetChanged`, `OnTargetReached`,
`OnTargetCleared` (reason `Requested` or `TargetLost`; the previous target is a dead handle on `TargetLost`).

## The model

The kernel (`CkRotateTowards_Kernel.h`, `ck::rotate_towards`) is world-free. Each frame, for an enabled
entity with a target:

1. `Compute_LookAtRotation(From, To)` = `(To - From).Rotation()`; unset when either point is non-finite or
   they are closer than `kCoincidentDistanceCm` (0.01 cm).
2. `Apply_AxisLocks` — a `Locked` axis takes the current angle.
3. If a range clamp is present: the rest rotation is the look-at from the entity to the rest reference point;
   `Apply_RangeClamp` clamps each ENABLED axis to `[Min, Max]` degrees of shortest-arc delta from the rest
   angle (`Clamp_AngleToRange`, result normalized to (-180, 180]).
4. `Step_Rotation` — `Instant` snaps free axes to the desired angle; `RateLimited` steps each free axis with
   `FMath::FixedTurn` at that axis's `_TurnRateDegPerSec` (shortest arc, never overshoots, lands exactly).
   Locked axes hold in both modes. A rate or dt `<= 0` holds.
5. "At target" is every axis of `Compute_RemainingDelta(New, Desired)` within `_ReachedToleranceDeg`,
   judged after the step.

Angles are Euler per axis, deliberately: per-axis locks, rates and ranges are the product. Full 3D
shortest-arc slerp is not supported; a large simultaneous pitch+yaw turn follows the per-axis path.

Differences from the Mars script feature this module replaces:

- `OnTargetReached` is judged post-step, so it fires the frame the final step is enqueued, and it fires for
  an entity whose axes are all locked (the script compared the pre-step rotation and early-returned when all
  three axes were locked).
- Stepping always converges; the tolerance only decides "reached" (the script's tolerance dead-band left up to
  one tolerance of residual error forever).
- No Pawn/Controller path. Control-rotation coupling is game policy: bind `OnTargetReached` or read
  `Get_DesiredRotation` in game code.
- Ordered request structs instead of latest-wins optional slots.
- An invalid rest reference point is rejected at the boundary instead of silently disabling the clamp.
- Axes are named by rotation axis (Pitch/Yaw/Roll), not by world axis.
- A lost target is reported (`OnTargetCleared`, `TargetLost`) instead of silently idling.

## Ordering

HandleRequests and Update run in `FGroup_Transform_Derived`, Update after HandleRequests. Update has no
`MarkedDirtyBy`: it runs every frame for entities with `FTag_RotateTowards_HasTarget` and without
`FTag_RotateTowards_Disabled`, so an untargeted or disabled entity costs nothing per frame. Update enqueues a
rotation only when the stepped rotation differs from the current one by more than `kApplyEpsilonDeg`:

- a scene node gets `UCk_Utils_SceneNode_UE::Request_UpdateOffset` with the world delta composed into its
  offset (`Compose_OffsetRotation`: `Offset * (CurrentWorld^-1 * NewWorld)`), which drains at the start of the
  next frame;
- any other transform gets a World rotation request through `UCk_Utils_Transform_UE`, which the Transform
  drain settles after `FGroup_Transform_Derived`, so the rotation lands the same frame.

`CancelPendingRequests` (`FGroup_EndPlay`) fires `Failed_Cancelled` for requests pending at teardown.

## Robustness

- Target destroyed → `OnTargetCleared(..., TargetLost)`; the entity holds its rotation. Never an ensure.
  A disabled entity does not notice a lost target until it is re-enabled (Update excludes
  `FTag_RotateTowards_Disabled`), so `OnTargetCleared(TargetLost)` fires on its first enabled frame; a
  `Request_ClearTarget` issued in that window still fires `OnTargetCleared(Requested)`, with an invalid previous
  handle.
- Rest reference point destroyed → one ensure (`RotateTowards [..] lost its rest reference point; range
  clamping removed`) and the clamp fragment is removed; motion continues unclamped.
- Coincident target or rest point → hold this frame (SolveResult desired = current, remaining = zero), no signal,
  no ensure, the AtTarget tag is left unchanged.
- Parent-driven scene nodes are rotated through their offset, never through a World rotation request (which
  the Transform feature rejects on such nodes).
- Not replicated and not snapshotable: composition rebuilds the feature. No snapshot registrations belong in
  this module.

## Diagnostics and tests

- Getters: `Get_Target`, `Get_HasTarget`, `Get_IsAtTarget`, `Get_IsEnabled`, `Get_Tunables`,
  `Get_HasRangeClamp`, `Get_RangeClamp` (an all-disabled default when none is set), `Get_DesiredRotation`
  and `Get_RemainingRotation` (from the last Update; zero without a target).
- **Debug draw:** cvar `ck.RotateTowards.DrawTargets` (default 0), mirrored into
  `UCk_RotateTowards_DebugSettings_UE::_DrawTargets` (Project Settings, "Rotate Towards Debug"; read with
  `UCk_Utils_RotateTowards_DebugSettings_UE::Get_DrawTargets`). `FProcessor_RotateTowards_DebugDraw`
  (`FGroup_Gameplay_Camera`, skipped entirely while the setting is off) draws, per entity with the feature: the
  current forward arrow and a readout of the desired and remaining rotation (`No target` when there is none),
  colored red when disabled, gray when there is no target, green at target, yellow while turning; the line to the target and the desired-forward arrow (white) while the target is valid; and, with a
  range clamp whose rest reference point is valid, the rest line (blue) plus the two yaw range edges (cyan) when
  the Yaw range is enabled.
- **C++ unit rows:** `Ck.RotateTowards.Kernel.*`, 12 rows, world-free
  (`CkTests/Private/UnitTests/CkRotateTowards/Test_RotateTowards_Kernel.cpp`).
- **AS autotests:** `Ck_AutoTest_RotateTowards_*`, 15 rows in `CkTests/Script/CkRotateTowards/`
  (fixture `CkRotateTowardsAutoTest_Fixture.as`): boundary rejections, rate-limited and instant turns, axis
  locks, the all-locked reach, target change / clear / loss signals, yaw range clamp, disable/enable, and a
  scene-node child rotated through its offset.
- **Gym:** "Rotate Towards" (`CkTests/Script/CkRotateTowards/CkRotateTowardsGym_*.as`), stations Tracker
  (rate-limited vs instant), Clamped (+-45 deg yaw about a rest line) and Modes (free / yaw-locked /
  pitch-locked / disabled); `O` toggles `ck.RotateTowards.DrawTargets`.

## Boundaries

The kernel is world-free and reusable without an entity. This module depends on CkEcsExt (Transform,
SceneNode) but never modifies it; it has no record, so it does not link CkLabel, and it has no dependency on
CkGameplayDebugger.

One writer per rotation: do not compose RotateTowards on an entity whose rotation another feature owns. On a
CkSway node the sway processor reports the foreign offset write once and overwrites it every frame, so the entity
never turns; a CkTween rotation tween on the same entity fights the same way. Give RotateTowards its own scene
node (or the parent) and hang the swayed/tweened node under it.
