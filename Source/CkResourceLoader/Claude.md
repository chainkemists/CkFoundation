# CkResourceLoader

**Purpose:** Async resource loading — wraps UE's `FStreamableManager` / `UAssetManager` to load assets into entity fragments asynchronously, firing a signal when complete.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkSettings`.
**Used by:** `CkEditorToolbar`, `CkAngelscriptGenerator`, runtime content streaming.

---

## Key API

- `UCk_Utils_ResourceLoader_UE` — request async load of an asset path; bind a delegate for completion; cancel pending loads on entity destroy.
- `FProcessor_ResourceLoader_HandleRequests` — processes load requests each tick.

---

## Pattern

Request a load at entity setup; bind the `OnLoaded` signal; the setup processor waits for the signal before moving the entity to its 'ready' state.

---

## Anti-patterns

Don't synchronously load assets inside a Processor `ForEachEntity` body — it stalls the game thread.

---

## See also

- `CkEcs/Claude.md` — signal binding for the `OnLoaded` callback.
