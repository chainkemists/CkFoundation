---
name: ck-change-control
description: "Use when classifying and gating a Ck change, selecting compile/test/review evidence, or enforcing ensure and cross-environment checks; not for diagnosis or campaign planning."
---

# ck-change-control

## Overview

Every change to the Ck plugin suite (CkFoundation, CkTests, CkGameplayDebugger) falls into one of
four classes, and each class has a gate it must pass before "done" may be claimed. The gates encode
what the maintainer actually enforces in review: loud validation, researched mimicry, and
verification in all three environments (C++, Blueprint, AngelScript). Facts below were verified
against source on 2026-07-02; re-verification commands are in the final section.

## When NOT to use this skill

| You are… | Load instead |
|---|---|
| Diagnosing a build/UHT/linker/AS-compile failure or crash | `ck-debugging-playbook` |
| Planning multi-session work (PROMPT/PHASE/PROGRESS docs) | `ck-methodology` |
| Writing or running the tests themselves | `ck-tests-authoring-and-running` |
| Checking whether an approach was already tried and abandoned | `ck-failure-archaeology` |
| Choosing what to build next | `ck-feature-frontier` |

## Change classification — find your row first

Jargon (defined once): a **fragment** is an ECS component; a **processor** is an ECS system; a
**handle** (`FCk_Handle`) is the typed entity reference; **Utils** (`UCk_Utils_[Feature]_UE`) is a
feature's only public API surface; a feature's **quartet** is its four file pairs
(`X_Fragment_Data.h`, `X_Fragment.h`, `X_Processor.h/.cpp`, `X_Utils.h/.cpp`).

| Class | You touched… | Gate (cumulative — each class adds to the one above) |
|---|---|---|
| **1 — Docs-only** | `*.md`, `Claude.md`, code comments | Verify every claim against code before writing it; date-stamp volatile facts. Comment-only edits in headers: recompile the host editor (headers rebuild their consumers) — nothing else. No tests, no editor session. |
| **2 — Additive API** | New UFUNCTION / request struct / signal / fragment / module; zero existing lines change behavior | Host editor target compiles. Style per root `Plugins/CkFoundation/CLAUDE.md` (do not restate it — read it). Three-environment verification (§Non-negotiable 4). Tests covering the new surface — tier decision per `ck-tests-authoring-and-running`. Target module's `Claude.md` updated if the public API moved. |
| **3 — Behavior change** | Existing processor/Utils logic, bugfix, UFUNCTION signature or default change | Capture the test baseline BEFORE editing (pass/fail counts + failing names); re-run the affected feature's suite after; report the delta, not a green screenshot. For a bugfix, add the failing repro test first where the tier allows. Check `ck-failure-archaeology` for prior attempts. Name what still speaks the old contract: host Blueprint graphs pinned to the old signature (they break on next asset open), AS callers (they fail at next editor boot), saved snapshots holding the old data shape. |
| **4 — Framework-invariant** | CkEcs core (`Handle/`, `EntityLifetime/`, `Signal/`, `Net/`, `Snapshot/`), replication paths, snapshot format, `CK_` macro definitions in CkCore/CkEcs, the ensure/build-config matrix | Full rebuild before trusting ANY test result (stale-binary trap below). Full suite across tiers, including net tests. **Maintainer review is mandatory, not optional.** Walk the invariant tripwire list below. |

Escalation at ANY class: an unwritten-norm fork → §When to stop and ask.

**Test tiers, one line each** (authoring/decision rules: `ck-tests-authoring-and-running`, home
CkTests): **AutoTest** = headless-PIE assertion (the default, ~95% of cases); **C++ unit** =
`FAutomationTestBase` for world-less utilities; **net** = generated client/server stubs
(`Ck.<Feature>.Net.AS_*`); **Gauntlet** = process-level boot of the real game; **Gym** =
interactive test level (manual, not a CI gate). The plugins are dual-hosted; BusterBlock is the
worked-example host throughout (build commands and env traps: `ck-build-and-env`).

### Class-4 invariant tripwires (walk before requesting review)

- **Typed-handle size.** `static_assert(sizeof(FCk_Handle_TypeSafe) == sizeof(FCk_Handle))` marked
  "DO NOT REMOVE" (`Source/CkEcs/Public/CkEcs/Handle/CkHandle_TypeSafe.h:76-80`). Typed handles
  must add zero data members.
- **Request-struct vtable variance.** `FCk_Request_Base`'s virtuals exist only
  `#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING` (`CkEcs/Public/CkEcs/Request/CkRequest_Data.h:95-103`);
  that flag is 0 in Debug/DebugGame/Dev-editor and 1 in Dev-noneditor/Test/Shipping
  (`CkBuildConfig/CkBuildConfig.Build.cs`). Requests are polymorphic in some configs and not in
  others — never memcpy them, serialize them raw, or static_assert their size.
- **Global fragment-storage pointer stability (`in_place_delete`) — not signal-only.** Every
  fragment pool is tombstone-mode by settled, deliberate design (`.claude/reports/DECISIONS.md`
  §45; ungated on purpose in `06938bba3`). Do not "fix", narrow, or re-gate it in passing —
  including the shadowed per-signal opt-ins.
- **Teardown/unbind area is a live defect campaign.** Anchor:
  `CkInteraction/Public/CkInteraction/InteractTarget/CkInteractTarget_Processor.cpp:222`
  ("This processor doesn't get called, can cause issues if teardown is mid interaction!!!"). Load
  `ck-lifecycle-teardown-campaign` before touching entity teardown or signal unbinding.
- **Stale-binary trap.** `CK_REGISTER_SNAPSHOTABLE` / `CK_REGISTER_PROCESSOR` are global
  registrations baked into the binary at static-init — a green run from a binary older than your
  last edit proves nothing. Rebuild, re-run the full gate (full telling: `ck-debugging-playbook`).


## Reference files — load only what the task needs

Section numbers cited elsewhere in this skill point into these files.

| Topic | Read |
|---|---|
| The non-negotiables — rationale and required evidence | `references/non-negotiables.md` |

## What "done" requires — the checklist

1. [ ] Class identified (table above); every gate for that class ran AFTER your final edit — a
   result produced before your last change is stale, not evidence.
2. [ ] C++ compiles: host editor target. If you touched AS-binding code: guard with
   `#if WITH_ANGELSCRIPT_CK` — the flag is auto-set per target (`CkBuildConfig.Build.cs:53-61`)
   and the `CK_ANGELSCRIPT_*` macros already have no-op twins for the off state
   (`CkCore/Public/CkCore/Macros/CkMacros.h:149-154`). Both states must compile (root `CLAUDE.md`
   Identity).
3. [ ] Tests per `ck-tests-authoring-and-running`, reported as a delta:
   "baseline N failing {names} → after: N failing {same names}" — never bare "tests pass".
4. [ ] AS: headless boot + fresh-log grep clean; runtime path exercised if AS-callable.
5. [ ] BP: `[EDITOR-VERIFY]` checklist run by a human, results reported per line.
6. [ ] Docs: target module's `Claude.md` updated if the public surface moved; a doctrine-level
   convention change gets a row in `.claude/reports/DECISIONS.md`.
7. [ ] Style self-review against root `CLAUDE.md` (trailing returns, `In*`/`_Member`, `{}`
   construction, `NOT`, named namespaces — the list lives there, not here).
8. [ ] Final re-read: every "verified" in your summary names its evidence (file:line, command run,
   log line). Anything only checkable in-editor is labeled `[EDITOR-VERIFY]`, not claimed.

## When to stop and ask — the ADJUDICATIONS protocol

Root `CLAUDE.md` #6, per the maintainer's standing instruction ("the agent just needs to ask me" —
`ADJUDICATIONS.md` header). Trigger: code and docs are silent, two reasonable conventions exist,
and the choice materially shapes future code.

1. Do NOT invent policy, and do not silently pick a side.
2. Ask the maintainer if reachable. Otherwise add an item to
   `Plugins/CkFoundation/.claude/reports/ADJUDICATIONS.md` with exactly:
   - **both sides**, steel-manned (Side A / Side B, each with its supporting argument);
   - **the evidence** — usage counts, file:line, git history for each side;
   - **the interim stance** your change follows until ruled (default: match the file you are
     editing; churn nothing in either direction).
3. When the maintainer rules, the item moves to `DECISIONS.md` with the ruling.
4. Check the open items first — your fork may already be filed. As of 2026-07-02: A1 `TOptional`
   in reflected surfaces, A2 C++ test pretty-name family, A4 entity preset pattern. (A3 global
   fragment-storage pointer stability is RESOLVED — see `DECISIONS.md` §45.)

Everyday judgment calls (naming within an established scheme, which sibling to mimic) are NOT
adjudications — decide, state the decision, move on. `DECISIONS.md` already records the settled
calls (44+ entries and growing) — check it first; do not re-litigate them.

## Common mistakes

| Mistake | Why it burns you |
|---|---|
| Trusting a green run from a binary older than your last edit | Global registrations (`CK_REGISTER_SNAPSHOTABLE`/`_PROCESSOR`) baked old code into that binary. Rebuild, re-run. |
| `ensureMsgf`/`check` for validation | Review rejection; compiled out (or crash-only) exactly where the Ck ensure would still guard. |
| Ensure recovery block that does work | Runs silently in Test/Shipping and is compiled out under Profile. Pure bail-out only. |
| Ensuring inside `TryGet_*` on legitimate absence | Absence is not an error; the TryGet contract returns an invalid handle quietly. |
| Claiming BP/AS parity from a C++ compile | Reflection/registration failures surface only at editor boot or node placement. Run env checks 4-5. |
| Hand-editing `Script/Generated/*.as` | Regenerated at editor startup; edits vanish (the "DO NOT EDIT" header — 273 of 274 files as of 2026-07-02). |
| Changing a UFUNCTION signature without sweeping callers | Host BP graphs break on next open; AS callers fail at next boot. Name them in your report (class-3 gate). |
| Filing an ADJUDICATIONS item for a settled rule | Check `DECISIONS.md` first. |

## Provenance and maintenance

Authored 2026-07-02 (handoff campaign). All file:line cites verified against the working tree that
day. Re-verify volatile facts (Git Bash, cwd `Plugins/CkFoundation`):

- Ensure compile modes: `sed -n '44,53p' Source/CkCore/Public/CkCore/Ensure/CkEnsure.h`
- Define matrix + Profile override const: `rg -n 'CK_DISABLE_ENSURE|BuildConfigurationOverride' Source/CkBuildConfig/CkBuildConfig.Build.cs`
- Fire-count / ignore behavior: `rg -n 'IncrementEnsureCountAtFileAndLine|IgnoreEnsureAtFileAndLine|IsUnattended' Source/CkCore/Public/CkCore/Ensure/CkEnsure.cpp`
- Real ensure shape: `sed -n '114,117p' Source/CkEcs/Public/CkEcs/EntityLifetime/CkEntityLifetime_Utils.cpp`
- Handle static_assert: `sed -n '74,82p' Source/CkEcs/Public/CkEcs/Handle/CkHandle_TypeSafe.h`
- Request vtable variance: `sed -n '93,105p' Source/CkEcs/Public/CkEcs/Request/CkRequest_Data.h`
- `ExpandEnumAsExecs` count (104 on 2026-07-02): `rg --no-ignore -c 'ExpandEnumAsExecs' Source --glob '*.h' | awk -F: '{s+=$2} END {print s}'`
- EditCondition exemplar: `sed -n '85,96p' Source/CkAttribute/Public/CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h`
- AS no-op stubs: `sed -n '149,154p' Source/CkCore/Public/CkCore/Macros/CkMacros.h`
- GC incident commits: `git log --oneline --no-walk d77810096 feb08ee94 a8a93baac`
- Teardown defect anchor: `sed -n '222p' Source/CkInteraction/Public/CkInteraction/InteractTarget/CkInteractTarget_Processor.cpp`
- Maintainer statements + settled calls: `.claude/reports/DECISIONS.md` (§15, §22-24, §26); open forks: `.claude/reports/ADJUDICATIONS.md`
- Generated AS wrapper exemplar: `rg --no-ignore -n -A4 '^\s+Add\(FCk_Handle' Script/Generated/utils_timer.as`

Tooling caveat: the agent Grep/Glob tools are silently blind under this plugin's `Script/`,
`docs/`, and `Content/` (superproject `.ignore`) — use `rg --no-ignore` in Bash there, and re-check
any zero-match with `rg --no-ignore --files` before concluding absence.
