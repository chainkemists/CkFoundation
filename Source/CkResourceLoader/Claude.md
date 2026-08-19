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

## Rooting contract — this module is how a feature loads anything it holds

UE GC does not trace EnTT fragments, so a fragment ref is never the GC root. For assets, **the
batch's streamable handle is the root** — which makes `RequestLoad_RootedBatch` the sanctioned load
path for any asset a feature keeps, not a convenience. Consequences a feature must honor:

- Asset references in `ParamsData` and in requests are `TSoftObjectPtr`, never `TObjectPtr` — a hard
  ref on an authorable field force-loads the asset with every DataAsset/Blueprint that names it.
- `Current` holds the `FCk_ResourceLoader_RootedAssetBatch` for as long as the assets must stay
  resident, and clears it (`= {}`) at EndPlay.
- A failed load ensures loudly and recovers (reset the batch, complete pending requests `Failed`,
  fall back to a default where one exists) — it never wedges the feature's queue.

Full rule + the ObjectPooling half (objects the feature *creates*): `Source/CLAUDE.md` §
"Objects and assets a fragment holds".

## Pattern

Request a load at entity setup; bind the `OnLoaded` signal; the setup processor waits for the signal before moving the entity to its 'ready' state.

Processor-side (no UObject to bind a delegate to): kick `RequestLoad_RootedBatch` in the fresh
branch of Setup, fall through to the ready check the same tick (a sync-overridden load AND a warm
async load complete immediately), and poll `Get_IsReady()` on later ticks while a
`FTag_<Feature>_PendingAssetLoad` tag gates the feature's requests — tag presence keeps the view
polling at zero registry ops per stalled tick.

Request-carried assets kick their batch at the **Utils boundary** instead (enqueue time), so the
load overlaps the frames before the drain; the batch dies with the consumed request, and handlers
resolve batch-first with a resident-or-null fallback for raw-built requests.

Synchronous creation sites have no deferred setup to queue behind, so there is no batch: resolve
**resident-or-fail**, ensuring on the unresolved path and taking the same recovery an *unset*
reference has always taken (`CkTween`'s curve channels).

Canonical consumers: `CkFx` Sfx/Vfx Setup, `CkAudio` AudioTrack Setup, `CkAnimation`,
`CkIskmRenderer` proxy requests (`IskmProxy.Requests`), `CkJolt` (`JoltBody.Setup`), `CkPmg`
(`PmgDonut.Material`), `CkRenderTarget` draw requests, `CkVat`, `CkWorldSpaceWidget`.

`ConsumerId` is `"<Feature>.<Site>"` and is a **public knob** — it is the key a project flips to
`Synchronous` in the loader settings to debug one consumer. Name it deliberately and record it in
the consuming module's `Claude.md`.

---

## Anti-patterns

Don't synchronously load assets inside a Processor `ForEachEntity` body — it stalls the game
thread. (The `Synchronous` per-consumer override does exactly this once per entity setup, by
explicit per-project opt-in, for debugging — that is the sanctioned exception, not a license.)

---

## See also

- `CkEcs/Claude.md` — signal binding for the `OnLoaded` callback.
