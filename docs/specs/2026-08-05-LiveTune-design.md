# LiveTune — live-tunable feature params, design

**Date:** 2026-08-05
**Status:** GREEN-LIT — CTO review (committed `ad9da3940`, `../reviews/2026-08-05-LiveTune-CTO-review.md`) returned CHANGES REQUESTED (surgical); the three sign-off amendments are applied below (§4.3 dispatch hygiene, §10.1/#6 rebuild hazards, engine-fork claim corrected) and the §8 fork calls are locked per the review.
**Author session:** Fable 5 (research + design); implementation intended for an Opus executor with per-phase Fable audit. Nothing is implemented — every API name below is a proposal.

---

## 1. Context — the ask

Enable a designer to edit feature params (a probe size, an attribute max/min, a scene-node relative transform) in a data asset — from AngelScript or Blueprint — and see the change reflected **in real time during PIE, without stopping and relaunching**.

Constraints stated up front:

1. "All params are data assets all the time" has two known negatives: pointer indirection on every access of effectively-immutable data, and authoring friction for the majority of features that never need live tuning.
2. The solution should "smartly" revert to a regular value copy in non-editor / shipping builds for performance.
3. Edge case: params sometimes feed an **involved setup with cascading effects** when changed at runtime (e.g. a probe's Jolt body bake). The answer must be systemic, not per-feature heroics.
4. Author ergonomics AND user ergonomics matter, alongside performance.
5. Evaluate the 10k-identical-entities concern: are per-entity params copies a real problem, and would a shared fragment / DA flyweight help?

## 2. Research findings (file-verified this session)

| Finding | Evidence |
|---|---|
| **CkProvider is dead code** — 10 DA provider families, **zero consumers ever** in repo history; per-read `BlueprintNativeEvent` dispatch; no change signal; its `Claude.md` "Used by" claims are false | `Source/CkProvider/Public/CkProvider/CkProvider_Data.h` + repo-wide consumer grep. Empirical proof the indirection model already failed here |
| **~116 params fragment types**, every one stored as a per-entity emplace **copy**; no sharing anywhere | `CkTimer_Utils.cpp:58` → `CkRegistry.h:499` |
| **Inverse size↔bake correlation**: live-read features have the smallest params (~24–120 B: Timer/Tween/VisibleRange/CrowdAgent); setup-baked features the fattest (Sensor ~250 B; Probe bakes into a Jolt body; FloatAttribute is fully decomposed and keeps **no params fragment at all**) | `CkProbe_Processor.cpp:448-509,522-563`; `CkFloatAttribute_Utils.cpp:27-144` |
| **Three incompatible runtime-mutation idioms** coexist today: `TReadWrite` view · whole-struct `Replace` · CkPmg `TOptional`-delta-to-Current | `CkPmg_Fragment_Data_Donut.h:66-104` et al. |
| **AS `asset … of …` hot reload re-inits the instance in place** — stable UObject identity — but **emits no signal**, and the heal is **full**: every script save re-inits ALL literal assets, not just changed ones | `Source/CkCore/Public/CkCore/IO/CkDeferredAssetInit_AngelScript.cpp:581-627` (`ReRunLiteralAssetInits`; "always ALL literals" at :606-609). Plugin code in CkCore — **not** engine-fork code |
| **Working precedents for post-setup mutation**: GOAP debugger inspector edits route through `Request_*`; PathNetwork ships `Request_UpdateTuningAndReplan` + `_TuningRevision`; the camera CTO review already blessed params-as-PDA for that feature; `CkItemQuery_Subsystem.cpp:239-251` is an in-repo `OnObjectPropertyChanged` consumer; `CkEntitySpawner_Actor.cpp:128-135` does an editor rebuild, PIE-gated | cited files |

## 3. Core thesis — change **transport**, not storage

"Params as data asset" conflates two things: **where param values live** (storage) and **how a change reaches a live entity** (transport). Only transport is missing. Therefore:

- **Storage is untouched.** Params remain a per-entity value copy, byte-identical to today. Both stated negatives (indirection, friction) attach to storage-as-DA and disappear structurally. Reads never change speed; features that never opt in pay nothing.
- **Live editing is an editor-only sidecar.** The entire mechanism is `WITH_EDITOR` and compiles out of shipping builds. The requested "smart revert to a regular copy in shipping" falls out for free — the copy is the *only* storage in every build; there is no runtime dual mode to design, configure, or get wrong.
- **Params = frozen seed.** For new code the three mutation idioms unify on: params are the construction-time seed; runtime reconfiguration goes through requests; the processor owns any cascade. LiveTune rides that contract instead of inventing a parallel one.

## 4. Architecture — the spine

Four pieces, all editor-only except the `Link` symbol (empty inline outside `WITH_EDITOR`):

### 4.1 `FFragment_DevTuningSource` — the stamp

A `WITH_EDITOR`-only fragment placed on the feature's entity by `Link`: `FObjectKey` of the tuning asset + the `FName` of the member property the params were copied from. Nothing reads it at runtime; it exists so an asset edit can be traced back to the entities seeded from it.

### 4.2 `UCk_Utils_LiveTune_UE::Link` — the entire authoring surface

```cpp
// after any normal feature Add:
auto Health = UCk_Utils_FloatAttribute_UE::Add(Handle, Tuning->Get_Health(), ECk_Replication::Replicates);
UCk_Utils_LiveTune_UE::Link(Health, Tuning, GET_MEMBER_NAME_CHECKED(UBb_GruntTuning_PDA, _Health));
```

- Validates at call time that the named `FProperty` exists on the asset's class and its struct type matches the handle's feature params type — `CK_ENSURE_IF_NOT` loudly on mismatch (non-negotiable #3 shape), ordinary early-out after.
- Outside the editor it is an empty inline: zero shipping cost, no `#if` at call sites.
- The tuning asset is **any UObject** with params-struct UPROPERTYs (plain PDA, AS `asset … of …` literal). No base class, no registration.

### 4.3 `UCk_LiveTune_EditorSubsystem` — the change listener

- Maintains the reverse map `(asset FObjectKey, property FName) → entities` from the stamps.
- Subscribes `FCoreUObjectDelegates::OnObjectPropertyChanged` (DA edits in the details panel) **and a NEW delegate `OnAssetsReinitialized`** added at the end of `ReRunLiteralAssetInits`, closing the AS-hot-reload signal gap found in §2. This is a **CkCore-local change, not an engine-fork touch**: `ReRunLiteralAssetInits` is plugin code (`Source/CkCore/Public/CkCore/IO/CkDeferredAssetInit_AngelScript.cpp`, driven by `UCk_DeferredAssetInit_UE::OnAngelscriptPostReload` off the fork's *existing* `FAngelscriptClassGenerator::OnPostReload`). The new delegate fires only when literals actually re-inited and carries the healed-literal set, so the subsystem scopes its diffing to exactly those assets.
- On change: look up affected entities, read the fresh params struct off the asset via the stamped property name (re-resolved at dispatch — never a cached `FProperty*`, which AS reinstancing would dangle), dispatch to the re-apply registry by params **type** — subject to the dispatch-hygiene gates below.

**Dispatch hygiene (mandatory, not optimizations):**

1. **Per-`(asset, member)` value-diff gate.** The subsystem keeps a snapshot/hash of the last-dispatched value per stamp key and dispatches only on a real change. Mandatory because the AS heal is full (§2): without it, **every `.as` save would rebuild every linked entity world-wide** in an AS-first project. The diff cache is a Phase 0 deliverable.
2. **Change-type policy.** `EPropertyChangeType::Interactive` (slider scrubs; broadcast per tick) dispatches to `ViaReplace` handlers only — that IS the live-tuning feel; `ValueSet` (final commit) dispatches to all tiers. A drag must never become a rebuild storm. (Verified on the 5.7 fork: `PropertyHandleImpl.cpp:372`.) Undo/redo event shape gets verified in Phase 0; with gate #1 in place it is benign either way.
3. **Authority gate.** For replicated features the dispatcher skips client-mode entities (`Get_IsEntityNetMode_Host`-style). In PIE listen-server + client the in-process delegate reaches both worlds; a client-side rebuild of a replicated feature would locally destroy a server-owned subtree and then fight the normal net path.

### 4.4 `FCk_LiveTuneHandlerRegistry` — per-feature opt-in

Registered in the feature's `_Fragment.cpp`, mirroring `FCk_PersistenceHandlerRegistry`'s named designated-init shapes:

```cpp
// Tier 1 — live-read features (Timer, Tween, VisibleRange): Replace IS the re-apply.
FCk_LiveTuneHandlerRegistry::Register_ViaReplace<FCk_Fragment_Timer_ParamsData>({});
// optional .PostReplace fixup lambda

// Tier 2 — setup-baked features that own an in-place rebuild path (Probe):
FCk_LiveTuneHandlerRegistry::Register_ViaRequest<FCk_Fragment_Probe_ParamsData>({
    .Apply = [](FCk_Handle& InProbe, const FCk_Fragment_Probe_ParamsData& InFresh) -> void
    { UCk_Utils_Probe_UE::Request_Reconfigure(UCk_Utils_Probe_UE::CastChecked(InProbe), FCk_Request_Probe_Reconfigure{InFresh}, {}); },
});

// Tier 3 (DEFAULT) — cascading setup: rebuild + hydrate.
FCk_LiveTuneHandlerRegistry::Register_ViaRebuild<FCk_Fragment_FloatAttribute_ParamsData>({
    .Scope = ECk_LiveTune_RebuildScope::Feature,   // vs ::Entity = full recipe-respawn fallback
    .ReAdd = [](FCk_Handle& InOwner, const FCk_Fragment_FloatAttribute_ParamsData& InFresh) -> FCk_Handle
    { return UCk_Utils_FloatAttribute_UE::Add(InOwner, InFresh, ECk_Replication::Replicates); },
    // .Produce / .Hydrate default to the feature's FCk_PersistenceHandlerRegistry entries
});
```

## 5. The three re-apply tiers

| Tier | For | Mechanism |
|---|---|---|
| `Register_ViaReplace<T>` | Live-read features — processors read params every tick (Timer, Tween, VisibleRange, CrowdAgent) | `Replace<Params>` on the fragment is the whole re-apply; optional `.PostReplace` fixup |
| `Register_ViaRequest<T>` | Setup-baked features that already own (or gain) an in-place rebuild path | Route through a small new `Request_Reconfigure` + dirty-tag re-bake — generalizes Probe's existing signal→tag→rebuild cascade |
| `Register_ViaRebuild<T>` — **default** | Features whose params feed an involved setup with cascading effects | Save runtime state via the feature's persistence `Produce` → destroy the feature child subtree (`Scope::Feature`) or full recipe-respawn (`Scope::Entity`) → re-`Add` with fresh params → `Hydrate`. **"Retune with cascades" ≡ "load this entity from disk"** — the v3 rebuild+hydrate machinery just shipped and is reused wholesale. Fidelity is bounded by persistence-handler coverage, which is exactly the same contract as save/load — a gap here is a save/load gap too, so fixing it pays twice. |

Two spec points on `ViaRebuild` (folded from review suggestions 3 and 5): the hydrate step rides `FProcessor_Hydration_Dispatch`'s **existing** pending-apply machinery — one `NotReady`/retry/loud-timeout code path, never a parallel mini-dispatcher. And `Scope::Entity` validates provenance and refuses loudly: only RuntimeSpawned entities carry a `FFragment_SpawnRecipe`; ConstructSpawned children and level-placed entities have nothing to respawn (ensure + ordinary early-out per non-negotiable #3, with an invalid-input test).

Edit-time flow (designer edits `_Health.MaxValue` mid-PIE):

```text
OnObjectPropertyChanged(Tuning, _Health)        // AS reload path: NEW OnAssetsReinitialized
  → subsystem reverse-map: (Tuning, _Health) → [Entity 512, Entity 890, …]
  → registry lookup by params TYPE → ViaRebuild<FloatAttribute> handler
  → per entity: Produce(save runtime state) → destroy feature subtree
              → ReAdd(fresh params) → Hydrate(restore runtime state)
```

## 6. The 10k-identical-entities question — verdict: defer

- Measured-ish envelope: median params 0.25–1.2 MB per feature per 10k entities; ~6–8 MB aggregate for a rich entity. Not the profile's sharp edge.
- The real sharp edge is **heap members**: a `FGameplayTagContainer`/`TArray` in a params struct is 10k identical mallocs. If this ever measures, the fix is **interning heap payloads** (the InventoryItem_Definition weak-ptr idiom), not restructuring storage.
- Sharing breaks per-instance divergence (CrowdAgent writes `_MaxSpeed` into its own copy); EnTT has no shared-component concept; our pools are tombstone-mode (`in_place_delete`); `FFragment_SpawnRecipe` (UObject ptr + `FInstancedStruct` per runtime-spawned entity) already costs more per entity than a typical params struct.
- Gap noted: no per-fragment byte accounting exists (CkEcsWorldStats candidate) — so any future decision here should start with adding measurement, not with a flyweight.

## 7. Decisions settled during the design session

1. **Storage stays a per-entity value copy** — params-as-DA rejected for the general case (CkProvider's zero-consumer history is the empirical closer).
2. **Live editing is editor-only change transport** (`WITH_EDITOR` sidecar); no runtime dual mode exists to design.
3. **Params = frozen seed; runtime mutation via requests; processor owns cascades** — the unifying contract for new code.
4. **The cascading-setup answer is `ViaRebuild`**: rebuild + hydrate reusing the v3 persistence handlers, feature-subtree scope by default.
5. **10k flyweight restructuring rejected for now**; heap-payload interning is the earmarked fallback, gated on real measurement.
6. **CkProvider is not the vehicle** — it stays untouched by this design; its disposition (delete/archive + `Claude.md` correction) is a separate small decision.

## 8. Forks — CALLED by the CTO review (2026-08-05; locked)

1. **Link surface → explicit `UCk_Utils_LiveTune_UE::Link`.** Per-feature `Add` overloads are not actually available as overloads — the house no-UFUNCTION-overloads rule would force ~116 distinctly-named `Add_WithTuning` variants across three environments, and it welds an editor-only concern into every runtime `Add` signature permanently. Explicit `Link` is one API, composes with every acquisition path (`Add`, `AddMultiple`, EntityScript composition), and stays `#if`-free at call sites. Revisit ergonomics only if the pilots show the call-site repetition grating.
2. **Default-tier policy → explicit opt-in only; NO implicit `ViaRebuild`.** (a) A rebuild severs consumers (§10 risk #6) — a feature author must consciously accept that against their feature's consumer contract, not have it inferred from persistence coverage. (b) Persistence handlers were authored under save/load assumptions (fresh world, EntityScripts re-run BeginPlay and rebind); silently repurposing them for in-place surgery on a live world flips those assumptions without the author present. (c) "Nothing silent" — a details-panel edit that tears down a subtree must be traceable to a registration line. Discoverability mitigation: when an edit maps to stamped entities whose params type has no handler, the subsystem logs Display `"LiveTune: no handler registered for [Type] — feature not live-tunable"` so silence never reads as breakage. Registration is one line per feature; coverage follows demand.
3. **Module home → `Source/CkEcs/LiveTune/`; no new module.** Handlers register from each feature's `_Fragment.cpp`, so the registry's home is a build dependency of every registering module — CkEcs is the only home that adds zero new dep edges (the exact reasoning that put `FCk_PersistenceHandlerRegistry` in `CkEcs/Persistence/`). In-module precedent for every piece: `ck::FFragment_EditorSelectionOwner` (`CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Fragment.h:11-34`) is already a `WITH_EDITOR`-gated plain fragment in CkEcs; `CkEcs/Subsystem/CkEcsEditor_Subsystem.h` already hosts an editor-world subsystem in-module; `CkEcs.Build.cs:55-70` already carries the `bBuildEditor` dep block (incl. `UnrealEd`). Layout: stamp fragment + `Link` utils + `FCk_LiveTuneHandlerRegistry` in `CkEcs/LiveTune/` (all `WITH_EDITOR`; `Link` empty-inline otherwise), subsystem beside the existing editor subsystem. Split a `CkLiveTuneEditor` T5 module out later only if the subsystem grows editor UI.

## 9. Suggested phases (post-green-light; each lands as its own gate)

| Phase | Deliverable | Gate |
|---|---|---|
| **0 — Spine** | Stamp fragment + `Link` (with `FProperty` validation) + editor subsystem (reverse map, `OnObjectPropertyChanged`, **per-`(asset, member)` value-diff cache**, change-type + authority gating per §4.3) + registry with the three `Register_*` shapes | AutoTests: Link validation rejects wrong-type/missing property loudly with zero partial state; registry dispatch by type; stamp cleanup on entity destroy; **diff gate suppresses no-op and full-heal dispatch**; verify undo/redo event shape on the fork |
| **1 — Pilots** | Timer via `ViaReplace` (trivial) + FloatAttribute via `ViaRebuild` (exercises persistence-handler reuse end to end) | AutoTests: ViaRebuild round-trip preserves runtime state (mirrors save/load assertion patterns, own thin fixtures); **across-rebuild test pinning the behavior of a bound signal + a cached typesafe handle through `ViaRebuild`** (even if the pinned answer is "documented limitation", §10 #6); **re-Add-vs-record-disconnect sequencing test** (`DisallowDuplicateNames`, §10.1); `[EDITOR-VERIFY]`: edit a DA mid-PIE, watch the attribute clamp live |
| **2 — AS path** | `OnAssetsReinitialized` delegate in `ReRunLiteralAssetInits` + `utils_live_tune` AS surface + BP node verification (tri-environment, non-negotiable #4) | `[EDITOR-VERIFY]`: edit an AS asset literal, hot reload, watch the change land; AutoTest for the delegate fire |
| **3 — Rollout** | `Request_Reconfigure` for Probe (`ViaRequest` pilot), registration guidance in module `Claude.md`s, `Source/CLAUDE.md` rows, CkProvider disposition executed per Adam's call | Full toolbox gate vs recorded baseline; comment audit |

## 10. Risks

1. **Rebuild timing & re-Add sequencing** — `ViaRebuild` destroys and re-Adds mid-frame from an editor delegate. Must route through the normal deferred-destroy path (`Request_DestroyEntity` → `EndPlay` cascade), never inline during the broadcast — same discipline as the persistence dispatchers. And "re-Add on the following tick" is NOT sufficient: destroy is a multi-tick pipeline (Initiate → EndPlay → Teardown → Await → Finalize), and records like `RecordOfFloatAttributes` use `DisallowDuplicateNames` (`CkFloatAttribute_Utils.cpp:35`) — a re-`Add` can collide with the same-named dying entry still connected to the record. The rebuild driver must sequence re-Add against **actual record disconnect**, not a fixed tick delay; pinned by a Phase 1 test.
2. **Identity stability** — `FObjectKey` of the tuning asset must survive the edit paths we care about: AS re-init is in-place (verified, §2) and DA detail-panel edits don't reinstance; C++ hot-reload reinstancing is editor-only churn the subsystem must tolerate (drop + re-resolve, never crash).
3. **Networking semantics** — the editor delegate fires in-process, so PIE listen-server + clients all see it; a real remote client would not. Tuning is authority-local by nature; replicated fragments re-added by `ViaRebuild` propagate through the normal net path. Documented AND gated: the §4.3 authority gate skips client-mode entities for replicated features, so a client world never locally rebuilds a server-owned subtree.
4. **Hydration fidelity** — bounded by persistence-handler coverage (deliberate, §5); the failure mode is silent state reset on rebuild for uncovered state. The `NotReady`/retry + loud-timeout semantics of the load path apply unchanged.
5. **Reverse-map hygiene** — stamps must unregister on entity destroy (subsystem listens to the same teardown the debugger tools use); a stale map entry re-applying onto a dead handle must be impossible, not merely rare.
6. **Observer invalidation across `ViaRebuild`** — destroying + re-Adding the feature subtree changes entity identity in a *live* world, silently severing two consumer classes: (a) cached typesafe handles — the *documented recommended pattern* ("cache the attribute handle at setup time", `CkAttribute/CLAUDE.md` anti-pattern #2) — become tombstones; (b) signal bindings (`BindTo_OnValueChanged` et al.) live on the destroyed entity and are gone — nothing rebinds, so after tuning Health a bound health-bar UI just stops updating. Save/load does not have this problem (the whole world rebuilds and EntityScript BeginPlay replays the binds); `Scope::Feature` replays nothing. **Stance (locked by the CTO review): accept + document.** The rebuild driver logs Display naming how many signal bindings were dropped per rebuild; features with hot external signal surfaces should prefer `ViaRequest`; a rebind-assist can come later if the pilots demand it. Pinned by the Phase 1 across-rebuild AutoTest — the FloatAttribute pilot (the most signal-observed feature in the framework) will exercise this immediately, by design.
