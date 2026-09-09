# One-line summary

Continue GroundNav from the verified Gate62 checkpoint: implement streaming chunk lifecycle (7C), then the required large-world cases and final acceptance, retaining Recast for A/B and aggressively delegating bounded work to Terra.

Written 2026-09-08 in `D:\Repos\CkPlugins_3`. This is an execution handoff, not the original planning prompt. The old root `CONTINUATION_PROMPT_CkNavigationReplacementCampaign.md` is historical and is preserved, not authoritative for this resumption.

## Model and usage recommendation

Use **`gpt-5.6-sol` / `medium` as the lead**. This is a task-specific judgment: 7C still needs ownership, identity, immutable publication, and active-path lifetime decisions. Sol retains these decisions, integration, and final evidence review. Use `gpt-5.6-terra` / `medium` for bounded implementation, production-path tests, and independent review; Terra / `low` for inventories. Use `gpt-5.6-luna` / `low` only for mechanical gate monitoring, extraction, and formatting with review.

Do not keep Astra running for routine orchestration, log polling, or broad scans. Escalate a specific unresolved architecture or root-cause question to Sol / high first; reserve Astra for a concrete difficult blocker with a compact evidence packet. Do not default to max/ultra. This is not a promise of a particular account-usage saving. Official general guidance: https://learn.chatgpt.com/docs/models#choosing-astra-sol-terra-and-luna .

Use short briefs with disjoint file ownership and clear verification. Do not repeatedly give agents the entire campaign history. Reuse agents; request concise evidence and inspect it. Prior lower-tier returns included incorrect timing units, a faulty formula checker, and embedded NUL bytes in docs: delegation does not replace lead verification.

# Repo state

Freshly checked on 2026-09-08; verify again before mutations:

| Repository | Branch | HEAD | State |
|---|---|---|---|
| `D:\Repos\CkPlugins_3` | `feature/ck-navigation` | `28ee8fb9ac45022201ba6182d31f0e041d3109db` | Ahead 1 of tracked branch; dirty |
| `Plugins/CkFoundation` | `feature/ck-navigation` | `e827b041f8d4d6bf03e13e806b318b9bfdcac88f` | Ahead 1; substantial modified and untracked campaign work |
| `Plugins/CkTests` | `feature/ck-navigation` | `f9075655aed851cb02cf7179cf0a578ebaf78519` | Ahead 1; modified tests and untracked native tests/assets |
| `Plugins/CkGameplayDebugger` | `feature/ck-navigation` | `f2527bafcecf0e91473af78d2d442dc66b69218c` | Clean at this checkpoint |

No commits, pushes, rebases, or gitlink updates were made by the resumed work. They are not authorized by this handoff. The committed S13 work and the current dirty S14 work are different artifacts. Many critical new files, including CellPathSearch and this campaign folder, are untracked; `git diff` alone omits them. Some module documentation may be ignored. Inspect explicit paths.

Preserve unrelated root `Config/DefaultGameplayTags.ini`, `Script/Generated/CkPlugins_EntitySpawnParams.as`, the old root continuation, and `Plugins/CkTests/Script/CkGroundNav/CONTINUATION_PROMPT_GroundNavTuningRange.md`. Preserve all inherited campaign code, tests, and the authored external-actor fixture under CkTests. Do not remove unknown dirt.

There is no known active Toolbox gate: Gate62 was reaped, exit 0. Recheck the project log lock before building. Do not inspect or repurpose sibling checkouts. No linked worktrees, clones, copies, alternate checkouts, deletion of build artifacts, or clean rebuilds. Work incrementally in this existing checkout.

# Active bugs or open questions

The user's requirement is: **"navigation to work 100% fully and in all existing circumstances."** The user explicitly expanded this batch to cooked profile variants and authored sublevels, and asks to reduce usage through aggressive Terra/Luna delegation without sacrificing quality.

GroundNav is already the default (`CkNav_ProjectSettings.h`, `_DefaultNavSurfaceProvider`). **S12-D1 keeps Recast selectable for A/B/shadow; deleting Recast/Tier C/PHASE_9 was struck.** Do not reopen retirement or make GroundNav optional. The older PROMPT/VALIDATION retirement wording is superseded by that user decision.

The latest strict-path/Crowd repair slice is complete at Gate62; there is no currently reproduced SteeringPerf blocker. The campaign is NOT complete.

| Remaining work | Requirement and acceptance boundary |
|---|---|
| **7C: next implementation** | Stable integer volume ID plus tile-lattice identity; load/connect, unload, disable/re-enable; immutable seam composition; correct active-path failure/invalidation on unload; all profile variants published atomically. |
| **7D: invokers** | ECS invokers form a deterministic required-tile set, with generation/removal radii, hysteresis, and correct purge. |
| **7E: World Partition/data layers** | Collect exactly admitted geometry; include selectors in fingerprints; visit actor descriptors during cook; WP-aware chunk assets. |
| **7F: transformed field instances** | Pure value offset/90-degree-axis-aligned rotation, ID rebasing, total tag remap, seam re-derivation; invalid transforms/remaps reject atomically. |
| **Editor authoring/verification** | Reconcile actual GroundNav editor-preview and PathNetwork snap behavior; perform or provide the outstanding gym/snap/undo-redo checks and LiveExtract cost measurement. Older U5 text saying no editor Jolt world exists is stale after P5-B2; it is not proof of current implementation absence. |
| **Packaged acceptance** | Build-machine Development/Test runs and Shipping compile; verify cooked/runtime parity, profile/sublevel coverage, streaming behavior, and intended diagnostics stripping. No local cook/package execution. |
| **Campaign close** | Final full-suite comparison on final source; Recast-default phase-close compatibility gate; complete current evidence/provenance/docs and report unresolved real-map acceptance. |

7D/7E/7F were restored in the work order as needed by existing World Partition/large-world use. They are not automatically complete or permanently deferred. Establish the actual consumer/content requirements without broadening to unrelated navigation domains. Floors, ramps, stairs, layered ground, and links are in scope; arbitrary wall/ceiling navigation and free-space flight are not.

Cooked profile bundles and ordinary source-level keyed sublevel loading are implemented. They do NOT provide dynamic chunk streaming. The current commandlet explicitly rejects World Partition maps and transformed streaming-level instances; removing those guards without the underlying support is forbidden.

Unresolved 7C design details to settle and record before delegating edits:

- Scope/uniqueness/admission of the authored integer volume ID, including repeated level instances. `CookKey` and source-level package identity are not automatically a stable runtime chunk ID.
- Ownership of retained disabled tile blobs and their refresh after a normal rebuild/repair; unload versus disable must be distinct.
- Initial streaming state and how ordinary whole-volume cook/runtime bootstrap remains compatible. Do not silently switch all volumes to initially empty.
- Lifecycle owner and level events: composition decides partitioning; a processor must not retrospectively divide a published field.
- How replacement epochs invalidate active paths through F1.34, including in-flight searches, profile variants, and teardown.

# Why prior fixes or investigations were insufficient

Do not restart the long Steering investigation. It was resolved using measured ownership, after several plausible but insufficient changes:

- Native strict-search cost lower bounds and exact-F A* tie breaking reduced fixture work while preserving Dijkstra-optimal costs, but did not independently resolve full-cohort timeouts. They remain useful, verified changes.
- An isolated SteeringPerf pass did not establish cohort correctness. The failing cohort had only about 93 GroundNav ticks over 14.18 seconds; the slice itself consumed about 81.94 ms total. Most wall time was elsewhere.
- Gate59/60 CPU traces attributed the expensive work to default-enabled `FProcessor_CrowdAgent_DrawNavStatus` geometry, not the GroundNav slice. Geometry was 96.15% of the parent scope in Gate60.
- Batching the SAME lines into one normal-world `DrawLines` call per agent fixed that measured cost. Dedicated-server forwarding, global debug-draw enablement, compile-out, streamer mode, line colors/order/thickness/depth/lifetime, labels, and per-frame refresh are preserved. No overlay disable/time-slicing workaround.
- Grounding's earlier duplicate failures were a fixture sequencing defect: `Request_SetTransform` is deferred. Same-step MoveTo queried the old position. The final test stops/quiesces, lifts, observes the off-mesh state, then requests the elevated terminal. It keeps assertions rather than dropping callbacks or increasing unrelated budgets.

Streaming is a separate missing capability. Successful cooked lookup and tile serialization do not prove load/unload correctness, seam equality, or active-path invalidation.

# Available diagnostics and the first evidence to collect

First inspect these bounded artifacts; do not rerun historical gates merely to strengthen wording:

| Evidence | Verified result and limitation |
|---|---|
| `Saved/Logs/S14-Resume-Gate62-CrowdFinal.log`, `.exit.txt`, `.editor.log` | Fresh test-only Crowd: **143/143**, zero failed/skipped/contaminated, 5m44s, exit 0. SteeringPerf own sample: 248 frames, avg 24.029005 ms, max 65.890901 ms, 41.616372 FPS. No performance threshold; not a whole-campaign gate. |
| `Saved/Logs/S14-Resume-Gate61-CrowdBatchedGeometry.log` | Incremental build + Crowd **143/143**, zero contamination, 6m37s, exit 0. New native `GeometryMatchesLegacyPrimitives` actually ran and passed. |
| `Saved/Logs/S14-Resume-Gate56-GroundNavNative.log` | Native GroundNav **435/435**, zero contamination, exit 0; goal/current-plate lower-bound differential coverage. Historical source subset, not a fresh gate for future 7C edits. |
| `Saved/Logs/S14-Resume-Gate49-GroundNav.log` | GroundNav **482/482**, zero contamination, exit 0 on that earlier artifact. |
| `Saved/Logs/S14-Gate60-CrowdSteering.GameThreadTimerStatistics.csv` and `S14-Gate61-CrowdSteering.GameThreadTimerStatistics.csv` | Geometry 13.086859 s / 25375 calls versus 0.057226 s / 51961 calls; approximately 0.515738 versus 0.001101 ms/call. Separate captured runs, not universal navigation/FPS improvement. |

Gate61 raw full-Crowd log is `Saved/Logs/CkPlugins-backup-2026.09.08-15.25.12.log`; the wrapper's Gate61 `.editor.log` is the later one-test native run. Use exact test Started/Completed windows, not arbitrary tails. `LogAutomationController` can replay runtime messages: exclude those replays when counting runtime events, but native AddInfo evidence often lives only in the controller block.

Gate59/60/61 traces are retained in `Saved/Profiling`. Gate59's TimingEvents CSV is roughly 343 MB; do not reread/export it wholesale. Prefer the bounded timer tables. GPU ordering issues prevent a GPU-bound claim. Log frame fields can wrap at 1000: never derive FPS from them. Use the benchmark's actual sample or full-frame-counter diagnostic.

Temporary hardcoded Trace.File/Trace.Stop, runtime-console query, census, CellSearchTiming enable, and SliceServiceWindow enable hooks were removed before Gate62. SteeringPerf's only remaining diagnostic addition versus HEAD is test-scoped `ck.Crowd.Debug.PendingTimeoutState=1`. Native `CellSearchTiming`, `SliceServiceWindow`, and `StrictCrowdCostTimeoutReplay` remain default-off. Do not casually re-enable expensive replay in a timing gate.

Next unused local gate number is **63** at handoff; check filenames before use. No capture/build/test should be launched merely to prepare this handoff.

# Likely symptoms, causes, and files

| Symptom | First cause/contract to inspect | Files |
|---|---|---|
| Loaded tile present but no crossing path | Deferred read not composed, missing seam topology, or layer-ID mismatch | `Field/CkGroundNav_FieldSerialize.*`, `Bake/CkGroundNav_Plates.*` |
| Unloaded chunk still used by an agent | Old field/search/corridor retained without correct geometry epoch invalidation | `Facade/CkGroundNav_WorldFieldRegistry.*`, `Path/CkGroundNavPath_Invalidate_Processor.*` |
| Default profile updates but variants stay stale | Partial publication instead of an all-profile transaction | WorldFieldRegistry and `Cook/CkGroundNav_CookedFieldLoad.*` |
| Disable/re-enable changes bytes or resurrects old markup | Retained blob stale, nondeterministic composition, or mixed bake/derived state | Field serialization, volume rebuild/repair integration |
| Cook works for an ordinary level but rejects WP/instances | Explicit unsupported-world/placement admission, not an incidental missing asset | `CkGroundNavEditor/Cook/CkGroundNavCook_Commandlet.cpp` |
| Large-agent timeout returns | First count service ticks and exact request revisions; inspect existing overlay CPU scopes before changing budgets | GroundNavPath processor, Crowd watchdog, DrawNavStatus processor |
| Grounding emits an unexpected second terminal | Move request issued before deferred transform is visible, or old episode not quiesced | `CkAutoTest_Crowd_Grounding_StationaryAgentReGrounds.as` |
| Tests pass but packaged build fails | Runtime references editor-only code, cooked bundle mismatch, or shipping guard | CkGroundNav/CkGroundNavEditor module boundaries and build-machine logs |

# Critical files (and roles)

All paths below are relative to `D:\Repos\CkPlugins_3\Plugins\CkFoundation` unless stated otherwise.

1. `docs/campaigns/navigation-next/PROGRESS.md` — latest decisions/status/evidence; huge historical file. Read top plus tail and targeted decision hits, never dump it wholesale.
2. `docs/campaigns/navigation-next/PHASE_7.md` — 7C seam-only derivation, lifecycle, identities, invokers, WP/data layers, transformed merge and exact gates. Read alongside `REPRESENTATION.md`, `MIGRATION_SEAM.md`, and current `FEATURE_MATRIX.md` rows F2.16–F2.19.
3. `Source/CkGroundNav/Public/CkGroundNav/Field/CkGroundNav_FieldSerialize.h/.cpp` — `Read_TileInto(...Deferred)`, `Compose_LoadedField`, write/read contracts, atomic rejection.
4. `Source/CkGroundNav/Public/CkGroundNav/Field/CkGroundNav_Field.h` and `CkGroundNav_FieldTypes.h` — fixed lattice, Unbuilt tiles, value-only identities.
5. `Source/CkGroundNav/Public/CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h/.cpp` — immutable field/profile publication and geometry versus cost-only revisions.
6. `Source/CkGroundNav/Public/CkGroundNav/Volume/CkGroundNavVolume_Fragment_Data.h`, `CkGroundNavVolume_EntityScript.*`, `CkGroundNavVolume_Processor.cpp` — authored params, source-level context, cook/runtime build lifecycle.
7. `Source/CkGroundNav/Public/CkGroundNav/Cook/CkGroundNav_CookedFieldLoad.*` and `Source/CkGroundNavEditor/Public/CkGroundNavEditor/Cook/` — complete profile bundle load, normalized source identity, pure cook driver, unsupported WP/instance guards.
8. `Source/CkGroundNav/Public/CkGroundNav/Path/CkGroundNavPath_Invalidate_Processor.*` and `CkGroundNavPath_Processor.*` — F1.34 invalidation, sliced/in-flight search ownership, fairness, repair.
9. `Source/CkGroundNav/Public/CkGroundNav/Search/CkGroundNav_CellPathSearch.*` and `Query/CkGroundNav_Query_DynamicObstacles.*` — strict dynamic snapshot, exact topology, validated costs; preserve while implementing streaming.
10. `D:\Repos\CkPlugins_3\Plugins\CkTests\Source\CkTests\Private\UnitTests\CkGroundNav\` and `Script/CkGroundNav/` — extend existing serialization, profile, invalidation, multiworld, cooked, and production-path tests rather than inventing partial registries.

# Things ruled out

- No Recast deletion or retirement project. Keep alternative-provider health checks and shadow machinery.
- No restarting solved SteeringPerf investigation, reducing costs to 1, relaxing pending timeout, altering search budgets, or hiding expensive drawing by default.
- No claim that 143 Crowd passes proves all maps/platforms or streaming. No claim that ordinary cooked sublevels imply WP support.
- No arbitrary rotation in generation 1: only axis-aligned 90-degree multiples per 7F. Do not snap unsupported transforms silently.
- No in-place mutation of a published field, partial profile bundle publication, reference-returning mutable failure sentinel, or engine-object pointer persisted in a field/blob/snapshot.
- No new public extension points without an actual consumer. No proprietary implementation references or derived code; follow PROMPT provenance policy using independent Ck requirements and permitted public algorithms.
- No blanket whole-suite baseline at every small fix: current NN-D54 scoped-gate policy supersedes older blanket language. Final campaign full-suite evidence is still owed.

# Architecture notes and gotchas

**Composition:** the current field owns a predeclared fixed lattice. `Read_TileInto` replaces a slot only when the lattice matches and leaves output unchanged on refusal. Deferred reads require composition. Existing `Compose_LoadedField` derives seams, links, and reachability over the WHOLE field; it is a useful correctness oracle/bootstrap API but does not by itself meet 7C's bounded newly-adjacent-seam requirement. Do not mislabel whole-field re-derivation as seam-only work. No public remove-tile API was found in the bounded audit.

**Proposed decomposition, not implemented design:** a pure candidate composition layer handles value transitions; an authored identity/manifest layer owns stable IDs and retained chunk state; the volume/world lifecycle layer publishes complete immutable all-profile candidates. First 7C implementation can target a declared same-volume lattice; cross-volume/transformed composition belongs to 7F. Record the lead's exact admission/ownership decisions before assigning disjoint files.

**Profile/cook identity:** source level package + CookKey + profile tag identifies cooked data. Current collision-safe variant assets use `__CkGroundNavProfiles`, encoded tag segments, and a source-level component boundary (see `Source/CkGroundNavEditor/Claude.md`); older `/Profiles/...` notes are historical. Normalize PIE/source-level names. Load all profiles into temporary state and publish once, or reject/fallback for the complete volume. Spawn level package is injected via EntitySpawner, not an authored UObject pointer. Do not conflate these names with new runtime stable integer chunk IDs.

**Strict paths:** graph nodes include full surface/layer identity and terminal/link role. Dynamic discs/OBBs are immutable per-query snapshots; finite-Z and exact closed-cell/union rules matter. Strict results retain route/corridor topology and avoid unsafe funnel shortcuts. Existing cost lower bounds have guards for layered/link cases and must remain admissible/consistent; don't adapt them blindly across new streaming topology.

**Publication/lifetime:** cost-only updates must not trigger the old corridor-repair storm. Geometry changes must invalidate correctly. An asynchronous repair holding a replaced source field must not publish stale results. Test unloaded/disabled chunks during both installed-path and pending-search episodes, and world teardown. Pointer ownership must be explicit and lifetime-appropriate; values inside fields/blobs contain stable IDs, not engine pointers.

**Malformed input:** compute safe validity once, diagnose with CK_ENSURE_IF_NOT, then repeat that boolean in an ordinary return branch. The ensure body may compile out. Its condition must not call the operation whose prerequisites are being validated. Multi-step composition/admission must be atomic. A bare semicolon after this ensure macro caused MSVC C4390 earlier; use the required body.

**Test semantics:** deferred transforms/requests are not immediate. Validate actual production state before creating a regression. Assertions should test failure, no downstream callbacks/mutation, and no partial state for malformed input. Compare serialized values with existing padding/timestamp-aware helpers where appropriate rather than raw object memory.

# Concrete numbered diagnostic and verification flow

1. Read this file fully, repository AGENTS, and relevant Foundation C++/AS and CkTests guides. Read current PROGRESS top/latest entry and PHASE_7 §5–7. Use current numbered user decisions to resolve stale planning text. Check status/HEAD and two concrete evidence artifacts (Gate62 summary and current cook rejection/field composition source).
2. Confirm no gate/live-editor conflict. Do not rebuild merely on resumption. Inventory the exact 7C integration points and current profile bundle behavior with Terra; have a second Terra define real production-path acceptance fixtures. The lead settles the unresolved ownership/ID/initial-state questions above and records a narrow 7C contract.
3. Assign disjoint work: Terra A pure composition/identity, Terra B publication/lifecycle after the interface contract is fixed, Terra C tests or independent review. With only three total worker slots, sequence tests as needed; do not manufacture parallelism across coupled edits. Keep lead work to decisions, integration, and evidence.
4. Verify 7C load A then B and reverse order against A-union-B; exact remaining A after B unload; disable/re-enable byte equality with zero geometry queries; reproducible identities; cross-chunk layer reindex; invalid/duplicate IDs and missing variants rejected without pointer/epoch change; active route and pending search failures through real F1.34; level unload and teardown without crashes/ensures. Seam mismatch is a failed gate, not a new tolerance.
5. Before a gate, freeze all source/AS/config, normalize actual CRLF bytes (including untracked files), inspect diff and test validity, and state planned editor boots. Use incremental C++ build + selected meaningful families once; AS-only runs reuse binaries. Focused gates are not campaign-wide proof. Update current cursor/evidence after each completed slice.
6. Build/test ONLY via `CkAuto/UnrealToolbox.exe`, explicit `--project=D:\Repos\CkPlugins_3`, detached PowerShell `Start-Process ... -PassThru -WindowStyle Hidden` with GUI/escalated permission as required. Never pass `--no-progress-window`; never invoke raw Build.bat, UBT, or editor automation. The wrapper `Saved/Logs/S14-Resume-RunGate.ps1` already follows these conventions and preserves exit/log artifacts. Choose an unused run name. Example after choosing actual coverage:

   ```powershell
   & ./Saved/Logs/S14-Resume-RunGate.ps1 -RunName S14-Resume-Gate63-StreamingNative -Pattern UnitTests.CkGroundNav -Build
   ```

   This is an example, not permission to run a gate before implementation. The wrapper accepts Build/Generate/Discover; use Generate only if genuinely required. Pattern dotted tokens are AND, not OR; do not invent a family-list syntax. After build Toolbox may perform inline discovery and net-pinned plus ordinary groups, then an extra lane for newly discovered tests. `--parallel=1` is not a guarantee of one editor boot. Monitor the existing process/output to completion; a shell timeout is not a reason to relaunch.
7. Inspect actual summary, failure names, fresh startup/runtime ensures and AS errors, and exact raw test windows. Reap exit status. Separate inherited failures, expected negative diagnostics, contamination, new defects, and infrastructure failures. If the same symptom survives two fixes, stop guessing: collect one discriminating measurement before another edit. Prior large diagnostic attempts were expensive; do not repeat them without new evidence.
8. After 7C gate/review, execute required 7D then 7E/7F through their documented gates, retaining ordinary runtime baking and complete profile semantics. Keep unsupported cook guards until proven support replaces them. Do not mix every remaining phase into one unreviewable patch.
9. Close editor-authoring evidence against actual current implementation, not stale U5 prose. Prepare exact build-machine cook/package commands and expected outcomes using the project's authorized tooling; do not run local cook/package. Packaged acceptance remains open until actual Development/Test/Shipping evidence is returned. Human real-map/gym evidence remains distinct from automation.
10. On the final campaign artifact run the prescribed full suite and Recast-default phase-close compatibility families, with proper setting restoration. Compare named failure sets, not totals alone. Reconcile FEATURE_MATRIX/VALIDATION with evidence; don't blindly check old boxes or resurrect retired requirements. Report the complete versus open boundary and leave Git delivery pending explicit authorization.

# Suggested first message

"I’m resuming from Gate62: GroundNav is the default, the Crowd repair slice passes, and Recast stays available for A/B. I’ll verify the checkpoint and define the 7C chunk identity/composition contract, then delegate implementation and production-path tests to Terra while retaining integration and final review. I’ll work in the existing checkout and keep packaged acceptance separate from local automation."

## Launcher prompt

I'm continuing GroundNav streaming and replacement acceptance. Read this continuation prompt fully before doing anything: D:\Repos\CkPlugins_3\Plugins\CkFoundation\docs\campaigns\navigation-next\CONTINUATION_PROMPT_GroundNavStreamingAndAcceptance.md
Resume at the verified Gate62 checkpoint in D:\Repos\CkPlugins_3; first verify the current state and define the 7C streaming identity/composition contract, then implement and verify the remaining work in order.
Aggressively delegate bounded implementation and review to Terra, use Luna only for mechanical work, preserve all dirty work, and retain Recast for A/B; do not create worktrees or duplicate checkouts, commit or push, or run local cook/package builds; use the documented UnrealToolbox gates and keep build-machine/human acceptance explicitly open.
