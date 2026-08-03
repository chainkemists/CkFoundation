---
name: ck-failure-archaeology
description: "Use when checking whether a Ck failure, workaround, rejected design, or stalled branch has prior history before retrying, removing, or resurrecting it."
---

# ck-failure-archaeology — the chronicle of dead ends, reverts, and workarounds

## Overview

This is the Ck suite's incident chronicle: investigations, dead ends, rejected fixes, and reverts,
each reduced to SYMPTOM → ROOT CAUSE → EVIDENCE → STATUS → LESSON. Its job is to stop you from
re-running an experiment that already failed, deleting a workaround that is secretly load-bearing,
or "improving" a design whose shape is the scar tissue of a documented incident. Every SHA and
file:line below was re-verified against the repos on 2026-07-02.

## When NOT to use this skill

| You are… | Load instead |
|---|---|
| Triaging a live bug / build break / red test right now | `ck-debugging-playbook` (come back here if the symptom matches an entry) |
| Deciding what to build next | `ck-feature-frontier` |
| Asking why the architecture is shaped this way (invariants, not incidents) | `ckecs-architecture-contract` |
| Working the teardown/unbind lifecycle campaign | `ck-lifecycle-teardown-campaign` |

## How to re-verify an entry

Cwd = the BusterBlock superproject root (the plugins are git submodules there; under a different
host, substitute its plugin paths). Git Bash or PowerShell — commands are identical:

```powershell
git -C Plugins/CkFoundation show --stat --format='%h %ad %s' <sha>   # entries 1-10, 13
git -C Plugins/CkTests show --stat --format='%h %ad %s' <sha>         # entry 11
git -C Plugins/CkGameplayDebugger show --stat --format='%h %ad %s' <sha>  # entry 12
```

Tooling caveat: the Grep/Glob tools are blind under `Plugins/CkFoundation/Script`, `docs/`,
`Content/` (superproject `.ignore`). Use `rg --no-ignore -n` or Read with exact paths there.

---


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| The chronicle | `references/chronicle.md` |

## Stalled-branch registry — DO-NOT-RESURRECT

Maintainer ruling 2026-07-02 (recorded in `.claude/reports/DECISIONS.md` #25): stalled/outdated
remote branches are **do-not-resurrect**. If a listed idea is wanted again, it goes through
`ck-feature-frontier` as new work — do not rebase or cherry-pick these lines. All in the
CkFoundation repo; tip dates/subjects re-verified 2026-07-02 via
`git -C Plugins/CkFoundation log -2 origin/<branch>`.

| Branch (`origin/…`) | Last commit | What it attempted (tip subject, verbatim or tight paraphrase) |
|---|---|---|
| `feature/dependency-injection-entity-script` | 2026-05-26 | "feat, wip: Dependency Injection for entty script" [sic] — parked WIP, the youngest fossil |
| `feature/entity-script-as-script-struct` | 2026-04-02 | "refactor: migrate EntityScript spawn params from UUserDefinedStruct to UScriptStruct" — INFERRED superseded by the AS-generator spawn-params pipeline |
| `upgrade/ue5.7` | 2026-03-27 | "fix: Update InstancedStruct includes for UE 5.7 (StructUtils path change)" — parked engine line; its tip has a dev twin (`565e85ae5` IS on dev), i.e. pieces were cherry-picked back, the line itself is parked |
| `dev-bb-5.7` | 2026-04-06 | UE 5.7 compatibility line (attribute/utils fixes for 5.7) — parked with `upgrade/ue5.7` |
| `dev-bb` | 2026-03-27 | "feat: register attribute handle type traits for handle-level concept enforcement" — the pre-5.7 BB-integration line, parked alongside the 5.7 pair |
| `backup/before-reverting-f8a3a55` | 2024-01-11 | insurance snapshot taken before a revert ("added Processors for all Attributes' OverrideBaseValue processors") — a fossil, not work |
| `backup/pre-exporter-rewrite-2026-05-28` | 2026-05-22 | insurance snapshot before the asset-exporter rewrite — a fossil, not work |
| `bugfix/test/investigate-cheat-in-build` | 2025-02-19 | "test: Traces for cheats in build" — investigation branch, outcome unrecorded |
| `feature/entity-replication-channel` | 2025-01-23 | "Ecs Replication Channel (Actors) tied to the Transient entity … 'dangling' entity" — superseded by ActorRelay channels (INFERRED: same problem shape; ActorRelay is the shipped answer, cf. entry 7) |
| `feature/flow-graph-module` | 2025-01-04 | "feat, wip: Initial commit for the flow graph module" — abandoned module skeleton |
| `feature/message-without-name` | 2025-05-26 | "wip, refactor: Working on having messages no longer require a gameplay tag" — abandoned experiment |
| `feature/editor-time-construction-scripts` | 2025-06-16 | "On dev: Running construction scripts at editor time (there is another stash too, don't forget)" — stalled; per its own subject, part of the state is in a lost stash |
| `feature/ability-traits` | 2024-11-30 | "feat, wip: Started working on Ability Traits that compose an ability script." — abandoned |
| `feature/modular-traits` | 2024-05-14 | "wip, feat: Added new ModularTrait module w/ Conditions BP" — abandoned |
| `shelf/dummy-changes-to-debug-networking-asserts` | 2023-10-03 | "Several dummy changes to try and narrow down why some networking asserts are happening" — debugging shelf, oldest fossil |

**Dead pointer in live code:** `Source/CkEcs/Public/CkEcs/Registry/CkRegistry.h:289` —
"`// TODO: exposing the storage like this is temporary - see branch
feature/registry-handle-storage-support for what we really want to do`" — that branch **does not
exist on origin** (verified 2026-07-02: `git -C Plugins/CkFoundation branch -r --list
'*registry-handle*'` → empty). Treat the comment as intent-without-a-design; do not go hunting
for the branch.

Note: many *other* undeleted remote branches are NOT stalled — they landed on dev via
rebase/cherry-pick and just weren't pruned (twin subjects on dev). Check
`git -C Plugins/CkFoundation log --grep "<tip subject>" origin/dev` before classifying a branch
as dead.

## Common mistakes when using this chronicle

- **Deleting a "temporary" workaround because the commit called it temporary.** `in_place_delete`
  (entry 8; settled design per DECISIONS.md §45) and the FireIfPayloadInFlight policies (entry 9)
  are load-bearing. Check DECISIONS.md/ADJUDICATIONS.md before removing anything a commit body
  apologized for.
- **Trusting a plan doc's status table over git.** `CkNavigation/PLAN.md` has said "Gates 2–7
  Pending" since 2026-04-29 while the work landed (entry 5). Dev history wins.
- **Re-proposing a dead design without new evidence.** Typed WorldState (entry 3), executor
  discovery (entry 7), descriptor-driven AS regen (entry 4 Rev 9) each died for a mechanism, not
  taste. To re-open one, first state what changed about the mechanism.
- **Citing this file without re-verifying.** Entries are snapshots of 2026-07-02; a later fix may
  have closed an "open" entry. Run the `git show` line before repeating a STATUS.

## Provenance and maintenance

- Campaign date: **2026-07-02**. Verified at heads: CkFoundation `7330c1bab`, CkTests `b89f110`,
  CkGameplayDebugger `d607751` (all contained in their `origin/dev`).
- Every SHA above re-verified via `git -C Plugins/<repo> show -s --format='%h %ad %s' <sha>`;
  subjects quoted exactly. File:line claims read from disk the same day.
- Re-verification one-liners for the volatile facts:
  - Entry statuses / new fixes since: `git -C Plugins/CkFoundation log --oneline --since=2026-07-02 -- Source/<Module>`
  - `in_place_delete` still present: `rg -n 'in_place_delete' Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Signal/CkSignal_Fragment.h`
  - Dead branch pointer still in code: `rg -n 'registry-handle-storage-support' Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Registry/CkRegistry.h`
  - Branch registry still accurate: `git -C Plugins/CkFoundation for-each-ref --sort=-committerdate --format='%(refname:short) %(committerdate:short)' refs/remotes`
  - PLAN.md staleness: compare `last_updated` in `Plugins/CkFoundation/Source/CkNavigation/PLAN.md` against `git -C Plugins/CkFoundation log -1 --format=%ad -- Source/CkCrowd`
  - SM campaign count: `git -C Plugins/CkFoundation log --oneline --since=2026-05-01 --until=2026-06-26 -- Source/CkStateMachine | wc -l` (66 at verification)
  - A3 is resolved (DECISIONS.md §45); for the still-open items (A1/A2/A4) read `.claude/reports/ADJUDICATIONS.md` (an item may have moved to DECISIONS.md with a ruling)
- Adding an entry: keep the strict format (SYMPTOM → ROOT CAUSE → EVIDENCE (SHA/file:line) →
  STATUS → LESSON), quote commit subjects exactly, and mark anything you could not re-confirm
  INFERRED — a wrong SHA in this file poisons every future search that trusts it.
