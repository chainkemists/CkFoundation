# CONTINUATION PROMPT — CkIntent campaign, entering Phase 1a

> **Written:** 2026-08-08. **Audience:** an orchestrator session (Fable) dispatching Opus 5 agents to implement.
> **This doc dies when:** Phase 1a lands and `PROGRESS.md` carries its result.
> On death: delete it. Do not let it outlive Phase 1a — stale continuation prompts are this repo's
> documented failure exhibit (`ck-methodology` §7, Exhibit B).
>
> **PROGRESS.md is authoritative for live state. PROMPT.md is authoritative for decisions.**
> If this file disagrees with either, they win and this file is stale.

---

## 1. One-line summary

Design and planning for a two-module input/intent system (`CkInput` raw layer → `CkIntent` intent
layer) are complete and reviewed four times; **Phase 1a — a keybinding/key-icon gym — is unblocked
and ready to implement**, with zero production code written so far.

## 2. Read these first, in this order

All under `D:\Repos\CkPlugins\Plugins\CkFoundation\docs\campaigns\2026-08-07-CkIntent\`:

| File | Why |
|---|---|
| `PROMPT.md` | Mission, **25 locked decisions (D1–D25b)**, non-goals, reading list, "things ruled out". The contract. |
| `PROGRESS.md` | Live state, decision log, dated entries, 16 open items. **Authoritative for anything volatile.** |
| `PHASE_1A.md` | The gate to execute. Self-contained. |
| `DESIGN_InputLayering.md` | The ftxc-derived layering design. Read before any layer work. |
| `PHASE_0_RESEARCH.md` | Phase 0 findings. **Its "open decisions" list is stale — all five settled**; the file says so. |
| `PHASE_0.md` | Research gate. 0A/0F still blocked on a human. |

Also binding: `Plugins/CkFoundation/CLAUDE.md` (non-negotiables), and the `ck-methodology`,
`ck-tests-authoring-and-running`, `build-test` skills.

## 3. Repo state — READ BEFORE STAGING ANYTHING

- Project: `D:\Repos\CkPlugins` (UE 5.7.4, engine at `D:\Repos\UnrealEngineAngelscript`). Branch `dev`.
- **This is a game-agnostic plugin project.** `D:\Repos\BusterBlock` is a *consumer* of CkFoundation;
  its content practices are NOT normative here. Prefer C++/AngelScript over Blueprint/`.uasset` — always.
- Superproject HEAD `e9aed7d`. CkFoundation `b982baf24`. CkTests `fd26b553`, **on branch
  `backup/pre-branch-audit-265-gfd26b553`, not dev.**
- **Another workstream's state is in this tree** — `Config/DefaultGameplayTags.ini` modified, CkTests
  pointer moved, `CONTINUATION_PROMPT_CrowdDebuggerRuntimeSidewalksInvisible.md` at repo root.
  **Never blanket `git add`.** Enumerate dirty paths you did not author and leave them alone.
- **This campaign has committed nothing and pushed nothing.** All campaign docs are untracked.
- The user has not authorised any commit or push. Ask.

## 4. Baseline — the number every phase diffs against

Captured 2026-08-08 via
`./CkAuto/UnrealToolbox.exe --build --target=Editor --test --no-nullrhi --parallel 1 --output=Saved/Logs/BuildTest.log --project="D:\Repos\CkPlugins"`

```
Build succeeded
Total 1005 / Passed 1003 / Failed 2 / Skipped 0 / Contaminated 0 — 12m14s
```

The two pre-existing failures — **not this campaign's**, which has written zero code:
- `Ck_AutoTest_PathNetworkFollower_DesiredNavmeshClearanceMovesInward`
- `Ck_AutoTest_PathNetworkFollower_ProjectsRibbonWaypointWithinNavQueryExtent`

**"No regressions" means 1005/1003/2 with those two names — not zero failures.**
⚠ Only ONE run was made. Whether those two are deterministic or flaky is **UNVERIFIED**. A second
full run would settle it and is worth doing early. Re-capture the baseline entirely if the CkTests
branch/pointer moves.

## 5. What the campaign is

```
Unreal (Slate)  →  CkInput  →  CkIntent
                   raw acquisition,        sampling, frame records,
                   per-game biasing,       grammar, matching, arbitration,
                   device ownership,       intent output
                   layer stack,
                   existing EI binding/glyphs
```

Goal: fighting-game-grade intent (MK motions, Souls tap/hold, Sekiro timing) from one grammar, with
**zero added latency on inputs that are not genuinely ambiguous**. `CkInput` already exists and ships
EI mapping-context lifetime + a rebinding stack + CommonUI glyphs; it is being *extended*, not replaced.

Phases: `1a` (gym, now) → `1`/`1b`/`2` (CkInput raw layer, biasing, ButtonId map) → `3`–`8` (CkIntent).

## 6. Immediate work — Phase 1a

**`PHASE_1A.md` is self-contained; follow it.** Summary: build `Gym_Input_KeyBinding` + headless
AutoTests against the **already-shipping** `UCk_Utils_KeyBinding_UE` / `UCk_Utils_KeyIcon_UE`.

Why first: **VERIFIED that surface has zero test and zero gym coverage** — 361 lines of shipped
remap/conflict/swap/reset/persist logic plus glyph resolution, and the sibling CkGameSettings
campaign is building its keybinding page on it right now. Building the gym first makes it a
regression net before CkInput is extended.

**1a-0 is the gate.** Author the input actions + mapping context as **AngelScript script-literal
assets** (pattern: `Script/curves.as`, 30 × `asset CurveFloatEaseX of UCurveFloat` inside
`namespace curve`). Register via `Get_InputUserSettings(PC)` → `RegisterInputMappingContext(...)`.
The asset-registry scan is NOT needed — it is only a feeder; `RegisterInputMappingContext` is the
real mechanism (`CkKeyBinding_Subsystem.cpp:58`).

**If `Get_AllRemappableKeys` returns empty, STOP** — every station below it will false-pass.

### Suggested dispatch (orchestrator view)

| Unit | Depends on | Tier | Human needed? |
|---|---|---|---|
| 1a-0 AS input content + registration | — | **Opus** (unverified AS hop) | no |
| 1a-1 gym scaffold | 1a-0 | Opus | no |
| 1a-2/3/4/6 stations | 1a-1 | Opus | **yes, to drive them** |
| 1a-5 key-icon station | 1a-1 | Opus | **yes — controller hot-swap** |
| 1a-7 headless AutoTests | 1a-0 | Opus | no |
| O11 `CkRecord` ordering research | — | Opus, parallel | no |
| `CkInput/Claude.md` rewrite | — | Opus, parallel | no |
| Poll-surface design (blocks Phase 6) | — | Opus, parallel | no |
| Second baseline run (flakiness) | — | any | no |

1a-0, O11, the Claude.md rewrite, and the poll-surface design are **mutually independent** — dispatch in parallel.

## 7. Blocked on the human — do not attempt

| Item | Why |
|---|---|
| **0A hardware spike** | Slate analog cadence, `UserIndex` fidelity per device, sub-frame press-order recoverability. All **UNKNOWN**, unverifiable from the repo. Needs a throwaway `IInputProcessor` built + someone pressing keys on a keyboard AND gamepad. **Phases 1/2/3 depend on the answers**; Phase 1a does not. |
| **0F CkGameSettings boundary** | Confirmation from that campaign's owner that CkIntent deriving its ButtonId map from EI resolved mappings introduces no conflict. Blocks Phase 2. |
| 1a-4 / 1a-5 `[EDITOR-VERIFY]` | PIE restart persistence; controller hot-swap glyph switching. |
| Any commit / push / `.uasset` | Explicitly withheld. |

## 8. Open questions with no answer yet

| ID | Question | Blocks |
|---|---|---|
| — | Is `RegisterInputMappingContext` `BlueprintCallable` (∴ AS-visible)? AS compile answers in seconds. Fallback: add a `Register_MappingContext` wrapper to `UCk_Utils_KeyBinding_UE` (arguably a real gap). | 1a-0 |
| O11 | Does `CkRecord` guarantee iteration order? Decides layer ordering: ordered child record vs explicit priority ints. | Phase 1 |
| — | **The poll-surface hole.** D6 makes poll primary; D25b makes delivery a signal. Signals gate at fire time; a poll reads state. If layering gates only signals, every gameplay consumer bypasses the layer stack and layering is decorative. Two candidate shapes in `DESIGN_InputLayering.md`. | Phase 6 |
| — | Cross-module processor ordering: CkInput routing must precede CkIntent matching. Same-group order is registration order and is NOT safe — needs explicit earlier groups. | Phase 1 |
| O8/O9 | Intent tag namespace is named for a deprecated carrier (`ByteAttribute.Intent.*`); four `*Intent*` namespaces exist. | Phase 4 |
| O16 | Keybinding/key-icon zero coverage — Phase 1a closes it. | — |

## 9. Things ruled out — do NOT re-investigate

| Ruled out | Evidence |
|---|---|
| `CkByteAttribute` as intent-phase carrier | Same-tick request coalescing → one `OnValueChanged` with the last value only (`CkAttribute/CLAUDE.md:54`). Clamping is NOT the issue (`CkByteAttribute_Fragment_Data.h:55`, `ECk_MinMax::None` default). |
| Divorcing Enhanced Input for **binding** | `docs/specs/2026-08-05-CkGameSettings-design.md:205`, `:139`, `:17` — sibling campaign locked EI as the store; BusterBlock already migrated. |
| A parallel fixed-timestep accumulator | `TProcessorBase` already has `TickRate` + `ReplayMissedTicks` + remainder carry (`CkProcessor.h:243-294`). Only a max-catch-up clamp is missing (D19). |
| `CkSubstep` for the logic step | Physics-only; its own doc says don't. |
| Observing CkUI's `SuspendInput` (N6) | Rejected for layered consumption. N6 is DEAD — marked in place in `PHASE_0_RESEARCH.md`. |
| ftxc's `Consume`-returns-false contract (D17) | DEAD — D25b made captures declarative; no return value exists. Do not reintroduce a callback. |
| Rebuilding a rebinding stack / an EI importer / a keyboard-layout renderer | Non-goals in `PROMPT.md`. |
| "Fighting-game surface is speculative generality" | Withdrawn — CkFoundation is a framework; breadth is the requirement (D12). |

## 10. Gotchas learned this session — these cost real time

- **`Grep`/`Glob` are blind under this plugin's `Script/`, `docs/`, `Content/`.** Use Bash
  `rg --no-ignore` or `find`. A zero-result search here is NOT evidence of absence.
- **Anchor greps carefully.** AS asset syntax is *indented* (`    asset X of Y`); a `^asset` pattern
  returns a false empty. This produced a wrong conclusion in this session — distrust empty results.
- **Toolbox exit 1 on `--test` = test failures, not a build break.** Read the `=== Test summary ===`
  block. Exits 75–79 are not failures either (see the `build-test` skill).
- **Full-suite gates need `--no-nullrhi`** (null-RHI poisons ~all tests after an Iskm ensure storm)
  **and `--parallel 1`** for `--build --test` (parallel + build is an untested codegen race).
- **New AutoTests need `--discover-fresh`** — the toolbox caches discovery; a green run with an old
  Total is stale-green.
- **`SaveKeyBindings` writes real user settings to `Saved/`** — Phase 1a mandates teardown, or
  rebinds leak into later tests and the next baseline.
- Signal late-binders under `FireIfPayloadInFlight*` receive **only the LAST payload**
  (`CkSignal_Fragment_Data.h:12-17`) — signal replay can never reconstruct a sequence.
- Two Slate preprocessors already register at **priority 0** (loading screen, viewport picker) —
  registration order decides between them. Pre-existing latent defect, flagged not fixed.
- The editor must be closed for any `--build`; a `PreToolUse` hook enforces it.

## 11. Critical files

| File | What it is |
|---|---|
| `Plugins/CkFoundation/Source/CkInput/Public/CkInput/CkKeyBinding_Utils.h` | The 361-line shipped rebinding surface Phase 1a tests. **Real names:** `RemapKey`, `RemapKeys`, `SwapKeys`, `ResetMappingToDefault`, `ResetAllToDefaults`, `SaveKeyBindings`, `BindTo_OnMappingKeyChanged`, `UnbindFrom_OnMappingKeyChanged`, `Get_AllRemappableKeys`, `Get_HasKeyConflicts`, `UnbindConflictAndRemap`. |
| `.../CkInput/Subsystem/CkKeyBinding_Subsystem.cpp` | `:25-70` — the scan-then-`RegisterInputMappingContext` path. `:58` is the real mechanism. |
| `.../CkInput/CkKeyIcon_Utils.h` | CommonUI glyph resolution. |
| `.../CkInput/Claude.md` | **Wrong** — describes only IMC lifetime, omits the whole keybinding/glyph surface. Rewrite is a Phase 1 item. |
| `Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Processor/CkProcessor.h` | `:243-294` `TProcessorBase::Tick` — the unbounded `ReplayMissedTicks` `while` loop (D19). |
| `Plugins/CkFoundation/Source/CkEcs/Public/CkEcs/Signal/CkSignal_{Macros,Fragment_Data}.h` | Signal definition + binding-policy semantics. |
| `Plugins/CkTests/Script/CkAStar/CkAStar_GymStation.as` | Gym station pattern to mimic. |
| `Plugins/CkTests/Script/CkGymStation_Showcase/` | GameMode + PlayerController gym pair. |
| `Plugins/CkFoundation/Script/curves.as` | **The AS script-literal asset pattern for 1a-0.** |
| `Plugins/CkGameplayDebugger/Source/CkDebuggerCommon/Public/CkDebuggerCommon/Widgets/SCkDebug_EventTimeline.h` | Shared timeline widget Phase 7 extends (already used by CkGoapDebugger). |

## 12. Recommended flow for the new session

1. Read `PROMPT.md` then `PROGRESS.md`. Do not trust this file over them.
2. Confirm the tree state in §3 still holds (`git status`, submodule branches). If CkTests moved,
   re-capture the baseline before anything else.
3. Dispatch in parallel: **1a-0**, **O11 research**, **`CkInput/Claude.md` rewrite**,
   **poll-surface design**. They are independent.
4. Gate on 1a-0's verify — `Get_AllRemappableKeys` non-empty. If empty, stop and diagnose the
   registration hop before building any station.
5. Build 1a-1 scaffold, then stations and AutoTests.
6. Run the gate: full suite, `--no-nullrhi --parallel 1`, `--discover-fresh`. Diff against
   **1005/1003/2 + the two known names**.
7. Update `PROGRESS.md` (dated entry, confirmed vs inferred split) and `PHASE_1A.md` Status **in the
   same commit** as the work. Ask before committing.
8. Hand the human their queue: 0A spike, 0F, and the two `[EDITOR-VERIFY]` steps.

## 13. Suggested first message to the user

> I've read the CkIntent campaign docs. Phase 1a (the keybinding/key-icon gym) is unblocked and I'm
> dispatching four parallel units: the AngelScript input content for 1a-0, the `CkRecord` ordering
> research, the `CkInput/Claude.md` rewrite, and the poll-surface design.
>
> Before I start — two checks: your tree still has another workstream's state in it (CkTests on
> `backup/pre-branch-audit-265-gfd26b553`, `DefaultGameplayTags.ini` modified). Is that still live,
> and should I leave it untouched? And do you want a second full-suite run first to settle whether
> the two `PathNetworkFollower` failures are deterministic or flaky — the baseline rests on a single
> run.
>
> Still queued for you whenever you're free: the 0A hardware spike (I build it, you press keys for
> ~10 min), the CkGameSettings boundary conversation, and two `[EDITOR-VERIFY]` steps in Phase 1a.
