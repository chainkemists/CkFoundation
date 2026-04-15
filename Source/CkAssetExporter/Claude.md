# CkAssetExporter

**Purpose:** Asset export utilities — exports CkFoundation asset data to external formats (JSON, etc.) for tooling, auditing, or external pipelines.

**Depends on:** `CkAi`, `CkCore`, `CkEcs`, `CkLog`.
**Used by:** Pipeline tooling only.

---

## Key API

- No `_Utils.h`. Invoked via editor commands.

---

## Pattern

Run via editor utility or commandlet.

---

## Anti-patterns

Don't invoke from game runtime — editor-only.

---

## See also

- `CkAngelscriptGenerator/Claude.md` — related code generation tooling.
