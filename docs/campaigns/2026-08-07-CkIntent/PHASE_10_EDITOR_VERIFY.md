# PHASE_10 — EDITOR-VERIFY drive script (v2, supersedes PHASE_9_EDITOR_VERIFY.md)

> **Freshness:** written 2026-08-10 at slice-5 close; §2/§3 re-tuned same day by slice 6
> (hold verdict 5→10 frames — [P10-D11]); §2/§4/§5 re-tuned same day by slice 8 (hitboxes,
> parry deflect, sprint attacks — [P10-D14..D16]). **Death condition:**
> superseded if the arena's move table or state machine changes after Phase 10 closes —
> re-derive from the pawn, not from this file.
> **Time budget:** ~10 minutes. Every check is a LOOK, not a measurement.

Launch: PIE in `TestGyms_CkTests_Level` → Tab → **Input Playground**.

## 1. Controls (slice 1 — previously signed off, regression pass)

- [ ] Camera is fixed top-down; the mouse NEVER moves or rotates it.
- [ ] The character faces the cursor continuously; WASD moves in SCREEN space (W = up the
      screen), walking backwards while facing the cursor works.
- [ ] Movement speed reads deliberate, not skatey (walk 600); holding SHIFT sprints (1000) —
      visibly faster, back to a walk the frame Shift lifts.

## 2. Chain feel (slice 3b — the rework under verdict)

- [ ] **Deliberate clicking chains.** Click LMB three times at a natural attack rhythm —
      1-2-3 chains without mashing. Steps are LONG now (light 2s, heavy 4s each — the step
      is the buffering window), so the pace is slow and deliberate by design.
- [ ] **A fast double-click chains.** The second press lands during step 1's wind-up and
      still buffers step 2 (the slice-7 fix: the chain window opens at step ENTRY, not at
      wind-up end — the old gate swallowed exactly this press).
- [ ] A wind-up is visible at each step's start (20% of the step — 0.4s light / 0.8s heavy);
      the swing shape appears where the wind-up ends.
- [ ] A slightly-LATE next click after a step expires still continues the chain (10-frame
      grace) rather than restarting at 1.
- [ ] **While a next attack is queued the state label reads `LIGHT 2 + BUFFERED` (amber) —
      the attack name never disappears.** The small dim sphere over the head is the
      buffered-attack MARKER and shows at the same time; both clear when the queued step
      starts.
- [ ] **Swings connect (slice-8 fix).** Walk up to the dummy and swing: the dummy flashes
      RED (~0.4s) and the floor HITS counter increments. Then swing from just out of reach
      and WALK IN while the shape is still up — it still connects (the drawn shape IS the
      hitbox for its whole lifetime, not just its spawn tick).
- [ ] Taps feel crisp: the tap fires on its release; only a press outliving ~166ms
      (`hold=10`) is a hold.
- [ ] **A lazy single click yields a LIGHT attack, never a special.** (The slice-6
      regression under verdict: at `hold=5`, ordinary ~90-150ms clicks graded as holds and
      one press came out as a special.)
- [ ] RMB mirrors all of the above, heavier and slower.

## 3. Charges / specials

- [ ] Hold LMB: the charge sphere grows and saturates over ~0.75s (display ripeness, 45f);
      release at ANY point past ~166ms fires the light special (cyan burst, bigger than
      chain swings). RMB mirrors (orange).
- [ ] Starting a hold mid-chain cancels the pending strike free (mid-wind-up charge
      cancels; no phantom swing).

## 4. Enemy + block (slice 3, reworked by slice 8)

- [ ] Swings that reach the dummy flash it RED and increment the floor HITS counter (red is
      the only hit tint now — which move landed is the swing's own colour on the way in).
- [ ] Inside 1600cm the dummy telegraphs (~3s cadence) and shoots; standing still unblocked
      = red beat at the body; holding Q = cyan beat at the block plate riding the cursor aim.
      **The red sphere at YOUR body is this** — an unblocked hit, not attack feedback (the
      input beat is pale-green when acted on, gray when ignored, never red).
- [ ] **PARRY: press Q just as the shot lands** (within ~133ms / 8 frames before impact) →
      GOLD beat at the plate, `[Playground] PARRY` print, **and the projectile turns GOLD
      and flies back** — when it reaches the dummy, the dummy flashes red and HITS
      increments. The dummy does not fire again while its own returned shot is in flight.
- [ ] A Q held from well before the shot stays an ordinary cyan BLOCK and the shot ends
      there — the parry is the read, not the wall. The press must feel INSTANT: Q carries
      no grammar verdict at all.

## 5. Combos (slice 4 — first PIE ever)

- [ ] **Tap LMB then tap RMB** within ~0.5s → floor label `COMBO L-H`, violet swing at
      extent 170 (bigger than the heavy special).
- [ ] **Tap RMB then tap LMB** → `COMBO H-L`, magenta swing. The two orders are DIFFERENT
      moves — confirm the labels differ.
- [ ] **Sprint (hold W + SHIFT) and press LMB** → `SPRINT ATTACK L`, a spring-green RING
      around the pawn (~300cm), INSTANTLY on the press (both held partners satisfy the
      chord on the press row). RMB mirrors → `SPRINT ATTACK H`, deeper green. Sprinting is
      unaffected before, during, and after.
- [ ] **Standing still (or walking without Shift), LMB/RMB never produce a sprint attack** —
      the chord needs both locomotion keys down, so "only while running" is the move's own
      shape.
- [ ] A sprint attack near the dummy flashes it red and increments HITS (the ring is a
      radial hitbox).
- [ ] A combo mid-chain supersedes the chain (no double-answer: the chain does not also
      advance on the same presses).
- [ ] Known and accepted: a charge landing the same tick stomps a combo.

## 6. Failure signatures (what broken looks like)

| You see | It means |
|---|---|
| Floor label `SET REJECTED` latched | Parse/Bake/swap rejection — the one surface autotests could not prove; read the `[Playground/Kit]` Print for the reason |
| `L p-1` on the input line while clicking | LMB not arriving through the Slate source (regression of a PROVEN path) |
| Editor-boot fail naming `CkWorldSpaceWidget` | Stale binaries — rebuild (`--build --test`) |
| Combos never fire but taps work | W or LeftShift not minted / swap-gate wait stuck — Diagnostics exec, check four minted keys |

## 7. Debugger cross-check (optional, 2 min)

- [ ] CkIntentDebugger timeline: `Kit_Combo_*` completions appear as spans/markers beside
      the `Kit_Light_*`/`Kit_Heavy_*` traffic; an L press with W and Shift held completes
      `Kit_Combo_WL` on the press row (no deferral span).
- [ ] Known deferred, do NOT re-flag: timeline scrub UX + the `s` suffix on logic-frame
      axis ticks ([P10-F1], debugger-qol campaign's).

## Open items on maintainer return

- PIE feel verdict on slice 3b (section 2) and slice 4 (section 5) — the AFK mandate waived
  per-slice PIE; these two sections are the accumulated debt.
- Gamepad parity was "if wanted" — not built; say the word and it becomes a new slice.
- `CkCameraGym_Pawn.as:89` still carries the pre-`34d89aa91` camera Add (latent PIE ensure,
  out of campaign scope, flagged since Phase 9).
