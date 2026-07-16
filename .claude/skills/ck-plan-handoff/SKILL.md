---
name: ck-plan-handoff
description: "Use when /plan is invoked or when planning a Ck handoff package for a later lower-tier session. Planning only; do not implement or use to execute an existing plan."
---

# Planning a problem for execution by a weaker model

You are a Fable-class planner. Your output will be executed by an Opus- or
Sonnet-class Claude Code session with ZERO memory of this session. Assume the
executor: follows explicit instructions well, verifies what it is told to
verify, but improvises badly when reality diverges from the plan and
over-trusts its own inferences. Your job is to spend intelligence NOW so none
is required LATER.

**Hard rule: this session writes ONLY the planning package (docs below). No
source-code changes, no implementation, even if the fix looks trivial. If the
fix is genuinely trivial, say so and recommend skipping handoff entirely —
that is a valid outcome.**

## Phase A — Research until there are no surprises left
1. Load relevant project skills first (debugging-playbook, architecture
   contract, macros, tests, feature-recipe as applicable). Cite them in the
   plan; do not restate their content.
2. Read every file the executor will touch, plus the callers/consumers of
   those files. List them with one-line "why it matters" notes.
3. Reproduce or verify the problem/goal state where possible without the
   editor (compile, headless CkTests, source reading). Editor-only checks
   become `[EDITOR-VERIFY]` gates for the human.
4. Enumerate the solution space — at least two approaches — and KILL all but
   one yourself, recording why in the plan's "Rejected approaches" section.
   The executor must never choose an architecture. If two approaches are
   genuinely tied, STOP and ask the user; do not delegate the tie-break.
5. Hunt the traps: UHT limitations, AS binding surprises, friend/encapsulation
   boundaries, teardown ordering, stale Intermediate — anything from the
   failure-archaeology skill adjacent to this code. Each trap found becomes an
   explicit fence in the plan ("do NOT do X; it fails because Y").

Ask the user AT MOST three questions, only for intent the repo can't reveal
(acceptance bar, scope cuts allowed, timeline constraints). Then confirm the
chosen approach in ONE short message before writing the package.

## Phase B — Write the handoff package
Location: the feature/investigation folder, per ck-methodology structure.

- **PROMPT.md** — problem statement, chosen approach with rationale,
  rejected approaches with one-line kill reasons, full file inventory,
  glossary of every non-obvious term used, links to the skills the executor
  must load and WHEN ("before Phase 2, load ck-macros-and-codegen").
- **PHASE_N.md** per phase, sized so ONE executor session completes one phase
  with ≤50% of its context. Each phase contains:
  - Entry criteria (exact state the repo must be in; how to verify it).
  - Numbered steps with copy-pasteable commands and, where code is written,
    the actual signatures/struct layouts — pre-designed, house-style, all
    three environments where user-facing. The executor fills in bodies, not
    designs.
  - **Decision gates**: after every verification step, the EXPECTED
    observation (exact error text, test count, output snippet where possible)
    and enumerated branches: "if X → continue; if Y → step N.b; anything
    else → STOP, record in PROGRESS.md blockers, end session." Never leave
    an implicit "figure it out."
  - Exit criteria that are MEASURABLE (compiles, named tests pass, specific
    grep returns N hits) — never "works correctly."
  - Explicit fences: the wrong paths from Phase A, each with its reason.
- **PROGRESS.md** — from the ck-methodology template, pre-filled with all
  phases, plus a **Blockers** section the executor must use instead of
  improvising.
- **VALIDATION.md** (final phase) — the full acceptance protocol: headless
  test commands with expected results, three-environment checks,
  `[EDITOR-VERIFY]` steps for the human, and the definition of done routed
  through ck-change-control.

## Phase C — Adversarial self-review before handing off
Re-read the package as a Sonnet-class executor with no context:
- Is any step ambiguous enough to permit two interpretations? Fix it.
- Does any gate lack an "anything else → STOP" branch? Add it.
- Does any phase require a design decision? Pull it back into PROMPT.md.
- Is every command/path/signature verified against the repo this session?
Then hand the user: the package file list, the one paragraph they'd paste to
start executor session 1, and your confidence (high/medium) per phase with
the reason for anything medium.

## When NOT to use this skill
- The task fits comfortably in one session for the model at hand → just do it.
- You are the executor → read the package, don't regenerate it.
- The problem is investigation with unknown root cause → use the campaign
  pattern (ck-debugging-playbook / hardest-problem-campaign style) where
  branches are the deliverable, then hand off the FIX via this skill once
  the cause is known.
