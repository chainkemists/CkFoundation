# CkResourceLoader

**Purpose:** Async resource loading — wraps UE's `FStreamableManager` / `UAssetManager` to load assets into entity fragments asynchronously, firing a signal when complete.

**Depends on:** `CkCore`, `CkEcs`, `CkLog`, `CkSettings`.
**Used by:** `CkEditorToolbar`, `CkAngelscriptGenerator`, runtime content streaming.

---

## Key API

- `UCk_Utils_ResourceLoader_UE` — request async load of an asset path; bind a delegate for completion; cancel pending loads on entity destroy.
- `UCk_Utils_ResourceLoader_UE::RequestLoad_RootedBatch(ConsumerId, SoftPaths)` — C++-only,
  processor-friendly load returning `FCk_ResourceLoader_RootedAssetBatch`. No request entity, no
  delegate: the caller polls `Get_IsReady()` / `Get_HasFailed()` and resolves via
  `Get_ResolvedObject(SoftPath)`. **The batch's streamable handle is the GC root for both
  policies** — store the batch in the feature's `Current`, reset it (EndPlay) to release the assets.
- `UCk_ResourceLoader_ProjectSettings_UE` — `_DefaultLoadingPolicy` (**Async**) +
  `_PerConsumerLoadingPolicyOverrides` (`TMap<FName, policy>`): flip one consumer (e.g.
  `"AudioTrack.Setup"`) to `Synchronous` in one project alone — the per-project debug knob.
- `FProcessor_ResourceLoader_HandleRequests` — processes delegate-API load requests each tick.

---

## Pattern

Request a load at entity setup; bind the `OnLoaded` signal; the setup processor waits for the signal before moving the entity to its 'ready' state.

Processor-side (no UObject to bind a delegate to): kick `RequestLoad_RootedBatch` in the fresh
branch of Setup, fall through to the ready check the same tick (a sync-overridden load AND a warm
async load complete immediately), and poll `Get_IsReady()` on later ticks while a NeedsSetup-style
tag gates the feature's requests. Canonical consumers: `CkAudio` AudioTrack Setup, `CkFx` Sfx Setup.

---

## Anti-patterns

Don't synchronously load assets inside a Processor `ForEachEntity` body — it stalls the game
thread. (The `Synchronous` per-consumer override does exactly this once per entity setup, by
explicit per-project opt-in, for debugging — that is the sanctioned exception, not a license.)

---

## See also

- `CkEcs/Claude.md` — signal binding for the `OnLoaded` callback.
