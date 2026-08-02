# Phases 2–5 — charters (each gets its full PHASE_N.md at its own boundary)

Deliberate deviation from one-file-per-phase, recorded: detailed phase docs written now would
decay before execution (plans snapshot conventions; ck-methodology §7). Each phase below is
chartered here; the orchestrator writes its full PHASE_N.md — with entry criteria, capability
contracts at signature level, decision gates, and fences — at the preceding phase's boundary
ritual, from the THEN-current code and reconciled sheets.

## Phase 2 — Continuous cadence + lifecycle layering → the Loops and Casts
Capabilities: C2 (parameterizable spawn-rate rows: rate, fractional rates, per-row lifetime,
mixed Once/Infinite sub-loops), C5 (Self/Once emitter windows + burst+rate layering inside one
behavior), C10 (curl-noise/vortex forces with GPU/CPU lockstep — budget real design time; no
closed form exists, so the contract must pin a deterministic shared-field approach before any
port uses it).
Ports (10): PickupLoop, HealLoop, BuffLoop, DebuffLoop, HealCast, DebuffCast, PickupCast,
Gunshot_Cast, FireBall_Cast, Lightning_Cast. DebuffCast needs the SlashClaw procedural mesh
(measured profile in its sheet §3).

## Phase 3 — Ribbons → trails and the ribbon effects
Capabilities: C6 (ribbon renderer kind + event-driven spawn expression; corpus event data comes
from Phase 0's E2 exporter fields), C11 (Spawn-Per-Unit distance ribbons). CkUsf needs a ribbon
usage flag added to the Niagara contract (opt-in, mirroring `_UsedWithNiagaraSprites`).
Ports (4): FireBall_Projectile, Bomb_Projectile, BuffCast, Lightning_Muzzle.

## Phase 4 — Light renderer + mesh facings + the explosion family
Capabilities: C7 (light renderer row kind), C8 (mesh facing Velocity / CameraPosition +
renderer-level mesh scale), remaining C9 (FresnelBomb family), the [C-D3] palette-table mechanism
(per-layer fire/ice palettes on one behavior; VfxExamples gets four pairs).
Ports: ExplosionGround + ExplosionOmni (each with fire+ice palettes), Bomb_Explosion (burst 162 —
verify the row builds before porting math).

## Phase 5 — Lightning_Hit (the everything effect)
22 emitters, mixed local/world space, 8 one-shot sub-loops, sub-UV ×2 modes, ribbons, custom
facings. Uses every prior capability; no new ones expected — if one surfaces, it is a STOP and a
recorded decision, not an inline addition. Close the campaign with VALIDATION.md's full protocol.
