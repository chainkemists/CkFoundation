---
name: ck-failure-archaeology
description: "Use when checking whether a Ck failure, workaround, rejected design, or stalled branch has prior history before retrying, removing, or resurrecting it."
---

# ck-failure-archaeology — the chronicle of dead ends, reverts, and workarounds

## Overview

This is the Ck suite's incident chronicle: investigations, dead ends, rejected fixes, and reverts,
each reduced to SYMPTOM → ROOT CAUSE → EVIDENCE → STATUS → LESSON. Its job is to stop you from
re-running an experiment that already failed, deleting a workaround that is secretly load-bearing,
or "improving" a design whose shape is the scar tissue of a documented incident. Every SHA and
file:line below was re-verified against the repos on 2026-07-02.

## When NOT to use this skill

| You are… | Load instead |
|---|---|
| Triaging a live bug / build break / red test right now | `ck-debugging-playbook` (come back here if the symptom matches an entry) |
| Deciding what to build next | `ck-feature-frontier` |
| Asking why the architecture is shaped this way (invariants, not incidents) | `ckecs-architecture-contract` |
| Working the teardown/unbind lifecycle campaign | `ck-lifecycle-teardown-campaign` |

## How to re-verify an entry

Cwd = the BusterBlock superproject root (the plugins are git submodules there; under a different
host, substitute its plugin paths). Git Bash or PowerShell — commands are identical:

```powershell
git -C Plugins/CkFoundation show --stat --format='%h %ad %s' <sha>   # entries 1-10, 13
git -C Plugins/CkTests show --stat --format='%h %ad %s' <sha>         # entry 11
git -C Plugins/CkGameplayDebugger show --stat --format='%h %ad %s' <sha>  # entry 12
```

Tooling caveat: the Grep/Glob tools are blind under `Plugins/CkFoundation/Script`, `docs/`,
`Content/` (superproject `.ignore`). Use `rg --no-ignore -n` or Read with exact paths there.

---

## The chronicle

Lingo (Fragment, Processor, Signal, Handle, Request) is defined in the root CLAUDE.md. "EnTT" is
the vendored C++ ECS library under `Source/CkThirdParty/`. Status vocabulary: **fixed** /
**patched-symptom-only** / **open** / **abandoned**.

### 1. Packaged-only GC crash — the disregard-for-GC pool trap

- **SYMPTOM:** 0xC0000005 in a packaged Development client; editor/PIE never reproduces it.
- **ROOT CAUSE:** AngelScript `asset … of …` owners and AS CDOs are created during AS
  InitialCompile, *before* FEngineLoop closes the disregard-for-GC set (UE's boot-time set of
  permanent objects the GC never traverses) — so they live in a pool GC never walks.
  CkDeferredAssetInit's post-boundary refresh then attaches normal-pool objects under them
  (minted item-trait sub-objects, `assets::load`'d cooked meshes); the first real GC reclaims
  those out from under the untraversed owner → dangling pointer.
- **EVIDENCE:** `feb08ee94` (2026-06-02, "fix(gc): root AS disregard-for-GC refs so the verifiers
  pass in packaged builds") — the body is the full mechanism write-up. Diagnostics kept:
  `d77810096` — `Ck.Diag.DumpAngelscriptAssets` / `Ck.Diag.VerifyGCAssumptions`, live at
  `Source/CkCore/Public/CkCore/Object/CkObject_Utils.cpp:603,621`. Hardening: `a8a93baac`
  (2026-06-24) adds a game-thread tripwire ensure at the rooting entry (a *detection* tripwire,
  not the fix — see `.claude/reports/DECISIONS.md` #21).
- **STATUS:** fixed at root — a pre-GC (non-editor-only) pass feeds AS disregard objects through
  `CollectReferences`/`FSimpleReferenceProcessorBase` and `AddToRoot`s every referenced target
  failing the engine verifier's accept-test. Root-only by design (never unroots).
- **LESSON:** PIE cannot exercise disregard-set timing; an object created during AS InitialCompile
  has different GC semantics from the identical object created one frame later. Reproducer harness
  (per the commit body): cooked Development client + `gc.VerifyAssumptions` +
  `gc.CollectGarbageEveryFrame` soak.

### 2. "Incidentally alive" — replication driver reclaimed the moment the netdriver was absent

- **SYMPTOM:** standalone / `-nullrhi` runs destroy the PlayerController ~1.2s after possession.
- **ROOT CAUSE:** the per-entity replication-driver UObject had no strong GC root — every
  reference was weak or GC-invisible: `FCk_ReplicatedObjects::_ReplicatedObjects` held
  `TWeakObjectPtr`, the entity back-ref is a `TObjectPtr` inside an EnTT USTRUCT (**UE GC does not
  trace UPROPERTY refs in ECS fragments**), and the actor's `FSubObjectRegistry` is
  `FWeakObjectPtr`. A live netdriver's UNetConnection/UActorChannel/FObjectReplicator chain had
  been *incidentally* keeping it alive; without one, first GC sweep reclaimed it and
  `BeginDestroy` cascaded `Request_DestroyEntity` → `FProcessor_OwningActor_Destroy` → dead PC.
- **EVIDENCE:** `56b344310` (2026-05-28, "fix(CkEcs): root replicated-driver objects via
  TStrongObjectPtr in their fragment") — body quoted above almost verbatim.
- **STATUS:** fixed — fragment owns via `TArray<TStrongObjectPtr>` (non-UPROPERTY; UHT can't
  reflect it), weak-converted at the wire boundary. A BusterBlock-side 82-field fragment audit
  followed (host repo `docs/Fragment-UObjectRef-GC-Audit.md`).
- **LESSON:** for every UObject an ECS fragment touches, enumerate who *actually* roots it — then
  test with the netdriver absent. "It hasn't crashed" may mean "something unrelated is rooting it."

### 3. Typed GOAP WorldState — dead in two days

- **SYMPTOM:** GOAP (Goal-Oriented Action Planning, the CkGoap AI planner) blew up state-space
  memory/time on even modest economy plans.
- **ROOT CAUSE:** typed WorldState (int/enum values, GE/LE comparisons, Add/Sub effects) versus a
  *regressive* A* (plans backward from the goal): every numeric shift produced a distinct
  ConstraintSet, destroying state-space dedup.
- **EVIDENCE:** introduced inside the initial module commit `c3a39727c` (2026-04-15, "feat: add
  CkGoap module — GOAP planner built on CkAStar with regressive search") → reverted `1b39d1cbd`
  (2026-04-17, "refactor(goap): revert to classical boolean GOAP"). The revert body: typed state
  "fought regressive A* head-on … blowing up state-space dedup on even modest economy plans";
  replacement is "F.E.A.R.-style bool-only state: flat TStaticArray<uint8, 64>, single Set effect
  op, memcmp equality"; game code "projects [numeric thresholds] to booleans before writing to
  WorldState."
- **STATUS:** abandoned (the typed design). Boolean GOAP is the shipped design; the *next*
  structural change (moving A* from Action-entity to Planner-entity) was run as a staged,
  CTO-reviewed campaign instead of a big-bang commit — PR-B.1b Stage 0 `7ae532de4` (2026-05-22)
  through Stage 6 `aea047915` (2026-05-23), e.g. `997bcbb5c` "Stage 1 — disable-toggle pipeline
  gate (spec §3.3, CTO A4)".
- **LESSON:** regressive planners need a finite state alphabet — stress the planner with
  realistic plan sizes *before* building API surface on top. Do not re-propose numeric WorldState;
  the sanctioned pattern is game-side numeric→boolean projection.

### 4. The self-heal AS generator — four revisions, three dated incidents

Background: `Source/CkAngelscriptGenerator` regenerates `Script/Generated/*.as` accessor files
("canonicals") and can emit temporary `_StubRecovery_*` "sibling" files to heal a broken compile.
The full saga is in `Source/CkAngelscriptGenerator/Claude.md` (primary source; all claims below
re-read from it 2026-07-02).

- **SYMPTOM (serial):** (a) stale committed generated `.as` wedges the editor at launch behind the
  AS error modal; (b) 2026-06-11: self-heal reports success ("green toast") while the compile stays
  red; (c) 2026-06-12: two editor processes of the same project rewrite one generated file forever.
- **ROOT CAUSES:**
  - **Rev 9** (2026-05-11): descriptor-driven regen via a pre-approved engine-fork delegate broke
    the editor worse than the corruption — CDO defaults unreachable from descriptors, BP-derived
    classes missing, recovery loop death-spiraled (595 → 595 → 0 → 17 classes). Archived on
    `archive/rev9-as-attempt-2026-05-11`; the engine fork was reverted the same day.
  - **Rev 10** (empirical finding 2026-05-12): files written synchronously inside
    `OnAngelscriptReloadHadErrors` become part of the hot-reload checker thread's **mtime
    baseline** (engine `AngelscriptManager.cpp:2885` fires the `GetReloadHadErrors()` broadcast,
    delegate fetched at `:2883`; the checker thread is `StartHotReloadThread()` at `:578`, gated
    by `bUseHotReloadCheckerThread`) — so no recompile ever fires and the modal sits forever. Fix: defer
    writes to a modal-tick dispatcher, hard-capped at 3 cycles/session.
  - **Rev 11** (2026-06-11 wedge): per-signature error-text stubs cannot heal a stale canonical
    with mixed-type callers — same-arity ambiguity blocks every further overload. Fix
    `666488272` ("fix(selfheal): Rev 11 - stale-canonical quarantine escalation"): forensic-copy
    the canonical to `Saved/CkSelfHeal/Quarantine/`, DELETE it (it regenerates on next successful
    compile), resynthesize full shapes.
  - **Rev 12** (2026-06-12 incident): a live editor (BP entity script loaded → 9097 lines) and a
    headless compile-check boot (without it → 9072 lines) drove **496 mirror-image rewrites** of
    `BusterBlock_EntitySpawnParams.as` and **686 full AS reloads**. Compare-before-write cannot
    converge this — both processes are "right" relative to their own reflection view. Fix: OS
    file-lock single-writer ownership (`FCkAngelscriptGenerator_RegenOwnership`); secondaries go
    read-only.
  - Bonus defect: the original BP-class exclusion `e55fe07df` used
    `InClass->IsChildOf(UBlueprintGeneratedClass)` — **never true** (a BP class is an *instance*
    of `UBlueprintGeneratedClass`, not a subclass), i.e. a shipped no-op filter. Rev 12 fix:
    `Cast<UBlueprintGeneratedClass>(InClass)`, pinned by unit test
    `CkAngelscriptGenerator.UnitTests.ParamsGenerator.ClassFilter_ExcludesBlueprintGeneratedClasses`.
- **EVIDENCE:** `Source/CkAngelscriptGenerator/Claude.md` (Rev history; incident numbers at its
  "Cross-process single-writer ownership" section); commits `666488272`, `e55fe07df`,
  `ad1a67b16` (2026-05-12 drift commandlet).
- **STATUS:** fixed — Rev 12 shipping; drift commandlet guards CI.
- **LESSON:** self-healing file generators need (a) OS-level single-writer ownership, (b) write
  deferral past the watcher's baseline scan, (c) quarantine-escalation when per-error healing is
  provably insufficient. And `Cast<>` ≠ `IsChildOf<>`: instance-of vs subclass-of confusion passes
  review and compiles clean. Never add a `Script/Generated` write path outside the ownership
  funnel — that re-opens the two-editor ping-pong.

### 5. dtCrowd — wiped, then re-derived line-by-line

- **SYMPTOM:** first, dtCrowd (Detour's crowd/steering library inside UE's Navmesh module):
  "Frozen agents, MoveTargetDirty races, regen-rebuild dance; corridor-optimize fights local
  avoidance. Endless edge cases" (verdict verbatim). Then the replacement's separation-only force
  avoidance vibrated and orbited goals.
- **ROOT CAUSE (of the second failure):** pure force-based separation without velocity sampling.
  (Session-lore addendum, INFERRED — confirmed only in operator memory, not repo docs: the orbit
  component was min turn radius `MaxSpeed/MaxTurnRate` exceeding arrival radius.)
- **EVIDENCE:** `Source/CkNavigation/PLAN.md:29` (the Wipe verdict);
  `Source/CkNavigation/Plan/Gate_03_Separation_Hybrid_Plan.md:7` — the pivot plan explicitly
  mirrors `DetourCrowd.cpp:integrate()`, `DetourObstacleAvoidance.cpp:processSample()`, and
  `DetourCrowd.cpp:updateStepMove() 1601-1662`, i.e. **they wiped dtCrowd, then re-derived
  dtCrowd's math inside their ECS** (with the reference source cited line-by-line). Phase 1
  landed on dev: `639c1c3a3` (2026-05-02, AccelClamp).
- **STATUS:** open — live campaign. `PLAN.md` says `window: 8 days`, `last_updated: 2026-04-29`,
  Gates 2–7 "⏳ Pending" — **the gate table is stale**: Gate-2 work landed 2026-05-01
  (`a6621f5a8`) and 10 more commits touched `Source/CkCrowd` in June alone (e.g. `db6eb1990`
  "coincident agents separate instead of co-sliding +X"). The 8-day window is 2+ months and
  counting.
- **LESSON:** "wipe the library" usually becomes "re-implement the library's math under your own
  architecture" — budget for that, and keep the reference source cited line-by-line (they did).
  An executive-index plan doc rots the moment execution outpaces bookkeeping; trust dev history
  over `PLAN.md` status tables.

### 6. OwningClientAuthoritative state machines — a two-month replication convergence

- **SYMPTOM:** replicated state machines (SMs) flaked: states double-constructed, listen-server
  hosts misclassified as non-owners, AutoStart echoes tripping authority gates, orphaned states
  after snapshot restore.
- **ROOT CAUSE(S):** not one — serially discovered authority special-cases: run-status relay never
  wired (`d5772f220` "wire up dead Server_PushRunStatus"), listen-server host ownership
  (`1a0bcffc3`), previous state freed before the transition commits (`1af2514fe`), duplicate
  next-state construction on PendingExit (`0096733a2`, 2026-04-25), sub-SM net identity +
  AutoStart echo (`d196fd716`), a transition-authority gate that had to be introduced then
  narrowed twice in one day (`8d83645e0` → `f4cfd864c` → `520ca5590`, all 2026-06-25, plus
  `eb30e69d6` probing the root SM for listen-host ownership), and snapshot-restore orphans
  (`4b9d5f0f3`, `13d7cf6cd`, `a13c56972`, 2026-06-22..24).
- **EVIDENCE:** 66 commits touch `Source/CkStateMachine` in 2026-05-01→06-25 (verified:
  `git -C Plugins/CkFoundation log --since=2026-05-01 --until=2026-06-26 -- Source/CkStateMachine`
  → 66). Early symptom-gate: `098c30b73` (2026-05-23, "drop non-authority local requests with
  ensure") — retained. Test plumbing arrived mid-campaign: `5638de2ba` (2026-05-24,
  fake-fingerprint injection for replication tests), de-flake `c543537a2` (2026-06-07).
- **STATUS:** open/converging — fixes were still landing 2026-06-25 (five days before this
  chronicle's verification date); scope per fix has narrowed but nobody has declared it closed.
- **LESSON:** client-authoritative distributed FSMs accrete authority special-cases one at a time
  (listen-host, echo, sub-SM, restore). Build the fingerprint/replay test harness *first* — here
  it arrived three weeks in. When you hit an SM replication flake, check this commit family before
  assuming a new bug.

### 7. Cue subsystem — three architectures in seven months

- **SYMPTOM:** cues (fire-and-forget gameplay effects) silently failing: wrong client, no executor
  yet, packaged-build discovery misses.
- **ROOT CAUSE:** the "discover an executor" architecture mismatched replication reality; each
  patch treated a symptom — discovery fallback for packaged builds (`362e8917a`, 2025-09-28) →
  executor pattern (`fa21e647a`, 2025-10-14) → retries (`a75009d85`), queueing (`9eb7534ba`),
  ownership moved PlayerController→PlayerState (`bb2736858`), RPC routing round-robin→owned
  executor (`2d5255806`), and a growing execution-policy enum (`2a1ebe747`, `9981d63ed`,
  `da70742dd`, `bf812ab14` — four policy commits on a single day, 2025-11-10 — plus ServerAndSelf
  `6f0e1587d`, 2026-01-15).
- **EVIDENCE:** the SHAs above; resolution `0d81451d7` (2026-04-03, "refactor: migrate cue
  subsystems to ActorRelay pattern"; file split `4eef0d681`, 2026-03-22, preceded it).
- **STATUS:** fixed by rearchitecture — ActorRelay channels (CkActorRelay: an explicit channel
  entity owned by a replicated actor) now answer "who owns the replicated thing."
- **LESSON:** when a subsystem's policy enum keeps growing, the architecture is wrong — stop
  adding enum values and ask what invariant is missing. ActorRelay ownership is the house answer
  to replicated-thing ownership; new "who executes this on which client" designs should start
  there, not at discovery/round-robin.

### 8. Signal `in_place_delete` — the workaround that became the architecture

- **SYMPTOM (2023):** signal bindings randomly disconnected.
- **ROOT CAUSE:** signal fragments own connections (they have a destructor) but lacked the full
  copy/move set; EnTT storage relocation copied/moved them and severed the connections.
- **EVIDENCE:** `2c8319c1c` (2023-11-09, "fix: fixed issue where Signals would randomly
  disconnect"); body verbatim: "A proper fix is upcoming, until then we turn on the pointer
  stability guarantee for the fragments so that they are not copied/moved at all." That guarantee
  is still the design today: `static constexpr auto in_place_delete = true;` at
  `Source/CkEcs/Public/CkEcs/Signal/CkSignal_Fragment.h:44` and `:99`.
- **STATUS: resolved-by-design-decision.** The "proper fix" the commit promised never landed as
  such; instead the guarantee graduated to framework-wide design: `745507381` (2024-03-07)
  introduced a global `entt::component_traits<Type>` specialization (debug-gated at birth), and
  **`06938bba3` (2026-02-17, "feat: fragments are always pointer stable") deliberately ungated
  it** — recorded in `.claude/reports/DECISIONS.md` §45 (ADJUDICATIONS A3 is a resolved tombstone
  pointing there). Treat pointer stability as a load-bearing invariant; the lifecycle campaign
  treats it as fenced context, not a target.
- **LESSON:** any EnTT fragment owning a resource needs `in_place_delete` or a full rule-of-five —
  storage relocation is invisible until it isn't. Do NOT remove the flag as a "cleanup"; that
  reintroduces the 2023 bug. When a workaround survives 3 years, record whether it graduated to
  design — this one did, explicitly (`06938bba3`, DECISIONS.md §45).

### 9. `FireIfPayloadInFlightThisFrame` — a fossilized bug class

- **SYMPTOM (2023):** promise-style signals ("Futures") intermittently never fired.
- **ROOT CAUSE:** payload fired before the listener bound, and the then-default policy treated
  prior-frame payloads as stale — a bind-after-fire race.
- **EVIDENCE:** `7f38dad33` (2023-11-08, "fix, feat: Signals now have a
  FireIfPayloadInFlightThisFrame policy as one of the binding policies"); body: promise signals
  must use `FireIfPayloadInFlight` since "the payload could be from a previous frame(s) …
  This fixes the issue in game where some Futures would not fire due to the Payload being
  considered 'stale'." Enum renamed from `ECk_Signal_PayloadInFlight` to
  `ECk_Signal_BindingPolicy` in `42f7b9602` (2023-09-11 — the concept predates the name). Current
  wiring: `CK_SIGNAL_BIND_PROMISE` binds `FireIfPayloadInFlight + PostFire::Unbind`
  (`Source/CkEcs/Public/CkEcs/Signal/CkSignal_Macros.h:44-45`);
  `CK_SIGNAL_BIND_REQUEST_FULFILLED` binds `IgnorePayloadInFlight + Unbind` (`:47-49`).
- **STATUS:** fixed — and each of the three `ECk_Signal_BindingPolicy` values encodes a bug class
  the codebase actually hit.
- **LESSON:** read the binding-policy enum as fossilized incident history. When a promise
  "randomly" doesn't resolve, check which policy the bind used *first* — before suspecting the
  producer.

### 10. Attribute clamp revert — the bug kept on purpose, decided in a lost Slack thread

- **SYMPTOM:** two mutually exclusive correctness expectations on one clamp. Fix `bf56e7582`
  (2024-03-07) stopped clamping the attribute base value (a Revokable modifier had been unable to
  push the final value past an unexpected minimum). The fix then caused: overriding adds an
  irrevokable modifier → base values go beyond min/max, "which is unexpected."
- **ROOT CAUSE:** an unstated invariant — nobody had decided whether base-value clamping is a
  guarantee or an obstacle; both behaviors were "correct" to someone.
- **EVIDENCE:** revert `817c5e9e8` (2024-03-14). Body, verbatim: "Reverting this _does_ result in
  the original problem resurfacing. We'll address that problem when it becomes a problem." The
  full design rationale lives in a linked Slack thread
  (`chainkemists.slack.com/archives/C06GC7FQPH6/p1710367008359489`) — **honest gap:** that thread
  is the only record of the actual design call; only the commit-body tl;dr survives in-repo.
- **STATUS:** open by explicit choice, 2+ years. The chosen failure mode: Revokable-modifier
  clamping bug kept; override/irrevokable overshoot prevented.
- **LESSON:** when a fix and its revert are both "correct," the real bug is an unstated invariant
  — name which failure mode you chose to keep, *in the commit body* (they did), and put the design
  rationale somewhere durable (they did not — it's in Slack).

### 11. CkTests — renaming a test class broke the level and the generated registry

- **SYMPTOM:** after renaming two AS autotest files + classes, AS compilation failed plugin-wide.
- **ROOT CAUSE:** the placed wrapper-actor blueprints in `AutoTests_CkTests_Level.umap` still
  referenced the old C++ class paths, so the asset-registry-generated `CkTestsAssets.as` emitted
  soft refs to the old names → AS compile failure. Test class names are load-bearing in *binary
  level assets* and *generated registries*, not just in the `.as` source.
- **EVIDENCE:** CkTests repo — revert `604a2d4` (2026-05-26, "revert(CkEntityTag): restore old
  test names + classes to keep level assets resolvable"), reverting rename `2be6c3f` (same day;
  the renames chased CkFoundation contract change `68e828b56`). The revert body documents the
  whole chain and leaves the new no-op contract as inline NOTEs in the old-named files.
- **STATUS:** fixed by revert — rename abandoned; the doc-vs-name mismatch was left for a later
  in-editor cleanup (repoint the two placed actor blueprints, then rename).
- **LESSON:** never rename a placed test class as a pure text edit. Order of operations: repoint
  the placed level actors in-editor (or delete + let the populator respawn), *then* rename the
  class. Details of that pipeline: load `ck-tests-authoring-and-running`.

### 12. CkGameplayDebugger — the legacy generation, frozen since 2024-04

- **SYMPTOM (task-shaped):** you're about to extend `Source/CkGameplayDebugger` (the UE
  GameplayDebugger category + data-asset DebugProfiles + Blueprint submenus) or its `Content/` BP
  assets.
- **ROOT CAUSE (why you shouldn't):** that module is the 2023-era architecture, superseded twice —
  first by a Cog-based experiment (`88cd131`, 2024-04-22, "wip, feat: First pass of integrating
  Cog as EcsDebugger"; Cog since removed entirely), then by the current standalone Slate
  editor-tab debugger suite + the `CkEntityDebugOverlay` runtime overlay (scaffold `b324634`
  2026-06-08 → `d607751` 2026-06-26, all in the same plugin).
- **EVIDENCE (CkGameplayDebugger repo):** last feature commit on the legacy module: `9c4a396`
  (2024-04-01, selected-actor title bar). Commits touching `Source/CkGameplayDebugger` by year:
  2023 = 25, 2024 = 13, 2025 = 4, 2026 = 1 — and every 2025/2026 commit is a build fix
  ("fix: Shipping build fix", "fix: general build fixes…", "fix: reacting to interface changes of
  ranges", non-unity include fix, file-rename chore).
- **STATUS:** frozen / maintenance-only. Per `.claude/reports/DECISIONS.md` #18 it is documented
  as maintenance-only but NOT proclaimed deprecated — that is the maintainer's call.
- **LESSON:** new debugger work goes to the Slate suite or the overlay — load
  `ck-gameplaydebugger-extension`. Touch the legacy module only to keep it compiling.

### 13. EnTT upgrades — cheap code, lagging docs

- **SYMPTOM:** feared big-bang upgrades of the vendored ECS library; actual breakage ≈ API renames.
- **EVIDENCE:** `631e43545` (2025-05-01, "feat: Upgrade Entt version from 3.12.2 to 3.15.0"),
  `8d6fec7ef` (2026-02-05, "feat: Upgrade Entt from 3.15 to 3.16"), and the one verified breakage
  fix `bf1c5d76c` (2026-03-27, "fix: update EnTT pool API from type() to info() for 3.16"). No
  other upgrade-breakage commits surfaced by `git log -i --grep entt` (checked 2026-07-02; the
  other hits are CkSnapshot feature work).
- **STATUS:** fixed/done. Residue: version strings in docs rot — the BusterBlock superproject
  CLAUDE.md still said "EnTT 3.15.0" as of 2026-07-02; the root doctrine here is correct (3.16.0).
- **LESSON:** EnTT bumps are low-risk *here* because usage funnels through Ck wrappers; the real
  cost is every doc/memory hardcoding the version. After a bump, grep docs for the old version
  string.

---

## Stalled-branch registry — DO-NOT-RESURRECT

Maintainer ruling 2026-07-02 (recorded in `.claude/reports/DECISIONS.md` #25): stalled/outdated
remote branches are **do-not-resurrect**. If a listed idea is wanted again, it goes through
`ck-feature-frontier` as new work — do not rebase or cherry-pick these lines. All in the
CkFoundation repo; tip dates/subjects re-verified 2026-07-02 via
`git -C Plugins/CkFoundation log -2 origin/<branch>`.

| Branch (`origin/…`) | Last commit | What it attempted (tip subject, verbatim or tight paraphrase) |
|---|---|---|
| `feature/dependency-injection-entity-script` | 2026-05-26 | "feat, wip: Dependency Injection for entty script" [sic] — parked WIP, the youngest fossil |
| `feature/entity-script-as-script-struct` | 2026-04-02 | "refactor: migrate EntityScript spawn params from UUserDefinedStruct to UScriptStruct" — INFERRED superseded by the AS-generator spawn-params pipeline |
| `upgrade/ue5.7` | 2026-03-27 | "fix: Update InstancedStruct includes for UE 5.7 (StructUtils path change)" — parked engine line; its tip has a dev twin (`565e85ae5` IS on dev), i.e. pieces were cherry-picked back, the line itself is parked |
| `dev-bb-5.7` | 2026-04-06 | UE 5.7 compatibility line (attribute/utils fixes for 5.7) — parked with `upgrade/ue5.7` |
| `dev-bb` | 2026-03-27 | "feat: register attribute handle type traits for handle-level concept enforcement" — the pre-5.7 BB-integration line, parked alongside the 5.7 pair |
| `backup/before-reverting-f8a3a55` | 2024-01-11 | insurance snapshot taken before a revert ("added Processors for all Attributes' OverrideBaseValue processors") — a fossil, not work |
| `backup/pre-exporter-rewrite-2026-05-28` | 2026-05-22 | insurance snapshot before the asset-exporter rewrite — a fossil, not work |
| `bugfix/test/investigate-cheat-in-build` | 2025-02-19 | "test: Traces for cheats in build" — investigation branch, outcome unrecorded |
| `feature/entity-replication-channel` | 2025-01-23 | "Ecs Replication Channel (Actors) tied to the Transient entity … 'dangling' entity" — superseded by ActorRelay channels (INFERRED: same problem shape; ActorRelay is the shipped answer, cf. entry 7) |
| `feature/flow-graph-module` | 2025-01-04 | "feat, wip: Initial commit for the flow graph module" — abandoned module skeleton |
| `feature/message-without-name` | 2025-05-26 | "wip, refactor: Working on having messages no longer require a gameplay tag" — abandoned experiment |
| `feature/editor-time-construction-scripts` | 2025-06-16 | "On dev: Running construction scripts at editor time (there is another stash too, don't forget)" — stalled; per its own subject, part of the state is in a lost stash |
| `feature/ability-traits` | 2024-11-30 | "feat, wip: Started working on Ability Traits that compose an ability script." — abandoned |
| `feature/modular-traits` | 2024-05-14 | "wip, feat: Added new ModularTrait module w/ Conditions BP" — abandoned |
| `shelf/dummy-changes-to-debug-networking-asserts` | 2023-10-03 | "Several dummy changes to try and narrow down why some networking asserts are happening" — debugging shelf, oldest fossil |

**Dead pointer in live code:** `Source/CkEcs/Public/CkEcs/Registry/CkRegistry.h:289` —
"`// TODO: exposing the storage like this is temporary - see branch
feature/registry-handle-storage-support for what we really want to do`" — that branch **does not
exist on origin** (verified 2026-07-02: `git -C Plugins/CkFoundation branch -r --list
'*registry-handle*'` → empty). Treat the comment as intent-without-a-design; do not go hunting
for the branch.

Note: many *other* undeleted remote branches are NOT stalled — they landed on dev via
rebase/cherry-pick and just weren't pruned (twin subjects on dev). Check
`git -C Plugins/CkFoundation log --grep "<tip subject>" origin/dev` before classifying a branch
as dead.

## Common mistakes when using this chronicle

- **Deleting a "temporary" workaround because the commit called it temporary.** `in_place_delete`
  (entry 8; settled design per DECISIONS.md §45) and the FireIfPayloadInFlight policies (entry 9)
  are load-bearing. Check DECISIONS.md/ADJUDICATIONS.md before removing anything a commit body
  apologized for.
- **Trusting a plan doc's status table over git.** `CkNavigation/PLAN.md` has said "Gates 2–7
  Pending" since 2026-04-29 while the work landed (entry 5). Dev history wins.
- **Re-proposing a dead design without new evidence.** Typed WorldState (entry 3), executor
  discovery (entry 7), descriptor-driven AS regen (entry 4 Rev 9) each died for a mechanism, not
  taste. To re-open one, first state what changed about the mechanism.
- **Citing this file without re-verifying.** Entries are snapshots of 2026-07-02; a later fix may
  have closed an "open" entry. Run the `git show` line before repeating a STATUS.

## Provenance and maintenance

- Campaign date: **2026-07-02**. Verified at heads: CkFoundation `7330c1bab`, CkTests `b89f110`,
  CkGameplayDebugger `d607751` (all contained in their `origin/dev`).
- Every SHA above re-verified via `git -C Plugins/<repo> show -s --format='%h %ad %s' <sha>`;
  subjects quoted exactly. File:line claims read from disk the same day.
- Re-verification one-liners for the volatile facts:
  - Entry statuses / new fixes since: `git -C Plugins/CkFoundation log --oneline --since=2026-07-02 -- Source/<Module>`
  - `in_place_delete` still present: `rg -n 'in_place_delete' Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Signal/CkSignal_Fragment.h`
  - Dead branch pointer still in code: `rg -n 'registry-handle-storage-support' Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Registry/CkRegistry.h`
  - Branch registry still accurate: `git -C Plugins/CkFoundation for-each-ref --sort=-committerdate --format='%(refname:short) %(committerdate:short)' refs/remotes`
  - PLAN.md staleness: compare `last_updated` in `Plugins/CkFoundation/Source/CkNavigation/PLAN.md` against `git -C Plugins/CkFoundation log -1 --format=%ad -- Source/CkCrowd`
  - SM campaign count: `git -C Plugins/CkFoundation log --oneline --since=2026-05-01 --until=2026-06-26 -- Source/CkStateMachine | wc -l` (66 at verification)
  - A3 is resolved (DECISIONS.md §45); for the still-open items (A1/A2/A4) read `.claude/reports/ADJUDICATIONS.md` (an item may have moved to DECISIONS.md with a ruling)
- Adding an entry: keep the strict format (SYMPTOM → ROOT CAUSE → EVIDENCE (SHA/file:line) →
  STATUS → LESSON), quote commit subjects exactly, and mark anything you could not re-confirm
  INFERRED — a wrong SHA in this file poisons every future search that trusts it.
