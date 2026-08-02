# Translation sheet: NS_ExplosionGround (Vefects Anime VFX)

Schema and evidence-tag conventions: [README.md](README.md). Exemplars: [NS_BasicAttack.md](NS_BasicAttack.md),
[NS_Lightning_Range.md](NS_Lightning_Range.md).

## Completion state — READ FIRST

**Status: IMPLEMENTATION-COMPLETE (2026-08-02) — BehaviorId 40.** `Behavior_ExplosionGround.ush`
(a thin entry point over `Behavior_ExplosionShared.ush`) + `ExecuteStage_CPU` case 40, the
`PS_CkParticles_Template_ExplosionGround` cadence row, twelve row renderers + a ribbon emitter,
one new look, three new textures, two new carrier meshes, an automation test and a VfxExamples
pair. Sections 7-14 record what actually happened. `[HUMAN-VERIFY]` open — §12.

No behavior, no `.ush`, no look, no mesh, no texture, no cadence row, no test, no gym station exists for
this effect. No behavior id is allocated. Nothing has been rendered or looked at. Sections 1–6 are
archaeology and a plan; everything in them comes from the extracted corpus and is tagged `[corpus]`.

This sheet is the **reference sheet for the whole four-system explosion family** — `NS_ExplosionOmni`,
`NS_ExplosionIceGround` and `NS_ExplosionIceOmni` are structural clones and their sheets carry only
their deltas against this one. Read this file first.

**Read §6's capability-gap callout before scheduling any implementation.** This effect needs three
things the CkParticles pipeline cannot express today (ribbon renderer, light renderer, sub-UV
flipbook) plus a per-emitter cadence divergence the single-row cadence table cannot carry. It is
**not** an S-tier port.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_ExplosionGround` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| User parameters | **none** — `userParameters: []` `[corpus]` |
| Behavior id | **not allocated** — take the next free id at implementation time from `ck::particles::NumBehaviors` |

Corpus evidence (regenerate per [README.md](README.md); `Saved/` is machine-local and gitignored):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_ExplosionGround.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Flames01,Flare01,Flat02,Part01,Part04,Rainbow,Ring01,Smoke01,Star01,Star04,Trail03}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_FlatAdd.json`
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_{Sphere01,Spike01}.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_*.json`

**The source Niagara asset was never opened, in the Niagara editor or otherwise.** Every fact below is
`[corpus]` unless tagged otherwise.

> ### Sibling-variant trap — take the right system
> `[corpus]` The pack ships a *related but different* stylized family at
> `Vefects/Anime_Stylized_VFX/VFX/Particles/`: `NS_Explosion_Fire_Ground`, `NS_Explosion_Fire_Omni`,
> `NS_Explosion_Ice_Ground`, `NS_Explosion_Ice_Omni`. Those are **not** name-identical to this batch's
> targets, so the collision is milder than `NS_Lightning_Range`'s — but they are the systems a search
> for "explosion" surfaces first.
>
> **Fastest one-line discriminator `[corpus]`: the stylized siblings render through `MI_VFX_*`
> material INSTANCES and expose a `User.*` parameter block (`User.Glow Color 01`, `User.Flames Color
> 01`, …). The `Anime_VFX/Shared/Skills` targets render through `M_VFX_DisAdd_*` and have an EMPTY
> user-parameter list.** That single check settles it without reading a single module.
>
> The stylized `NS_Explosion_Fire_Ground` additionally uses `MI_VFX_Flat_01`, `MI_VFX_Lens_Rainbow_01`
> and `MI_VFX_Ring_01`, none of which appear in this system.

> ### Naming skew in the source — do NOT "fix" it
> `[corpus]` The light-carrying emitter is called **`Light`** in `NS_ExplosionGround` and
> `NS_ExplosionIceOmni`, but **`Glow_01001`** in `NS_ExplosionOmni` and `NS_ExplosionIceGround` — the
> same emitter, byte-identical modules, two names. An implementation that keys off the emitter name
> across the family will silently miss it in half the variants.

---

## 2. System anatomy `[corpus]`

**18 CPU emitters, all enabled, all WORLD space (`localSpace: false`), all bounds Dynamic,
`determinism: false`. 70 particles per loop across the 17 non-ribbon emitters** (the ribbon's count is
event-driven and not exported — see §2.1).

Every emitter's `Spawn Burst Instantaneous` carries `UseLoopCountLimit = false`, so its stored
`Loop Count Limit = 1` is an **inert authored leftover** — the same trap `NS_Lightning_Range.md` §4
records. Every emitter bursts once per loop, forever.

| # | Emitter | Count | Spawn t | Loop behav. / dur. | Lifetime | Renderer | Mesh | Material |
|---|---|---|---|---|---|---|---|---|
| 1 | `Bubble_First_Explo` | 1 | 0 | Infinite / 1.0 | 0.15 | Mesh | `SM_VFX_Sphere01` | `M_VFX_DisAdd_Flat02` |
| 2 | `Flare01` | 1 | 0.1 | Infinite / 1.0 | 0.1 | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Flare01` |
| 3 | `Glow_01` | 2 | 0 | Infinite / 1.0 | 0.2 | Sprite, Unaligned + **CustomFacingVector** | — | `M_VFX_DisAdd_Part01` |
| 4 | `Sparkles_02` | 7 | 0.05 | Infinite / 1.0 | rand 0.2–0.4 **⚠§2.3** | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Star01` |
| 5 | `Sparkles_01` | 20 | 0 | Infinite / 1.0 | rand 0.2–0.4 **⚠§2.3** | Sprite, **VelocityAligned** + FaceCamera | — | `M_VFX_DisAdd_Part04` |
| 6 | `Sparkles_02_Trail` | **event-driven** | — | Once / **0.4** | 0.2 | **Ribbon** | — | `M_VFX_DisAdd_Trail03` |
| 7 | `Smokes` | 5 | 0.05 | Once / **0.3** | rand 0.2–0.4 **⚠§2.3** | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Smoke01` |
| 8 | `SmokesCenter` | 5 | 0.05 | Once / **0.3** | rand 0.2–0.4 **⚠§2.3** | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Smoke01` |
| 9 | `Spike01` | 5 | 0 | Infinite / 1.0 | 0.15 | Mesh | `SM_VFX_Spike01` | `M_VFX_DisAdd_Flat02` |
| 10 | `Glow_02` | 3 | 0 | Infinite / 1.0 | 0.2 | Sprite, Unaligned + **CustomFacingVector** | — | `M_VFX_DisAdd_Part01` |
| 11 | `Ring` | 1 | 0.05 | Infinite / 1.0 | 0.3 | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Ring01` |
| 12 | `Sparkles_02001` | 10 | 0 | Infinite / 1.0 | rand 0.2–0.4 **⚠§2.3** | Sprite, **VelocityAligned** + FaceCamera | — | `M_VFX_DisAdd_Part04` |
| 13 | `Glow_03` | 1 | 0 | Infinite / 1.0 | 0.25 | Sprite, Unaligned + **CustomFacingVector** | — | `M_VFX_DisAdd_Part01` |
| 14 | `Glow_04` | 1 | 0 | Infinite / 1.0 | 0.2 | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 15 | `Ground_Mark` | 1 | 0 | Infinite / 1.0 | **1.5** | Sprite, Unaligned + **CustomFacingVector** | — | `M_VFX_DisAdd_Star04` |
| 16 | `Raimbow` (sic) | 1 | 0.1 | Infinite / 1.0 | 0.2 | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Rainbow` |
| 17 | `Light` | 1 | 0 | Infinite / 1.0 | 0.25 | Sprite + **`NiagaraLightRendererProperties`** (RadiusScale 10, AffectsTranslucency false) | — | `M_VFX_DisAdd_Part01` |
| 18 | `Flames` | 5 | 0.1 | Once / **0.3** | rand 0.2–0.4 **⚠§2.3** | Sprite, Unaligned + FaceCamera, **SubUV 2×2** | — | `M_VFX_DisAdd_Flames01` |

Particle counts per loop: 1+1+2+7+20+5+5+5+3+1+10+1+1+1+1+1+5 = **70** (excluding the ribbon).

### 2.1 Spawn shapes and forces `[corpus]`

| Emitter | Location module | Velocity module | Forces |
|---|---|---|---|
| `Bubble_First_Explo` | Simulation Position, offset (0,0,0) | — | — |
| `Flare01` | Simulation Position | — | — |
| `Glow_01`/`Glow_02`/`Glow_03`/`Glow_04` | Simulation Position | — | — |
| `Sparkles_02` | Sphere Location, radius **40**, `Hemisphere Z = true`, non-uniform scale (1,1,1) | Add Velocity = `Random Range Vector` **min (1500, 1500, 500) / max (−1500, −1500, 2000)** (as exported — the "min" holds the larger X/Y; treat per-axis lo/hi) | **Acceleration Force (0, 0, −4000)** |
| `Sparkles_01` | Sphere Location, radius **80**, `Hemisphere Z = true` | Add Velocity from Point, strength rand **1000–4000**, falloff distance 100, origin offset (0,0,0) | — |
| `Sparkles_02_Trail` | Initialize **Ribbon**, position offset **(100, 0, 0)**, ribbon width 10. Its `Add Velocity from Point` module is **DISABLED** | — | — |
| `Smokes` | Sphere Location, radius **70**, Surface Only, `Surface Expansion Mode = Outside`, `UseNonUniformScale = true` with non-uniform scale **(1, 1, 0)** (⇒ a flat ground ring), `Hemisphere Z = true` | Add Velocity = `Random Range Vector` min (−100,−100,100) / max (100,100,200) | — |
| `SmokesCenter` | **Cone Location**: angle 25, axis (0,0,1), length 130, point distribution 0, wedge H/V 45/45 | Add Velocity = `Random Range Vector` min (−30,−30,100) / max (30,30,200) | — |
| `Spike01` | Simulation Position. Its `Cone Location` module is **DISABLED** | — | — |
| `Ring` | Simulation Position | — | — |
| `Sparkles_02001` | Sphere Location, radius **100**, `Hemisphere Z = true` | Add Velocity from Point, strength rand **50–200** | **Curl Noise Force**: frequency 10, strength **5000**, randomization vector (0.65, 0.125, 0.37), quality Baked (Medium), `Randomize Noise Sample = true`, mask off |
| `Ground_Mark` | Simulation Position | — | — |
| `Raimbow` | Simulation Position | — | — |
| `Light` | Simulation Position | — | — |
| `Flames` | Sphere Location, radius **50**, Surface Only, `Surface Expansion Mode = Outside`, `UseNonUniformScale = true` (scale (1,1,1)), `Hemisphere Z = true` | Add Velocity = `Random Range Vector` min (−100,−100,100) / max (100,100,200) | — |

Every emitter with a velocity module also runs `Solve Forces and Velocity` with
`Clamp Velocity = false`, `Limit Acceleration = false`, `Rotational Solver Is Enabled = true`,
`Acceleration Limit 9999`, `Speed Limit 1000` — the limits are authored but **not engaged**
(`Clamp`/`Limit` both false), so treat them as inert.

### 2.2 The event chain `[corpus]` + `[corpus-v3]`

`Sparkles_02` runs a **`Generate Location Event`** module in Particle Update:
`Event Type = Every Frame`, `Event Probability Evaluation Type = Every Frame`,
`Use Event Probability = false`, `UseEventDelay = false`, `Event Send Rate = 30`,
`Delay Before Sending Events = 0.5`, `Event Probability = 0.5`, `Movement Tolerance = 0.5`,
`Unit Spacing = 20`. (The probability/delay values are authored but the two `Use*` booleans are
false, so they are inert.)

`Sparkles_02_Trail` has **no spawn module at all** in its Particle Spawn stack — only
`Initialize Ribbon` — which means its particles are spawned by an **event-handler stack** consuming
`Sparkles_02`'s location events.

**The event-handler stack IS exported now `[corpus-v3]`** (`emitters[].eventHandlers`):

| Field | Value |
|---|---|
| `sourceEmitter` | `Sparkles_02` |
| `eventName` | `LocationEvent` |
| `executionMode` | `SpawnedParticles` |
| `spawnCount` | **1 per event** (`randomSpawnCount = false`) |
| `maxEventsPerFrame` | **0** (unbounded) |
| handler module | `Receive Location Event` — `Position`, `Velocity`, `Acceleration`, `Ribbon ID`, `Ribbon UV Distance`, `Coordinate Space Transform` = **Apply**; `Color`, `Normalized Age`, `Random Normalized Float` = Output; `Interpolate Spawned Positions = true` |

So each `Sparkles_02` particle emits location events at up to 30/s and each event spawns exactly one
ribbon particle inheriting its position/velocity and **Ribbon ID** — one strand per sparkle. Ribbon
particle lifetime is **0.2 s** (`Initialize Ribbon.Lifetime`, `Lifetime Mode = Direct Set`); note the
emitter's `lifetimeResolved` reads `NO_MODULE` because that field only inspects `Initialize Particle`.
*(Was `[unresolved]` — the pre-v3 exporter wrote only three stacks per emitter.)* The ribbon is still
a §6 capability gap, but it is no longer an archaeology gap.

### 2.3 Randomized lifetimes — RESOLVED `[corpus-v3]`

Six emitters (`Sparkles_02`, `Sparkles_01`, `Smokes`, `SmokesCenter`, `Sparkles_02001`, `Flames`) set
`Lifetime Mode = Random` **and** carry `[override] Lifetime = dyn:Random Range Float`. Per [P0-D2]
the mode's static switch selects the driving pin: `Random` ⇒ **`Lifetime Min / Max` DRIVES**, and the
override on the unselected Direct-Set pin is INERT (`lifetimeResolved.source = minmax` on all six,
override under `inertOverrides`).

| Emitter | LIVE `Lifetime Min / Max` | inert `RandomRangeFloat` |
|---|---|---|
| `Sparkles_02` | **0.4 / 0.7** | ~~0.2 / 0.4~~ |
| `Sparkles_01` | **0.2 / 0.4** | ~~0.2 / 0.4~~ *(coincide)* |
| `Smokes` | **0.7 / 1.3** | ~~0.2 / 0.4~~ |
| `SmokesCenter` | **0.7 / 1.3** | ~~0.2 / 0.4~~ |
| `Sparkles_02001` | **0.4 / 0.7** | ~~0.2 / 0.4~~ |
| `Flames` | **0.35 / 0.7** | ~~0.2 / 0.4~~ |

**This sheet previously took the OPPOSITE reading** — "the `[override]` wins, so every one of them is
rand 0.2–0.4 and the `Lifetime Min / Max` pins are dead", following the `NS_BasicAttack.md` §2
precedent. That reading is WRONG under [P0-D2]. `Smokes` / `SmokesCenter` are more than 3× longer
than the sheet assumed. (It does not move THIS sheet's cadence row — `Ground_Mark`'s Direct-Set 1.5 s
is still the longest layer — but it does move the Omni pair's; see those sheets.)

---

## 3. Mesh geometry `[corpus, measured from the .obj]`

Two carrier meshes. Both `.json` files report the section material as `M_VFX_DisAdd_Slash01` — that is
the mesh asset's *default* slot material, **overridden by the emitter's renderer material** in every
case; it is not what draws.

### `SM_VFX_Sphere01` — 559 verts / 960 tris / 2 UV sets

- Bounds X −99.9999..100.0000, Y −99.9999..99.9999, Z −100.0000..100.0000 — a **unit UV-sphere of
  radius 100**, closed.
- Latitude rings measured at radius(XY) 19.51 … 100.00 with |Z| ≤ 98.08 off-pole, plus exact poles at
  Z = ±100 (radius 0) — a standard 32×16-ish lat/long sphere.
- **UV: `u` wraps longitudinally** (`corr(u, angle) = −0.742`, u = 0 at angle −180°, u = 1 at +180°);
  **`v` runs pole-to-pole, v = 0 at the +Z pole and v = 1 at the −Z pole** (`corr(v, z) = −0.991`,
  v≈0 mean Z +100, v≈1 mean Z −100). uv0 covers 0..1 fully.
- Used by `Bubble_First_Explo` with `Mesh Uniform Scale 0.8` and a scale curve peaking at 1.5 (§5) —
  so the visible first-explosion bubble reaches ≈ 100 × 0.8 × 1.5 = **120 units radius** at t = 0.2.

### `SM_VFX_Spike01` — 16 verts / 6 tris / 2 UV sets

- Bounds X ±100, Y ±100, Z 0..200 — a **4-sided pyramid**: apex at (0, 0, 200), four base corners at
  XY radius 141.42 (= the corners of a 200×200 square) at Z = 0. 6 triangles = 4 sides + a 2-triangle
  base.
- **UV: `v` runs along the spike's axis, v = 0 at the APEX (Z = 200, radius 0) and v = 1 at the BASE**
  (`corr(v, z) = −1.000`, `corr(v, radius) = +1.000`); `u` runs around the four base corners
  (`corr(u, angle) = −0.894`, u = 0 at angle +45°, u = 1 at angle −135°).
- Used by `Spike01` with per-particle random non-uniform mesh scale min (0.2, 0.2, 0.5) /
  max (0.4, 0.4, 1.5) under a scale curve peaking at 1.5 — so spikes are 40–120 units wide and
  150–450 units tall at peak.

Neither mesh is a shape the CkParticles mesh generator ships today (`Sweep`/`Tube`/`Shell`/`Disc` +
`Crescent`); both are trivially procedural (§6).

---

## 4. Material family and per-instance deltas `[corpus]`

**Two families, not one.**

### 4.1 `M_VFX_DissolveAdd` — 10 of the 11 materials

All ten are instances of `/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd` — the
**same family the CkUsf `DissolveAdd` look already implements** (`/CkUsf/Looks/DissolveAdd.ush`,
entry `CkUsf_Look_DissolveAdd`; see `NS_BasicAttack.md` §9). All are
`MD_Surface` / `BLEND_Translucent` / `MSM_Unlit`, `twoSided: false`, connected outputs
**`EmissiveColor` + `Opacity`** only, and all declare the same 4-channel dynamic parameter
**`[dissolve, distortion, offset, core_color]`**.

Deltas are stated **against `M_VFX_DisAdd_Slash01`**, the family reference `NS_BasicAttack.md` §4
already documents (Brightness 5, `Dissolve_Speed` (0.3, −0.1), `Distortion_Intensity` 1,
`Opacty_DepthFade` 20, `GradientMap_Displacement` 0.1, `Opacty_StepAdd` 0.1, `Opacity_Boldness` 1,
`Gradient_Invert` 0, unit scales/speeds elsewhere, `Color_Core` RGBA(1,1,1,0),
`GradientMap_Tex` = `T_VFX_WhitePixel`). Anything not listed is unchanged from that reference.

| Material | Main/Color tex | Dissolve tex | Distort tex | Brightness | Other deltas vs Slash01 |
|---|---|---|---|---|---|
| `Flare01` | `T_VFX_Ring_02` | `T_VFX_Part_01` | *(Noise_02)* | **2** | `Dissolve_Speed` (0,0); `Distortion_Intensity` **0**; `Gradient_Invert` **0.847619**; `GradientShape_Tex` = `T_VFX_Part_01` |
| `Part01` | `T_VFX_Part_01` | `T_VFX_Part_01` | *(Noise_02)* | **1** | `Dissolve_Speed` (0,0); `Distortion_Intensity` **0**; `Gradient_Invert` **0.5**; `Opacity_Boldness` **0.5** |
| `Part04` | `T_VFX_Part_04` | `T_VFX_Part_04` | *(Noise_02)* | **6** | `Dissolve_Speed` (0,0); `Distortion_Intensity` **0**; `Opacty_DepthFade` **30** |
| `Star01` | `T_VFX_Star_01` | `T_VFX_Star_01` | *(Noise_02)* | **6** | `Dissolve_Speed` (0,0); `Distortion_Intensity` **0**; `Opacty_DepthFade` **10** |
| `Star04` | `T_VFX_Star_04` | `T_VFX_Part_01` | *(Noise_02)* | **1** | `Dissolve_Speed` (0,0); `Distortion_Intensity` **0**; `Core_Intensity` **1**; `Opacty_DepthFade` **0** |
| `Ring01` | `T_VFX_Ring_01` | `T_VFX_Ring_01` | *(Noise_02)* | **10** | `Dissolve_Speed` (0,0); `Distortion_Intensity` **0** |
| `Rainbow` | Main `T_VFX_Ring_02`, Color `T_VFX_Part_01` | `T_VFX_Part_01` | *(Noise_02)* | **1** | `Dissolve_Speed` (0,0); `Distortion_Intensity` **0**; `GradientMap_Displacement` **0.9**; `Gradient_Invert` **2**; `Opacity_Boldness` **1.5**; `Opacty_StepAdd` **0.3**; **`GradientMap_Tex` = `T_VFX_LUT_Rainbow_01`** (the only non-white gradient map in the batch); `GradientShape_Tex` = `T_VFX_Part_01` |
| `Smoke01` | Main `T_VFX_Cloud_05`, Color `T_VFX_Cloud_04` | `T_VFX_Noise_07` | `T_VFX_Noise_04` | **10** | `Dissolve` **−0.1**; `Dissolve_Speed` (0,0); `Distortion_Intensity` **0.4**; `Distortion_Speed` (**0.1, 0.1**); `Core_Intensity` **1**; `Color_Core` **RGBA(0.001, 0.001, 0.001, 1)**; `GradientMap_Displacement` **0.75**; `Opacity_Boldness` **3** |
| `Trail03` | `T_VFX_Gradient_02` | `T_VFX_Gradient_02` | *(Noise_02)* | **4** | `Dissolve_Speed` (0,0); `Distortion_Intensity` **0**; `Opacty_DepthFade` **30** |
| `Flames01` | `T_VFX_Wind_01` | `T_VFX_Noise_04` | `T_VFX_Noise_04` | **10** | `Dissolve` **−0.1**; `Dissolve_Scale` (**2, 2**); `Dissolve_Speed` (0,0); `Distortion_Intensity` **0.5**; `Distortion_Scale` (**2, 2**); `Distortion_Speed` (**−0.3, −0.3**); `Core_Intensity` **1**; `Glow_Intensity` **2**; `Color_Core` **RGBA(0.015996, 0.014444, 0.014444, 1)** |

*(Noise_02)* = inherited unchanged from the family reference.

**Three family parameters this batch exercises that the CkUsf `DissolveAdd` look does NOT plumb
today** (`NS_BasicAttack.md` §13 records the first two as known gaps):

- `Opacty_DepthFade` — 30 / 10 / 0 across these instances. CkUsf surface looks do not wire scene depth.
- `GradientMap_Displacement` / `Gradient_Invert` / `GradientShape_Tex` — inert on every prior
  recreation because `GradientMap_Tex` was a white pixel. **It is NOT inert here**: `Rainbow` maps its
  gradient through `T_VFX_LUT_Rainbow_01`, a real 512×2 sRGB colour LUT. Recreating the `Raimbow`
  emitter faithfully requires plumbing that chain.
- `Core_Intensity` / `Color_Core` (Smoke01, Flames01, Star04) — a dark core tint the current look
  exposes as `CoreColor` but drives only through the `core_color` dynamic channel.

### 4.2 `M_VFX_FlatAdd` — 1 material (`M_VFX_DisAdd_Flat02`)

A **second, much simpler family**, despite the `DisAdd_` name prefix. `MD_Surface` /
`BLEND_Translucent` / `MSM_Unlit`, `twoSided: false`, outputs `EmissiveColor` + `Opacity`.

Expression histogram — the whole graph: `ScalarParameter ×3`, `VectorParameter ×1`,
`ParticleColor ×1`, `Multiply ×2`, `DepthFade ×1`, `MaterialFunctionCall ×1`, `WorldPosition ×1`.
**No texture parameters at all** (`textureParams: []`), **no dynamic parameters**.

Effective values on `Flat02`: `Brightness` **10**, `Opacty_DepthFade` **0**, `CamOffset` 0,
`Color_Core` RGBA(1, 1, 1, 0).

So `Flat02` is, effectively, `Emissive = ParticleColor.rgb × 10`, `Opacity = ParticleColor.a`, with
the depth fade disabled `[inferred — from the parameter set and the histogram; the exact composite
order is not reconstructable from a histogram]`. It draws both mesh emitters
(`Bubble_First_Explo`, `Spike01`).

### 4.3 Texture dependency audit `[corpus]`

All greyscale masks (`TC_Alpha`, `sRGB=false`, `TEXTUREGROUP_World`) unless noted, so Particle Color
does all the tinting.

| Texture | Size | Src fmt | Address | Used as | Procedural stand-in available today? |
|---|---|---|---|---|---|
| `T_VFX_Part_01` | 512² | TSF_G8 | Clamp/Clamp | Part01 all, Flare01 dissolve+gradient, Star04 dissolve, Rainbow color/dissolve/gradient | **YES** — `T_CkParticles_SoftParticle` (`NS_BasicAttack.md` §7 measured this exact texture: radially symmetric, fits `pow(1−r, 2.2)`) |
| `T_VFX_Part_04` | 512² | TSF_G16 | Wrap/Wrap | Part04 all | **YES** — `T_CkParticles_SparkStreak` (measured in `NS_BasicAttack.md` §7) |
| `T_VFX_Noise_02` | 512² | TSF_G16 | Wrap/Wrap | family-default dissolve/distort/gradient-shape | **YES** — `T_CkParticles_TileNoise` (`NS_BasicAttack.md` §7) |
| `T_VFX_Wind_01` | 512² | TSF_G16 | Wrap/Wrap | Flames01 main/color | **PARTIAL** — `T_CkParticles_WindBand` was baked from `T_VFX_Wind_03`, a different texture. Needs measurement. |
| `T_VFX_Noise_04` | 512² | TSF_G16 | Wrap/Wrap | Flames01 + Smoke01 dissolve/distort | probably `TileNoise`; **needs measurement** |
| `T_VFX_Noise_07` | 512² | TSF_G16 | Wrap/Wrap | Smoke01 dissolve | **needs measurement** |
| `T_VFX_Ring_01` | 512² | TSF_G16 | Wrap/Wrap | Ring01 all | **YES** — the existing `Ring` SDF bake, or the measured `T_VFX_Ring_04` characterization |
| `T_VFX_Ring_02` | 512² | TSF_G16 | Wrap/Wrap | Flare01 + Rainbow main | **needs measurement** — a *flare*, not necessarily a ring |
| `T_VFX_Star_01` | 512² | TSF_G16 | Wrap/Wrap | Star01 all | **PARTIAL** — the existing `Flare` (star) bake is close; needs measurement to confirm point count |
| `T_VFX_Star_04` | 512² | TSF_G16 | Wrap/Wrap | Star04 main (the ground mark) | **needs measurement** |
| `T_VFX_Cloud_04` | 512² | TSF_G16 | Wrap/Wrap | Smoke01 color | **PARTIAL** — the existing `Smoke` (FBM + erosion) bake; needs measurement |
| `T_VFX_Cloud_05` | 512² | TSF_G16 | Wrap/Wrap | Smoke01 main | same |
| `T_VFX_Gradient_02` | 512² | TSF_G8 | Clamp/Clamp | Trail03 all (the ribbon) | trivial to bake **if the ribbon is ever implemented** |
| `T_VFX_LUT_Rainbow_01` | **512×2** | TSF_BGRA8 | Wrap/Wrap | Rainbow gradient MAP | **NO** — `TC_Default`, **`sRGB = true`**, full colour. This is a 1D colour LUT, not a mask; the texture generator has no colour-LUT bake and the look has no gradient-map chain |
| `T_VFX_WhitePixel` | **1×1** | TSF_RGBA16 | Wrap/Wrap | every other instance's gradient map | **no-op** — not copied, not needed |

---

## 5. Per-emitter runtime curves — EXACT keyframes `[corpus]`

All curves sample **NormalizedAge** (0→1 over that emitter's own particle lifetime) unless a
`CurveIndex` binding says otherwise. `C` = constant key, `L` = linear key. Values are verbatim from
the corpus, including its own precision and its own float noise (`2.46502e-08` etc. are authored
"zeros" — reproduce them as 0).

Note the **HDR colour values > 1** on `Smokes`, `Glow_02` and `Flames` — those are deliberate
emissive over-drive, not data errors, and they multiply on top of the material's `Brightness`.

### 1. `Bubble_First_Explo` — mesh, lifetime 0.15
- **Color** (Color from Curve): R (0, 1)C (1, 1)C | G (0, 0.698996)C (1, 0.272849)C | B (0, 0.016)C (1, 0.016)C | A (0, 1)C
- **Scale Mesh Size** (Scale Float by Curve): X, Y, Z all (0, 0)C (0.2, 1.5)C (1, 1)C
- Initialize: `Color = RGBA(1, 0.184475, 0.386429, 1)` (overridden by the curve), `Mesh Uniform Scale 0.8`, `Uniform Sprite Size 500` (inert — mesh renderer), `Color.Scale Alpha 1`

### 2. `Flare01` — sprite, lifetime 0.1
- **Color**: R (0, 1)C (1, 1)C | G (0, 0.708376)C (1, 0.341915)C | B (0, 0.046665)C (1, 0.109462)C | A (0, 1)C (1, 0)L
- **Scale Sprite Size** (Uniform Curve): (0, 0.5)C (0.1, 1)L (1, 1)L
- Size 320 uniform; `Color.Scale Alpha` **0.8**; dyn params **[1, 0, 0, 0]** constant

### 3. `Glow_01` — sprite (CustomFacingVector), 2 particles, lifetime 0.2
- **Scale Color** (Vector4 from Curve, RGBA Together): R, G, B all (0, 3)L (1, 1)L | A (0, 1)L (1, 0)L
- **Scale Sprite Size**: (0, 0.5)C (0.1, 1)L (1, 1)L
- Initialize `Color = RGBA(1, 0.0908417, 0.0437351, 0.3)`; size **3500** uniform; dyn params **[0.6, 0, 0, 0]**
- `Align Sprite to Mesh Orientation`: alignment (0,0,1), facing **(0,0,1)**, orientation `quat(0,0,0,1)` — i.e. a quad flat in the local XY plane facing +Z (the same construction `NS_Lightning_Range.md` §6 documents)

### 4. `Sparkles_02` — sprite, 7 particles, lifetime rand 0.2–0.4 (⚠§2.3)
- **Scale Velocity** (Vector from Curve): X, Y, Z all (0, 1)C (0.2, 0.3)C (1, 3.91223e-08)C
- **Color**: R (0, 1)C (0.416541, 1)L (1, 1)C | G (0, 0.708376)C (0.416541, 0.341915)L (1, 0.109462)C | B (0, 0.0466651)C (0.416541, 0.109462)L (1, 0.130136)C | A (0, 1)C
- **Scale Sprite Size** (Uniform Curve): (0, 0)C (0.1, 1)C (1, 0)C
- **Scale Sprite Size 001 is DISABLED** — recorded, not applied: mode Non-Uniform Curve, uniform (0,0)L (1,1)L, non-uniform X (0, 0.2)C (0.3, 0.7)C | Y (0.2, 1)L (1, 1.2)L
- **Sprite Rotation Rate** (Float from Curve): (0, 720)C (1, 0)C — degrees/s, spinning down to a stop
- Sizes: Random Uniform, `Uniform Sprite Size Min 40 / Max 70`; dyn params **[1, 0, 0, 0]**
- Also runs `Generate Location Event` (§2.2)

### 5. `Sparkles_01` — velocity-aligned sprite, 20 particles, lifetime rand 0.2–0.4 (⚠§2.3)
- **Scale Velocity**: X, Y, Z all (0, 1)C (0.2, 0.35)C (1, 0.05)C
- **Color**: R (0, 1)C (0.416541, 1)L (1, 1)C | G (0, 0.69869)C (0.416541, 0.30758)L (1, 0.109462)C | B (0, 0.0149999)C (0.416541, 0.063)L (1, 0.130136)C | A (0, 1)C
- **Scale Sprite Size** (Uniform Curve): (0, 0)C (0.1, 1)C (1, 0)C; the module's non-uniform curve X (0,0)L (1,1)L | Y (0,0)L (1,1)L is authored but the mode is Uniform, so it is inert
- **Scale Sprite Size 001** (Non-Uniform Curve, ENABLED): X (1, 1)L | Y (0, 1)C (1, 0.6)C — the streak shortens over life
- Sizes: Random Non-Uniform, min **(35, 120)** max **(50, 140)** — width × length; dyn params **[0, 0, 0, 0]**

### 6. `Sparkles_02_Trail` — RIBBON, lifetime 0.2
- **Color**: R (0, 1)C (0.416541, 1)L (1, 1)C | G (0, 0.708376)C (0.416541, 0.341915)L (1, 0.109462)C | B (0, 0.0466651)C (0.416541, 0.109462)L (1, 0.130136)C | A (0, 1)C (0.119529, 1)L (0.997283, 0)L — **`CurveIndex = linked:Emitter.Age`**, i.e. this curve is sampled on EMITTER age, not particle age. The only such binding in the batch.
- **Scale Ribbon Width** (Float from Curve): (0, 1)C (1, 0)C
- `Initialize Ribbon`: `Ribbon Width 10`, `Position Offset (100, 0, 0)`, `Color RGBA(1,1,1,1)`, lifetime 0.2; dyn params **[1, 0, 0, 0]**

### 7. `Smokes` — sprite, 5 particles, lifetime rand 0.2–0.4 (⚠§2.3)
- **Scale Velocity**: X, Y, Z all (0, 1)C (1, 0.2)C
- **Color** (HDR): R (0, 1)C (0.0458799, **5**)L (0.162994, **5**)L (0.433444, 1)L (0.591609, 0)L | G (0, 1)C (0.0458799, **2.33892**)L (0.162994, **1.0684**)L (0.433444, 0)L (0.591609, 0)L | B (0, 1)C (0.0458799, 0.0649151)L (0.162994, 0.0149998)L (0.433444, 0)L (0.591609, 0)L | A (0.15575, 1)L (0.591609, 0.35)L
- **Dyn param 1 (`dissolve`)** (Float from Curve): (0.4, −2.46502e-08)C (1, −1)C
- **Dyn param 4 (`core_color`)** (Float from Curve 001): (0, −1)C (0.3, 1)C
- Dyn params 2, 3 = 0 constant
- **Scale Sprite Size**: (0, 0.5)C (0.2, 0.9)C (1, 1)C
- **Sprite Rotation Rate**: `Random Range Float 001`, **−30..30** deg/s
- Sizes Random Uniform **200..300**; `Color.Scale Alpha` **0.4**; initial sprite rotation random 0..360°

### 8. `SmokesCenter` — sprite, 5 particles, lifetime rand 0.2–0.4 (⚠§2.3)
Identical module set to `Smokes` except the location module (Cone, §2.1) and the colour curve:
- **Color**: R (0, 1)C (0.0458799, 1)L (0.162994, 1)L (0.433444, 1)L (0.591609, 0)L | G (0, 1)C (0.0458799, 0.467784)L (0.162994, 0.238398)L (0.433444, 0)L (0.591609, 0)L | B (0, 1)C (0.0458799, 0.012983)L (0.162994, 0.0343398)L (0.433444, 0)L (0.591609, 0)L | A (0.15575, 1)L (0.591609, 0.35)L — **no HDR overdrive here**, unlike `Smokes`
- Same velocity curve, same two dynamic-parameter curves, same sprite-size curve, same rotation rate
- Sizes Random Uniform **100..200**; `Color.Scale Alpha` **0.4**

### 9. `Spike01` — mesh, 5 particles, lifetime 0.15
- **Color**: R (0, 1)C (1, 1)C | G (0, 0.698996)C (1, 0.272849)C | B (0, 0.016)C (1, 0.016)C | A (0, 1)C — identical to `Bubble_First_Explo`'s
- **Scale Mesh Size**: X (0, 0)C (0.2, 1.5)C (1, 4.17233e-08)C | Y (0, 0)C (0.2, 1.5)C (1, 5.66244e-08)C | **Z (0, 0)C (0.2, 1.5)C** — the Z channel has only TWO keys, so it HOLDS at 1.5 past t = 0.2 while X and Y collapse to 0. The spike stays long and pinches to a needle.
- `Initial Mesh Orientation`: coordinate space **Mesh**, `Use Orientation Vector = true`, `Use Rotation Vector = true`, orientation axis (1,0,0), orientation vector (1,0,0), **rotation = `Random Range Vector` min (0, 0, 1) / max (0, 0.5, −1)** (as exported)
- Mesh scale Random Non-Uniform min (0.2, 0.2, 0.5) max (0.4, 0.4, 1.5)
- Its `Cone Location` module is **DISABLED** (values angle 25 / axis (0,0,1) / length 130 recorded but unused)

### 10. `Glow_02` — sprite (CustomFacingVector), 3 particles, lifetime 0.2
- **Color** (HDR): R (0, **10**)C (1, 1)C | G (0, **7.00525**)C (1, 0.341915)C | B (0, 0.21)C (1, 0.109462)C | A (0, 1)C (1, 0)L
- **Scale Sprite Size**: (0, 0.5)C (0.1, 1)L (1, 1)L
- Initialize `Color = RGBA(0.55, 0.0499629, 0.0240543, 1)`; size **2400** uniform; `Color.Scale Alpha` **0.3**; dyn params **[0.4, 0, 0, 0]**
- Same `Align Sprite to Mesh Orientation` triple as `Glow_01`

### 11. `Ring` — sprite, lifetime 0.3
- **Color**: R (0, 1)C (0.165409, 1)L (1, 1)L | G (0, 1)C (0.165409, 0.561483)L (1, 0.441672)L | B (0, 1)C (0.165409, 0.0691788)L (1, 0.025)L | A (0, 1)C
- **Dyn param 1 (`dissolve`)**: (0.5, **0.15**)C (1, −1)C — note the POSITIVE start; the ring is eroded early and re-forms
- **Scale Sprite Size**: (0, 0.5)C (0.5, 0.975)C (1, 1)C
- Size **500** uniform; `Color.Scale Alpha` **0.5**; initial sprite rotation random 0..360°; dyn params 2, 3, 4 = 0
- Its `Lifetime Min/Max 0.3/0.7` and `Uniform Sprite Size Min/Max 150/160` are authored but the modes are Direct Set / Uniform, so they are **inert**

### 12. `Sparkles_02001` — velocity-aligned sprite, 10 particles, lifetime rand 0.2–0.4 (⚠§2.3)
- **Scale Velocity**: X, Y, Z all (0, 1)C (0.2, 0.35)C (1, 0.05)C
- **Color**: identical to `Sparkles_01`'s (R (0,1)C (0.416541,1)L (1,1)C | G (0,0.69869)C (0.416541,0.30758)L (1,0.109462)C | B (0,0.0149999)C (0.416541,0.063)L (1,0.130136)C | A (0,1)C)
- **Scale Sprite Size** (Uniform Curve): (0, 0)C (0.1, 1)C (1, 0)C
- **Scale Sprite Size 001** (Non-Uniform Curve): X (1, 1)L | Y **(0.5, 1)C (1, 0.4)C** — later, sharper shortening than `Sparkles_01`'s
- Sizes Random Non-Uniform min **(25, 40)** max **(40, 50)**; dyn params **[0, 0, 0, 0]**
- Curl Noise Force (§2.1) is what makes this layer read as swirling embers rather than straight streaks

### 13. `Glow_03` — sprite (CustomFacingVector), lifetime 0.25
- **Scale Color** (Vector4 from Curve): R, G, B all (0, 1)L (1, 1)L | A (0, 1)L (1, 0)L
- **Scale Sprite Size**: (0, 0.5)C (0.1, 1)L (1, 1)L
- Initialize `Color = RGBA(0.55, 0.0499629, 0.0240543, 1)`; size **1600** uniform; dyn params **[1, 0, 0, 0]**
- **`ScaleColor.Scale RGB = (100, 100, 100)`** — a 100× emissive multiplier on top of everything else

### 14. `Glow_04` — sprite, lifetime 0.2
- **Color**: R (0, 1)C (1, 1)C | G (0, 0.708376)C (1, 0.341915)C | B (0, 0.046665)C (1, 0.109462)C | A (0, 1)C (1, 0)L
- **Scale Sprite Size**: (0, 0.5)C (0.1, 1)L (1, 1)L
- Initialize `Color = RGBA(0.55, 0.0499629, 0.0240543, 1)`; size **800** uniform; dyn params **[1, 0, 0, 0]**

### 15. `Ground_Mark` — sprite (CustomFacingVector), lifetime **1.5** (the longest in the system)
- **Scale Sprite Size** (Uniform Curve, runs BEFORE Color in the stack): (0, 5.66244e-08)C (0.2, 1)L (1, 1)L
- **Color** (6 keys per channel): R (0, 1)C (0.0296571, 1)L (0.0518999, 1)L (0.0803213, 0.428)L (0.133457, 0.019)L (0.289157, 0.002372)L | G (0, 1)C (0.0296571, 0.467784)L (0.0518999, 0.238398)L (0.0803213, 0.0272904)L (0.133457, 0.00252532)L (0.289157, 0.002372)L | B (0, 1)C (0.0296571, 0.012983)L (0.0518999, 0.03434)L (0.0803213, 0.0272904)L (0.133457, 0.0111253)L (0.289157, 0.004)L | A (0.00123571, 1)L (1, 0)L
- Initialize `Color = RGBA(0.55, 0.0499629, 0.0240543, 1)`; size **1500** uniform; `Color.Scale Alpha` **0.4**; initial sprite rotation random 0..360°; dyn params **[1, 0, 0, 0]**
- Same `Align Sprite to Mesh Orientation` triple as `Glow_01` — a scorch decal lying flat on the ground

### 16. `Raimbow` — sprite, lifetime 0.2
- Its `Color` module carries **no override** — `Color.Color = RGBA(1, 1, 1, 1)` constant
- **Scale Color** (Vector4 from Curve): R, G, B each a SINGLE key (0, 0.5)L | A (0, 1)L (1, 0)L
- **Scale Sprite Size**: (0, 0.5)C (0.2, 0.9)C (1, 1)L
- Initialize `Color = RGBA(0.913099, 0.913099, 0.913099, 0.25)`; size **800** uniform; initial sprite rotation random 0..360°; dyn params **[0.5, 0, 0, 0]**

### 17. `Light` — sprite + LIGHT renderer, lifetime 0.25
- **Scale Color** (RGB and Alpha Separately): Vector4 curve R, G, B all (0, 1)L (1, 1)L | A (0, 1)L (1, 0)L; **Scale Alpha** float curve (0, 1)L (1, 0)L
- **`ScaleColor.Scale RGB = (1e+06, 1e+06, 1e+06)`** — a one-million× multiplier. This is the *light's* intensity channel: `NiagaraLightRendererProperties` derives brightness from particle colour, and the sprite it also draws is only **9.16604** units across, so the sprite contribution is negligible and the number is not a typo.
- Initialize `Color = RGBA(1, 0.464488, 0.031026, 1)`; size **9.16604** uniform; dyn params **[1, 0, 0, 0]**
- Light renderer: `RadiusScale 10`, `AffectsTranslucency false`

### 18. `Flames` — sprite, **SubUV 2×2**, 5 particles, lifetime rand 0.2–0.4 (⚠§2.3)
- **Scale Velocity**: X, Y, Z all (0, 1)C (1, 0.2)C
- **Color** (HDR, and note the curves do NOT start at t=0): R (0.269242, **5**)L (0.464835, **5**)L (0.854814, 1)L | G (0.269242, **2.30392**)L (0.464835, 0.845833)L (0.854814, 0)L | B (0.269242, 0)L (0.464835, 0.0149998)L (0.854814, 0)L | A (0, 0)L (0.452762, 1)L (0.992454, 0)L
- **Dyn param 1 (`dissolve`)**: (0, −2.46502e-08)C (1, −1)C; **dyn param 2 (`distortion`) = 5 constant**; param 3 = 0; **`Param3WriteEnabled = true`**
- **Scale Sprite Size**: (0, 0.5)C (0.2, 0.9)C (1, 1)C
- **Sprite Rotation Rate**: `Random Range Float 001`, −30..30 deg/s
- `Sub UVAnimation`: mode **Random**, start frame 0, end frame 3, loop count 1
- Sizes Random Uniform **50..200**; initial sprite rotation random 0..360°

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.0 Capability-gap callout — SUPERSEDED (2026-08-02), kept for its evidence

**Every gap in the table below is CLOSED or RULED, and §8-§11 record what was actually built.**
The table is left in place because its per-emitter evidence is still the archaeology of record.

- **G1 ribbon renderer — CLOSED by [P3-D1]** (a second GPU emitter + the seed bank). The trail is
  a ribbon renderer on the row's own ribbon emitter, VisTag 197.
- **G2 light renderer — RULED [P4-D2]: DROPPED.** A Niagara light renderer is CPU-sim only
  (`NiagaraLightRendererProperties.h:37`) and every CkParticles emitter is GPU, so one would be
  silently skipped. The emitter's SPRITE is kept; only the light is gone (§13).
- **G3 sub-UV flipbook — CLOSED by C4.** `Flames` draws a 2x2 sheet on VisTag 191.
- **G4 event chain — CLOSED by [P3-G8]**: the every-frame location event becomes a BURST of 301
  points carrying solved sample times, and the trail point calls the SAME closed form the sparkle
  layer draws itself with (C6c, an identity — §11).
- **G5 — was never a gap** (`[corpus-v3]`, already recorded below).

**The original table:**

#### The archaeology (as written 2026-08-01)

Five things this source does that the pipeline **cannot express today**. Each is a real engineering
item, not a tuning knob. Be conservative: a wrong "we can do this" costs an implementation session.

| # | Gap | Why the pipeline can't express it | Cheapest honest options |
|---|---|---|---|
| G1 | **Ribbon renderer** (`Sparkles_02_Trail`, `NiagaraRibbonRendererProperties` + `M_VFX_DisAdd_Trail03` + `Scale Ribbon Width`) | `FCkParticles_StageOutput` has no ribbon attributes (`RibbonWidth`, `RibbonID`, `RibbonLinkOrder`); `ECk_ParticlesRenderer_Kind` has only `Mesh` and `VelocityAlignedSprite`; and **CkUsf deliberately does not ship a ribbon usage flag** (`NS_Lightning_Range.md` §9: "Ribbon and mesh-particle usages were deliberately NOT added" — the mesh one has since been added, the ribbon one has not) | (a) **Drop the emitter** and record it in the fidelity-gap section; (b) approximate with a chain of velocity-aligned sprites; (c) build ribbon support end to end (DI attributes + renderer kind + CkUsf ribbon usage flag + a ribbon look). (c) is its own project. |
| G2 | **Light renderer** (`Light` emitter, `NiagaraLightRendererProperties`, RadiusScale 10) | No light-renderer kind exists on the cadence row and the DI writes nothing a light renderer reads | (a) Drop it — the effect loses a dynamic point light but keeps every visible sprite; (b) add a `Light` renderer kind to `FCk_ParticlesRendererSpec` (small, self-contained, reusable across the whole explosion family and probably many future ports) |
| G3 | **Sub-UV flipbook** (`Flames`, SubUV **2×2**, `Sub UVAnimation` mode Random, frames 0–3) — also `NS_Dash`'s `Wind_Smokes` | No `SubImageIndex` output on `FCkParticles_StageOutput`; the shared sprite renderers declare no `SubImageSize`; no CkUsf look samples a flipbook atlas | (a) Bake a single non-atlased flame texture and drop the flipbook (visible as less variety, not as breakage); (b) add `SubImageIndex` to the DI contract + a `SubImageSize` on the renderer spec + atlas UV maths in the look. Reusable, but it touches the DI contract (GPU + CPU mirror + template builder) |
| G4 | **Event generation + event-handler spawn** (`Sparkles_02` → `Sparkles_02_Trail`) | CkParticles has no event bus. The handler stack IS exported now `[corpus-v3]` (§2.2), so this is a pure capability gap — the source behaviour is fully specified | Moot if G1 is dropped. If the ribbon is ever built, §2.2 carries the complete contract (1 particle per `LocationEvent`, unbounded, Position/Velocity/Acceleration/RibbonID applied) |
| G5 | ~~**Per-emitter cadence divergence**~~ — **NOT A GAP `[corpus-v3]`** | Every emitter is `Life Cycle Mode = System`, so the stored per-emitter Loop rows (1.0 Infinite / 0.3 Once / 0.4 Once) are ALL inert; the system's single `Once / 2.0 s` governs uniformly. Only the particle lifetimes still span **0.1 … 1.5 s** | Route through one row with `ParticleLifetime` = the longest layer (1.5 s) and let each layer die at its own lifetime (the `NS_BasicAttack` §8 pattern: layers past their own lifetime write zero colour/size/scale) |

**Loop authority — RESOLVED `[corpus-v3]`.** Every emitter here has `Life Cycle Mode = System`, so per
[P0-D1] the SYSTEM's rows rule: **`Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
The emitter-local 1.0 / 0.3 / 0.4 values are inert leftovers and their disagreement is irrelevant.
*(Was `[unresolved]`, with the emitters' 1.0 s as the working figure.)*

Two further items that are **work, not gaps** — expressible with the current contract:

- **`Curl Noise Force`** (`Sparkles_02001`, frequency 10 / strength 5000) must become closed-form
  noise-advected position in the `.ush` + its exact C++ mirror. Doable, but it is the one layer whose
  position is not a clean integral, so it is the likeliest source of GPU/CPU drift. Consider a cheap
  deterministic curl approximation over `CkParticles_Rand` rather than porting Niagara's baked noise.
- **`Acceleration Force (0,0,−4000)`** on `Sparkles_02` composes with the velocity-scale curve; the
  combination still integrates in closed form (the `NS_BasicAttack` §8 spark-position pattern
  extended with a quadratic term).

### 6.1 Cadence row

**A new row is required** — no existing row matches. Per [P0-D3] (loop = system loop duration,
lifetime = max resolved lifetime, burst = §2 counts):

```
{ TEXT("PS_CkParticles_Template_Explosion"), 2.0f, 1.5f, 70, Get_ExplosionRendererSpecs() }
```

- `LoopDuration` **2.0** `[corpus-v3]` — the system's `Once` loop duration. *Was 1.0, taken from the
  inert emitter rows.*
- `ParticleLifetime` **1.5** — `Ground_Mark`, the longest-lived layer (max resolved). Every shorter
  layer must write zero colour, zero size and zero scale past its own lifetime, or dead layers hang in
  the air. **§2.3's corrected lifetime reading does not move this number here** — `Ground_Mark` is a
  Direct-Set 1.5 s and is the longest layer either way. (It DOES move the Omni pair's row; see those
  sheets.)
- `BurstCount` **70** — the §2 total. Layer index = `Seed % 70`, the `NS_BasicAttack` §8 partition
  (double-modulo so a negative Seed still lands in range). Ranges: 0 `Bubble`, 1 `Flare01`,
  2–3 `Glow_01`, 4–10 `Sparkles_02`, 11–30 `Sparkles_01`, 31–35 `Smokes`, 36–40 `SmokesCenter`,
  41–45 `Spike01`, 46–48 `Glow_02`, 49 `Ring`, 50–59 `Sparkles_02001`, 60 `Glow_03`, 61 `Glow_04`,
  62 `Ground_Mark`, 63 `Raimbow`, 64 `Light`, 65–69 `Flames`.
- **Spawn-time offsets are layer state, not cadence.** Five layers spawn late (0.05 or 0.1 s). Use the
  `NS_BasicAttack` §5 spark treatment: the layer hides (colour/size 0) for `age < delay` and runs its
  curves on `(age − delay) / lifetime`.

**This row shares its shape with all four explosion variants** — see the three sibling sheets. If the
family is ported together, ONE row serves all four (the counts differ: 70 / 65 / 70 / 65, so either
take the max and hide the surplus, or add a second row for the 65-particle Omni pair). Decide that at
implementation time; do not pre-commit here.

### 6.2 Renderer / VisTag needs

The shared set (0–4) covers more of this system than of `NS_BasicAttack`:

| Source draw | Shared VisTag | Note |
|---|---|---|
| `Flare01`, `Glow_04`, `Raimbow`, `Ring`, `Sparkles_02`, `Smokes`, `SmokesCenter`, `Flames` | **0** (camera sprite) | but each needs a DIFFERENT material, and VisTag 0 binds ONE `User.SpriteMaterial` |
| `Sparkles_01`, `Sparkles_02001` | **1** (velocity-aligned) | same one-material problem |
| `Glow_01`, `Glow_02`, `Glow_03`, `Ground_Mark` | **4** (custom-facing) | matches `CustomFacingVector` + `Align Sprite to Mesh Orientation` exactly — the capability `NS_Lightning_Range` added |
| `Bubble_First_Explo`, `Spike01` | **3** or row-declared mesh | needs two new carrier meshes |
| `Sparkles_02_Trail` | — | **G1** |
| `Light`'s light renderer | — | **G2** |

**The one-material-per-VisTag constraint forces row-declared renderers** (`NS_BasicAttack.md` §8.1 and
`CkParticles/CLAUDE.md`): "one user parameter cannot carry several materials." This system needs
**11 distinct materials**, so a faithful port declares roughly **11 row renderers** above the shared
band, i.e. VisTags 10..20 given behavior 7 owns 5..9. That is legal (the ceiling is derived via
`Get_RosterVisTag_Max()`), but it is also the largest row-renderer set the pipeline has carried —
**verify the renderer count is not a practical limit on the template builder before committing**.

Two of the eleven need a renderer kind that does not exist yet: **`CustomFacingSprite`** as a
*row-declared* kind (the shared VisTag 4 exists but binds `User.SpriteMaterial`, and four distinct
looks need four distinct bindings). Adding `CustomFacingSprite` to `ECk_ParticlesRenderer_Kind`
alongside `Mesh` and `VelocityAlignedSprite` is a small, mechanical, reusable extension.

### 6.3 Mesh needs

Both are procedural in the `NS_BasicAttack` §6.3 sense — generate from §3's measurements, never import:

- **`SM_CkParticles_UvSphere`** — radius-1 UV sphere, u longitudinal (0 at −180°, 1 at +180°),
  v pole-to-pole (0 at +Z, 1 at −Z), ~960 triangles to match the source. Check first whether the
  existing `Shell` carrier already IS this; if so, reuse it and record that.
- **`SM_CkParticles_Pyramid4`** — 4-sided pyramid, apex at +Z, square base at Z = 0 with corner radius
  √2 × base half-width, 6 triangles, v = 0 at apex / v = 1 at base, u around the base corners.

### 6.4 Look needs

Ten `DissolveAdd` parameterizations (§4.1) + **one new family shader** for `M_VFX_FlatAdd`
(§4.2 — `ParticleColor × Brightness`, no textures, no dynamic parameters). The FlatAdd look is the
cheapest thing in this sheet: one tiny `.ush` and one LookDefinition.

The `DissolveAdd` family entry point needs **three new parameters** to serve these instances
faithfully: `CoreIntensity`, `GradientDisplacement` + `GradientInvert`, and a `GradientMapTex`
sampler. All three are inert on every existing look (their instances resolve `Core_Intensity 0`,
white-pixel gradient maps), so the extension follows the `NS_BasicAttack` §9 precedent: extend the
shared shader, prove existing looks regenerate unchanged.

### 6.5 Texture needs

Three of the fourteen already have procedural stand-ins (§4.3). Seven need new measurement-driven
bakes in `CkParticles_TextureGenerator.cpp`, parameterized from measurements of the corpus PNGs the
way `NS_BasicAttack.md` §7 did — derive numbers, never copy pixels. `T_VFX_LUT_Rainbow_01` is the
odd one out: a 512×2 **sRGB colour** LUT, which the greyscale-mask generator cannot produce and no
look samples. Either add a colour-LUT bake or drop the `Raimbow` layer's gradient chain and record it.

### 6.6 Behavior id

**Do NOT allocate an id in this document.** At implementation time take the next free id from
`ck::particles::NumBehaviors` (18 as of 2026-08-01, ids 0..17) and bump it. If the four explosion
variants ship together, consider whether they are four ids or one id with a variant selector — the
Ice variants differ from the Fire ones **only in colour curves and four scalars** (see the sibling
sheets), which is exactly the shape a single behavior with a colour-palette branch expresses well.
That is a design fork for the implementation session, not a decision this sheet should make.

### 6.7 Known deviations already implied

- **World space → local space.** All 18 emitters are world-space; CkParticles templates are
  local-space because self-driving behaviors write absolute positions. Same deviation
  `NS_BasicAttack.md` §13.2 records. Visible only if the spawning actor moves during the effect.
- **`Opacty_DepthFade`** (30 / 10 / 0 across §4.1) is dropped — CkUsf surface looks do not wire scene
  depth.
- **HDR colour values > 1** (up to 10 on `Glow_02`, 5 on `Smokes`/`Flames`, and the ×100 / ×1e6
  `Scale RGB` multipliers on `Glow_03` / `Light`) must survive the behavior's colour path unclamped.
  `RosterSanity` deliberately does not assert an alpha upper bound; check it does not assert a colour
  one either before writing these values.

---

## 7. Textures — THREE new bakes for the whole batch, and one gap that measured itself away

Reused, each measured off this very paint by an earlier batch: `T_VFX_Part_01` → `SoftParticle`,
`T_VFX_Part_04` → `SparkStreak`, `T_VFX_Noise_02` → `TileNoise`, `T_VFX_Noise_04` →
`TileNoiseCoarse`, `T_VFX_Noise_07` → `TileNoiseBanded`, `T_VFX_Cloud_04/05` → `Cloud04`/`Cloud05`,
`T_VFX_Ring_01` → `RingUneven`, `T_VFX_Ring_02` → `RingFlare`, `T_VFX_Star_01` → `StarFour`,
`T_VFX_Wind_01` → `WindSheet`, `T_VFX_Gradient_02` → `LinearRamp`, `T_VFX_LUT_Rainbow_01` →
`LutWhite` (held white pending [P1-D1], as every Rainbow consumer does). Every one of those reaches
this port through a look an earlier batch already carries, so none of them was re-measured.

### 7.1 `ExpGroundScorch` — the paint whose NAME is the trap

`T_VFX_Star_04` is not a star. Measured against `T_VFX_Star_01` (the source of the existing
`StarFour` bake), the two look close by the crude statistic — **pixelwise correlation 0.870**, the
second-closest near-miss the cookbook has seen — and are structurally different by the discriminating
one:

| Statistic (annulus r 0.30-0.40) | `T_VFX_Star_01` | `T_VFX_Star_04` |
|---|---|---|
| angular harmonic 4 | **0.165**, and harmonics 1/2/3/5/6/7 are all exactly **0.000** | 0.098, buried in a flat spectrum where every harmonic is 0.05-0.16 |
| min/max angular ratio | 1.94 | **6.54** |
| p90 over the whole image | 0.6118 | **0.3216** |
| mean at r 0.60-0.65 | 0.055 | **0.006** |

A clean isolated k=4 spike IS a four-point star; a flat spectrum with a 6.5x lumpiness ratio is an
irregular hand-painted blob. Rejected — for scale, the one reuse this campaign has ACCEPTED on a
correlation measured **1.00000**, `T_VFX_Lightning_02` was rejected at 0.72 and `LensSheet` at 0.56.

The bake is the `Px_LightningBolt` MEASURED-PROFILE idiom: **24 angular anchors of the measured
half-maximum radius** (min 0.2871, mean 0.3718, max 0.5091) give the blob its lumpy outline, and one
radial falloff `saturate(1.60 - rho*0.90)^3.75` in the rim-normalized radius gives it its softness.
Bake vs source: **pixelwise correlation 0.9677** — the highest any bake in this library has reached —
mean 0.0932 / 0.0974, p90 0.327 / 0.3216.

### 7.2 `GradientTrapezoid` — a transcription, and the death of §6.5's colour-texture gap

`T_VFX_Gradient_03` (the bomb explosion's shape paint, baked in this batch and shared with it) is
recorded by its sidecar as `TSF_BGRA8`, which is why `NS_Bomb_Explosion.md` §6.5 lists "a COLOUR shape
texture in a slot the look treats as a greyscale mask" as gap 9. **Measured, its channel spread is
exactly 0.0000 over all 262 144 pixels.** There is no colour in it, so there is no gap.

What it is instead: constant in u to a variance of **4.9e-32**, and a symmetric TRAPEZOID in v — 0 at
row 0, rising linearly to 1 at row 129, flat to row 382, falling to 0 at row 511. So it is functional
config rather than art and is transcribed exactly, the way `LinearRamp` was:
`saturate(min(v, 1-v) * 511/129)`. Largest deviation from the closed form: **0.008**, two 8-bit quanta,
at the two rows next to the ends.

### 7.3 `TileNoiseFine` — the first paint that needs TWO Fbm scales

`T_VFX_Noise_05` (the FresnelBomb dissolve, and the bomb's first noise bubble) was measured against
every existing noise bake's source and rejected by all four: best correlation at ANY roll is **0.11**
(against `T_VFX_Noise_02`, i.e. `TileNoise`), and 0.00 to 0.07 direct against Noise_02/04/06/07.

What separates it is its autocorrelation TAIL. It decays to 0.49 by 8 px like a fine noise but is
still at 0.233 by 32 px and 0.096 by 64 — which a single-base Fbm cannot do, because a fine base has
decayed away by 32 and a coarse one has not decayed at all by 8. So the bake is a weighted sum of
both, 0.6 fine (48 tiles) + 0.4 coarse (6 tiles), rescaled about the mixed field's own mean.

| | bake | source |
|---|---|---|
| mean / std | 0.3651 / 0.1338 | 0.3650 / 0.1341 |
| coverage > 0.5 / > 0.25 | 0.1587 / 0.8024 | 0.1491 / 0.8051 |
| autocorrelation at 4/8/16/32/64 px | 0.814 / 0.543 / 0.329 / 0.243 / 0.098 | 0.648 / 0.492 / 0.369 / 0.233 / 0.096 |

The FresnelBomb family's three looks were repointed from their capability-landing placeholder
(`TileNoise`) onto this bake — that measurement was explicitly deferred to this batch and is now made.

---

## 8. Meshes — TWO new carriers

- **`SM_CkParticles_UvSphere`** — `SM_VFX_Sphere01`. The existing `Shell` is NOT this mesh: it is
  radius 50 with u starting at angle 0, where the source is radius 100 with its seam at ±180°. The
  new carrier is a 32×16 lat/long sphere, v pole-to-pole (0 at +Z), u wrapping from −180° to +180°.
  Both signs are load-bearing — the bubbles' dissolve pans in that space. It carries 1024 triangles
  against the source's 960; the difference is two rings of pole slivers 0.05 rad across, the same
  pole-gap trade the `Shell` carrier already makes (§13).
- **`SM_CkParticles_FlatAnnulus`** — `SM_VFX_Ring03`, used by the bomb explosion in this batch. Its
  three measured radii 53.96 / 76.98 / 100.00 are EVENLY spaced (both gaps 23.02), so the two radial
  bands are one lerp, and 64 segments at an 11.25° step reproduces the source's **256 triangles
  exactly**.

Reused unchanged: `SM_CkParticles_Spike` IS `SM_VFX_Spike01` — the same 4-sided pyramid the hit ports
already generate, apex at +Z, base ±100, v = 0 at the tip.

---

## 9. The behavior — `Behavior_ExplosionShared.ush` + `ExecuteStage_CPU`'s `Explosion_Run`

**This port did not get its own layer math.** Behaviors 40, 41, 42 and 43 are four thin entry points
over ONE shared implementation, per the campaign's [P4-D1] fence, because the four source systems are
two structural variants of one effect in two palettes. The shared file takes a two-field
`FCkParticles_ExplosionParams` — a VARIANT axis (Ground / Omni: spawn shapes, layer partition, three
counts) and a PALETTE axis (Fire / Ice: colour tables plus four scalars) — and the CPU mirror is one
`Explosion_Run` function with four two-line cases.

**Where the palette tables live, and why it is not the twins' own files.** [P4-D1] asks for "a thin
file: include + palette table + entry function". HLSL has no function pointers, so shared layer math
cannot call back into a per-twin table; putting the tables in the twin would mean duplicating the
layer code that reads them, which is the exact defect the fence exists to prevent. Both palettes
therefore sit side by side in the shared file — which is also where they are most reviewable, because
they are read directly off one corpus diff.

### 9.1 The corpus diff this port rests on

Re-run at implementation against the `.txt` exports (`NS_ExplosionGround` vs `NS_ExplosionIceGround`,
with the light emitter's two names normalized): **zero renderer, material, mesh, spawn-shape, count,
lifetime, spawn-time or module-structure differences.** What the diff produces is twelve
`Color from Curve` tables, three scalars (`Glow_01`/`Glow_03`/`Light` initialize colours are colour
values; `Glow_02`'s dissolve 0.4 → 0.6 and `Flames`' `Color.Scale Alpha` 1 → 0.4 are not) and ONE
curve-index binding. Nothing contradicts the sheets. That diff is what the batch's palette tests
assert, in both directions.

### 9.2 The one difference that is neither a colour nor a scalar

The ribbon's colour curve carries `CurveIndex = linked:Emitter.Age` in the FIRE variants and carries
no binding at all in the ICE ones. It is a real behavioural difference: under the emitter binding the
whole trail fades together, under particle age each point fades on its own clock. It rides the palette
as a boolean (`CkParticles_Explosion_RibbonColorOnEmitterAge`) and both readings are pinned by a test.

### 9.3 The event ribbon, per [P3-G8]

`Sparkles_02` runs `Generate Location Event` with `Event Type = Every Frame` and both `Use*` booleans
false, so it sends ONE event per particle per frame unconditionally — the `[P3-G1]` reading, second
sighting. A rate on the ribbon emitter would force the behavior to read the emitter clock to know when
each point spawned, so the emitter BURSTS instead and each point carries a solved sample time:
**43 steps × 7 strands = 301**, 43 being `Sparkles_02`'s longest resolved life (0.7 s) at the 60 Hz
the source's Send Rate is quoted against. Point (strand k, step s) holds
`CkParticles_Explosion_Spark02Pos(P, 4 + k, s/60)` — the SAME closed form the sparkle sprite layer
draws itself with, which makes the collapse an IDENTITY the test asserts at exactly 0.0.

`Initialize Ribbon`'s `Position Offset (100, 0, 0)` is INERT: the handler's `Receive Location Event`
applies Position, and the handler stack runs after the spawn stack, so the offset is overwritten.

### 9.4 Curl, and the one measured conversion

`Sparkles_02001` carries a `Curl Noise Force` at frequency 10 / strength 5000. The frequency is the
source figure read as METRES ⇒ **0.01/unit** (the `NS_DebuffCast` §9.3 conversion); the strength is an
ACCELERATION where `CkParticles_CurlPath` advects with a VELOCITY, so the equal-ground constant
velocity over a life L is `0.5 × 5000 × L`, divided by the MEASURED mean magnitude of this plugin's own
curl field over the region these embers visit — **0.7329**, from 4000 samples within ±400 units at
frequency 0.01, seed 7. (The same field sampled within ±200 measures 0.7262, so the sampling region is
not what sets it.)

### 9.5 Closed forms the layers use

- `Sparkles_02`: `Spawn + V0 × Int3(t, 0.2, 1, 0.3, 0) × Life + ½·(0,0,−4000)·Age²` — the
  `NS_BasicAttack` §8 spark pattern with the quadratic term the `Acceleration Force` adds. Its
  Sprite Rotation Rate 720 → 0 deg/s integrates exactly to `720 × Life × Int2(t, 1, 0)`.
- `Smokes`/`SmokesCenter`/`Flames`: `Spawn + V0 × Int2(t, 1, 0.2) × Life`.
- `Spike01`: no velocity at all; its `Initial Mesh Orientation` rotation vector is carried onto the
  mesh's +Z by `CkParticles_QuatFromZTo`, drawn from the source's constrained (0, [0, 0.5], [1, −1])
  range — a mostly-upward fan.

---

## 10. Looks and renderers

**ONE new look for the whole Ground pair**, and it is the only new DissolveAdd parameterization in the
batch: `ExpGroundMarkDisAdd` (`M_VFX_DisAdd_Star04`) — the scorch decal, shape `ExpGroundScorch`,
dissolve `SoftParticle`, distortion `TileNoise`, Brightness 1, Opacity_Boldness 1, Gradient_Invert 0.
It is the batch's only instance whose shape, dissolve and distortion paints are three different assets.

Every other material this system wears was already carried, checked value-by-value against §4.1 before
reuse: `Part01` → `PartDisAdd01`, `Part04` → `PartDisAdd04`, `Star01` → `StarDisAdd01`, `Ring01` →
`RingDisAdd01`, `Rainbow` → `RainbowDisAdd`, `Flare01` → `FlareDisAdd01`, `Smoke01` → `SmokeDisAdd01`,
`Flames01` → `FlamesDisAdd01`, `Trail03` → `TrailDisAdd03`, `Flat02` → `FlatAdd02`. Ten of eleven.

**Twelve row renderers + one ribbon renderer, VisTags 185-197**, and the palette twin declares the
SAME array — literally the same function, so the two rows' `RendererOverrides` pointers compare equal
(a test asserts that). A VisTag is compared against `Particles.VisibilityTag` within one emitter of one
system, so two templates may carry the same numbers.

| VisTag | Kind | Look | Source emitters |
|---|---|---|---|
| 185 | CameraFacingSprite | `PartDisAdd01` | `Glow_04`, `Light`'s sprite |
| 186 | CameraFacingSprite | `FlareDisAdd01` | `Flare01` |
| 187 | CameraFacingSprite | `StarDisAdd01` | `Sparkles_02` |
| 188 | CameraFacingSprite | `SmokeDisAdd01` | `Smokes`, `SmokesCenter` |
| 189 | CameraFacingSprite | `RingDisAdd01` | `Ring` |
| 190 | CameraFacingSprite | `RainbowDisAdd` | `Raimbow` |
| 191 | CameraFacingSprite, 2×2 | `FlamesDisAdd01` | `Flames` |
| 192 | VelocityAlignedSprite | `PartDisAdd04` | `Sparkles_01`, `Sparkles_02001` |
| 193 | CustomFacingSprite | `PartDisAdd01` | `Glow_01`, `Glow_02`, `Glow_03` |
| 194 | CustomFacingSprite | `ExpGroundMarkDisAdd` | `Ground_Mark` |
| 195 | Mesh `UvSphere`, MeshScale 0.8 | `FlatAdd02` | `Bubble_First_Explo` |
| 196 | Mesh `Spike` | `FlatAdd02` | `Spike01` |
| 197 | Ribbon (2nd emitter) | `TrailDisAdd03` | `Sparkles_02_Trail` |

**C8's `MeshScale` earns its keep here**: the source's per-emitter `Mesh Uniform Scale 0.8` is a
CONSTANT on the carrier, so it belongs on the renderer and not folded into the behavior's animated
`Scale Mesh Size` curve. The spike's scale IS per-particle random, so its renderer stays at unit scale
and the behavior writes it.

**Cadence row:** `{ "PS_CkParticles_Template_ExplosionGround", 2.0f, 1.5f, 70, …, 0.0f, { 0.0f, 301, … } }`.

---

## 11. Tests

`CkTests.UnitTests.CkParticles.ExplosionGroundBehavior` drives `Execute_Stage_CPU` directly — no
Niagara system, no template asset, no RHI, no forked engine. What it pins:

- the cadence row's four numbers and its ribbon emitter's shape, plus the renderer-level mesh scales
  (0.8 on the bubble carrier, 1.0 on the spike one);
- the burst partition as the source's per-emitter counts, held across 200 moduli;
- **the event collapse at exactly 0.0** — a trail point sits on its leader's path, an identity rather
  than a tolerance, because both sides evaluate the same closed form;
- the emitter-age curve index (two trail points of different ages carry one colour at one instant);
- a sample past its leader's death drawing nothing;
- `Smokes`' HDR peak of 5.0 AND its `Color.Scale Alpha` of 0.4 — the [P2-D2] class, where a missing
  scale reads 2.5x too bright;
- `Ground_Mark`'s 1.5 s life, its six-key table's first knee, its 0.4 alpha scale, and a decal facing
  pair that is not degenerate;
- `Glow_03`'s 100x multiplier and the light sprite's 1e6 one surviving unclamped;
- emitter-clock INDEPENDENCE over 400 seeds — the row declares no rate, so nothing may read it.

---

## 12. Verification — A/B protocol `[HUMAN-VERIFY]`

VfxExamples gym, station **EXPLOSION GROUND**, spawn offset (0, 0, 20) — this effect is judged from
the FLOOR up. `Ck_GymVfxExamples_RestartAll` re-fires both sides in sync.

| # | What to compare | What "right" looks like |
|---|---|---|
| a | the first 0.05 s | one bubble shell blowing to 1.5x and three flat ground-plane glows opening under it |
| b | the sparkle spray | hemispherical — nothing goes BELOW the pedestal plane. If it does, the Omni variant's shapes leaked in |
| c | the trails | one strand behind each of the seven `Sparkles_02` motes, all fading TOGETHER rather than each on its own clock |
| d | the smoke | five ring puffs at radius 70 plus five up a cone, both HDR-hot for their first sixth and then falling to black |
| e | the scorch decal | still visible at 1.5 s, long after everything else is gone, lying flat and fading |
| f | the floor illumination | **DROPPED** — the original lights the ground and the recreation does not (§13.1). Judge the sprite layers, not this |
| g | the flames | five sub-UV puffs, each on a random start frame |

## 13. Confirmed fidelity differences or intentional deviations

1. **The dynamic light is DROPPED** ([P4-D2]). A Niagara light renderer is CPU-sim only
   (`NiagaraLightRendererProperties.h:37`, enforced through every `ForEachEnabledRenderer`) and every
   CkParticles emitter is `GPUComputeSim` (`CkParticles_TemplateBuilder.cpp:917`), so a light renderer
   on one is silently skipped — the exact class of invisible failure the C6a stop existed to prevent.
   The emitter's SPRITE is kept, including its ×1e6 `Scale RGB`; that multiplier is the light's
   intensity channel, so with the light gone it now drives only a 9-unit sprite's bloom. Revisit
   clause: if the A/B shows the original's floor illumination as a visible gap, the options are a
   first CPU light emitter or a proxy glow, decided on that evidence.
2. **The custom-facing pair is corrected.** All four `Align Sprite to Mesh Orientation` emitters
   author alignment AND facing as `(0, 0, 1)`, which is DEGENERATE — the sprite's up axis is the
   alignment projected off the facing plane, and that projection is zero. The recreation writes
   alignment `(0, 1, 0)` / facing `(0, 0, 1)`: a ground-flat quad with a well-defined up. The decal's
   in-plane angle is randomized 0-360° by the source anyway.
3. **World space → local space**, the standing deviation (`NS_BasicAttack.md` §13.2). Visible only if
   the spawning actor moves during the effect; the A/B pedestals do not.
4. **`Opacty_DepthFade`** (30 / 10 / 0 across §4.1) is dropped — CkUsf surface looks do not wire scene
   depth. It matters most on `Ground_Mark`, a 1500-unit quad lying on the floor.
5. **`Core_Intensity` / `Color_Core`** on `Smoke01`, `Flames01` and `Star04` are not plumbed: the
   family exposes `CoreColor` but blends toward it by the `core_color` dynamic channel, and only
   `Smokes`/`SmokesCenter` drive that channel at all.
6. **The Rainbow gradient chain** ships against the flat white ramp pending [P1-D1], as every Rainbow
   consumer does — a provable multiply by one.
7. **`SM_CkParticles_UvSphere` carries 1024 triangles against the source's 960**, because a grid
   surface whose pole row collapses emits degenerate triangles. The difference is two rings of slivers
   0.05 rad across.
8. **The seven trail strands repeat across firings.** The two emitters keep separate UniqueID
   counters, so a strand's leader is named by its BURST SLOT rather than by a shared particle
   identity — the same cost `NS_BuffCast` §13.2 records.

## 14. Reusable lessons

1. **A palette twin is an id, not a file.** Four systems, one implementation, four two-line entry
   points. The thing that made it work is that the VARIANT axis (shapes/counts/partition) and the
   PALETTE axis (colour tables/scalars) are ORTHOGONAL — check that before reaching for a shared
   include, because a shared file whose two axes interact is worse than two files.
2. **HLSL has no function pointers, so "the table lives in the thin file" is not always available.**
   When it is not, the tables belong in the shared file next to each other, not duplicated into the
   twins along with the code that reads them.
3. **A palette twin's test is a DIFFERENTIAL test.** Driving both ids over the same seeds and ages and
   asserting field-by-field identity outside the diff catches both failure directions at once: a
   structural field that moved (the sharing broke) and a colour that did not (the palette was not
   wired). It is also the only test in the cookbook whose subject is the implementation rather than
   the source.
4. **`0.870` correlation is still a rejection.** Two paints of the same family correlate highly by
   construction; the discriminating statistic is structural (here, whether the angular spectrum has an
   isolated harmonic at all). Run the structural statistic before the correlation, not after.
5. **A partition census must read each slot WHILE IT IS ALIVE.** A layer outside its own
   [delay, delay + life] window leaves its branch through the early `Hide(); return;`, which runs BEFORE
   that branch assigns `Out.VisTag` — so the sample carries the switch's pre-branch default of 0, not the
   layer's tag. That is true of every behavior in the cookbook and it is harmless on screen (the particle
   has zero colour, size and scale). It is NOT harmless in a test: this port's first gate bucketed the
   partition at one convenient instant and every layer with a 0.05 or 0.1 s beat landed in bucket 0. Sweep
   the loop instead — it is also strictly stronger, because it proves every slot draws at some point and
   that each keeps ONE renderer for its whole life. §14.7 has the full write-up.
6. **A sidecar's pixel format is not a measurement.** `T_VFX_Gradient_03` is recorded `TSF_BGRA8` and
   is greyscale to 0.0000. One `numpy` line dissolved a capability gap that had been on the sheet
   since the archaeology pass.

### 14.7 The first gate went 31/35: THREE reds from one test defect, and one editor death

`Particles` returned **31/35** on this batch's first run. Three tests failed on assertions —
`ExplosionGroundBehavior`, `ExplosionOmniBehavior` and `BombExplosionBehavior`, with both palette twins
GREEN — and those nineteen failing assertions share ONE root cause and **no behavior defect**. The fourth
red is unrelated and is covered at the end of this section.

**The signature.** In the Ground test exactly six buckets read 0 — `Flare01`, `Sparkles_02`, `Smokes` +
`SmokesCenter`, `Ring`, `Raimbow`, `Flames` — and every other bucket was exactly right (`Part01Custom` 6,
`Part04` 30, `Spike` 5, `Mark` 1, `Sphere` 1). The failing set is precisely the set of layers with a SPAWN
BEAT; the passing set is precisely the beat-free ones. Bomb failed on `Ring01` (life 0.1 s, sampled at
0.12), `FlareImpact` (life 0.05 s) and `LightningStrip` (read 2 of 5 — its life is a random 0.1-0.15 s, so
some seeds had died and some had not). That pattern cannot be produced by drift; it is structural.

**The cause.** The census read `Evaluate(age, Seed).VisTag` at ONE age. A layer outside its window returns
through the early hide, before its branch assigns the tag, so it reports 0.

**What was ruled OUT, with evidence.** Seed banking was not implicated: the event-collapse identity, the
ribbon-bank partition assertions and BOTH palette-twin differential tests passed, and they share the same
`Evaluate` path and the same `RibbonSeedBase + n` construction. A wrong variant/palette id was not
implicated either: the Omni test's Ground-vs-Omni discriminators (hemispherical vs full-sphere spawn, the
constrained vs isotropic spike fan) passed, and each variant's variant-SPECIFIC counts passed — Ground's
custom-facing quad at 6 against Omni's at 2, and Omni's "no scorch decal" at 0.

**The fix (tests only, three files).** Bucket by the tag a slot reports while ALIVE, swept across the loop,
and assert additionally that no slot is never-drawn and that each keeps ONE renderer for its whole life.
Two Bomb assertions were separately WRONG and were corrected against the corpus rather than relaxed:

- *"almost nothing survives to 0.45 s"* contradicted this system's own §6.1. The row's 0.5 s lifetime IS
  `Sparkles_01`'s resolved `Lifetime Max`, and `Sparkles_01` fires off the 0.05 s beat, so the true tail is
  **0.55 s**. It now asserts that something IS alive at 0.45, that it is ONLY the sparkle layer, and that
  nothing at all survives 0.56 — three claims where there was one wrong one.
- the VisTag-band assertion counted hidden samples as band violations (261 of them). It now checks the band
  on DRAWING samples only and asserts hidden samples are fully INERT, which is the property that matters.

**The process lesson, and it is the batch-E lesson again.** The v1 self-check reproduced the PARTITION
FUNCTION (`Seed % N` → layer) and was correct; the tests assert something else — the `StageResult` the CPU
mirror returns AT AN AGE. A sim that does not model the harness's own evaluation path cannot catch a defect
that lives in it. The self-check now models `ExecuteStage_CPU`'s output including hide-semantics and tag
defaulting, driven by the real `Rand(Seed, Salt)` so random lifetimes are per-seed correct. It reproduces
the lane exactly: the same six failing roles on Ground and Omni, the same three on Bomb, and **261** hidden
band samples against the lane's reported 261.

**The FOURTH red is a different animal, and it is not an assertion.** The lane reported 35 discovered,
31 Success, 3 Fail — and `Ck_AutoTest_Particles_SpawnAllBehaviors` never completed at all: it "was in
flight when the editor died (exit=0x1)" after the harness saw no output inside its idle window. The log
says exactly what it was doing:

```
LogStaticMesh: Display: Waiting for static meshes to be ready 0/1 (/CkFoundation/.../SM_CkParticles_Spike) ...
LogStaticMesh: Display: Waiting for static meshes to be ready 0/1 (/CkFoundation/.../SM_CkParticles_Bomb) ...
LogStaticMesh: Display: Waiting for static meshes to be ready 0/1 (/CkFoundation/.../SM_CkParticles_SlashClaw) ...
=== utb: editor produced no output within the idle window — treated as HUNG ===
```

It was blocking on a COLD STATIC-MESH BUILD, 8-30 s per carrier. `RebuildTemplateAssets` runs earlier in
the same lane and re-bakes and re-SAVES every carrier mesh, which invalidates each one's DDC; this test
then spawns the whole roster and is the first thing to load them all back. That has been true of every
lane, and this batch pushed it over the edge: **thirteen carriers instead of eleven, thirty templates
instead of twenty-five, four of them dual-emitter, and the run is `--no-nullrhi` so the spawns are real.**

Its `_TimeoutSeconds` was **10**, which the mesh builds alone blew past. Raised to **120** with the
measurement in the comment — headroom, not a tuning: the test still asserts every one of the 45 spawns
returns a live component, and a genuine hang still fails it. **That change cannot rescue a run whose
EDITOR is killed first**, and tuning the harness idle window is the orchestrator's, not this batch's.

**Adjacent finding, flagged not fixed:** `Generate_AllVfxMeshes` re-bakes and re-saves all thirteen
carriers on every `RebuildTemplateAssets`, whether or not their surface changed — which is both the
`.uasset` churn every batch has seen in `git status` and the cause of this cold-DDC stall. Skipping
unchanged meshes would remove both. It is a generator change with real blast radius and it belongs to
whoever owns the regen path, not to a port batch.
