# PHASE 5 — Nav links, CkPathNetwork migration, editor authoring snap

> **Freshness:** authored 2026-08-31 during the P4 planning package (documentation-only session,
> [NN-D3]). **Status of record: [PROGRESS.md](PROGRESS.md)** — never this file.
> **Authoritative upstream:** [PROMPT.md](PROMPT.md) (provenance policy §0, executor rules §8),
> [REPRESENTATION.md](REPRESENTATION.md) [NN-D7], [MIGRATION_SEAM.md](MIGRATION_SEAM.md) [NN-D8],
> [FEATURE_MATRIX.md](FEATURE_MATRIX.md) (capability freeze), [VALIDATION.md](VALIDATION.md)
> (gate ids). Executors implement these; they do not revisit them.
> **Line references** in this file and in `research/R3-coupling-inventory.md` are a snapshot @
> CkFoundation `a25ec9539`. **Re-verify every one before editing** — a moved line is not a licence
> to guess which call site was meant.

---

## 0. Scope note — resolved

- **P5 scope is F2.1–F2.8.** Sub-phase **5F** (F2.5–F2.8: EQS migration, CkQueue migration +
  revision consolidation, named markup regions with live cost update, per-agent-profile field
  variants) is **in scope and entered normally**, per **[NN-D10]** in PROGRESS.md. There is no
  contradiction to record and no ruling to wait for.
- Sub-phase **5E** (automatic nav-link generation) is **Tier 3** (F3.1–F3.3) and **not entered**,
  per **[NN-D11]**. See §5E.

---

## 1. Goal and feature ids

Remove three of the remaining reasons Recast cannot be retired, and add the last generation-1
capability the representation does not produce on its own.

| Sub-phase | Features | One-line goal |
|---|---|---|
| **5A** | **F2.1** | Manual nav links: authored, baked into the published field as a distinct portal kind, visible to A* and to reachability. |
| **5B** | **F2.2** | Link runtime state (no rebuild) + **path-carried traversal metadata** + the traversal handshake and per-agent veto. |
| **5C** | **F2.3** | CkPathNetwork off its Recast **safety oracle**, its **off-path-leg connector builder**, and its filter-class threading — onto the neutral facade. |
| **5D** | **F2.4** | Editor authoring snap onto the CkGroundNav surface, in-editor, without PIE. |
| **5E** *(NOT entered — [NN-D11])* | F3.1–F3.3 | Automatic link generation. Tier-3; CTO decision required to enter. |
| **5F** *(in scope — [NN-D10])* | F2.5–F2.8 | EQS, CkQueue + revision consolidation, named markup regions, profile variants. |

**Ordering is load-bearing:** 5A before 5B (state needs geometry), 5B before 5C (PathNetwork's
off-path legs may cross links), 5C before 5D (the editor snap consumes the same facade projection
the runtime oracle migration proves out).

VALIDATION gate served: **A7 — Links, PathNetwork, authoring (P5)**.

---

## 2. Entry criteria — confirm all before writing a line

Do not begin this phase on criteria you could not confirm. If any check fails, stop and record it.

- [ ] **P4 closed** in PROGRESS.md with its exit evidence recorded (A6 green: markup paints, the
      markup-live probe is ground truth, local repair == full rebake, repair never mutates a
      published field, epoch semantics hold).
- [ ] **F1.13 projection, F1.16 surface raycast, F1.23 A*, F1.24 funnel, F1.27 neutral filters,
      F1.34 path invalidation** are landed and green — 5C and 5D consume all six through the facade
      and build none of them.
- [ ] **Session-start ritual** run verbatim (PROMPT.md §7), including the two "Done"-claim
      spot-checks.
- [ ] **Phase-entry baseline captured this session** — full toolbox `--build --test`, `--parallel 1`,
      `--no-nullrhi`, with totals **and failing-test names** and the artifact identity recorded in
      PROGRESS.md. The known inherited `Nav.Filter.Customer` mapping failure
      (`CkCrowdDebugger/CLAUDE.md:29-30`) is recorded **by name** in that baseline.
- [ ] **`[MEASURE]` budgets for A7 filled at entry**, measured on the **Recast** path, on the same
      fixtures, and recorded in PROGRESS.md: (a) PathNetwork segment-safety verdict distribution on
      every shipped authored network, (b) off-path-leg resolution time, (c) editor snap latency per
      node placement. A budget nobody measured is not a budget.

**Verify commands** (project root; the `build-test` skill owns the canonical shapes and the
editor-closed pre-flight table — consult it before every invocation):

```powershell
# Repo state — every submodule clean and on the expected commit before starting
git -C . status --short --branch
git -C Plugins/CkFoundation status --short --branch

# Baseline (build path: editor MUST be closed)
Set-Location "D:\Repos\CkPlugins_3"; ./CkAuto/UnrealToolbox.exe --build --target=Editor --test --parallel 1 --no-nullrhi `
    --output=Saved/Logs/P5-Baseline.log --project="D:\Repos\CkPlugins_3"

# Re-verify the R3 line references this phase edits, before touching them
rg --no-ignore -n 'Resolve_OffPathLeg|Get_DefaultRecastNavmesh|Is_NavmeshSegmentDirectlyWalkable|Try_ResolveNavmeshSegment' `
    Plugins/CkFoundation/Source/CkPathNetwork
rg --no-ignore -n 'ProjectPointToNavigation|NavMeshRaycast' `
    Plugins/CkFoundation/Source/CkPathNetworkEditor
```

---

## Standing decision gate (applies to EVERY `-> verify:` line in this file)

Read this once; it is the default branch set for every verification step below. Steps that need
extra branches carry their own **Gate** block.

- Observation matches the stated expectation → continue to the next step.
- Build fails to compile → fix mechanically (missing include, moved symbol, UHT complaint) and
  re-verify. **Two failed attempts at the same step → STOP.**
- A test that was red in the P5 phase-entry baseline is still red with the same name → not yours;
  continue, and carry it forward in the delta-zero comparison.
- A test that was green in the baseline is now red → **STOP**, restore the known-good state (revert
  your last step), then diagnose before re-applying.
- A test that was red in the baseline is now green → **STOP**. An unexplained improvement is as much
  a defect as a regression — explain it or record it (VALIDATION.md A1).
- **Any observation not enumerated above → STOP, record it in PROGRESS.md § Blockers with verbatim
  evidence, end session.**

---

## 3. Sub-phase 5A — manual nav links (F2.1)

Links are **baked value records inside the published field**, resolved to the plates their endpoints
project onto, exposed to search as portals of a distinct kind. They are never pointers, never
authored into the plate graph by hand, and never patched into a published field.

1. **Author the link value type and its stable identity.** `FCk_GroundNav_LinkId` = owning tile id +
   per-tile link index + the tile's epoch, so a link id carried across a rebuild is **detectably
   stale by construction**.
   → *verify:* Layer 1 — a link id minted against epoch N compares unequal to the same index at
   epoch N+1; `rg` over the field value types finds no pointer, `TObjectPtr`, `TWeakObjectPtr`, or
   engine-object reference (A3's "stable integer identity only" assertion, restated).

2. **Author the link record**: start/end position, per-end up vector, per-direction traversal
   permission, per-direction cost, area tag, user-type tag, enabled flag. House shape, illustrative
   only — *the executor refines this mechanically, it does not redesign it*:

   ```cpp
   namespace ck
   {
       struct CKGROUNDNAV_API FCk_GroundNav_LinkRecord
       {
           CK_GENERATED_BODY(FCk_GroundNav_LinkRecord);

       private:
           FCk_GroundNav_LinkId          _LinkId;
           FVector                       _Start          = FVector::ZeroVector;
           FVector                       _End            = FVector::ZeroVector;
           FCk_GroundNav_PlateId         _StartPlate;
           FCk_GroundNav_PlateId         _EndPlate;
           ECk_GroundNav_LinkDirection   _Direction      = ECk_GroundNav_LinkDirection::Bidirectional;
           float                         _CostForward    = 1.0f;
           float                         _CostBackward   = 1.0f;
           FGameplayTag                  _AreaTag;
           FGameplayTag                  _UserTypeTag;

       public:
           CK_PROPERTY_GET(_LinkId);
           CK_PROPERTY_GET(_Start);
           CK_PROPERTY_GET(_End);
           CK_PROPERTY_GET(_StartPlate);
           CK_PROPERTY_GET(_EndPlate);
           CK_PROPERTY_GET(_Direction);
           CK_PROPERTY_GET(_CostForward);
           CK_PROPERTY_GET(_CostBackward);
           CK_PROPERTY_GET(_AreaTag);
           CK_PROPERTY_GET(_UserTypeTag);
       };
   }
   ```
   → *verify:* Layer 1 — construct, round-trip through the field's flat arrays, byte-compare.

3. **Resolve endpoints at bake time** using F1.13 projection, and record the resolved plate ids in
   the published field. An endpoint that does not project is **dropped with a counted diagnostic**,
   never silently kept and never resolved to a nearest-anything.
   → *verify:* Layer 1 — a link whose end hangs over a hole is dropped and the drop counter reads
   exactly one; the field is otherwise identical to the same bake without that link.

4. **Extend the search graph** so links appear as portal-kind edges in `Neighbors` and price through
   `Cost` with their per-direction cost. The `AStarGraph` concept
   (`CkAStar_GraphConcept.h:16-31` — `Neighbors`/`Cost`/`Heuristic`/`IsGoal`, nothing geometric) is
   **not modified**; the graph model is.
   → *verify:* Layer 1 — the `static_assert` on the concept still compiles (A5's evidence, restated);
   a path across a gap uses the link when its cost beats the detour and takes the detour when it does
   not, at the analytic crossover ± tolerance; a one-directional link is traversable in exactly one
   direction.

5. **Extend reachability (F1.9)** so links contribute connectivity to the union-find pass, and
   **the funnel (F1.24)** so a link crossing emits its two endpoints as waypoints rather than being
   string-pulled through.
   → *verify:* Layer 1 — two otherwise disconnected components joined only by a link carry the same
   component label; removing the link re-splits them; the funnel over a corridor containing one link
   emits the link's start and end as distinct consecutive waypoints and never interpolates across it.

6. **Author the AngelScript and Blueprint surface** for authoring and querying links (three
   environments — VALIDATION §0).
   → *verify:* the same link is authored and read back from C++, from Blueprint, and from
   AngelScript; AS bindings generate with no error and `Script/Generated/*` churn is **regenerated,
   not hand-edited**.

### Gate 5A → 5B

- Every step above green **and** the targeted family `Ck.GroundNav.*` green → **proceed to 5B**.
- A link endpoint resolution needs a projection mode F1.13 does not offer → **STOP**, record in
  PROGRESS.md § Blockers, end session. Do not add a projection mode.
- The plate/portal graph cannot express a link edge without changing `AStarGraph` → **STOP**, record,
  end session. The concept is shared with CkGoap, CkVoxelNav and CkPathNetwork.
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 4. Sub-phase 5B — link runtime state and traversal metadata (F2.2)

The split is fixed by FEATURE_MATRIX and is not an executor's call: **geometry is baked and
immutable; enabled/cost live in a per-world overlay the search consults.** A state change therefore
costs no bake and bumps no epoch.

1. **Per-world link-state overlay** (enable/disable, cost update, add/remove by id or by volume,
   batch update), keyed by `FCk_GroundNav_LinkId`, consulted by `Neighbors`/`Cost`.
   → *verify:* Layer 1 — toggling a link changes subsequent search results with **zero** geometry
   probes and **no** epoch bump; two worlds' overlays are provably independent (A2/F1.36 multi-world
   discipline, restated).

2. **Path-carried link metadata.** The neutral result (F1.28) gains, per waypoint, whether it is a
   link-start or link-end, which `FCk_GroundNav_LinkId`, and the expected entry direction. This is
   **additive to `FCk_Nav_PathResult`'s existing shape** — the request/result contract stays
   byte-compatible for consumers that ignore it.
   → *verify:* Layer 1 — a path across a link carries exactly one start/end metadata pair; a path
   with no link carries none; an existing consumer reading only `_Waypoints` is bit-identical to its
   pre-change behaviour.

3. **Link queries**: "the next link beyond distance d along this path" and "all links on this path".
   → *verify:* Layer 1 — both agree with a brute-force walk of the waypoint list on 1k generated
   paths.

4. **Traversal handshake** — `FCk_Request_NavSurface_LinkTraversal` carries the **link id** plus a
   **correlator id**, enqueued through the utils facade and **drained on the game thread like every
   Ck request**. There is **no any-thread entry**: an off-thread caller marshals to the game thread
   first. Completion is a paired `Request` with the same correlator over the same transport, in the
   standard house request shape with the completion delegate **last**, exactly-once, and
   `Succeeded` for an idempotent no-op. Illustrative — *executor refines mechanically, does not
   redesign*:

   ```cpp
   UFUNCTION(BlueprintCallable,
             Category = "Ck|Utils|NavSurface",
             DisplayName="[Ck][NavSurface] Request Begin Link Traversal")
   static FCk_Handle
   Request_BeginLinkTraversal(
       UPARAM(ref) FCk_Handle& InHandle,
       const FCk_Request_NavSurface_BeginLinkTraversal& InRequest,
       const FCk_Delegate_Request_OnCompleted& InDelegate);
   ```
   → *verify:* Layer 2 — an agent traverses a link, the handshake delegate fires **exactly once**;
   cancelling mid-traversal completes `Failed_Cancelled`; a begin issued twice for the same
   correlator completes `Succeeded` the second time (idempotent no-op) and does not double-fire.

5. **Per-agent traversal veto** — part of the **per-query compiled filter** (F1.27), evaluated when
   search expansion considers a link edge (the same point at which plate-edge filters run), shaped
   as an **allow/deny plus cost-rewrite on link records**. Not a callback into agent code from
   inside the search. The veto excludes the link **for that agent's query only**, never globally.
   → *verify:* Layer 1 — a vetoing agent routes around the link in the same field epoch in which a
   non-vetoing agent routes across it.

6. **Path invalidation on link state change** — disabling a link invalidates paths that use it
   (through F1.34) and no others.
   → *verify:* Layer 2 — a crowd agent whose installed path crosses a disabled link replans
   **exactly once**; an agent on an unrelated path does not replan at all (assert on a replan
   counter, not on a settle).

### Gate 5B → 5C

- Steps 1–6 green, targeted `Ck.GroundNav.*` green, no new ensures → **proceed to 5C**.
- The handshake appears to require a second install path beside
  `MarkPathPending`/`InstallExternalPath`/`AbandonPath`/`FailPath` → **STOP**, record, end session.
  "Do not invent a second one" is a standing fence.
- Link metadata cannot be added to the result without breaking the byte-compatible contract →
  **STOP**, record, end session.
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 5. Sub-phase 5C — CkPathNetwork migration (F2.3)

Exactly three dependencies move, and they are enumerated by R3's PathNetwork rows. Nothing else in
CkPathNetwork is touched: this is **surgical foreign-module surgery** — match the existing style,
make no adjacent improvements.

| R3 row | Site (re-verify) | Bucket | Becomes |
|---|---|---|---|
| off-path leg | `Network/CkPathNetwork_Processor.cpp:165-200` `Resolve_OffPathLeg` (FindPathSync) | `[REIMPLEMENT]` | a facade path query (F1.23 + F1.24) |
| safety oracle | `same:349-410` `Get_DefaultRecastNavmesh`, `Is_NavmeshSegmentDirectlyWalkable` (`NavMeshRaycast`), `Try_ResolveNavmeshSegment` (raycast → FindPathSync fallback) | `[REIMPLEMENT]` | `Try_SurfaceRaycast` (F1.16) **with the cost cap**, falling back to a bounded path query exactly as today |
| filter threading | `same:461-797` filter class through the route compiler; `556-650` ribbon containment via `ResolveQueryFilter` | `[ADAPTER]` | neutral filter definitions (F1.27) threaded through `FRouteCostPolicy` |

1. **Migrate the safety oracle first** — it is the retirement-gate criterion and the highest-value
   verdict-agreement evidence.
   → *verify:* a **verdict-agreement test over every shipped authored network**: for each compiled
   ribbon segment, the facade verdict vs the recorded Recast verdict. Disagreements are
   **enumerated and individually adjudicated in PROGRESS.md — never summarized away** (A7's wording).

2. **Migrate the off-path-leg connector builder** to a facade path query, keeping the existing
   failure-reason enumeration intact.
   → *verify:* Layer 2 — the PathNetwork suite delta-zero on the **Recast** provider through the
   facade (this step must change nothing behaviourally), then green on CkGroundNav; off-path-leg
   resolution time within the entry-measured budget.

3. **Thread neutral filter definitions** through `FRouteCostPolicy` and ribbon containment.
   **No back-compat shim** — the filter-class parameter is deleted, and its call sites are updated
   in the same change (CkFoundation doctrine).
   → *verify:* `rg --no-ignore -n 'UNavigationQueryFilter|ResolveQueryFilter'
   Plugins/CkFoundation/Source/CkPathNetwork` → zero hits; the same route compiles differently under
   two filter definitions.

4. **Correct the stale dependency comments** the migration touches ([NN-D8h] is P0's; if a
   PathNetwork-side comment is falsified by this change, correct it in the same change).
   → *verify:* comment audit before done — every comment the diff added is a load-bearing *why* or a
   `/** contract */` block; no gate/phase/campaign breadcrumbs anywhere in the source
   (root CLAUDE.md, mandatory closing step).

### Gate 5C → 5D

- PathNetwork suite delta-zero on Recast **and** green on CkGroundNav; verdict agreement recorded
  with every disagreement individually adjudicated → **proceed to 5D**.
- Any segment-safety verdict disagreement that cannot be adjudicated to a stated cause → **STOP**,
  record the verbatim evidence (network name, segment index, both verdicts), end session. A verdict
  disagreement is not a tolerance to widen.
- The oracle needs a query the facade does not expose → **STOP**, record, end session. Do not add a
  facade capability; the twelve are frozen by [NN-D8].
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 6. Sub-phase 5D — editor authoring snap (F2.4)

1. **Editor-world build path** so a field exists at authoring time (no PIE).
   → *verify:* Layer 1 where hermetic; otherwise the `[EDITOR-VERIFY]` in step 3.

2. **Migrate the snap sites** — `CkPathNetworkEditor/CkPathNetwork_EditorUtils.cpp:27-35,176-186,356-366`
   — to `Try_ProjectPoint` (F1.13) through the facade against the editor world's provider.
   An **unbuilt** region reports its status; it never snaps to nothing and never snaps to an
   arbitrary layer.
   → *verify:* `rg --no-ignore -n 'ProjectPointToNavigation' Plugins/CkFoundation/Source/CkPathNetworkEditor`
   → zero hits; a unit-testable projection-through-facade call returns `Unbuilt` (not `NoSurface`)
   over an unbuilt tile.

3. **`[EDITOR-VERIFY]` per VALIDATION §4.3** — write the result (verifier name + artifact) into
   PROGRESS.md. Nodes snap on flat ground, on a ramp, and on the **upper of two overlapping floors**;
   a node dragged off the surface shows a clear un-snapped indication; no editor hitch per placement.
   → *verify:* the recorded human result. An agent cannot perform this step and must not claim it.

### Gate 5D → phase exit

- 5A–5D green, full suite delta-zero on the **final** artifact, `[EDITOR-VERIFY]` §4.3 recorded →
  **phase exits**; author PHASE_6 at the boundary.
- The `[EDITOR-VERIFY]` returns a fail signature (wrong layer over an overlap, snap requiring PIE,
  per-placement hitch) → **STOP**, record the observation verbatim, end session.
- Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 7. Sub-phase 5E *(NOT entered)* — automatic link generation

Automatic link generation is **Tier 3** (F3.1–F3.3) and is **not entered by this campaign**, per
**[NN-D11]** in PROGRESS.md: manual links reach replacement parity, so auto-links are a
post-retirement enhancement.

Entering 5E requires a **CTO decision** that (a) re-tiers F3.1–F3.3 out of Tier 3 in
FEATURE_MATRIX by the orchestrator, and (b) commissions a fresh design pass from cited public
sources — no pipeline is specified here.

Anything routing an executor into this sub-phase → **STOP, record in PROGRESS.md § Blockers,
end session.**

---

## 8. Sub-phase 5F — F2.5–F2.8

In scope and entered normally per **[NN-D10]** (§0):

- **F2.5 EQS** — migrate **both** sites (`CkEqs/Query/CkEqs_Algorithm.cpp:266-280` inline projection
  post-pass, plus generators/tests), then **re-measure**; the "keep the two in sync" note dies with
  the duplication **unless measurement vetoes** ([NN-D8g]).
  → *verify:* the measured batch-through-facade cost vs the inline path, recorded in VALIDATION.md —
  the number decides, not a preference.
- **F2.6 CkQueue** — `CkQueue_Formation_Processor.cpp:159-209` to the facade, and
  `CkQueue/Navigation/CkQueue_NavigationRevisionSubsystem.cpp:30-64` **deleted** ([NN-D8a]).
  → *verify:* `rg --no-ignore -n 'NavigationRevisionSubsystem' Plugins/CkFoundation/Source` returns
  exactly one implementation (A2's evidence).
- **F2.7 named markup regions** — cost/tag update with **zero geometry probes and no epoch bump**;
  walkability-changing markup still bumps the epoch.
  → *verify:* Layer 1 assertions on the probe counter and the epoch.
- **F2.8 profile variants** — the mechanism only. **Adding a profile is a CTO decision, not an
  executor's** ([NN-D7] fences).
  → *verify:* Layer 1 — two profiles over exactly one geometry collection (assert the probe counter).

Anything else → **STOP, record in PROGRESS.md § Blockers, end session.**

---

## 9. Exit criteria (measurable)

- [ ] **Targeted families green**, run with `--parallel 1`: `Ck.GroundNav.*` (link, search, funnel,
      reachability additions), the CkPathNetwork family, and the CkPathNetworkEditor family.
      New AS autotests were discovered (`--discover-fresh`) — a green run whose Total matches the old
      count is **stale-green** and has not run them.
- [ ] **Full suite delta-zero** vs the phase-entry baseline, on the **final** artifact of the phase,
      plus this phase's new tests green. A run predating the last edit does not count.
- [ ] **Zero new ensures, zero new warnings** (the AutoTest harness escalates `ck::Warning` to a
      failure); a fresh editor startup log inspected for boot ensures and AS compile errors.
- [ ] **A7 evidence complete**: links traverse and are costed; PathNetwork off-path legs resolve
      through the facade with route quality and failure reasons unchanged; the segment safety oracle
      is migrated with **verdict agreement over every compiled ribbon in the shipped networks**, every
      disagreement individually adjudicated; `[EDITOR-VERIFY]` §4.3 recorded with verifier + artifact.
- [ ] **Contract greps clean**: no `UNavigationQueryFilter` / `NavMeshRaycast` / `FindPathSync` /
      `ProjectPointToNavigation` in CkPathNetwork or CkPathNetworkEditor.
- [ ] **Three environments** exercised for every public API this phase added (C++, Blueprint,
      AngelScript).
- [ ] **Provenance cells non-empty** for every feature this phase shipped; PR bodies name the public
      references; **no forbidden-source language** in any doc, comment, commit message, or PR body.
- [ ] **Comment audit** run over the whole diff.
- [ ] **PROGRESS.md updated**; **PHASE_6.md re-verified at the boundary** against what actually
      landed.

---

## 10. Fences (each with its reason)

- **Module tier.** Link runtime state, the overlay, and everything a packaged build must run live in
  **Runtime**-tier modules. The editor snap half is **Editor** tier. Runtime code must **never**
  depend on an editor-tier module — the failure surfaces only in a packaged build, long after the
  PIE run looked fine.
- **No raw pointers, `TObjectPtr`, `TWeakObjectPtr`, or engine-object references inside the field or
  inside any snapshot** — stable integer ids only ([NN-D7] fences). A pointer in a value that
  outlives its producer is a use-after-free with a debugger in front of it.
- **Never hide a new failure behind the known inherited `Nav.Filter.Customer` baseline failure**
  (`CkCrowdDebugger/CLAUDE.md:29-30`). It is recorded by name in the phase-entry baseline precisely so
  a second failure in the same family cannot be waved through as "the known one".
- **Cook and packaging are build-machine-only. Do not cook or package locally in this phase.** A
  local cook races the editor for `Saved/`/`Intermediate/`, and its output is not the artifact any
  gate is defined against. Packaging work belongs to PHASE_7/PHASE_8 and runs where those phases say.
- **Shipping-configuration builds require explicit maintainer approval**, requested and granted in
  chat. This phase never needs one.
- **No second install path.** Links and PathNetwork legs ship through
  `MarkPathPending`/`InstallExternalPath`/`AbandonPath`/`FailPath` + the revision ring, unchanged.
- **No back-compat shims**; the replaced API is deleted in the same change (CkFoundation doctrine).
- **Deferred `Request_*` ends with the completion delegate as the LAST parameter**, no C++ default —
  moving it breaks AngelScript and silently rebinds positional callers.
- **Surgical in foreign modules.** CkPathNetwork gets the three migrations and nothing else: no
  refactor of the route compiler, no "while I'm here" cleanups.
- **Never edit source, `Script/`, or config while a build or test run is in flight** — mid-run edits
  poison the run and mis-attribute the failure.
- **Commits, pushes, submodule pointer bumps, and PRs only when the maintainer asks.** Stage only
  paths you authored.

---

## 11. [P5] Done means

**[P5] is done when**, and only when:

1. `VALIDATION.md` **A7** is fully checked with evidence recorded in PROGRESS.md — nav links traverse
   and are costed; PathNetwork off-path legs resolve through the facade with unchanged route quality
   and failure reasons (PathNetwork suite delta-zero); the segment safety oracle is migrated with a
   verdict-agreement test over the shipped networks and every disagreement individually adjudicated;
   editor authoring snap works in-editor without PIE and `[EDITOR-VERIFY]` §4.3 is recorded with its
   verifier and artifact.
2. Every **§0 standing gate** of VALIDATION.md is green for this phase: baseline captured at entry,
   phase-exit full suite delta-zero on the final artifact, zero new ensures/warnings, editor boots
   clean with AS bindings regenerated, three environments for every new public API, Provenance rows
   complete, attribution obligations discharged, no forbidden-source language.
3. Sub-phase **5F** is closed with its own recorded sign-off. Sub-phase **5E** is **not entered**
   per [NN-D11] — record that, do not treat it as an open gate.

Everything else — including a gym that looks right, a link that visibly works, or a demo path across
a ladder — is **not** evidence for this gate.
