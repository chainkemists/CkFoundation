# LiveTune design — CTO Review

> **Workflow:** Read the brief below, then read the linked design doc. Fill in the **CTO Review Response** section at the bottom of this file and commit — the plan author / their assistant picks up your notes from there. You need no context from any prior conversation; everything required is in this file, the design doc, and the repo.

---

## Reviewer brief

### Your role

Senior reviewer / architect for CkFoundation. Your responsibilities:

- Catch architectural issues before implementation starts (layer boundaries, editor/runtime split, seams that will block future work).
- Catch convention/idiom mismatches with the existing codebase (naming, registry shapes, macro usage, ensure discipline).
- Identify unclear, missing, or unsafe steps in the phase plan.
- Green-light, green-light with notes, or list concrete blockers — and weigh in on the three deliberately-open forks (design doc §8).

You are expected to **read code in the repo**, not just review the plan in isolation — the "Critical context" list below is the minimum reading set.

### What's being built

**LiveTune** — live-tunable feature params: a designer edits a data asset (Blueprint PDA or AngelScript `asset … of …` literal) mid-PIE and the change lands on live entities in real time, without stopping and relaunching. The design's core move is that **params storage is untouched** — per-entity value copies exactly as today — and only an editor-only *change transport* is added: a `WITH_EDITOR` stamp fragment + one `Link` call at the Add site + an editor subsystem that maps asset edits back to entities and dispatches through a per-feature re-apply registry (three tiers: `ViaReplace` / `ViaRequest` / `ViaRebuild`). Shipping builds compile the whole mechanism out. The cascading-setup edge case ("changing this param re-bakes a Jolt body / re-decomposes an attribute") is answered by `ViaRebuild`: save → destroy feature subtree → re-Add → hydrate, reusing the v3 persistence handlers wholesale.

### Reference material the design is informed by

- **CkProvider** (`Source/CkProvider/`) — the in-repo params-as-DA indirection attempt: 10 provider families, zero consumers ever. The design treats this as empirical evidence against storage-as-DA; verify you agree with that reading.
- **v3 persistence machinery** (`Source/CkEcs/Persistence/CkPersistenceHandlerRegistry.h`, `CkSnapshot`) — `ViaRebuild` is a thin driver over `Produce`/`HydrationApply`; the registration ergonomics (named designated-init shapes) are copied from it.
- **Probe's rebuild cascade** (`CkProbe_Processor.cpp:448-509,522-563`) — the existing signal→tag→re-bake pattern `ViaRequest` generalizes.
- **PathNetwork** `Request_UpdateTuningAndReplan` + `_TuningRevision` (`CkPathNetwork_Utils.h:189`) — prior art for post-setup retuning through a request.
- **AS deferred asset init** (`CkDeferredAssetInit_AngelScript.cpp:581-627`, `ReRunLiteralAssetInits`) — hot reload re-inits asset literals **in place** (stable UObject identity) but emits no signal; the design adds one new delegate there.

### Plan location

[2026-08-05-LiveTune-design.md](../specs/2026-08-05-LiveTune-design.md)

### Critical context — read before reviewing

You **must** read these (paths relative to `Plugins/CkFoundation/`):

- `CLAUDE.md` (plugin root) — non-negotiables (esp. #3 ensure discipline, #4 tri-environment, #9 reuse-before-bespoke), the persistence-handler contract section.
- `Source/CLAUDE.md` — module tier table, "Replicated + persisted fragments" pattern (the registration shape being mirrored), the `Request_*`-takes-a-struct rule.
- `Source/CkEcs/Persistence/CkPersistenceHandlerRegistry.h` — the registry whose designated-init ergonomics and `NotReady`/retry semantics `ViaRebuild` reuses.
- `Source/CkProvider/Public/CkProvider/CkProvider_Data.h` (+ its `Claude.md`) — confirm the zero-consumer claim for yourself; it is load-bearing for the storage decision.
- `Source/CkSpatialQuery/.../CkProbe_Processor.cpp:448-563` — the bake/rebuild cascade that motivates tiers 2 and 3.
- `Source/CkAttribute/.../CkFloatAttribute_Utils.cpp:27-144` — a feature with **no params fragment at all** (fully decomposed at Add); the hardest case `ViaRebuild` must handle, and pilot #2.
- `CkDeferredAssetInit_AngelScript.cpp:581-627` — where the new `OnAssetsReinitialized` delegate would live (the design's only engine-fork touch).
- `Source/CkEcs/Handle/CkHandle.h:71-77` — tombstone-mode pools context for the §6 flyweight verdict.

### Design decisions already settled (do NOT relitigate unless you see a real problem)

1. Params storage stays a per-entity value copy — params-as-DA rejected for the general case.
2. Live editing is an editor-only sidecar (`WITH_EDITOR` end to end); there is deliberately no runtime dual mode — shipping "revert to a copy" is structural, not configured.
3. Params = frozen seed; runtime mutation goes through requests; the processor owns cascades (unifying today's three mutation idioms for new code).
4. The cascading-setup answer is `ViaRebuild` reusing the v3 persistence handlers — its fidelity is *deliberately* bounded by persistence coverage (a gap there is a save/load gap too).
5. The 10k-identical-entities flyweight is deferred; if it ever measures, the fix is interning heap payloads, not restructuring storage (design doc §6 has the numbers).
6. CkProvider is untouched by this design; its disposition is a separate decision.

### What I specifically want you to scrutinize

#### A. Architecture / decomposition

- **The three open forks (design doc §8) — these are yours to call:** (1) explicit `Link` vs per-feature `Add` overloads; (2) `ViaRebuild` implicit for any persistence-covered feature vs explicit opt-in only; (3) module home — new `CkLiveTune` vs inside `CkEcs`.
- Is coupling live-tune re-apply to the persistence registry sound, or does it create a hidden contract (features now can't change their `Produce` shape without thinking about tuning)?
- Is `(FObjectKey, FName)` the right stamp identity, or should the stamp capture the resolved `FProperty` offset / a struct-typed accessor?
- The subsystem dispatches by params **type** — is that granular enough, or do multi-feature assets need per-stamp dispatch?

#### B. Convention compliance

- Registry shape vs `FCk_PersistenceHandlerRegistry` (named `Register_*` variants, designated-init args, compile-enforced required slots) — faithful mirror?
- A `WITH_EDITOR`-only fragment in a runtime module: precedent-legal, or does the stamp belong behind the editor-module boundary with a runtime-side indirection?
- `Link`'s validation follows non-negotiable #3 (hoisted condition, empty ensure body, ordinary early-out) and needs the invalid-input rejection tests the ensure-boundary rule mandates — does Phase 0's gate cover that fully?
- Tri-environment surface (`utils_live_tune` in AS, BP node) — anything in the proposed signatures that won't survive the AS wrapper generator (delegates-last, no overloads, no `BlueprintInternalUseOnly`)?

#### C. Version-specific API specifics (UnrealEngine-Angelscript 5.7 fork)

- `FCoreUObjectDelegates::OnObjectPropertyChanged` granularity on 5.7 — interactive vs final change events, `MemberProperty` vs leaf `Property` for nested struct edits (the design keys on the top-level member FName).
- Adding `OnAssetsReinitialized` inside `ReRunLiteralAssetInits` — acceptable fork surface? Any existing hook that already fires there and could be reused instead?
- `FObjectKey` behavior across C++ hot-reload reinstancing on the fork (risk §10.2's "tolerate, drop + re-resolve" — is that sufficient?).

#### D. Test coverage

- Phase 0/1 gates lean on headless AutoTests for Link validation, dispatch, and the ViaRebuild round-trip, with `[EDITOR-VERIFY]` only for the actual mid-PIE edit UX — is the split right? Can the `OnObjectPropertyChanged` path itself be exercised headlessly (fire the delegate by hand)?
- The ViaRebuild round-trip test mirrors save/load specs — is "state survives rebuild" asserted against the same fixtures the snapshot suite uses, or does it need its own?

#### E. Risks — sized correctly?

- Design doc §10 lists five. Are any missing or under-weighted — e.g. re-entrancy (a rebuild triggering construction scripts that themselves `Link`), undo/redo transactions firing `OnObjectPropertyChanged` storms, or PIE-vs-editor-world entity disambiguation in the reverse map?

#### F. Forward-compat

- Does the stamp/registry design leave room for later: runtime (non-editor) tuning consoles, per-instance param overrides, or network-synced tuning — without building any of them now?
- Pilot choice (Timer `ViaReplace` + FloatAttribute `ViaRebuild`) — right pair, or would Probe (`ViaRequest`) surface more risk earlier?

### Output format — fill in the CTO Review Response section below

Be direct. If the plan is good, say so and green-light it — don't manufacture issues to look thorough. Specific blockers tied to a phase/section, not vague concerns. The three §8 forks need an explicit call each.

---

## CTO Review Response

### Verdict

`CHANGES REQUESTED` — surgical, not structural. The architecture (transport-not-storage, editor-only sidecar, three tiers, persistence-handler reuse) is approved as designed. Two design-doc amendments are required before Phase 0 starts, because implementation built from the doc as written would be wrong by default in ways Phase 1/2 gates would only discover painfully. Both are ~an hour of spec work. Fork calls below stand regardless.

### Fork calls (design doc §8)

1. **Link surface: explicit `UCk_Utils_LiveTune_UE::Link`.** Per-feature `Add` overloads are not actually available as overloads — the house "No UFUNCTION overloads" rule (root CLAUDE.md, Naming) would force ~116 distinctly-named `Add_WithTuning` UFUNCTIONs across three environments, and it welds an editor-only concern into every runtime `Add` signature permanently. Explicit `Link` is one API, composes with every acquisition path (`Add`, `AddMultiple`, EntityScript composition), and its empty-inline shipping shape keeps call sites `#if`-free. The call-site noise (asset + member repeated after the `Get_` call) is real but mild; revisit ergonomics only if pilots show it grating.
2. **Default-tier policy: explicit opt-in only. No implicit ViaRebuild.** Three reasons, first one decisive: (a) a rebuild destroys and re-creates the feature subtree — entity identity changes, which severs cached typesafe handles and signal bindings held by consumers (see Blocking 1); a feature author must consciously accept that against their feature's consumer contract, not have it inferred from persistence coverage. (b) Persistence handlers were authored under save/load assumptions (fresh world, EntityScripts re-run BeginPlay and rebind); silently repurposing them for in-place surgery on a live world flips those assumptions without the author present. (c) House culture is "nothing silent" — an edit that triggers a subtree teardown as a side effect of a details panel should be traceable to a registration line. Mitigate the discoverability cost cheaply: when an edit maps to stamped entities whose params type has no handler, the subsystem logs Display `"LiveTune: no handler registered for [Type] — feature not live-tunable"` so silence never reads as breakage. Registration is one line per feature; coverage will follow demand.
3. **Module home: CkEcs, no new module.** Decisive argument: handlers register from each feature's `_Fragment.cpp`, so the registry's home is a build dependency of every registering module — CkEcs is the only home that adds zero new dep edges. This is the exact reasoning that put `FCk_PersistenceHandlerRegistry` in `CkEcs/Persistence/` ("split out … so the save path reuses it without a Net dependency"). Precedent for every piece, verified in-module: `ck::FFragment_EditorSelectionOwner` (`CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Fragment.h:11-34`) is already a `#if WITH_EDITOR`-gated plain fragment in CkEcs — the stamp is the same shape; `CkEcs/Subsystem/CkEcsEditor_Subsystem.h` already hosts an editor-world subsystem in-module, and `CkEcs.Build.cs:55-70` already carries the `if (Target.bBuildEditor)` dep block (incl. `UnrealEd`) the subsystem needs. Suggested layout: `Source/CkEcs/LiveTune/` (stamp fragment + `Link` utils + `FCk_LiveTuneHandlerRegistry`, all `WITH_EDITOR`; `Link` empty-inline otherwise) with the subsystem beside the existing editor subsystem. Split a `CkLiveTuneEditor` T5 module out later only if the subsystem grows editor UI.

### Blocking issues

1. **§10 is missing the biggest ViaRebuild risk: observer invalidation.** Destroying + re-Adding the feature subtree changes entity identity in a *live* world. Two consumer classes are silently severed: (a) cached typesafe handles — which is the *documented recommended pattern* ("cache the attribute handle at setup time", `CkAttribute/CLAUDE.md` anti-pattern #2) — become tombstones; (b) signal bindings (`BindTo_OnValueChanged` et al.) live on the destroyed entity and are gone; nothing rebinds, so after tuning Health the health-bar UI just stops updating. Save/load does not have this problem — there the whole world rebuilds and EntityScript BeginPlay replays the binds; LiveTune's `Scope::Feature` rebuild replays nothing. The FloatAttribute pilot (the most signal-observed feature in the framework) will hit this in the first hour of Phase 1 — which is an argument *for* that pilot choice, but the design must enter it with a stance, not discover it. Required: add as §10 risk #6 with a chosen mitigation (recommended: accept + document + a Display log naming how many bindings were dropped per rebuild, plus guidance that features with hot external signal surfaces should prefer `ViaRequest`; a rebind-assist can come later if pilots demand it), and extend the Phase 1 gate with an AutoTest pinning the across-rebuild behavior of a bound signal and a cached handle — even if the pinned answer is "documented limitation". Related sequencing hazard to fold into §10.1: destroy is a 3-tick pipeline (Initiate → EndPlay → Teardown → Await → Finalize), and `RecordOfFloatAttributes` uses `DisallowDuplicateNames` (`CkFloatAttribute_Utils.cpp:35`) — a re-`Add` "on the following tick" can collide with the same-named dying entry still connected to the record. The rebuild driver must sequence re-Add against actual record disconnect, not just "next tick"; pin with a test.
2. **§4.3 dispatch hygiene is unspecified, and the defaults are wrong for this repo.** Three sub-items, one amendment: (a) **Value-diff gating is mandatory, not an optimization.** The AS hot-reload path is full-heal — `ReRunLiteralAssetInits(FullHeal=true)` re-inits *every* literal asset on *every* script save (`CkDeferredAssetInit_AngelScript.cpp:606-609`, "Always ALL literals"). Without a per-(asset, member) value snapshot/hash compared before dispatch, every `.as` save rebuilds every linked entity world-wide in a BB project that is AS-first. (b) **Interactive vs final change policy.** Verified on the 5.7 fork: slider scrubs broadcast `EPropertyChangeType::Interactive` per tick and commits broadcast `ValueSet` (`Engine/Source/Editor/PropertyEditor/Private/PropertyHandleImpl.cpp:372`). `ViaReplace` may follow Interactive (that is the live-tuning feel); `ViaRequest`/`ViaRebuild` must dispatch only on final commits or a drag becomes a rebuild storm. Undo/redo event shape should be verified in Phase 0 — with (a) in place it is benign either way. (c) **Authority gating.** §10.3's "documented, not fixed" is not quite enough: in PIE listen-server + client (a standard BB test config) the in-process delegate reaches both worlds, and a client-side rebuild of a *replicated* feature locally destroys a server-owned subtree and then fights the normal net path. One line in the dispatcher — skip client-mode entities for replicated features (`Get_IsEntityNetMode_Host`-style gate) — plus the doc note. Required: amend §4.3 to specify all three; Phase 0's subsystem deliverable includes the diff cache.

### Non-blocking suggestions

1. **Correct the "engine-fork touch" claim (§2, §4.3) — it overstates the risk.** `ReRunLiteralAssetInits` is *plugin* code: `Source/CkCore/Public/CkCore/IO/CkDeferredAssetInit_AngelScript.cpp` (namespace `ck_deferred_asset_init_angelscript`), driven by `UCk_DeferredAssetInit_UE::OnAngelscriptPostReload`, which subscribes to the fork's existing `FAngelscriptClassGenerator::OnPostReload` delegate (`CkDeferredAssetInit_AngelScript.cpp:38-39`). The new `OnAssetsReinitialized` delegate is therefore a CkCore change — zero engine-fork surface. On the "existing hook" question: LiveTune *could* subscribe to `OnPostReload` directly and would in practice run after CkCore's heal (static registration at module init precedes subsystem init), but that ordering is implicit; the explicit delegate at the end of `ReRunLiteralAssetInits` is the right call — it fires only when literals actually re-inited and should carry the healed-literal set so the subsystem scopes its value-diff to exactly those assets.
2. **Stamp identity: agree with `(FObjectKey, FName)`; do not capture the resolved `FProperty`.** AS reload reinstances script-defined types, so a cached `FProperty*` dangles exactly when LiveTune is most useful; name-based re-resolve at dispatch is the robust half, and `FObjectKey` survives the in-place re-init (verified stable-identity claim in §2). Two boundaries to document: the member must be a *top-level* UPROPERTY of the tuning asset — `Link`'s FindPropertyByName validation enforces this structurally, and it matches `MemberProperty` keying (nested-struct edits report the top-level member; verified broadcast site `Obj.cpp`) — and a `TArray`-of-params-structs member cannot be element-addressed by FName alone; document as unsupported for now, extend the stamp with an optional index if demand appears.
3. **Specify that ViaRebuild's hydrate rides `FProcessor_Hydration_Dispatch`'s existing pending-apply machinery** rather than a parallel mini-dispatcher. §10.4 implies it ("semantics of the load path apply unchanged"); make it explicit so there is one NotReady/retry/loud-timeout code path, not two that drift.
4. **Test seams (answers to §D):** yes, the `OnObjectPropertyChanged` path is headlessly exercisable — hand-build an `FPropertyChangedEvent` and broadcast, or better, expose a `#if WITH_EDITOR` `Test_SimulatePropertyChange(Asset, MemberName)` UFUNCTION on the subsystem so AS AutoTests drive the full dispatch path without a details panel. The ViaRebuild round-trip test should use its own thin fixtures that reuse the snapshot suite's *assertion patterns*, not its world fixtures — coupling LiveTune's gate to the save/load suite's fixtures makes both brittle.
5. **`Scope::Entity` must validate provenance and refuse loudly.** Only RuntimeSpawned entities carry a SpawnRecipe; ConstructSpawned children and level-placed entities have nothing to respawn. Ensure + early-out per non-negotiable #3, plus an invalid-input test.
6. **CkProvider disposition (settled #6 — not relitigating):** this review independently re-confirmed the zero-consumer claim, and both `CkProvider/CLAUDE.md`'s "Used by" list and `CkAttribute/CLAUDE.md`'s "modifiers come from providers" line are false today. The stale docs actively mislead reviewers; worth executing the disposition decision soon after this lands.

### Convention compliance spot-checks performed

- `Source/CkEcs/Public/CkEcs/Persistence/CkPersistenceHandlerRegistry.h` (whole file) — registry shape being mirrored: named `Register_*` variants, designated-init args structs, `FRequired*` deleted-default-ctor slot enforcement, lazy typed registration. The §4.4 proposal is a faithful mirror; carry the `FRequired*` pattern over so `ViaRequest.Apply` / `ViaRebuild.ReAdd` are compile-enforced.
- `Source/CkProvider/Public/CkProvider/CkProvider_Data.h` + repo-wide greps (`Ck_Provider_` across CkFoundation `Source/` + `Script/`, BB `Source/` + `Script/`) — zero consumers outside CkProvider's own two files. Claim confirmed.
- `Source/CkSpatialQuery/Public/CkSpatialQuery/Probe/CkProbe_Processor.cpp:440-563` — the dirty-tag → `TProcessor_ProbeUpdateShape` re-bake cascade `ViaRequest` generalizes; reading confirms it is a real in-place rebuild path (shape replaced, body kept).
- `Source/CkAttribute/Public/CkAttribute/FloatAttribute/CkFloatAttribute_Utils.cpp:27-144` — fully-decomposed Add (no params fragment retained; child entity per attribute; min/max/refill sub-composition; `DisallowDuplicateNames` record at :35). Confirms both "hardest ViaRebuild case" and Blocking 1's sequencing hazard.
- `Source/CkEcs/Handle/CkHandle.h:69-75` — global `in_place_delete` component_traits; supports the §6 flyweight-defer verdict.
- `Source/CkCore/Public/CkCore/IO/CkDeferredAssetInit_AngelScript.cpp:36-39, 581-627` — post-reload subscription + full-heal semantics (basis of Blocking 2a and Suggestion 1).
- `Source/CkEcs/EditorSelectionOwner/CkEditorSelectionOwner_Fragment.h`, `CkEcs/Subsystem/CkEcsEditor_Subsystem.h`, `CkEcs.Build.cs:55-70` — precedent set for Fork 3.
- Engine fork (`D:\Repositories\UnrealEngine-Angelscript`): `PropertyHandleImpl.cpp:372` (Interactive vs ValueSet), `Obj.cpp` (`OnObjectPropertyChanged.Broadcast` site).
- In-repo `OnObjectPropertyChanged` precedents: `CkInventory/Query/CkItemQuery_Subsystem.cpp:33-34` and `CkVoxelNavEditor/.../CkVoxelNavPreview_EditorSubsystem.cpp:145-146` — two existing consumers; the subscribe/unsubscribe shape to copy.

### Design / architecture observations

- **The core thesis is correct and well-evidenced.** Separating transport from storage dissolves both stated negatives structurally instead of configuring around them, and the CkProvider zero-consumer history (now triple-verified) is legitimate empirical evidence against storage-as-DA. The "params = frozen seed; requests own runtime mutation; processor owns cascades" contract is the right unification of the three existing idioms.
- **Reusing the persistence handlers for ViaRebuild is sound, not a hidden contract** (§A2 question): the Produce/HydrationApply contract is already "restores runtime state faithfully", and LiveTune adds a caller, not a clause — a Produce-shape change that breaks tuning would break save/load identically, and it is *better* that both break together (fix pays twice, exactly as §5 argues). The one caveat is Suggestion 3: keep it literally the same dispatcher so the contract cannot fork.
- **Dispatch granularity (§A4) is right as designed:** per-stamp granularity comes from the `(asset, member)` reverse map; the type-keyed registry only selects the handler. Two same-typed members on one asset dispatch independently. The type-keyed-handler constraint (one handler per params type) mirrors the persistence registry's constraint and is acceptable.
- **§6 flyweight verdict: agree with defer, and with the framing.** Tombstone-mode pools + per-instance divergence make sharing schemes strictly worse than interning heap payloads; "add measurement first" (per-fragment byte accounting) is the correct gate on any future revisit.
- **Pilot pair (§F): keep it.** Timer/ViaReplace proves the trivial path; FloatAttribute/ViaRebuild will surface Blocking 1 immediately, which is what a pilot is for. Probe/ViaRequest in Phase 3 is correctly sequenced — its `Request_Reconfigure` is new API surface and shouldn't gate the spine.
- **Forward-compat (§F) is adequate without pre-building:** the stamp/registry seam is transport-agnostic — a future runtime tuning console or per-instance override system would replace the *listener*, not the registry; network-synced tuning would ride requests, which tiers 2/3 already route through. Nothing in the design forecloses these.
- Confidence labeling: everything cited with file:line above was read this session. Inferred (flagged for Phase 0 verification): undo/redo event shape on the fork; exact record-disconnect tick within the destroy pipeline (Blocking 1's sequencing test pins it).

### Sign-off conditions (only if "CHANGES REQUESTED")

The minimal set — all design-doc edits, no re-architecture:

1. Add §10 risk #6 (observer invalidation across ViaRebuild: cached handles + signal bindings; chosen stance per Blocking 1), fold the record/`DisallowDuplicateNames` re-Add sequencing hazard into §10.1, and extend the Phase 1 gate with the across-rebuild binding/handle AutoTest.
2. Amend §4.3 with the dispatch-hygiene spec (value-diff gate; Interactive→ViaReplace-only, final-commit→all-tiers; authority gating for replicated features) and add the diff cache to Phase 0's deliverable.
3. Editorial: correct the "engine-fork touch" claim (§2, §4.3) to a CkCore-local delegate per Suggestion 1 — it changes the risk accounting, so it rides the sign-off rather than waiting.

With those in, this is a GREEN-LIGHT — fork calls as above, phases and gates otherwise approved as written.

---

### Reviewer

- **Name:** Claude (Fable 5) — CTO review session
- **Date:** 2026-08-05
