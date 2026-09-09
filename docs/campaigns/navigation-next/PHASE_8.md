# PHASE 8 — Parity measurement, promotion, and retirement

> **Freshness:** authored 2026-08-31 during the P4 planning package (documentation-only session,
> [NN-D3]). **Status of record: [PROGRESS.md](PROGRESS.md).**
> **Authoritative upstream:** [PROMPT.md](PROMPT.md), [MIGRATION_SEAM.md](MIGRATION_SEAM.md) §4
> ([NN-D8], incl. [NN-D8f]), [FEATURE_MATRIX.md](FEATURE_MATRIX.md) (F2.20–F2.23),
> [VALIDATION.md](VALIDATION.md) (**§3** shadow protocol, **Tier B** promotion, **Tier C**
> retirement, **§4** `[EDITOR-VERIFY]`, **§5** packaging/teardown/isolation),
> `research/R3-coupling-inventory.md` (the `[ADAPTER]` / `[RETIRE]` / `[REIMPLEMENT]` buckets, row by
> row). Line references are a snapshot @ CkFoundation `a25ec9539` — re-verify before deleting
> anything.
>
> **This phase contains two sign-offs and they are never granted together.** Promotion says "the new
> provider is good enough to lead". Retirement says "the old one is no longer needed". Different
> claims, different evidence, separate reviews (VALIDATION.md §6 (item 3)). Conflating them removes the
> rollback path that makes the whole migration safe.

---

## 1. Goal and feature ids

| Sub-phase | Features / gates | Goal |
|---|---|---|
| **8A** | **F2.21, F2.22** + VALIDATION §3 | Run the shadow / A-B protocol across the named suites; **fill every `[MEASURE]` budget** in VALIDATION.md with a measured Recast baseline **and** a measured CkGroundNav number. |
| **8B** | Tier B (**B1–B8**) | **Promotion**: CkGroundNav becomes the default provider while Recast remains present and selectable. Ends in a **CTO sign-off** recorded as a numbered decision. |
| **8C** *(separate sub-phase, separate CTO sign-off)* | **F2.20, F2.23** + Tier C (**C1–C9**) | **Retirement**: delete the `[ADAPTER]`/`[RETIRE]` sites, drop `NavigationSystem`/`AIModule`, delete the legacy navmesh draw module ([NN-D8f]), reconcile the docs, full-suite delta-zero on the final artifact. |

---

## 2. Entry criteria — confirm all before writing a line

### 2.1 For 8A/8B (promotion track)

- [ ] **All Tier A gates A1–A8 green** with evidence recorded in PROGRESS.md. **A9 (P7) is not
      required for promotion** — if PHASE_7 was deferred, its deferral is recorded as a numbered
      decision and Tier C is blocked on it (see §2.2).
- [ ] **Shadow mode (F1.40) is operational** and its diagnostics fragment is **value-only**; the
      debugger renders it (P6/6C).
- [ ] **Session-start ritual** run verbatim (PROMPT.md §7) with two "Done"-claim spot-checks.
- [ ] **Phase-entry baseline captured this session** — full toolbox `--build --test`, `--parallel 1`,
      `--no-nullrhi`; totals **and failing-test names** and the artifact identity recorded, with the
      inherited `Nav.Filter.Customer` mapping failure recorded **by name**.
- [ ] **Every `[MEASURE]` placeholder in VALIDATION.md is enumerated into a worksheet in
      PROGRESS.md** before any measuring begins — one row per budget, naming the fixture, the metric,
      and the artifact it will be measured on. A budget discovered mid-run is a budget that was never
      designed.

### 2.2 For 8C (retirement track) — additional, and all of them

- [ ] **B1–B8 green and promotion signed off** as its own numbered decision.
- [ ] **Promotion has been live through at least one full campaign phase** (VALIDATION Tier C
      preamble). Not "a while". A named phase.
- [ ] **A9 / PHASE_7 complete.** Retirement removes Recast's cook-time bake; PHASE_7 is what replaces
      it. If P7 was deferred, **Tier C does not start** — record the block and end the session.
- [ ] **C1 and C2 prerequisites landed in P5**: CkPathNetwork fully migrated off the safety oracle
      and the connector-path builder; editor authoring snap migrated.

**Verify commands:**

```powershell
Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test --parallel 1 --no-nullrhi `
    --output=Saved/Logs/P8-Baseline.log --project="D:\Repos\CkPlugins_3"

# The retirement symbol census — record these counts at entry; R3's snapshot was 60/48/25/45/1/1
rg --no-ignore -c 'UNavigationSystemV1|ARecastNavMesh|UNavigationQueryFilter|UNavArea|INavRelevantInterface|FPathFindingQuery' `
    Plugins/CkFoundation Plugins/CkTests Plugins/CkGameplayDebugger

# The dependency rows Tier C removes
rg --no-ignore -n 'NavigationSystem|AIModule' Plugins/CkFoundation/Source/**/*.Build.cs
```

---

## Standing decision gate (applies to EVERY `-> verify:` line in this file)

Read this once; it is the default branch set for every verification step below. Steps that need
extra branches carry their own **Gate** block.

- Observation matches the stated expectation → continue to the next step.
- Build fails to compile → fix mechanically (missing include, moved symbol, UHT complaint) and
  re-verify. **Two failed attempts at the same step → STOP.**
- A test that was red in the P8 phase-entry baseline is still red with the same name → not yours;
  continue, and carry it forward in the delta-zero comparison.
- A test that was green in the baseline is now red → **STOP**, restore the known-good state (revert
  your last step), then diagnose before re-applying.
- A test that was red in the baseline is now green → **STOP**. An unexplained improvement is as much
  a defect as a regression — explain it or record it (VALIDATION.md A1).
- **Any observation not enumerated above → STOP, record it in PROGRESS.md § Blockers with verbatim
  evidence, end session.**

---

## 3. Sub-phase 8A — parity measurement

**Parity evidence is produced by running suites in shadow mode — never by demonstrating a working
scene** (VALIDATION §3). A gym that looks right, a video, or a hand-picked query is not admissible
here, and the correct response to anyone offering one is to ask for the report.

1. **Run the shadow suites**, all of them, per VALIDATION §3.1: the full **CkCrowd** suite (movement,
   containment, steering, block detection, path refresh, avoidance sampling, avoidance volumes,
   obstacle fixtures); the full **CkQueue** suite; the full **CkPathNetwork** suite; the **CkEqs**
   projection post-pass tests; the **CkGroundNav** L2 autotest family itself; and the crowd, queue
   and path-network **gyms** and test maps run manually under shadow mode for the `[EDITOR-VERIFY]`
   legs.
   → *verify:* every suite ran with shadow mode **enabled**, `--parallel 1`, on a **stated artifact**;
   the run's artifact identity is recorded beside the report.
   → *rollback:* shadow mode is a setting — disable it and re-run to return to the pre-measurement
   behaviour. Measurement changes no shipped behaviour, so this step's rollback is a setting flip.

2. **Record every §3.2 metric per comparison**, aggregated per map and per suite: success/failure
   agreement (with the **name** of every disagreeing test), fail-reason agreement, path-length delta
   (absolute and relative; per-map mean, p95, max), endpoint delta, waypoint-count delta, containment
   escapes, query time (both providers; mean, p95, max), partial-path agreement.
   → *verify:* the counters accumulate into the **value-only diagnostics fragment** (§3.3) — no
   handles, no `UObject`s, no shared field structures — and the debugger renders it.

3. **Emit the shadow report into PROGRESS.md**: one row per map, every metric, plus the artifact
   identity. **This report *is* the B1 promotion evidence.**
   → *verify:* the report exists, is complete, and names its artifact.

4. **Fill every `[MEASURE]` budget** in VALIDATION.md from the §2.1 worksheet: A3 collapse-factor
   floor and bake cost/memory; A4 constrained-surface-walk endpoint agreement and raycast hit
   tolerance; A5 funnel improvement floor and search cost; A6 markup-live latency and
   moved-obstacle repair latency; B1 per-map path-length and endpoint budgets; B3 dynamic latencies;
   B4 query-per-frame, bake and repair costs. **Each is measured on the Recast path, on the same
   fixture, and paired with a measured CkGroundNav number, both dated and reproducible** (F2.21,
   F2.22).
   → *verify:* zero `[MEASURE at phase entry — no estimated numbers]` placeholders remain in
   VALIDATION.md; every filled cell names its fixture, its artifact, and its date.

### Decision gate 8A → 8B

- **Zero disagreements** (not a percentage — every test agrees), **zero containment escapes**, every
  delta inside its measured budget, and every `[MEASURE]` filled → **proceed to 8B**.
- **Any success/failure disagreement, any fail-reason disagreement, or any partial-path disagreement**
  → it is **enumerated and individually adjudicated in PROGRESS.md**, never summarized away. If the
  adjudication concludes "CkGroundNav defect", → **STOP**, record, end session; the fix belongs to the
  owning phase, not here.
- **Any containment escape** → **STOP** immediately, record the frame and the agent, end session.
  A containment escape is a hard promotion blocker (VALIDATION §3.2, A4).
- **A systematic waypoint-count skew** → investigated **before** promotion; record the investigation
  or **STOP**.
- **A budget cannot be measured** (no fixture, no harness) → **STOP**, record, end session. An
  estimated number is not a budget and must never be written into VALIDATION.md.
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 4. Sub-phase 8B — promotion

Promotion is **one setting change plus the evidence that earns it**. Nothing is deleted here.

1. **B7 first — prove the rollback before using it.** Flip the provider setting to CkGroundNav, then
   flip it back to Recast and run the full suite green on the **same artifact**.
   → *verify:* a full suite green with the setting reverted, recorded with its artifact.
   → *rollback:* this step **is** the rollback proof. If it does not hold, promotion stops here.

2. **B2 — fixture vocabulary fully served.** Every crowd obstacle test green on CkGroundNav via
   **neutral markup**; the enumerated list of the 10+ `UNavArea_Null` fixture sites, each with its
   migrated test name and result.
   → *verify:* the enumerated list in PROGRESS.md, no site unaccounted for.

3. **B3 — dynamic gates.** Markup-live latency and moved-obstacle repair latency **≤** the
   Recast-measured baseline on the same fixtures.
   → *verify:* the recorded pairs of numbers per fixture.

4. **B4 — performance.** Query-per-frame budget adherence at the reference agent count; bake and
   repair cost within budget; **no frame-time spike beyond the processor budget**. All measured on a
   stated artifact.
   → *verify:* the recorded numbers, produced by the PerfLab surveys P6/6F migrated.

5. **B5 — both debugger surfaces at parity** (runtime in-world draw + the CkCrowdDebugger adapter) in
   **PIE**, in a packaged **Development** build, and in a packaged **Test** build.
   → *verify:* `[EDITOR-VERIFY]` §4.2 and §4.4 recorded with verifier and artifact; packaged runs
   produced on the **build machine** (§7).

6. **B6 — multi-PIE and teardown clean.** No cross-world leaks, no ensures on world death, per-world
   state proven isolated.
   → *verify:* the automated teardown test of VALIDATION §5 plus `[EDITOR-VERIFY]` §4.5 (including
   five PIE start/stop cycles with steady memory and log behaviour).

7. **Flip the default.** Provider selection stays data-driven; only the **default** changes.
   → *verify:* the default is a setting, not a code path; the Recast branch remains selectable and
   green.
   → *rollback:* **one setting revert**, proven in step 1. Nothing else is required, because nothing
   has been deleted. State this in the sign-off request.

8. **Request the CTO sign-off**, presenting: the §3.3 shadow report, the filled `[MEASURE]` table, the
   B2 fixture list, the B3/B4 numbers, the B5/B6 `[EDITOR-VERIFY]` records, and the B7 rollback proof.
   → *verify:* **B8** — the sign-off is recorded as a **numbered decision** in PROGRESS.md.

### Decision gate 8B → 8C

- **B1–B8 all green and promotion signed off** → 8B closes. **8C does not start in the same session
  or the same review.**
- **Any B-gate red** → **STOP**, record which, end session. Promotion is not partially granted.
- **The CTO grants promotion and retirement together** → **STOP** and return to the orchestrator:
  VALIDATION.md §6 (item 3) requires them to be separate, later reviews, and an executor may not accept a
  combined grant. Record the request verbatim.
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 5. Sub-phase 8C — Recast retirement (separate sub-phase, separate CTO sign-off)

> **Do not enter without §2.2 satisfied.** From here on, each step **removes** the rollback path it
> touches, so each step ships as **its own commit** with an explicit revert instruction, and the
> phase's branch is a `feature/` branch (additive campaign work) with a **named backup branch** taken
> before any history rewrite.

**Standing rollback for the whole sub-phase:** every deletion is one commit; the rollback for any
step is `git revert <that commit>` (or `git -C Plugins/<Submodule> revert <sha>` for a submodule
commit), followed by a full-suite re-run. Nothing in 8C is squashed until the sub-phase closes,
precisely so a single bad deletion is revertible without unpicking the rest.

1. **C1 / C2 confirmation (no work, pure verification).** CkPathNetwork carries **zero** remaining
   calls to the Recast safety oracle and the connector-path builder; the editor authoring snap no
   longer reaches Recast.
   → *verify:* `rg --no-ignore -n 'Get_DefaultRecastNavmesh|Is_NavmeshSegmentDirectlyWalkable|NavMeshRaycast|FindPathSync|ProjectPointToNavigation' Plugins/CkFoundation/Source/CkPathNetwork Plugins/CkFoundation/Source/CkPathNetworkEditor`
   → zero hits.
   → *rollback:* none needed; this step changes nothing.

2. **Delete the `[RETIRE]` sites** — `CkCrowdAgent_DiagNavClip_Processor.cpp`,
   `CkCrowdAgent_DrawNavProjection_Processor.cpp`, and the debug-settings probe
   (`Settings/CkCrowd_DebugSettings.cpp:124`). These are **deleted, not migrated** ([NN-D8], A2).
   → *verify:* grep confirms deletion; targeted CkCrowd family green.
   → *gate:* verify the deletion target matches the R3 bucket tag and the full-suite gate stays
   delta-zero after the commit; any unexpected diff or test movement → **STOP + revert the step**.
   → *rollback:* revert this commit; the processors return with their registrations.

3. **Delete the `[ADAPTER]` sites, one R3 table row at a time**, annotating the R3 table as each row
   reaches zero. Includes the five framework `UNavArea`/`UNavigationQueryFilter` subclasses
   (`UCk_NavArea_Restricted`, `UCk_NavArea_CrowdAgent`, the three avoidance-volume areas,
   `UCk_NavQueryFilter_AvoidStandingCrowds`) and `Request_SetActorNavigationRegistered` ([NN-D8c]).
   **No back-compat shims** — deleted, not deprecated.
   → *verify:* **C3** — the R3 table annotated to **zero remaining adapter rows**; targeted families
   green after each row.
   → *gate:* verify the deletion target matches the R3 bucket tag and the full-suite gate stays
   delta-zero after the commit; any unexpected diff or test movement → **STOP + revert the step**.
   → *rollback:* revert the individual row's commit. This is why rows are not batched into one commit.

4. **Delete the legacy navmesh draw module** — `CkNavmeshDebugDraw` — **retired, not rewritten**
   ([NN-D8f]). CkGroundNav's own draw module (P6/6B) is its replacement and must be the **only**
   navigation draw module remaining.
   → *verify:* **C5** — the module's `.uplugin` row is gone; `rg` finds no reference to it; a
   packaged Development build boots with no missing-module ensure.
   → *gate:* verify the deletion target matches the R3 bucket tag and the full-suite gate stays
   delta-zero after the commit; any unexpected diff or test movement → **STOP + revert the step**.
   → *rollback:* revert this commit **and** re-add the `.uplugin` row; note that a `.uplugin` change
   requires a rebuild, so the rollback is not hot-reloadable.

5. **Remove `NavigationSystem` and `AIModule`** from `CkNavigation.Build.cs:17-18`,
   `CkCrowd.Build.cs:21`, and every other affected `.Build.cs`.
   → *verify:* **C4** — `rg -n 'NavigationSystem|AIModule' Plugins/CkFoundation/Source/**/*.Build.cs`
   → zero hits in the affected modules, **and the build is green without them**. The green build is
   the evidence; the grep alone is a claim.
   → *gate:* verify the deletion target matches the R3 bucket tag and the full-suite gate stays
   delta-zero after the commit; any unexpected diff or test movement → **STOP + revert the step**.
   → *rollback:* revert this commit; the dependency rows return and the build recovers.

6. **Repo-wide symbol sweep.**
   → *verify:* **C6** — `UNavigationSystemV1`, `ARecastNavMesh`, `UNavigationQueryFilter`, `UNavArea`,
   `INavRelevantInterface`, `FPathFindingQuery`, and `UNavArea_Null` return **zero** hits across
   CkFoundation, CkTests and CkGameplayDebugger (`.h/.cpp/.cs/.as`), searched with `--no-ignore`.
   Compare against the entry census (R3's snapshot: 60/48/25/45/1/1).
   → *gate:* verify the deletion target matches the R3 bucket tag and the full-suite gate stays
   delta-zero after the commit; any unexpected diff or test movement → **STOP + revert the step**.
   → *rollback:* per-symbol, revert the commit that removed its last site.

7. **Documentation reconciliation (F2.23).** `Source/CkGroundNav/Claude.md` current with its boundary
   paragraph, API and anti-patterns; `Source/CLAUDE.md` decision-tree row and tier-table row landed;
   every consumer module's `Claude.md` updated; the stale `CkNavigation/Claude.md` and the historical
   `CkNavigation/Plan/` reconciled or tombstoned; `CkAStar/Claude.md`'s CkGrid-only description
   corrected; every campaign doc updated or tombstoned; **`FEATURE_MATRIX.md` Provenance complete for
   every shipped feature**.
   → *verify:* **C8** — every claim in the new module doc verified against code **on the day it is
   written**, and dated.
   → *rollback:* docs revert like any other commit; a stale doc is a trap, not a crash.

8. **Full-suite delta-zero on the final artifact, after all deletions**, with packaged Development and
   Test builds re-verified — deletion regressions surface in packaged builds first.
   → *verify:* **C7** — the gate re-runs **after the last deletion**. A green run from before the last
   deletion is stale-green and is not evidence.
   → *rollback:* if the final gate is red, revert deletions in reverse order until green, then
   diagnose. **Restore the known-good state first; never stack a fix on a broken base.**

9. **Request the retirement CTO sign-off**, presenting C1–C8.
   → *verify:* **C9** — recorded as its **own** numbered decision, distinct from B8.

### Decision gate 8C → campaign close

- **C1–C9 green** → the campaign's Definition of Done (VALIDATION.md §6) is met; this doc folder becomes
  history and `Source/CkGroundNav/Claude.md` is the living doc.
- **Any post-deletion red** → **STOP**, revert to the last green state, record, end session.
- **A `[RETIRE]` or `[ADAPTER]` row turns out to have a live consumer** → **STOP**, record the
  consumer verbatim, end session. Do not migrate it opportunistically; that is a scope change.
- **A test flips red→green unexplained** during deletion → **STOP** and investigate. An unexplained
  improvement during a deletion phase is a coverage loss until proven otherwise.
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 6. Exit criteria (measurable)

- [ ] **8A**: shadow report in PROGRESS.md, one row per map, every §3.2 metric, artifact identity
      named; **zero** success/failure disagreements; **zero** containment escapes; every
      `[MEASURE]` placeholder in VALIDATION.md replaced by a measured, dated, reproducible pair of
      numbers (Recast baseline + CkGroundNav).
- [ ] **8B**: **B1–B8** all checked, promotion recorded as a **numbered decision**; the rollback
      (single setting revert) demonstrated green on the same artifact.
- [ ] **8C**: **C1–C9** all checked; the R3 table annotated to zero adapter rows; the symbol sweep at
      zero against the recorded entry census; `NavigationSystem`/`AIModule` gone with a **green build
      without them**; `CkNavmeshDebugDraw` deleted with CkGroundNav's draw module the only navigation
      draw module remaining; full-suite delta-zero on the **final** artifact plus packaged
      Development and Test re-verified; retirement recorded as its **own** numbered decision.
- [ ] Every **§0 standing gate** green for each sub-phase: entry baseline, delta-zero on the final
      artifact, zero new ensures/warnings, clean editor boot with regenerated AS bindings, three
      environments, Provenance rows complete, attribution obligations discharged, **no
      forbidden-source language** in any doc, comment, commit message, or PR body.
- [ ] **Comment audit** run over every diff; PROGRESS.md updated; the campaign's Definition of Done
      (VALIDATION.md §6) walked line by line.

---

## 7. Fences (each with its reason)

- **Promotion and retirement are never signed off in the same review** (VALIDATION.md §6 (item 3)). They rest
  on different evidence, and combining them deletes the rollback path at the same moment it is first
  relied upon. An executor offered a combined grant stops and returns to the orchestrator.
- **A working demo is never parity evidence.** Only the §3 shadow report over the full suites, with
  agreement on **every** test and deltas inside **measured** budgets, satisfies B1.
- **No estimated numbers, ever.** Every `[MEASURE]` cell is a measurement of the **Recast** path on
  the **same fixture** at the owning phase's entry, paired with a CkGroundNav measurement, both
  dated. A carried-over number from another machine is not a budget.
- **Cook and packaging are build-machine-only. Do not cook or package locally.** B5, C7 and §4.4
  require packaged **Development** and **Test** artifacts; request them from the build machine and
  record the artifact identity. A local cook races the editor for `Saved/`/`Intermediate/` and its
  output is not an artifact any gate is defined against.
- **Shipping-configuration builds require explicit maintainer approval** in chat, requested and
  granted before anyone starts one. No gate in this campaign needs Shipping.
- **Module tier.** After 8C the only navigation draw module is CkGroundNav's, at **Runtime** tier;
  the debugger window stays **DeveloperTool** and editor halves stay **Editor**. Deleting a module
  is exactly when a tier inversion elsewhere becomes visible — and it becomes visible in a **packaged**
  build, which is why C7 re-verifies packaged Development and Test.
- **No raw pointers, handles, `UObject`s, or shared field structures inside any snapshot or
  diagnostics fragment** — including the shadow-parity diagnostics this phase adds to. A snapshot
  outlives its producer by design.
- **Never hide a new failure behind the known inherited `Nav.Filter.Customer` baseline failure**
  (`CkCrowdDebugger/CLAUDE.md:29-30`). It is named in the entry baseline so a second red in that
  family is unmistakable — most of all in a phase whose whole job is deleting things.
- **Stale-green is not green.** The C7 gate re-runs **after the last deletion**, on the final
  artifact.
- **One deletion per commit in 8C.** A batched deletion cannot be reverted selectively, and a batched
  red does not localize its cause — bisect it, do not guess.
- **Restore the known-good state first** when a deletion regresses behaviour: revert, diagnose,
  re-sequence, re-apply. Never stack a fix on a broken base.
- **Never edit source, `Script/`, or config while a build or test run is in flight.**
- **Commits, pushes, submodule pointer bumps, and PRs only when the maintainer asks.** Stage only
  paths you authored; scope a submodule pointer bump to the single gitlink path; before bumping,
  confirm the pointed-at commit is itself pushed.

---

## 8. [P8] Done means

**[P8] is done when**, and only when:

1. **VALIDATION §3** is satisfied: the shadow report exists, covers §3.1's suites, records every
   §3.2 metric per map with its artifact, shows **zero** success/failure disagreements and **zero**
   containment escapes, and every remaining disagreement class (fail-reason, partial-path) is
   individually adjudicated in PROGRESS.md.
2. **VALIDATION Tier B (B1–B8)** is fully checked with evidence, and **promotion carries its own CTO
   sign-off** recorded as a numbered decision.
3. **VALIDATION Tier C (C1–C9)** is fully checked with evidence, and **retirement carries its own,
   separate, later CTO sign-off** recorded as its own numbered decision.
4. **F2.21 and F2.22** are satisfied: every budget in VALIDATION.md carries a measured Recast
   baseline and a measured CkGroundNav number, both dated and reproducible; markup-live and
   moved-obstacle repair latencies are recorded as pairs per fixture.
5. **F2.23** is satisfied: `Source/CkGroundNav/Claude.md`, `Source/CLAUDE.md`, every consumer
   module's `Claude.md`, and every campaign document are reconciled or tombstoned, each claim
   verified against code on the day it was written and dated.
6. Every **`[EDITOR-VERIFY]`** section of VALIDATION.md §4 has a recorded human result naming the
   **verifier** and the **artifact**.

At that point the campaign's Definition of Done (VALIDATION.md §6) is met, this folder becomes history,
and `Source/CkGroundNav/Claude.md` is the living doc.

---

## Boundary re-verification (2026-09-04)

Read-only pass over this phase's entry state, ruled as [NN-D93] (PROGRESS.md).

- **§5.2's `[RETIRE]` deletions were already done in 0C** — `CkCrowdAgent_DiagNavClip_Processor.cpp`,
  `CkCrowdAgent_DrawNavProjection_Processor.cpp`, and the debug-settings probe are gone ahead of this
  phase's own retirement track.
- **Repo-wide symbol census, re-taken**: `36/32/3/31/1/2` (R3's entry census was `60/48/25/45/1/1`).
- **`CkCrowd.Build.cs:24`** still carries the `NavigationSystem` module dependency, with its comment
  naming the one remaining direct consumer (PathRefresh's provider resolve) plus the `UNavArea`/
  `UNavigationQueryFilter` subclasses the area-tag table resolves to — part of C4's inventory, not yet
  removed.
- **`Request_SetActorNavigationRegistered` has two AS consumers** — kept per [NN-D8c]; not a stray
  reference to sweep.
- **[NN-D19] vs C6**: PathNetwork's safety-oracle raycasts were ruled P5-scope and kept Recast-direct
  ([NN-D19]); C6's repo-wide symbol sweep excludes this ruled keep rather than counting it as a
  violation, pending the [P5-B1] migration.
- **F1–F7 rulings** (full text: [NN-D93], PROGRESS.md): **F1** two full suites only — the
  campaign-end run doubles as B7's reverted-setting proof, C7 gets the second, no third run. **F2**
  shadow coverage via one harness-staged whole-band field, then per-fixture as the next Tier B unit
  after promotion is requested. **F3** C7's delta-zero holds modulo a deletion manifest (one line per
  removed test, traced to its removing commit). **F4** B4's shared metric is completed queries per
  frame at the reference agent count, each provider's own budget knobs recorded alongside. **F5**
  retirement is a later phase (PHASE_9), honouring "promotion live through one phase" by splitting
  promotion and retirement. **F6** C6's sweep excludes comments and `Script/Generated/**`; the level's
  placed Recast actor is deleted last, by the maintainer, as the final retirement act. **F7**
  `Nav.Filter.Customer` is registered natively rather than depending on the uncommitted
  `Config/DefaultGameplayTags.ini` diff.
