# PROMPT — mutable-params-dissolution

Follow-up campaign to `spec-fragment-granularity`. Self-contained: everything needed to execute is
below. Do not assume access to the originating conversation.

## Mission

Find and fix every feature whose **retained `_Params` fragment is mutated at runtime**. A fragment
named `_Params` is a promise of retained immutable config; where a request writes to it, the promise
is false and the feature has no single home for that datum.

This is NOT a campaign to convert wholesale `using FFragment_X_Params = FCk_X_Spec;` aliases. That
alias is *correct* wherever every Spec field is read at steady state (root CLAUDE.md, "Retained
immutable config … All-hot features may alias"). Converting one without evidence is churn.

## Why this class matters (the worked exemplar)

`CkMinimap`, fixed on this branch in `9347e2062`, carried both directions of the defect at once:

- `_ViewExtent` / `_RotationMode` were copied into `Current` at Setup, so the authored copies left
  behind in the retained Spec went stale.
- `_CategoryFilter` was mutated **in the Params fragment**, through the reflected Spec's own setter
  (`InParams.Set_CategoryFilter(...)`). That is the ONLY reason `FProcessor_Minimap_HandleRequests`
  declared Params `TReadWrite`. "Params" was a mutable fragment in all but name.

Fix shape: every request-mutable field moved to the state fragment; Params became a genuine residue
`{_ProjectionMode, _FrameShape, _FixedBounds, _MaxEntries, _UpdateInterval}`; HandleRequests took it
`TReadOnly`; authoring ensures moved to `Add`, where the Spec still exists. Read that commit before
starting — it is the template for most of the list below.

## The two scan signatures (re-run these; do not trust this list as final)

Run from `Plugins/CkFoundation/Source`. PowerShell + `Select-String`, NOT the Grep tool — the
superproject `.ignore` can silently false-empty under this plugin.

```powershell
# A) the reliable structural tell: a processor declaring a Params fragment writable
Get-ChildItem -Recurse -Include *.h -File |
  Select-String -Pattern 'TReadWrite<\s*FFragment_\w*_Params\s*>'

# B) runtime Set_* onto a RETAINED Params fragment.
#    Only hits where the target came from an entity are defects:
#      InParams.Set_*            inside a processor  -> DEFECT
#      Handle.Get<...Params>().Set_*                 -> DEFECT
#      LocalSpec.Set_*  then  Add(Handle, LocalSpec) -> FINE, that is authoring
Get-ChildItem -Recurse -Include *.cpp -File |
  Select-String -Pattern 'InParams\.Set_|Params\.Set_|_Params>\(\)\.Set_'
```

Scan B returns ~180 hits and the large majority are the FINE case (CkPmg shape utils, CkVoxelNav
ChunkParams, CkVoiceChat/CkCrowd ProbeParams, CkGrid, CkInventory, CkDialog, CkUnrealComponent,
CkEntityVisualizer). Classify before acting; do not bulk-edit scan B.

## Scope — the list as of 2026-08-06

Confirmed by scan A (processor declares Params writable):

| # | Feature | Evidence | Notes |
|---|---|---|---|
| 1 | **CkCompass** | `CkCompass_Processor.cpp:128` `InParams.Set_CategoryFilter(...)`; view `CkCompass_Processor.h:44` | **Do this FIRST.** Minimap's 1D sibling, same field, same mechanism; its CLAUDE.md states it mirrors Minimap's delivery contract. Mirror `9347e2062` closely. |
| 2 | **CkUI** WorldSpaceWidget | `CkWorldSpaceWidget_Processor.cpp:305,317,346,358` (`Set_LocationInfo/ScalingInfo/FadingInfo/OcclusionInfo`); view `:69` | Four struct-valued config blobs, all request-mutable. Likely wants a `_Tunables`-style state fragment (VoiceTalker precedent) rather than folding into a `_Current`. |
| 3 | **CkPathNetwork** + Follower | `CkPathNetwork_Processor.cpp:2037-2051` (13 `Set_*`, a tuning-application block); `CkPathNetwork_Utils.cpp:791` `Set_Network`; views `Processor.h:44,106` | Largest. **Hazard:** the two known-red tests are `PathNetworkFollower_*` — see Baseline. Do not read those reds as caused by this work, and do not assume they mask a new one; diff failing names. |
| 4 | **CkGoap** Action → AStar | `CkGoap_Action_Processor.cpp:609,613` (`InAStarParams.Set_BudgetMicroseconds/CostThreshold`); view `Action_Processor.h:86` | **Cross-module:** Goap mutates *CkAStar's* Params fragment. Decide whether the budget/threshold are AStar state or a Goap-owned override; do not unilaterally reshape CkAStar's public surface. |
| 5 | **CkCameraShake** | view `CkCameraShake_Processor.h:16`, signature `:32` | Scan A only — find the mutation before designing. May be a false positive (writable view, nothing writing). |
| 6 | **CkCrowdAgent** | view `CkCrowdAgent_HandleRequests_Processor.h:22`, signatures `:41,:86` | Same: confirm the mutation first. |

Utils-side mutations of a retained Params (scan B, confirmed defect shape):

| # | Feature | Evidence | Notes |
|---|---|---|---|
| 7 | **CkPmg** Text | `CkPmg_Utils_DebugShapes.cpp:94` `InHandle.Get<FFragment_Pmg_Text_Params>().Set_Text(...)` | A legitimate change-the-text-at-runtime API pointed at the wrong fragment. |
| 8 | **CkVoxelNavPath** | `CkVoxelNavPath_Utils.cpp:112` `InPath.Get<...Params>().Set_Volume(...)` | |
| 9 | **CkAStar** | `CkAStar_Test_Utils.cpp:312,327` | Test-only utils; fix alongside #4 or note as intentional test seam. |

## Per-feature method

1. Re-run both scans for that feature; read every hit. The list above is a starting point, not truth.
2. Classify each Params field: **construction-only** (dissolve — tag, state seed, or drop),
   **steady-state read** (keep in the residue), **request-mutable** (move to the state fragment).
3. Decide the destination for mutable fields: an existing `_Current`/state fragment, or a new
   purpose-named one. `FFragment_VoiceTalker_Tunables` (this branch, `82a58b252`) is the precedent
   for "the block requests mutate". `_Params` never holds it.
4. Unpack in `Add`/`Create`. Move any authoring ensure there too — it then fires at compose time
   rather than a tick later, which is strictly better.
5. Narrow the processor views: Params to `TReadOnly`, drop it entirely from processors that no
   longer read it (P4 precedent: dead Params view members are removed, not left).
6. Update the module `CLAUDE.md` wherever it now asserts something false, and any anti-pattern entry
   that the change turns into a compile error.
7. Build + FULL suite gate. Commit. Next feature.

## Rules

- **Branch:** `refactor/spec-fragment-params-residues` in the **CkFoundation submodule**
  (`D:\Repositories\CkRepos\CkPlugins\Plugins\CkFoundation`), based on `dev` at `4afcd039f`.
  Continue on it. **Do not push** without explicit instruction; no submodule pointer bumps.
- **Builds and tests are toolbox-only**, launched from the CkPlugins root:
  `./CkAuto/UnrealToolbox.exe --build --target=Editor --project='D:\Repositories\CkRepos\CkPlugins'`
  then `--test` for the full suite. Never `Build.bat`, UnrealBuildTool, or `UnrealEditor-Cmd`
  directly. Do NOT pass `--config` (follow last-built = Development; a config flip is refused with
  exit 79). Do NOT pass `--generate`.
- **Gate every feature separately** with a full suite run, and commit per feature.
- **Launch gates detached** (`Start-Process pwsh -WindowStyle Hidden`) writing a verdict file, and
  watch that file. A foreground/background toolbox run is killed by the harness at 10 minutes,
  mid-test, with no notification.
- **Editor-lock preflight** before any toolbox run that spawns the editor:
  `[IO.File]::Open('<CkPlugins>\Saved\Logs\CkPlugins.log','Open','Write','None')` — if it throws,
  another editor is up; wait, never kill it.
- **Never rename UFUNCTION parameters or UPROPERTY members.** Reflected Spec fields stay as authored;
  this campaign changes what is RETAINED, not the authoring surface.
- **A fragment must never know its Spec** — `CK_DEFINE_CONSTRUCTORS` over its own members, mapping
  in the factory. Root CLAUDE.md records this rule and its corollary (a "Params" fragment a request
  mutates is mislabelled). That corollary IS this campaign.
- **CRLF.** House style is CRLF and the repo stores endings literally. A scripted patcher that
  normalizes to `\n` and forgets to restore will rewrite every line and bury the real diff — this
  happened on this branch and had to be amended out. After any scripted edit, verify:
  `git show HEAD:<path> | grep -c $'\r'` must equal `| wc -l`.
- **Stage only files you changed.** `docs/digests/` and `Tools/` are untracked and NOT ours — never
  stage them. Never blanket `git add <dir>`.
- **Sibling checkouts.** `BusterBlock`, `BusterBlock_5.5`, `CkPlugins_Other` (D:\Repositories\CkRepos)
  and `D:\Repositories\Venus` each hold their own CkFoundation working copy with local commits.
  Before touching a module, probe for contention:
  `git -C <path> status --short -- Source/<Module>` and
  `git -C <path> log --oneline origin/dev..HEAD -- Source/<Module>`.

## Baseline (measured 2026-08-06, branch tip `6f5d12b12`)

- Full suite: **1004 total / 1002 passed / 2 failed / 0 skipped / 0 contaminated**, ~4m10s.
- The 2 failures are **pre-existing and not ours** — owned by the sibling navmesh work:
  - `Ck_AutoTest_PathNetworkFollower_ProjectsRibbonWaypointWithinNavQueryExtent`
  - `Ck_AutoTest_PathNetworkFollower_DesiredNavmeshClearanceMovesInward`
- **Known flake:** `Ck_AutoTest_CkJolt_ChaosParity_CcdProjectileStopsAtThinWall` fails
  intermittently under 3-lane parallel contention. It reports NO assertion, ensure, or error in its
  window — that silent profile is the tell. Verified 2026-08-06: passes 2/2 in isolation and the
  full-suite re-run on the same binary returned 1002/1004. **Before blaming any change for it,
  re-run the full suite on the same binary.**
- "No regressions" means the failing-test NAMES match the two above. A count alone is not enough —
  a flake can mask a real red at equal count.

## Definition of done

Scan A returns zero hits, or every remaining hit is recorded in `PROGRESS.md` with the evidence for
why it is legitimate. Each fixed feature has its own commit and its own green gate. Module docs no
longer assert anything the code contradicts.

## Out of scope

- Converting wholesale Params aliases that nothing mutates. ~50 features hold one; doctrine permits
  it when every field is read at steady state. The count of actual offenders in that *second* class
  (Params retaining construction-only fields — tidiness, no divergence risk) is **unknown and
  unaudited**; it needs per-field read analysis per feature. Do not guess at it.
- Removing dead reflected Spec fields. `FCk_VoiceTalker_Spec::_AutoJoinChannels` is authored and read
  nowhere in the framework, deliberately left alone: removing an authoring field is the owning
  module's call.
- The `spec-fragment-granularity` campaign's open `[EDITOR-VERIFY]` (BP redirects + 16-asset resave)
  and its shipping. Tracked in `docs/campaigns/spec-fragment-granularity/PROGRESS.md`.

## Reading order before writing code

1. `Plugins/CkFoundation/CLAUDE.md` — Spec unpacking / data-placement doctrine, and the
   fragment-never-knows-its-Spec rule.
2. `docs/specs/2026-08-05-config-naming-and-fragment-granularity-design.md` §4.
3. `docs/campaigns/spec-fragment-granularity/PROGRESS.md` — the 2026-08-06 P6b entry records this
   audit, the triage of all seven original hits (including three that were false positives on
   inspection), and the coverage answer.
4. `git show 9347e2062` (CkMinimap) — the template. Then `82a58b252` (VoiceTalker `_Tunables`) and
   `b0da2190d` (PoiDisplayDefinition, the smallest clean example).
