# CkConsoleCommands

**Purpose:** CkFoundation console commands — registers `ck.*` console commands for runtime inspection and control of ECS state (entity dump, processor stats, label queries).

**Depends on:** `CkCore`, `CkEcs`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** Developer workflow.

---

## Key API

- No `_Utils.h`. Commands registered on module startup via `IConsoleManager`.

---

## Pattern

Available in development and debug builds. Guarded by `!UE_BUILD_SHIPPING`.

---

## Anti-patterns

Don't add gameplay-affecting console commands here — use `CkCVar` for runtime tuning.

---

## See also

- `CkCVar/Claude.md` — runtime-mutable values.
- `CkEcs/Claude.md` — entity/processor state being inspected.
