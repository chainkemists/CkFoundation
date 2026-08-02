# Vefects → CkParticles porting plan (roll-up)

Synthesized 2026-08-01 from 29 per-effect translation sheets (`NS_*.md`, all status
**PLANNED — TRANSLATION SHEET ONLY**) covering every remaining `Anime_VFX/Shared/Skills`
system. Shipped ports: `NS_BasicAttack` (behavior 7), `NS_Lightning_Range` (17).
Per-effect facts live in the sheets; this file owns the cross-effect picture: capability
matrix, port order, shared assets, open questions. **Behavior ids are allocated only at
implementation time, in port order — never in a sheet.**

## Status (2026-08-02) — EXECUTED

All 29 systems this plan scoped are now **implementation-complete**: every sheet below
carries an allocated behavior id (18–45) with its `.ush` + CPU mirror, cadence row, row
renderers, looks/textures/meshes and automation test authored — see
[README.md](README.md)'s recipe index for the per-recipe id/status roster. Combined with
the two pre-campaign ports (`NS_BasicAttack` 7, `NS_Lightning_Range` 17), the roster is
**31 of 31 behaviors ported** (incl. `NS_Dash` as 46 — omitted from this plan's own wave
table between the tier census and the port order, caught by the close-out index sweep and
ported last; the lesson — verify completion claims against the SOURCE inventory, not the
plan's internal tables — is recorded in the campaign PROGRESS). The tier census, capability
matrix and port-order tables
below are the plan's historical record and are left as originally synthesized; they no
longer describe remaining work. **Open stage: inspection** — the `[HUMAN-VERIFY]` §12 A/B
walk is outstanding on most ports (README index has the per-recipe detail).

## Tier census

| Tier | Meaning | Effects |
|---|---|---|
| **M** | portable once Wave-1 capabilities exist (some today) | Gunshot_Projectile (**zero structural gaps — port first**), Arrow_Projectile, FireBall_Hit, Fire, Dash, Bomb_Spawn (mesh caveat Q3), HealLoop (schedule last of its family) |
| **L** | blocked on ≥1 named capability below | the other 22 |

`ExplosionIceGround`/`IceOmni` are byte-identical recolors of the fire pair — zero
structural diffs; only color keys + a few scalars. Recommend ONE behavior per pair with a
per-layer palette table (Q4) — halves the explosion family.

## Capability matrix — build once, ranked by unlock leverage

| # | Capability | Blocks | Notes |
|---|---|---|---|
| C1 | **CameraFacingSprite row-renderer kind** (+ CustomFacingSprite variant) | ~24 of 29 | Mirrors `VelocityAlignedSprite` in `FCk_ParticlesRendererSpec`; multiple distinct camera-sprite looks per system is the pack's dominant idiom (up to 11 in one system). VisTag 0/4 carry only ONE `User.SpriteMaterial`. **Highest leverage in the campaign.** |
| C2 | Parameterizable continuous cadence (spawn-rate rows, fractional rates, mixed Once/Infinite) | all 4 Loops + rate layers in Dash, FB_Projectile, Casts | Current continuous row is fixed-function. |
| C3 | Color pipeline: non-grayscale bakes (512×2 sRGB LUTs: Rainbow, Bomb; 2×2 atlases) + live gradient-map chain in `DissolveAdd.ush` (`GradientMap_Displacement`, `Gradient_Invert`) + HDR color keys (→5.0) | ~10 effects | The shipped "gradient chain is a no-op" lesson was instance-specific (white-pixel LUT) — the Rainbow/Bomb LUTs are real. |
| C4 | Sub-UV flipbook (DI `SubImageIndex` output + renderer `SubImageSize` + atlas bakes) | ~10 emitters across 8+ effects | Includes one velocity-aligned flipbook (HealCast — hardest case). `[unresolved]`: sheets record `End Frame = 4` on 2×2 atlases (valid 0–3). |
| C5 | Emitter lifecycle layering: `Self`/`Once` sub-loops inside a looping system; burst+rate on one emitter | most Casts, Dash, Gunshots, Lightning family | Partially expressible in-behavior (age windows); the sheets mark which. |
| C6 | **Ribbon renderer** + event-driven spawn | BuffCast, Lightning_Muzzle/Hit, FB_Projectile, Bomb_Projectile, ExplosionGround(+Ice) | CkUsf ships no ribbon usage flag; exporter is blind to event stacks (E2). |
| C7 | Light renderer | explosion family | New renderer class; first seen in this sweep. |
| C8 | Mesh facing modes (Velocity, CameraPosition) + renderer-level mesh scale | Arrow/FB Cast/Hit, Bomb_Explosion | CameraPosition-facing flat annulus vanishes edge-on without it. |
| C9 | New look families: **FlatAdd** (`ParticleColor × Brightness`, ~7 expressions — trivial, 12+ effects incl. `M_VFX_DisAdd_Flat02` naming trap: NOT DisAdd); FresnelBomb; MI_VFX_Bomb toon (Step-banded, opaque) | FlatAdd everywhere; the other two are Bomb-only | Do FlatAdd in Wave 1 — near-free. |
| C10 | Curl-noise / vortex forces with GPU/CPU lockstep | DebuffCast, BuffLoop, Lightning_Muzzle/Hit, FB_Projectile | No closed form — needs a deterministic shared-field design; budget real time. |
| C11 | `Spawn Per Unit` (distance-driven ribbon) | Bomb_Projectile | With C6. |
| C12 | World-space / mixed-space emitters | many (trail effects on moving spawners) | **Defer**: stationary A/B pedestals render identically; recorded as §13 known-difference per sheet. Matters for gameplay use, not the fidelity gym. |

## Exporter fixes (cheap, do before Wave 1)

- **E1 — export the System State stack.** `Life Cycle Mode = System` makes emitter-level
  Loop rows inert, and 5-of-6 systems in some batches are system-governed — cadence rows
  currently cannot be finalized from the corpus. One exporter change removes ~20 sheets'
  `[unresolved]` cadence entries. (Also explains apparent contradictions: emitters bursting
  at t=0.55 with `Loop Duration 0.3` stored.)
- **E2 — export Event Handler stacks** (ribbon spawn chains are invisible today).
- Recorded caveat, no fix needed: `[values]` blocks dump the whole Rapid-Iteration store,
  including disabled/removed modules — presence ≠ evidence.

## Port order (waves; within a family, Cast before Hit — Hit then adds zero new looks)

| Wave | Prereqs | Ports |
|---|---|---|
| 0 | none (today) | E1+E2 exporter fixes; **Gunshot_Projectile + Arrow_Projectile** (share one cadence row 1.0/10.0/3) — validates projectile class in the A/B gym with zero new capability |
| 1 | C1, C3, C4, C9-FlatAdd | Fire, FireBall_Hit, Gunshot_Hit, Arrow_Cast → Arrow_Hit, Bomb_Spawn (Q3) |
| 2 | C2, C5 (+C10 for Debuff) | PickupLoop, HealLoop, BuffLoop, DebuffLoop, HealCast, DebuffCast, PickupCast, Gunshot_Cast, FireBall_Cast, Lightning_Cast |
| 3 | C6, C11 (+C10) | FireBall_Projectile, Bomb_Projectile, BuffCast, Lightning_Muzzle |
| 4 | C7, C8, C9-rest, palette table (Q4) | ExplosionGround+Omni (Ice via palettes), Bomb_Explosion (burst 162 — 1.7× largest row; verify) |
| 5 | everything | Lightning_Hit (22 emitters, mixed space, all capabilities) |

## Shared assets — build once

- **Meshes (procedural, from measured sheet profiles):** Spike01 pyramid (6 effects),
  Plane01 doubled quad, Ring01 open cylinder, Ring04 truncated cone, Ring03, one UvSphere
  (Sphere01≡Sphere02, measured identical), SlashClaw (from SM_VFX_Slash02, measured in
  NS_DebuffCast §3). **Exception:** `SM_VFX_Bomb_01_Small` (6066 v, hand-atlas UVs) is not
  procedurally derivable → Q3.
- **Looks:** every material in ALL 29 systems is DissolveAdd family except FlatAdd (trivial),
  FresnelBomb, MI_VFX_Bomb. `PartDisAdd04` and `SlashDisAdd04` reusable verbatim from the
  Slash port. Look reuse is heavy (Part01 in all six of one batch; shared color curves are
  byte-identical across systems — a shared curve table in the naming header would pay for itself).
- **Textures:** existing stand-ins already cover Part_01/Part_04/Noise_02 (+candidates
  Ring/Flare/WindBand/Streak — measure before trusting). New bakes queued per sheet;
  two new bake *kinds* needed: sRGB color LUT and 2×2 atlas (C3/C4).

## Open questions (human / editor)

- **Q1 (highest value):** the recurring lifetime ambiguity — `Lifetime Min/Max` AND a
  random-range pin override with different numbers, ~28 emitters. One editor look at
  DebuffLoop's `Arrow_Green`/`Arrow_Purple` settles the rule pack-wide. Moves some cadence
  rows 3×.
- **Q2:** system loop durations — resolved by E1 instead if done first.
- **Q3:** the bomb prop mesh — procedural stylization vs recorded fidelity gap.
- **Q4:** Ice explosions as palette variants of one behavior (recommended) vs separate ids.

## Adjudications (2026-08-01)

1. `SlashDisAdd04` DissolveSpeed corrected `(−0.1, 0)` → `(−0.1, −0.1)` — corpus ground
   truth; the §4 delta-table presentation caused the misread. **Rule for all sheets: a delta
   table must state the full inherited pair, not just the changed axis.** (Regen looks before
   the next visual check.)
2. `NS_Lightning_Range.md` §4 loop rows annotated with the System-lifecycle evidence caveat
   (cadence stands on gym observation, not the inert emitter rows).
3. "Gradient chain is a no-op" demoted from lesson to instance-specific fact.
4. Sibling-pack discriminator standardized: Skills variants have `userParameters: []` and
   `M_VFX_DisAdd_*` materials; Stylized siblings expose `User.*` params and `MI_VFX_*`.
   Emitter counts do NOT discriminate.

## Per-port checklist (the loop each port runs)

1. Pick the sheet per wave order; re-verify its corpus files are fresh; resolve its
   `[unresolved]` entries (E1/Q1 first).
2. Allocate the next behavior id; implement the sheet's §6 (behavior + mirror in lockstep,
   cadence row, row renderers, looks, bakes, meshes).
3. Regenerate **looks BEFORE templates**; `grep -ac ExecuteStage` non-zero on every template.
4. Gates: Particles / CkUsf / VfxExamples lanes, `--parallel 1 --discover-fresh --no-nullrhi`.
5. Add the VfxExamples pair row (original path + credit); autotest covers it automatically.
6. Human A/B in the gym; fill §7–14; record §13 gaps honestly.
