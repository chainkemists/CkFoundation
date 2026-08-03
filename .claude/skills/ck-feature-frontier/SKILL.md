---
name: ck-feature-frontier
description: "Use when choosing what Ck framework work to build, harden, optimize, or prioritize next; not for live bug triage, campaign execution, or performance measurement."
---

# ck-feature-frontier — where the framework could advance next

## Overview

This is the vetted portfolio of candidate framework advancements for CkFoundation /
CkGameplayDebugger / CkTests, grounded in what the code actually contains as of 2026-07-02.
Everything here is a **candidate** — evidence-backed and scoped, but not committed roadmap; the
maintainer picks. Every "Gap" and "asset" claim below was re-verified against the repos on the
campaign date (re-verification commands in the final section).

## The ruling (maintainer, 2026-07-02 — binding)

> "Advancing the framework" = **ROBUSTNESS and PERFORMANCE first; debug tooling next** (tooling
> usually lands in CkGameplayDebugger — debug tooling is explicitly valued). **Stale/outdated
> branches MUST NOT be touched — they are context, not work items.**

In-repo record: `.claude/reports/DECISIONS.md` §25. Consequences:

- Rank candidates robustness > performance > tooling. Within a class, prefer the candidate whose
  **first step is a measurement or a failing test**.
- A proposal that amounts to reviving a fenced branch (below) is out of bounds — even if
  re-described in new words.

## The fence — do-not-resurrect branches

**The canonical registry lives in `ck-failure-archaeology`** (§"Stalled-branch registry —
DO-NOT-RESURRECT") — per-branch tip dates, history, and assessments live there, once. This section
only states the rule and the headline names (verified on origin 2026-07-02):
`feature/dependency-injection-entity-script` · `feature/entity-script-as-script-struct` ·
`upgrade/ue5.7` · `dev-bb-5.7` · `dev-bb` · `feature/editor-time-construction-scripts` ·
`feature/message-without-name` · `bugfix/test/investigate-cheat-in-build` ·
`feature/entity-replication-channel` · `feature/flow-graph-module` · `feature/ability-traits` ·
`feature/modular-traits` · `shelf/dummy-changes-to-debug-networking-asserts` · plus the backup
fossils `backup/before-reverting-f8a3a55` and `backup/pre-exporter-rewrite-2026-05-28` (insurance
snapshots, not work). Do not check them out, merge them, or restart their feature under a new name
without the maintainer's explicit say-so.

`feature/registry-handle-storage-support` is cited by a live comment
(`Source/CkEcs/Public/CkEcs/Registry/CkRegistry.h:289`) but **does not exist on origin** — don't
go hunting for it. Branches with tips from ~June 2026 onward are likely **active sibling work**
(e.g. the iskm/save-load lines) — coordinate, never adopt.

## When NOT to use this skill

| You actually have… | Load instead |
|---|---|
| A live bug / build break / crash to fix | `ck-debugging-playbook` |
| The entity-teardown / signal-unbind defect campaign | `ck-lifecycle-teardown-campaign` |
| "Has this been tried before?" | `ck-failure-archaeology` |
| How to benchmark / make a perf claim | `ck-performance-and-analysis` |
| Process, phase-gates, multi-session discipline | `ck-methodology` |
| How to add a debugger view once a tooling candidate is picked | `ck-gameplaydebugger-extension` |


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| The portfolio | `references/portfolio.md` |

## Prioritization

1. **Robustness first** (ruling): candidates 1, 2, 3 — and 6, whose payoff is robustness even
   though it ships as tooling.
2. **Performance second**: 4 and 5. Both are deliberately measure-first — the FIRST step is the
   benchmark, and "measured: negligible" closes them honorably. No perf change lands without the
   A/B number (root non-negotiable #7).
3. **Tooling third**: 7 and 8 — explicitly valued by the maintainer, and both land in their proper
   homes (7 in CkGameplayDebugger; 8 in CkFoundation's own analyzer module).
4. Within a class, prefer the candidate whose first step is a measurement or a failing test — all
   eight above were shaped to satisfy that.

## Common mistakes

- **Reviving a fenced branch by paraphrase.** "Dependency injection for entity scripts" in new
  words is still `feature/dependency-injection-entity-script`. The live DI-flavored mechanisms are
  CkProvider + ContextOwner (root CLAUDE.md lingo table); anything beyond them needs the
  maintainer.
- **Building before measuring** on candidates 4/5. The define flip and the group prototype are
  cheap to *write* — that is exactly why the ruling is benchmark-first; an unmeasured "optimization"
  is a review rejection.
- **Treating `in_place_delete` as a work item.** It is settled design (DECISIONS.md §45 — the
  ungating was deliberate); the open part is only candidate 5's measurement. "Restoring the debug
  gate" is not a cleanup — it would revert a considered 2026 storage decision.
- **Landing debug tooling in CkFoundation runtime modules.** Overlay/inspector work belongs in
  CkGameplayDebugger (`ck-gameplaydebugger-extension`); only the analyzer (already a CkFoundation
  UncookedOnly module) and the AS checker (generator-adjacent) live here.
- **Presenting this portfolio as a roadmap.** Every entry is a candidate. When you pitch one,
  carry its evidence (the file:lines above), not its conclusion.
- **Skipping the ask.** If developing a candidate surfaces a genuine convention fork, add it to
  `.claude/reports/ADJUDICATIONS.md` instead of inventing policy (root non-negotiable #6).

## Provenance and maintenance

Authored 2026-07-02 (documentation campaign; CkFoundation HEAD `7330c1bab`). All counts and lines
below drift — re-verify before relying on them. Commands are Git Bash from
`d:\Repos\BusterBlock\Plugins\CkFoundation` (the Grep/Glob tools are blind under `Script/` — use
`rg --no-ignore`):

- Ruling + fence records: `.claude/reports/DECISIONS.md` §25 + §45 (`ADJUDICATIONS.md` A3 is a
  resolved tombstone pointing at §45).
- Branch list: `git for-each-ref --sort=-committerdate --format='%(refname:short) | %(committerdate:short)' refs/remotes/origin`
- Teardown TODOs: `rg --no-ignore -n "teardown is mid interaction" Source/CkInteraction` (expect 2)
  · `rg --no-ignore -n "bullet-proof way" Source/CkRelationship` (expect 3)
- GC census: `rg --no-ignore -c "TStrongObjectPtr<" Source/ --glob '*_Fragment.h'` (13 total) ·
  same for `TWeakObjectPtr<` (35) and `TObjectPtr<` (4) · tripwire:
  `rg --no-ignore -n "IsInGameThread" Source/CkCore/Public/CkCore/IO/CkDeferredAssetInit_AngelScript.cpp` (:521)
- Snapshot: `rg --no-ignore -c "CK_REGISTER_SNAPSHOTABLE\(" Source/ --glob '*.cpp'` (sum = 119) ·
  `rg --no-ignore -c "struct (CK\w+_API )?FFragment_" Source/ --glob '*_Fragment.h'` (sum = 295) ·
  policy doc at `Source/CkEcs/Public/CkEcs/Snapshot/CkSnapshot_Policy.h:9-19`
- Net-param copy: `rg --no-ignore -n "CK_DISABLE_NET_PARAM_COPY_PER_ENTITY" Source/` (6 Build.cs
  sites all `=0` + 1 consumer at `CkEntityLifetime_Utils.cpp:478`)
- Groups / deletion policy: `rg --no-ignore -n "static_assert" Source/CkThirdParty/Public/CkThirdParty/entt-3.16.0/src/entt/entity/group.hpp`
  (in-place-delete rejection at :697) · funnel at `Source/CkEcs/Public/CkEcs/Registry/CkRegistry.h:162` ·
  global trait at `Source/CkEcs/Public/CkEcs/Handle/CkHandle.h:71-77` — confirm it is still
  ungated via `grep -n "#if" Source/CkEcs/Public/CkEcs/Handle/CkHandle.h | head -1` (first hit :79,
  i.e. after the trait) · origin chain: `git log --format='%h %ad %s' --date=short -G component_traits -- Source/CkEcs/Public/CkEcs/Handle/CkHandle.h`
  (returns `6b54d2e38` relocate + `745507381` introduce-gated) and
  `git log --format='%h %ad %s' --date=short -G CK_DISABLE_ECS_HANDLE_DEBUGGING -- Source/CkEcs/Public/CkEcs/Handle/CkHandle.h`
  (returns `06938bba3` = the deliberate ungating, + `232ae5e06` param rename). `-S` cannot find the
  move/ungate commits — the string count never changed.
- AS surface: `ls Script/Generated/*.as | wc -l` (274) · emitted-namespace claim at
  `Script/CLAUDE.md:113` · commandlet scope in `CkAngelscriptGenerator_DriftCommandlet.h:11-20`
- Overlay providers: `ls ../CkGameplayDebugger/Source/CkEntityDebugOverlay/Private/Providers/ | wc -l`
  (38 files = 19 providers)
- Processors: `rg --no-ignore -c "CK_REGISTER_PROCESSOR\(" Source/ --glob '*.cpp'` (sum = 388)
