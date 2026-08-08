# Phase 1 — CkInput raw layer: event fragment, device ownership, layer stack, thin writer

> **Status:** ✅ Code-complete, gate green (2026-08-08, same session) — full suite `1019/1017/2`
> delta-zero (7 new tests: 1 InputSource + 6 InputLayer, all named-verified). All four units +
> the [P1-D5] mouse-move follow-up landed; `Claude.md` extended; 0A instrument S1-S7 ready.
> **Open:** BP-side exercise of the new surface (visible-by-construction via BPFL, not yet
> exercised — flagged, not silently claimed); follow-ups (a)-(c) in PROGRESS's 1-4 entry;
> `[EDITOR-VERIFY]`/0A human steps; commit (withheld).
> **Depends on:** Phase 1a (✅ gate green, `1012/1010/2`). **Does NOT wait for the 0A spike:**
> PHASE_0.md's own expected-observations table pre-writes a graceful response for every possible
> 0A outcome (ordering degrades to `Simultaneous` per D21; deadzone operates on processed input,
> documented; `UserIndex` routing is composite per D22) — 0A refines docs and Phase 3/4 tuning,
> it cannot change this phase's structures. The Phase 1 thin writer + event dump IS the
> recommended 0A instrument (supersedes the throwaway probe, pending maintainer agreement).
> **Scope of record:** PROMPT.md phase-index row 1. Decisions referenced live in PROMPT.md's
> table and DESIGN_InputLayering.md — this file does not restate them.
> **Baseline for this phase:** `1012 / 1010 / 2` with the two known `PathNetworkFollower` names.

## Rulings made at phase open (recorded in PROGRESS.md decision log)

- **[P1-D1]** The Slate `IInputProcessor` registers at index 0 and is **observe-only** — it never
  returns handled/eats input, so it coexists with the two existing index-0 preprocessors
  (loading screen, viewport picker; their mutual collision remains the debugger owner's,
  flagged not fixed). All consumption happens at ECS routing (D14/D25b), never in Slate.
- **[P1-D2]** The local-player ↔ entity seam (open item O2) is a minimal
  `ULocalPlayerSubsystem` that owns the per-local-player **input-source entity** handle and
  creates it once a PlayerController exists (the 1a lesson: EI settings and PCs do not exist at
  subsystem Initialize — bind/create from `PlayerControllerChanged` with a lazy backstop).
  Engine-lifecycle seams are exactly where D25 permits subsystems. Revisit only if the
  maintainer wants a general LocalPlayer↔entity mechanism instead — this one stays CkInput-local.

## Work units (dispatch order; gate between waves)

### 1-1 Raw-event fragment + device ownership + processor groups (FOUNDATION — alone in wave 1)
Feature-quartet shape (mimic `CkTimer`), in `Source/CkInput/`:
- Input-source entity per local player via the [P1-D2] subsystem seam.
- `ck::FFragment_InputSource_*`: params + current (raw event rows appended by writers, drained by
  the router); event row carries device class, per-device-class capability provenance (D21),
  key/button, press/release/axis payload (axes recorded per D18, no grammar).
- Explicit device→local-player ownership requests/queries (D22) — never inferred from
  `UserIndex` alone.
- **Named processor groups** for cross-module ordering (CkInput routing before future CkIntent
  matching — same-group registration order is NOT safe; the groups are the contract).
→ **verify:** builds green; a unit AutoTest creates the source entity synthetically, appends a
synthetic event row, reads it back. Invalid-input test per non-negotiable #3 (e.g. assigning a
device to an invalid player rejects atomically, no partial state).

### 1-2 Layer stack + declarative captures + router (heart of the phase)
Per DESIGN_InputLayering.md "The port", with [P1A-D3] explicit `_Priority` ints:
- One stable layer fragment holding a `TArray` of captures (never per-capture fragments —
  tombstone pools).
- Registration-time priority **collision detection** (ensure + reject, not silent tie-break).
- Routing processor: top-down, first matching `Consume` capture wins; `PassThrough` acts and
  continues; capture edits are **deferred requests** (one-frame transition contract — TESTED).
- **Press→release ownership in the router**: a layer that consumed a press receives the matching
  release even if its captures changed or the layer popped (router holds per-key consumed-by
  state; layers stay declarative).
- Bottom-of-stack **global action** surface (D16).
→ **verify:** AutoTests via the synthetic writer: consume masks lower layers; passthrough does
not; global action fires only when unmasked; press→release pairing survives a mid-hold layer
pop; a capture edit lands next frame (the one-frame contract); priority collision rejects loudly.

### 1-3 Slate thin writer (parallel with 1-2 — different files)
- `IInputProcessor` per [P1-D1]: registered index 0, observe-only, PIE-focus-gated, writes raw
  event rows (keyboard, mouse buttons, gamepad buttons, axes) into the owning local player's
  source fragment (D22 routing).
- An event-dump surface (console-toggleable verbose log of arriving rows with device/provenance)
  — this is the 0A instrument for the human.
→ **verify:** compiles + suite green headlessly (real-device behavior is `[EDITOR-VERIFY]` /
the 0A session). The dump's output format is written into the 0A steps.

### 1-4 `CkInput/Claude.md` extension + PROGRESS wrap
Document the raw layer: entity topology, fragment contract, groups, layer stack, writer,
capabilities. No campaign breadcrumbs.

## Expected observations at the gate

| Run | Expect | If instead | Response |
|---|---|---|---|
| Full suite after each wave | delta-zero vs `1012/1010/2` + new tests | new failing names | Stop the wave; root-cause before stacking (restore known-good first if our change regressed) |
| Capture-edit deferral test | edit lands frame N+1 | lands frame N | The deferred-request contract is broken somewhere — this is load-bearing for O13's closure; STOP |
| Priority collision test | loud ensure + reject | silent last-wins | Defect in 1-2; fix before 1-4 |

## Exit criteria

- [ ] Full suite green + delta-zero vs `1012/1010/2` (plus this phase's new test names)
- [ ] Layer routing, press→release pairing, one-frame capture transition, collision rejection,
      device ownership — each pinned by a named AutoTest
- [ ] Public surface exercised from C++ (tests), AS (autotests), BP-visible (BPFL) — non-negotiable #4
- [ ] `CkInput/Claude.md` extended; PROGRESS.md decision log + dated entry current
- [ ] 0A instrument documented as exact `[EDITOR-VERIFY]` steps for the human
- [ ] Comment audit run over the phase diff

## Explicitly NOT in this phase

- No biasing stage (1b), no ButtonId/EI map derivation (2), no sampler/ring/octants (3), no
  grammar/matcher (4-5), no CkIntent module at all.
- No changes to the keybinding/glyph surface beyond the 1a fix already landed.
- No replication of anything (D4).
