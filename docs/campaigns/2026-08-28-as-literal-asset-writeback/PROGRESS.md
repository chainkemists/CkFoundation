# AngelScript Literal-Asset Write-Back — Progress

**Status:** implemented; all headless gates green. Two checks remain that only a human at the editor
can make (§5).
**Design:** [PLAN.md](PLAN.md) · **Review:** [CTO review](../../reviews/2026-08-28-as-literal-asset-writeback-CTO-review.md)

---

## 1. What shipped

`Plugins/CkFoundation/Source/CkAngelscriptGenerator/`

| File | Role |
|---|---|
| `WriteBack/CkAngelscriptGenerator_AssetBlockPatcher.{h,cpp}` | Pure text. Locates `asset <Name> of <Type> { }` over a comment/string-blanked copy, brace-matches the body, and splices per statement (replace RHS / insert line / delete statement). Preserves indentation, trailing comments, CRLF-vs-LF and a UTF-8 BOM. Atomic temp+move write. |
| `WriteBack/CkAngelscriptGenerator_AccessorResolver.{h,cpp}` | Pure text. Parses the **generated** `.as` accessor files into `{object path, namespace, function, has `_Class`, is `#if Editor`}` and emits the four accessor spellings. |
| `WriteBack/CkAngelscriptGenerator_AssetValueDiff.{h,cpp}` | Builds the scratch baseline (`NewObject` + re-run `__Init_<Name>`), walks `(live, baseline)` as a pair, dispatches object-ish leaves to the resolver, and produces the patch set. |
| `WriteBack/CkAngelscriptGenerator_AssetWriteBack.{h,cpp}` | The toolbar button: dynamic section, cached enable state, confirmation dialog, confirm-time freshness guard, orchestration. All UObject/editor coupling lives here. |
| `Tests/Test_AssetBlockPatcher.cpp`, `Test_AccessorResolver.cpp`, `Test_AssetValueDiff{,_Fixtures}.{cpp,h}`, `Test_AssetWriteBack_Integration.cpp` | 43 new automation tests. |

One existing file changed: `CkAngelscriptGenerator_Module.cpp` registers and unregisters the toolbar
extension.

**Deviation from the plan's file layout:** the diff engine got its own translation unit
(`_AssetValueDiff`) rather than living in `_AssetWriteBack.cpp`. The plan requires the patcher and
resolver to be testable without an editor; the diff engine needs UObject reflection but no editor UI,
and folding it into the UI file would have made the pair-recursion tests impossible to write.

---

## 2. Corrections to the plan

Both were found by compiling and running, not by reading.

1. **Fact 6 is wrong in one detail.** `FAssetEditorToolkit::DefaultAssetEditorToolBarName` is
   **private** (`AssetEditorToolkit.h:522-523`), so a plugin cannot name the symbol. The value and the
   design are unaffected — the literal `"AssetEditor.DefaultToolBar"` is spelled out with a citation
   to its definition (`AssetEditorToolkit.cpp:52`) and to the parent registration that makes the
   design work (`:1450-1453`).
2. **The pair recursion stops at a pod sub-struct.** A nested struct that carries no object field
   emits its whole constructor at that path rather than descending to the scalar leaf — matching what
   `Get_StructFieldOverrides` documents. The plan's §5.4 wording implies a leaf-by-leaf walk
   throughout.

---

## 3. Finding: five literal assets hold a null reference at runtime

`ScratchBaselineReproducesEveryLiveAsset` re-runs `__Init_` against every native-class literal asset
in the project and compares. Five diverge — the live object holds **null** where the source assigns a
mesh:

| Asset | Source assigns |
|---|---|
| `Asset_PlaceableTest_Cube` | `/Engine/BasicShapes/Cube.Cube` |
| `Asset_PlaceableTest_Sphere` | `/Engine/BasicShapes/Sphere.Sphere` |
| `Asset_RegularCube` | `/Engine/EngineMeshes/Cube.Cube` |
| `Asset_BackgroundCube` | `/Engine/EngineMeshes/BackgroundCube.BackgroundCube` |
| `Asset_StationMarker` | `/Engine/EngineMeshes/Cube.Cube` |

They assign through `utils_i_o::LoadAssetByName`, which resolves via the AssetRegistry before it is
scanned. Unlike `assets::load::X()`, that path does not trip the premature-blocking-load flag the
deferred-asset heal attributes from, so these are never re-run and stay null for the session.

Load-bearing for write-back: on those assets the button would otherwise read "the user cleared the
mesh" and offer to erase a working line. Two things now stand between that and the user:

1. The diff reports such properties as `ClearedObjectReferences` and the confirmation dialog names
   them explicitly before anything is written.
2. **The underlying defect is fixed** (see §7): `DoLoadAssetsByName` now notes the deferral, so the
   heal re-runs those initializers and the values are no longer null.

`ScratchBaselineReproducesEveryLiveAsset` asserts the count is zero, so a new way of resolving a
reference that the heal does not know about turns the gate red rather than silently reappearing.

---

## 4. Gates

| Gate | Before | After |
|---|---|---|
| Unscoped suite (`--test`, no pattern) | 1274 total, 1257 passed, **17 failed** | 1274 total, 1258 passed, **16 failed** |
| `--test-pattern AngelscriptGenerator` | n/a (module tests are new) | 137 total, **137 passed, 0 failed** |
| Editor build | clean | clean |

**One flake investigated and cleared.** The first unscoped run after the CkCore change (§7.1) showed
16 of the baseline failures plus a new one, `Ck_AutoTest_Crowd_BunchUp_SettlesAtSharedGoal`. That
change does measurably alter boot — the deferred heal goes from 0 CDOs / 2 literal assets to 15 CDOs
/ 7 assets (mode stays `surgical`; no full-heal escalation) — so it was not dismissed. Evidence it is
a flake: the test passes in isolation on the same binary, passes again in a second full run under
identical parallel conditions (3 passes / 1 fail post-change), its assertion is a 0.2 s
hold-oscillation threshold in a crowd sim, five sibling crowd/queue tests are persistently red, and
no script under `CkCrowd`/`CkQueue`/`CkPathNetwork` calls `LoadAssetByName` at all — the newly healed
CDOs are audio, tween, transform, grid and state-machine gym actors.

**The two runs cover disjoint populations.** The unscoped run contains **zero**
`CkAngelscriptGenerator` tests — before this work and after — so its 1274 is the same population on
both sides and the comparison is valid. This module's tests only execute under the scoped pattern,
which is therefore the real gate for this code. Both were run.

No new failures. The 17 inherited failures are unrelated to this module (8 `SceneNodeTween`, 4
`Queue`/`QueueCoordinator`, 2 `PathNetworkFollower`, 1 `Crowd_Grounding`, 1 `IntegrationTest`); the
16-vs-17 difference is `IntegrationTest` passing on the second run — an inherited flake, not a fix
from this work.

---

## 5. Not yet verified — needs a human at the editor

Agents cannot launch the editor or click a toolbar button.

**[EDITOR-VERIFY] 1 — the button appears where it should, and nowhere else.**
1. Open the editor. In the Content Browser, find an AngelScript literal asset with a native parent
   class (e.g. `Asset_RendererData_Demo`, or any `ItemDef_InvGym_*`).
2. Open it. Confirm a **Write Back** button appears in the asset-editor toolbar.
3. Open any ordinary `.uasset` (a DataAsset, a Blueprint, a material). Confirm **no** Write Back
   button appears.
4. Open a literal asset whose declared class is AngelScript-declared (e.g. one of the
   `UCk_GameplayTags` assets). Confirm **no** button appears.

The mechanism is traced end-to-end in engine source: every asset-editor toolbar registers with
`AssetEditor.DefaultToolBar` as its parent (`AssetEditorToolkit.cpp:1450-1453`); assembly walks that
hierarchy and, for a dynamic section, builds a temp menu whose `Context` is the **generated child
menu's** context before invoking the section delegate (`ToolMenus.cpp:900`, `:925`). So the section
sees the asset editor's `UAssetEditorToolkitMenuContext`. What remains genuinely unobserved is only
whether the entry renders where expected — no in-repo or engine code extends this particular parent
directly, so nobody has exercised it here before.

**[EDITOR-VERIFY] 2 — the round trip.**
1. Open `Asset_RendererData_Demo`. Change `_NumCustomDataFloat` from 1 to 3 in the details panel.
2. Press **Write Back**. Confirm the dialog lists exactly one line change and no warnings.
3. Accept. Then `git diff Plugins/CkTests/Script/CkIskmRenderer/CkIskmRenderer_Assets.as` — confirm
   exactly one line changed and every `_Submeshes.Add(...)` line is untouched.
4. Confirm the editor hot-reloads and `_NumCustomDataFloat` still reads 3 afterwards.

Step 4 is the real proof, and the claim most likely to be wrong. The unit tests assert the
*expression text* write-back emits; nothing yet asserts that the AngelScript compiler accepts and
evaluates that text back to the same value. A float literal, an enum spelling, or an `n"..."` name
that AngelScript parses differently would only show up here — the reload re-runs `__Init_` and resets
every non-instanced property from the CDO, so a value that survives the round trip is proof the
emitted text is not merely well-formed but correct.

---

## 6. Known limits (as designed for v1)

- Containers (`TArray`/`TSet`/`TMap`) are not expressible. A container populated by `.Add()` in the
  body matches the baseline and never enters the patch set; one edited in the details panel aborts
  the whole write with a named reason.
- Weak / lazy / interface references abort the write rather than resolving.
- A cross-file literal-asset reference aborts with a hint toward the namespace-wrapper idiom.
- `FText` is written as a culture-invariant `FText::FromString`, losing its localization
  namespace/key. The dialog says so when it applies.
- The button's enabled state is cached off `OnObjectPropertyChanged`. An AngelScript hot reload
  changes property values without firing that delegate, so the enabled state can be stale until the
  toolbar regenerates. The write itself always recomputes the diff, so this is cosmetic.

---

## 7. Follow-up defects fixed after the campaign

Three follow-ups were filed during the campaign and have since been implemented. A fourth was found
while investigating the third.

### 7.1 Literal assets left holding null references (`CkIO_Utils.cpp`)

`UCk_Utils_IO_UE::LoadAssetByName` resolves purely through `IAssetRegistry`, which has not scanned
when literal assets are materialised at AngelScript module load — so it returned nothing and the
caller silently stored null. The deferred-asset heal never repaired it: that sweep re-runs only
initializers recorded by `Note_DeferredAssetLoad_FromActiveContext`, and the only thing calling it
was `ck::EnsureIfNot_PrematureAssetLoad`, emitted exclusively into `assets::load::*` accessors.
`ResolveAllPending` additionally short-circuits unless something queried the blocking-load flag.
`LoadAssetByName` did neither, so five assets stayed null for the whole session.

**Fix:** `DoLoadAssetsByName` — the single funnel behind all four public entry points — now notes the
deferral when a lookup comes back empty before the engine is safe for blocking loads. The querying
`IsEngineSafeForBlockingLoads()` is used deliberately: its side effect is what stops
`ResolveAllPending` short-circuiting.

*Rejected:* switching the `.as` bodies to `assets::load::` — no such accessor exists for any
`/Engine/` path, because no `UCkAssetRegistryConfig` has an `/Engine` discovery root. It would have
meant authoring a config that emits accessors for every asset under `/Engine`.

*Cost:* a C++ caller whose early lookup fails has no AngelScript context, which sets
`GAttributionUncertain` and escalates that boot's heal from surgical to full. That is the sweep's own
documented never-under-heal fallback, and it is idempotent.

### 7.2 Asset-reference tracking was blind to every plugin (`CkAssetRegistrySubsystem.cpp`)

Found while investigating 7.3, and considerably more serious than it. `Get_ScriptDirectory()`
returned only `ProjectDir()/Script`, so the usage scan never reached `Plugins/*/Script/` — where
essentially all AngelScript in this layout lives. Log evidence from before the fix:

```
[AssetRegistry] Seeded 870 asset references from 1 generated files (1 namespaces)
[AssetRegistry] Script usage scan complete: 0 asset functions referenced from 3 script files
```

870 accessors known, 3 files scanned, 0 references found — against 1,916 `assets::` references under
`Plugins/`. `Get_ScriptReferencersOfAsset` returned empty for every asset while
`Get_HasAnyProvider()` still answered true: "nobody was there to ask" reported as "asked and got
nothing", which is the exact confusion `FCk_AssetReferenceProviderRegistry` exists to prevent. Any
auditor consulting it would have offered script-critical assets for deletion.

**Fix:** the scan now enumerates through `FCkAsSourceScanner::Get_DefaultScanRoots()` +
`Enumerate_AsSourceFiles()` — project Script/ plus every enabled plugin's, already used and tested by
the self-heal path, and already excluding `Generated/` and `_StubRecovery_*`. The now-unused
`Get_ScriptDirectory()` is removed.

*Cost:* the post-compile scan reads ~2,000 files instead of 3. Per file it is two linear `Find`
passes per namespace with hash-set membership — no quadratic term.

### 7.3 `AssetPathToFunctionName` map-wipe (`CkAssetRegistrySubsystem.cpp`)

The map is process-wide by design (both readers are config-agnostic, and `SeedMapsFromGeneratedFiles`
accumulates over every config) but was `Reset()` per config, while `GloballyGeneratedAssets`
suppresses re-emitting an earlier config's assets. With two or more configs the map ended up holding
only the last one's entries.

**Fix:** the per-config reset is gone; `SeedMapsFromGeneratedFiles` clears the map itself and is now
called **unconditionally** from `ScanScriptFilesForUsage`, rebuilding it from the generated files —
the only statement of what actually compiles.

*Rejected:* moving the reset to the pass boundary beside `GloballyGeneratedAssets.Reset()`. It was
tried and reverted: `GenerateAssetRegistryForConfig` also resets there, so a single-config regen
would have wiped every other config's entries — the same bug, narrower. It also cannot repair a pass
that aborts between configs, because the old is-it-empty guard sees a partially-populated map as
non-empty and declines forever.

**Latent, not live, in this project:** there is exactly one config (`CkTestsAssets`), so the map was
being wiped and immediately refilled. It is live in any project with two or more.

### 7.4 Unbounded `FString::Find` loops (`Test_StubSynthesizer.cpp`)

`FString::Find` clamps `StartPosition` to `Len()-1`, so a needle matching at end-of-string is
returned forever. Seven copies of the pattern existed; all now route through one bounded
`Count_Occurrences` helper. This hung two automation-test lanes during the campaign until the
toolbox watchdog killed them.

### Tests added

| Test | Guards |
|---|---|
| `AssetReferenceTracking.ScanRootsCoverPlugins` | 7.2 — enumeration reaches plugin scripts and still excludes `Generated/` |
| `AssetReferenceTracking.PluginReferencedAssetIsReported` | 7.2 — an asset referenced only from a plugin `.as` is reported as referenced, and an unreferenced path still reports none |
| `AssetWriteBack.ScratchBaselineReproducesEveryLiveAsset` (tightened) | 7.1 — asserts **zero** literal assets hold null where their source assigns a reference |

A two-config regression test for 7.3 is **not** included: generation is asynchronous, multi-frame and
driven by real `UCkAssetRegistryConfig` data assets, and there is no seam that lets a headless test
drive two configs without fabricating them and a full pass. What is covered instead is the merge
semantics the fix relies on (`AccessorResolver.DuplicatePathAcrossFiles`) and the reachability
contract the defect actually harmed (the two tests above). Making the requested test possible would
mean extracting the per-config emit step behind a seam that takes a config and returns its entries.

---

## 8. Adversarial review — findings acted on

An independent review of the whole implementation found four MAJOR defects. All are fixed and gated;
one further defect fell out of fixing them.

### 8.1 `PPF_DeepComparison` false-equalled two distinct assets (silent edit loss)

The diff compared every property with `PPF_DeepComparison`. `FObjectPropertyBase::StaticIdentical`
deep-compares **any** two objects sharing a class and an FName — it is `PPF_DeepCompareInstances`,
not `PPF_DeepComparison`, that restricts the test to instances. Re-pointing a reference from
`/Engine/EngineMeshes/Cube` to `/Engine/BasicShapes/Cube` could therefore read as unchanged, drop out
of the patch set, and be reset from the file by the reload the write triggers — Trap B's harm through
a side door. Deep comparison is now used only for genuinely instanced properties.

### 8.2 `#else` on a non-editor guard reported the block as editor-guarded

`Get_IsInsideEditorGuard` flipped its flag on any `#else`, so the else-arm of a **non**-editor `#if`
read as editor-guarded and would have admitted an editor-only accessor into cooked code. Now tracks
`{IsEditor, InElseArm}` per guard, recognises `#elif`, and scans the comment-blanked copy so a
directive inside a comment cannot open a phantom guard. Latent in this repo — no `.as` file currently
uses a non-editor `#else` — but a live cooked-build break the first time one did.

### 8.3 An unmatched Delete wrote an unchanged file and let the reload undo the revert

A revert of a property whose value comes from a nested scope produced no edit, no diff row, and no
error. The unchanged file was still written, still bumped its timestamp, and the reload still put the
old value back over the user's revert. `Apply_Patch` now reports `UnmatchedDeletes` and the
orchestrator aborts on it, and separately refuses to write when the patch would change no line.

### 8.4 The headline integration gate could not see object divergences

`ScratchBaselineReproducesEveryLiveAsset` iterated only `Diff.Entries`. A divergence on a **non-null**
object reference lands in `Diff.Unresolved` (the test runs with an empty accessor index), so the gate
was blind to precisely the case it exists to catch. It now fails on `Unresolved` too.

### 8.5 Fell out of 8.4 — non-editable properties produced phantom divergences

With the gate repaired it immediately failed, reporting **64 divergences across all 32 `UCurveFloat`
literal assets**: `FloatCurve` and `AssetImportData`, on every one.

An A/B (forcing deep comparison back on, rebuilding, re-running) produced an identical divergence
set, proving 8.1's change did not cause them — they were pre-existing and merely invisible.

Root cause: neither property can receive a details-panel edit. `UCurveFloat::FloatCurve` is a bare
`UPROPERTY()` with no `CPF_Edit` at all, and the `FRichCurve` inside it carries a `transient`,
lazily-generated, per-instance key-handle map that differs between any two instances.
`UCurveBase::AssetImportData` is `VisibleAnywhere` editor import metadata with a distinct subobject
per instance. The button saves what the user changed **in the details panel**, so the candidate
predicate now requires `CPF_Edit` and not `CPF_EditConst`. Without this, write-back would have
aborted on every curve asset in the project.

### Also fixed

Blueprint-generated parent classes excluded from the visibility gate (PLAN §5.5 wording); `FText`
inside a whole-emitted struct now sets the lossy flag; the `asset` regex gained a left token
boundary; the container-preservation assertions no longer silently skip when a fixture line drifts.

### Reported, not fixed

`CaptureDeferredAttribution` mutates three globals with no lock, from a path this codebase documents
as running off the game thread (`CkIO_Utils.cpp:489`). The race is **pre-existing** — the AngelScript
caller runs in the same window — but §7.1 widened its caller set. Filed as a follow-up rather than
fixed here: the correct fix is a lock inside CkCore's own machinery, and an `IsInGameThread()`
early-out would silently drop attribution and leave assets unhealed.

Also outstanding: the per-save cost of §7.2's wider scan is characterised but not measured on a real
editor save, and the enable-state cache goes stale on undo and on instanced-subobject edits.

---

## 9. Deferred-attribution data race — fixed

Reported in §8 as pre-existing and not fixed. Investigated and fixed since.

### It is real, and it is a cooked-build defect

`FAngelscriptManager::ShouldInitializeThreaded()` (`AngelscriptManager.cpp:184-195`) returns **false in
editor** (opt-in via `-as-force-threaded-initialize`) and **true in cooked** (opt-out via
`-as-skip-threaded-initialize`). When true, `Initialize()` dispatches
`AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask, ...)` and the whole compile runs on a worker:

```
Initialize()                                   [game thread]
 └─ Initialize_AnyThread()                     [WORKER THREAD when threaded]  :346
     └─ InitialCompile()                                                      :548
         └─ CompileModules(ECompileType::Initial, ...)                       :1004
             └─ ClassGenerator.PerformSoftReload()/PerformFullReload()       :2694-2750
                 ├─ CallPostInitFunctions()   → Get<Name>() → __Init_<Name>  ClassGenerator :2275
                 └─ InitDefaultObjects()      → CDO DefaultsFunction          ClassGenerator :2276
```

So literal `__Init_` bodies and CDO defaults — and therefore every `assets::load::*` and
`LoadAssetByName` they call — execute off the game thread in a packaged build. Meanwhile the game
thread spins inside `Initialize()` running `ProcessThreadUntilIdle(ENamedThreads::GameThread)`, so a
queued game-thread task can reach the same attribution globals concurrently. Editor being
un-threaded by default is why this has never been seen here.

`PostInitialize_GameThread()` only broadcasts `OnInitialCompileFinished` — it does none of this work,
so there is no game-thread-only path to fall back on.

### Fix

A `FCriticalSection` guards the three globals. `CaptureDeferredAttribution` locks only around the
shared-set mutations — the callstack walk reads this thread's own AngelScript context and needs no
lock. `ResolveAllPending` calls a new `Consume_Attribution()`, which moves all three out under the
lock and leaves them empty; the heal then runs against that private snapshot, so **no lock is ever
held while AngelScript executes**. `ReRunDeferredClassDefaults` and `ReRunLiteralAssetInits` take the
attributed set as a parameter instead of reading the globals. The end-of-sweep reset is gone —
`Consume_Attribution` already performed it atomically, and anything recorded since now belongs to the
next sweep rather than being silently discarded.

Deliberately **not** done: an `IsInGameThread()` early-out in `CaptureDeferredAttribution`. That
would drop attribution for off-thread deferrals and leave those assets unhealed — the exact bug class
this machinery exists to prevent.

### Testing

**A deterministic test is not expressible.** The failure needs two threads inside
`CaptureDeferredAttribution` at once during AngelScript init, which the test harness cannot schedule;
a test that merely calls it twice in sequence would pass with or without the lock and prove nothing.
What does guard the refactor is
`CkAngelscriptGenerator.Integration.AssetWriteBack.ScratchBaselineReproducesEveryLiveAsset`: it
asserts zero literal assets hold null where their source assigns a reference, which only holds if
attribution still reaches the heal. Had the snapshot plumbing dropped or mis-scoped the attributed
set, the five assets in §3 would go unhealed and that test would go red.

### Gates

| Gate | Result |
|---|---|
| `--test-pattern AngelscriptGenerator` | 137 total, **137 passed, 0 failed** |
| Unscoped suite | 1274 total, 1258 passed, **16 failed** — the known inherited set exactly |

The 16: 8 `SceneNodeTween`, 5 `Queue`/`QueueCoordinator`, 2 `PathNetworkFollower`, 1
`Crowd_Grounding`. Both known flakes (`Crowd_BunchUp_SettlesAtSharedGoal`, `IntegrationTest`) passed
this run. No new failures.
