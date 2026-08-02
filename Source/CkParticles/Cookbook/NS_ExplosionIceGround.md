# Translation sheet: NS_ExplosionIceGround (Vefects Anime VFX)

Schema and evidence-tag conventions: [README.md](README.md). **Family reference sheet:
[NS_ExplosionGround.md](NS_ExplosionGround.md) — read it first.** This system is a **recolour** of
`NS_ExplosionGround`: identical emitter list, identical spawn shapes, identical materials, identical
meshes, identical cadence. This sheet carries the deltas and its own exact colour keys.

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior, no `.ush`, no look, no mesh, no texture, no cadence row, no test, no gym station. No
behavior id allocated. Nothing rendered or looked at.

**The §6 capability gaps are the SAME five as `NS_ExplosionGround.md` §6.0** — ribbon renderer, light
renderer, sub-UV flipbook, event chain, per-emitter cadence divergence. Not an S-tier port.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_ExplosionIceGround` |
| Pack | Vefects — *Anime VFX* |
| User parameters | **none** — `userParameters: []` `[corpus]` |
| Behavior id | **not allocated** |

Corpus evidence:

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_ExplosionIceGround.{json,txt}`
- The same eleven material JSONs, two mesh JSON/OBJ pairs and texture JSONs as
  [NS_ExplosionGround.md](NS_ExplosionGround.md) §1 — **the asset dependency set is byte-for-byte the
  same.**

**The source Niagara asset was never opened.** Every fact below is `[corpus]` unless tagged otherwise.

> ### THE FINDING THAT MATTERS MOST FOR THIS SHEET
> `[corpus]` **The "Ice" variant reuses the Fire variant's materials and meshes UNCHANGED.** A full
> textual diff of `NS_ExplosionGround.txt` against `NS_ExplosionIceGround.txt` produces **zero**
> renderer, material, mesh, spawn-shape, count, lifetime, spawn-time or module-structure differences.
> Every difference is a **particle colour curve**, plus four scalars (listed in §5.0).
>
> The implementation consequence is large: if the Fire variant is ever ported, **the Ice variant is a
> colour palette on the same behavior**, not a second behavior. Do not plan it as an independent port.

> ### Sibling-variant trap
> `[corpus]` `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Explosion_Ice_Ground` is a different,
> parameterized system. **Discriminator: the stylized sibling renders through `MI_VFX_*` instances and
> exposes `User.Glow Color 01` = RGBA(0.043735, 0.313989, 1, 0.3), `User.Ground Mark Color 01`, etc.
> This target renders through `M_VFX_DisAdd_*` and has an EMPTY user-parameter list.**

> ### Naming skew — the light emitter is called `Glow_01001` here
> `[corpus]` Named **`Glow_01001`** in this system and in `NS_ExplosionOmni`, but **`Light`** in
> `NS_ExplosionGround` and `NS_ExplosionIceOmni`. Byte-identical modules in all four.

---

## 2. System anatomy `[corpus]`

**Identical to [NS_ExplosionGround.md](NS_ExplosionGround.md) §2 in every structural respect** —
18 CPU emitters, all enabled, all WORLD space, bounds Dynamic, `determinism: false`,
**70 particles per firing** across the 17 non-ribbon emitters, every burst module carrying an inert
`Loop Count Limit = 1` under `UseLoopCountLimit = false`.

**System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`** —
identical to the Ground variant's. All 18 emitters are `Life Cycle Mode = System`, so per [P0-D1]
this rules and every per-emitter Loop row is inert.

The emitter table (counts, spawn times, loop behaviours, lifetimes, renderers, meshes, materials) is
reproduced there and is unchanged here, with the single exception that emitter #17 is named
**`Glow_01001`** rather than `Light`.

§2.3 (**randomized lifetimes — RESOLVED `[corpus-v3]`**) transfers verbatim: six emitters carry both
a `Lifetime Min / Max` pair and an `[override] Lifetime = dyn:Random Range Float` at 0.2 / 0.4, and
per [P0-D2] `Lifetime Mode = Random` ⇒ **Min/Max drives, the override is inert**. Live ranges:
`Sparkles_02` **0.4/0.7**, `Sparkles_01` **0.2/0.4**, `Smokes` **0.7/1.3**, `SmokesCenter`
**0.7/1.3**, `Sparkles_02001` **0.4/0.7**, `Flames` **0.35/0.7**. It does not move this system's
cadence row (`Ground_Mark` is a Direct-Set 1.5 s and is the longest layer either way), but it makes
every smoke and spark layer live up to 3× longer than the sheet's original override-wins reading.

§2.1 (spawn shapes and forces) and §2.2 (the event chain, whose handler stack is now **exported and
resolved `[corpus-v3]`** — `LocationEvent` from `Sparkles_02`, `SpawnedParticles`, 1 particle per
event, unbounded, `Receive Location Event` applying Position/Velocity/Acceleration/RibbonID; ribbon
lifetime 0.2 s from `Initialize Ribbon`) also transfer verbatim: the
`Hemisphere Z = true` hemispherical spawns, `Smokes`' flat (1,1,0) ring, `SmokesCenter`'s Cone
Location (angle 25 / axis (0,0,1) / length 130), `Sparkles_02`'s `Acceleration Force (0,0,−4000)`,
`Sparkles_02001`'s Curl Noise Force (frequency 10, strength 5000, randomization (0.65, 0.125, 0.37)),
and the disabled `Cone Location` on `Spike01`.

---

## 3. Mesh geometry

**Identical to [NS_ExplosionGround.md](NS_ExplosionGround.md) §3.** `SM_VFX_Sphere01`
(559 v / 960 t, radius-100 UV sphere, u longitudinal, v pole-to-pole +Z→−Z) and `SM_VFX_Spike01`
(16 v / 6 t, 4-sided pyramid, apex (0,0,200), base corners at XY radius 141.42, v = 0 at apex).

---

## 4. Material family and per-instance deltas

**Identical to [NS_ExplosionGround.md](NS_ExplosionGround.md) §4 — the same eleven materials at the
same parameter values.** Ten `M_VFX_DissolveAdd` instances plus the one `M_VFX_FlatAdd` instance
(`M_VFX_DisAdd_Flat02`). Same texture set (§4.3), including `T_VFX_LUT_Rainbow_01`, the 512×2 sRGB
colour LUT with no procedural path.

**Nothing in the material layer is blue.** Every ice-vs-fire difference is carried by the emitters'
particle colour, which the `DissolveAdd` family reads as `ParticleColor` and multiplies against a
greyscale mask.

---

## 5. Per-emitter runtime curves — EXACT keyframes `[corpus]`

All curves sample **NormalizedAge** over that emitter's own lifetime unless a `CurveIndex` says
otherwise. `C` = constant key, `L` = linear key. Verbatim, including the source's own float noise and
its inconsistent precision (`0.04588` and `0.0458799` are the same authored key exported twice).

### 5.0 Non-colour deltas vs `NS_ExplosionGround` — the complete list `[corpus]`

Exactly four, and nothing else:

| Emitter | Field | Ground | **IceGround** |
|---|---|---|---|
| `Glow_01` | `InitializeParticle.Color` | RGBA(1, 0.0908417, 0.0437351, 0.3) | **RGBA(0.043735, 0.313989, 1, 0.3)** |
| `Glow_02` | `DynamicMaterialParameters.Index 0 Param 1` (`dissolve`) | 0.4 | **0.6** |
| `Glow_03` | `InitializeParticle.Color` | RGBA(0.55, 0.0499629, 0.0240543, 1) | **RGBA(0.043735, 0.313989, 1, 1)** |
| `Flames` | `Color.Scale Alpha` | 1 | **0.4** |

`Glow_01001`'s `InitializeParticle.Color` also changes — RGBA(1, 0.464488, 0.031026, 1) →
**RGBA(0.0356013, 0.83077, 1, 1)** — but that is the light's tint, i.e. a colour value, listed again
in §5.17.

### 5.1–5.18 Colour curves

**Curves IDENTICAL to `NS_ExplosionGround.md` §5** (transcribe from there): every velocity-scale
curve, every mesh-scale curve, every sprite-size curve, every dynamic-parameter curve, both rotation
rates, `Glow_01`'s and `Glow_03`'s and `Raimbow`'s and `Glow_01001`'s `Scale Color` Vector4 curves.
Also unchanged: `Ring`'s `dissolve` curve **(0.5, 0.15)C (1, −1)C** — the positive start survives here
(it is `NS_ExplosionOmni` that changes it to −0.1).

The `Color from Curve` overrides, in full:

1. **`Bubble_First_Explo`** — R (0, 0.036)C (1, 0.057)C | G (0, 0.828877)C (1, 0.298116)C | B (0, 1)C (1, 1)C | A (0, 1)C
2. **`Flare01`** — R (0, 0.0466651)C (1, 0.109462)C | G (0, 0.783538)C (1, 0.366253)C | B (0, 1)C (1, 1)C | A (0, 1)C (1, 0)L
3. **`Glow_01`** — no `Color` module; `Scale Color` Vector4 R, G, B all (0, 3)L (1, 1)L | A (0, 1)L (1, 0)L (unchanged). The blue comes from `InitializeParticle.Color` (§5.0).
4. **`Sparkles_02`** — R (0, 0.046665)C (1, 0.109462)C | G (0, 0.879623)C (1, 0.287441)C | B (0, 1)C (1, 1)C | A (0, 1)C
   **Note: TWO keys per channel here; Ground has THREE** (its middle key at t = 0.416541 is gone).
5. **`Sparkles_01`** — R (0, 0.0466651)C (1, 0.109462)C | G (0, 0.879623)C (1, 0.287441)C | B (0, 1)C (1, 1)C | A (0, 1)C
   Same two-key collapse.
6. **`Sparkles_02_Trail`** — R (0.0929671, 0.046665)C (1, 0.109462)C | G (0.0929671, 0.879623)C (1, 0.287441)C | B (0.0929671, 1)C (1, 1)C | A (0, 1)C (0.101419, 1)L (1, 0)L
   **`CurveIndex = linked:Emitter.Age` is ABSENT here.** Ground's ribbon samples its colour on
   *emitter* age; this one samples on particle age like everything else. That is a real behavioural
   difference, not an export artefact — the binding is printed when present and is not printed here.
7. **`Smokes`** — R (0, 1)C (0.0458799, 0.168269)L (0.162994, 0.0343398)L (0.433444, 0)L (0.591609, 0)L | G (0, 1)C (0.0458799, 0.991102)L (0.162994, 0.658375)L (0.433444, 0.318547)L (0.591609, 0)L | B (0, 1)C (0.0458799, 1)L (0.162994, 1)L (0.433444, 1)L (0.591609, 0)L | A (0.15575, 1)L (0.591609, 0.35)L
   **No HDR overdrive** — Ground peaks R/G at 5 / 2.33892; the ice smoke stays inside [0, 1].
8. **`SmokesCenter`** — R (0, 1)C (0.04588, 0.168269)L (0.162994, 0.03434)L (0.433444, 0)L (0.591609, 0)L | G (0, 1)C (0.04588, 0.991102)L (0.162994, 0.658375)L (0.433444, 0.318547)L (0.591609, 0)L | B (0, 1)C (0.04588, 1)L (0.162994, 1)L (0.433444, 1)L (0.591609, 0)L | A (0.15575, 1)L (0.591609, 0.35)L
   Identical to `Smokes`' in this variant (Ground's two differ).
9. **`Spike01`** — R (0, 0.0356013)C (1, 0.057)C | G (0, 0.83077)C (1, 0.298116)C | B (0, 1)C (1, 1)C | A (0, 1)C
10. **`Glow_02`** — R (0, 0.0466651)C (1, 0.109462)C | G (0, 1)C (1, 0.434154)C | B (0, 1)C (1, 1)C | A (0, 1)C (1, 0)L
    **No HDR overdrive** — Ground starts at R 10 / G 7.00525.
11. **`Ring`** — R (0, 1)C (0.165409, 0.147027)L (1, 0.0409152)L | G (0, 1)C (0.165409, 0.89627)L (1, 0.318547)L | B (0, 1)C (0.165409, 1)L (1, 1)L | A (0, 1)C
12. **`Sparkles_02001`** — R (0, 0.046665)C (1, 0.109462)C | G (0, 0.879623)C (1, 0.287441)C | B (0, 1)C (1, 1)C | A (0, 1)C
13. **`Glow_03`** — no `Color` module; `Scale Color` Vector4 R, G, B all (0, 1)L (1, 1)L | A (0, 1)L (1, 0)L (unchanged), still under `ScaleColor.Scale RGB = (100, 100, 100)`. Blue via `InitializeParticle.Color` (§5.0).
14. **`Glow_04`** — R (0, 0.046665)C (1, 0.109462)C | G (0, 1)C (1, 0.434154)C | B (0, 1)C (1, 1)C | A (0, 1)C (1, 0)L
15. **`Ground_Mark`** — R (0, 1)C (0.0296571, 0.109462)L (0.0518999, 0.0356013)L (0.0803213, 0.0356013)L (0.133457, 0.00182116)L (0.289157, 0.000664557)L | G (0, 1)C (0.0296571, 0.791298)L (0.0518999, 0.226966)L (0.0803213, 0.226966)L (0.133457, 0.00518152)L (0.289157, 0.00189078)L | B (0, 1)C (0.0296571, 1)L (0.0518999, 0.558341)L (0.0803213, 0.558341)L (0.133457, 0.0137021)L (0.289157, 0.005)L | A (0.00123571, 1)L (1, 0)L
    Sprite-size curve unchanged: (0, 5.66244e-08)C (0.2, 1)L (1, 1)L
16. **`Raimbow`** — unchanged from Ground: no `Color` override; `Scale Color` Vector4 R, G, B each a single key (0, 0.5)L | A (0, 1)L (1, 0)L
17. **`Glow_01001`** (Ground's `Light`) — unchanged curves: Vector4 R, G, B (0, 1)L (1, 1)L | A (0, 1)L (1, 0)L, plus `Scale Alpha` float (0, 1)L (1, 0)L, under `ScaleColor.Scale RGB = (1e+06, 1e+06, 1e+06)`. `InitializeParticle.Color` = **RGBA(0.0356013, 0.83077, 1, 1)**.
18. **`Flames`** — R (0.254754, 0.168269)L (0.475702, 0.03434)L (0.858436, 0)L | G (0.254754, 0.991102)L (0.475702, 0.658375)L (0.858436, 0.318547)L | B (0.254754, 1)L (0.475702, 1)L (0.858436, 1)L | A (0, 0)L (**0.482946**, 1)L (**1**, 0)L
    **No HDR overdrive** (Ground peaks R/G at 5 / 2.30392), and the key TIMES also move: Ground's
    are 0.269242 / 0.464835 / 0.854814 with alpha at 0 / 0.452762 / 0.992454.
    `Color.Scale Alpha` **0.4** (§5.0).
    Dynamic params unchanged: `dissolve` (0, −2.46502e-08)C (1, −1)C, `distortion` = **5** constant,
    `Param3WriteEnabled = true`. Sprite-size curve (0, 0.5)C (0.2, 0.9)C (1, 1)C. Rotation rate
    rand −30..30.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.0 Capability-gap callout

**The same five gaps as [NS_ExplosionGround.md](NS_ExplosionGround.md) §6.0, all present here
unchanged** — G1 ribbon renderer (`Sparkles_02_Trail` + `M_VFX_DisAdd_Trail03` + `Scale Ribbon
Width`), G2 light renderer (`Glow_01001`, RadiusScale 10), G3 sub-UV flipbook (`Flames`, 2×2, mode
Random, frames 0–3), G4 event generation → event-handler spawn (**now a pure capability gap** — the
handler stack IS exported `[corpus-v3]`, §2.2), and ~~G5 per-emitter cadence divergence~~ which
`[corpus-v3]` is **NOT a gap**: every emitter is `Life Cycle Mode = System`, so the stored
1.0 Infinite / 0.3 / 0.4 Once rows are all inert and the system's `Once / 2.0 s` governs uniformly.
Only the particle lifetimes still span 0.1 … 1.5 s.

Ground §6.0's loop-authority item applies verbatim and is likewise **RESOLVED `[corpus-v3]`**: the
system's own rows are `Loop Once / 2.0 s`.

`Curl Noise Force` and `Acceleration Force` remain **work, not gaps** (closed-form in the `.ush` plus
its exact C++ mirror).

### 6.1 Cadence row

**IDENTICAL to `NS_ExplosionGround` §6.1** — per [P0-D3]: **2.0 s loop** (the system's `Once` loop
duration `[corpus-v3]`; *was 1.0 s*), **1.5 s** particle lifetime (max resolved — `Ground_Mark`),
**70** burst, same `Seed % 70` layer partition and the same layer-index ranges. **One row serves both
systems.** Do not add a second.

### 6.2–6.5 Renderer, mesh, look and texture needs

**IDENTICAL to `NS_ExplosionGround` §6.2–6.5.** Eleven distinct materials ⇒ ~11 row-declared
renderers; the same two new renderer kinds (`CameraFacingSprite` and `CustomFacingSprite` with
explicitly bound looks); the same two procedural carriers (`SM_CkParticles_UvSphere`,
`SM_CkParticles_Pyramid4`); the same ten `DissolveAdd` parameterizations plus the new `FlatAdd` family
look; the same three new `DissolveAdd` family parameters (`CoreIntensity`, gradient-map chain,
`GradientInvert`); the same texture set with `T_VFX_LUT_Rainbow_01` unserviceable.

**Nothing in §6.2–6.5 is duplicated by porting this variant** — the whole asset set is shared.

### 6.6 Behavior id — the family decision

**Do NOT allocate an id in this document.**

This sheet's §5.0 is the strongest argument in the batch for **one behavior with a colour-palette
branch**: `NS_ExplosionIceGround` differs from `NS_ExplosionGround` in colour curves and **four
scalars**, and in nothing else — not one renderer, material, mesh, count, lifetime, spawn shape or
module. Two ids would mean two `.ush` files and two C++ mirrors that must stay in lockstep with each
other *as well as* with themselves, for a difference a palette parameter expresses exactly.

The counter-argument is that CkParticles selects behavior by `User.BehaviorId` int and has no
per-component palette parameter, so a palette branch means either two ids that share a helper function
or a new user parameter. **That is a design fork for the implementation session** — record it, do not
pre-decide it here.

### 6.7 Known deviations already implied

Same as Ground §6.7 — world→local space, `Opacty_DepthFade` dropped. **The HDR concern is milder
here**: the ice palette keeps `Smokes`, `SmokesCenter`, `Glow_02` and `Flames` inside [0, 1]. The
×100 (`Glow_03`) and ×1e6 (`Glow_01001`) `Scale RGB` multipliers still apply and must survive the
colour path unclamped.

---

## 7+. Reserved for implementation.
