# CkK2Nodes

**Purpose:** Blueprint K2 nodes for CkFoundation — custom Blueprint graph nodes providing better ergonomics for ECS handle operations, dynamic struct dispatch, and messaging. Editor-only.

**Depends on:** `CkCore`, `CkDynamic`, `CkEcs`, `CkEditorGraph`, `CkLog`, `CkMessaging`, `CkRecord`, `CkVariables`.
**Used by:** Blueprint users who need custom node visuals.

---

## Key API

- Custom K2 nodes appear in Blueprint graph; no C++ API surface for game code.

---

## Pattern

These nodes are available automatically in Blueprint editors when the module is loaded.

---

## Anti-patterns

Don't invoke K2 node logic from C++ — the custom node logic is graph-compiler-only.

---

## See also

- `CkEditorGraph/Claude.md` — editor graph infrastructure.
