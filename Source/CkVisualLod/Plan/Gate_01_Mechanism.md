# Gate 1 — Mechanism

> **Status:** ✅ Done (2026-08-27) — code-complete and compile-green; [EDITOR-VERIFY] +
> [MAINTAINER-RUN] items in PROGRESS.md remain open for the maintainer
> **Depends on:** Gate 0 ✅ (a6a2ef14c)
> **Estimate:** 1-2 sessions — actual: same session as Gate 0

## Goal

After this gate: an arbiter with a config asset and an observer drives real flips — members
acquire crowd slots lazily, promote to proxies by ranked in-view-first selection under the two
budgets, crossfade both ways with all reversal/preempt/hidden semantics, release deterministically
on death, and every signal fires at its contracted moment. Verifiable headlessly by the C++
automation tests (authored this gate, **maintainer-run**) and visually via a later gym.

## Entry criteria (pre-flight)

- [x] Gate 0 exit re-verified on current HEAD (a6a2ef14c): build green, 8/8 registrations
- [x] Baseline: same as Gate 0 (build-green; no test baseline — standing no-agent-tests directive)
- [x] Code shapes spot-checked against doctrine this session (macros skill + Timer/VisibleRange read)

## Work items

Each names its proven pattern; the port's semantic source is the BB flip processor (spec-by-example,
`BB_NpcVisualLod_Processor_Flip.as`) with the §5 defect fixes from DESIGN_CkVisualLod.md.

1. **Member↔arbiter resolution** — the arbiter's member scan claims unresolved members whose
   params tag matches its domain tag (caches the handle in Current; no extra tag needed — the
   scan already walks every member); `SetArbiter` handler overrides explicitly.
   Duplicate-domain-tag ensure at arbiter Setup (scan sibling arbiters' resolved configs).
   NEW INFRASTRUCTURE — `Handle.View<...>()` secondary iteration + shadowed `DoTick` (snapshot
   arrays, mechanism outside live iteration, `_LastVisitedCount` written).
2. **Crowd stand-up + pools** — lazy `Ensure_Crowd` port: rooted-batch collection load,
   `Create_Crowd`/`Add_CrowdMember`×N/`Finalize_Crowd`, park + hide all, free-list init. Pattern:
   BB `Ensure_Crowd` + CkFx rooted batch.
3. **Per-member pass** — port of `Process_Entity`: stale-crowd invalidation, hidden handling,
   acquisition (+ exhaustion policy: PromoteInstead | Unrendered+ensure-once), lock/distance
   promote gating, fade ticking (reversals, `PreemptDemote` suppression, lock override,
   hidden-mid-fade snap, member-vanished resolution), distance-only demote, far update
   (transform per frame, anim on change, speed-driven vs fixed).
4. **Ranked flip pass** — whole-domain candidates/incumbents → `ck::visual_lod::Select_Flips`
   (already landed) → promotes + rate-limited preempt demotes. Budget accounting per design:
   near / locked / unbudgeted counted separately.
5. **Promote/demote bodies** — scene-node child + `IskmRenderer::Add` + `IskmProxy::Add`,
   renderer via rooted batch (Params or override, soft ptr), signals `OnPromoted` /
   `OnDemoteFinishing` / `OnMemberAcquired` (BEFORE first visible write — verify the signal
   broadcast is synchronous; if not, STOP and consult the maintainer per the design's flagged
   fallback) / `OnMemberReleased`.
6. **EndPlay release** — deterministic slot release + budget refund in
   `FProcessor_VisualLod_EndPlay`; amortized sweep in the arbiter as reconciliation
   (Resilience Tenets: converge from arbitrary state).
7. **View resolve (Gate 1 scope)** — observer handle → `UCk_Utils_Camera_UE` view info → cone
   precompute. No observer ⇒ arbiter no-ops (local-view discovery is Gate 2).
8. **C++ automation tests (authored, NOT run by agent)** — ranking (budget spend, in-view-first,
   preempt margin/rate-limit, worst-incumbent), pure-function level.

## Expected observations at the gate — and what to do on each branch

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| toolbox `--build` | green | compile/link errors | fix; playbook if macro-shaped |
| grep registrations | 8/8 unchanged (+ any new) | missing | add |
| [EDITOR-VERIFY] (maintainer): gym/PIE with an arbiter + members | promote ring near camera, crossfades, budgets hold | pops/leaks/wrong counts | report verbatim; fix before Gate 2 |
| [MAINTAINER-RUN] `--test-pattern VisualLod` when convenient | authored C++ tests pass | failures | agent fixes from the pasted output |

## Exit criteria — ALL items land in the SAME commit as the last work item

- [x] Build green on final artifact (`=== Build succeeded ===`, 0 error lines, log read directly);
      registration grep 8/8
- [x] Signal-timing resolved: `Broadcast` publishes inline (CkSignal_Utils.inl.h:144-181) —
      synchronous; acquire-before-visible contract stands, no fallback needed
- [x] [EDITOR-VERIFY]/[MAINTAINER-RUN] lists written out in PROGRESS.md
- [x] PLAN.md row + this Status header updated — this commit
- [x] PROGRESS.md dated entry appended
