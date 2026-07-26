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

## Implementation notes

- **The display policy is resolved from the command line with `FParse`, not as a CVar argument** — the
  engine's CVar command-line pass can be skipped in Shipping. `-CkWatermark` (defaults to Regular) or
  `-CkWatermark=Minimal|Regular|Detailed|Hidden`. Precedence: the arg > project setting (Shipping) /
  CVar default, resolved once on first widget creation.
- **Activity-bar chips are pooled and updated in place** (`SetText`/`SetVisibility`); destroying and
  recreating `STextBlock`s flashes the layout. Re-activating an already-active Id is ignored so the
  existing entry keeps its held-underline accent, and each press/release cycle adds a NEW entry — the
  history therefore reads e.g. `LMB, LMB, TAB`.
- **The center group is ECS pump pressure:** the frame's worst-case scheduler pump count vs the budget,
  color-banded lower-is-better, plus a red "PUMP LIMIT EXCEEDED" banner once the budget is reached. It
  replaces the per-frame log spam the scheduler used to emit, and is the reserved home for further
  Server/Client connection + version-mismatch rows (add them below the warning).
- **Connection rows are decoupled from the project's GameState/PlayerState classes** — all data comes
  from `UCk_NetVersion_WorldSubsystem_UE` (one server-spawned `ACk_NetVersionReport_UE` per player).
  Client (`NM_Client`): one `Server <hash> [OK|VERSION MISMATCH]` row. Host (`NM_ListenServer`): a
  `Clients (N)` header plus up to `MaxClientRows` fixed per-client rows. Dedicated server: no watermark
  at all — its surface is the server-side log in `ACk_NetVersionReport_UE`.

---

## See also

- `CkSettings/Claude.md`, `CkUI/Claude.md`.
