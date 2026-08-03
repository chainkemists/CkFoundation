# The campaign doc set — templates

Reference for `ck-methodology`: the literal PROMPT.md / PHASE_N.md / PROGRESS.md templates to copy, with every section's purpose.

## 3. The campaign doc set

| Doc | Role | Volatility | Update trigger |
|---|---|---|---|
| `PROMPT.md` | Mission brief: goal, constraints, non-goals, success criteria, locked decisions | Stable | Rare, dated edits only |
| `PLAN.md` + `Plan/Gate_NN_*.md` | Executive index + one self-contained contract per gate | Per-gate | **Same commit** as the gate landing |
| `PROGRESS.md` | Living log — the ONLY home for volatile state | Every session | Every session, dated |

Location precedent: alongside the code they steer — `Source/<Module>/PLAN.md` + `Plan/Gate_*.md`
(CkNavigation's layout). CTO review files live in `Plugins/CkFoundation/docs/reviews/` (§8).
Reviewed specs/plans have historically lived in the **host superproject's** `docs/superpowers/`
tree (host-dependent — the review files link there). Campaign docs are disposable: CkNavigation's
PLAN.md §"Post-ship cleanup" is explicit that gate plans get deleted post-ship and the module's
`Claude.md` is the permanent survivor. Plan for that deletion from day one.

### 3.1 PROMPT.md — mission brief

**Provenance:** distilled from the CkTests continuation prompts
(`Plugins/CkTests/Script/CkIskmRenderer/CONTINUATION_PROMPT_GymTesting.md` is the best-built:
one-line state, expectation tables, a diagnostic branch table, "Things ruled out — do not
re-investigate"). Those docs are ALSO this skill's cautionary exhibit (§7): their **content shape
is excellent, their lifecycle failed** — volatile state (branch names, uncommitted-file lists,
"next session do X", even "a screenshot will be pasted alongside this prompt") was baked into
undated docs that outlived their truth and now mislead every reader. This template keeps the good
shape and fixes the failure: **only stable content lives here; anything volatile points at
PROGRESS.md; a freshness header declares when the doc dies.**

```markdown
# <Campaign> — mission brief (PROMPT.md)

> **Written:** YYYY-MM-DD. STABLE content only — current state lives in [PROGRESS.md](PROGRESS.md).
> **This doc dies when:** <campaign ships / superseded by Source/<Module>/Claude.md>.
> On death: delete it, or replace the body with one tombstone line ("Superseded by X — kept for history").

## Goal
One paragraph. The world-state after the campaign, phrased observably.

## Success criteria
Numbered, checkable. Each an observation ("100 agents navigate the store map at 60fps in PIE"),
never an activity ("implement steering").

## Constraints & locked decisions
| Decision | Choice | Why |
(Reproduce from the plan's own table. When sending for CTO review, REGENERATE this table by
reading the plan — never from session memory; see §8, the CkEqs drift lesson.)

## Non-goals
Explicit out-of-scope, each with a one-line why (model: PLAN.md:33's queueing row — "gameplay/GOAP
concern, not a steering concern; CkCrowd's contract ends at 'moves agent to a target'").

## Reading list
The gate index + the named reference modules to mimic (§2 item 3).

## Things ruled out — do not re-investigate
| Ruled out | Why | Evidence |
```

### 3.2 Gate_N.md — the per-phase contract

**Provenance:** `Source/CkNavigation/Plan/Gate_00_Foundation.md` and `Gate_02_Locomotion.md`
(header block, goal-as-world-state, numbered acceptance criteria, sub-tasks citing proven patterns,
file inventory, gym spec, risks table, done-criteria checklist). Two fixes are baked in beyond the
originals: (a) the risks table is upgraded to **expected observations + branch-on-observation** —
Gate 2's risk rows are the embryo ("Turn rate clamp creates orbits… If observed, allow stride-skip…
Defer until Gate 4") but they only cover failure guesses, not the positive observations the gate
exists to make; (b) the exit checklist's doc-update items are welded to the landing commit, because
the originals had the item and it was still skipped (§7).

```markdown
# Gate N — <Name>

> **Status:** ⏳ Pending | 🟡 In progress | ✅ Done (YYYY-MM-DD) | ⚠️ Blocked | 🔁 Superseded by <file>
> **Depends on:** Gate N-1 ✅
> **Estimate:** X days — re-date at entry; record actual at exit

## Goal
"After this gate: <observable behavior a human or test can see>."

## Entry criteria (pre-flight — run these, don't assume them)
- [ ] Prior gate's exit checklist re-verified on current HEAD (<hash recorded here>)
- [ ] Baseline captured: the current behavior this gate changes, and HOW you observed it
      (model: Gate_03_Separation_Hybrid_Plan.md pre-flight — "Ck_GymCrowd_Sep_HeadOnNS in PIE
      produces the existing vibration behavior, so we have a baseline to measure improvement against")
- [ ] Plan's code shapes spot-checked against current doctrine — plans snapshot conventions and
      conventions move (Gate_00 still teaches the retired ProcessorInjector/; root CLAUDE.md retired it)

## Work items
Sequenced. Each either names the proven pattern it replicates (file:line) or is flagged
"NEW INFRASTRUCTURE — unknown", which is where the schedule risk lives.

## Expected observations at the gate — and what to do on each branch
| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| <gym station / AutoTest / PIE step> | <specific, quantified> | <plausible deviation> | <tune X / write addendum / ask maintainer "[A] vs [B]?"> |

## Exit criteria — ALL items land in the SAME commit as the last work item
- [ ] Every expected observation above confirmed; evidence (counts, log verdicts) recorded in PROGRESS.md
- [ ] `ck-change-control` done-checklist run for this change class
- [ ] Anything only verifiable in editor/PIE listed as [EDITOR-VERIFY] with exact manual steps
- [ ] Index status row (PLAN.md) AND this file's Status header updated — both, same commit
- [ ] Module Claude.md updated with this gate's permanent contribution
- [ ] PROGRESS.md dated entry appended
```

(Adapt section names to the campaign — CkNavigation gates add "Gym spec" and "Debugger additions"
because each gate shipped a gym — an interactive in-editor test station — and debugger panels.
Keep the five bones: entry criteria, work items, expected observations + branches, exit criteria,
same-commit doc updates.)

### 3.3 PROGRESS.md — living log

**Provenance:** `Source/DEBUG_CALLSTACK_PROGRESS.md` (a finished 16-module rollout with good
bones: status legend, summary stats up top, per-item checkboxes, and SKIP-entries that record WHY
— "CkAggro [SKIP] — no request fragment exists — uses direct mutation pattern" — so nothing gets
re-litigated). Three failure modes observed in it are fixed here: it has **no dates anywhere**; it
declares "Implementation Complete! / 100% coverage" while a "Modules to Investigate" section still
lists 19 unchecked modules (a self-contradiction a reader cannot resolve); and it names modules
that no longer exist (CkEcsTemplate/CkTemplate — removed in `ad045415b`).

```markdown
# <Campaign> — PROGRESS.md (living log)

## Current state  <!-- supersedes everything below; update at EVERY gate and session end -->
**As of YYYY-MM-DD (commit <hash>):** Gate N done; Gate N+1 in progress at <work item>.
**Baseline being diffed against:** <test pass/fail counts + failing names, captured YYYY-MM-DD>.
**Next action:** <one line>.
**Blocked on:** <thing, or "nothing">.

## Decision log
| Date | Decision | Why | Revisit when |
(Ruled-out approaches go here WITH the reason — the "do not re-investigate" table of §3.1 is
promoted from these rows.)

## Dated entries (append-only, newest first)
### YYYY-MM-DD — <what happened>
- Ran: <command/test> → <result, counts vs baseline>
- Confirmed: <claim + its evidence (file:line / log line / exit code)>
- Inferred (unconfirmed): <claim + what would confirm it>
- Follow-ups recorded, not chased: <one-liners>

## Open items
| Item | Status | Next step |
**Rule: no completion claim may be written anywhere in this file while any row here is unresolved.**
```

