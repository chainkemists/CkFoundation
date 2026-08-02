# Phase 2 — Continuous cadence + lifecycle layering + curl noise → the Loops and Casts

**Goal:** land C2/C5/C10 so continuous-rate and windowed-sub-loop systems become expressible,
then port the ten Loop/Cast effects (behaviors 26–35 in the batch order below).

**Entry criteria:** Phase 1 closed (PROGRESS 2026-08-02: Particles 16/16, 11 templates @41,
10 pairs); baseline = those counts.

## Capability contracts (decisions MADE — executors fill bodies)

### C2 — spawn-rate rows
`FCk_ParticlesTemplateSpec` gains `float SpawnRate = 0.0f` (particles/sec; fractional fine).
When > 0 the builder emits a spawn-rate stack (mirror `Add_BurstEmitterStack`'s structure with
the SpawnRate module) — a row may carry burst, rate, or BOTH (C5's burst+rate emitters).
Template loop/lifetime semantics unchanged. Rate values come from each sheet's reconciled §6
(`[P0-D4]` numbers: e.g. HealLoop 32.5/s @ 1.0/2.0, DebuffLoop 36/s, BuffLoop, PickupLoop).

### C5 — spawn-phase input + windowed layers
The DI gains ONE new input: `EmitterAge` (read `Emitter.Age` in the template's Map Get; CPU
mirror receives it through the stage context the same way DeltaTime does). Behaviors derive
`SpawnPhase = fmod(EmitterAge - Age, LoopDuration)` (LoopDuration as a behavior-local constexpr
from the row) → burst particles sit at phase ≈ 0, rate particles spread; `Self/Once` emitter
windows become age/phase gates (a layer is hidden outside [delay, delay+window]). This is a DI
SIGNATURE change (8th input): update GetFunctionsInternal, the GPU template wrapper, VM plumbing,
the .ush StageInput struct, and the CPU mirror IN LOCKSTEP; RebuildTemplates is then mandatory
(cached call node carries the old signature). Layer selection on rate emitters = weighted pick by
rate share via `CkParticles_Rand(Seed, 0)` (the marketplace-recreation idiom).

### C10 — deterministic curl noise
`CkParticles_CurlNoise(float3 p, float freq, uint seed) -> float3` in Common.ush + exact CPU
mirror: finite-difference curl over the existing tileable Fbm (fixed eps = 0.01 — identical
constant both sides; do NOT use analytic gradients). Curl-driven motion stays STATELESS: position
= spawn + fixed-16-step Euler accumulation over [0, Age] (step = Age/16), identical loop both
sides. Cost is bounded and lockstep-safe; document the step count as a §13 fidelity note wherever
used.

## Ports (sheet §6 + per-port checklist; ids in this order)

- **Batch C (26–29):** PickupLoop, HealLoop, BuffLoop, DebuffLoop — the four rate-only systems.
  BuffLoop's vortex force uses C10's curl (or a simpler analytic swirl if its sheet's §5 says the
  source force is a plain vortex — follow the sheet).
- **Batch D (30–32):** PickupCast, HealCast, DebuffCast — DebuffCast needs the SlashClaw
  procedural mesh (measured profile in its sheet §3) and C10 curl; HealCast has the
  velocity-aligned sub-UV case.
- **Batch E (33–35):** Gunshot_Cast, FireBall_Cast, Lightning_Cast — Self/Once sub-loop windows
  via C5; FireBall_Cast row = 2.0/2.05/50 per [P0-D5].

Rainbow-consuming looks keep shipping against LutWhite until [P1-D1] is ruled.

## Exit criteria
Capability gates green on unchanged counts (C2/C5/C10 land before any port); then per batch:
lanes green (Particles 16→19→22→25), templates non-inert, pairs staged (10→14→17→20); recipes
§7–14 filled; PROGRESS current. Phase closes when all ten ports are gated.

## Fences
- No ribbon/light/mesh-facing work (Phases 3–4 own C6/C7/C8/C11).
- The DI signature change lands ONCE in the capability unit — ports must not touch the DI
  input set again.
- DissolveAdd semantics, behaviors 0–25, existing looks' parameters: untouched.
- Anything unenumerated → STOP into PROGRESS Blockers.
