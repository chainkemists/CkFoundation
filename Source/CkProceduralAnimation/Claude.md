# CkProceduralAnimation

Procedural N-leg locomotion with independent surface motion and rigid N-joint leg presentation. The public ECS entry points are `UCk_Utils_ProceduralLeg_UE`, `UCk_Utils_ProceduralGait_UE`, `UCk_Utils_ProceduralRig_UE`, `UCk_Utils_SurfaceMotion_UE` and the one-call accelerant `UCk_Utils_ProceduralAnimation_UE::Add_Walker`; their generated AngelScript namespaces are `utils_procedural_leg`, `utils_procedural_gait`, `utils_procedural_rig`, `utils_surface_motion` and `utils_procedural_animation`.

## Composition

The body is a lifetime-owned entity with a transform. Legs are child entities of the body (`FCk_Handle_ProceduralLeg`), connected through `FFragment_RecordOfProceduralLegs`. The order is fixed: legs, then gait, then rigs.

1. **Legs.** `UCk_Utils_ProceduralLeg_UE::Create(Body, Params)` once per leg. A leg carries a unique `FName` Id, a body-local placement (hip, rest foot, phase offset, step-threshold scale) and its chain geometry (1..8 hip-first segment lengths in centimetres plus a body-local pole). Creating a leg on a body that already has a gait is rejected; the topology is fixed for the gait's lifetime.
2. **Surface motion (optional).** `UCk_Utils_SurfaceMotion_UE::Add(Body, Params)` when this module owns locomotion. Params split into `_Contact` (clearance, probe reach, contact grace, query filter) and `_Movement` (max speed, surface turn rate, clearance speed, gravity). The accepted support frame is independent of the body's interpolated orientation. `Request_Steering` takes a world direction and speed; the direction is projected onto the accepted surface tangent.
3. **Gait.** `UCk_Utils_ProceduralGait_UE::Add(Body, GaitData)` requires 2..64 legs already connected. It captures the legs in record order; that order is the solver's leg index and never changes. `Get_Legs(Body)` returns the record's current order, which changes after a detach; address legs by `Get_Id` or keep the handles `Create` returned. Tuning comes from a `UCk_ProceduralGait_Data` preset (timing, step, probe), copied at `Add`. The gait consumes body motion and performs passive Jolt queries. It does not translate the body.
4. **Rigs.** `UCk_Utils_ProceduralRig_UE::Add(Leg, Chain)` binds caller-owned transform entities to one leg, hip-first, plus an optional foot. The segment count must equal the leg's segment-length count. Every part must carry a transform, be a unique direct lifetime child of the leg's body, and not be bound by another leg's rig. Segment geometry must be centred on its transform origin with length along local +X. Omit the foot with an empty handle; a stale supplied handle is an error. One segment aims at the foot target, two segments use two-bone IK, three or more use FABRIK seeded toward the pole.
5. **Readiness.** Wait for `Get_IsReady` on gait and rig. Observe `ProceduralGait.Get_HasFailed` and `ProceduralRig.Get_Failure` rather than treating permanent failure as delayed readiness. Ready means evaluated, not grounded. `SurfaceMotion.Get_IsGrounded` includes the contact-grace interval; `Get_HasTrustedContact` reports a current accepted query. Gait and rig failures latch: recovery requires replacing the composition. The rigid rig requires a unit-scale body; author segment lengths and part geometry at the intended size.

**Data assets.** `UCk_ProceduralGait_Data` is a tuning preset with no legs, reusable across creatures. `UCk_ProceduralRig_Data` is the leg layout for one creature: per-leg placement and chain geometry, no runtime handles. Both validate in the editor through `IsDataValid`. Values are copied into the runtime fragments at `Add`; editing the asset during play does not retune a live gait.

**Accelerant.** `UCk_Utils_ProceduralAnimation_UE::Add_Walker(Body, RigData, GaitData, Chains)` creates one leg per layout entry (asset order becomes record order), adds the gait, then binds each supplied `FCk_ProceduralWalker_LegChain` (leg Id + rig chain). It validates everything before the first mutation and returns an empty `FCk_ProceduralWalker` on rejection. An empty chain list yields a gait-only walker.

The module creates no render components or assets. Callers can use CkPmg or CkIsmRenderer for rigid parts; a future skeletal adapter can consume the same world foot outputs (`UCk_Utils_ProceduralLeg_UE::Get_Foot`: position, normal, rotation, swing alpha, planted, trusted contact).

## Disabling a leg

`UCk_Utils_ProceduralLeg_UE::Request_EnableDisable` is reversible. A disabled leg leaves the step schedule and its foot freezes body-relative: the gait captures the foot pose in body space on the transition and re-expresses it every frame, so the rig keeps posing the leg and it rides rigidly with the body. Disabled legs cost no Jolt queries. Survivors keep their indices; nothing is packed or re-indexed.

## Losing a leg

`UCk_Utils_ProceduralLeg_UE::Request_Detach` is irreversible. In the leg's request drain it collects the parts the rig was posing, optionally transfers their lifetime ownership to the world (`ECk_ProceduralLeg_ReleasedPartsOwnership::TransferToWorld`), fires `OnProceduralLeg_Detached(Leg, ReleasedParts)` while the leg handle is still readable, then destroys the leg entity. The drain runs before the gait and rig updates, so released parts never receive another pose request. The gait masks the lost index and keeps running on the survivors.

Ownership stays with the body unless `TransferToWorld` is requested: destroying the body then destroys body-owned debris.

Ragdolling the released parts is the game's job. The recipe:

1. Bind `BindTo_OnDetached` on the leg, or read `UCk_Utils_ProceduralRig_UE::Get_Chain` first.
2. Call `Request_Detach`.
3. In the callback, for each released part: add a dynamic Jolt body with an explicit box shape sized like the part's visual (`FCk_Fragment_JoltBody_ParamsData{ECk_JoltBody_ShapeSource::ExplicitShape}` with `FCk_Jolt_ShapeDimensions{ECk_Jolt_ShapeType::Box}` half-extents, `ECk_MotionType::Dynamic`, an explicit mass, then `UCk_Utils_JoltBody_UE::Add`). CkJolt does not read CkShapes. Give the body a collision profile the gait and surface-motion probes do not trace: the gym uses `Ragdoll` (PhysicsBody objects that ignore the Visibility channel the probes use). Released debris must never read as ground for the survivors.
4. Optionally push the parts with `UCk_Utils_JoltBody_UE::Request_AddImpulse` — only once `UCk_Utils_JoltBody_UE::Get_IsBodyAdded` is true, because an impulse before the body exists is dropped — and add a CkTimer that destroys the debris later. The gym fixture (`CkProceduralAnimationGym_Fixture.as`) is the reference implementation of steps 3-4.
5. Bind `UCk_Utils_ProceduralGait_UE::BindTo_OnLegSetChanged` to react to the new leg count: switch locomotion mode, or `Request_ApplyPreset` a slower preset.

**Leg-loss policy.** `FCk_ProceduralGait_Timing::_LegLossPolicy` decides how survivors adapt. `KeepAuthoredOffsets` (default) keeps every leg's authored phase. `RedistributeOffsets` re-spaces the enabled legs evenly by index and blends there through the solver's pattern-blend machinery. With `_MaxSimultaneousSwings = 0` the swing budget is `max(1, enabled legs / 2)`, recomputed whenever the enabled set changes.

**Signals and live retune.** `OnProceduralGait_LegSetChanged(Gait, EnabledCount, TotalCount)` fires on the frame the enabled set changes (disable, enable, detach, destroyed leg). `UCk_Utils_ProceduralGait_UE::Request_ApplyPreset` copies a preset's timing, step and probe into the live gait and re-applies solver settings; leg state, planted feet and the gait clock are kept.

## Scheduling and ownership

SurfaceMotion processes steering and movement in `FGroup_Physics`. Its deferred root transform is settled by the ordinary transform pass. In `FGroup_Transform_Derived` the order is: leg requests (enable/disable, detach), gait requests (apply preset), gait update, rig update. The existing derived-transform settle barrier applies limb outputs before post-transform render consumers. Pending requests on a destroyed leg or gait complete `Failed_Cancelled` in `FGroup_EndPlay`.

No processor is parallel. Gait and SurfaceMotion issue Jolt ray casts, and the Jolt query API is game-thread-only. The rig only reads its own leg and the body and writes through deferred transform requests, so it could become a parallel processor; that switch waits for a benchmark.

SurfaceMotion must be the sole movement owner when composed. Do not also integrate root velocity through another mover. Gait can instead consume a root driven by another system without SurfaceMotion. Its ground provider is CkJolt; purely Chaos collision that is absent from Jolt will not produce foot contacts.

All `Request_*` functions carry the Ck completion delegate. Invalid admission ensures, fires `Failed_NotEnqueued` and attaches no partial feature state. Request completion reports local processing, not network delivery.

## Core contract

`ck::FProceduralGaitSolver` and its swing/probe helpers contain no world or UObject operations. Coordinates use a support-aligned frame with +Z as up. This is a rotation about the world origin, not a moving body-local origin. Re-express stored state with `NewBasis.Inverse() * OldBasis`; sample translation velocity in world space before rotating it. Otherwise turning the support frame manufactures false velocity or moves planted feet.

All durations use FCk_Time; phase remains dimensionless. Explicit Reset establishes topology. Step rejects mismatched arrays, malformed settings and nonfinite inputs without partial mutation. Non-advancing evaluation uses a solver copy, preserving internal state. Core rejection results must be handled by adapters. Each leg input carries an enabled flag; disabled legs keep their index and are skipped by scheduling, take-off, landing and the airborne tuck.

Foot probe states distinguish current hits from fallback targets. A brief miss withholds a target to hold the plant; after the duration-based grace expires, the foot may gather toward its fresh rest target. Guessed targets are never counted as trusted surface contacts. Retry spans are bounded and remove outward lean on fallback attempts, supporting both convex and concave geometry.

The ECS facade interprets MaxSimultaneousSwings = 0 as max(1, enabled leg count / 2); the pure core interprets zero as unlimited. Emergency and catch steps may exceed the nominal swing budget.

## Scope

This release targets rigid N-leg rigs with 1..8 segments per leg and static surfaces. It does not claim arbitrary surface pathfinding, moving-platform attachment, skeletal/ControlRig integration, combat sequencing, adding legs after the gait exists, or replicated/persisted simulation state. Gait pattern tables, settle and airborne tuning exist in the core but are not exposed on the data asset yet. The CkTests courses use authored patrol steering; their motion and articulation still run through production features.

## Provenance

Adapted from the user's AlchemyAnimus checkout in Venus at commit `7114d161277e60639ad6a15262877e34bb5c3368` (2026-09-24 audit). The core preserves scheduling, inhibition, emergency/catch stepping, cadence, settling, landing, swing profiles and support-frame transport. Changes include domain-typed time, explicit rejection contracts, nonmutating preview, duration-based contact loss and index-stable leg disabling (Animus packs survivors instead). The surface-motion ECS implementation is a revised design, not a copy of the source Actor driver.

Reference source: `AlchemyGaitSolver`, `AlchemyGaitSwingProfile`, `AlchemyGaitFootProbe`, `AlchemyAnimusGaitDriverComponent`; reference showcase: `AlchemyAnimusCompositeGym` and `AlchemyAnimusPartTestWalker`. Existing source AnimGraph walking uses another solver and is not part of this port.

## Verification

Tests live in CkTests under `Private/UnitTests/CkProceduralAnimation/` and `Script/CkProceduralAnimation`. The registered gym is **Procedural Animation Surface Traversal** in the shared gym switchboard. It shows nine 4/6/8-leg rigs across uneven ground, ramp/wall and concave-loop courses. P controls travel, R resets, V toggles contact diagnostics, K disables or re-enables leg 0 of every crawler, J detaches leg 1 of the first crawler and ragdolls its parts (one-shot), Home frames the overview, 1/2/3 frame the courses. The 4-leg crawlers use a `RedistributeOffsets` preset, the 6- and 8-leg crawlers the default `KeepAuthoredOffsets`; a crawler that drops below three enabled legs applies the slow preset through `Request_ApplyPreset`. Source presence and UHT generation do not prove runtime traversal or visual quality.

## Debugger

The CkGameplayDebugger plugin provides the standalone Procedural Animation tool under Systems (`ck.ProceduralAnimationDebugger 1`) and an ECS inspector. `UCk_Utils_ProceduralAnimation_Debug_UE::Get_Snapshot` accepts the body, one of its legs or any other direct lifetime child of the body, and returns the body's accepted solver state, actual probe results, per-leg enabled state and entity identity, and each rigged leg's segment and foot transforms. History contains copied values and releases live entity references on world/session teardown. The debugger does not rerun traces or modify simulation.
