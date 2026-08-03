# The CTO-review pattern

Reference for `ck-methodology`: how to prepare and run a plan review with a senior reviewer, and what the reviewer is asked to attack.

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

