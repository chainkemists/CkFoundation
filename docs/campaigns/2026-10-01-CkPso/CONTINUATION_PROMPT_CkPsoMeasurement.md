Start with: Fable · high · plan mode yes

## Paste this to resume

```text
Start with: Fable · high · plan mode yes
I'm continuing the CkPso campaign (PSO preparation for CkFoundation loading screens). Read this continuation prompt fully before doing anything: D:\Repos\CkPlugins2\Plugins\CkFoundation\docs\campaigns\2026-10-01-CkPso\CONTINUATION_PROMPT_CkPsoMeasurement.md
The campaign was PARKED on 2026-10-02: P1 and the miss-log parser are committed locally and unpushed on feature/ck-pso (based on chainkemists history), while BusterBlock is moving CkFoundation to the CommitAndChill history, which shares no merge-base with it. Remaining work, in order: port feature/ck-pso onto the post-adoption history and gate it in the editor, build a packaged Development BusterBlock from that line, then Neil runs three measurement runs that decide the P2/P3 scope.
Start by asking Neil whether the CommitAndChill adoption has been submitted and which BusterBlock and CkFoundation branches it landed on; then plan the port in plan mode.
```

# CkPso — parked; port onto the CommitAndChill history, then measure

Picking up: P1 is built and editor-gated on the old CkFoundation history. The next step is no longer "package BusterBlock
against this branch" — it is porting the branch onto the history BusterBlock is moving to, then the packaged measurement,
then re-scoping P2 (warm-up) and the rest of P3 (miss capture) from its numbers.

This file was rewritten in place on 2026-10-02 and replaces the 2026-10-01 version of itself. If anything quotes
"package BusterBlock against this branch" as the next step, that is the old text.

As of: 2026-10-02 · CkFoundation@90db7a10e (feature/ck-pso) · CkTests@052dc0eb5 (feature/ck-pso) ·
CkGameplayDebugger@243c0cd4e (dev) · CkPlugins2 superproject@6cabe81 (dev-ckplugins2) ·
BusterBlock@1dfd6a8b98 (feature/ckf-rename-adoption-cnc, another conversation's work)

If HEAD has moved past this, re-check Repo state before trusting the rest.

## Repo state

Checkout: `D:\Repos\CkPlugins2`. Nothing was built, committed or pushed on 2026-10-02; the session was plan mode plus
read-only recon. BusterBlock, BusterBlock_alt and the engine were read, never written.

| Repo | Branch | Commits (local, unpushed) |
|---|---|---|
| `Plugins/CkFoundation` | `feature/ck-pso` (off chainkemists dev `faa395509`) | `96518fe8f` module + gate · `c6b27489c` miss-log parser · `90db7a10e` campaign docs |
| `Plugins/CkTests` | `feature/ck-pso` (off dev `ac3e2e0a`) | `d73ce2fb` two AutoTests · `052dc0eb` generated wrapper + two placed actors |
| superproject | `dev-ckplugins2` | `815a001` PlaceableTests script fix · `6cabe81` AutomationGate.json |

**This work exists only on this machine.** Neil was offered a backup push of the branches and has not answered.

Uncommitted, this campaign's, left for Neil's commit word:
- This file (untracked; the skill does not commit it).
- `PROGRESS.md` beside it (tracked, modified 2026-10-02: a "Parked" section, a research correction, open item 6).

Uncommitted, NOT this campaign's authored work — do not stage:
- Superproject submodule pointers for CkFoundation / CkTests / CkGameplayDebugger (un-bumped since before the campaign).
- `Config/DefaultGameplayTags.ini`, `Script/Generated/CkPlugins_EntitySpawnParams.as` (written by editor test boots).
- Untracked: `.claude/skills/{debug-issue,explore-codebase,refactor-safely,review-changes}.md`,
  `Content/CkPlugins/Cube_EntityScript.uasset`, `Content/EntitySpawnParams/EntitySpawnParams_{Cube_TEST,Mesh_EntityScript}.uasset`.

Scratch (gitignored, safe to delete): `Saved/CkPsoStaging/`.

### Sibling repos (state on 2026-10-02 — expect all of it to have moved)

- `D:\Repos\BusterBlock`: branch `feature/ckf-rename-adoption-cnc`, 201 dirty entries, a toolbox test run was live.
  Its `.gitmodules` points CkFoundation and CkTests at `github.com/CommitAndChill/…`. Its CkFoundation was `2fce06417`
  on a `cnc` remote. **Owned by another conversation. Do not touch.**
- `D:\Repos\BusterBlock_alt`: a git worktree of BusterBlock at `f065755748` (four unpushed gate/docs commits over
  `origin/dev` `fb2976f10d`), CkFoundation `589375cff` (on chainkemists `origin/release/busterblock-1.0.4`). Idle that
  day. Owner not confirmed with Neil.
- Other sessions run toolbox tests on this machine most of the day and hold the engine lock shared.

### Last gate baseline (unchanged since 2026-10-01)

- Pre-change (serial, fresh boot, toolbox v1.51, CkFoundation `faa395509` / CkTests `ac3e2e0a` / CkGameplayDebugger
  `243c0cd4` / superproject `c14c308` + the script fix): **3819 run, 3775 passed, 44 failed, 0 contaminated.** The 44
  names are the `knownReds` in `D:\Repos\CkPlugins2\AutomationGate.json`.
- Post-change full suite (two lanes): **3959 run, 3907 passed, 52 failed** = 44 known + 8 new, each attributed to
  something other than CkPso. The table is in `PROGRESS.md`.
- Final scoped run on the committed tree: `Pso` 37/37.
- **None of this applies to the CommitAndChill history.** The port needs its own baseline there, captured before the
  first change.

## Confirmed vs inferred

Confirmed on 2026-10-02 (evidence named):
- Repo heads above (`git rev-parse` in each repo).
- BusterBlock HEAD's `.gitmodules` uses CommitAndChill URLs; BusterBlock_alt's commit uses chainkemists URLs
  (`git show <rev>:.gitmodules`).
- BusterBlock_alt pins CkFoundation `589375cff`; between that and `faa395509` there is zero diff in
  `Source/CkLoadingScreen`, `Source/CkSettings`, `Source/CkBuildConfig`; the seam CkPso uses exists at that pin
  (`CkLoadingScreen_Subsystem.h:78,95,107`).
- Outside `Source/CkPso/`, commits `96518fe8f` and `c6b27489c` touch only `CkFoundation.uplugin`, `Source/CLAUDE.md`,
  `Source/CkLoadingScreen/Claude.md` (`git show --stat`).
- `r.PSOPrecache.GlobalShaders=1` on Windows (`Engine/Config/Windows/BaseWindowsEngine.ini:26`).
- BusterBlock's `EngineAssociation` `{E4464C1C-43B7-7194-E787-E7B5711509D4}` is not registered in
  `HKCU\Software\Epic Games\Unreal Engine\Builds`; the engine is `D:/Repos/UnrealEngine-Angelscript`.
- Buildkite package dialog fields: `mode`, `cook`, `as-mode`, `test-content`, `deployments`, `steam`
  (`BusterBlock_alt/.runreal/steps/package.steps.ts:42`); `Build.xml` reads `BB_AS_MODE` (`:256-261`) and
  `BB_PRISTINE_COOK` (`:125-127`).

Reported by read-only recon agents with file:line, not re-checked by me (trust, but re-open the line before leaning on it):
- BusterBlock's cnc CkFoundation shares no merge-base with `faa395509`; the cnc counterpart of `faa395509` is
  `2f9b145f5`; `Register_LoadingProcessor`, `BindTo_OnVisibilityChanged`, `Get_IsLoadingScreenShowing` exist at cnc HEAD.
- Every engine symbol in CkPso's never-compiled `#if PSO_PRECACHING_VALIDATE && UE_WITH_PSO_PRECACHING` regions
  (`CkPso_Subsystem.cpp:22-24, 67-79`) exists with the expected signature (`PSOPrecacheValidation.h:81,162,225-228,244,327`).
- `PSO_PRECACHING_VALIDATE` is 0 in the editor and 1 in Development and Shipping game builds (`PSOPrecacheFwd.h:18,22`).
- The engine's miss-block format strings match what `CkPso_MissLogParser` expects (`PSOPrecacheValidation.cpp:524-555`).
- BusterBlock has no PSO config and no bundled pipeline cache; RHI is D3D12.

Inferred (what would confirm):
- CkPso compiles and links in a non-editor target — the first packaged build.
- The pending count drains to zero on a cold run instead of stalling — measurement run 1. **This is the claim most
  likely to be wrong.**
- Plugin cvars set with `-ini:Engine:[SystemSettings]:ck.Pso.…` before the module loads are applied at registration —
  the gate line in the run's log.
- The rename did not touch any identifier CkPso uses — nobody has looked at CkPso's includes on the cnc history yet.
- Everything already listed as inferred on 2026-10-01 (Fab plugin's precompile is a no-op, four Ck render paths lack
  precache coverage, the three "passes alone" gate failures are noise) is still inferred.

## Remaining plan

Plans:
- Parked plan, with the runbook, the outcome table and a recon-facts section:
  `C:\Users\neilj\.claude\plans\pasted-content-id-26ee-start-with-glimmering-leaf.md`. Its "Build route" section
  (temp branches off BusterBlock `origin/dev` + CkFoundation `589375cff`) only applies if Neil decides to measure
  *before* the adoption lands. He decided against that on 2026-10-02.
- Original campaign plan: `C:\Users\neilj\.claude\plans\d-users-neilj-downloads-pso-smart-utils-compiled-orbit.md`.
- Living state: `PROGRESS.md` beside this file.

Done: P1 slices A and B; the P3 parser; campaign docs; host gate file; measurement runbook (written, not run).

Next, in order:
1. **NEIL:** say the adoption is submitted, and name the BusterBlock branch and the CkFoundation / CkTests remotes and
   branches it landed on. Until then there is nothing to do.
2. Plan the port (plan mode). Read CkPso's Ck includes against the new history first: `CkCore/{Algorithms,Ensure,Enums,
   Format,Log,Macros,Time,Validation}`, `CkLoadingScreen/{CkLoadingScreen_Common.h, LoadingProcess/CkLoadingProcess_Interface.h,
   Subsystem/CkLoadingScreen_Subsystem.h}`, `CkSettings/ProjectSettings/CkProjectSettings.h`. The two histories share no
   merge-base, so this is a cherry-pick of `96518fe8f`, `c6b27489c`, `90db7a10e` (and CkTests `d73ce2fb`, `052dc0eb`)
   onto a new branch there, not a rebase. Which checkout hosts it is an open question for Neil.
3. Baseline on the new history, port, toolbox build, scoped `Pso` + `LoadingScreen` run, then the full gate once.
   Report as a delta against that new baseline.
4. Build a packaged **Development** BusterBlock from the post-adoption line. Route options are in the parked plan:
   CI (Buildkite dialog: Development, pristine cook, AngelScript vm, AutoTests off, Steam deploy to a spare branch) or
   local BuildGraph. Any push is Neil's yes.
5. **NEIL:** the three runs, on a machine with the other sessions' builds and tests paused for about 20 minutes:

   | Run | Flags on top of the common set | Answers |
   |---|---|---|
   | 1 cold, gate on | `-clearPSODriverCache` | Does the drain reach zero; peak and duration; what still hitches |
   | 2 warm, gate on | — | What the driver cache carries; cost of the gate on a normal launch |
   | 3 cold, gate off | `-clearPSODriverCache -ini:Engine:[SystemSettings]:ck.Pso.WaitForPrecache=0` | What the gate buys |

   Common set: `-ini:Engine:[SystemSettings]:r.PSOPrecache.Validation=2 -ini:Engine:[SystemSettings]:ck.Pso.LogDrainEveryFrame=1
   -LogCmds="CkPso Verbose, LogPSOHitching Verbose" -abslog=<folder>\pso_<n>_<name>.log`.
   Same route each time: boot, ~10 s at the main menu, start or load the same game, ~3 minutes through a crowd, a
   VFX-heavy moment and the menus, back to the main menu, quit from the menu. Back up `Saved\SaveGames` and
   `Saved\Config` first; do not delete `Saved`. No console typing is needed — the log carries arm / complete /
   timed-out lines and the gameplay-window hitch summary.
6. Read the three logs against the outcome table in the parked plan, write the numbers into `PROGRESS.md`, fix
   `P2_WARMUP_RESEARCH.md` §6 (see Gotchas), then re-plan P2/P3 in plan mode.
7. **NEIL:** push + PRs (on hold since 2026-10-02; the target depends on step 1), superproject pointer bumps, and what
   to do with `Ck_AutoTest_Crowd_AvoidanceVolume_InitialPathAvoidsExpandedObb` (not in `knownReds`; fails alone
   without CkPso loaded).

## Decisions made rather than asking (each has an opt-out)

From 2026-10-02:
- Rewrote this file in place instead of adding a second prompt beside it. Say the word and I split it.
- Refreshed `PROGRESS.md` by hand and left it uncommitted. Say the word and I commit it (that path only).
- In the parked plan: CI is the primary build route and local BuildGraph the fallback; three runs rather than two;
  validation level 2 on all three; CkTests' branch is not part of the measurement build. Each is one line to change.

From 2026-10-01, still standing:
- Gate is default-on with fail-open budgets. Opt-out: `_WaitForPsoPrecache` default Disable.
- Stall = count unchanged for the timeout, not "not decreased". One line in `CkPso_DrainTracker.cpp`, one test block.
- A Complete window only re-opens while the loading screen is showing.
- Subsystem ticks without a game viewport (the headless tests need it).
- `ck.Pso.Debug.StallTimeoutOverride` (non-Shipping) exists so the fail-open test takes about 0.5 s.
- AutoTests renamed `HoldReleasesAfterQuietPeriod` / `HoldFailsOpenOnStall` (the originals were a prefix of one another).
- Committed locally although the full gate was red (8 new failures, each attributed elsewhere). Say the word and I
  soft-reset them.
- Commit messages carry no Co-Authored-By footer (the repo's commit hook says so).
- Baseline failures recorded as known reds after a single run, each reason led by `[owner: <repo>]`.
- The post-change full gate ran on two lanes, not serially like the baseline. A serial re-run is about 1h45m.

## Critical files

- `C:\Users\neilj\.claude\plans\pasted-content-id-26ee-start-with-glimmering-leaf.md` — parked plan: runbook, outcome
  table, recon facts (engine cvars, log format strings with line numbers, packaging traps).
- `Plugins/CkFoundation/Source/CkPso/Claude.md` — module doc: API, cvars, the inert rule, telemetry, ini recipe.
- `.../CkPso/Public/CkPso/Subsystem/CkPso_Subsystem.{h,cpp}` — arming, inert rule, hitch report, every log line the
  measurement reads; the never-compiled validation regions are at `.cpp:22-24, 67-79`.
- `.../CkPso/Public/CkPso/Drain/CkPso_DrainTracker.{h,cpp}` — the state machine; all timing semantics.
- `.../CkPso/Public/CkPso/Settings/CkPso_Settings.{h,cpp}` — project settings and the `ck.Pso.*` cvars.
- `.../CkPso/Public/CkPso/Diagnostics/CkPso_MissLogParser.{h,cpp}` — P3 parser, not wired to anything.
- `Plugins/CkFoundation/docs/campaigns/2026-10-01-CkPso/{PROGRESS.md,P2_WARMUP_RESEARCH.md}` — state and engine research.
- `Plugins/CkTests/Script/CkPso/*.as` — the two AutoTests.
- `D:\Repos\CkPlugins2\AutomationGate.json` — gate roots + 44 known reds (old history only).
- `D:\Repos\BusterBlock_alt\.runreal\buildgraph\Build.xml` and `.runreal\steps\package.steps.ts` — how the game is packaged.

## Ruled out / already proven

- Measuring before the adoption lands, on temp branches off the old history: workable (designed in the parked plan),
  declined by Neil on 2026-10-02 because the machine is busy and the port would then happen twice.
- Checking BusterBlock's CkFoundation out at `90db7a10e` directly: wrong on both checkouts (no shared history on main;
  +89 framework commits of drift on BusterBlock_alt).
- Building in main BusterBlock: it is another conversation's in-flight work.
- A capture device for the measurement: not needed, miss blocks can be aggregated offline from the log.
- Shader precompile, runtime thread-pool sizing, runtime validation toggles, live "repair" sessions, an engine-fork
  miss delegate, `PrecachePSOsBoostToHighestPriority`: dropped, reasons in the original plan.
- CkPso causing any of the 8 new gate failures: ruled out per the table in `PROGRESS.md`.

## Gotchas

New on 2026-10-02:
- **`P2_WARMUP_RESEARCH.md` §6 has an unproven row.** It calls the CkUsf outline compute shaders a miss because
  `r.PSOPrecache.GlobalShaders` is 0; it is 1 on Windows. The file itself is not corrected yet (`PROGRESS.md` notes it).
- **The toolbox has no package verb.** Packaging is BuildGraph (`Package Clients`) or CI, and it needs the engine lock
  exclusively, which the other sessions' test runs starve.
- **`ClientTargetName` must be set to `BusterBlock`** for a local BuildGraph run; the default resolves to a target that
  does not exist (agent-reported, `Build.xml:304`).
- **`Get-ProjectEnginePath.ps1` fails for BusterBlock** (unregistered GUID), and PowerShell script execution is blocked
  by policy on this machine. Hardcode `D:/Repos/UnrealEngine-Angelscript`.
- **Miss blocks are multi-line and unprefixed.** The engine message starts with two newlines, so `PSO PRECACHING MISS:`
  and its fields are on lines without the log prefix. Use `grep -A14`. There is no `-logPSOPrecacheMiss` switch.
- **Packaged builds print raw enum names** (`TimedOut`, `MaxWait`); the editor prints spaced display names. Do not
  write greps from editor output.
- **`-clearPSODriverCache` wipes the GPU vendor's whole DX cache folder** (D3D12 only, top level only). Other running
  UE processes hold files there, which is one reason the runs need a quiet machine. Counts survive a busy machine;
  durations and the 20 ms hitch counter do not.
- **`-ini:` overrides are ignored in Shipping**; the measurement build must be Development.
- **A zero peak everywhere means the build or flags are wrong**, not that there is nothing to precache.
- BusterBlock_alt and BusterBlock share one `.git` object store; a fetch in one moves remote-tracking refs for both.

From 2026-10-01, still true on the old history:
- `--generate` starves on the shared engine lock; a plain `--build` picks up a new module.
- `Ck.Snapshot.Meta.WallTimeReadsAreAllowListed` counts wall-time reads per file and is already red on dev.
- CkFoundation ignores `*.md` outside `.claude/` and `docs/`; a module's `Claude.md` needs `git add -f`.
- Automation matches test names by substring: never name one AutoTest as a prefix of another.
- Removing an AutoTest leaves its placed actor behind; the stale actor runs as a fake pass.
- The editor's source-control integration stages placed-actor uassets during test boots; check the index before committing.
- Every Warning logged while an AutoTest runs fails it; declare expected ones via `Get_ExpectedLogErrors`.
- Headless editor crash `GetRestoredDimensions is not expected to be called on this platform` kills whichever test is
  in flight; worse with more lanes.
