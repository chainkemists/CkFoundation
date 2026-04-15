# CkScripts

**Purpose:** AngelScript script assets and script host infrastructure. Provides the runtime scripting subsystem that loads, compiles, and executes `.as` script files. Also the home of the script-hosted processor subsystem.

**Depends on:** (varies — loaded dynamically per project build config).
**Used by:** Any feature implemented in AngelScript.

---

## Key API

- `CkProcessorScript_Subsystem` (in `CkEcs`) hosts script-driven processors.
- Script assets are loaded by the resource loader and compiled at startup.

---

## Pattern

Scripts live in `Content/Scripts/`. The subsystem discovers and loads them at world init. Script processors run in the same tick pipeline as C++ processors.

---

## Anti-patterns

Don't put permanent game logic in scripts that would be better as a C++ module — scripts are for rapid iteration, not performance-critical systems.

---

## See also

- `CkEcs/Claude.md` — `CkProcessorScript_UE` and `CkProcessorScript_Subsystem`.
- `/Source/CLAUDE.md` section 14 — AngelScript compatibility guidelines.
