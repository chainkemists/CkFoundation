# Phase 1b — the biasing stage (D20): per-game conditioning between raw and intent

> **Status:** ✅ Code-complete, gate green (2026-08-08, same session) — scoped 71/71, full suite
> `1026/1024/2` delta-zero (7 new `InputBias` tests named-verified). Conditioning contract
> documented in `CkInput/Claude.md`. **Open:** maintainer review of the `TryGet_AxisBias`
> sentinel-vs-exec-pins call ([P1B] entry in PROGRESS), `[EDITOR-VERIFY]` items, commit
> (withheld).
> **Depends on:** Phase 1 (✅ `1019/1017/2`). Does not need 0A or 0F.
> **Scope of record:** PROMPT.md phase-index row 1b + D20. The biasing params must be a
> **runtime-mutable fragment**, not settings-only (recorded follow-up from the 2026-08-08 review).
> **Baseline:** `1019 / 1017 / 2` with the two known `PathNetworkFollower` names.

## Rulings at phase open

- **[P1B-D1]** Bias v1 is **parametric**: per-axis-key deadzone [0..1), response exponent,
  sensitivity scalar, invert flag — no curve-asset references until a consumer demands them
  (minimum that satisfies D20; a `UCurveFloat` override is additive later).
- **[P1B-D2]** Biasing NEVER mutates the recorded raw rows (D15: recording is unconditional —
  physical facts stay physical). It maintains a separate **conditioned axis state** on the source
  entity: latest conditioned value per axis key, recomputed when axis rows route. Buttons are
  untouched by bias. Consumers (the future CkIntent sampler, gameplay reading axes today) read
  the conditioned state; the raw inbox/record keeps verbatim values.

## The unit (single dispatch)

**Feature `InputBias`** on the input-source entity (quartet, mimic the landed `InputSource`/
`InputLayer` shapes): ParamsData carrying a `TArray` of per-axis-key bias rows (key, deadzone,
exponent, sensitivity, invert) with request-driven runtime mutation (`Request_SetAxisBias`,
idempotent semantics per house Result rules); a conditioned-state fragment (axis key → last
conditioned value + last raw value); a conditioning processor ordered `Collect → Bias → Route`
(CORRECTED at execution: the router drains the inbox in Route, so "after Route" was
unimplementable — `FGroup_Input_Bias` sits between, reading the inbox before the drain; bonus:
capture handlers fired during Route read same-frame conditioned values) applying, in order:
inversion → deadzone (radial per-axis v1: values inside deadzone → 0, outside rescaled to [0,1])
→ exponent curve → sensitivity. Utils queries: `Get_ConditionedAxisValue`, `TryGet_AxisBias`.
Unbiased axes pass through verbatim (identity defaults).

→ **verify (AutoTests, synthetic injection):** identity default passthrough; deadzone zeroes
inside + rescales outside (threshold−ε, threshold, threshold+ε); inversion flips sign; exponent
+ sensitivity compose in documented order; runtime `Request_SetAxisBias` re-conditions on the
NEXT event (deferred-request contract); raw rows remain verbatim while conditioned state differs.

## Exit criteria

- [ ] Full suite green + delta-zero vs `1019/1017/2` + the new test names
- [ ] Conditioning order documented in `CkInput/Claude.md` (one added subsection) — the contract
      CkIntent consumes
- [ ] PROGRESS decision log + dated entry current; comment audit run

## NOT in this phase

No ButtonId map (Phase 2, 0F-gated), no sampler/ring (Phase 3), no octant/hysteresis mapping
(Phase 3 consumes conditioned axes there), no UCurveFloat assets, no settings-object plumbing
(runtime-mutable fragment only; per-game defaults can ride ParamsData at compose time).
