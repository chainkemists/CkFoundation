---
name: ck-feature-frontier
description: Use when deciding what to build, harden, or optimize next in the Ck plugin suite — "what should we work on", roadmap/investment questions, picking a robustness/performance/tooling target, evaluating a proposed new framework feature, or judging whether an old branch is worth reviving. Not for fixing a live bug (ck-debugging-playbook), the in-flight teardown defect (ck-lifecycle-teardown-campaign), or process/multi-session discipline (ck-methodology).
---

# ck-feature-frontier — where the framework could advance next

## Overview

This is the vetted portfolio of candidate framework advancements for CkFoundation /
CkGameplayDebugger / CkTests, grounded in what the code actually contains as of 2026-07-02.
Everything here is a **candidate** — evidence-backed and scoped, but not committed roadmap; the
maintainer picks. Every "Gap" and "asset" claim below was re-verified against the repos on the
campaign date (re-verification commands in the final section).

## The ruling (maintainer, 2026-07-02 — binding)

> "Advancing the framework" = **ROBUSTNESS and PERFORMANCE first; debug tooling next** (tooling
> usually lands in CkGameplayDebugger — debug tooling is explicitly valued). **Stale/outdated
> branches MUST NOT be touched — they are context, not work items.**

In-repo record: `.claude/reports/DECISIONS.md` §25. Consequences:

- Rank candidates robustness > performance > tooling. Within a class, prefer the candidate whose
  **first step is a measurement or a failing test**.
- A proposal that amounts to reviving a fenced branch (below) is out of bounds — even if
  re-described in new words.

## The fence — do-not-resurrect branches

**The canonical registry lives in `ck-failure-archaeology`** (§"Stalled-branch registry —
DO-NOT-RESURRECT") — per-branch tip dates, history, and assessments live there, once. This section
only states the rule and the headline names (verified on origin 2026-07-02):
`feature/dependency-injection-entity-script` · `feature/entity-script-as-script-struct` ·
`upgrade/ue5.7` · `dev-bb-5.7` · `dev-bb` · `feature/editor-time-construction-scripts` ·
`feature/message-without-name` · `bugfix/test/investigate-cheat-in-build` ·
`feature/entity-replication-channel` · `feature/flow-graph-module` · `feature/ability-traits` ·
`feature/modular-traits` · `shelf/dummy-changes-to-debug-networking-asserts` · plus the backup
fossils `backup/before-reverting-f8a3a55` and `backup/pre-exporter-rewrite-2026-05-28` (insurance
snapshots, not work). Do not check them out, merge them, or restart their feature under a new name
without the maintainer's explicit say-so.

`feature/registry-handle-storage-support` is cited by a live comment
(`Source/CkEcs/Public/CkEcs/Registry/CkRegistry.h:289`) but **does not exist on origin** — don't
go hunting for it. Branches with tips from ~June 2026 onward are likely **active sibling work**
(e.g. the iskm/save-load lines) — coordinate, never adopt.

## When NOT to use this skill

| You actually have… | Load instead |
|---|---|
| A live bug / build break / crash to fix | `ck-debugging-playbook` |
| The entity-teardown / signal-unbind defect campaign | `ck-lifecycle-teardown-campaign` |
| "Has this been tried before?" | `ck-failure-archaeology` |
| How to benchmark / make a perf claim | `ck-performance-and-analysis` |
| Process, phase-gates, multi-session discipline | `ck-methodology` |
| How to add a debugger view once a tooling candidate is picked | `ck-gameplaydebugger-extension` |

## The portfolio

Vocabulary used once: *fragment* = ECS component; *processor* = ECS system; *ensure* =
`CK_ENSURE_IF_NOT`, the loud validation macro; *EnTT* = the vendored C++ ECS library (3.16.0).
Every entry is a **candidate (open)**. File paths are relative to `Plugins/CkFoundation/` unless
another repo is named.

### 1. Teardown/unbind lifecycle hardening (continuation) — robustness

- **Gap.** Entity cleanup paths are the framework's weakest guarantee. Two processors carry the
  same live landmine comment — "This processor doesn't get called, can cause issues if teardown is
  mid interaction!!!" (`Source/CkInteraction/Public/CkInteraction/InteractTarget/CkInteractTarget_Processor.cpp:222`,
  `.../InteractSource/CkInteractSource_Processor.cpp:177`) — and Team cannot tell when a listener
  tag is safe to remove: "figure out a bullet-proof way to remove the FTag_TeamListener if ALL the
  delegates have been unbound" ×3 (`Source/CkRelationship/Public/CkRelationship/Team/CkTeam_Utils.cpp:376,402,428`).
  Stock UE has no equivalent concept to help; this is Ck's own contract to harden.
- **This codebase's asset.** The campaign already exists — load `ck-lifecycle-teardown-campaign`
  for the live defect and its plan. This entry is that campaign's **continuation into
  framework-wide guarantees**: e.g. a debug-build *bind-leak detector* that, on entity destruction,
  enumerates surviving signal bindings and fires an ensure naming them.
- **First three steps.** (1) Finish the campaign skill's live target (the two dead-teardown-path
  processors above). (2) Design the bind-leak detector; acceptance case = it answers the exact
  question the three `CkTeam_Utils.cpp` TODOs ask ("are ALL delegates unbound?"). (3) Author a
  CkTests autotest that binds a delegate, destroys the entity without unbinding, and expects the
  detector's ensure (test authoring: `ck-tests-authoring-and-running`).
- **You have a result when** the planted-leak autotest fails without the detector and passes with
  it, and the existing suite runs with zero new ensure fires.
- **Class:** robustness.
- Constraint: signal storage's `in_place_delete` (`Source/CkEcs/Public/CkEcs/Signal/CkSignal_Fragment.h:44,99`)
  is settled, load-bearing design (global fragment-storage pointer stability, DECISIONS.md §45) —
  the detector must work *with* it, not propose replacing it.

### 2. Fragment-held UObject GC audit automation — robustness

- **Gap.** UE's garbage collector does not trace fragment members (root CLAUDE.md, "UObject refs
  in fragments") — an object only a fragment points at gets collected unless something else roots
  it. This shipped a real incident: the per-entity replication driver was reclaimed mid-session
  whenever no netdriver incidentally kept it alive (fix `56b344310`; history:
  `ck-failure-archaeology`). The only defense today is a **hand-maintained audit**
  (`d:\Repos\BusterBlock\docs\Fragment-UObjectRef-GC-Audit.md`, 82 fields checked manually) that
  rots as fragments are added.
- **This codebase's asset.** The convention is already machine-countable: 13 `TStrongObjectPtr<`,
  35 `TWeakObjectPtr<`, 4 `TObjectPtr<` members across `*_Fragment.h` (2026-07-02 counts, commands
  below). And a tripwire-ensure precedent exists at the GC rooting pass:
  `CK_ENSURE_IF_NOT(IsInGameThread(), ...)` at
  `Source/CkCore/Public/CkCore/IO/CkDeferredAssetInit_AngelScript.cpp:521` (commit `a8a93baac`,
  hardening the `feb08ee94` rooting pass in the same file).
- **First three steps.** (1) Regenerate the census with the rg commands below and diff against the
  82-field audit doc — the drift since it was written is the first deliverable. (2) Script the
  census so an unlisted new UObject-ptr field becomes a red report line (natural home:
  `Source/CkScripts/`, the existing maintenance-script dir — propose it to the maintainer). (3) Design open: a runtime debug-build scanner asserting rooted-ness needs per-type
  registration — fragments are plain C++, invisible to reflection — so follow the
  `CK_REGISTER_SNAPSHOTABLE` registration-macro precedent (candidate 3) for the highest-risk types.
- **You have a result when** an autotest planting an unrooted UObject behind a registered
  must-outlive fragment field goes red via the scanner, and the static census flags a deliberately
  added unjustified field.
- **Class:** robustness.

### 3. Snapshot coverage expansion — robustness

- **Gap.** Save/load only captures fragment types that opted in; a fragment that was never
  *classified* is dropped **silently** — the audit's own ensure text names the failure: "would be
  silently dropped on save/load" (`Source/CkEcs/Public/CkEcs/Snapshot/CkSnapshot_Audit.cpp:55-57`).
  Today: **119** `CK_REGISTER_SNAPSHOTABLE(` call sites vs **295** `FFragment_` declarations in
  `*_Fragment.h` — nobody can currently say which part of that gap is intentional (Transient) vs
  unowned. CkSnapshot also has no per-module `Claude.md` (Source/CLAUDE.md table notes).
- **This codebase's asset.** The classification machinery is built and self-documenting:
  `FSnapshotPolicy_RoundTrip` / `FSnapshotPolicy_Transient` with the design rationale in-header
  (`Source/CkEcs/Public/CkEcs/Snapshot/CkSnapshot_Policy.h:9-19`); a capture-time audit that
  catches "classified but unregistered" (`CkSnapshot_Audit.cpp:48-62`, invoked from
  `Source/CkSnapshot/Public/CkSnapshot/Snapshot/CkSnapshot_Capture.cpp:41`); and a Tier-A opt-in
  exemplar (`using IsSnapshotable = void;` with rationale comment,
  `Source/CkTimer/Public/CkTimer/CkTimer_Fragment_Data.h:90-92`). What nothing catches is
  "never classified at all" outside the holder/record families the policy parameter forces.
- **First three steps.** (1) Produce the coverage report: script the diff of the fragment census
  vs registration sites, and classify each gap RoundTrip-owed / Transient-correct / needs-ruling
  using the policy vocabulary. (2) Pick N high-value unclassified families (agree N with the
  maintainer) and classify them. (3) Prove each new RoundTrip family with a save→load round-trip
  autotest in CkTests (existing family naming: `Ck.Snapshot.*` — see ADJUDICATIONS A2 before
  naming).
- **You have a result when** the report exists as a re-runnable command and N new families are
  round-trip-proven red-then-green.
- **Class:** robustness.

### 4. Replication param-copy cost — performance (measure first)

- **Gap (suspected, unmeasured).** Every entity creation copies the net-params fragment and up to
  three net-mode tags from its lifetime owner:
  `Source/CkEcs/Public/CkEcs/EntityLifetime/CkEntityLifetime_Utils.cpp:478-512`
  (`FFragment_Net_Params` copy + `FTag_HasAuthority` / `FTag_NetMode_IsClient` / `FTag_NetMode_IsHost`).
  At crowd/ISM entity counts that is per-spawn registry work on a hot path. Someone anticipated
  this: the kill switch `CK_DISABLE_NET_PARAM_COPY_PER_ENTITY` exists in **all six** config
  branches of `Source/CkBuildConfig/CkBuildConfig.Build.cs` (:78,:93,:115,:128,:151,:171) — and is
  hardwired `=0` in every one. The cost was never measured.
- **This codebase's asset.** The code already compiles both ways (`#if NOT
  CK_DISABLE_NET_PARAM_COPY_PER_ENTITY` at `CkEntityLifetime_Utils.cpp:478`), so an A/B
  measurement build is a one-line Build.cs flip, no code authored.
- **First three steps.** (1) Benchmark FIRST (`ck-performance-and-analysis` for the harness): a
  many-entity spawn scene, stock vs a local `=1` build. The `=1` build is a **measurement build
  only** — it breaks every consumer of the copied tags, so nothing from it ships. (2) Census the
  consumers (`rg -n "FTag_NetMode_IsClient|FTag_NetMode_IsHost|FTag_HasAuthority" Source/`) so any
  future alternative knows what it must serve. (3) Only if the number is material: write the
  replacement *proposal* (e.g. owner-chain resolution at read time) — design, not code, until the
  maintainer rules.
- **You have a result when** an A/B number for entity-creation cost on a many-entity scene is
  recorded in the perf skill's format. "Negligible — close the candidate" is a valid result.
  Measurement needs a running game — a human step for anything an agent session can't launch.
- **Class:** performance.

### 5. Storage deletion-policy cost + EnTT owning groups — performance (measure first)

- **Gap (verified structure, unmeasured cost).** Every fragment storage in the framework is
  tombstoned, in every build config including Shipping: `CkHandle.h:71-77` declares a **global,
  unconditional** `entt::component_traits<Type>` override with `in_place_delete = true` for all
  types (`Source/CkEcs/Public/CkEcs/Handle/CkHandle.h` — the file's first `#if` is at :79, after
  the trait, so no debug gate applies). This is deliberate: introduced debug-gated in `745507381`
  (2024-03-07, "with ECS debugging enabled, we force pointer stability"), relocated still-gated in
  `6b54d2e384` (2024-04-12), then **deliberately ungated in `06938bba3` (2026-02-17, "feat:
  fragments are always pointer stable")** — DECISIONS.md §45. In-place delete costs iteration density
  (storages never repack) and statically forbids EnTT owning groups — the cache-packing iteration
  mode the vendored library offers: `group.hpp:697`
  `static_assert(((Owned::storage_policy != deletion_policy::in_place) && ...), "Groups do not
  support in-place delete");`. Zero group usage exists in Ck code today (rg: 0 hits outside the
  vendored library).
- **This codebase's asset.** All iteration funnels through one site — `ck::TView::ForEach` →
  `_Registry.template view<...>(entt::exclude<...>).each(...)`
  (`Source/CkEcs/Public/CkEcs/Registry/CkRegistry.h:152-168`, the `.view` call at :162) — so both
  the measurement hook and any later prototype have exactly one integration point. The types that
  genuinely NEED pointer stability already declare it per-type (signal fragments
  `CkSignal_Fragment.h:44,99`; handle-debug `CkHandle_Debugging.h:67` — currently shadowed by the
  global trait, but load-bearing the moment the global narrows).
- **First three steps.** (1) Benchmark FIRST (`ck-performance-and-analysis`): trace a heavy scene,
  rank processors by cost; then A/B a *local measurement build* that narrows the global trait to
  per-type opt-in (keep signals' and handle-debug's opt-ins) and re-run — this quantifies the
  tombstone/density cost in isolation. (2) Hidden-coupling sweep before trusting the B build:
  2.5 years of code may incidentally rely on fragment pointer stability (cached `&fragment`
  across adds) — run the full test suite on B and treat any delta as a dependency inventory, not
  noise. (3) Only after a material number: prototype ONE owning group for the hottest eligible
  fragment set on a branch, A/B it. Owning groups also claim exclusive ownership of their storages
  (two owning groups can't share a fragment type).
- **You have a result when** the A/B number for the global-vs-narrowed deletion policy exists,
  plus the suite-delta dependency list — that package is what the maintainer needs to rule.
  A negative result (cost negligible) closes the candidate honorably; negative results are house
  practice (cf. the TSR breadcrumbs at `CkIsmRenderer_Processor.h:135`).
- **Class:** performance.
- Constraint: global pointer stability is settled design (DECISIONS.md §45; A3 resolved), so this
  candidate is a prioritization/measurement question, not a pending doctrine ruling. Narrowing the
  GLOBAL trait in a *local measurement build* while keeping signals' per-type opt-in leaves shipped
  semantics untouched. Flipping the global trait for real is high-blast — it ships only with the
  maintainer's explicit go, never as part of this measurement.

### 6. AS-surface completeness checker — robustness/tooling

- **Gap.** AngelScript binding regressions are silent. Verified failure shape: a Utils function
  whose first parameter type matches its class's ScriptMixin target binds as a handle **member
  only** — "the static form does not even resolve" (`Script/CLAUDE.md:116-119`). A signature drift
  can therefore remove a callable symbol with no compile error on the C++ side and no runtime
  error until some script calls it. The existing CI guard does NOT cover this: the drift
  commandlet checks only the three self-heal generators — "EntitySpawnParams, AutoTestActors, and
  DynamicHandleTypes.json" (its own header,
  `Source/CkAngelscriptGenerator/Commandlets/CkAngelscriptGenerator_DriftCommandlet.h:11-20`).
- **This codebase's asset.** The generator already enumerates the C++ surface: the wrapper
  generator emits 268 `utils_*` namespaces at editor boot (`Script/CLAUDE.md:113`; on disk today:
  274 generated `.as` files under `Script/Generated/`, which includes non-wrapper files). The
  drift commandlet is the ready-made headless-CI harness pattern to extend.
- **First three steps.** (1) Build the inventory: reflection-walk `UCk_Utils_*_UE` classes for the
  *expected* exposed symbols; diff against the emitted `Script/Generated/` surface at symbol
  granularity. (2) Wire the diff into `UCkAngelscriptGenerator_DriftCommandlet` (or a sibling
  commandlet) so CI's `git diff --exit-code` pattern extends to symbol presence. (3) Red test:
  plant a deliberate mixin break in a test module (add a first param matching the ScriptMixin
  target) and confirm the checker flags the vanished static form. (AS mechanics:
  `ck-angelscript-interop`.)
- **You have a result when** the checker is red on the planted break, green on a clean tree, and
  runs headless.
- **Class:** robustness/tooling. Home: CkFoundation (the generator lives here, not the debugger).

### 7. Signals + requests overlay providers — tooling (home: CkGameplayDebugger)

- **Gap.** In a request-deferred ECS (root non-negotiable #5: utilities enqueue, processors
  mutate), the two most load-bearing invisible states are *pending requests* and *bound signals* —
  and none of the 19 in-game overlay providers shows either (provider list:
  `Plugins/CkGameplayDebugger/Source/CkEntityDebugOverlay/Private/Providers/` — attributes, Timer,
  StateMachine, Goap, Team, …; no Signals, no Requests). A swallowed request is undiagnosable
  on-screen today.
- **This codebase's asset.** Registration is one macro — `CK_REGISTER_DEBUG_OVERLAY_PROVIDER`
  (`.../Public/CkEntityDebugOverlay/Provider/CkDebugOverlay_Registry.h:66-69`) — with 19 exemplars
  and 7 automation specs (`Private/Tests/*.spec.cpp`). Requests are already debug-identifiable:
  every request struct carries `CK_REQUEST_DEFINE_DEBUG_NAME` → `Get_RequestDebugName()`
  (`Source/CkEcs/Public/CkEcs/Request/CkRequest_Data.h:113-119`), and the finished 16/16-module
  callstack campaign (`Source/DEBUG_CALLSTACK_PROGRESS.md`) left per-request-fragment history
  entries — frame number, function, message (`ck::TFragment_Debug_Callstack`,
  `Source/CkEcs/Public/CkEcs/Handle/CkDebugCallstack_Fragment.h:17-48`).
- **First three steps.** (1) Load `ck-gameplaydebugger-extension`; copy the closest provider
  shape. (2) Requests card first: the callstack fragments are templated per tracked type, so
  design the small enumeration registry that lets one provider walk them generically — that design
  note is the real step-1 deliverable. (3) Signals card second (per-entity bound connections),
  then a spec test alongside the existing 7.
- **You have a result when** `[EDITOR-VERIFY]` in PIE with the overlay enabled (the
  `ck.DebugOverlay*` commands — see the plugin's CLAUDE.md), focusing an entity shows its pending
  requests and bound signals updating live. Exact human steps: PIE → enable overlay via its
  console command → focus a request-heavy entity (e.g. one mid-interaction) → observe the new
  cards on the focus card.
- **Class:** tooling.

### 8. CkInsightsAnalyzer processor-scheduling view — tooling/performance

- **Gap.** 388 registered processors (count of `CK_REGISTER_PROCESSOR(` sites, 2026-07-02)
  schedule through groups, ordering edges, and pump passes — and ordering bugs are a repeat
  offender here (the dirty-marker rake bit in 2023 and again in 2026; history:
  `ck-failure-archaeology`). No existing view answers "which processors ran, in what group order,
  and what did each pump pass cost" from a captured trace; stock Unreal Insights has no concept of
  Ck's scheduler.
- **This codebase's asset.** The analyzer module exists and is UncookedOnly with a full pipeline
  to extend: `Source/CkInsightsAnalyzer/Public/CkInsightsAnalyzer/` — `Core/` (`CkFrameAnalyzer`,
  `CkTimerCategorizer`, `CkTraceSession`), `Report/` (`CkFrameReport`, `CkMultiFrameReport`),
  `Tab/` (`SCkFrameBarChart`, `SCkInsightsAnalyzerTab`), plus a commandlet. Scheduler ground truth
  is already structured: `Source/CkEcs/Public/CkEcs/Scheduler/CkSchedulerDebugData.h` and
  `CkProcessorGraph.h`.
- **First three steps.** (1) Read the existing analyzer pipeline (`CkTimerCategorizer` is the
  precedent for categorizing trace timers). (2) Map trace timer names onto the scheduler's
  processor/group registry so per-group and per-pump attribution is possible. (3) Add one analysis
  view (report column or tab section) over a captured trace.
- **You have a result when** the new view renders over a real captured trace and answers the
  per-processor / per-group / per-pump question for one session's capture.
- **Class:** tooling/performance.

## Prioritization

1. **Robustness first** (ruling): candidates 1, 2, 3 — and 6, whose payoff is robustness even
   though it ships as tooling.
2. **Performance second**: 4 and 5. Both are deliberately measure-first — the FIRST step is the
   benchmark, and "measured: negligible" closes them honorably. No perf change lands without the
   A/B number (root non-negotiable #7).
3. **Tooling third**: 7 and 8 — explicitly valued by the maintainer, and both land in their proper
   homes (7 in CkGameplayDebugger; 8 in CkFoundation's own analyzer module).
4. Within a class, prefer the candidate whose first step is a measurement or a failing test — all
   eight above were shaped to satisfy that.

## Common mistakes

- **Reviving a fenced branch by paraphrase.** "Dependency injection for entity scripts" in new
  words is still `feature/dependency-injection-entity-script`. The live DI-flavored mechanisms are
  CkProvider + ContextOwner (root CLAUDE.md lingo table); anything beyond them needs the
  maintainer.
- **Building before measuring** on candidates 4/5. The define flip and the group prototype are
  cheap to *write* — that is exactly why the ruling is benchmark-first; an unmeasured "optimization"
  is a review rejection.
- **Treating `in_place_delete` as a work item.** It is settled design (DECISIONS.md §45 — the
  ungating was deliberate); the open part is only candidate 5's measurement. "Restoring the debug
  gate" is not a cleanup — it would revert a considered 2026 storage decision.
- **Landing debug tooling in CkFoundation runtime modules.** Overlay/inspector work belongs in
  CkGameplayDebugger (`ck-gameplaydebugger-extension`); only the analyzer (already a CkFoundation
  UncookedOnly module) and the AS checker (generator-adjacent) live here.
- **Presenting this portfolio as a roadmap.** Every entry is a candidate. When you pitch one,
  carry its evidence (the file:lines above), not its conclusion.
- **Skipping the ask.** If developing a candidate surfaces a genuine convention fork, add it to
  `.claude/reports/ADJUDICATIONS.md` instead of inventing policy (root non-negotiable #6).

## Provenance and maintenance

Authored 2026-07-02 (documentation campaign; CkFoundation HEAD `7330c1bab`). All counts and lines
below drift — re-verify before relying on them. Commands are Git Bash from
`d:\Repos\BusterBlock\Plugins\CkFoundation` (the Grep/Glob tools are blind under `Script/` — use
`rg --no-ignore`):

- Ruling + fence records: `.claude/reports/DECISIONS.md` §25 + §45 (`ADJUDICATIONS.md` A3 is a
  resolved tombstone pointing at §45).
- Branch list: `git for-each-ref --sort=-committerdate --format='%(refname:short) | %(committerdate:short)' refs/remotes/origin`
- Teardown TODOs: `rg --no-ignore -n "teardown is mid interaction" Source/CkInteraction` (expect 2)
  · `rg --no-ignore -n "bullet-proof way" Source/CkRelationship` (expect 3)
- GC census: `rg --no-ignore -c "TStrongObjectPtr<" Source/ --glob '*_Fragment.h'` (13 total) ·
  same for `TWeakObjectPtr<` (35) and `TObjectPtr<` (4) · tripwire:
  `rg --no-ignore -n "IsInGameThread" Source/CkCore/Public/CkCore/IO/CkDeferredAssetInit_AngelScript.cpp` (:521)
- Snapshot: `rg --no-ignore -c "CK_REGISTER_SNAPSHOTABLE\(" Source/ --glob '*.cpp'` (sum = 119) ·
  `rg --no-ignore -c "struct (CK\w+_API )?FFragment_" Source/ --glob '*_Fragment.h'` (sum = 295) ·
  policy doc at `Source/CkEcs/Public/CkEcs/Snapshot/CkSnapshot_Policy.h:9-19`
- Net-param copy: `rg --no-ignore -n "CK_DISABLE_NET_PARAM_COPY_PER_ENTITY" Source/` (6 Build.cs
  sites all `=0` + 1 consumer at `CkEntityLifetime_Utils.cpp:478`)
- Groups / deletion policy: `rg --no-ignore -n "static_assert" Source/CkThirdParty/Public/CkThirdParty/entt-3.16.0/src/entt/entity/group.hpp`
  (in-place-delete rejection at :697) · funnel at `Source/CkEcs/Public/CkEcs/Registry/CkRegistry.h:162` ·
  global trait at `Source/CkEcs/Public/CkEcs/Handle/CkHandle.h:71-77` — confirm it is still
  ungated via `grep -n "#if" Source/CkEcs/Public/CkEcs/Handle/CkHandle.h | head -1` (first hit :79,
  i.e. after the trait) · origin chain: `git log --format='%h %ad %s' --date=short -G component_traits -- Source/CkEcs/Public/CkEcs/Handle/CkHandle.h`
  (returns `6b54d2e38` relocate + `745507381` introduce-gated) and
  `git log --format='%h %ad %s' --date=short -G CK_DISABLE_ECS_HANDLE_DEBUGGING -- Source/CkEcs/Public/CkEcs/Handle/CkHandle.h`
  (returns `06938bba3` = the deliberate ungating, + `232ae5e06` param rename). `-S` cannot find the
  move/ungate commits — the string count never changed.
- AS surface: `ls Script/Generated/*.as | wc -l` (274) · emitted-namespace claim at
  `Script/CLAUDE.md:113` · commandlet scope in `CkAngelscriptGenerator_DriftCommandlet.h:11-20`
- Overlay providers: `ls ../CkGameplayDebugger/Source/CkEntityDebugOverlay/Private/Providers/ | wc -l`
  (38 files = 19 providers)
- Processors: `rg --no-ignore -c "CK_REGISTER_PROCESSOR\(" Source/ --glob '*.cpp'` (sum = 388)
