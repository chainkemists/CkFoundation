---
name: ck-change-control
description: Use when gating a change to the Ck plugins — classifying the diff (docs-only, additive API, behavior change, framework-invariant touching CkEcs core/handles/signals/replication/snapshot) and picking its compile/test/review gates; when tempted by stock ensure/ensureMsgf/check or log-and-continue; before claiming done without Blueprint/AngelScript verification; when two conventions look equally plausible and nothing written decides. Not for diagnosing failures (ck-debugging-playbook) or multi-session planning (ck-methodology).
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

## The non-negotiables (rationale + evidence)

The one-line rules are root `CLAUDE.md` non-negotiables #1–#4. This section carries the *why* and
the evidence, so you can defend the rule instead of just obeying it.

### 1. Fire `CK_ENSURE_IF_NOT` — never stock `ensure`/`ensureMsgf`/`check` for recoverable validation

Compile modes, verified at `Source/CkCore/Public/CkCore/Ensure/CkEnsure.h:44-53` + the define
matrix in `Source/CkBuildConfig/CkBuildConfig.Build.cs`:

In two lines (the full 17-define × 5-config matrix and expansion mechanics are owned by
`ck-macros-and-codegen` §2.4): **`CK_DISABLE_ENSURE_CHECKS=0` in every standard configuration —
including Test and Shipping** — so the predicate evaluates and the recovery block runs (silently
where `_DEBUGGING=1` drops the diagnostics). Only the possibly-vestigial Profile override expands
to `if constexpr(false)` and compiles guards out.

Why this macro is the mandate:

- **It stays active in Shipping.** `CHECKS=0` in every standard configuration — the guard branch
  ships, unlike stock `ensure` (compiled out of Shipping by default). The Profile configuration
  that removes guards is reachable only by editing the hardcoded const at
  `CkBuildConfig.Build.cs:47` and is flagged possibly-vestigial (`DECISIONS.md` §26).
- **It is loud with context.** fmt-style message with the failing values, plus C++ AND Blueprint
  AND AngelScript callstacks, frame number, PIE-ID, server/client attribution
  (`CkEnsure.cpp:92+`). `PLATFORM_BREAK()` is expanded textually at the call site so the debugger
  lands in *your* frame, not a wrapper (`CkEnsure.h:34-35`).
- **It cannot spam.** Fires are counted per file:line (`CkEnsure.cpp:108`) against an ignore list
  (`:110-111`); log-only display policies auto-ignore the site after the first fire, and the
  editor dialog offers per-site ignore.
- **It fails your test gate.** The AutoTest runner forces LogOnly display
  (comment `CkEnsure.cpp:217-219`), so a fired ensure logs at **Error** severity — which the
  automation framework reports as a test failure. Ensures are visible in CI; warnings are not.
- **Shipped tripwires catch real bugs.** The packaged-build GC crash was diagnosed
  (`d77810096`), root-caused (`feb08ee94` — pre-GC rooting pass), and then fenced with a tripwire
  ensure that stays in the build (`a8a93baac`). Detection survived the fix.

Consequences for the `{ ... }` recovery block — it must be a **pure bail-out** (`return {};` /
`return;` / `continue;`) that is *correct silent-failure behavior*, because in Test/Shipping it
runs for real with zero diagnostics, and under Profile it does not exist at all. Real shape
(`Source/CkEcs/Public/CkEcs/EntityLifetime/CkEntityLifetime_Utils.cpp:114-117`):

```cpp
CK_ENSURE_IF_NOT(InHandle.Has<ck::FFragment_LifetimeOwner>(),
    TEXT("The Entity [{}] does NOT have a LifetimeOwner. Was this Entity created by Request_CreateEntity(RegistryType)?"),
    InHandle)
{ return {}; }
```

Variants for other shapes: `CK_TRIGGER_ENSURE` (unconditional), `CK_TRIGGER_ENSURE_IF`,
`CK_INVALID_ENUM` (unreachable switch defaults), `CK_ENSURE_VALID_IF_NOT(_MSG)`
(`CkEnsure.h:68-85`). Full macro mechanics: `ck-macros-and-codegen`.

Self-review grep before submitting (Git Bash, cwd `Plugins/CkFoundation`):

```bash
rg --no-ignore -n '\b(ensure|ensureMsgf|ensureAlways|check|checkf)\s*\(' <your changed files>
```

Any hit you introduced in runtime Ck code is a review rejection unless engine-forced.

### 2. Never silently handle an error

Maintainer's direct statement (2026-07-02, recorded in `.claude/reports/DECISIONS.md` §22): the
mandate is to fire an ensure rather than silently ignore the error behind a Warning/Error log,
because **logs get ignored, ensures don't**. A log-and-continue where validation failed is a
review rejection — and so is any fallback that hides a problem (root `CLAUDE.md` #3).

Decision table when a check fails:

| Situation | Do |
|---|---|
| Recoverable precondition violation (bad handle, missing fragment, invalid params) | `CK_ENSURE_IF_NOT` + pure bail-out |
| Unreachable branch (switch default on an enum) | `CK_INVALID_ENUM` / `CK_TRIGGER_ENSURE` |
| Legitimate, expected absence | `TryGet_*` contract: return the invalid handle quietly, **no ensure** — absence is not an error. (`Get_*` must not fail; `TryGet_*` may. Root `CLAUDE.md` naming rules.) |
| "Log a warning and use a default" | Never, unless the maintainer explicitly requested a fallback |

### 3. Research first — the reading ritual

Maintainer's direct statement (recorded in `DECISIONS.md` §23): incoming sessions "don't research
the codebase enough". The test from root `CLAUDE.md` #1: **if your change doesn't resemble the
code around it, you have not researched enough.**

For an ECS change, read IN THIS ORDER before writing anything:

1. Root `Plugins/CkFoundation/CLAUDE.md` — re-read the style block and non-negotiables.
2. `Source/CLAUDE.md` — locate your module via the "I need to…" decision tree; confirm dependency
   tier discipline (deps only point to same-or-lower tiers).
3. `Source/<TargetModule>/Claude.md` — purpose, key API, anti-patterns. Some are stale
   (`DECISIONS.md` §15): trust code over doc on conflict and note the drift.
4. The nearest sibling feature's **quartet** — `CkTimer` is the canonical small exemplar
   (`Source/CkTimer/Public/CkTimer/`), in this order:
   - `CkTimer_Fragment_Data.h` — reflected surface: ParamsData, requests, typed handle, delegates
   - `CkTimer_Fragment.h` — runtime fragments, tags, signals, record-of-children
   - `CkTimer_Processor.h` + `.cpp` — phases, `CK_REGISTER_PROCESSOR`, request draining
   - `CkTimer_Utils.h` + `.cpp` — the only public API surface
   (When scaffolding a whole module, also mimic the module-root `CkTimer_Log.h/.cpp` and
   `CkTimer_Module.cpp/.h`.)
5. Macro mechanics you're about to use → `ck-macros-and-codegen`. Architecture invariants you're
   about to lean on → `ckecs-architecture-contract`.

Then mimic. In your plan, cite what you read (file:line) — mimicry of adjacent code beats
invention, and reviewers check.

### 4. Three environments: C++, Blueprint, AngelScript

Root `CLAUDE.md` #4: every public API must work — and be **verified** — in all three. "Works in
C++" is one third of done. What each environment's check actually is:

| Env | "Verified" means | How |
|---|---|---|
| C++ | Compiles AND is exercised — a test calls it | Host editor build (`ck-build-and-env`) + tests (`ck-tests-authoring-and-running`) |
| Blueprint | The reflected surface renders and behaves in the BP editor | `[EDITOR-VERIFY]` checklist below — agents cannot launch the editor; a human runs it |
| AngelScript | The binding registers at editor boot and the call form works at runtime | Headless boot + log grep (shape below); full recipe + the silent-break catalog: `ck-angelscript-interop` |

One API in all three (Timer exemplar):

```cpp
// C++ — CkTimer_Utils.h:53-56
auto TimerHandle = UCk_Utils_Timer_UE::Add(InHandle, TimerParams);
```

- Blueprint: node **"[Ck][Timer] Add New Timer"** (DisplayName; Category `Ck|Utils|<Feature>` for
  the public surface, `Ck|BLUEPRINT_INTERNAL_USE_ONLY` for plumbing — both real, `CkTimer_Utils.h:50-68`).

```angelscript
// AngelScript — generated wrapper, Script/Generated/utils_timer.as:265-270
auto TimerHandle = utils_timer::Add(Handle, TimerParams);
```

#### `[EDITOR-VERIFY]` Blueprint checklist (exact steps — human-only)

1. Build the host editor and launch it (worked example BusterBlock; commands: `ck-build-and-env`).
2. Toolbar → Blueprints dropdown → Open Level Blueprint (any BP graph works).
3. Right-click the graph → type `[Ck][<Feature>]` → your new node appears under its DisplayName
   (untick "Context Sensitive" if hunting). Title must render as `[Ck][<Feature>] <Action>` — a
   missing prefix is a style bug.
4. Place the node. Every UENUM parameter renders as a dropdown listing all enumerators.
5. For functions with `meta = (ExpandEnumAsExecs = "OutResult")` (104 header call sites as of
   2026-07-02): the node shows one exec **output** pin per enum entry — e.g. `[Ck][Timer] Cast`
   shows `Succeeded` + `Failed` pins (`CkTimer_Utils.h:99-106`).
6. Request struct visibility: right-click → search `Make FCk_Request_<Feature>_<Action>` → the
   Make node exists and lists the private `_`-prefixed UPROPERTYs (BP-visible via
   `AllowPrivateAccess`). Alternatively add a BP variable of the struct type and inspect Details.
7. EditCondition behavior: open a Details panel showing a struct whose fields carry
   `EditCondition` (exemplar: `FCk_FloatAttribute_Magnitude`,
   `CkAttribute/Public/CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h:85-96` — this
   codebase usually pairs it with `EditConditionHides`). Flip the driving field
   (`_CalculationMode`): dependent fields must appear/disappear (or enable/disable without
   `Hides`).
8. If you added a typed handle: the `<As<Feature>>` autocast node appears, and a generic
   `FCk_Handle` wire auto-converts into the typed pin (`BlueprintAutocast` on the CastChecked
   surface — §2.7 machinery in `ck-macros-and-codegen`).
9. If you added a signal: the generated `BindTo_On<X>` node appears and accepts your delegate type.

Report the checklist as per-line pass/fail in your change summary.

#### AngelScript verification — the one-line shape

Boot the host editor headless so AS compiles, quit, then grep the **fresh** log (PowerShell, host
project root — BusterBlock worked example):

```powershell
& "Binaries\Win64\BusterBlockEditor-Cmd.exe" "$PWD\BusterBlock.uproject" -skipcompile -nullrhi -unattended -nosplash -ExecCmds="Quit"
rg -n "Angelscript: Error" Saved\Logs\BusterBlock.log
```

`-skipcompile` suppresses only the boot-time UBT/C++ compile (AS still compiles in-process, so the
gate is unaffected); without it, a canceled boot can orphan UBT and delete module DLLs in
multi-session environments — `ck-build-and-env` §4 owns the boot shape (trap T3).

Exact flags, the multi-instance generator lock, and why a clean boot is NOT enough (e.g. a raw
handle in an AS f-string throws only at PIE/exec time — `Script/CLAUDE.md` §22 item 1): all owned
by `ck-angelscript-interop`. If the change is callable from AS, exercise it at runtime (AutoTest or
PIE), not just at boot.

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
