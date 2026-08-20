# CkCamera

**Purpose:** Two subsystems —
1. **`CameraShake/`** — camera shake. Adds `FCk_Handle_CameraShake` entities to a Record on a camera-owner entity; the processor drives `PlayerCameraManager->StartCameraShake`.
2. **`Camera/`** — the gameplay camera system: a per-viewer **director** (`FCk_Handle_Camera`). Every leaf of the `FCk_CameraProfile` is materialized as a **non-replicated tuner attribute** on the director (Float / Vector / Rotator / Integer; `FCk_FloatRange` → Float-with-MinMax), tagged `Camera.<Section>.<...>` (e.g. `Camera.Sensor.FOV`). The director owns a Record of **layer** child entities (`FCk_Handle_CameraLayer`); each layer is an **entity script** (`UCk_CameraLayer_EntityScript`, the camera analog of `UCk_SmState_EntityScript`) that **acquires attribute modifiers** on the tuner attributes via `UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_<Tuner>`. The framework auto-blends those modifiers in/out. A single POV pipeline resolves an `FMinimalViewInfo` consumed by `UCk_CameraComponent::GetCameraView`. Client-local only (no replication).

**Depends on:** `CkAttribute`, `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Ability/VFX systems (shake); player/pawn setup (gameplay camera).

---

## Key API (gameplay camera)

- `UCk_Utils_Camera_UE::Add(InHandle, InParams)` — `InParams._OutputComponent` (a **required** `UCk_CameraComponent`, supplied via the params ctor) is the view sink; the camera system only supports `UCk_CameraComponent` because its `GetCameraView` override is what delivers the resolved view to the default PlayerCameraManager (no output-mode enum, no auto-creation — the caller owns/supplies the component). `InParams._Profile` (an `FCk_CameraProfile`) supplies the defaults; `Add` materializes every leaf into a tuner attribute and writes the bool/curve leaves onto `FFragment_Camera_Current`. It then spawns a **persistent default layer** (`UCk_CameraLayer_Default_EntityScript`, `_IsDefault`, lowest priority, pinned at alpha 1) that represents the resting profile — it acquires no modifiers (the resting state IS the tuner base values) and is never evicted (OneOnly), pruned, or removed, so feature layers always blend back to the base rather than to an empty stack. **The profile you pass to `Add` is the first-person/base resting state** — there is no separate "base layer" to push from gameplay. `Add` also seeds `_ViewInfo` from the anchor transform so the first `GetCameraView` (before `UpdatePOV` ticks) returns the real POV, not an origin snap. `Get_LayerCount` counts only gameplay-pushed layers (excludes the default).
- `Request_AddLayer` / `Request_RemoveLayer` — push/blend-out a `UCk_CameraLayer_EntityScript` (OneOnly stacking evicts same-ordering-group layers).
- `Request_Set_<Flag>` — toggle the non-blending bool leaves (`UseFixedBoomRotation`, `ConstrainAspectRatio`, `HasOrientationControl`, `HasAutoReorient`, `HasCollision`, `UseAsyncTrace`, `UsePostProcess`).
- `FCk_Fragment_Camera_ParamsData::_DriveControllerControlRotation` — opt-in (default false). When set, `FProcessor_Camera_UpdatePOV` publishes the resolved view rotation to the owning pawn's **local** `APlayerController` each frame (`SetControlRotation`), so control-rotation consumers (facing/aim/movement) follow the camera. This is the framework-owned replacement for a per-frame "write view rotation back to the controller" gameplay task — a player pawn just calls `Add` once (e.g. on local possession) with this flag true; no ticking task needed. It never feeds back into the POV (the camera reads input intention, not control rotation).
- `Get_ComposedProfile` — the live `FCk_CameraProfile`, assembled each frame from the attributes' final values + the Current bool/curve leaves (cached on `FFragment_Camera_Current`).
- **Layer side:** subclass `UCk_CameraLayer_EntityScript`; in `DoEnter`, call `Acquire_CameraModifier_<Tuner>(OwningCamera, Operation)` to get a typed attribute-modifier handle, then set its **target delta** via the matching `...AttributeModifier_UE::Override`. Never set the modifier's live delta — the blend processor owns it.

### Auto-blend (the core mechanic)
- Each layer has `FFragment_CameraLayer_Blend` (alpha ramps 0→1 on enter, 1→0 on exit). `FProcessor_CameraLayer_Blend` (in `FGroup_Gameplay_TimeDelta`, before the attribute recompute in `FGroup_Gameplay`) rewrites each acquired modifier's effective delta from the stored target × alpha, per op: Additive → `target*α`; Multiplicative → `Lerp(1,target,α)`; Override → `α*(target−base)` against an **Add** modifier (revocable Override is unsupported, so Override is realized additively from base).
- `FProcessor_CameraLayer_Lifecycle` (`FGroup_Gameplay_Camera`) prunes alpha-0 layers (destroying their acquired modifiers), dispatches `DoTick`, resolves the dominant layer + look-at, and refreshes the composed-profile cache. `FProcessor_Camera_UpdatePOV` runs the POV pipeline against that cache.

### Implementation notes

- **`FCk_CameraProfile` deliberately carries no bone-name field.** The anchor is a caller-supplied `FCk_Handle_Transform` (e.g. a socket-following transform entity), so the camera module never needs one. Every profile leaf uses `CK_PROPERTY` (get + set) so modifiers can mutate a running profile.
- **Tuner materialization is adopt-or-add.** A restore brings the tuner attributes back, so `DoMaterializeAttributes` re-seats the profile onto the existing attribute (`TryGet` + `Request_Override`) instead of adding a duplicate, which would trip the attribute's one-per-label ensure.
- **`FProcessor_CameraLayer_Blend` tolerates husk layers.** An owner-destroyed layer can still carry the blend + acquired-modifier fragments with no EntityScript; the tuner blend runs regardless and only the opt-in script hook (`Blend`/`FullyBlendedIn`/`FullyBlendedOut`) is skipped.

---

## Anti-patterns

0. (Camera) The orientation intention (`Request_SetOrientationIntention`) is a **per-frame delta**: `FPov::Compute_OrientationControl` does NOT multiply by delta-time and does NOT run it through the profile's `X/YIntentionCurve` (those clamp to [0,1], wrong for a raw mouse delta). The caller pre-scales by user sensitivity; output rotation = intention × OrientationControl `Speed`. **`UpdatePOV` consumes (zeroes) the intention each frame after applying it** — the caller must push a fresh delta every frame while the input is active (e.g. mouse `Triggered`); otherwise the camera would keep re-applying the last delta and drift after input stops. A sustained-rate source (gamepad stick) must pre-multiply by delta-time at the caller. The `X/YIntentionCurve` profile leaves are currently unused (reserved for an explicit analog-shaping path).
1. Don't call UE's `PlayerCameraManager->StartCameraShake` directly in processors — route through the shake entity.
2. (Camera) Don't set an acquired modifier's live `_ModifierDelta` from a layer — set the TARGET via the modifier-Override util; the blend processor drives the live delta.
3. (Camera) Don't store profile values as plain fragment floats — they're tuner attributes so layers can modify them. Only the bool/curve leaves live on `Current`.

---

## See also

- `CkAttribute/Claude.md` (modifier flow), `CkRecord/Claude.md`, `CkLabel/Claude.md`.

## Frame position and the view anchor

The composition chain (`FProcessor_Camera_HandleRequests` → `FProcessor_CameraLayer_Lifecycle` →
`FProcessor_Camera_UpdatePOV`) runs in `FGroup_Transform_Derived`: after the main transform pass has
settled every input anchor, before the frame's second resolution point (`FGroup_Transform_LateResolve`).
`UCk_Utils_Camera_UE::Add` requires a TRANSFORM handle — the director's transform is the composition's
input anchor and is non-optional by construction.

The composed view is published two ways each compose:
- `FFragment_Camera_Current::_ViewInfo` — the render authority, pulled by `UCk_CameraComponent::GetCameraView`.
- the **view anchor** (`UCk_Utils_Camera_UE::Get_ViewAnchor`) — an attachable child transform whose pose is
  enqueued as an ordinary `Request_SetTransform` and drained by the late-resolve pass. Scene-node-attach
  content here to have it follow the rendered view with zero special-casing; children compose in the same
  frame, before components are pushed. Input anchor and view anchor are distinct on purpose — the compose
  must never consume its own output.
