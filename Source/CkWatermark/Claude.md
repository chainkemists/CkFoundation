# CkWatermark

**Purpose:** Visual watermark overlay — draws a text/logo watermark on screen for demo/build identification. Entity-scoped; the watermark entity's fragment controls visibility and content.

**Depends on:** `CkCore`, `CkEcs`, `CkJolt` (threading stats panel), `CkLog`, `CkMemory`, `CkSettings`, `CkUI`.
**Used by:** Demo builds, playtest builds.

---

## Key API

- No `_Utils.h` found. Configure via settings (`UCk_Watermark_Settings`) and the watermark entity is managed automatically.

---

## Pattern

Enable via project settings; the watermark processor creates the overlay entity on world start.

---

## Anti-patterns

Don't ship with watermark enabled — it's guarded by build config but verify it's off in shipping.

---

## See also

- `CkSettings/Claude.md`, `CkUI/Claude.md`.
