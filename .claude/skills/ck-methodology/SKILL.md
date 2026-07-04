---
name: ck-methodology
description: Use when work spans >1 session, >~10 files, a new module, or an architectural change; when starting a campaign with unknowns; when writing or resuming PLAN.md, PROMPT.md, Gate/PHASE_N docs, PROGRESS.md, or a continuation prompt; when a campaign doc looks stale or contradicts code; when stuck after two failed attempts; when preparing or answering a CTO review. Not for gating a single change (ck-change-control) or diagnosing failures (ck-debugging-playbook).
---

# ck-methodology — working discipline for long and multi-session tasks

## Overview

This skill is the depth behind the root doctrine's **Collaboration protocol**
(`Plugins/CkFoundation/CLAUDE.md` — Research → Plan → Implement, phase gates, stuck protocol; cited,
not restated). It turns that section into templates and rituals, each distilled from a **named real
campaign artifact in this repo** — including two that failed and now serve as cautionary exhibits.
Core principle: a campaign is steered by three documents — PROMPT.md (stable mission), Gate_N.md
(per-phase contract), PROGRESS.md (the only home for moving state) — and every living doc is either
updated at each gate **or explicitly tombstoned**. A doc that is neither is worse than no doc.

## When NOT to use this skill

| You need | Load instead |
|---|---|
| Classify one change; what "done" requires for it | `ck-change-control` |
| Diagnose a build/UHT/linker/AS/test failure | `ck-debugging-playbook` |
| "Has this been tried before?" — incidents, dead ends | `ck-failure-archaeology` |
| Mechanics of writing/running tests | `ck-tests-authoring-and-running` |
| What to build next | `ck-feature-frontier` |

---

## 1. Simple plan, or the phase-gate system?

Invoke the full doc set (§3) when **any one** of these holds:

| Trigger | Why the machinery earns its cost |
|---|---|
| Work will span **more than one session** | Context dies between sessions; only documents survive |
| **>~10 files** touched | Too big to hold the state in one head or one diff |
| **New module or architectural change** | Needs locked decisions + usually a CTO review (§8) |
| **Any campaign with unknowns** — a step whose outcome you cannot predict | Unknowns need gates with branch-on-observation, not a straight-line task list |

Otherwise a **simple plan suffices**: files touched / approach / risks (root CLAUDE.md,
Collaboration protocol), written in the conversation, plus per-step verification in the
`step → verify:` form (§2). No PROMPT/GATE/PROGRESS files for an afternoon's work.

**Budget honestly.** The CkNavigation campaign was chartered `window: 8 days`
(`Source/CkNavigation/PLAN.md:4`, plan dated 2026-04-29); execution continued through 2026-06-27
(confirmed: `git log --since=2026-04-30 -- Source/CkNavigation Source/CkCrowd` = 67 commits,
newest `db6eb1990` 2026-06-27). Real campaigns overrun several-fold — write docs that survive
that, and re-date estimates at each gate entry instead of pretending the calendar held.

## 2. Research → Plan → Implement, operationalized

The root doctrine gives the rule ("never jump to code"); this is what each phase's **output** looks
like.

**Research output is a list, not a feeling.** Before planning, write down (in PROGRESS.md for a
campaign, in the conversation otherwise):

1. Files read — actual paths.
2. Patterns extracted — named, with source (e.g. "request-enqueue is
   `AddOrGet<FFragment_X_Requests>()._Requests.Emplace(...)`, per CkIsmProxy_Utils.cpp").
3. **"The neighboring feature I will mimic is X"** — one sentence, one named module. This is root
   non-negotiable #1 (mimicry beats invention; `CkTimer` is the canonical small quartet). If you
   cannot name the neighbor, research is not done.
4. What you verified vs what you assumed.

The CTO reviews enforce exactly this: every review brief names reference modules and expects the
reviewer to "spot-check that the patterns it claims to follow actually look like existing code"
(2026-05-06 IskmRenderer review, Reviewer brief). Research that names its exemplars survives review;
research that doesn't produces blockers.

**A plan contains, per step:** files touched / approach / risks / **verification** — the
`step → verify:` form. Real example (this is Gate 2's own sub-task and smoke test,
`Source/CkNavigation/Plan/Gate_02_Locomotion.md`, Sub-task 2A):

```
Step: FProcessor_CrowdAgent_ApplyOffset — pattern replication of CkProjectile_Processor.cpp:23-31
→ verify: CkCrowd compiles clean; set velocity manually to (100,0,0);
          SceneNode advances ~100 cm/s in PIE [EDITOR-VERIFY: open Crowd gym, spawn 1 agent, observe]
```

Two properties make that a good step: it **names the proven pattern it copies** (file:line), and its
verify clause is an **observation**, not an activity ("SceneNode advances", not "test the
processor"). A step that replicates a proven pattern is cheap; a step that is new infrastructure
must be flagged as such — Gate 2's risk table literally records the CTO downgrading a risk from
High to Resolved once the chain was proven to "work today via the projectile pattern".

**Implementation runs against checkpoints** — §5. Never more than one work item between
checkpoints.

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

## 4. Reality checkpoints — named moments and what to actually run

| Moment | What to actually run | Record in PROGRESS.md |
|---|---|---|
| **After each work item** | Compile the touched modules; run the feature's targeted AutoTests (invocation shapes: `ck-tests-authoring-and-running`) | Result counts **vs the baseline captured at gate entry** — "no regressions" is meaningless without the recorded starting numbers |
| **Before each new component/gate** | Re-run the previous gate's exit observations once more (catches drift landed since); execute the new gate's entry pre-flight; run the §7 staleness sweep over the doc set | New baseline + HEAD hash in the gate's entry block |
| **When something feels wrong** | STOP. Write the observation down (dated entry) BEFORE touching code. Then §5 stuck protocol / `ck-debugging-playbook` | The symptom verbatim, and what you expected instead |
| **Before declaring a gate/campaign done** | The gate's full exit checklist; the `ck-change-control` done-checklist for the change class; every editor-only check listed as `[EDITOR-VERIFY]` with exact steps (root non-negotiable #7 — agents cannot launch PIE) | Evidence per exit item; the confirmed/inferred split (§6) |

A green compile is not a checkpoint result; the observation the gate names is. Gate 0's acceptance
criteria are the model: not "debugger module compiles" but "removing an agent removes the row from
the list within 1 frame" (`Gate_00_Foundation.md`, criterion 6).

## 5. The stuck protocol

From root CLAUDE.md (Collaboration protocol), operationalized:

**STOP → delegate investigation → step back and re-read the requirements → simplify → ask the
maintainer "[A] vs [B]?".** Two failed attempts = stop and present options. **Never a silent third
attempt.**

- *Delegate investigation* = a read-only research pass (subagent or your own) that gathers
  evidence without editing code — and a check of `ck-failure-archaeology` for prior attempts.
- *Present options* = each option with its concrete failure mode and cost, your recommendation
  first.

**The repo's own exhibit of this done right — Gate 3.** Separation sub-tasks landed; PIE smoke
tests showed head-on agents **vibrate in place, then clip through each other** (two observed
failures of the tuned approach, commits `adb75c6` + `03b28c8` cited in the doc). The response was
not a third tuning pass. It was a written pivot:
`Source/CkNavigation/Plan/Gate_03_Separation_Addendum.md` — dated 2026-05-02, headed
"**Supersedes:** the post-Gate-2 portion of Gate_03_Separation.md / **Does not change:** Gate 0,
Gate 1, Gate 2", diagnosing the core defect by name (the solver has "neither a commitment bias nor
predictive avoidance"), citing the two reference systems studied, and proposing a phased hybrid —
followed by a fresh implementation plan (`Gate_03_Separation_Hybrid_Plan.md`) whose pre-flight
**reproduces the baseline defect first**. The failed approach was superseded in writing, with the
supersession scope stated, instead of silently patched.

## 6. The evidence bar

Applies to every root-cause claim and every status report in a campaign:

1. **One mechanism must explain ALL observations — including the negatives.** Ask explicitly: "if
   my theory were true, X would also happen — does it?" A fix whose theory cannot explain a
   negative observation is not root-caused; it is a symptom patch wearing a theory. (The Gate 3
   addendum passes this bar: one named defect — no commitment bias, no prediction — explains BOTH
   the vibration AND the clip-through.)
2. **Reproduce before claiming cause.** A diagnosis you haven't reproduced is a hypothesis; write
   it as one.
3. **Every status line distinguishes confirmed from inferred.** Confirmed names its evidence — the
   file:line, the command run and its output, the log verdict read. Inferred says so and names
   what would confirm it. The PROGRESS.md entry template (§3.3) has both slots; use both. The CTO
   reviews model this at review scale — each carries a "Convention compliance spot-checks
   performed" section listing the exact files read and what each confirmed.
4. A subagent's "COMPLETE", a stale plan note, a green run from before your last edit — all
   hypotheses until you re-run the gate or read the artifact yourself.

Deeper failure-diagnosis method (instrumentation, bisection, log forensics):
`ck-debugging-playbook`.

## 7. Staleness discipline — the campaign's own excavated lesson

**The rule: every living doc is updated at every gate, or explicitly killed with a tombstone line
("Superseded by X — kept for history", dated).** A doc that is neither current nor tombstoned
misleads with the full authority of a committed file. Two real exhibits, both verifiable today:

**Exhibit A — the CkNavigation gate table.** `Source/CkNavigation/PLAN.md` says
`last_updated: 2026-04-29`; its status table shows Gates 2–7 "⏳ Pending". The table was last
touched by `51813611b` (2026-04-30, "docs(Navigation): mark Gate 1 done") — while **67 commits**
landed on `Source/CkNavigation` + `Source/CkCrowd` from 2026-04-30 through 2026-06-27 (steering,
separation, the Gate-3 hybrid, doorways, debugger overlays) and the table never moved.
`Source/CLAUDE.md` (2026-07-02) confirms both modules are fully built. The executive index rotted
the moment execution outpaced bookkeeping — and note the sharpest part: **the update rule was
written inside the doc** ("Update the Status column here as gates land", PLAN.md:89) and every
gate's done-checklist ends
with "PLAN.md status row updated to ✅ Done" — the rule existed and was still skipped. A rule inside
a doc is not enforcement; **welding the doc update into the gate's landing commit is** (§3.2 exit
criteria). Secondary drift in the same campaign: `Gate_00_Foundation.md`'s own header still says
"⏳ Pending" with an unchecked done-list while PLAN.md marks it ✅ Done — when status lives in two
places, update both in one commit or keep it in one place.

**Exhibit B — the CkTests continuation prompts.** `Plugins/CkTests/Script/Progress.md` (a
2026-01-17 one-shot conversion report wearing a living-doc name),
`Script/CkIskmRenderer/CONTINUATION_PROMPT_{GymTesting,PostFixCleanup}.md`, and
`Script/Common/CONTINUATION_PROMPT_GymStation.md` are finished-session campaign docs from the
CkPlugins host era. Every repo path in them (`D:/Repos/CkPlugins/...`) is wrong for this host,
their "next session:" instructions completed long ago, and GymStation's opening line depends on a
screenshot "pasted alongside this prompt" that no longer exists. They mislead every later reader —
CkTests/CLAUDE.md now carries a Warnings entry against them and
`.claude/reports/DECISIONS.md` item 17 records the deletion recommendation. Their failure was
lifecycle, not content: no freshness header, no death condition, no owner. §3.1's header block is
the fix.

**Corollaries:**

- Plans snapshot **conventions** too, and conventions move: `Gate_00_Foundation.md`'s file
  inventory teaches a `ProcessorInjector/` subdirectory — a mechanism the root doctrine has since
  retired ("any doc mentioning it is stale"). On resuming any plan, spot-check its code shapes
  against current doctrine before executing them (that's the §3.2 entry-criteria item).
- Doc-drift is a **review finding**: the CkEqs CTO review caught the brief's "locked-in decisions"
  contradicting the plan twice in one review and noted "this is the second time I've reviewed a
  CkFoundation plan where the brief's locked-in section drifted from the plan's actual state" —
  with the fix: regenerate the brief's decision table **by reading the plan**, not from session
  memory (`docs/reviews/2026-05-08-CkEqs-CTO-review.md`, "Brief context drift").
- **Session-resume staleness sweep** (Git Bash, cwd = the plugin root) — run before trusting any
  campaign doc:

  ```bash
  # When did the doc last move, and how much code moved since?
  DOC=Source/<Module>/PLAN.md
  LAST=$(git log -1 --format='%ad' --date=short -- "$DOC")
  echo "doc last touched: $LAST"
  git log --oneline --since="$LAST" -- Source/<Module> Source/<SiblingModule> | wc -l
  ```

  Code moved and the doc didn't → distrust the doc; update or tombstone it FIRST, then work.

## 8. The CTO-review pattern

**When to request one:** the change classes `ck-change-control` routes to maintainer review —
framework invariants, new modules, architectural changes. In the record, reviews happened at two
altitudes: **design-spec review before any plan exists** (CkSnapshot — "if you flag a blocker now,
we revise the spec… then come back to you for a separate plan review") and **implementation-plan
review before execution** (IskmRenderer's ~4000-line 17-phase plan; CkEqs's ~1000-line prompt).
Both are "the last set of eyes before N weeks of work" — the review exists because plan-stage fixes
are mechanical and code-stage fixes are refactors.

**Mechanics:** one file per review in `Plugins/CkFoundation/docs/reviews/`, named
`YYYY-MM-DD-<Subject>-CTO-review.md`. The requesting author writes the **Reviewer brief**; the
reviewer fills the **CTO Review Response** section in the same file and commits; re-review passes
append to the same file, so the verdict's evolution is one readable history.

**What the brief contains** (all three reviews share this anatomy — distill, don't skip parts):

| Brief section | Content — and the standard it sets |
|---|---|
| Role framing | "Senior reviewer/architect, last eyes before N weeks"; catch (1) expensive-mid-implementation architecture issues, (2) convention mismatches that cause review churn |
| What's being built | 2–3 paragraphs + product motivation |
| Artifact location | The spec/plan path; "read it in full" |
| Reference modules to spot-check | Named files — with the explicit instruction to **read repo code, not review the plan in isolation** |
| Critical context | The CLAUDE.md chain to pre-load |
| **Design decisions already locked in** | Numbered; "do NOT relitigate unless you see a real problem". Regenerate this table from the plan at send time (§7, CkEqs lesson) |
| **What I specifically want you to scrutinize** | The recurring axes: **A** architecture/decomposition · **B** CkFoundation convention compliance · **C** engine/version-specific APIs and algorithm correctness · **D** test coverage · **E** are the called-out risks sized correctly · **F** forward-compat with deferred work. Plus any bespoke hard question (CkSnapshot: "scrutinize the AngelScript surface in particular") |
| Output format demand | "Be direct. Green-light or list specific blockers; don't manufacture issues to look thorough. 'Testing strategy is unclear' is not actionable; 'Phase Q needs explicit failure modes for the async-load test' is." |

**What the response contains** (what a CTO review actually checks, observed across all three):

1. **Verdict** — `CHANGES REQUESTED` / `GREEN-LIGHT WITH NON-BLOCKING NOTES` / `GREEN-LIGHT`.
2. **Blocking issues** — numbered; each states what is wrong, the **repo evidence at file:line**
   that proves it, and the exact fix (e.g. IskmRenderer blocker #2 cites the sibling
   `CkIsmProxy_Processor.h` group assignments; CkSnapshot blocker #1 samples five live fragments to
   prove the chosen serializer cannot compile against them).
3. **Non-blocking suggestions** — folded or skipped at the author's discretion.
4. **"Convention compliance spot-checks performed"** — the files the reviewer actually read, with
   what each confirmed. This is the review's own evidence trail (§6 applies to reviewers too).
5. **Design/architecture observations** — including forks considered and rejected, so they aren't
   re-litigated ("EQS and Targeting eventual merge? No — they share zero algorithm").
6. **Sign-off conditions** — "GREEN-LIGHT flips when…", numbered and mechanical.
7. Reviewer name + date.
8. **Re-review** appended in-file, mapping each blocker → where it was resolved → verdict; the
   CkSnapshot v2 table (`v1 blocker | v2 location | Verdict`) is the model, and re-reviews verify
   resolutions at the artifact's line numbers, not by trusting the author's summary.

All three reviewed campaigns shipped (CkEqs, CkIskmRenderer, CkSnapshot are live modules in
`Source/`) — and each review found real blockers first. That is the pattern working.

## Common mistakes

| Mistake | Correction |
|---|---|
| Gate marked done in the index but not the gate file (or vice versa) | Both in ONE commit — or keep status in one place only (Exhibit A secondary drift) |
| "Next session should…" written into an undated doc | Volatile state lives only in PROGRESS.md's current-state block; every campaign doc carries a freshness header + death condition |
| Completion claimed while an open-items section is unresolved | DEBUG_CALLSTACK's "100% coverage" beside 19 unchecked modules — resolve or tombstone the open section first |
| PROMPT.md carrying branch names / uncommitted-file lists | That is PROGRESS.md content; PROMPT.md is stable by contract |
| A silent third attempt after two failures | §5: stop, write the pivot (addendum with Supersedes:/Does-not-change:), present "[A] vs [B]?" |
| Executing a resumed plan's code shapes verbatim | Plans snapshot conventions; re-verify against root doctrine first (ProcessorInjector trap) |
| "No regressions" without a recorded baseline | Capture counts + failing names at gate entry; diff at exit |
| Green suite treated as gate exit | Run the gate's own expected observations; a suite says nothing about paths it doesn't exercise |
| CTO brief's locked-decisions written from memory | Regenerate from the plan text at send time (CkEqs drift, flagged twice) |
| Treating a review blocker list as advisory | Sign-off conditions are gates; re-review maps each to its resolution before work starts |

## Provenance and maintenance

Authored 2026-07-02 (Ck skill campaign, Phase 2). Every artifact cited was read in full and every
staleness/count claim below was re-derived from git that day. Re-verification (Git Bash, cwd =
`Plugins/CkFoundation` unless noted):

- **Gate-table staleness numbers:** `git log -1 --format='%h %ad %s' --date=short -- Source/CkNavigation/PLAN.md`
  (expect `51813611b 2026-04-30`) and
  `git log --oneline --since=2026-04-30 -- Source/CkNavigation Source/CkCrowd | wc -l` (67 at
  authoring; grows). PLAN.md status table: `Source/CkNavigation/PLAN.md:93-104`; the in-doc update
  rule at `:89`. These docs are slated for post-ship deletion per PLAN.md §"Post-ship cleanup" —
  if gone, this section's summary stands as the record.
- **Gate exhibits:** `rg --no-ignore --files Source/CkNavigation/Plan` — Gate_00/02 (template
  sources), Gate_03_Separation_Addendum.md + Gate_03_Separation_Hybrid_Plan.md (pivot exhibits,
  both committed 2026-05-02).
- **CTO reviews:** `ls docs/reviews/` — exactly 3 files at authoring
  (2026-05-06 IskmRenderer plan-1, 2026-05-08 CkEqs, 2026-05-20 CkSnapshot design).
- **Rollout-checklist exhibit:** `Source/DEBUG_CALLSTACK_PROGRESS.md` (its "Modules to
  Investigate" vs "Implementation Complete!" contradiction is at lines 176-231).
- **CkTests stale docs:** `rg --no-ignore --files ../CkTests/Script | grep -Ei 'CONTINUATION|Progress'`
  — 4 files at authoring. Flagged stale in `../CkTests/CLAUDE.md` (Warnings) and
  `.claude/reports/DECISIONS.md` item 17 (deletion recommended) — they may legitimately vanish;
  the lesson doesn't.
- **Root collaboration section:** `rg -n 'Collaboration protocol' CLAUDE.md`.
- **Tooling caveat:** the Grep/Glob tools are blind under this plugin's `docs/`, `Script/`,
  `Content/` and CkTests' `Script/` (superproject `.ignore`) — verify with Bash
  `rg --no-ignore -n` or `Get-ChildItem`, per root CLAUDE.md provenance notes.
