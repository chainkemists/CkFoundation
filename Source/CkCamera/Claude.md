# CkCamera

**Purpose:** Camera shake system. Adds `FCk_Handle_CameraShake` entities to a Record on a camera-owner entity. Each shake entity has params (shake asset, intensity, falloff) and a lifecycle managed by processors.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkProvider`, `CkRecord`, `CkSettings`.
**Used by:** Ability/VFX systems that trigger camera feedback.

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
