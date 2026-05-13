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

The dispatcher lives under `SelfHeal/`. Three pieces:

- `CkAngelscriptGenerator_AsErrorParser.{h,cpp}` — parses Hazelight's AS compile-error output (sourced via `FAngelscriptManager::Get().FormatDiagnostics()`) into typed root-cause records. Recognizes two error classes: `No matching signatures to '<NS>::<func>(<args>)'` and `Identifier '<X>' is not a data type`. Cascade noise (`Unknown`, `_Iterator`, etc.) is dropped.
- `CkAngelscriptGenerator_StubSynthesizer.{h,cpp}` — given a parsed `NoMatchingSignatures` error on an entity-script `Params(...)` overload, builds a minimum-viable `namespace U<X> { F<X>_SpawnParams Params(<args>) { ... } }` (plus a stub struct if absent) and atomically appends it to the right `<Plugin>_EntitySpawnParams.as`.
- `CkAngelscriptGenerator_Dispatcher.{h,cpp}` — classifies parsed roots into one of three recovery strategies and applies them.

### Recovery strategies

| Error pattern | Strategy | Wired? |
|---|---|---|
| `<EntityScriptClass>::Params(<args>)` not found | **Synthesize stub** into the affected `<Plugin>_EntitySpawnParams.as` — caller sees AS merge the existing namespace block with our stub block, finds the missing overload, compile succeeds. Real generator overwrites the stub on `OnPostCompile` regen. | ✓ |
| `Identifier 'FCk_Handle_<X>' is not a data type` | **Synthesize JSON entry** in `DynamicHandleTypes.json`, register a (temporarily permissive) validator in-memory, touch caller mtime to nudge a recompile. `OnPostEngineInit` then runs `GenerateHandleTypeRegistry` + `DiscoverAndRegisterAllDefinitions` — the now-discoverable data asset's strict `RequiredFragments` is written into the JSON AND the in-memory validator is replaced via `FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle`. No restart needed. | ✓ |
| `assets[::*]::<func>(<args>)` not found | **Manual intervention required** — the accessor's return type encodes the asset's UClass which the dispatcher can't reliably infer at modal-tick time. See `Apply_Strategy::KickGenerator_AssetRegistry` block for the rationale + remediation steps. | Classified, log-only |
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

### Opt-out surfaces

Two paths (either short-circuits hook registration in `StartupModule`):

- `-NoCkAsRegen` CLI flag — per-launch escape, for debugging a real authoring bug whose error happens to match one of our recovery patterns and you don't want the dispatcher intervening.
- `UCk_AngelscriptGenerator_ProjectSettings_UE::_EnableAsBootstrapSelfHeal` (Editor Settings → "AngelScript Generator", or `CkFoundation.ini`) — project-wide, persistent across launches.

---

## Error-format stability — the load-bearing soft contract

Hazelight's AS compile-error output is **not a stable public API**. The parser's two regex patterns are derived from empirical capture of the three probe formats (2026-05-12). Snapshot tests under `Tests/Test_AsErrorParser.cpp` pin the three known-good formats verbatim — they are the engine-upgrade canary. If those tests fail after a Hazelight upgrade, the format has changed and the parser must be updated before the dispatcher is re-enabled.

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

The synthesizer follows the same rules — its stub blocks have no timestamps and use `LINE_TERMINATOR` for platform-native EOL.

### Stub-mutation safety on force-quit (CTO Rev 10 pushback #6)

If the user kills the editor while a synthesized stub is on disk but the real generator hasn't yet overwritten it, the stub persists to the next launch. The recovery is automatic in both stub flavors:

**EntitySpawnParams stub (in `<Plugin>_EntitySpawnParams.as`):**
- **Marker comments** wrap every synthesized block (`// CkAngelscriptGenerator: synthesized stub for emergency recovery; ...`). Forensic readers can immediately tell injected blocks apart from real generator output. Public accessor: `FCkAsStubSynthesizer::Get_MarkerComment()`.
- **Stubs are non-conflicting by construction** — the synthesizer only emits an overload that the existing namespace lacks. AS merges multiple `namespace X { ... }` blocks into a single namespace; the caller resolves to our stub and compile succeeds on next launch. After that success, `OnPostCompile` fires `Run_AllGenerators` which rewrites the file fresh and the stub naturally vanishes.

**AssetRegistry stub (in `<Plugin>_*Assets.as`):**
- Synthesized by `FCkAsAssetRegistryStubSynthesizer` with the same marker-comment convention. Public accessor: `FCkAsAssetRegistryStubSynthesizer::Get_MarkerComment()`.
- Two cleanup paths:
  1. **Startup path** (`Maybe_RegenAssetRegistry_OnPostInit` → `OnFEngineLoopInitComplete` + shader-idle poll → `GenerateAllAssetRegistries`). Handles cold-start, including force-quit-from-prior-session leftover stubs. Gated by either the session flag or the on-disk marker scan.
  2. **PostCompile path** (`Maybe_RegenAssetRegistry_OnPostCompile`). Handles mid-session: a stub synthesized AFTER `OnFEngineLoopInitComplete` has already fired would otherwise persist until next launch. The PostCompile hook detects the marker on disk and runs `GenerateAllAssetRegistries` synchronously. Gated by `sg_EngineLoopInitComplete` so it can't race the startup path during cold-start contention. Cost in the steady-state (no marker) is one cheap fs walk per AS recompile.

**DynamicHandle stub (in `DynamicHandleTypes.json`):**
- The synthesized JSON entry has correct `TypeName` + `ShortName` and a placeholder `Description` (marker text). Next launch reads the JSON, registers the type with a *permissive* validator (empty `RequiredFragments`).
- AS compile succeeds — the type is at least registered — but `As_<X>()` casts succeed unchecked until the deferred regen fires.
- `OnPostEngineInit::Maybe_RegenDynamicHandleJson_OnPostInit` then regenerates the JSON from the data asset (proper `RequiredFragments`) AND calls `DiscoverAndRegisterAllDefinitions` to update the in-memory validator to strict via `FCkAngelScript_HandleRegistry::UpdateExistingDynamicHandle`. From that point the session is fully safe.
- Window of permissive-validator state: typically <1 second between editor reaching main screen and OnPostEngineInit firing. A plain `Log` line announces both states.

The intended source-hash header that would have made the file "self-detect dirty" at next launch was reverted with Rev 9. The above mitigations cover the same failure mode without it.

---

## Anti-patterns

1. **Don't commit `Script/Generated/*.as` files manually.** Either let the generators land them on next editor launch, or trigger a deliberate regen from `UCkDynamicHandleSubsystem::GenerateHandleTypeRegistry()` / `UCk_Utils_AssetRegistry_UE::Generate_All_Asset_Registries()` inside the editor.
2. **Don't apply file mutations synchronously inside `OnReloadHadErrors`.** Use the modal-tick deferral — see the timing-constraint paragraph above.
3. **Don't introduce non-determinism into generator output** (timestamps, GUIDs, machine-specific paths). The drift commandlet + atomic-write diff-skip both rely on determinism.

---

## See also

- `/Source/CLAUDE.md` section 15 — `asset ... of UCkAssetRegistryConfig` AngelScript syntax.
- `CkCore/Reflection/README.md` — property/class introspection used by the generators.
- `Plugins/CkFoundation/Script/CLAUDE.md` — adding a new dynamic handle (two-phase workflow).
- BB `Script/CLAUDE.md` "Codegen lag" section — what the dispatcher automates.
