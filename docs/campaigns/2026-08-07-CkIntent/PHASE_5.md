# Phase 5 — matcher, deferral, hold accumulators, arbiter, poll/claim API

> **Status:** ✅ CLOSED (2026-08-09, same session) — unit 5-1 gated 108/108 (after two
> inline AS fixes: Request_* struct arity, adjacent literals), unit 5-2 gated 113/113
> first-try. Success criteria 1, 2, 3, 4 all PROVEN HEADLESS. Comment audit clean.
> Full-suite delta-zero deferred per [P2-D4]. **Depends on:** Phase 3 (record + delivery
> retention), Phase 4 (compiled sets + verdicts). **Gate regime:** scoped per [P2-D4].
> **Scope of record:** PROMPT.md phase-index row 5. Governing: D6 (poll primary), D7 (the
> latency law), D8 (O(1) set swap), D11 (accumulators ≠ matcher), D15-revised, D25b.
> **`DESIGN_PollSurface.md` was adversarially reviewed at phase open and its analysis
> ACCEPTED** — including the reframe that Shape B's rows were already mandated by D15 and are
> now partially BUILT ([P3-D4]/[P3-D6]: per-event delivery outcomes in the frame record).
> The rulings below answer every question the dossier said must be answered in one sitting
> (§3a, α/β/γ, §7's claim rules, §2's naming) — recorded here and in PROGRESS.

## Rulings at phase open (the poll-surface ruling batch)

- **[P5-D1] A LAYER IS THE COMPILED-SET ANCHOR (the dossier's §3a, answered by design).**
  The matcher is a feature composed on an INPUT-LAYER entity (opt-in — `IntentMatcher`),
  carrying the active `FCk_Intent_CompiledSet` (value data; swapping it is one deferred
  request = D8's O(1) swap) plus matcher state. The vehicle case IS D8's motivating case:
  entering the vehicle pushes a layer that carries the vehicle's set. A local player with one
  move set = one set-carrying layer; layers without a matcher just mask, as today.
- **[P5-D2] α/β/γ = γ.** Matching runs per set-carrying layer, over the events VISIBLE to
  that layer. Visibility is derived from the recorded per-event outcome ([P3-D4] rows):
  an event is visible to layer L iff nothing ABOVE L consumed it (outcome is
  no-consumer, or consumer priority ≤ L's). α's "matched with empty delivery set" is
  unrepresentable (masked events never reach the layer's matcher); β's record-the-
  suppression requirement is already satisfied by the routed-event rows (the debugger shows
  the modal consuming the press — the WHY of a non-match).
- **[P5-D3] Shape C is the primary read API, WITHOUT minting intent entities in v1.**
  No API returns intent state without the layer in the lookup path (the doctrine line goes
  in `CkIntent/Claude.md`, greppable — C-F1's pin). V1 poll = tag-keyed reads on the
  matcher layer handle: `Get_IntentPhase(FCk_Handle_IntentMatcher, FGameplayTag)` etc.
  Intent state = persistent rows per definition in ONE stable fragment (the capture
  precedent; kills C-F3's churn concern by never spawning per-activation entities).
  Phase 6 decides whether signals force per-intent HANDLES; nothing in v1 precludes it.
  The dossier's C-F5 (layer resolution for consumers) is deferred with it — v1 consumers
  hold the matcher-layer handle they composed/were given, exactly like every other feature
  handle in this codebase.
- **[P5-D4] Claiming:** the gameplay claim is an **immediate mutator** — declared here as
  the house "Immediate mutators" escape hatch, with the dossier's reason (a deferred claim
  makes same-frame double-claim a race; two pollers must observe the first claim
  synchronously). Per-layer by construction under γ (each matcher's state is its own —
  `PassThrough` stays honest for free). Claim-through-a-mask is unrepresentable (an
  undelivered layer's matcher never completed the intent). NO down-stack claim — that
  request is `handled == true` reborn; the mechanism for "block below me" remains a capture
  edit. Same-layer claim order = processor order; ACCEPTED and documented (intents needing
  single-consumer semantics live behind one consumer).
- **[P5-D5] Naming = the dossier's N1.** The capture behavior keeps `Consume` (routing-time
  masking, ftxc parity, shipped). The gameplay verb is **`Request_Claim`** — claiming, not
  consuming; the two mechanisms stop sharing a word. D6's "poll/consume" prose is satisfied
  by claim semantics.
- **[P5-D6] D15 transition policies ship as the DEFAULT PAIR ONLY in v1** (`Cancel` on
  delivery loss, `RequireRePress` on gain — the conservative, loud pair). The policy enums
  exist on the definition/compiled surfaces but only defaults are constructible until a
  consumer demands more ([P1B-D1] minimalism precedent). The notation does not grow policy
  modifiers yet.
- **[P5-D7] Capture registration follows the SET, and rebinds follow the MAP.** Activating
  a set on a matcher layer registers `Key` captures for the set's terminal ButtonIds,
  resolved to current physical FKeys through the source's `InputButtonMap`; behavior
  (Consume vs PassThrough) is a per-activation parameter (default Consume). On the map's
  re-derive (`OnSettingsChanged` → [P2-D1] seam), the matcher re-resolves and edits its
  captures — THIS is success criterion 4's wiring (rebind moves the physical key with no
  definition edit), and it closes anti-pattern #13 for intent-driven captures specifically.

## Units (sequential)

**5-1 — matcher core:** `IntentMatcher` feature on the layer ([P5-D1]); `Request_SwapSet`
(deferred, atomic — a set with buttons the source's map cannot resolve rejects whole);
capture registration + rebind re-resolution ([P5-D7]); the D7 backward scan on terminal
delivery (sequences + chords over the source's ring: octant steps consult recorded octants,
button steps consult recorded edges + visibility; window `w=`, lenience per flag); the
arbiter (per-terminal resolution table order = priority; first full match wins the frame);
per-definition phase rows (`Idle / Completed(frame)` v1 — no deferral phases yet); poll API
(`Get_IntentPhase`, `TryGet_CompletionFrame`, `Get_ActiveSetName`); AutoTests: success
criterion 1 shaped (inject 2,3,6+punch synthetically → `Completed` ON the press frame —
assert frame indices equal), suffix-no-defer (bare punch completes same frame even though a
236+punch shares the button), masked-events-never-match (Consume layer above → matcher
stays Idle, and the retention row names the masker), rebind-moves-the-match (remap the
button, re-derive, old key inert + new key completes — criterion 4 headless).

**5-2 — deferral + holds + claim:** deferral execution from the baked verdicts (chord
window: press held ambiguous until partner-or-timeout; hold sibling: until threshold-or-
release) with `Pending → Completed/Failed` phases added to the row; the hold accumulator
(D11: separate struct, physical fact from the record's held set, policy-applied per-layer
per [P5-D6]); `Request_Claim` ([P5-D4]/[P5-D5], immediate, per-definition-row claimed-by);
AutoTests: tap-vs-hold at threshold−1/at/+1 (success criterion 3, all three), chord window
partner/timeout both branches, claim excludes the second poller same-frame, claim on an
uncompleted intent rejects, delivery-loss mid-hold cancels (default policy) and the record
shows the physical hold continuing (fact vs policy).

## Exit criteria

- [ ] Scoped gates green: `Ck_AutoTest_In` (all rows) + `Ck.Intent.` C++ pattern if 5-x adds
      hermetic tests
- [ ] `CkIntent/Claude.md`: matcher/arbiter/claim contract + the [P5-D3] doctrine line +
      the [P5-D4] immediate-mutator declaration with its reason
- [ ] PROGRESS current; comment audit
- [ ] Full suite NOT run ([P2-D4])

## NOT in this phase

No intent-phase SIGNALS or decay lifecycle (6), no two-surface contract polish (6), no
per-intent entities unless Phase 6 forces them ([P5-D3]), no non-default D15 policies
([P5-D6]), no debugger lanes (7), no gyms (8), no octant-terminal (direction-driven)
triggers (recorded Phase-5 gap from 4-2 — STAYS a gap; button-terminal intents only until a
consumer demands otherwise; record it in Claude.md).
