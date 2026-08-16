# PROGRESS — mutable-params-dissolution

Living doc. Newest entries on top within each section. See PROMPT.md for scope, rules and method.

## State

- **Phase:** NOT STARTED. Scope defined 2026-08-06 from the audit at the end of the
  `spec-fragment-granularity` campaign; nothing executed yet.
- **Branch:** `refactor/spec-fragment-params-residues` (CkFoundation), tip `6f5d12b12`, based on
  `dev` at `4afcd039f`. Local only — nothing pushed, no submodule pointer bumps.
- **Baseline to diff against:** 1004 total / 1002 passed / 2 failed. The two reds are the
  pre-existing `PathNetworkFollower_*` pair (sibling navmesh work), NOT ours. Known contention
  flake: `CkJolt_ChaosParity_CcdProjectileStopsAtThinWall` — re-run the full suite before blaming a
  change for it. Full detail in PROMPT.md § Baseline.

## Feature ledger

Order is the recommended one; CkCompass first because it is CkMinimap's twin and the least
defensible thing to leave behind.

| # | Feature | Status | Commit | Gate |
|---|---|---|---|---|
| 1 | CkCompass | not started | — | — |
| 2 | CkUI WorldSpaceWidget | not started | — | — |
| 3 | CkPathNetwork + Follower | not started | — | — |
| 4 | CkGoap Action → CkAStar Params | not started | — | — |
| 5 | CkCameraShake | not started (confirm the mutation exists first) | — | — |
| 6 | CkCrowdAgent | not started (confirm the mutation exists first) | — | — |
| 7 | CkPmg Text | not started | — | — |
| 8 | CkVoxelNavPath | not started | — | — |
| 9 | CkAStar test utils | not started | — | — |

## Log

- 2026-08-06: campaign scoped. Scan A (`TReadWrite<FFragment_*_Params>` in a processor view) is the
  reliable structural tell and returned the six processor-side entries above; scan B
  (`Set_*` onto a retained Params) added three Utils-side ones and ~170 legitimate
  build-a-local-Spec-then-Add hits that are NOT defects. Predecessor campaign fixed CkMinimap
  (`9347e2062`), which is the template.

## Scope (measured 2026-08-06 on the branch)

- **24 Spec-wrapper fragments** (22 files; CkAcceleration and CkVelocity carry two each):
  CkAnimation ×3, CkCamera ×2, CkChaos, CkEntityCollection, CkFx ×2, CkGraphics,
  CkInteraction ×3, CkOverlapBody ×2, CkPhysics ×6, CkResolver ×3.
- **72 `FFragment_*_Current` fragments** to retire (ruling 3). Full list reproducible with:
  `Select-String -Pattern '^\s*struct\s+\w*API\s+FFragment_(\w+)_Current\b'` over `Source/**/*.h`.

~96 conversions total, each needing per-field read analysis, consumer updates across
processor/utils/debugger, and its own gate. This is a multi-session campaign; work it in gated
batches and keep this ledger current, because the checkout is shared and context is lost easily.

## Wrapper residue worksheet (derived 2026-08-06)

For a WRAPPER fragment the analysis is mechanical: every `Get_Params().Get_X()` read in the tree IS
a steady-state read, so that set is the residue and every other Spec field is construction-only.
Regenerate with:

```powershell
Get-ChildItem -Recurse -Include *.h,*.cpp -File |
  Select-String -Pattern 'Get_Params\(\)\.(Get_\w+)' -AllMatches
```

**Validate each hit before trusting it** — the pattern also catches `Get_Params()` on things that
are not our fragment (`CkMeter.cpp|Get_Capacity`, `CkWidgetComponent.cpp|Get_WidgetSpacePolicy` are
known false positives from this run).

| Feature | Residue fields (steady-state reads) |
|---|---|
| CkAcceleration | `_AccelerationParams`, `_TargetChannels` |
| CkAnimPlan | `_AnimGoal` |
| CkAutoReorient | `_ReorientPolicy` |
| CkEntityCollection | `_Name` |
| CkGeometryCollection | `_GeometryCollection` |
| CkInteraction | `_InteractionChannel`, `_Source`, `_Target`, `_CompletionPolicy`, `_Instigator` |
| CkInteractSource | `_ConcurrentInteractionsPolicy`, `_InteractionChannel` |
| CkInteractTarget | `_CompletionPolicy`, `_InteractionChannel`, `_InteractionDuration`, `_ConcurrentInteractionsPolicy` |
| CkMarker | `_AttachmentParams`, `_RelativeTransform`, `_ShapeParams`, `_DebugParams`, `_PhysicsParams`, `_ReplicationType` |
| CkMontagePlayer | `_SkeletalMeshComponent` |
| CkPmg Donut | `_EnableCollision`, `_FillAngle`, `_InnerRadius`, `_Material`, `_OuterRadius`, `_RenderMode`, `_Segments` — likely 1:1 with its Spec, so a strong **alias** candidate (ruling 1) |
| CkResolverDataBundle | `_Causer`, `_Instigator`, `_Phases`, `_Target` |
| CkResolverSource | `_ResolutionPhases` |
| CkSensor | `_AttachmentParams`, `_FilteringParams`, `_RelativeTransform`, `_SensorName`, `_ShapeParams`, `_DebugParams`, `_PhysicsParams`, `_ReplicationType` |
| CkVelocity | `_TargetChannels`, `_VelocityParams` |
| CkVfx | `_AttachmentSettings`, `_ParticleSystem` |

**Wrappers with ZERO `Get_Params()` reads** — CkAnimAsset, CkCameraShake, CkRenderStatus, CkSfx,
CkPredictedVelocity, CkBulkAccelerationModifier, CkBulkVelocityModifier, CkResolverTarget. Their
whole retained Spec is read by nothing. Check whether the fragment survives ONLY as the Has/Cast
anchor; if so the residue may be empty, and the anchor should move to the feature's state fragment
(the Timer pilot's precedent) rather than keeping an empty Params alive.

## Log (cont.)

- 2026-08-06: **CkCamera converted** (`fd38e91bb`) — first application of ruling 1. `FCk_Camera_Spec`
  is two fields; only `_DriveControllerControlRotation` is read back, so an entire
  `FCk_CameraProfile` was retained per camera entity as dead weight. Residue is that one bool.
  **Gate incomplete:** compiles clean (no CkCamera diagnostics), but the full suite was never
  reached — CkTests and CkGameplayDebugger had been moved by a sibling session to
  `feature/usf-celshade-dither-stylize` and `dev`, neither of which pairs with this branch. Restore
  with `git -C Plugins/CkTests switch --detach 2cd9c62` and
  `git -C Plugins/CkGameplayDebugger switch --detach 28fad97`, then re-gate.
- **Submodule pairing is a PRE-GATE CHECK.** Three times this campaign a sibling session moved a
  shared checkout mid-work. Before every gate, verify all three submodules:
  CkFoundation on this branch, CkTests at `2cd9c62`, CkGameplayDebugger at `28fad97`. A mismatch
  produces compile errors in modules you never touched, which reads exactly like your own regression.

## Log (cont. 2)

- 2026-08-06/07: **WRAPPER WORKSTREAM COMPLETE AND GATED.** `30ea12fd3` (the batch) + `ef5a8ffd6`
  (the fixes real compilation found) + CkTests `f9d592cc` + superproject `9261659`.
  **Gate: build clean, zero AngelScript errors, full suite 1004 / 1002 passed / 2 failed** — the two
  being the known pre-existing PathNetworkFollower pair BY NAME with identical assertion text.
  Final tally: **22 of 24 aliased**, 2 reverted (empty Spec), 1 untouched (CkMontagePlayer).

- **RULING 1 NEEDS A THIRD BRANCH: the EMPTY Spec.** `FCk_ResolverTarget_Spec` and
  `FCk_PredictedVelocity_Spec` have no data members at all. `CkRegistry.h` classifies EMPTY types as
  TAGS and static_asserts they derive from `ck::TTag`, so aliasing them turned two fragments into
  tags and broke the build. **The wrapper is load-bearing there** — wrapping an empty Spec in a
  struct with a member makes it non-empty, hence a fragment. Decision tree is therefore:
  - retained fields a strict subset of the Spec → residue struct;
  - retained fields match the Spec 1:1 → `using FFragment_X_Params = FCk_X_Spec;`;
  - **Spec is EMPTY → leave the wrapper.** The fragment carries no data and exists only as the
    Has/Cast anchor. Doctrinally that wants a real `CK_DEFINE_ECS_TAG`, but that changes
    Add/Has/Cast semantics and the anchor — a design call, not a mechanical conversion.

- **PRE-GATE CHECK IS FOUR REPOS, NOT THREE.** The earlier note said "verify all three submodules".
  That was wrong and cost a full gate cycle: the **superproject** is itself a consumer of the
  renamed API (`CkPlugins/Script/PlaceableTests/*.as`), and it was sitting on `dev` while the three
  submodules were on the branch. Worse, the superproject's campaign commit `af63213` was
  **ORPHANED** — `git branch --contains` returned nothing, no ref pointed at it, and it would
  eventually have been garbage-collected. It is now `9261659` on a real superproject branch.
  Before EVERY gate verify all four: superproject, CkFoundation, CkTests, CkGameplayDebugger.

- **What real compilation caught that static analysis could not** (4 classes, 3 compiles):
  1. An alias cannot be forward-declared (`struct FFragment_X_Params;` → C2371, ~46 cascade errors).
  2. Sweeps scoped to CkFoundation/Source MISS CkTests and CkGameplayDebugger — they are separate
     submodules. A "zero residual" audit is only as wide as the tree you walked.
  3. Empty Specs register as tags (above).
  4. Superproject AngelScript on an orphaned commit — and AS failures fail the run via
     `AS_COMPILE_FAILED` (exit 76) with **zero tests executed**, because the editor keeps stale
     bytecode. A suite that reports Total: 0 is not a pass.

## Log (cont. 3) — 2026-08-08: ruling 3 discharged, Params tail closed

- **RULING 3 DONE — `_Current` is purged.** 83 fragment types / 2380 refs across CkFoundation,
  CkTests and CkGameplayDebugger. Target name is the bare `FFragment_[Feature]` from the two-tier
  table; verified zero collisions against all 415 declared fragment names AND every `using`-alias
  first, and confirmed none of the 83 was a reflected USTRUCT (so no BP/serialization exposure).
  Parameter `InCurrent` followed its type to `In[Feature]`.

- **A REGEX THAT MISSES AND AN AUDIT THAT AGREES WITH IT.** The first sweep matched
  `FFragment_([A-Za-z0-9]+)_Current`, which cannot match a feature segment containing an underscore
  — so `Pmg_DebugShape`, `Pmg_Donut`, `Goap_Planner` and `Goap_Action` (246 refs) were silently
  skipped. The audit then reported "0 residual" because **it reused the same narrow pattern**. It
  surfaced only via an unrelated grep. Two rules follow, and they generalize past this campaign:
  1. Feature segments are NOT `[A-Za-z0-9]+`. Use `[A-Za-z0-9_]+` (greedy, anchored on the suffix).
  2. **Never audit with the expression you edited with.** A sweep and its verification must not
     share a regex, or the verification can only confirm the sweep's own blind spot. Verify with a
     different tool (`rg` over the tree) than the one that made the change.

- **SCAN D — direct member writes.** Scans A (writable view), B (`Set_` on a param) and C
  (`Get<>().Set_`) all miss `InParams._Field = ...`, which is legal inside a friend. That form is
  what hid CkCrowdAgent's `_MaxSpeed` and CkPathNetwork's `_Ribbons` through two earlier passes and
  produced two wrong "false positive" calls. Scan D is
  `In(Params|Tunables)\._[A-Za-z0-9]+ *=` — note it also matches `==`, so read the hits.

- **Scans key on the TYPE, not the parameter name.** `CkPathNetwork_Processor.cpp` hosts both the
  network handler and the follower handler, and both name their parameter `InParams` even though
  the follower's type is `_Tunables`. A file-scoped rename would hit the wrong one, so the name was
  left alone deliberately — expect scan B to report it forever. Judge by the fragment type.

- **`_Tunables` is now a documented category, not an ad-hoc escape.** Config that a request or the
  debugger REPLACES at runtime, where there is no immutable residue to split off. Members:
  CkCrowdAgent (19 fields; the Crowd debugger's tuner live-writes 5), CkAStar,
  CkPathNetworkFollower, CkVoiceTalker. Recorded in the root doctrine's two-tier table.

- **Params tail closed.** Split: CkVoxelNavPath (`_Volume` → its own binding fragment, deliberately
  NOT merged with `_Result._Volume`, which records what a COMPLETED plan was made against),
  CkPathNetwork (`_Ribbons` → `_Graph`, beside the `_Network` they build), CkPmg Text (`_Text` →
  `FFragment_Pmg_Text`). Dissolved the last two Spec-wrappers: CkMontagePlayer (one-field Spec, the
  field is a rebindable skeletal mesh) and CkPmg Donut (1:1 and unmutated → alias per ruling 1).

- **Still open, by choice:** CkUI `WorldSpaceWidget` (4 `Set_*`) — deferred at the maintainer's
  request while renames are in flight there. `FFragment_IsmRenderer_Params` keeps its name but is
  NOT a Spec-wrapper: it wraps `TWeakObjectPtr<const UCk_IsmRenderer_Data>`, a data-asset pointer.

- **Toolbox trap: do not redirect a detached run's stdout into `Saved/Logs/`.** The toolbox's
  editor guard probes `Saved/Logs/*.log` for an exclusive write lock. A `Start-Process`
  `-RedirectStandardOutput` pointed there holds such a lock, so the toolbox detects the launcher's
  own redirect file and waits forever on a nonexistent editor. Redirect to the scratchpad. Verify a
  claimed editor two ways before overriding — no `UnrealEditor` process AND `CkPlugins.log` free.

## Log (cont. 4) — 2026-08-08: rebased onto dev, campaign objective met

- **ZERO mutable `_Params` fragments remain framework-wide**, CkUI included. The objective of this
  campaign is discharged; what is left is naming polish, listed at the bottom.

- **Rebasing onto a moving `dev` REINTRODUCES what a rename campaign removed.** `origin/dev` added
  files after this branch forked, so the rename commits never saw them — the rebase landed 47
  dangling `FFragment_{IsmProxy,IskmProxy,OwningActor,Sm}_Current` refs and 5 dangling
  `FCk_Fragment_{IsmProxy,IskmProxy}_ParamsData` refs, all naming types that no longer exist. These
  are compile breaks, not style drift. **After ANY rebase in a rename campaign, re-run every sweep
  and audit before gating** — the branch being green before the rebase says nothing about after.

- **A collision gate can block on the campaign's own work.** The `_Current` gate refused to proceed
  because the bare names were "already declared" — by this branch's own rename. The `_Current` types
  had ZERO declarations left, so the refs were dangling, not colliding. A true collision requires
  BOTH names to still be declared; test that, or the gate stops exactly when it should act.

- **The audit-regex lesson has a second, opposite failure mode.** Earlier the sweep pattern was too
  NARROW (`[A-Za-z0-9]+` missing underscored feature segments). Here the audit pattern was too
  LOOSE: `FCk_Fragment_[A-Za-z0-9_]+_ParamsData` with no trailing `\b` matches
  `FCk_Fragment_ByteAttribute_ParamsDataCustomization` — a detail-customization CLASS name in
  CkAttributeEditor that merely starts with the old type name. It reported 152 phantom "dangling
  refs"; the true count was 0. Anchor both ends, and confirm a hit is a REFERENCE before believing it.

- **Plain string replace is unsafe when one identifier contains another.** `InParams.Get_LocationInfo()`
  contains `Params.Get_LocationInfo()`, so replacing the local `Params.` would have silently rewritten
  the parameter too. The assertion (expected 1, found 2) is the only reason it was caught — use
  `(?<!In)\bParams\.` and keep the counts.

- **Module extraction + rename = modify/delete conflicts, but git's rename detection handles it.**
  `dev` extracted WorldSpaceWidget out of CkUI into its own module. Both conflicts were the same
  shape — `CKUI_API` -> `CKWORLDSPACEWIDGET_API` on one side, the struct rename on the other — and
  the resolution is always "take both". Backups: `backup/prerebase-2026-08-08` in all four repos.

- **CkWorldSpaceWidget split:** 4 of 9 Spec fields carry their own `Request_Set*` (`_LocationInfo`,
  `_ScalingInfo`, `_FadingInfo`, `_OcclusionInfo`) -> `FFragment_WorldSpaceWidget_Tunables`; the
  other 5 stay as the Params residue, which stops aliasing the Spec. `HandleRequests` takes Tunables
  ReadWrite and NO Params — it never read one.

- **Remaining, all cosmetic:** `FFragment_IsmRenderer_Params` (wraps a data-asset pointer, not a
  Spec); `FCk_Fragment_*_ParamsDataCustomization` class names in CkAttributeEditor; and the
  PathNetworkFollower handler's parameter still named `InParams` on a `_Tunables` type (the same file
  hosts the network handler's real `InParams`, so it needs a per-function rename, not a file sweep).

## Log (cont. 5) — 2026-08-08: what the post-rebase compiles caught

Five defects, over three build attempts, none of which any textual audit had flagged. They are
listed with the reason the audit missed each, because the reasons are all different and all
reusable:

1. **`CkPathNetwork_Utils.cpp` called `Get_Ribbons()` on Params** after the field moved to `_Graph`.
   Missed because `rg` is LINE-based and the parameter's type sat three lines above its use inside a
   lambda. **No single-line pattern can bind a use to a type declared elsewhere** — a textual scan
   cannot answer "did I break a consumer of a field I moved". Only the compiler can.
2. **`CkPmg_Fragment_TextShapes.h` friended an undeclared class.** `friend class ::X` does NOT
   introduce `X` at global scope; a new fragment that friends its Utils needs the forward
   declaration in its own header.
3. **`FCk_ObjectiveOwner_ParamsData`** — no `FCk_Fragment_` infix, which every sweep required.
4. **`FCk_Fragment_Goap_ActionParamsData`** — no underscore before `ParamsData`, which even the
   pattern written as the "broadest possible" check still required.
5. **`CkWorldSpaceWidget` HandleRequests DID read a Params field** (`Get_RenderMode`, inside
   SetScalingInfo) after being told it read none.

**The two lessons worth carrying past this campaign:**

- **A count is not a location.** #5 happened because `rg -o 'InParams.Get_X' | uniq -c` was read as
  evidence about WHICH FUNCTIONS used the field. It answers how many, never where. If the decision
  depends on where, the query must return where.
- **Never re-audit with a pattern that shares an assumption with the one that failed.** #3 and #4 are
  the same mistake twice: a regex missed something, the fix was a slightly wider regex carrying the
  same separator assumption, and the re-audit "confirmed clean". The escape is to enumerate the bare
  substring with NO structure assumed —
  `rg -o '[A-Za-z0-9_]*ParamsData[A-Za-z0-9_]*' | sort -u` — read the whole list, and classify each
  entry as type / method / local / comment / string. That enumeration should be the FIRST step after
  a rebase in a rename campaign, not the fifth pattern. Done for both `ParamsData` and `Current`:
  the only surviving hits are method names, locals, comments, test-name strings, and
  `...ParamsDataCustomization` class names.

**Structural checks beat pattern checks.** Before the third attempt, two were run that are worth
keeping as a habit for any view/signature change: (a) every processor's view type order matches its
`ForEachEntity` parameter order, parsed rather than grepped; (b) no `In*` fragment parameter is used
in a function whose signature does not declare it.

## Log (cont. 6) — 2026-08-08: GREEN on the rebased tree

**Gate 5: 1032 total / 1030 passed / 2 failed** — the known `PathNetworkFollower` pair by name
(`DesiredNavmeshClearanceMovesInward`, `ProjectsRibbonWaypointWithinNavQueryExtent`), same assertion
text as the pre-rebase baseline. Integrity on the SAME run: 0 `inline discovery FAILED`,
0 `AS_COMPILE_FAILED`, 0 compile/link errors.

- **Compare failures by NAME, never by count.** The total moved 1004 -> 1032 because dev's commits
  added 28 tests. A count-based "no regressions" check would have read as a 28-test regression, or
  worse, hidden two real ones behind a net-zero.

- **`Build succeeded` + a passing test count is NOT sufficient evidence on this toolchain.** Gate 4
  reported `=== Build succeeded ===`, 0 compile errors, and ran 42 tests — while the editor was
  executing the PREVIOUS build's script bytecode. The tell is in the logs, not the summary:
      toolbox log     : `inline discovery FAILED` / `AS_COMPILE_FAILED`
      per-editor logs : `Angelscript: Error` (Saved/Logs/CkPlugins_N.log, NOT the toolbox log)
  Check those three before believing ANY green run.

- **First editor run after new AS-visible C++ API always fails, by construction.** `Script/Generated/`
  is UNTRACKED and generator-owned; the editor compiles AngelScript BEFORE CkAngelscriptGenerator
  emits updated `utils_*.as` wrappers. Rebasing onto dev brought a new UFUNCTION
  (`Get_AllRibbonsInWorld`, `e9a8b4fa4`) plus a `.as` test calling it, so the first run that reached
  the AS stage could not resolve it. The generator then wrote the wrapper and the next run was clean.
  Diagnosis method that settled it: the wrapper on disk CONTAINED the symbol the error said was
  missing — which is the signature of a generate-after-compile ordering issue, not a code defect.
  Do not "fix" the script; re-run.

## Log (cont. 7) — 2026-08-16: second rebase onto dev (+126 commits), and the scan that was blind

Rebased all four repos onto `origin/dev` (CkFoundation 126 behind / 36 ahead, CkTests 74/7,
CkGameplayDebugger 125/10, superproject 17/3). Backups: `backup/prerebase-2026-08-16` in all four.

- **A rename commit's conflicts have ONE correct resolution, and it is mechanical.** Take the DEV
  side and re-apply the rename to it. Taking "ours" (the rename commit) silently deletes whatever
  dev added to that file — `CkTween_Processor.h` would have lost a whole new `DoComputeValue`
  overload. Before trusting that resolution, PROVE the commit was a pure rename for that file:
  apply the derived map to the commit's own pre-image and require it to reproduce the post-image
  byte-for-byte. It held for 19/19 files (`_Current`), 7/7 (`_Tunables`), 2/2 (CkTests). Where it
  does NOT hold, the commit did more than rename and the file needs reading.
- **Derive the rename map from the commit, never from the naming rule.** The `_Current` retire
  looks like "strip the suffix", and for all 82 TYPES it was. The PARAMETER renames were not:
  CkGoap's `InCurrent` became `InPlannerComp`, not `InGoap_Planner`. Diffing the commit's own
  pre/post identifier sets per file produces the real map; assuming the rule would have been wrong
  in exactly the files nobody would re-check.
- **The superproject's two submodule-pointer commits are derived state.** After rebasing the
  submodules, resolve the first bump to the new tips and SKIP the second — it replays as empty.
  Machine-local dirt (EngineAssociation GUID, editor-generated gameplay tags) stashes and pops
  cleanly; dev happened to bump `CkAuto` to the same SHA the local dirt already had.

### The coverage gap: two whole modules arrived pre-campaign-shaped

`CkInput` and `CkIntent` landed on dev after the P3 sweep, so nothing had ever converted them:
7 features carrying both `FCk_Fragment_X_ParamsData` and `FFragment_X_Current`. Converted here —
6 keep the earned `using FFragment_X_Params = FCk_X_Spec;` alias (nothing mutates them), and
`_Current` becomes the bare `FFragment_X`. CoreRedirects added for all 7 Specs.

- **`CkInputBias` was a genuine mutable-`_Params` defect, the campaign's core class, reintroduced.**
  `Request_SetAxisBias` edits the table in place, which is why `FProcessor_InputBias_HandleRequests`
  declared Params `TReadWrite` and why the reflected Spec FRIENDED a processor. The whole Spec is
  the tunable — there is no immutable residue to split off — so it becomes
  `FFragment_InputBias_Tunables` (the CkCrowdAgent/CkAStar precedent), the Spec drops the friend,
  and `Add` unpacks `Get_AxisBiases()` rather than handing the fragment its Spec.

### SCAN E — all four earlier scans are blind to container mutation

Scans A–D missed `CkInputBias` at the source level. The mutations are:

```cpp
InParams._AxisBiases[ExistingIndex] = AxisBias;   // scan D wants \._Field *=, the [i] breaks it
InParams._AxisBiases.Emplace(AxisBias);           // no scan looks for a mutating METHOD at all
```

Scan D (`In(Params|Tunables)\._[A-Za-z0-9]+ *=`) only matches assignment to a WHOLE member. A
container member mutated by index, or by `Add`/`Emplace`/`Remove`/`Reset`/`RemoveAll`, is invisible
to every pattern scan written so far. **Stop pattern-matching the mutation and match the STRUCTURE
that permits it instead** — three queries, all of which the compiler enforces and none of which can
be evaded by how the write is spelled:

```
A) TReadWrite<\s*(ck::)?FFragment_\w*_Params\s*>        a processor DECLARING the write
B) (?<!const )FFragment_\w*_Params\s*&\s*In\w*          a non-const Params parameter
C) auto\s*&\s*\w+\s*=\s*\w+\.Get<\s*(ck::)?FFragment_\w*_Params\s*>   a non-const local
```

Run over CkFoundation + CkTests + CkGameplayDebugger this returns 1 / 4 / 4 real hits.

### Two PRE-EXISTING defects this surfaced — both on `origin/dev`, neither caused by the rebase

The earlier claim "ZERO mutable `_Params` fragments remain framework-wide" is **false**. Scan C
finds two more, and both are the `Handle.Get<>` shape of Workstream 3, which the log recorded as
discharged. Verified present on `origin/dev` itself, so they are not rebase regressions:

| Feature | Evidence | Note |
|---|---|---|
| `CkCameraLayer` | `CkCamera_Processor.cpp:110-112` (`Set_Priority`, `Set_CameraTarget`), `CkCamera_Utils.cpp:51-52` (`Set_IsDefault`, `Set_Priority`) | Listed verbatim in this PROMPT's Workstream 3 and never actually fixed. `FFragment_Camera_Params` WAS converted; `FFragment_CameraLayer_Params` is a different fragment and slipped through on the name. |
| `Ck2dGridCell` | `Ck2dGridCell_Utils.cpp:171,195` — `Params.Get_Tags().AddTag/RemoveTag` from `Request_AddTag`/`Request_RemoveTag` | Not on any list. `CK_PROPERTY_GET` handed out a non-const ref through a non-const local, so no `Set_` and no `_Field =` ever appears. |

Both are left for their own commit and their own gate, per the one-feature-per-commit rule.
**The lesson is the same one twice: a converted `FFragment_Camera_Params` does not mean
`FFragment_CameraLayer_Params` was looked at.** Audit the fragment list, not the feature list.

## Log (cont. 8) - 2026-08-16: gate on the twice-rebased tree

**Build: succeeded, 0 errors.** **Suite: 1032 total / 1030 passed / 2 failed / 0 skipped /
0 contaminated**, 4m08s. The two are the known PathNetworkFollower pair BY NAME
(`DesiredNavmeshClearanceMovesInward`, `ProjectsRibbonWaypointWithinNavQueryExtent`) - identical to
the pre-rebase baseline. Integrity on the SAME run: 0 `inline discovery FAILED`,
0 `AS_COMPILE_FAILED`, 0 compile/link errors.

- **The AS generate-after-compile re-run is now confirmed twice.** The first test run exited 76
  (`AS_COMPILE_FAILED`, "kept STALE bytecode... counts are MEANINGLESS") with
  `Namespace 'utils_input_bias' doesn't exist`. Renaming AS-visible types invalidates the untracked,
  generator-owned `Script/Generated/` wrappers, and the editor compiles AngelScript BEFORE the
  generator rewrites them. Diagnosis method, unchanged and worth keeping: check the wrapper ON DISK
  for the symbol the error says is missing. It was there, freshly written during that very run. Do
  not touch the script; re-run. The second run was clean.

### The count matched the baseline exactly - and that is the thing to distrust

1032 before the rebase, 1032 after +126 dev commits. Equal totals across that much upstream work is
not reassurance, it is a question. The answer: **97 CkInput/CkIntent AutoTests exist and NONE of
them run.** They compile (AngelScript emits warnings citing them by path, and CkInput's runtime
logs appear in every lane), they derive from `UCk_AutoTest_Base` exactly like tests that do run -
and no automation entry is ever created for them. Zero occurrences of `InputBias` in a 4-minute
suite log that boots CkInput three times.

Pre-existing, not caused by this work, and the evidence is positive rather than an alibi:
`git status` reports **0 asset/level changes across all four repos**, the tests were authored on dev
on 2026-08-08 (`69ba4a74`), and the Gate-5 baseline recorded here on 2026-08-08 was ALREADY
1032/1030/2 with the same two names. They have never run in this suite.

**Consequence for this campaign, stated plainly: the CkInput/CkIntent conversion is
COMPILE-verified and NOT runtime-verified.** A green suite says nothing about a path it does not
exercise. `CkInputBias`'s split is the part that actually changes behaviour - the retune request now
writes `FFragment_InputBias_Tunables` instead of the retained Spec - and the seven AutoTests that
would pin it (`InputBias_RetuneAppliesToNextEvent` foremost) are among the 97 that never execute.

`[EDITOR-VERIFY]` / follow-up, whoever owns CkTests wiring: find out why a `UCk_AutoTest_Base`
subclass under `Script/CkInput/` produces no automation entry, then re-gate. Until then, treat
CkInput and CkIntent as untested by the suite regardless of what the totals say.
