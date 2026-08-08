# Phase 0 — Research, spike, and boundary settlement

> **Status:** 🟡 In progress — 0B/0D/0G/0H/0I answered; **0A and 0F blocked on a human**.
> **Gate exemption (2026-08-08):** the "Phase 1 does not start without maintainer review" clause below
> means *do not build the Slate preprocessor blind*. **Phase 1a is explicitly exempt** — it writes no
> production code and touches only already-shipping API, so it does not depend on any 0A answer.
> **Depends on:** nothing (campaign entry)
> **Estimate:** 2–3 days — re-date at entry; record actual at exit
> **Output:** `PHASE_0_RESEARCH.md` beside this file, plus PROGRESS.md updates. **No production code.**

## Goal

After this gate: every unknown that Phases 1–5 would otherwise design around blindly has a written,
evidenced answer — and the three questions that cannot be answered from the repo have been answered
on real hardware.

## Entry criteria (pre-flight — run these, don't assume them)

- [ ] **Baseline captured** — full CkTests suite via the toolbox (`/build-test`), recording
      pass/fail counts AND the names of any already-failing tests, into PROGRESS.md with the HEAD
      hash. "No regressions" is meaningless without this.
- [ ] Working tree clean, or every dirty path enumerated as "left untouched for its owning session".
- [ ] `PROMPT.md` "Things ruled out" table re-read — do not re-litigate those.
- [ ] Confirm engine on disk is still 5.7.x (`Engine/Build/Build.version`); the original design
      said 5.6 and the superproject CLAUDE.md says 5.5.

## Work items

### 0A — Hardware spike (NEW INFRASTRUCTURE — unknown; do this FIRST)

The only items in this campaign that cannot be answered by reading. A throwaway
`IInputProcessor`, logging to a file, exercised on a real gamepad and keyboard. Delete it at exit.

Answer, with logged evidence:
1. Does Slate deliver **raw analog axis** events, or only post-`FSlateApplication` processing
   (deadzone / repeat)? At what cadence relative to render frames?
2. Is `FInputEvent::GetUserIndex()` reliable for keyboard vs gamepad? What does a keyboard report
   when a gamepad is also connected?
3. Is **sub-frame arrival order** recoverable within one render frame for two near-simultaneous
   presses — and does it survive at both high and low framerates?
4. Behaviour in PIE with multiple windows, and with the editor focused vs the viewport focused.

→ **verify:** a log file showing, for a scripted physical input sequence, per-event
`(FKey, UserIndex, timestamp, analog value)` — and an explicit written verdict per question.
`[EDITOR-VERIFY: run the spike build, perform the scripted sequence on a DualSense + keyboard, attach the log]`

### 0B — Preprocessor coexistence policy

Three `IInputProcessor`s already ship: `CkLoadingScreen_Subsystem` (consumes ALL input while active,
priority 0, `:81-89`/`:658`), `CkDebuggerModel_ViewportPicker` (`:154`), `CkDebugOverlay_Subsystem`.

→ **verify:** a written priority policy naming CkInput's registration index relative to all three,
and the documented behaviour when the loading screen is up.

**Focus gating (C7 — this need has no other owner).** "Capture policy" was deleted by D14, but the
underlying problem survived it: a preprocessor sees the *editor's* typing in PIE. That gating is now
a property of the Slate writer (D25a), decided in Phase 1, default `GameViewportFocusOnly`. It is
**not** the same thing as layer consumption — layers gate gameplay delivery, focus gates whether the
writer records at all.

### 0C — Netcode stance, written down

D4 is already decided. This item records *how* it lands in code.

→ **verify:** a written statement of which consumer shapes are legal (OwningClientAuth SM, cue,
request at the replicated owner) and a grep-backed confirmation that no CkIntent surface will
require a client→server transport. Cross-check against `ck-game-replication-patterns`.

### 0D — Local player model (O2 + O3)

Neither review round found existing machinery associating a local player with an entity.

→ **verify:** either a named existing mechanism with file:line, or an explicit "none exists" plus a
proposed shape covering split-screen, keyboard+gamepad-same-player, possession change, and pawn
respawn.

### 0E — AngelScript exposure plan (non-negotiable #4)

The original design never mentioned AS. It constrains every phase.

→ **verify:** a written API-shape plan — the `utils_intent` namespace surface, how intent
definitions are authored as AS assets given the notation decision (D9), delegate-last ordering, no
UFUNCTION overloads, and which phases owe AS verification. Read `Script/CLAUDE.md` first.

### 0F — CkGameSettings boundary settlement (O6)

→ **verify:** written confirmation from that campaign's owner that CkIntent deriving its ButtonId
map from EI resolved player mappings, and re-deriving on `CkKeyBinding_Subsystem::OnSettingsChanged`,
introduces no conflict with the keybinding page.

### 0G — Read the neighbours (non-negotiable #1)

Work the `PROMPT.md` reading list. The research output must name, in one sentence, **the module this
campaign mimics** for each of: the feature quartet, the processor tick shape, the signal shape, and
the debugger widget.

### 0H — Confirm the fixed-timestep gap

`TickRate` + `ReplayMissedTicks` exist. The clamp does not.

→ **verify:** read `CkProcessor.h`'s replay loop and state precisely what a 1-second hitch does
today, then specify the clamp trait's shape (name, default, drop-vs-replay semantics, logging).

### 0I — Intent tag namespace, and the boundary of what counts as an intent (D13)

Two questions the design never asked, both surfaced by the CkInteraction study.

1. **Namespace.** D13 makes a shared `Intent.*` gameplay-tag vocabulary the consolidation
   mechanism. But the existing downstream tags are named for the carrier D5 removes — e.g.
   `GameplayTags::ByteAttribute_Intent_Interact_Primary` (`BB_NpcAI_Combat_Feature.as:537`). Once
   intents stop being ByteAttributes that name is a misnomer, and resolver mapping keys point at it.
2. **Boundary.** Continuous axes currently ride VectorAttributes downstream
   (`VectorAttribute_Intent_LookDirection` / `MoveDirection`). CkIntent's frame record carries a
   discrete SOCD-cleaned direction, but the campaign has **no stance** on whether continuous
   look/move belongs to CkIntent at all.

→ **verify:** a written decision on (1) the canonical namespace shape and whether a downstream
rename is proposed or the misnomer is tolerated, and (2) an explicit in-scope / out-of-scope call on
continuous look/move axes. Out-of-scope is a perfectly good answer — but it must be *chosen*, not
left to omission, because "intent" already means both things in shipped code.

## Expected observations at the gate — and what to do on each branch

| I will run | I expect to observe | If instead I see | Prewritten response |
|---|---|---|---|
| 0A spike, two near-simultaneous presses | Distinguishable arrival order in the event log | Identical timestamps / unordered delivery | Order-sensitive chords degrade to `Simultaneous`; write it into the grammar spec as a supported outcome, not a bug. Tell the maintainer the `X+Y` vs `Y+X` case is platform-limited. |
| 0A spike, gamepad stick sweep | Analog samples at or above render cadence, raw values | Pre-deadzoned or coalesced values only | CkIntent's own deadzone/hysteresis operates on already-processed input — document the precision loss and re-check whether the octant mapping still needs hysteresis. |
| 0A spike, keyboard + gamepad connected | Distinct `UserIndex` per device, or a documented keyboard convention | Both report 0 with no way to separate | Routing cannot key on `UserIndex` alone; propose device-class + `UserIndex` composite and flag split-screen as needing a maintainer decision. |
| 0B policy draft | A priority index that leaves CkIntent reachable whenever gameplay input is legal | No index satisfies all three existing preprocessors | Escalate as "[A] register above the debugger overlay and accept debugger input bleed vs [B] add a shared arbitration point in CkDebuggerCommon" — maintainer picks. |
| 0D search | An existing local-player↔entity mechanism | Nothing exists | Do NOT invent policy silently (non-negotiable #6). Propose one shape, mark it a fork, ask. |

## Exit criteria — ALL items land in the SAME commit as the last work item

- [ ] `PHASE_0_RESEARCH.md` written, every question above answered with **VERIFIED (file:line or log
      line) / INFERRED (+ what would confirm) / UNKNOWN (+ the spike that would settle it)** labels
- [ ] Every expected observation above confirmed or its prewritten branch taken and recorded
- [ ] Baseline counts + HEAD hash recorded in PROGRESS.md
- [ ] Every open item in PROGRESS.md (the list has grown well past O6 — check the file, not this
      number) either resolved or updated with a concrete next step
- [ ] Spike code **deleted** — confirm with a diff that nothing from 0A remains
- [ ] `[EDITOR-VERIFY]` steps for 0A written out as exact manual instructions with their results
- [ ] This file's Status header updated; PROGRESS.md dated entry appended — same commit
- [ ] **Maintainer review held.** Phase 1 does not start without it.

## Explicitly NOT in this phase

- No `CkIntent` module created, no `.Build.cs`, no fragment, no processor.
- No production code of any kind. The 0A spike is throwaway and must be deleted at exit.
- (D1 is closed — `CkInput` + `CkIntent`, two modules. This section previously said the name was
  still open; that was stale.)
