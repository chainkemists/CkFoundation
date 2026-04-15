# CkDataViewer

**Purpose:** Runtime data viewer — an in-editor / in-game overlay that displays ECS entity state (fragments, handles, labels) for debugging. Reads from the registry and formats output using CkCore debug utilities.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** Developer debugging workflow.

---

## Key API

- Activated via console command or editor hotkey.
- Renders fragment data for a selected entity handle.

---

## Pattern

Enable on a specific entity handle; the viewer processor reads all fragments and formats them for display.

---

## Anti-patterns

Don't leave data viewer active in playtests — it has significant per-frame overhead.

---

## See also

- `CkConsoleCommands/Claude.md` — the commands that activate the viewer.
- `CkCore/Debug/README.md` — debug draw / formatting utilities.
