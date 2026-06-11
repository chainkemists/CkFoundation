# CkAngelscriptGenerator

**Purpose:** Editor-only code generator that emits AngelScript accessor files (`Script/Generated/*.as`) and a dynamic-handle JSON registry (`Script/Generated/DynamicHandleTypes.json`). Also owns the **AS bootstrap self-heal dispatcher** that auto-recovers the editor from "stale committed accessor files cause AS compile to fail at launch" deadlocks.

**Depends on:** `CkCVar`, `CkCore`, `CkDynamic`, `CkEcs`, `CkEcsExt`, `CkEntityExtension`, `CkLabel`, `CkLog`, `CkRecord`, `CkSettings`.
**Used by:** Editor build process — invoked at engine startup, before each AS compile, and after each successful compile.

---

## What this module is responsible for

Three loosely-coupled responsibilities under one module:

1. **Generators** — write the canonical generated `.as` / `.json` accessor files from live reflection + asset state.
   - `FCkAngelscriptEntityScriptParamsGenerator` — `<Plugin>_EntitySpawnParams.as`, one per plugin.
   - `FCkAutoTestWrapperGenerator` — `<Plugin>_AutoTestActors.as`, one per plugin.
   - `UCkAssetRegistrySubsystem` — `BusterBlockAssets.as`, `RawAssets.as`, `EngineAssets.as` (driven by `UCkAssetRegistryConfig` data assets).
   - `UCkDynamicHandleSubsystem::GenerateHandleTypeRegistry()` — `DynamicHandleTypes.json`.

2. **AS bootstrap self-heal dispatcher (Rev 10)** — module-level wiring in `FCkAngelscriptGeneratorModule::StartupModule()` that recovers automatically when a committed accessor file is stale at editor launch. See *Self-heal dispatcher* below.

3. **Drift-detection commandlet** *(not yet re-introduced in Rev 10; preserved as unwired reference on `archive/rev9-as-attempt-2026-05-11`)* — CI guardrail that runs the deterministic Tier 1+2 generators and lets `git diff --exit-code` flag drift between committed and freshly-regenerated files.

---

## Self-heal dispatcher

The dispatcher lives under `SelfHeal/`. Four pieces:

- `CkAngelscriptGenerator_AsErrorParser.{h,cpp}` — parses Hazelight's AS compile-error output (sourced via `FAngelscriptManager::Get().FormatDiagnostics()`) into typed root-cause records. Recognizes four error classes: `No matching signatures to '<NS>::<func>(<args>)'`, `No matching signatures to '<Ident>(<args>)'` (bare constructor-style call — no `::`; the direct-construction shape of a missing `F<X>_SpawnParams`), `Identifier '<X>' is not a data type`, and `Instead found '<string constant>'` (the unique signature of a `"foo " "bar"` C-style adjacent-literal splice — AS rejects, Hazelight emits as a two-line pair with the generic `Expected ')' or ','` companion that the parser ignores). Cascade noise (`Unknown`, `_Iterator`, `'<Field>' is not a member of 'Unknown'`, etc.) is dropped.
- `CkAngelscriptGenerator_AsSourceScanner.{h,cpp}` — finds `class U<X>` in the project + plugin `Script/` trees (excluding `Generated/` and `_StubRecovery_*`) and textually parses its flattened `UPROPERTY(ExposeOnSpawn)` members: verbatim type text, names, base-first declaration order (mirroring `Get_ExposedPropertiesOfClass`'s filter + ordering). AS bases recurse through more source scans; the first base with no `.as` declaration switches to reflection — C++ classes ARE registered during a failed compile. This is what makes SOURCE-DERIVED stub synthesis possible: the class is invisible to `UObjectIterator`, but its declaring source file is just a file on disk.
- `Assets/CkAssetRegistry_ClassResolver.{h,cpp}` *(not under `SelfHeal/`, but load-bearing for it)* — asset → emit-ready AS class-name resolution (Tiers 2/2.5/2.6: LoadObject walk, AssetData tags, .uasset linker walk), **shared by the canonical generator's sync-resolve path and the AssetRegistry stub synthesizer**. Both MUST resolve through the same tiers or canonical-vs-self-heal divergence triggers a synth-cleanup loop. Every tier walks past Blueprint-generated classes in the parent chain (BP-of-BP) to the nearest native/AS ancestor — a BPGC name is not an AS identifier and would make the emitted accessor fail AS compile (pinned by the 2026-06-11 `LD_Light_BB_BP` incident).
- `CkAngelscriptGenerator_StubSynthesizer.{h,cpp}` — synthesizes recovery stubs into a SIBLING file `_StubRecovery_<Plugin>_EntitySpawnParams.as` next to the canonical (atomic write; the canonical is never touched — AS merges the multi-file namespace at compile time). Two flavors: **source-derived full shape** (preferred — fielded struct with real names/types + positional ctor + both `Params()` overloads, built from the scanner's output; covers direct-construction and field-access callers) and the **error-text fallback** (empty struct + `Params(<arg types>)` with `Arg0..ArgN` names, built from the compile error alone; covers `Params()`-overload callers and owns incremental drift when the struct already exists in the canonical). See *Sibling-file stub model* below.
- `CkAngelscriptGenerator_Dispatcher.{h,cpp}` — classifies parsed roots into recovery strategies and applies them.

### Recovery strategies

| Error pattern | Strategy | Wired? |
|---|---|---|
| `<EntityScriptClass>::Params(<args>)` not found | **Synthesize stub** into a sibling `_StubRecovery_<Plugin>_EntitySpawnParams.as` — AS merges the sibling's namespace block with the canonical's, finds the missing overload, compile succeeds. `OnPostCompile` deletes the sibling file after a successful regen. Prefers the **source-derived full shape** (below); falls back to the error-text stub when the class source can't be found or the struct already exists in the canonical (incremental drift — the per-signature path owns it). | ✓ |
| `F<X>_SpawnParams` direct construction — bare ctor call (`No matching signatures to 'F<X>_SpawnParams(<args>)'`) or missing declared type (`Identifier 'F<X>_SpawnParams' is not a data type`) | **Source-derived full-shape stub**: scan `Script/` trees for `class U<X>`, parse its flattened ExposeOnSpawn properties (names + verbatim types + base-first order; C++ bases via reflection), emit a fielded struct + positional ctor + both `Params()` overloads into the sibling. This is what lets `*_EntitySpawnParams.as` be gitignored: on a fresh clone, direct-construction / field-access callers (`P.Phase = ...`) compile against the stub, the compile succeeds, and `OnPostCompile` regenerates the canonical. Defaults are deliberately omitted (compile-irrelevant; canonical regen restores them seconds later). When the class source can't be found (e.g. deleted class, stale caller) the fallback emits the no-arg empty-struct shape — field-access callers then surface the convergence banner, which is correct (the caller is genuinely broken). | ✓ |
| `Identifier 'FCk_Handle_<X>' is not a data type` | **Synthesize JSON entry** in a sibling `_StubRecovery_DynamicHandleTypes.json`. `FCkDynamic_HandleTypeRegistry::LoadFromJsonRegistry` merges the sibling's `HandleTypes` into the canonical load (dedup by `TypeName`, stub entries win). Register a temporarily permissive validator in-memory. `OnPostEngineInit` then runs `GenerateHandleTypeRegistry` + `DiscoverAndRegisterAllDefinitions` — the now-discoverable data asset's strict `RequiredFragments` is written into the canonical JSON AND the in-memory validator is replaced via `FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle`. No restart needed. | ✓ |
| `assets[::*]::<func>(<args>)` not found | **Synthesize stub** into a sibling `_StubRecovery_<MatchedAssetsFile>.as` when the synthesizer can resolve the asset's real UClass (Tier 1: native class on `AssetData`; Tier 2: sync-load the BP and walk `Get_NonBlueprintParentClass`). **Tier 3 UObject fallback was REMOVED 2026-05-13** (probe_a2.log) — when neither tier resolves a real UClass, refuse and surface the actionable banner so Hazelight's modal continues displaying the original `No matching signatures` error to the user instead of a parser-blind typed-conversion derivative. See `Tier3_IsAllowed` and the `Apply_Strategy::KickGenerator_AssetRegistry` block. | ✓ (Tier 1/2 synthesize; Tier 3 refuses) |
| `Instead found '<string constant>'` | **Diagnose only — no auto-fix.** This is an author-side bug (`"foo " "bar"` C-style adjacent literals; AS doesn't splice them) in user-authored AS source, not in a generated file the dispatcher owns. Mutating user code is out of contract. The recognized branch emits an actionable banner with file:line:col plus the suggested fix ("join into one literal" / "f-string interpolation" / "local-variable chain") and **counts as an actionable strategy** so the outer flow doesn't fall through to the "no recognized roots" terminal — that's the path that wedges headless test runs after AS post-compile with no usable diagnostic in the editor's main log. | ✓ |
| Anything else | **Terminal banner** (log only) — dispatcher cannot act | ✓ |

#### Why DynamicHandle uses a temporary permissive validator (and how it gets fixed)

The synthesized JSON stub has empty `RequiredFragments` because at modal-tick time the data asset (`asset X of UCkDynamic_HandleDefinition`) hasn't materialized yet — it's defined in AS code that hasn't compiled successfully. `FCkDynamic_HandleTypeRegistry::CreateMultiFragmentValidator` turns an empty fragment list into a *permissive* validator that returns true for any valid handle. That's enough to unblock the AS compile.

Without further action, this would be unsafe: `handle.As_<X>()` calls would succeed regardless of fragments, returning a typed handle that references an entity without the expected fragments. To close that hazard, Rev 10 adds:

1. **`FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle`** (CkEcs) — public API that replaces an existing entry's validator + cast lambdas + metadata in place. Possible because the AS-engine method bindings hold a stable raw pointer to the `FCkAngelScript_HandleTypeInfo` struct (via `userData` for self-type methods, and via the refactored `FAsMethodAuxData::TargetType` / `FIsMethodAuxData::TargetType` for cross-handle methods) and read the function fields at call time, not at binding time. No AS-engine re-registration needed.
2. **`FCkDynamic_HandleTypeRegistry::RegisterHandleType` is register-or-update** — if the type is already registered, it routes to `UpdateExistingDynamicHandle` instead of returning false. This means *any* path that re-runs registration (the PreCompile hook's `LoadFromJsonRegistry`/`DiscoverAndRegisterAllDefinitions`, the manual editor button `ForceRefreshDynamicHandleBindings`, the dispatcher's deferred OnPostEngineInit refresh) updates existing entries.
3. **OnPostEngineInit deferred regen** (`Module.cpp::Maybe_RegenDynamicHandleJson_OnPostInit`) — after the recovered session reaches main screen, calls `UCkDynamicHandleSubsystem::GenerateHandleTypeRegistry` (writes proper JSON sourced from the now-discoverable data asset) followed by `FCkDynamic_HandleTypeRegistry::DiscoverAndRegisterAllDefinitions` (which, via register-or-update, replaces our permissive in-memory validator with the strict one from the data asset).

Side-effect bug fix: the manual `ForceRefreshDynamicHandleBindings` editor button now actually updates existing handle types when their `RequiredFragments` change on the underlying data asset. Previously a silent no-op for that case (the append-only `RegisterHandleType` skipped, so the button regenerated the JSON file but in-memory bindings stayed stale until editor restart).

### The modal-tick deferral — critical timing constraint

**Do not write files synchronously inside `OnAngelscriptReloadHadErrors`.** Empirical finding 2026-05-12:

`GetReloadHadErrors().Broadcast()` fires synchronously from inside `CompileModules` (engine: `AngelscriptManager.cpp:2835`) on initial-compile failure. At that point Hazelight has not yet opened its modal and has not yet started its hot-reload checker thread (the thread is created at `:990` inside the modal-open block, AFTER our broadcast returns). The thread's first scan establishes mtime baselines for every `.as` file by reading current disk state. Any file we write during the broadcast becomes part of the baseline, not a delta — so subsequent scans see no change, no compile retry fires, and the modal sits open until the user kills the editor.

Fix: the dispatcher's `OnAngelscriptReloadHadErrors` does only parse + classify + queue, then subscribes to `FSlateApplication::Get().GetOnModalLoopTickEvent()`. The modal-tick handler waits ~2 frames (cheap settling margin for the 1ms-sleep-loop hot-reload thread) and only THEN applies strategies. By the time we mutate a file, the thread is running and its next scan will detect our write. Hazelight's own modal-tick handler at `AngelscriptManager.cpp:1066` calls `CheckForHotReload(FullReload)` every frame, so the new change is picked up on the very next iteration and triggers `PerformHotReload`.

The next person to look for "how do I run code while the AS startup modal is up" should not have to re-derive this — that's why this paragraph exists.

### Cycle cap

Hard cap at `FCkAsRecoveryDispatcher::MaxCycles = 3` per editor session (CTO Rev 10 pushback #2). The session-static counter increments on each modal-tick apply that ran at least one strategy successfully. On exceeding the cap, the dispatcher logs a terminal banner and stops attempting recovery — no further retry loop, no death spiral. `Reset_CyclesRun` is called once from `StartupModule` when the hook is armed.

**The cap is bootstrap-only.** Once `OnFEngineLoopInitComplete` flips `sBootstrapComplete = true` (and resets `sCyclesRun`), mid-session synthesis runs without the cycle cap — the interactive editor can mediate a hung loop, where bootstrap can't. See *Per-signature convergence cap* below for the mid-session backstop.

### Per-signature convergence cap

Hard cap at `FCkAsRecoveryDispatcher::MaxPerSignatureRepeats = 3` for each distinct `<NS>::<func>(<args>)` (or `DynamicHandle::<TypeName>`) the dispatcher synthesizes within a session — `<args>` is the normalized form (`Normalize_ArgsList`), so literal-vs-lvalue variants of the same call share one tracker. Catches "dueling-overloads" loops where the upstream cause is something self-heal can't repair — caller arg order doesn't match the entity script's declared `ExposeOnSpawn` field order, or a typed handle slot has drifted vs base. (The literals-vs-lvalues shape that used to land here is now fixed at the source: the synthesizer normalizes argument categories away, so those variants collapse to one value-typed stub instead of two ambiguous overloads.) Symptom before this cap: 724+ `Self-heal recovered: <NS>::<func>(...)` lines per session, every cleanup-boundary wipe undone on the next compile, editor never settles. The bug class is pinned by the 2026-05-21 `UBb_StoreDriver_EntityScript::Params` incident; commit `01a39b58f` on BB `dev-57` is the canonical repro fixture.

Mechanics, mirroring `MaxCycles`:

- Per-key state in the anonymous namespace alongside `sCyclesRun`: `TMap<FString, FConvergenceTracker> sPerSignatureRecoveryCount` (tracks count + observed callsites) plus `TSet<FString> sBlacklistedSignatures` (tripped keys).
- Gate sits between the drain handlers (`OnModalLoopTick` / `OnTicker_DrainActions`) and `Apply_Strategy`. `Try_ReserveSynthesis(InAction)` builds the key, increments the counter, records the callsite, and returns `true` while `count < MaxPerSignatureRepeats`. On the Nth attempt it blacklists the key, fires `Log_TerminalBanner_ConvergenceFailed(Key, Callsites)`, surfaces a `Show_TerminalToast` (coalesced with the existing fallback path), and returns `false`. Subsequent attempts for that key short-circuit with a single `Skipping convergence-blacklisted signature` log line and no UI repaint.
- Banner shape (one line, wraps in the log channel):
  ```
  [SELF-HEAL] Convergence failed for <NS>::<func>(<args>) after 3 synthesis attempts.
  Root cause is upstream of self-heal — likely a caller/entity-script signature
  mismatch (arg order, mutability, or type drift).
  Callers seen:
    - <file>:<line>:<col>
    - ...
  Self-heal will NOT retry this signature for the rest of this session.
  Fix the call site or the entity script, then restart the editor.
  ```
- Only file-mutating strategies (`SynthesizeStub_EntitySpawnParams`, `KickGenerator_AssetRegistry`, `KickGenerator_DynamicHandle`) are tracked. `Author_FixupRequired_AdjacentStringLiteral` doesn't write files and can't loop. `Unrecognized` already terminates earlier.
- Reset semantics match `sCyclesRun`: both maps clear at `Reset_CyclesRun()` (cold-launch) and at `Mark_BootstrapComplete()` (bootstrap → mid-session transition). A signature that *was* on the boot-mode blacklist gets a fresh chance to converge mid-session — appropriate because brand-new authoring inside a running editor is a different problem class from a stale-canonical cold-start.
- "Restart the editor to retry." No auto-rearm. If the user fixes the source, the synthesis requests stop arriving and the blacklist is harmless; if they don't, the blacklist suppresses the loop and the toast coalescer prevents banner spam (`sLastBanner.RepeatCount` collapses identical bursts).

### Cleanup boundaries vs in-memory state

The per-signature tracker and blacklist live in the anonymous namespace alongside `sCyclesRun` — pure session-static memory. They **survive every cleanup boundary**:

- `StartupModule`'s `Delete_AllStubRecoveryFiles()` sweep (cold-launch only)
- `OnPostCompile` synchronous ESP+DH stub deletion
- `Maybe_RegenAssetRegistry_OnPostCompile`'s FTSTicker post-regen AR stub deletion

A signature that flips to blacklisted stays blacklisted regardless of how many stub files self-heal cleans up between attempts. This is exactly the case the cap is designed for — the dueling-overloads symptom hides itself between cleanup boundaries.

**One piece of in-memory state IS cleared on successful compile:** the pending-action queue. `OnPostCompile` calls `FCkAsRecoveryDispatcher::Clear_PendingRecoveryState()` first thing — it drops queued-but-undrained `sPendingActions` and disarms the modal-tick / FTSTicker drains. Without this, a recovery cycle deferred from an earlier failed reload could fire *after* a subsequent successful compile and re-synthesize stubs from stale error records, re-corrupting a healthy state (observed 2026-06-10: a stale cycle re-wrote a `<null handle>` stub minutes after the wedge had been hand-fixed and compiled clean). The convergence tracker and blacklist deliberately survive this clear — they exist precisely to span cleanup boundaries.

### Opt-out surfaces

Two paths (either short-circuits hook registration in `StartupModule`):

- `-NoCkAsRegen` CLI flag — per-launch escape, for debugging a real authoring bug whose error happens to match one of our recovery patterns and you don't want the dispatcher intervening.
- `UCk_AngelscriptGenerator_ProjectSettings_UE::_EnableAsBootstrapSelfHeal` (Editor Settings → "AngelScript Generator", or `CkFoundation.ini`) — project-wide, persistent across launches.

---

## Error-format stability — the load-bearing soft contract

Hazelight's AS compile-error output is **not a stable public API**. The parser's regex patterns are derived from empirical capture of probe formats (2026-05-12 for the original three drift-recovery formats; 2026-05-17 for the adjacent-string-literal pattern). Snapshot tests under `Tests/Test_AsErrorParser.cpp` pin every known-good format verbatim — they are the engine-upgrade canary. If those tests fail after a Hazelight upgrade, the format has changed and the parser must be updated before the dispatcher is re-enabled.

When upgrading the AngelscriptCode engine plugin: run `CkAngelscriptGenerator.UnitTests.AsErrorParser.*` from Session Frontend BEFORE letting the new engine plugin ship. If any test goes red, fix the parser regex against the new format, re-run, then proceed.

---

## AutoTestActors drift is out of scope

`<Plugin>_AutoTestActors.as` files resolve their test entity-script class via `FSoftClassPath` *string* lookup, not by a static AS identifier. So drift between the wrapper file and the actual test classes manifests as a runtime "test not found in Session Frontend," NOT as an AS compile error. The self-heal dispatcher only acts on compile-deadlock errors; AutoTestActors drift is a separate, runtime-only failure mode and is intentionally not covered.

---

## EntitySpawnParams.as is NOT resilient to deleted entity-script classes

`<Plugin>_AutoTestActors.as` was deliberately designed to tolerate missing entity-script classes — its emitted wrappers use `FSoftClassPath::TryLoadClass()`, so the file always compiles even when its referenced classes are gone. `<Plugin>_EntitySpawnParams.as` was **not** built with the same resilience. It emits direct AS identifiers — a `namespace U<EntityScript> { ... }` block plus an `F<EntityScript>_SpawnParams` struct — and these blocks reference the entity-script class name as a real AS identifier.

When the underlying entity-script `.as` source is deleted but the stale `EntitySpawnParams.as` block survives on disk (e.g., a recovery procedure that reverts `AutoTestActors.as` but forgets `EntitySpawnParams.as`), the next editor launch creates a phantom AS namespace pointing at a class that no longer exists. AS tolerates the dangling reference at compile time, but if someone later re-adds an `.as` file defining a class with the same name, AS's registry hits a conflict path: it treats the name as already-known (from the phantom namespace) and silently fails to register the new class as a live `UClass`. `UObjectIterator<UCk_AutoTest_Base>` then doesn't see the class, the wrapper generator emits no wrapper for it, and downstream consumers (CkTestsEditor's `UCkAutoTestMapPopulator`) silently fail to discover the test.

Empirically reproduced 2026-05-12 during AutoTest map OFPA work: a probe test added, then deleted during recovery from an unrelated bug, then re-added with the same class name. The re-add never appeared in Session Frontend until the class name was changed.

**Workaround for recoveries**: when reverting populator/test-related state, revert *every* file under `<Project|Plugin>/Script/Generated/*.as`, not just `AutoTestActors.as`. Both `EntitySpawnParams.as` and `AutoTestActors.as` are output of generators tied to the same entity-script class set; treat them as a single atomic state.

**Proper fixes** (out of scope for the immediate AutoTest work, deferred to a focused CkAngelscriptGenerator pass):

- Make `EntitySpawnParams.as` emission self-pruning: detect stale blocks (e.g., via a marker convention) and rewrite the file on every generator run instead of relying on diff-skip. This is the lower-risk approach because it doesn't change the generated code shape downstream consumers rely on.
- OR introduce an indirection so the namespace block doesn't reference the entity-script class as a direct AS identifier (analogous to `AutoTestActors`'s `FSoftClassPath` trick, though the equivalent at AS code-level is non-obvious — the namespace name itself is the identifier).

The first approach is the recommended starting point. The downstream populator behavior (silent test-discovery failure when the entity-script registry is corrupted) is *also* worth hardening, but that's a CkTests concern — fixing the root cause here is the higher-leverage move.

---

## Engine-fork relaxation — narrow precedent, opened then closed

In Rev 9 (2026-05-11) the CTO pre-approved a single surgical engine-fork change to `Plugins/Angelscript`: a multicast `FAngelscriptParsedModulesDelegate` exposing the parsed-class descriptors during failed compile, so a descriptor-driven generator could substitute for `UObjectIterator` (which doesn't expose AS-defined classes during a failed compile). The fork was implemented and built clean.

On first end-to-end test that same day, the Rev 9 descriptor-driven regen broke the editor more thoroughly than the original corruption: CDO field defaults aren't accessible via descriptors, Blueprint-derived UClasses don't appear in `FAngelscriptModuleDesc::Classes`, and cross-file type resolution failed in the hot-reload context. The recovery loop death-spiraled (595 → 595 → 0 → 17 classes across cycles).

**Rev 10 replaced the descriptor-driven approach with the error-driven dispatcher described above, which doesn't need any engine signal beyond `GetReloadHadErrors` (pre-existing public delegate).** The engine fork is no longer load-bearing and was reverted locally the same day (CTO Rev 10 pushback #4). The reverted fork is preserved on a local engine-repo stash (`stash@{0}` on `D:/Repos/UnrealEngine-Angelscript`, `main-ck` branch) and the upstream PR to Hazelight remains open as an independent contribution.

**Policy precedent:** engine-fork relaxations require the *specific* justification to hold. When the load-bearing role evaporates, the fork is reverted — adjacent value (e.g. CI utility, drift commandlet feed) does not retroactively justify keeping the precedent open.

---

## Determinism + drift safety

Two regenerations of the same input must produce byte-identical output. This is load-bearing for:

- The real generators' "diff-skip when content is current" optimization (otherwise the post-compile hook would write-itself-into-an-infinite-recompile loop).
- The drift-detection commandlet's `git diff --exit-code` verdict.

Determinism guarantees:

1. **No timestamps in output.** No `// generated at <time>` lines.
2. **Stable sort orders** — buckets sorted by plugin name, classes within a bucket sorted by name, properties within a class sorted by declaration order.
3. **Platform-native line endings on write** (CRLF on Windows) with LF-normalized compare beforehand, so externally-converted files don't trigger unnecessary rewrites.

### Mtime stability is startup-time load-bearing (2026-06-11)

The Hazelight hot-reload checker baselines `.as` file **mtimes** from its initial script scan. Any `Script/Generated/*.as` whose mtime changes after that baseline — even with byte-identical content — triggers a hot reload AFTER engine init: a recompile of the touched modules plus a multi-second soft-reload sweep of all script classes, a full literal-asset re-init, and a debug-database rebroadcast to attached VS Code clients, all on the game thread exactly while the editor window is visible but not yet interactive. Two generator bugs used to reproduce this on EVERY editor launch (~8s of frozen editor):

- `FCkAngelscriptWrapperGenerator::GenerateAllWrappers` (runs at every boot via `FAngelscriptBinds` Early bind) **deleted every `*.as` in CkFoundation's `Script/Generated/`** before regenerating its own wrappers — including `CkFoundation_EntitySpawnParams.as`, owned by the ES Params generator which only runs post-compile. The post-compile regen then re-created the file → "new module" structural reload. Fixed: cleanup is now manifest-based — the previous run's `_index.as` lists the wrapper files the generator owns; only `manifest − generated-this-run` is deleted. **Never add a blanket delete of shared Generated directories.**
- The same generator rewrote all ~250 wrapper files unconditionally each boot (byte-identical, fresh mtimes) → code-only reload of every wrapper module post-init. Fixed: `SaveWrapperFile_IfChanged` (LF-normalized compare) on every write site.

The ES Params generator logs a **rewrite reason** (missing file, or first differing line old/new) whenever a bucket rewrites — if a post-init structural reload of `Generated.<Plugin>_EntitySpawnParams` ever reappears, that log line says why.

The synthesizer follows the same rules — its stub blocks have no timestamps and use `LINE_TERMINATOR` for platform-native EOL.

### Sibling-file stub model + force-quit safety

The self-heal dispatcher writes synthesized stubs to SIBLING files in the same directory as the canonical, prefixed `_StubRecovery_`:

- `Script/Generated/_StubRecovery_<Plugin>_EntitySpawnParams.as`
- `Script/Generated/_StubRecovery_<MatchedAssetsFile>.as`
- `Script/Generated/_StubRecovery_DynamicHandleTypes.json`

The canonical files are never touched. Both the BB root `.gitignore` and CkFoundation's `.gitignore` cover `Script/Generated/_StubRecovery_*` patterns, so stub files physically cannot be staged. Cleanup is **per-generator** — each generator owns deletion of its own sibling pattern, fired AFTER that generator's canonical write completes — and runs at three sites:

* **`StartupModule`** — broad `Delete_AllStubRecoveryFiles()` sweep before AS delegates are wired. Catches force-quit survivors that would otherwise collide with regenerated canonical output.
* **`OnPostCompile` (synchronous part)** — `Delete_StubRecoveryFiles_ForPatterns({ESP, DH})` after `Run_AllGenerators` and `Maybe_RegenDynamicHandleJson_OnPostCompile` (both synchronous). AR pattern is intentionally excluded — its regen is async.
* **AssetRegistry FTSTicker post-regen** — `Delete_StubRecoveryFiles_ForPatterns({AR})` inside the ticker callback in `Maybe_RegenAssetRegistry_OnPostCompile` (and its `OnPostInit` twin), right after `Subsystem->GenerateAllAssetRegistries()` returns. Moving this earlier (the outer PostCompile lambda or any other synchronous site) is the bug pinned by `_probe_assetregistry_loop`: stub deletion races regen, hot-reload recompiles against a still-stale canonical, dispatcher re-synthesizes, loop.

**Per-accessor dedup gate:** Both synthesizers (`Inject_EntityScriptParamsStub` and `Inject_AssetRegistryStub`) scan existing sibling content for the unique `// End synthesized stub for <NS>::<FUNC>(<args>)` marker line before appending. If present, the append is a no-op (returns success). Without this, a dispatcher re-fire for the same accessor within one session produces duplicate `Params(...)` / accessor declarations in the merged namespace, which AS rejects with `A function with the same name and parameters already exists`. The ESP marker embeds the **canonical (const-stripped) args** (`Normalize_ArgsList`), NOT the raw error args — two call sites passing differently-qualified arguments to the same overload (`"const FTransform"` vs `"FTransform"`) report distinct raw strings but emit the identical declaration, and raw-keyed markers let both blocks append (the 2026-06 fresh-clone bulk-synthesis failure). A declaration-level backstop additionally no-ops when the exact emitted overload declaration already exists in the sibling (catches alias classes the canonicalization doesn't enumerate). Regression tests: `...StubSynthesizer.Inject_DedupOnSameAccessor`, `Inject_DedupOnConstAliasedArgs`, `Inject_BulkSynthesis_NoDuplicateOverloads`.

The marker (and the emitted parameter list, and the dispatcher's convergence key) uses a **normalized** args list (`FCkAsStubSynthesizer::Normalize_ArgsList` — strips `const ` and trailing `&` per token). The AS error reports the *call site's argument category*, not the declared parameter: the same `Params()` called with a literal vs an lvalue produces `const int` vs `int&` in two otherwise-identical error records. Keying/emitting on the raw list synthesized one stub per variant; the two stubs were mutually ambiguous at every call site (`Multiple matching signatures` — an error kind the parser doesn't recognize), wedging the boot past the cycle cap. Pinned by the 2026-06-10 `UBb_CheckoutSettle_InteractableHost_EntityScript::Params` incident; regression test: `Inject_DedupOnArgCategoryVariants`. Stubs emit value-typed params throughout — matching the real generator's shape, and a value param accepts every argument category.

**Uninferable arg types (bare `nullptr`):** a call site passing a bare `nullptr` for a UObject-typed arg reports the compiler placeholder `<null handle>` — there is no inferable type. Emitting the placeholder verbatim makes the stub file itself unparseable (`Expected data type / Instead found '<'`), which is a *worse* state than the original mismatch: the parse error kills the whole module, and self-heal cannot recover because the corrupt file is its own output (it re-parses, finds 0 actionable roots, and gives up at the cycle cap). `Normalize_TypeToken` maps any bracketed placeholder to a **UObject fallback** — stub bodies discard their args, and a `nullptr` literal binds to a UObject param. Additionally, `Inject_EntityScriptParamsStub` enforces a **same-arity ambiguity gate**: a fallback-bearing stub is never appended alongside an existing typed stub of the same `NS::FUNC` + arity (nor vice versa) — `nullptr` binds to both and a typed handle implicitly converts to UObject, so the pair is mutually ambiguous at every call site; whichever stub landed first satisfies them all. The gate scans the same per-overload end-markers full-shape blocks emit, so a nullptr variant of a full-shape typed overload is suppressed too. Distinct fully-typed same-arity overloads (int vs float) still dedup independently. Pinned by the 2026-06-10 `UBb_StoreDriver_EntityScript::Params` incident (29-arg migration, several call sites passing a trailing `nullptr` config); regression tests: `Build_NullHandleArg_EmitsUObjectFallback`, `Inject_NullVariant_SkippedWhenTypedSameArityExists`, `Inject_TypedVariant_SkippedWhenFallbackSameArityExists`.

**EntitySpawnParams stub:** AS merges multiple `namespace X { ... }` blocks across files, so the sibling's namespace block contributes the missing accessor to the merged scope and compile succeeds. The sibling carries a recovery-header banner (`FCkAsStubSynthesizer::Get_StubFileHeader()`) plus per-stub marker comments (`Get_MarkerComment()`). Multiple drifts for distinct accessors in the same session accumulate into the same sibling file; same-accessor re-injects are dedup-gated. A **source-derived full-shape** block additionally ends with a class-level marker (`Get_FullShapeMarkerLine`, re-fire dedup) plus per-overload end-markers carrying the same prefix the error-text gate scans — so a later error-text inject for either canonical overload no-ops against the full-shape block (cross-path dedup; pinned by `Inject_SourceDerived_EndToEnd`). A caller whose signature genuinely diverges from the full shape (arg-order drift, partial args) still error-texts an EXTRA overload alongside it — self-correcting by design.

**AssetRegistry stub:** Same shape — sibling `_StubRecovery_<MatchedAssetsFile>.as` containing the same `namespace assets { ... }` block. AS namespace-merge handles the rest. The choice of which `*Assets.as` file to land alongside is made by `Pick_BestSite_ByAssetPath` — prefix-match of the asset's package path against each candidate's `DiscoveryRoot`, longest-match wins. Falls back to first candidate when no root owns the asset on disk.

**DynamicHandle stub:** JSON has no comment syntax, so the sibling uses a top-level `"_WARNING"` field instead. `FCkDynamic_HandleTypeRegistry::LoadFromJsonRegistry` reads the canonical, then reads the sibling stub (if present) and merges its `HandleTypes` entries — deduped by `TypeName`, sibling entries win. The synthesized stub has correct `TypeName` + `ShortName` and a placeholder `Description`; `RequiredFragments` is empty, which produces a *permissive* validator. `OnPostEngineInit::Maybe_RegenDynamicHandleJson_OnPostInit` regenerates the canonical JSON from the now-discoverable data asset, then `DiscoverAndRegisterAllDefinitions` upgrades the in-memory validator to strict via `FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle`. Window of permissive-validator state: typically <1 second between editor reaching main screen and OnPostEngineInit firing.

**Force-quit between recovery and PostCompile:** Sibling files survive to next launch. AS compile succeeds again (the sibling is still on disk, merge-via-namespace still works, the JSON merge still works); `StartupModule`'s broad sweep then deletes any leftover stubs. The `_StubRecovery_*` files double as the on-disk signal for the deferred-regen paths (`Maybe_RegenAssetRegistry_OnPostInit`, `Maybe_RegenDynamicHandleJson_OnPostInit`) to know they should fire a proper regen.

---

## Anti-patterns

1. **Don't commit `Script/Generated/*.as` files manually.** Either let the generators land them on next editor launch, or trigger a deliberate regen from `UCkDynamicHandleSubsystem::GenerateHandleTypeRegistry()` / `UCk_Utils_AssetRegistry_UE::Generate_All_Asset_Registries()` inside the editor.
2. **Don't apply file mutations synchronously inside `OnReloadHadErrors`.** Use the modal-tick deferral — see the timing-constraint paragraph above.
3. **Don't introduce non-determinism into generator output** (timestamps, GUIDs, machine-specific paths). The drift commandlet + atomic-write diff-skip both rely on determinism.
4. **Don't mutate canonical generated files from the self-heal dispatcher.** Recovery stubs go to sibling `_StubRecovery_*` files only — anything else breaks the "canonical files are byte-clean from HEAD" invariant the new model depends on.

---

## See also

- `/Source/CLAUDE.md` section 15 — `asset ... of UCkAssetRegistryConfig` AngelScript syntax.
- `CkCore/Reflection/README.md` — property/class introspection used by the generators.
- `Plugins/CkFoundation/Script/CLAUDE.md` — adding a new dynamic handle (one-step self-heal landing; manual two-phase is historical edge-case recovery).
- BB `Script/CLAUDE.md` "Codegen lag" section — what the dispatcher automates.
