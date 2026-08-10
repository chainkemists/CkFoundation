# PHASE_10 — EDITOR-VERIFY drive script (v2, supersedes PHASE_9_EDITOR_VERIFY.md)

> **Freshness:** written 2026-08-10 at slice-5 close, against CkTests `96bed75f` + the slice-5
> prune. **Death condition:** superseded if the arena's move table or state machine changes
> after Phase 10 closes — re-derive from the pawn, not from this file.
> **Time budget:** ~10 minutes. Every check is a LOOK, not a measurement.

Launch: PIE in `TestGyms_CkTests_Level` → Tab → **Input Playground**.

## 1. Controls (slice 1 — previously signed off, regression pass)

- [ ] Camera is fixed top-down; the mouse NEVER moves or rotates it.
- [ ] The character faces the cursor continuously; WASD moves in SCREEN space (W = up the
      screen), walking backwards while facing the cursor works.
- [ ] Movement speed reads deliberate, not skatey (MaxSpeed 600).

## 2. Chain feel (slice 3b — the rework under verdict)

- [ ] **Deliberate clicking chains.** Click LMB three times at a natural attack rhythm —
      1-2-3 chains without mashing. (The buffer answers the PRESS; the old build graded
      releases and could not do this.)
- [ ] A brief wind-up is visible at each step's start (20% of the step); the swing shape
      appears where the wind-up ends.
- [ ] A slightly-LATE third click still continues the chain (10-frame grace after an
      unbuffered expiry) rather than restarting at 1.
- [ ] Taps feel crisp: the tap fires on release with at most a ~83ms verdict wait
      (`hold=5`), not the old 45-frame sluggishness.
- [ ] RMB mirrors all of the above, heavier and slower.

## 3. Charges / specials

- [ ] Hold LMB: the charge sphere grows and saturates over ~0.75s (display ripeness, 45f);
      release at ANY point past ~83ms fires the light special (cyan burst, bigger than
      chain swings). RMB mirrors (orange).
- [ ] Starting a hold mid-chain cancels the pending strike free (mid-wind-up charge
      cancels; no phantom swing).

## 4. Enemy + block (slice 3)

- [ ] Swings that reach the dummy tint it and increment the floor HITS counter.
- [ ] Inside 1600cm the dummy telegraphs (~3s cadence) and shoots; standing still unblocked
      = red beat at the body; holding Q = cyan beat at the block plate riding the cursor aim.

## 5. Combos (slice 4 — first PIE ever)

- [ ] **Tap LMB then tap RMB** within ~0.5s → floor label `COMBO L-H`, violet swing at
      extent 170 (bigger than the heavy special).
- [ ] **Tap RMB then tap LMB** → `COMBO H-L`, magenta swing. The two orders are DIFFERENT
      moves — confirm the labels differ.
- [ ] **Hold W (walk forward) and press LMB** → `COMBO W+L`, spring-green swing,
      INSTANTLY on the press (no verdict wait — the held W satisfies the chord on the
      press row). Walking is unaffected before, during, and after.
- [ ] A combo mid-chain supersedes the chain (no double-answer: the chain does not also
      advance on the same presses).
- [ ] Known and accepted: a charge landing the same tick stomps a combo; the dummy tints a
      combo hit with the heavy-special tint (pawn-side hue answers which combo).

## 6. Failure signatures (what broken looks like)

| You see | It means |
|---|---|
| Floor label `SET REJECTED` latched | Parse/Bake/swap rejection — the one surface autotests could not prove; read the `[Playground/Kit]` Print for the reason |
| `L p-1` on the input line while clicking | LMB not arriving through the Slate source (regression of a PROVEN path) |
| Editor-boot fail naming `CkWorldSpaceWidget` | Stale binaries — rebuild (`--build --test`) |
| Combos never fire but taps work | W not minted / swap-gate wait stuck — Diagnostics exec, check three minted keys |

## 7. Debugger cross-check (optional, 2 min)

- [ ] CkIntentDebugger timeline: `Kit_Combo_*` completions appear as spans/markers beside
      the `Kit_Light_*`/`Kit_Heavy_*` traffic; an L press with W held completes
      `Kit_Combo_WL` on the press row (no deferral span).
- [ ] Known deferred, do NOT re-flag: timeline scrub UX + the `s` suffix on logic-frame
      axis ticks ([P10-F1], debugger-qol campaign's).

## Open items on maintainer return

- PIE feel verdict on slice 3b (section 2) and slice 4 (section 5) — the AFK mandate waived
  per-slice PIE; these two sections are the accumulated debt.
- Gamepad parity was "if wanted" — not built; say the word and it becomes a new slice.
- `CkCameraGym_Pawn.as:89` still carries the pre-`34d89aa91` camera Add (latent PIE ensure,
  out of campaign scope, flagged since Phase 9).
