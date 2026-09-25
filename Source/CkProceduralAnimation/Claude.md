# CkProceduralAnimation

Procedural N-leg locomotion with independent surface motion and rigid-part presentation. The public ECS entry points are `UCk_Utils_ProceduralGait_UE`, `UCk_Utils_SurfaceMotion_UE` and `UCk_Utils_ProceduralRig_UE`; their generated AngelScript namespaces are `utils_procedural_gait`, `utils_surface_motion` and `utils_procedural_rig`.

## Composition

1. Create a lifetime-owned entity with a transform. The rigid rig requires a unit-scale root; author the limb lengths and part geometry at the intended size.
2. Optionally add SurfaceMotion when this feature owns locomotion. Its accepted support frame is independent of the body's interpolated orientation. Steering takes a world direction and speed; the direction is projected onto the accepted surface tangent.
3. Add ProceduralGait with stable, unique leg IDs, body-local hips/rest feet, phase offsets and probe/cadence settings. It consumes body motion and performs passive Jolt queries. It does not translate the body.
4. Add ProceduralRig with matching IDs, segment lengths, body-local pole targets and caller-owned upper/lower/optional-foot transform entities. Segment meshes must be centered on their transform origin, with length along local +X. Parts must be unique direct lifetime children of the rig owner. Omit an optional foot with an empty handle; a stale supplied handle is an error.
5. Wait for each feature's `Get_IsReady`. Observe `ProceduralGait.Get_HasFailed` and `ProceduralRig.Get_Failure` rather than treating permanent failure as delayed readiness. Ready means evaluated, not grounded. SurfaceMotion.Get_IsGrounded includes the contact-grace interval; Get_HasTrustedContact reports a current accepted query. Gait and rig failures latch: recovery currently requires replacing the owner/rig composition, because correcting authored inputs does not reset the feature. Destroying the owner retires its parts through normal entity lifetime ownership.

The module creates no render components or assets. Callers can use CkPmg or CkIsmRenderer for rigid parts; a future skeletal adapter can consume the same world foot positions, normals, rotations and swing state.

## Scheduling and ownership

SurfaceMotion processes steering and movement in `FGroup_Physics`. Its deferred root transform is settled by the ordinary transform pass. ProceduralGait runs in `FGroup_Transform_Derived`; ProceduralRig explicitly runs after gait. The existing derived-transform settle barrier applies limb outputs before post-transform render consumers.

SurfaceMotion must be the sole movement owner when composed. Do not also integrate root velocity through another mover. Gait can instead consume a root driven by another system without SurfaceMotion. Its ground provider is CkJolt; purely Chaos collision that is absent from Jolt will not produce foot contacts.

Steering requests carry the existing Ck completion delegate. Accepted requests are drained locally; a pending request on a destroyed owner completes `Failed_Cancelled`. Invalid admission does not attach partial feature state. Request completion reports local processing, not network delivery.

## Core contract

`ck::FProceduralGaitSolver` and its swing/probe helpers contain no world or UObject operations. Coordinates use a support-aligned frame with +Z as up. This is a rotation about the world origin, not a moving body-local origin. Re-express stored state with `NewBasis.Inverse() * OldBasis`; sample translation velocity in world space before rotating it. Otherwise turning the support frame manufactures false velocity or moves planted feet.

All durations use FCk_Time; phase remains dimensionless. Explicit Reset establishes topology. Step rejects mismatched arrays, malformed settings and nonfinite inputs without partial mutation. Non-advancing evaluation uses a solver copy, preserving internal state. Core rejection results must be handled by adapters.

Foot probe states distinguish current hits from fallback targets. A brief miss withholds a target to hold the plant; after the duration-based grace expires, the foot may gather toward its fresh rest target. Guessed targets are never counted as trusted surface contacts. Retry spans are bounded and remove outward lean on fallback attempts, supporting both convex and concave geometry.

The ECS facade interprets MaxSimultaneousSwings = 0 as max(1, leg count / 2); the pure core interprets zero as unlimited. Emergency and catch steps may exceed the nominal swing budget.

## Scope

This release targets rigid, two-bone N-leg rigs and static surfaces. It does not claim arbitrary surface pathfinding, moving-platform attachment, skeletal/ControlRig integration, combat sequencing, or replicated/persisted simulation state. The CkTests courses use authored patrol steering; their motion and articulation still run through production features.

## Provenance

Adapted from the user's AlchemyAnimus checkout in Venus at commit `7114d161277e60639ad6a15262877e34bb5c3368` (2026-09-24 audit). The core preserves scheduling, inhibition, emergency/catch stepping, cadence, settling, landing, swing profiles and support-frame transport. Changes include domain-typed time, explicit rejection contracts, nonmutating preview and duration-based contact loss. The surface-motion ECS implementation is a revised design, not a copy of the source Actor driver.

Reference source: `AlchemyGaitSolver`, `AlchemyGaitSwingProfile`, `AlchemyGaitFootProbe`, `AlchemyAnimusGaitDriverComponent`; reference showcase: `AlchemyAnimusCompositeGym` and `AlchemyAnimusPartTestWalker`. Existing source AnimGraph walking uses another solver and is not part of this port.

## Verification

Tests live in CkTests under `Private/UnitTests/CkProceduralAnimation/Core` and `Script/CkProceduralAnimation`. The registered gym is **Procedural Animation Surface Traversal** in the shared gym switchboard. It shows nine 4/6/8-leg rigs across uneven ground, ramp/wall and concave-loop courses. P controls travel, R resets, V toggles contact diagnostics, Home frames the overview, 1/2/3 frame the courses.

During implementation, current gate evidence is recorded in the host's `docs/campaigns/procedural-animation/PROGRESS.md`. Source presence and UHT generation do not prove runtime traversal or visual quality.

## Debugger

The CkGameplayDebugger plugin provides the standalone Procedural Animation tool under Systems (`ck.ProceduralAnimationDebugger 1`) and an ECS inspector. Select a gait entity or one of its owned parts to inspect accepted solver state, actual probe results, rig transforms and bounded Hold/Scrub/Live history. History contains copied values and releases live entity references on world/session teardown. The debugger does not rerun traces or modify simulation.