# CkCamera

**Purpose:** Two subsystems —
1. **`CameraShake/`** — camera shake. Adds `FCk_Handle_CameraShake` entities to a Record on a camera-owner entity; the processor drives `PlayerCameraManager->StartCameraShake`.
2. **`GameplayCamera/`** — the gameplay camera (stack) system: a per-viewer **director** (`FCk_Handle_GameplayCamera`) owning a Record of **modifier** child entities (`FCk_Handle_CameraModifier`). Each modifier is an **entity script** (`UCk_CameraModifier_EntityScript`, the camera analog of `UCk_SmState_EntityScript`) that contributes to a composed `FCk_GameplayCamera_Profile`; a single POV pipeline resolves an `FMinimalViewInfo` consumed by `UCk_GameplayCameraComponent::GetCameraView`. Client-local only (no replication).

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkInput`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Ability/VFX systems (shake); player/pawn setup (gameplay camera).

---

## Key API

- `UCk_Utils_CameraShake_UE::Add(InHandle, InParams)` — add shake to Record.
- `UCk_Utils_CameraShake_UE::AddMultiple(InHandle, InParams)` — batch add.
- `Has`, Cast, CastChecked, InvalidHandle — standard helpers.

---

## Pattern

Use `CkLabel` to track active vs. queued shakes. Start shakes at the owner entity level; the processor drives the UE camera shake subsystem.

---

## Anti-patterns

1. Don't call UE's `PlayerCameraManager->StartCameraShake` directly in processors — route through the shake entity so cancellation and lifetime are managed.
2. Don't add shakes every frame without checking `Has` and deduplicating.

---

## See also

- `CkRecord/Claude.md`, `CkLabel/Claude.md`.
