# Translation sheet: NS_ExplosionIceGround (Vefects Anime VFX)

Schema and evidence-tag conventions: [README.md](README.md). **Family reference sheet:
[NS_ExplosionGround.md](NS_ExplosionGround.md) — read it first.** This system is a **recolour** of
`NS_ExplosionGround`: identical emitter list, identical spawn shapes, identical materials, identical
meshes, identical cadence. This sheet carries the deltas and its own exact colour keys.

## Completion state — READ FIRST

**Status: IMPLEMENTATION-COMPLETE (2026-08-02) — BehaviorId 41.** A PALETTE TWIN: it shares every
line of layer math with behavior 40 through `Behavior_ExplosionShared.ush` and differs only by a
palette id. Its own `PS_CkParticles_Template_ExplosionGroundIce` row, its own automation test (the
one that proves the sharing), and its own VfxExamples pair. `[HUMAN-VERIFY]` open — §12.

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

## 7. Textures, 8. Meshes, 9. The behavior — SHARED, not duplicated

**This port added no texture, no mesh, no look, no shader file and no CPU mirror function.** It is
BehaviorId 41: two lines in `Behavior_ExplosionGroundIce.ush` (include + entry point) and two lines in
`ExecuteStage_CPU` case 41, over the same `Behavior_ExplosionShared.ush` /
`NDICkParticlesLocal::Explosion_Run` that behavior 40 runs. Read
[NS_ExplosionGround.md](NS_ExplosionGround.md) §7-§9 for the implementation of record.

What this file adds is the ice palette's own key tables, which live beside the fire ones in the shared
file (see NS_ExplosionGround.md §9 for why they cannot live here) and are transcribed verbatim from
§5.1-5.18 above.

### 9.1 The corpus diff, re-run at implementation

`diff NS_ExplosionGround.txt NS_ExplosionIceGround.txt`, with the light emitter's two names
normalized, produces **88 lines and nothing structural**:

| Class | Count | Notes |
|---|---|---|
| `Color from Curve` overrides | 12 | including the two-key collapses on `Sparkles_02`/`Sparkles_01` and the moved key TIMES on `Flames` |
| Initialize colours | 3 | `Glow_01`, `Glow_03`, `Light` |
| Non-colour scalars | 2 | `Glow_02`'s dissolve 0.4 → **0.6**; `Flames`' `Color.Scale Alpha` 1 → **0.4** |
| Curve-index bindings | 1 | the ribbon loses `CurveIndex = linked:Emitter.Age` |
| Renderers / materials / meshes / spawn shapes / counts / lifetimes / spawn times / module structure | **0** | |

§5.0's "exactly four, and nothing else" non-colour list is confirmed with one clarification: the
ribbon's curve-index binding is a fifth non-colour difference, and §5.6 already records it in prose
(it is simply not in the §5.0 table). That is the port's only correction to this sheet.

---

## 10. Looks and renderers

**Zero new looks.** The twin declares the SAME renderer array as behavior 40 — literally
`Get_ExplosionGroundRendererSpecs()`, so the two rows' `RendererOverrides` data pointers compare equal,
which the test asserts. VisTags **185-197**, the same band; a VisTag is compared against
`Particles.VisibilityTag` within one emitter of one system, so two templates may carry the same
numbers, and the twins sharing them is what lets the shared include name one set of tag constants
instead of taking them as a parameter.

**Cadence row:** `{ "PS_CkParticles_Template_ExplosionGroundIce", 2.0f, 1.5f, 70, …, 0.0f, { 0.0f, 301, … } }`
— identical to its original's in every field but the asset name. It needs its own row because a
behavior id resolves to exactly one template path and that path IS the spawn contract.

---

## 11. Tests

`CkTests.UnitTests.CkParticles.ExplosionGroundIceBehavior` is not a second copy of behavior 40's gate.
It is a DIFFERENTIAL test whose subject is the IMPLEMENTATION: it drives behaviors 40 and 41 over the
same 70 seeds × 61 ages and asserts

- every one of VisTag, Position, Velocity, Size, Scale, Rotation, MeshIndex, the sprite facing pair,
  the flipbook frame and (outside `Glow_02`) the dynamic parameter is IDENTICAL — **a recolour moves
  nothing structural**, which is the [P4-D1] fence made executable;
- colour DIFFERS on every layer the corpus diff lists, and is IDENTICAL on `Raimbow`, the one layer
  with no `Color` override in either variant;
- the two rows share the same renderer array POINTER and the same cadence;
- `Glow_02`'s dissolve is 0.4 in fire and 0.6 in ice while its size is untouched;
- `Flames`' alpha PEAKS at 1.0 in fire and 0.4 in ice — pinned at the peak, because a missing
  `Color.Scale Alpha` reads 2.5x too bright and nothing else would catch it;
- the ribbon's curve index flips: two trail points of different ages share one colour under the fire
  binding and do not under the ice one, while their geometry stays identical.

Both failure directions are real defects: a structural field that moved means the twins stopped
sharing; a colour that did not move means the palette was never wired and the ice port renders fire.

---

## 12. Verification — A/B protocol `[HUMAN-VERIFY]`

VfxExamples gym, station **EXPLOSION GROUND (ICE)**, spawn offset (0, 0, 20).

| # | What to compare | What "right" looks like |
|---|---|---|
| a | against its own original | the same blast, in blue-cyan |
| b | against the FIRE Ck pedestal | the same SHAPES, frame for frame — if the two Ck pedestals differ in silhouette, timing or spray, the twins are not sharing their math |
| c | the smoke | stays inside [0, 1] here, where the fire smoke flashes to 5x. The ice ground smoke is the one layer in the family with no overdrive at all |
| d | the flames | noticeably fainter than the fire ones — 40 % alpha |
| e | the trails | each point fading on its OWN clock rather than the strand fading together; this is the ice binding, and it is the difference easiest to lose |
| f | the floor illumination | **DROPPED**, as on the fire variant (§13) |

## 13. Confirmed fidelity differences or intentional deviations

**All eight of [NS_ExplosionGround.md](NS_ExplosionGround.md) §13 apply verbatim** — the light drop,
the corrected custom-facing pair, world→local space, the dropped depth fade, the unplumbed core chain,
the white Rainbow ramp, the sphere's pole slivers and the repeating trail strands. This variant adds
none of its own: it is the same implementation.

The [P4-D2] light-drop clause, verbatim: *a Niagara light renderer is CPU-sim only and every
CkParticles emitter is GPU, so the layer is dropped and recorded here; if the maintainer's A/B shows
the original's floor illumination as a visible gap, the options are a first CPU light emitter or a
proxy glow, decided on real evidence at inspection rather than speculatively.*

## 14. Reusable lessons

1. **The strongest evidence for a shared implementation is a diff you re-ran.** The sheet said
   "byte-identical recolour"; the port re-ran the diff before relying on it, found 88 lines, and
   classified every one. That classification is what the differential test encodes.
2. **A "colour-only" difference is rarely only colour.** Three of the five non-colour differences here
   (a dissolve scalar, an alpha scale, a curve-index binding) would each have been invisible in a test
   that only compared RGB.
3. **Two rows may share a renderer array.** Asserting the POINTERS are equal is a cheap, exact way to
   prove a twin did not fork its renderer set — much stronger than comparing contents.
