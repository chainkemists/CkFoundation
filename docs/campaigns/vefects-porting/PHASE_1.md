# Phase 1 — The four cheap capabilities + six M-tier ports

**Goal:** land the four capabilities with the highest unlock leverage (C1, C3, C4, C9-FlatAdd),
then port the six effects they unblock: `NS_Fire`, `NS_FireBall_Hit`, `NS_Gunshot_Hit`,
`NS_Arrow_Cast` → `NS_Arrow_Hit`, `NS_Bomb_Spawn`.

**Entry criteria:** Phase 0 exit VERIFIED (see its list); fresh baseline lane counts recorded.

## Capability contracts (design decisions are MADE here — executors fill bodies, not designs)

### C1 — CameraFacingSprite + CustomFacingSprite row-renderer kinds
Extend `ECk_ParticlesRenderer_Kind` with `CameraFacingSprite` and `CustomFacingSprite`; both carry
`LookName` (explicit master binding via `bOverrideMaterials`, same as the existing kinds — one
`User.SpriteMaterial` cannot carry several). Builder emission mirrors `VelocityAlignedSprite`
exactly, differing only in alignment/facing enum values (`CustomFacingSprite` = the VisTag-4 pair:
CustomAlignment + CustomFacingVector, per-particle `SpriteAlignment`/`SpriteFacing` attributes —
which `CkParticles_DefaultOutput` already seeds valid). VisTag ids stay row-allocated after the
shared 0–4 set; `Get_RosterVisTag_Max()` stays derived. Contract test: extend
`Test_Usf_NiagaraSpriteContract`/roster invariants for the new kinds.

### C3 — Color pipeline
(a) Texture generator: two new bake KINDS — sRGB color LUT (512×2, BGRA, `TC_Default`, sRGB=true)
and 2×2 grayscale atlas; both parameterized from measured corpus characteristics per [C-D5]
(for LUTs: sample the corpus LUT's color stops as measured data points — a color ramp is
functional config, transcribe the stop values from the PNG measurement).
(b) `DissolveAdd.ush`: make the gradient-map chain LIVE (GradientMap sample by luminance,
`GradientMap_Displacement`, `Gradient_Invert`) — DEFAULT-INERT: with a white 1×1 gradient map the
output must be bit-identical to today (behaviors 7/17 regression bar; verify numerically the way
the pan-clamp fix was verified). (c) HDR color keys: confirm the pipeline carries >1 values
(likely already does — additive emissive); test with a 5.0 key.

### C4 — Sub-UV flipbook
DI: new `OutSubImageIndex` output (GPU + CPU lockstep); renderer spec field `SubImageSize`
(FIntPoint) on `FCk_ParticlesRendererSpec`; builder sets renderer SubUV properties when non-zero.
Frame index driven by behavior math (age-based). `[unresolved]` from the sheets: several emitters
declare `End Frame = 4` on 2×2 atlases (valid 0–3) — port with frames 0–3 and record in §13;
verify against original at A/B.

### C9-FlatAdd — the second look family
`FlatAdd.ush`: `EmissiveColor = ParticleColor.rgb × Brightness`, `Opacity = ParticleColor.a`;
unlit translucent two-sided; no textures, no dynamic params. One look asset `FlatAdd02`
(Brightness from the corpus instance). Trap from the sheets: the instance is NAMED
`M_VFX_DisAdd_Flat02` but is NOT DissolveAdd-parented — do not fold it into the family.

## Ports (each = sheet §6 + per-port checklist; Cast before Hit for look reuse)

Order: NS_Fire → NS_FireBall_Hit → NS_Gunshot_Hit → NS_Arrow_Cast → NS_Arrow_Hit → NS_Bomb_Spawn
(ids 20–25 in that order). Bomb_Spawn uses [C-D2]'s procedural bomb stand-in (sphere + fuse in the
mesh generator) + the toon-banded look (Step-banded color — a third tiny family, in scope here).

## Exit criteria
Capabilities: contract tests green + behaviors 7/17 numerically unchanged (C3b proof). Ports: all
six pairs at maintainer A/B parity; lanes green in the orchestrator session; recipes §7–14 filled;
PROGRESS.md current.

## Fences
- No ribbon/light/cadence-parameterization work (Phases 2–4 own those).
- The gradient chain must be default-inert — a change in 7/17's output is a STOP.
- Sub-UV only where a sheet names it; no speculative flipbook conversions.
