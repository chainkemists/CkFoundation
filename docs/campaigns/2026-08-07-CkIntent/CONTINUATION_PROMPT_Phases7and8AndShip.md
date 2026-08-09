# CONTINUATION — CkIntent campaign: finish Phase 7, run Phase 8, campaign-end full suite, ship

**One line:** you are the ORCHESTRATOR of the CkIntent campaign (docs in this directory);
Phases 0-6 are CLOSED and gate-green, Phase 7 is half-landed (unit 7-1 scan diagnostics was
in flight when this session ended — VERIFY ITS STATE FIRST), Phase 7-2 (debugger UI) and
Phase 8 (gyms + the 40-move bake) remain, then ONE campaign-end full suite and the ship flow.

## Read these before acting, in order

1. `PROGRESS.md` (this dir) — the living log. **Trust it over this file and over any
   compaction summary.** Every ruling is a numbered `[Pn-Dm]` entry; honor them without
   re-litigating.
2. `PHASE_7.md` — the open phase contract ([P7-D1..D3]).
3. `PROMPT.md` — mission brief, decision table D1-D25b, phase index, success criteria.
4. Load skills: `meta-campaign`, `Plugins/CkFoundation:ck-ship-dev` (at ship time),
   `build-test` (before any toolbox run). The executor for 7-2 must load
   `ck-gameplaydebugger-extension` + `ck-slate-tools` FIRST.

## Standing authorizations and constraints (user-granted, still in force)

- Campaign phase-chaining is authorized ("move on to the next phase(s)"); aggressive
  delegation to Opus 5 subagents; orchestrator reviews every unit and re-runs every gate.
- **[P2-D4] gate policy:** phases close on SCOPED gates only; exactly ONE full suite when
  the whole campaign is green (`--test --no-nullrhi --parallel 1`, no pattern; diff anchor
  `1027/1025/2` + every test name added since — the two eternal reds are the
  `PathNetworkFollower` pair).
- Commits: the user authorized the 2026-08-09 commit checkpoint (done). NEW work needs
  fresh commit authorization. **Push has NEVER been authorized** — the ship flow
  (fetch/divergence table/backup branch/rebase/regate/push/pointer bumps) is a separate
  conversation with the user.
- No `.uasset` hand-authoring without asking (populator-placed external actors are
  pipeline artifacts and are fine — precedent committed twice).

## Repo state at handoff (2026-08-09)

All on `dev` in each repo. Committed this session (LOCAL, unpushed):

**CkFoundation** (`Plugins/CkFoundation/`): `241048f2d` unbind fix → `46f92c369` ButtonId
map + subsystem seam + CkInput doc → `6d7431b15` router delivery retention → `e90384363`
CkEcs clamp trait → `0bb6febde` CkIntent module (sampler/grammar/compiled sets — matcher
EXCLUDED) → `8f27fd68f` campaign docs (PHASE_2-7 + PROGRESS). Earlier same-campaign commits
`9b49261bd..d9cd4e147` beneath.

**CkTests** (`Plugins/CkTests/`): `9fc7ffb7` gym rework → `e552626b` unbind pin + ButtonMap
tests → `c6dc4605` 22 Intent tests + C++ batteries + Build.cs (+CkInput/+CkIntent link
deps) → `6260c453` wrapper regen + 34 external actors. Earlier: `9d5a3278`.

**UPDATE (same session, later): unit 7-1 LANDED, REVIEWED, GATED (118 suite greens + the
diagnostics test 1/1 isolated after two orchestrator test fixes — see PROGRESS), and the
matcher surface is COMMITTED** (matcher + `Source/CkIntent/Claude.md` + PROGRESS +
this file in CkFoundation; the diagnostics test + pipeline artifacts in CkTests — see the
repos' `git log -3` for the SHAs). **Phase 7's remaining work is unit 7-2 only** (the
`CkIntentDebugger` UI). Nothing is uncommitted except foreign dirt.

**UNIT 7-2 WAS DISPATCHED AND KILLED MID-READING (machine restart)** — it wrote NOTHING
(verified: no `CkIntentDebugger` dir, CkGameplayDebugger status clean). Re-dispatch it
fresh from the package in PHASE_7.md + [P7-D3]: fresh Opus agent, MUST load
`ck-gameplaydebugger-extension` + `ck-slate-tools` skills first, mimic `CkGoapDebugger`,
consume ONLY recorded state ([P7-D1] — the read APIs are enumerated in PROGRESS's Phase-7
entries), deliver the `[EDITOR-VERIFY]` list incl. criterion 5's scenario. Note its last
observation before the kill: "the launcher catalog spec enforces a census — check it
before adding a module" — the CkGameplayDebugger plugin appears to have a module-census
spec the new module must be registered in; make sure the re-dispatched unit honors it.
One more gitignore trap learned: CkFoundation's `.gitignore:49` is a blanket `*.md` —
NEW module docs must be `git add -f`'d (89 tracked sibling docs are the precedent) or
they silently vanish from commits.

**Foreign dirt — NEVER stage:** CkFoundation: 78-82 `Content/CkUsf/GeneratedLooks/*.uasset`,
`docs/reviews/`, `docs/superpowers/`, request-completion-delegates continuation. Superproject:
`Config/DefaultGameplayTags.ini`, `CONTINUATION_PROMPT_CrowdDebuggerRuntimeSidewalksInvisible.md`,
`_scratch/`, and the two submodule gitlink pointer entries (bump only at push time —
cross-repo publish guard: only pushed SHAs).

**Divergence (stale, re-fetch at ship):** CkTests was 1 ahead / 13 BEHIND origin/dev before
this session's commits — ship needs backup branch + rebase + regate. CkFoundation and root
were clean-ahead.

## The remaining work

1. **Unit 7-1 verification** (see above). Its gate: build + `Ck_AutoTest_In`
   `--discover-fresh` (expect 119 = 118 + 1).
2. **Unit 7-2 — the `CkIntentDebugger` module** (in `Plugins/CkGameplayDebugger`, a
   SEPARATE submodule with its own git). Per [P7-D3]: mimic `CkGoapDebugger` (category +
   MVVM + reuse `CkDebuggerCommon`'s `SCkDebug_EventTimeline` — parameterized lanes widget).
   Views: timeline (layer spans + intent phase spans + blocked-by), layer-stack tree,
   key/state list (held/axes/octant dial/SOCD), resolution-table view, near-miss list
   (renders 7-1's ring). [P7-D1]: render RECORDED FACTS only, never recompute. Gate =
   compile + suite green; visuals go on the `[EDITOR-VERIFY]` queue with exact steps
   (success criterion 5's scrub is one of them). Memory note: `SListView` rows have two
   click-traps — the executor must read CkDebuggerCommon's CLAUDE.md "List / tree rows".
3. **Phase 8** — author `PHASE_8.md` first (orchestrator). Scope per PROMPT row 8: gyms
   (`Fighting`, `Souls`, `Debugger`) + autotest gap-closing. Success criterion 6 lives
   here: a ~40-move set authored in AS compact notation bakes with ZERO hand-written
   per-move struct construction (an AutoTest proves it — parse+bake 40 notation strings
   from an asset-shaped container). Success criterion 1's `[EDITOR-VERIFY]` leg (gamepad
   QCF+Punch on-screen 0-frame counter) needs the Fighting gym. Gym framework:
   `CkGym_StationSm` step states, colored lines API (`Update_StationDisplay_Colored`),
   registry row in `Script/Common/CkTests_GymRegistry.as`, gym color contract [P1A-D8]
   (red = true failure only). Gym content faces world -X; runtime floors need Z scale
   ≥ 0.5 for navmesh (memory).
4. **Campaign-end full suite** ([P2-D4]): editor CLOSED, `--test --no-nullrhi --parallel 1`,
   verdict from the log's `=== Test summary ===` block. Expect baseline 1027 + all names
   added (118-row scoped set at handoff + 7-1/7-2/Phase-8 additions); failures must be
   EXACTLY the two `PathNetworkFollower` names.
5. **Ship** (separate user conversation): `/ck-ship-dev` flow — commit outstanding, fetch,
   divergence table, backup branches, rebase CkTests, REGATE on the rebased base, push
   submodules, pointer bumps (publish guard), push root. Flag to the user: the
   CkGameplayDebugger submodule joins the ship set once 7-2 lands.
6. **Human queue (remind the user at close):** EDITOR-VERIFY items (gym drive-throughs,
   persistence check, controller hot-swap, criterion 1 + 5 scenarios), 0A hardware spike
   (S1-S7 — octant/hysteresis defaults are PROPOSALS pending it), maintainer review of the
   reconstruction flags ([P2-D2] button model, [P4-D1] grammar, [P4-D4] cycle validation,
   [P3-D4] retention) and the CkGameSettings defect escalation (1-5, 7; #13 FIXED).

## Working method (proven this session — keep it)

- One Opus unit per dispatch, micro-package with STOP conditions; design forks return to
  the orchestrator and become numbered `[Pn-Dm]` rulings in PROGRESS BEFORE implementation.
- Orchestrator reviews every unit (spot-read the load-bearing code), runs every gate
  itself, checkpoints PROGRESS after EVERY event (compaction-proofing).
- Reuse a unit's agent for follow-ups in its own files (context intact); fresh agent for
  new modules.
- Serialization: never edit Script/ during a test run; never edit source during a build;
  one toolbox editor at a time; probe the editor lock before builds
  (`[IO.File]::Open('D:/Repos/CkPlugins/Saved/Logs/CkPlugins.log','Open','Write','None')`).

## Gate commands that work (copy verbatim)

```powershell
# scoped build+test (the workhorse; add --generate IFF any .Build.cs/.uplugin changed):
Set-Location "D:\Repos\CkPlugins"; ./CkAuto/UnrealToolbox.exe --build --config=Auto --target=Editor --test --test-pattern "Ck_AutoTest_In" --discover-fresh --parallel 1 --output="D:\Repos\CkPlugins\Saved\Logs\BuildTest-<name>.log" --project="D:\Repos\CkPlugins"
# C++ pattern (test-only — NEVER pass --config without --build, exits 107):
./CkAuto/UnrealToolbox.exe --test --test-pattern "Ck.Intent.Grammar" --parallel 1 --output="..." --project="D:\Repos\CkPlugins"
```
Verdict = the log's trailing `=== Test summary ===` block, NEVER the exit code (exit 1 =
test failures = normal; exit 76/AS_COMPILE_FAILED = stale bytecode, results invalid; the
toolbox names the failing .as line — fix and re-run). `Ck_AutoTest_In` matches
Input/Intent/Interaction/Inventory (~118 rows at handoff, all green).

## Branch table — likely failure modes

| Symptom | Probable cause | Look at |
|---|---|---|
| "No matching signatures to Request_*(...)" AS error | Missing request-struct arg (only the trailing delegate is defaulted in AS wrappers) | The call site; pass `FCk_Request_X()` |
| AS "Expected ')' — found string constant" | Adjacent string literals (never legal in AS) | Join into one literal |
| Scoped gate green but Total didn't grow | Stale discovery cache | Re-run with `--discover-fresh` |
| New C++ test "No tests matched" | Relink not triggered | Touch the test .cpp + rebuild |
| LNK2019 on another module's type in a CkTests TU | Missing link dep in `CkTests.Build.cs` (transitive public deps did NOT link this session) | Add the module to the deps list |
| Editor exits 0xFF after all tests | Normal when failures exist — results are kept | Read the summary block |
| Full suite has an extra red under load | Known Jolt `KinematicPlatformCarriesDynamicBox` load flake (proven editor-open-only) | Re-run isolated; editor closed |
| Warnings escalate to test failures | AutoTest harness escalates ck::Warning | Log Verbose in legitimate-state paths |

## Critical files (beyond the campaign docs)

- `Source/CkIntent/Public/CkIntent/CkIntentMatcher_Processor.cpp` — matcher/deferral/claim/
  signals/decay; `Set_Phase` (~:430) is the ONLY phase writer (friend-enforced); scan sites
  are where 7-1's diagnostics record.
- `Source/CkIntent/Public/CkIntent/CkIntentSampler_*.{h,cpp}` — record ring, octant/SOCD,
  pending-events accumulator ([P3-D6]).
- `Source/CkIntent/Public/CkIntent/CkIntentGrammar_Utils.cpp` — Parse + Bake (one parser,
  D9); `CkIntentCompiledSet_Data.h` — verdicts-as-data.
- `Source/CkInput/Public/CkInput/CkInputLayer_Processor.cpp` — router + retention writes.
- `Source/CkInput/Public/CkInput/CkInputButtonMap_*.{h,cpp}` — the rebind-following map.
- `Source/CkInput/Claude.md` + `Source/CkIntent/Claude.md` — the module contracts (the
  latter is UNCOMMITTED until 7-1's commit).
- `Plugins/CkGameplayDebugger/Source/CkDebuggerCommon/.../SCkDebug_EventTimeline.h` — the
  shared lanes widget 7-2 extends; `CkGoapDebugger` — the module shape to mimic.
- `Plugins/CkTests/Script/CkInput/CkAutoTest_Intent_*.as` — the battery (idiom source for
  all new tests); `CkInput_Assets.as` — Jump=SpaceBar, Crouch=C, Interact=E, Flashlight=F.

## Ruled out — do not re-investigate (full list = PROGRESS decision log)

Per-intent entities (dead, [P6-D1]); α/β readings of masked matching (γ ruled, [P5-D2]);
deferral for sequence suffixes (D7 law, baked as data); dense ButtonId ints before Phase-4
bake ([P2-D2]); a second sampler-side reader of the ring (race, 5-2 note); down-stack
claims ([P5-D4]); ByteAttribute as phase carrier (D5); notation conditionals (D9);
rated matcher at 60 Hz (unrated + record frame indices, 5-1); WaitFrames as a decay-timing
instrument (render-vs-logic frames — set `LatchDecayFrames(600)` in latch-holding tests).

## Gotchas accumulated (the expensive ones)

- Test keys already claimed by the battery: B, F1, F2, F3, F9, F10, F11, H, I, O, R, T, W,
  Y, Z, A, D, S, F4 + the four authored mappings' keys. Grep before choosing a new one.
- `FireIfPayloadInFlightThisFrame` is the only binding policy that cannot contradict the
  poll (a latch cannot decay in its stamping frame).
- The swap-reset `→ Idle` signal carries `INDEX_NONE` deliberately ([P6-D4]).
- Physical-tier ButtonMap buttons let tests avoid the EI registration dance entirely.
- The AS wrapper generator emits `utils_<snake_case>` on editor boot; new UFUNCTIONs are
  AS-visible only after a build boots the editor once (the gate does this).
- Octant/hysteresis/SOCD defaults (0.25 / 5° / Neutral) are PROPOSALS pending 0A.

## First-session flow

1. Re-read `PROGRESS.md` tail + this file. Verify 7-1's disk state (test file present?
   `Get_ScanDiagnostics` in the matcher utils? PROGRESS entry?).
2. If 7-1 landed unreviewed: review (report may be lost — re-derive from diff:
   `git -C Plugins/CkFoundation diff Source/CkIntent/` + the new test), gate (expect 119),
   checkpoint PROGRESS, ask the user to authorize the matcher commit.
3. Dispatch 7-2 (fresh Opus agent, skills first). Gate. Close Phase 7 (EDITOR-VERIFY queue
   written into PROGRESS).
4. Author PHASE_8.md (rulings at open), dispatch its units, gate, close.
5. Campaign-end full suite (editor closed). Diff against `1027/1025/2` + all added names.
6. Report to the user: results, the human queue, and ask about the ship flow.

## Suggested first message (for the user to paste)

> I'm resuming the CkIntent campaign as orchestrator. Read
> `D:\Repos\CkPlugins\Plugins\CkFoundation\docs\campaigns\2026-08-07-CkIntent\CONTINUATION_PROMPT_Phases7and8AndShip.md`
> fully, then `PROGRESS.md` in the same directory, before doing anything. Phases 0-6 are
> closed; verify unit 7-1's state on disk first, then continue: 7-2 debugger UI, Phase 8,
> campaign-end full suite. Same rules: scoped gates, Opus delegation, ask before commits
> and before any push.
