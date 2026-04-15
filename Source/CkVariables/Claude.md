# CkVariables

**Purpose:** Typed game variables — named, replicated, BP-readable variables stored on entities. Supports bool, int, float, string, tag, and struct types. Use for designer-set properties that need BP/AS read access without a custom fragment.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`.
**Used by:** `CkActor`, `CkGraphics`, `CkK2Nodes`, `CkProjectile`.

---

## Key API

- `UCk_Utils_Variables_Bool_UE`, `UCk_Utils_Variables_Float_UE`, etc. — set/get named variables on an entity.
- Variables are stored in a map fragment and replicated.

---

## Pattern

Use for one-off named properties that designers need to read from Blueprint without you authoring a custom fragment and Utils. For performance-sensitive data, prefer a typed fragment.

---

## Anti-patterns

1. Don't use variables as a substitute for typed fragments in hot paths — map lookups are slower than direct fragment access.
2. Don't use variables for signals — use the signal system instead.

---

## See also

- `CkEcs/Claude.md` — typed fragments for performance-critical data.
