# Phase 6 — the intent surface: signals, decay lifecycle, two-surface contract

> **Status:** ✅ CLOSED (2026-08-09, same session) — gate 118/118 after one fix cycle
> (decay/settle signal payloads carried `INDEX_NONE`; root-caused, fixed, plus a latent
> render-rate coupling in three tests hardened). [P6-D4] ruled: the swap-reset's `→ Idle`
> keeps `INDEX_NONE` (tied to no row; documented exception). Full-suite delta-zero deferred
> per [P2-D4]. **Depends on:** Phase 5 (✅). **Gate regime:** scoped per [P2-D4].
> **Scope of record:** PROMPT.md phase-index row 6: "Intent fragment + signals, decay
> lifecycle, two-surface contract, AS + BP verification." Governing: D5 (fragment +
> dedicated signals, never ByteAttribute — the reasoning in `CkAttribute/CLAUDE.md`
> anti-pattern #3 is settled, do not relitigate), D6 (poll primary / signals presentation),
> D4 (client-local, nothing replicates).

## Rulings at phase open

- **[P6-D1] Signals live on the MATCHER entity; intent identity rides the payload. No
  per-intent entities — this CLOSES the question [P5-D3] deferred and retires the
  dossier's C-F5 permanently:** consumers never resolve layers; they bind on the matcher
  handle they already hold, and filter by intent name in the handler when they care about
  one intent. *Revisit only if a shipped consumer measurably suffers from handler-side
  filtering.*
- **[P6-D2] Decay:** a `Completed`/`Failed` latch decays to `Idle` after
  `_LatchDecayFrames` (ParamsData on the matcher, int32 logic frames, default 20 ≈ 333 ms —
  a generous input-buffer feel; reject <= 0), clearing any claim with it. `Pending` NEVER
  decays (episodes carry their own windows). Decay is the Match processor's job — the
  single-reader rule that kept 5-2 out of a second processor holds here too. Decay without
  this: a stale unclaimed completion is claimable seconds later, which is the detonation
  bug reborn at the claim level.
- **[P6-D3] Two signals, not one and not many:** `OnIntentPhaseChanged` (payload: intent
  name FName + tag, old phase, new phase, frame — the complete transition surface) and
  `OnIntentCompleted` (payload: name + tag + frame — the presentational 90% case, fired
  only on `* → Completed`). Both via `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE`, fired
  from the Match processor at transition time (including decay's `→ Idle` on PhaseChanged
  only). Signal-vs-poll law restated where the signals are declared: late binders under
  `FireIfPayloadInFlight*` receive only the LAST payload — sequences are reconstructed
  from the poll surface and the frame record, never from signal replay (D6's reason,
  stated at the API so nobody re-learns it).

## The unit (single dispatch — 6-1)

Signals declared beside the matcher (`CkIntentMatcher_Fragment_Data.h` additions are
additive-only; the gated 5-x surfaces do not change semantics); transition detection inside
the Match processor where phases already mutate (every write funnels through one helper so
a phase can never change without its signal — structural, not disciplinary); decay per
[P6-D2]; `BindTo_/UnbindFrom_` UFUNCTION surface per house macros.

→ **verify (AutoTests):** AS bind of `OnIntentCompleted` with the EXACT const-ref delegate
signature (the recorded AS trap — this test is partly ABOUT the binding path) fires on a
synthetic completion with the right name + frame; `OnIntentPhaseChanged` observes
`Idle→Pending→Completed→Idle(decay)` in order for a hold intent; decay returns the row to
`Idle` at exactly `_LatchDecayFrames` after completion and clears the claim
(`Get_IsClaimed` false, `TryGet_ClaimedBy` invalid); a late binder after a completion
still standing receives the last payload under the binding policy the unit selects
(document which and why); claim-then-decay-then-recomplete-then-claim proves the full
lifecycle loop.

## Exit criteria

- [ ] Scoped gate green (`Ck_AutoTest_In`, all prior rows + the new signal tests)
- [ ] `CkIntent/Claude.md`: the two-surface contract section (poll primary, signals
      presentation, the D6 law verbatim), decay semantics, signal payload reference
- [ ] PROGRESS current; comment audit
- [ ] Full suite NOT run ([P2-D4])

## NOT in this phase

No debugger (7), no gyms (8), no per-intent entities ([P6-D1]), no replication (D4), no
non-default D15 policies, no additional signal kinds (near-miss/deferral-started etc. —
Phase 7 may motivate them; record, don't build).
