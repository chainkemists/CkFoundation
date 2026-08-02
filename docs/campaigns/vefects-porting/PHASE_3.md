# Phase 3 — Ribbons → the four trail effects

**Goal:** land C6 (ribbon renderer + the stateless trail translation) and C11 (distance-driven
ribbons), then port FireBall_Projectile (**36**), Bomb_Projectile (**37**), BuffCast (**38**),
Lightning_Muzzle (**39**).

**Entry criteria:** Phase 2 closed (PROGRESS 2026-08-02: Particles 26/26, 21 templates, 20
pairs); baseline = those counts.

## Capability contracts (decisions MADE)

### C6a — Ribbon renderer kind
`ECk_ParticlesRenderer_Kind::Ribbon` + `LookName` on the spec; builder emits
`UNiagaraRibbonRendererProperties` with the look master (explicit `Material`), VisTag-gated like
every other kind. Ribbon linking uses Niagara's default age-based link order — our trail
particles are spawned in path order (below), so age order IS path order. Width rides `Size.x`.

### C6b — CkUsf ribbon usage flag
`_UsedWithNiagaraRibbons` opt-in on the look contract, mirroring the sprite/mesh flags exactly
(batch-E finding: the flags are separate; a shared material drawn on ribbon + sprite needs two
masters). Contract-test coverage in the NiagaraSpriteContract style.

### C6c — THE TRAIL TRANSLATION (the load-bearing design)
Source trails are event-spawned: a leader particle emits location events; ribbon particles spawn
there and persist. Our behaviors are stateless closed-form functions — so the events COLLAPSE
INTO MATH: a trail particle at spawn-phase p evaluates the LEADER's closed-form trajectory at
its own spawn time and HOLDS that position (or fades per its curves). Rate-spawned trail
particles in age order therefore trace the leader's path exactly. No runtime event machinery
exists or is needed; the sheets' `eventHandlers` corpus data supplies spawn rates and inherited
attributes. Record per port in §13 that trail density ≈ event cadence (exact when the source
event rate is the sheet's constant).

### C11 — distance-driven (Spawn-Per-Unit) trails
Per-unit spawn = arc-length parameterization of the leader path: N per loop with spawn times
solved so consecutive samples are equidistant ALONG the path (fixed-step accumulation, same
16-step idiom as CurlPath, identical both sides). Where the leader speed is constant the
closed form is exact; where not, the 16-step table is the documented approximation (§13).

## Ports (batches F then G; ids in order)

- **Batch F (36, 37):** FireBall_Projectile (10 s Self system: 400+ sprites/s trail + 2 ribbons
  + curl — the heaviest continuous system; its rate load is the first real stress of C2),
  Bomb_Projectile (2.5 s loop; Spawn-Per-Unit ribbon via C11; the toon bomb prop rides [C-D2]'s
  stand-in mesh + BombToon).
- **Batch G (38, 39):** BuffCast (event ribbon + 23-burst support cast; its `eventHandlers`
  corpus block is the first consumed in anger), Lightning_Muzzle (17 emitters: 2 ribbons
  (LightningArc pair), spike/plane meshes, sub-UV, curl — mesh facing beyond Default is Phase 4;
  where its sheet needs Velocity facing, orientation math in-behavior per the batch-B precedent
  or a §13 note).

Batch prompts carry the standing rules: self-diff audit, corpus-derived curve assertions
(tolerance < smallest key delta), recovered-parameter color keys, measure-before-reuse,
Rainbow→LutWhite pending [P1-D1].

## Exit criteria
Capability gates green on unchanged counts; then per batch: lanes green (Particles 26→28→30),
templates non-inert, pairs staged (20→22→24); recipes §7–14; PROGRESS current. Phase closes
with all four ports gated.

## Fences
- No light renderer / mesh-facing modes / palette table (Phase 4 owns C7/C8/palettes).
- DissolveAdd semantics, behaviors 0–35, existing looks: untouched.
- The optional exporter v3.1 (ribbon lifetimeResolved, [P0-D6]) is NOT required — the sheets
  already carry hand-applied ribbon lifetimes; only do it if a sheet-vs-corpus ribbon lifetime
  actually blocks, as its own STOP.
- Anything unenumerated → STOP into PROGRESS Blockers.
