# CkSnapshot v3 ergonomics — mission brief (PROMPT.md)

**You are the executor (Opus-class).** This package was planned by a Fable session on 2026-07-14 with full
research; execute, do not design. When reality diverges from a step: STOP, record the divergence in
`PROGRESS.md` → Blockers, end the session. Every design decision is already made here.

## Mission

Five independently-shippable phases improving the v3 save/load authoring surface, **one build per phase**
(builds cost ~15 min — never build mid-phase):

| Phase | One line | Repos touched | Risk |
|---|---|---|---|
| 1 | Round-trip test harness (FIRST, per Adam) + `ck::StaticCast`→utils-`Cast` sweep + Timer Jump absolute mode | CkTests + CkFoundation | low |
| 2 | Rename bundle: persistence vocabulary (absorbs parity campaign PHASE_6 6A+6B) + `FHandler` slot renames + named `Register_*` entry points | CkFoundation | low (mechanical) |
| 3 | Produce symmetry, class (a): 7 trivially-symmetric features' wire builders consume the registered `Produce` | CkFoundation | medium |
| 4 | Produce symmetry, class (b): record-aggregated features (attributes ×5 kinds, EntityCollection, Inventory ×2) fold the child-keyed `Produce` | CkFoundation | medium-high |
| 5 | Relocate persistence machinery to `CkEcs/Persistence/` (header split; Net keeps the wire plumbing) — LAST on purpose: tip-of-stack, droppable in review; does not depend on Phase 4 | CkFoundation | low (mechanical, wide) |

Adam may stop the campaign after ANY phase — each ends with a green gate and clean trees.

## Relationship to the in-flight `saveload-v3-parity` campaign (CRITICAL)

`docs/campaigns/saveload-v3-parity/` has its own executor. Its Phases 0–5 must be **committed** before this
campaign starts (entry criteria, Phase 1). Its **PHASE_6 (6A vocab rename + 6B T_Policy deletion) is ABSORBED
into this campaign's Phase 2** — the parity executor must NOT run it. At Phase 2 start, add one line to
`saveload-v3-parity/PROGRESS.md` phase-status rows 6A/6B: "ABSORBED by saveload-v3-ergonomics Phase 2".
Parity PHASE_7/VALIDATION remains the parity campaign's own.

## Chosen approach (settled — do not revisit)

- **Harness** = a `CKTESTS_API` latent-command composition layer (`ck::auto_test::snapshot`) over the existing
  `CkNetAutomation_Common` primitives — NOT a new test framework, NOT `DEFINE_SPEC`. ~21 existing tests
  hand-roll the identical StartPIE→save→load→wait→assert skeleton; the harness factors it. Proof = convert
  `Ck.Snapshot.Parity.Timer_MPReload` to the harness (same test name, same assertions).
- **Renames** = parity PHASE_6's table verbatim, PLUS `FHandler::Apply`→`NetApply`, `Remove`→`NetRemove`
  (bare "Apply" is net-coupled despite the generic name — the TagSet trap), PLUS four named registration
  entry points that make participation compile-visible. `Produce` and `HydrationApply` KEEP their names
  (`Produce` must stay transport-neutral for Phases 3–4; `HydrationApply` matches the existing
  PendingHydration/Hydration_Dispatch vocabulary family).
- **Symmetry** = unify the PROJECTION, not the transport write: a new `UCk_Utils_Net_UE::TryProduce<T>`
  helper resolves the registered `Produce` and returns the typed payload; each wire site keeps its own
  container-write mechanics (self-resident vs owner container, value vs mutator overload) and consumes the
  helper's result instead of hand-building the struct. Class (b) features fold the child-keyed `Produce`
  into the owner container.

## Rejected approaches (kill reasons — never resurrect)

- **Relocating the registry MID-campaign or as a whole-file move** — killed (parity PHASE_6 rule 3;
  56 include lines / 47 files of churn on an unpushed stack). The relocation DOES happen, but only as the
  header-SPLIT version, only as the FINAL phase (Adam ruling 2026-07-14 — see PHASE_5.md). Do not
  front-load it, and never move the FastArray wire types out of `Net/`.
- **Separate save registry** — re-creates the two-pipeline divergence the whole v3 design exists to kill.
- **Typed-thunk `ProduceTyped` storage in the registry** (avoid FInstancedStruct boxing on the wire path) —
  killed: wire builds are `MarkedDirtyBy`-gated (per-change, not per-tick); boxing cost is negligible; the
  type-erased `Produce` + `TryProduce<T>` unwrap is simpler and keeps one registration shape.
- **Produce symmetry for StateMachine / RenderTarget** — killed: both are class (c) instruction/delta
  streams where `Produce` DELIBERATELY emits a different canonical snapshot than the wire
  (SM: wire = sequenced transition-event ring, Produce = synthetic single-event
  `CkStateMachine_Replication.cpp:409-413` comment; RenderTarget: wire = owner-keyed draw-batch rings,
  Produce = child-keyed `AuthoredLog` slice). Do not touch either in Phases 3–4.
- **`DEFINE_SPEC`-based harness rework** — killed: every existing snapshot test is
  `IMPLEMENT_SIMPLE_AUTOMATION_TEST` + latent commands; a second idiom in the same suite is churn.
- **NumCycles=1 harness default** — killed: `CkSnapshot/Claude.md` §5 mandates two-cycle pins (idempotency).
  Default is 2; a cycle-2 failure on a previously-green test is a REAL stacking defect → STOP + Blocker,
  do not paper over by dropping to 1 cycle.

## THE TEST GATE (every phase — the no-regression contract)

All builds/tests via **UnrealToolbox** from the BB root; **editor must be CLOSED** (a PreToolUse hook blocks
builds while UnrealEditor runs):

```powershell
D:\Repositories\CkRepos\BusterBlock\CkAuto\UnrealToolbox.exe --build --target Editor --config Development --output CkAuto\logs\erg-build-p<N>.log
D:\Repositories\CkRepos\BusterBlock\CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.Snapshot" --output CkAuto\logs\erg-test-snapshot-p<N>.log
D:\Repositories\CkRepos\BusterBlock\CkAuto\UnrealToolbox.exe --test --test-pattern "Ck.Net" --output CkAuto\logs\erg-test-net-p<N>.log
```

Rules (violating any invalidates a "gate green" claim):
1. **Pattern gotcha (verified):** `--test-pattern` splits on `.` into case-insensitive SUBSTRING tokens — no
   glob. `"Ck.*.Net"` matches NOTHING. Use `"Ck.Net"` (matches all `Ck.*.Net.*` plus ~11 incidental
   AutoTests — harmless; measure deltas against the identical pattern).
2. **`--discover-fresh`** on the first `--test` after adding/removing any test without an intervening
   `--build` — the toolbox caches its test list and silently skips unknown tests.
3. **Baseline discipline:** Phase 1 records exact pass/fail counts + failing NAMES per pattern into
   PROGRESS.md (planner expectation from parity PROGRESS: Ck.Snapshot **30/30**, Ck.Net **90/90** — RECORD
   FRESH, do not assume). Every later phase reports the delta against those names.
4. **Read the verdict from the `--output` log file** (result summary + `Error:` lines naming your files) —
   a toolbox "completed" notification is a proxy, not ground truth.
5. If you edited source after the last build, any prior test run is STALE — rebuild before gating.

## Repo / branch / sibling-session state (verify at every session start)

- CkFoundation submodule: branch `feature/save-load-improvements` (~83+ ahead of origin/dev, unpushed).
  CkTests submodule: same branch name. **Do NOT push anything. No `Co-Authored-By` lines.**
- A sibling session (the parity executor) works in this same worktree family. **Stage ONLY files you
  changed, by name — never `git add <dir>`.** If `git status` shows dirty files you did not author at
  session start → STOP unless PROGRESS.md of the parity campaign shows its Phase 5 committed and the trees
  clean; record any unexplained dirt as a Blocker.
- `.gitignore` blanket-ignores `*.md` — campaign docs are staged with `git add -f`.
- **CkTests commits must never be merged/pushed ahead of the CkFoundation commits they test.**
- Grep/Glob tools can false-empty under these plugins — zero matches ⇒ re-check with `rg --no-ignore`.

## Skills to load, and when

| When | Skill |
|---|---|
| Session start, every session | `ck-change-control`, `ck-tests-authoring-and-running` |
| Phase 1 (harness authoring) | (the two above suffice — mimic existing tests) |
| Phase 2 (registry/macro churn) | `ck-macros-and-codegen` |
| Phases 3–4 (replication path) | `ckecs-architecture-contract` |
| Any build/UHT/linker failure | `ck-debugging-playbook` |

House style (root `CLAUDE.md`) is mandatory: trailing returns (except UFUNCTION decls), `CK_ENSURE_IF_NOT`,
`NOT` macro, `_Member`/`In*` naming, no anonymous namespaces in NEW shared code (filename-derived or
`ck::auto_test::snapshot` namespaces; existing per-test anon-namespace helper blocks in CkTests test .cpp
files are established convention and stay), `MoveTemp`, fmt-style logging.
**NEW house ruling (Adam, 2026-07-14): in feature/handler code use the feature utils' public `Cast`
(generated by `CK_DEFINE_CPP_CASTCHECKED_TYPESAFE`), never raw `ck::StaticCast<FCk_Handle_X>`.**

## File inventory (why each matters)

### CkFoundation — registry core (Phase 2)
- `Source/CkEcs/Public/CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h` — FHandler
  (4 slots: Apply/Remove/HydrationApply/Produce, lines 58-153), registry class, `ECk_RepFragment_ApplyResult`
  (48-52), `FFragment_PendingHydration` (34-41), FastArray types (158-226). The rename epicenter.
- `…/CkReplicatedFragmentContainer.inl.h` — `RegisterLazyTyped<T>` body; named entry points go beside it.
- `…/CkReplicatedFragmentContainer.cpp` — registry storage/methods; FastArray callbacks call `Resolve` at
  162/192/218 (slot presence checks → rename to NetApply/NetRemove).
- `…/CkReplicatedFragmentContainer_Processor.{h,cpp}` — net dispatcher invokes `Remove` at cpp:49, `Apply`
  at cpp:73; `ck::persistence_apply::ApplyOne` invokes `HydrationApply` at cpp:132.
- `Source/CkEcs/Public/CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.cpp:104` —
  `Resolve` + `Apply`-presence check (rename site). `FindEntry` at 469-476, `MarkFragmentDirty` at 459-465.
- `Source/CkEcs/Public/CkEcs/Net/CkNet_Utils.h` — `TryUpdateContainerFragment` value overload def 484-507,
  mutator overload def 511-542. `TryProduce<T>` (Phase 3) goes beside them.

### CkFoundation — the 24 registrars (Phases 2–4)
Full slot-usage census is in this campaign's planning record; the files (all `RegisterLazyTyped` callers):
MontagePlayer/AnimPlan (CkAnimation), Byte/Integer(+Refill)/Float(+Refill)/Vector/Rotator attribute
`_Fragment.cpp`s (CkAttribute; shared templates in `CkAttribute_RestorePersistence.h` +
`CkAttribute_RefillPersistence.h`), Velocity/Acceleration (CkPhysics), EntityTag, GeometryCollectionOwner
(CkChaos, net-only), EntityCollection, StateMachine `Net/CkStateMachine_Replication.cpp` (2 types),
Dynamic `CkDynamic_Fragment.cpp` + `CkDynamic_Module.cpp:22` (RegisterFallback — stays), EntityScript
`CkEntityScript_SaveFields.cpp`, Transform `CkTransform_Fragment.cpp` (3 net-only types), Timer, Grid
Occupancy, Inventory Spatial/DataOnly, RenderTarget `Net/CkRenderTarget_Replication.cpp`, Team, Player,
TagSet. **No registrar sets `.Remove` today.**

### CkFoundation — wire builders (Phases 3–4; classification verified 2026-07-14)
Class (a) — trivial symmetry: Velocity `CkVelocity_Processor.cpp:320-331`, Acceleration
`CkAcceleration_Processor.cpp:253-264`, TagSet `CkTagSet_Processor.cpp:116-131`, MontagePlayer
`CkMontagePlayer_Processor.cpp:419-433` (direct `Driver->SetFragmentData`, resolves driver via
`FFragment_ContainerRef_MontagePlayer` — keep that resolution), Grid Occupancy
`Ck2dGridOccupancy_Processor.cpp:87-116`, Team `CkTeam_Utils.cpp:55,102-103` (imperative — no Replicate
processor exists), Player `CkPlayer_Utils.cpp:51,107-108` (imperative).
Class (b) — record-aggregated: Attributes `CkAttribute_Processor.inl.h:221-255`
(`TProcessor_Attribute_Replicate`, owner-keyed find-or-emplace by (name,component), one component per
call), EntityCollection `CkEntityCollection_Processor.cpp:307-333` (per-child find-or-emplace by
CollectionName into owner), Inventory Spatial `CkInventory_Spatial_Processor.cpp:60-90` / DataOnly
`CkInventory_DataOnly_Processor.cpp:32-57` (full-replace of owner container from inventory record).
Class (c) — FENCED OUT: StateMachine, RenderTarget.

### CkFoundation — Timer (Phase 1)
- `Source/CkTimer/Public/CkTimer/CkTimer_Fragment_Data.h:185-203` — `FCk_Request_Timer_Jump` (gains `_JumpMode`).
- `Source/CkTimer/Public/CkTimer/CkTimer_Processor.cpp:199-240` — Jump request handler (Tick/Consume by
  Params direction; signals at 233+).
- `Source/CkTimer/Public/CkTimer/CkTimer_Fragment.cpp:15-118` — save handler; HydrationApply's relative-jump
  baseline math (55-80) gets replaced by one absolute Jump; `ck::StaticCast` at 55/105 → utils `Cast`.
- `Source/CkCore/Public/CkCore/Enums/CkEnums.h:252-258` — `ECk_RelativeAbsolute` (exists; has formatter).

### CkTests (Phase 1)
- `Source/CkTests/Public/CkTests/Net/CkNetAutomation_Common.h` — the latent-command primitives the harness
  composes (`FCk_Latent_StartPIEMultiClient`, `WaitForPIEReady`, `RunOnServer`, `WaitForCondition`,
  `TickWorlds`, `AssertCondition`, `EndPIE`; delegates `FCk_NetAutoTest_ServerAction/Assertion`).
- `Source/CkTests/Private/CkSnapshot/Test_Snapshot_M2b_LevelReload_Gate.spec.cpp` — simplest exemplar
  (single-world; post-travel-world helper at 34-52; save at 154; load+in-progress assert at 164-166).
- `Source/CkTests/Private/CkSnapshot/Test_Snapshot_TransformParity_MPReload_Gate.spec.cpp` — 2-world
  exemplar (reload-wait predicate at 246-259; resolve-by-SpawnRecipe at 97-116).
- `Source/CkTests/Private/CkSnapshot/Test_Snapshot_TimerParity_MPReload_Gate.spec.cpp` — the conversion target.
- `Source/CkTests/CkTests.Build.cs` — already depends on every needed module; no dep changes expected.
- New files: `Source/CkTests/Public/CkTests/Snapshot/CkSnapshot_TestHarness_Common.h` +
  `Source/CkTests/Private/Snapshot/CkSnapshot_TestHarness_Common.cpp`.

## Glossary

- **Registrar** — the static-init struct in a feature's `_Fragment.cpp` that registers its payload handler(s).
- **Slot** — one `TFunction` member of `FHandler`: net-receive apply (`Apply`→`NetApply`), net removal
  (`Remove`→`NetRemove`), authority-side load apply (`HydrationApply`), save-capture emitter (`Produce`).
- **Self-resident vs owner-keyed** — whether the replicated container entry lives on the feature's own
  entity or on its `LifetimeOwner` (child entities don't have their own replication drivers).
- **Class (a)/(b)/(c)** — trivially-symmetric / record-aggregated / instruction-stream wire builders (see
  file inventory).
- **Fold** — deriving the owner-keyed wire payload by calling the child-keyed `Produce` and merging its
  entries into the owner container entry.
- **Two-cycle rule** — a round-trip pin must save→load→assert TWICE with exact counts
  (`CkSnapshot/Claude.md` §5): cycle 2 catches double-apply stacking.
- **Parity campaign** — `docs/campaigns/saveload-v3-parity/`, the sibling in-flight campaign this one
  sequences after and partially absorbs.

## Phase docs

- [PHASE_1.md](PHASE_1.md) — harness + StaticCast sweep + Jump absolute (build 1)
- [PHASE_2.md](PHASE_2.md) — rename bundle (build 2)
- [PHASE_3.md](PHASE_3.md) — symmetry class (a) (build 3)
- [PHASE_4.md](PHASE_4.md) — symmetry class (b) (build 4)
- [PHASE_5.md](PHASE_5.md) — `CkEcs/Persistence/` header split (build 5; independent of Phase 4)
- [VALIDATION.md](VALIDATION.md) — final acceptance protocol
- [PROGRESS.md](PROGRESS.md) — living status doc; update at every phase boundary and divergence
