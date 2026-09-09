# CONTINUATION PROMPT — navigation-next campaign, post-PHASE_0 (for an Opus 5 executor session)

**One-line summary:** PHASE_0 of the navigation-next campaign (CkGroundNav) is COMPLETE and
gate-verified in this worktree, with NOTHING committed anywhere; you are picking up as the
next executor session — most likely to ship P0 on the maintainer's instruction, and/or to
start PHASE_1 as a fresh executor per PHASE_1.md §2.

**You are Opus, not Fable.** The planning/ruling session ran out of usage. You follow the
written plan; you do NOT make design decisions. Any design question, any observation a gate
doesn't enumerate, or two failed attempts on one step → STOP, record verbatim in PROGRESS.md
§ Blockers, report, end turn. This discipline is the campaign's backbone — honor it.

---

## 0. Read these first, in order (non-negotiable)

1. `Plugins/CkFoundation/docs/campaigns/navigation-next/PROMPT.md` — campaign contract,
   executor rules, §7 session-start ritual, §6 skill list.
2. `Plugins/CkFoundation/docs/campaigns/navigation-next/PROGRESS.md` — **state of record.**
   Trust it over this file wherever they disagree. Honor every [NN-D1..D21] decision; re-litigate
   none. The "P0 CLOSE-OUT" section has the final gate evidence, exit-criteria table,
   [EDITOR-VERIFY] list, follow-ups, and the authoritative dirty-path census.
3. If starting PHASE_1: `PHASE_1.md` in the same folder (entry criteria in its §2).
4. Load skills before touching code: `build-test` (project root wrapper → CkAuto canonical),
   `ck-macros-and-codegen`, `ckecs-architecture-contract`, `ck-change-control` (all under
   `Plugins/CkFoundation/.claude/skills/`), plus user-level `meta-verification-discipline`.

Perform the §7 ritual: read PROGRESS.md and distrust it — spot-check two Done claims against
cited artifacts before building on them.

## 1. Repo state (verified 2026-08-31, end of P0 session)

- **Worktree:** `D:\Repos\CkPlugins_3` (a linked git worktree of D:\Repos\CkPlugins — run
  everything from here, never cd to the main checkout). Root branch `feature/ck-navigation`
  @ `a6a327c`.
- **Submodules are on DETACHED HEADs** at their recorded entry SHAs:
  CkFoundation `a25ec9539`, CkTests `d18b1ca0`, CkGameplayDebugger `d85b77c`.
  Before committing in a submodule you must create/checkout a branch (branch names MUST begin
  `feature/` or `bugfix/` — anything else is invisible to CI and fails silently; this work is
  additive → `feature/ck-navigation` in each submodule is the natural choice, but ASK the
  maintainer which branch/PR shape they want before shipping).
- **Nothing is committed or staged.** All P0 work is dirty in the working trees.
- **Standing directives:**
  - [NN-D15]: `docs/campaigns/navigation-next/` must NEVER be committed, ever.
  - Do NOT commit or push anything unless the maintainer explicitly asks in-session.
  - Stage only files this campaign authored (list below). Other sessions' work in this
    checkout is untouchable.
- The root-level `CONTINUATION_PROMPT_CkNavigationReplacementCampaign.md` is a STALE handoff
  from before P0 executed — ignore it; this file + PROGRESS.md supersede it.

### Authored-by-P0 dirty paths (the only things you may ever stage)

**CkFoundation** (`Plugins/CkFoundation/`):
- NEW: `Source/CkNavigation/Public/CkNavigation/NavSurface/` (entire folder — facade, fragments,
  processors, gameplay tags, filter-definition asset, `Recast/CkNavSurface_RecastAdapter.*`)
- NEW: `Source/CkCrowd/Public/CkCrowd/CkCrowd_NavGameplayTags.{h,cpp}`
- Modified: everything `git status` shows under `Source/CkNavigation/`, `Source/CkCrowd/`,
  `Source/CkEqs/`, `Source/CkQueue/`, `Source/CkPathNetwork/`, `Source/CkPathNetworkEditor/`,
  `Source/CkAngelscriptGenerator/` (2 files: CkAssetRegistryConfig.h, CkAssetRegistrySubsystem.cpp),
  plus module docs `CkNavigation/Claude.md`, `CkCrowd/Claude.md`, `CkQueue/CLAUDE.md`.
- Deleted: `CkCrowd/Agent/CkCrowdAgent_DiagNavClip_*` (3 files),
  `CkCrowdAgent_DrawNavProjection_*` (2 files),
  `CkQueue/Navigation/CkQueue_NavigationRevisionSubsystem.*` (2 files) — all [RETIRE] rulings.
- NEVER STAGE: `docs/campaigns/navigation-next/` ([NN-D15]).

**CkTests** (`Plugins/CkTests/`):
- NEW: `Script/CkNavigation/` (all NavSurface autotests),
  `Source/CkTests/Private/UnitTests/CkNavigation/` (2 C++ test files, roots renamed to
  `CkTests.UnitTests.CkNavigation.*`),
  `Source/CkTests/Private/Net/Generated/Navigation_NetAutoTestStubs.spec.cpp`.
- Modified: the 12 `Script/CkCrowd/` + `Script/CkQueue/` fixture/gym files git shows,
  `Script/CkTests_Assets.as`.
- `Script/Generated/CkTests_AutoTestActors.as` — changed as a BYPRODUCT of our new autotests
  (generated but tracked). Diff-review, then include with the CkTests ship.
- NOT OURS (leave untouched): `Content/__ExternalActors__/*` (editor-boot artifacts).

**CkGameplayDebugger** (`Plugins/CkGameplayDebugger/`): all 7 dirty files ARE ours (batch-B
migrations): `CkCrowdDebugger/Data/CkCrowdDebugger_DataCollector.cpp`, `.../Types.h`,
`.../Window/SCkCrowdDebuggerWindow.cpp`, `.../SCkCrowdDebugger_NavmeshStatusPanel.cpp`,
`CkPerfLab/CkPerfLab.Build.cs`, `CkPerfLab/Claude.md`,
`CkPerfLab/Runner/CkPerfLab_WorldSurvey_Builder.cpp`.

**Root (CkPlugins_3):** we authored NOTHING at root. `Config/DefaultGameplayTags.ini` and
`Script/Generated/CkPlugins_EntitySpawnParams.as` are other-session/editor artifacts —
leave untouched, enumerate as "left for owning session" in any ship report. Root's only
eventual commit is the three submodule gitlink bumps (scope `git add` to the exact
`Plugins/<Name>` gitlink paths, one at a time).

## 2. What P0 built (context for reading the diffs)

Provider-neutral navigation facade `UCk_Utils_NavSurface_UE` (12 capabilities) + a Recast
adapter (`ck::nav_surface_recast`) so every Ck consumer (CkEqs, CkQueue, CkPathNetwork,
CkPathNetworkEditor, CkCrowd incl. ConstrainToNavmesh, CkCrowdDebugger, CkPerfLab) speaks the
neutral contract, with ZERO behavior change on the Recast path. Also: per-world deferred nav
request queue (`FFragment_Nav_DeferredRequests`, revision ring over [1, MAX_int32], 5s
watchdog), consolidated `Get_SurfaceRevision` observer, area gameplay tags → UNavArea class
table (incl. `TAG_Nav_Area_Impassable` → `UNavArea_Null` = navmesh hole), filter-definition
assets compiled per-provider, neutral fixture vocabulary (`Request_ImpassableBox`), leak
replacements in `CkNav_Fragment_Data.h` (`_ExcludedAreaTags`, `_QueryFilterOverride`).

## 3. Gate evidence (do not re-derive)

- Entry baseline: Total 1289 / Passed 1267 / Failed 22 (names recorded in PROGRESS.md).
- Final P0 gate: **Total 1300 / Passed 1280 / Failed 20**, fail set ⊆ baseline names,
  ZERO new reds at any gate all phase. Delta-zero = identical failing-test NAMES, and
  red→green counts as a defect until adjudicated.
- Three red→green flips were probe-adjudicated flake-class (3× fixed-artifact probes each):
  `NoRouteFailsClean`, `Homing_ClearTarget`, `Crowd_Grounding_StationaryAgentReGrounds`
  (red-leaning). Do NOT re-adjudicate; registry is in PROGRESS.md.
- EQS A/B (Gate 0C-4): facade ≤ inline (2.337/2.347/2.337 s vs 2.375/2.350/2.551 s). PASS.
- Ensure-aggregate sites identical to baseline throughout.

## 4. Build/test — exact invocation and traps (mistakes here poison everything)

- **Only** via UnrealToolbox per the `build-test` skill. NEVER `Build.bat`, UBT, or
  `UnrealEditor-Cmd.exe` directly. Editor must be closed (toolbox exits **77** if open).
- Full-suite gate shape used all phase:
  `--build --target=Editor --test --parallel 1 --no-nullrhi` (+ `--discover-fresh` whenever
  new AS autotests were added; omit `--config` for test-only runs).
- Exit codes: 76 = AS compile failed (stale bytecode), 77 = editor open, 78 = contaminated run.
- **No Script/ or source edits while a build or test run is in flight** — poisons the run.
- Suite scoping (cost a day to learn): the host full suite discovers ONLY
  `CkTests.UnitTests.*` + `Project.Functional Tests.*` roots. All `Ck.*`-rooted C++ tests and
  ALL `Ck.*.Net.*` net tests are excluded. New CkTests C++ tests must be named
  `CkTests.UnitTests.*`; net tests run via explicit `--test-pattern`.
- New automation tests need a relink (touch + rebuild); `--generate` alone doesn't surface them.
- Toolbox caches test discovery: green-with-old-Total = stale-green; use `--discover-fresh`.

### Worktree-specific traps (this exact checkout)

| Symptom | Cause | Fix |
|---|---|---|
| Toolbox: "No engine selected" | Per-worktree setting, no CLI flag, hash irreproducible | Maintainer must pick engine interactively once (already done for CkPlugins_3 — should not recur) |
| "Cannot open output file Saved/Logs/BuildTest.log" | `Saved/Logs` missing in fresh worktree | `mkdir` it (already exists here) |
| Editor-lock probe says "locked" with no editor | Probe throws on NONEXISTENT `Saved/Logs/CkPlugins.log` and misreads it as locked | Check the file exists before trusting a "locked" verdict |
| Exit 76 + `_StubRecovery_*.as` + gutted `CkTests_EntitySpawnParams.as` | T9: cold headless boot regenerates AS spawn-params from incomplete type DB | ONE unattended GUI editor boot regenerates correctly (file ≈514 KB); never re-run the commandlet or hand-edit the stub |

## 5. What is pending (the actual work queue)

**Maintainer-gated — do not do these unprompted:**
1. Two [EDITOR-VERIFY] items (maintainer runs them): BP-graph smoke of NavSurface facade
   nodes; one PIE session on a crowd gym confirming grounded agents constrain normally.
2. Optional: click `[Ck][AssetRegistry] Generate All Asset Registries` in-editor to clear the
   2 stale `ARecastNavMesh` refs in generated `CkTestsAssets.as` (generator is event-armed;
   idle boots don't regenerate — this is satisfied-at-source residue, NOT a defect).
3. Review of rulings [NN-D17..D21] (veto windows open, recorded in PROGRESS.md).
4. **The ship decision.** Only on explicit instruction. Order: commit+push each submodule
   first (CkFoundation → CkTests → CkGameplayDebugger), THEN root gitlink bumps. Cross-repo
   publish guard: before bumping a gitlink, verify the pointed-at submodule commit is itself
   pushed (`HEAD ⊆ origin/<branch>`). Stage ONLY the authored paths in §1. Re-run the full
   gate on the final artifact before claiming green (stale-green is not green).

**Next phase:** PHASE_1 (bake core) in a fresh executor session per PHASE_1.md §2. Its entry
pattern-greps were pre-verified passing; it requires its OWN fresh full-suite baseline
captured in that session before any edit. Do not start P1 in a ship session.

**Logged follow-ups (out of P0 scope — do not fix unprompted, they're in PROGRESS.md):**
stranded completion delegates on the client-refusal path (`CkNav_Processor.cpp:322-331`,
pre-existing); RewindHistory net test's latent authority-branch bug; `Get_IsMarkupLive`
cannot observe holes (PHASE_4); `_QueryFilters` data-migration check for BusterBlock before
pointer bumps; dead `AIModule` dep in `CkCrowdDebugger.Build.cs`.

## 6. Things ruled out / already adjudicated — do NOT re-investigate

- The 3 flake-class tests (§3) — probe evidence recorded; delta-zero holds.
- `CkNavmeshDebugDraw` module still full of Recast calls — DELIBERATE ([NN-D8f]: retired at
  Recast retirement, never rewritten). Leave it alone.
- `Resolve_OffPathLeg` keeps its byte-identical `HasValidNavmesh` gate; CkPerfLab keeps
  `GetWorldBounds` — both ruled correct in [NN-D19]'s companion notes.
- PathNetwork raycast overlay-drop → [NN-D19] P5-scope (not a P0 miss); [NN-D20] added
  `_QueryFilterOverlay` to the neutral Raycast/MoveAlongSurface queries preemptively.
- 2 `ARecastNavMesh` grep hits in generated `CkTestsAssets.as` — stale generated residue, §5.2.
- Net autotest authority: locally-spawned test entities are authoritative in EVERY world.
  Branch on `Get_HasAuthority(Get_SubjectEntity())`, never the test entity. Client-side
  immediate refusal is `NotAuthority || NoNavSystem` (never `NoNavData`). The shipped tests
  encode this — do not "fix" them back.
- `Queue_NavigationChangeRetriesImpossibleFormation`'s recovery step calls ONLY
  `DestroyMarkup()` — deferred unpaint drives exactly one rebuild; adding an explicit rebuild
  re-bakes the markup and doubles generation events. Comment in the file explains it.
- `SurfaceBoundsAreNonDegenerate` accepts `Size.Z >= 0.0` — a flat single-floor navmesh
  legitimately has zero Z thickness.

## 7. Critical files (beyond the campaign docs)

- `Plugins/CkFoundation/Source/CkNavigation/Public/CkNavigation/NavSurface/CkNavSurface_Utils.{h,cpp}` — the 12-capability facade; the campaign's public contract.
- `.../NavSurface/Recast/CkNavSurface_RecastAdapter.{h,cpp}` — sole Recast touchpoint post-P0 (plus legacy `CkNav_*` and the retire-later debugger module).
- `.../Nav/CkNav_Fragment.h` — per-world deferred queue + hoisted `ck::nav::IsNewerRevision`/`AddDeferredLatest` ([NN-D21]).
- `.../Nav/CkNav_Processor.cpp` — request pipeline; authority check at :318 precedes world/navsys checks.
- `.../Nav/CkNav_Fragment_Data.h` — neutralized params (`_ExcludedAreaTags`, `_QueryFilterOverride`).
- `Plugins/CkTests/Script/CkNavigation/` — the NavSurface autotest suite (incl. net tests run by explicit pattern).
- Module docs updated for the new contract: `Source/CkNavigation/Claude.md`, `Source/CkCrowd/Claude.md`, `Source/CkQueue/CLAUDE.md`.

## 8. Recommended opening flow for the new session

1. Read PROMPT.md → PROGRESS.md (§0 order). Spot-check two Done claims.
2. `git status --short` at root and in all three submodules; diff against §1's census. Any
   NEW dirty path neither §1 nor PROGRESS.md explains → another session touched the tree;
   enumerate it as untouchable and tell the maintainer before proceeding.
3. Ask the maintainer which task they're commissioning: ship P0, or start P1. Do not guess.
4. If shipping: confirm branch/PR shape first (submodules are detached — need branches),
   then §5.4's order, then a final full-suite gate on the shipped SHAs.
5. If P1: fresh session is the designed shape; verify PHASE_1.md §2 entry criteria with
   evidence, capture a fresh baseline via the toolbox BEFORE any edit, then follow its steps.

## 9. Suggested first message (for the maintainer to paste)

> I'm continuing the navigation-next campaign after PHASE_0 completed. Read
> `D:\Repos\CkPlugins_3\Plugins\CkFoundation\docs\campaigns\navigation-next\CONTINUATION_PROMPT_P0Ship_OpusExecutor.md`
> fully, then PROMPT.md and PROGRESS.md per its §0, before doing anything. Task for this
> session: [SHIP P0 / START PHASE_1 / other]. Nothing is committed; [NN-D15] and the
> stage-only-authored-paths rule are in force.
