# CkUI

**Purpose:** UI utilities — widget math helpers (line/plane intersection for 3D-to-screen projection), UI entity lifecycle, and bridge between UE's `UMG` and the ECS world.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkGameSession`, `CkLog`, `CkSettings`, `CkThirdParty`.
**Used by:** `CkCueEditor`, `CkDynamicEditor`, `CkEditorToolbar`, `CkWatermark`.

---

## Key API

- `ECk_LinePlaneIntersectionStatus` — result of 3D projection queries.
- UI entity lifecycle (show/hide, bind to entity position).
- `UCk_Utils_UI_UE::Request_LockCursorToWidget` / `Request_UnlockCursor` — confine the mouse cursor
  to a `UWidget`'s screen bounds. Slate owns the lock afterwards (rect follows layout changes, auto-
  releases when the widget leaves the screen), so there is no per-frame upkeep and no teardown unlock.
  Returns `ECk_UI_CursorLock_Result` — a lock is silently dropped by the platform when the widget's
  window is not foreground, and that case is reported rather than swallowed.

---

## Pattern

Attach a UI entity to a gameplay entity; the UI processor updates the widget's screen position each frame from the entity's world transform.

---

## Anti-patterns

Don't update widget positions from Tick in a `UUserWidget` subclass — route through the UI processor so updates are batched.

---

## See also

- `CkGameSession/Claude.md` — session state drives some UI visibility.
- `CkGraphics/Claude.md`.
