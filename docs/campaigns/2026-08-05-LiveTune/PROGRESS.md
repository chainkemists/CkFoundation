# PROGRESS — LiveTune campaign

**Living doc.** Updated before ending any session. Design of record:
[../../specs/2026-08-05-LiveTune-design.md](../../specs/2026-08-05-LiveTune-design.md) (GREEN-LIT);
CTO review: [../../reviews/2026-08-05-LiveTune-CTO-review.md](../../reviews/2026-08-05-LiveTune-CTO-review.md).

**Host project for this campaign: CkPlugins_Other** (`D:\Repositories\CkRepos\CkPlugins_Other`) — the
isolated plugin-dev host. Code lands in its `Plugins/CkFoundation` / `Plugins/CkTests` checkouts;
builds + tests run through its toolbox against `CkPlugins.uproject`. (Adam's call, 2026-08-05 —
the campaign docs speak of "BusterBlock repo" because the design session ran there; same submodule
repos, different superproject checkout.)

**Branch discipline (Adam's call, 2026-08-05):** all campaign work is committed progressively on a
local branch `livetune/phase-0` in EACH touched submodule (CkFoundation off `dev` @ `870ad0172`,
CkTests off `dev` @ `0ea0d6a2`). Never pushed; merge/rebase onto `dev` is Adam's post-audit step.

## Phase status

| Phase | Status |
|---|---|
| 0 — Spine | **GATE GREEN** (2026-08-06) |
| 1 — Pilots | **GATE GREEN** (2026-08-06) — Adam authorized full-campaign continuation in-session |
| 2 — AS path | **GATE GREEN** (2026-08-06) |
| 3 — Rollout | **GATE GREEN** (2026-08-06) — CkProvider disposition still pending Adam's call |

Commits on `livetune/phase-0` (local only, never pushed) — CkFoundation:
`7aa1dbd26` (PROGRESS start) → `8db8eb08e` (baseline) → `7bf179a20` (Phase 0 spine) → `fb11243c0`
(Phase 0 docs) → `41469bdd4` (Phase 1 driver+pilots) → `1a85118cc` (Phase 2 reload transport +
UFUNCTION Link) → Phase 3 commit (Probe Reconfigure + docs) → final docs commit.
CkTests: `45fc7ecd` (Phase 0 tests) → `2df8b63d` (Phase 1 tests) → `e677a5cd` (Phase 2 test)
→ Phase 3 tests commit.

## Baseline (recorded BEFORE first edit)

- **Baseline of record (2026-08-05, before any source landed):** toolbox `--test --no-live`
  full suite on CkPlugins_Other against the freshly built Development editor:
  **Total 1002 · Passed 999 · Failed 3 · Skipped 0 · Contaminated 0** (4m06s,
  `Saved/Logs/Test-Baseline.log`). Pre-existing failures, NOT ours:
  `Ck_AutoTest_PathNetworkFollower_DesiredNavmeshClearanceMovesInward`,
  `Ck_AutoTest_PathNetworkFollower_ProjectsRibbonWaypointWithinNavQueryExtent`,
  `IntegrationTest`. "No regressions" claims diff against this list.
- Environment incident before the baseline: a stale `Saved/CkAngelscriptGenerator_RegenOwner.lock`
  (owner pid dead — an old GitLink automation run) made every test lane run regen-SECONDARY and
  the AS compile failed spuriously (exit 76, errors in generated wrappers that were in fact
  current — zero `Script/Generated` diffs after the heal boot). Fix: deleted the stale lock;
  single-lane heal boot came back clean (3/3, zero AS errors). If exit 76 with all-lanes
  "[RegenOwnership] Another editor/commandlet instance owns" recurs, check that lock's pid first.
- Repo ground at start: `Plugins/CkFoundation` on `dev` @ `870ad0172`, tree clean.
  `Plugins/CkTests` on `dev` @ `0ea0d6a2`, tree clean — one commit behind the BusterBlock
  checkout's `403d887` (inventory-test commit; no overlap with this campaign; deliberately NOT
  pulled — sibling sessions own the pointer sync). Superproject dirty paths NOT ours, left for
  their owning session: `CkAuto`, `CkPlugins.uproject`, `Config/DefaultGameplayTags.ini`,
  submodule pointers.
- An earlier baseline run was mistakenly launched on BusterBlock and stopped mid-build before
  its test phase; no artifacts of it are used.

## Fork-source verifications (Phase 0 gate item: undo/redo event shape)

- **Undo/redo → `OnObjectPropertyChanged` arrives with NULL property, ChangeType `Unspecified`.**
  Chain (read on the 5.7 fork, `D:\Repositories\UnrealEngine-Angelscript`, this session):
  `FTransaction::Apply` → `Editor/UnrealEd/Private/EditorTransaction.cpp:953,957` `PostEditUndo()` →
  `CoreUObject/Private/UObject/Obj.cpp:847-852` `PostEditChange()` → `Obj.cpp:513-514` builds
  `FPropertyChangedEvent(NULL)` → `Obj.cpp:520` broadcast. Default ChangeType is `Unspecified`
  (`UnrealType.h:6874`). **Consequence encoded in the subsystem:** an event with no MemberProperty
  is treated as "any linked member of this asset may have changed" — every stamped member of that
  asset is re-diffed; the value-diff gate suppresses the non-changes. Undo/redo therefore
  dispatches the undone member only, at final-commit (all-tiers) policy.
- **Interactive vs ValueSet** re-confirmed at `PropertyHandleImpl.cpp:370-372` (matches CTO review).
- `FInstancedStruct::operator==` compares type + `Identical(PPF_None)` (`InstancedStruct.h:235-237`)
  — used as the diff-gate comparison.

## Decisions made this session (executor-level; design not relitigated)

1. **`Link` is a plain static C++ function in Phase 0, not a UFUNCTION.** The design's "empty
   inline outside the editor" is only literally achievable for a non-UFUNCTION (UHT thunks are
   never inline, and a UFUNCTION must exist in cooked builds for BP/AS callers). The tri-env
   surface (`utils_live_tune`, BP node) is Phase 2's deliverable per design §9; AS AutoTests reach
   Link through a CkTests shim meanwhile. Revert hook: wrap Link as UFUNCTION in Phase 2.
2. **Stamp fragment holds an ARRAY of (asset, member) entries**, not a single pair — two features
   added directly on the same entity (no child) can each Link; single-slot would silently drop
   one. Cleanup removes all entries for the dying entity. Named `ck::FFragment_LiveTune_Stamp`
   (design's `FFragment_DevTuningSource` renamed to the house `FFragment_[Feature]_[Type]` shape;
   spec header says all names are proposals).
3. **Registry + subsystem compile in all builds; functionality is `WITH_EDITOR`-gated internally**
   (mirrors `ACk_EditorSelectionProxyHost_Actor_UE` / in-module precedent — a fully `#if`-gated
   UCLASS in a Runtime module is not house practice). Non-editor: `Link` is an empty inline,
   `Register_*` are empty inlines, the subsystem never instantiates (`ShouldCreateSubsystem`
   false). The stamp fragment itself IS fully `#if WITH_EDITOR` (precedent
   `FFragment_EditorSelectionOwner`).
4. **ViaRebuild dispatch is NOT reachable in Phase 0.** `Register_ViaRebuild` stores config
   (Scope/ReAdd/Produce/Hydrate slots, compile-enforced ReAdd) but the subsystem's dispatch fires
   `CK_TRIGGER_ENSURE` naming the type if a rebuild-tier handler is ever hit — the rebuild driver
   is the Phase 1 pilot deliverable (its gate tests — round-trip, record-disconnect sequencing —
   pin it). No production registrations exist until Phase 1, so the path is unreachable outside
   tests, and no Phase 0 test dispatches it.
5. **Link-time deep validation is tier-aware.** "Struct type matches the handle's feature params
   type" is enforced via the handler's synthesized `Has<T>` check — ViaReplace tier only
   (ViaRequest/ViaRebuild features may keep no params fragment; FloatAttribute keeps none).
   Unregistered types stamp fine (fork call 2: dispatch logs Display "no handler registered").
6. **Diff cache is seeded at Link time** with the asset's current value — the first full-heal
   after linking with no actual change is suppressed, not just later ones.
7. **Authority gate reads the ENTITY, not the handler registration:** skip when
   `Get_IsEntityNetMode_Client(E)` && `Get_EntityReplication(E) == Replicates` (both on
   `UCk_Utils_Net_UE`). No per-handler replication flag needed.
8. **Stamp cleanup rides `on_destroy<FFragment_LiveTune_Stamp>`** on the EcsWorld registry's EnTT
   sink (the CkDebugFeatureFlags listener pattern), connected in the subsystem's Initialize with
   `InitializeDependency<UCk_EcsWorld_Subsystem_UE>` guaranteeing registry lifetime brackets the
   connection. Dispatch additionally validity-checks and iterates a copy (risk §10.5 "impossible,
   not merely rare").
9. **Test seams are plain C++ on the subsystem** (`Test_SimulatePropertyChange`,
   `Test_Get_LinkCount`), wrapped as UFUNCTIONs by a CkTests BPFL shim for AS — keeps the
   framework's reflected surface free of test-only API (CTO suggestion 4 asked for the seam, not
   its reflection). The simulate seam BROADCASTS a hand-built `FPropertyChangedEvent` through the
   real `FCoreUObjectDelegates` path, so AutoTests cover subscription → extraction → gates →
   dispatch end to end.

## File list (Phase 0) — all landed + committed

- CkFoundation `Source/CkEcs/Public/CkEcs/LiveTune/CkLiveTune_Fragment.h` — stamp fragment
- CkFoundation `Source/CkEcs/Public/CkEcs/LiveTune/CkLiveTune_HandlerRegistry.h/.inl.h/.cpp` — registry
- CkFoundation `Source/CkEcs/Public/CkEcs/LiveTune/CkLiveTune_Utils.h/.cpp` — `Link`
- CkFoundation `Source/CkEcs/Public/CkEcs/Subsystem/CkLiveTune_Subsystem.h/.cpp` — change listener
  (beside the existing editor subsystem, per fork call 3's layout)
- CkTests `Source/CkTests/Public/CkLiveTune_AutoTest_Utils.h` + `Private/CkLiveTune_AutoTest_Utils.cpp`
  — AS-facing shim: test params structs (Replace/Request + Spec*), tuning asset, Link/simulate
  wrappers, invocation counters, static-init handler registrations
- CkTests `Source/CkTests/Private/UnitTests/CkEcs/Test_LiveTune_Registry.cpp` — C++ specs
  (`Ck.LiveTune.Registry.*`, `Ck.LiveTune.LinkValidation.*`)
- CkTests `Script/CkEcs/CkAutoTest_LiveTune_DispatchByType.as` / `_DiffGate.as` /
  `_InteractivePolicy.as` / `_StampCleanup.as` — PIE autotests

## Gate status (Phase 0) — ALL GREEN 2026-08-06

| Gate item | Status |
|---|---|
| Link validation rejects wrong-type/missing property loudly, zero partial state | **GREEN** — `CkTests.UnitTests.LiveTune.LinkValidation.{MissingProperty, NonStructMember, MissingParamsFragment, InvalidHandle}` |
| Registry dispatch by type | **GREEN** — `CkTests.UnitTests.LiveTune.Registry.DispatchByType` (registry contract) + `Ck_AutoTest_LiveTune_DispatchByType` (full pipeline) |
| Stamp cleanup on entity destroy | **GREEN** — `Ck_AutoTest_LiveTune_StampCleanup` (waits on actual unregistration, then proves no dispatch) |
| Diff gate suppresses no-op + full-heal dispatch | **GREEN** — `Ck_AutoTest_LiveTune_DiffGate` (seeded / changed / repeat) + `Ck_AutoTest_LiveTune_InteractivePolicy` (change-type policy) |
| Undo/redo event shape verified on fork | **GREEN** — source-verified (see verifications); null-property events re-diff all linked members |

### Gate run of record (2026-08-06, `Saved/Logs/BuildTest-Gate3.log`, fresh build + fresh discovery)

**Total 1013 · Passed 1010 · Failed 3 · Contaminated 0** (3m55s). Delta vs baseline (1002/999/3):

- The two stable baseline reds are unchanged: `Ck_AutoTest_PathNetworkFollower_DesiredNavmeshClearanceMovesInward`,
  `Ck_AutoTest_PathNetworkFollower_ProjectsRibbonWaypointWithinNavQueryExtent`.
- `Ck_AutoTest_UsfOutline_VatShadowCustomData` red in this run only — **flake, not a regression**:
  green in the baseline and in BOTH intermediate full runs of the same code, and green isolated on
  the same binary (`Test-UsfIsolate.log`, 1/1). No shared code path with LiveTune.
- `IntegrationTest` (Angelscript coverage) was red at baseline, green in all three later full runs —
  baseline-side flake.
- +11 tests total = the 9 LiveTune tests (all green; also 9/9 in the earlier pattern run,
  `BuildTest-LiveTune.log`) + 2 pre-existing tests the baseline's CACHED discovery had silently
  skipped (`Ck_AutoTest_PoiDisplayDefinition_DisplayOverride`,
  `CustomMainPassRequiredFragments_SkipsAndWakes` — both green; the `--discover-fresh` trap, live).
- **Delta on pre-existing suites: zero regressions.**

### Success criteria closure

1. `Source/CkEcs/LiveTune/` exists and compiles: **editor target** green (gate run);
   **non-editor (Game target)**: CkEcs incl. every LiveTune TU compiled + linked clean with
   `WITH_EDITOR` out (`Build-Game.log` — zero LiveTune/CkEcs errors); the Game build then fails in
   PRE-EXISTING `CkEntityVisualizer_Utils.cpp:169-170` (uses `ck::FFragment_EditorSelectionOwner`
   outside `WITH_EDITOR`) — not ours, flagged as follow-up. `WITH_ANGELSCRIPT_CK` both-ways:
   verified by inspection — the Phase 0 surface has zero AS-conditional code (no AS includes,
   no bindings; the tri-env surface is Phase 2 by design).
2. Gate AutoTests in CkTests, green via toolbox fresh boot, baseline delta-zero: **done** (above).
3. PROGRESS current; local commits on `livetune/phase-0` in both submodules; nothing pushed: **done**.
4. Close-out report: delivered in-session.

## Post-baseline findings (for the auditor / future phases)

- **`Ck.<Feature>.*` C++ pretty names never run in this host's full pass.** The toolbox's default
  `--test` runs "project tests" = rows whose TOP path segment is a plugin name (`CkTests`,
  `GitLink`, …). `Ck.Registry.SlotTable.*` and `Ck.DebugFeatureFlags.*` are likewise invisible to
  full passes (pre-existing; they only run via explicit `--test-pattern`). The LiveTune specs
  therefore joined the `CkTests.UnitTests.<Module>` family. Feeds the A2 pretty-name adjudication.
- The flags change that preceded the rename (EngineFilter → `ck::tests::kCkUnitTestFlags`) is kept:
  it is the documented house constant for this family.

## Phases 1-3 (2026-08-06, same session — Adam authorized full-campaign continuation)

### Phase 1 — ViaRebuild driver + Timer/FloatAttribute pilots (`41469bdd4` / `2df8b63d`)

- Driver on the (now tickable) subsystem: capture → deferred destroy → re-Add keyed on ACTUAL
  destruction (`IsValid_Policy_IncludePendingKill` false = fully gone; loud 5s timeout) → hydration
  enqueued into `FFragment_PendingHydration` + tag (the standard dispatcher; no parallel path) →
  automatic re-link + cache catch-up for mid-rebuild edits. `Scope::Entity` validates spawn-recipe
  provenance (refusal test) and respawns via `Request_SpawnEntity` (respawn path implemented but
  has NO dedicated success-path test — the script's own Construct re-links; flagged for audit).
- **Capture-override saga (three iterations, each hypothesis-tested):** (1) owner-keyed capture was
  WRONG — the attribute save handler is keyed per-attribute-entity (`CkAttribute_RestorePersistence.h`
  says so; only the net apply is owner-keyed) → empty payloads; (2) delta-riding capture was WRONG —
  `Request_Override(value, Current)` writes the Current component's BASE, so base IS the live value
  and delta was always 0; (3) correct: keep the Current entry VERBATIM, drop Min/Max config entries
  so fresh clamps win. Registry gained a typed optional `.Capture(LinkedEntity, FreshParams)` slot
  for features whose save payload conflates config with runtime state; the useless owner-capture +
  hydrate-override slots were REMOVED (hydration must have exactly one code path).
- Timer pilot: `Replace<Params>` + `.PostReplace` re-enqueues `Request_ChangeCountDirection` from
  the fresh params. Known bound: the chrono GOAL is baked at Add — a Duration retune needs a Timer
  request that does not exist (follow-up candidate, not built).
- Phase 1 gate run: 1017 total / 13 LiveTune green / failures = 2 stable baseline reds + 3 proven
  flakes (`FallsBackToNavigation` pass+fail on the same binary — zero-margin arrival assert, chip
  filed; `FormatterRoundTrip` — SQLite lane-contention error attributed into its window;
  `IntegrationTest` — coverage-report contention, red at baseline too). Zero deterministic
  regressions.

### Phase 2 — AS-reload transport + tri-env Link (`1a85118cc` / `e677a5cd`)

- `UCk_DeferredAssetInit_UE::OnAssetsReinitialized` (CkCore-local) fires at the end of every heal
  sweep that re-initialized literals, carrying the healed set; the subsystem re-diffs every linked
  member of each healed asset (shared code path with undo/redo's null-property events).
- `Link` is now a UFUNCTION → BP node + generated `utils_live_tune.as` (CkFoundation's
  `Script/Generated` is gitignored; the wrapper regenerates on boot). `AsReloadHeal` drives the REAL
  delegate and links through `utils_live_tune::Link` — the AS-environment proof. Binding discovery:
  handle-first UFUNCTIONs bind as handle METHODS in AS, not class statics — direct
  `UCk_Utils_X_UE::Request_*(...)` calls do not compile for typesafe-handle firsts; use the
  `utils_*` wrapper (cost one exit-76 iteration).

### Phase 3 — Probe ViaRequest pilot + docs

- New Probe API: `FCk_Request_Probe_Reconfigure` + `Request_Reconfigure` + handler. Contract: the
  live-read subset (Filter/ResponsePolicy/ContextOverlapPolicy/SurfaceInfo — read from the params
  fragment at every overlap evaluation) re-applies via params Replace; baked/identity fields
  (ProbeName/MotionType/MotionQuality/StartingState) must match or the request is rejected loudly
  and ATOMICALLY (completion Failed, nothing applied — pinned by
  `Ck_AutoTest_Probe_Reconfigure_RejectsBakedFieldChange`). LiveTune registration in
  `CkProbe_Fragment.cpp` routes edits through it (`Ck_AutoTest_LiveTune_ProbeViaRequest`).
- Docs: `Source/CLAUDE.md` decision-table row; `CkEcs/Claude.md` § LiveTune (registration guidance,
  three tiers, severance warning); module-layout line.
- **CkProvider disposition: NOT executed** — needs Adam's explicit call (delete vs archive + stale
  doc correction). The CTO review confirmed its docs actively mislead; untouched per scope discipline.

## FINAL CAMPAIGN GATE (2026-08-06, `Saved/Logs/Test-Gate-Final.log`, fresh boot + fresh discovery)

**Total 1020 · Passed 1016 · Failed 4 · Contaminated 0** (3m59s) on the final binary carrying all
four phases. All 16 LiveTune tests green. Failure accounting vs the recorded baseline (1002/999/3):

- The two stable baseline reds, unchanged: `Ck_AutoTest_PathNetworkFollower_{DesiredNavmeshClearance
  MovesInward, ProjectsRibbonWaypointWithinNavQueryExtent}`.
- `Angelscript...IntegrationTest` — the recurring coverage-report contention flake (red at baseline,
  green in three intermediate full gates, red here).
- `GitLink.Stage.LfsPointerNotRaw` — flake: green isolated on the same binary
  (`Test-IsoGitLink.log`, 1/1); git/LFS I/O under parallel-lane contention; no code-path overlap
  with this diff.
- +18 total vs baseline = 16 LiveTune tests + the 2 cached-discovery gaps found at the Phase 0 gate.
- **Zero deterministic regressions across the entire campaign.**

## [EDITOR-VERIFY] items (for Adam)

All headless behavior is test-pinned; these are the real-editor UX checks the design's gates name:

1. **Mid-PIE details-panel retune (Phase 1 gate item).** Author a quick PDA (any UObject asset) with
   a `FCk_Fragment_FloatAttribute_ParamsData` member; in an EntityScript's Construct:
   `auto Attr = utils_float_attribute::Add(...asset member...)` then
   `utils_live_tune::Link(Attr, Asset, n"<member name>")`. PIE → damage the attribute → edit the
   member's MaxValue below current in the details panel → the attribute re-clamps live within a few
   frames, and the log shows `LiveTune: rebuilt [...] re-applying [1] captured payload(s)`.
2. **AS literal hot reload (Phase 2 gate item).** Same script but with an `asset ... of ...` literal;
   change a tuned value in the .as file and save → hot reload → the change lands on live entities;
   an unrelated .as save does NOT re-apply anything (diff-gate suppression; no `LiveTune:` log).
3. **BP node (Phase 2 tri-env).** In any Blueprint graph: search "[Ck][LiveTune] Link" — the node
   exists, takes Handle/Tuning Asset/Member Name, returns Handle.
4. **Interactive scrub feel.** With a linked ViaReplace feature (e.g. a Timer's params member), DRAG
   a slider mid-PIE: values land per-tick (Interactive events), no rebuild storms from ViaRebuild
   features while dragging (their edits land on mouse-release commit only).

## Known limitations / notes for the auditor

- `Ck.LiveTune.LinkValidation.MissingProperty` pins loudness with expected-error occurrences=0
  (require ≥1). If the ensure display policy registers per-site ignores (LogOnly/MessageLog), a
  warm-server RE-run of the same process could see the site suppressed — the fresh-boot gate of
  record is unaffected. The other validation tests use tolerant `-1` + structural asserts.
- Test counters are process-global statics; every AS test asserts DELTAS against its own
  fresh asset instance, so parallel lanes (separate processes) and sequential same-world runs
  are both safe.

## [EDITOR-VERIFY] items (for Adam)

- (accumulating; none yet — the real details-panel mid-PIE edit lands with Phase 1 pilots)
