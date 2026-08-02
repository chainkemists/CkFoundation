# Translation sheet: NS_ExplosionOmni (Vefects Anime VFX)

Schema and evidence-tag conventions: [README.md](README.md). **Family reference sheet:
[NS_ExplosionGround.md](NS_ExplosionGround.md) — read it first.** This system is a structural clone of
`NS_ExplosionGround` and this sheet carries its deltas plus its own exact numbers.

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior, no `.ush`, no look, no mesh, no texture, no cadence row, no test, no gym station. No
behavior id allocated. Nothing rendered or looked at. Sections 1–6 are archaeology and a plan, tagged
`[corpus]`.

**The §6 capability gaps are the SAME five as `NS_ExplosionGround.md` §6.0** (ribbon renderer, light
renderer, sub-UV flipbook, event chain, per-emitter cadence divergence). This is not an S-tier port.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_ExplosionOmni` |
| Pack | Vefects — *Anime VFX* |
| User parameters | **none** — `userParameters: []` `[corpus]` |
| Behavior id | **not allocated** |

Corpus evidence:

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_ExplosionOmni.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Flames01,Flare01,Flat02,Part01,Part04,Rainbow,Ring01,Smoke01,Star01,Trail03}.json` — **ten**; `Star04` is NOT used here (no `Ground_Mark`)
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_{Sphere01,Spike01}.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_*.json`

**The source Niagara asset was never opened.** Every fact below is `[corpus]` unless tagged otherwise.

> ### Sibling-variant trap
> `[corpus]` `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Explosion_Fire_Omni` is a different,
> parameterized system. **Discriminator: the stylized sibling renders through `MI_VFX_*` instances and
> exposes `User.Bubble First Color 01` / `User.Flames Color 01` / `User.Glow Color 03` /
> `User.Light Color 01` / `User.Rainbow Color 01` / `User.Ring Color 01` / `User.Flare Color 01` /
> `User.Glow Color 04`. This target renders through `M_VFX_DisAdd_*` and has an EMPTY user-parameter
> list.**

> ### Naming skew — the light emitter is called `Glow_01001` here
> `[corpus]` The emitter carrying the `NiagaraLightRendererProperties` is named **`Glow_01001`** in
> this system and in `NS_ExplosionIceGround`, but **`Light`** in `NS_ExplosionGround` and
> `NS_ExplosionIceOmni`. Its modules are byte-identical in all four. Do not key off the name.

---

## 2. System anatomy `[corpus]`

**15 CPU emitters, all enabled, all WORLD space, bounds Dynamic, `determinism: false`.
65 particles per loop** across the 14 non-ribbon emitters.

Against `NS_ExplosionGround`'s 18: **`Glow_01`, `Glow_02` and `Ground_Mark` are absent**, and `Light`
is named `Glow_01001`. Everything else is the same emitter list in the same order.

| # | Emitter | Count | Spawn t | Loop behav. / dur. | Lifetime | Renderer | Mesh | Material |
|---|---|---|---|---|---|---|---|---|
| 1 | `Bubble_First_Explo` | 1 | 0 | Infinite / 1.0 | **0.1** | Mesh | `SM_VFX_Sphere01` | `M_VFX_DisAdd_Flat02` |
| 2 | `Flare01` | 1 | 0.1 | Infinite / 1.0 | 0.1 | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Flare01` |
| 3 | `Sparkles_02` | 7 | 0.05 | Infinite / 1.0 | rand 0.2–0.4 **⚠§2.3** | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Star01` |
| 4 | `Sparkles_01` | 20 | 0 | Infinite / 1.0 | rand 0.2–0.4 **⚠§2.3** | Sprite, **VelocityAligned** | — | `M_VFX_DisAdd_Part04` |
| 5 | `Sparkles_02_Trail` | **event-driven** | — | Once / 0.4 | 0.2 | **Ribbon** | — | `M_VFX_DisAdd_Trail03` |
| 6 | `Smokes` | **3** | 0.05 | Once / 0.3 | rand 0.2–0.4 **⚠§2.3** | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Smoke01` |
| 7 | `SmokesCenter` | 5 | 0.05 | Once / 0.3 | rand 0.2–0.4 **⚠§2.3** | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Smoke01` |
| 8 | `Spike01` | 5 | 0 | Infinite / 1.0 | **0.1** | Mesh | `SM_VFX_Spike01` | `M_VFX_DisAdd_Flat02` |
| 9 | `Ring` | 1 | 0.05 | Infinite / 1.0 | 0.3 | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Ring01` |
| 10 | `Sparkles_02001` | 10 | 0 | Infinite / 1.0 | rand 0.2–0.4 **⚠§2.3** | Sprite, **VelocityAligned** | — | `M_VFX_DisAdd_Part04` |
| 11 | `Glow_03` | **2** | 0 | Infinite / 1.0 | 0.25 | Sprite, Unaligned + **CustomFacingVector** | — | `M_VFX_DisAdd_Part01` |
| 12 | `Glow_04` | **3** | 0 | Infinite / 1.0 | 0.2 | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 13 | `Raimbow` (sic) | 1 | **0.05** | Infinite / 1.0 | 0.2 | Sprite, Unaligned + FaceCamera | — | `M_VFX_DisAdd_Rainbow` |
| 14 | `Glow_01001` | 1 | 0 | Infinite / 1.0 | 0.25 | Sprite + **`NiagaraLightRendererProperties`** (RadiusScale 10, AffectsTranslucency false) | — | `M_VFX_DisAdd_Part01` |
| 15 | `Flames` | 5 | 0.1 | Once / 0.3 | rand 0.2–0.4 **⚠§2.3** | Sprite, Unaligned + FaceCamera, **SubUV 2×2** | — | `M_VFX_DisAdd_Flames01` |

1+1+7+20+3+5+5+1+10+2+3+1+1+5 = **65** particles/loop (excluding the ribbon).

Every `Spawn Burst Instantaneous` carries `UseLoopCountLimit = false`; the stored `Loop Count Limit = 1`
is inert. Same `Solve Forces and Velocity` settings as Ground (limits authored but not engaged).

### 2.1 Spawn shapes and forces — where this differs from Ground `[corpus]`

**"Omni" is literal: every hemispherical spawn in Ground becomes a full sphere here.**
`Hemisphere Z` flips `true → false` on **`Sparkles_02`, `Sparkles_01`, `Smokes`, `Sparkles_02001`**,
and three emitters swap a directional velocity module for an isotropic outward one.

| Emitter | Ground | **Omni** |
|---|---|---|
| `Sparkles_02` | Sphere r 40, Hemisphere Z **true**; Add Velocity = `Random Range Vector` min (1500,1500,500) max (−1500,−1500,2000) | Sphere r 40, Hemisphere Z **false**; **`Add Velocity from Point`, strength rand 500–2000**, falloff distance 100, origin offset (0,0,0) |
| `Sparkles_01` | Sphere r 80, Hemisphere Z **true** | Sphere r 80, Hemisphere Z **false**; velocity module unchanged (from-point, rand 1000–4000) |
| `Smokes` | Sphere r **70**, Surface Only, `UseNonUniformScale = true` with scale (1,1,0) (a flat ring), Hemisphere Z **true**; Add Velocity = `Random Range Vector` min (−100,−100,100) max (100,100,200) | Sphere r **50**, Surface Only, `UseNonUniformScale = **false**`, Hemisphere Z **false** (a full sphere shell); **`Add Velocity from Point`, strength rand 50–200** |
| `SmokesCenter` | **Cone Location** (angle 25, axis (0,0,1), length 130, wedge 45/45); Add Velocity = `Random Range Vector` min (−30,−30,100) max (30,30,200) | **Sphere Location**: r **20**, Surface Only, `Surface Expansion Mode = Outside`, Hemisphere Z false, **non-uniform scale (1, 1, 0)** (a flat ring), `Radius Position 1`, `V Position 0.5`, `Uniform Distribution 1`; **`Add Velocity from Point`, strength rand 0–150** |
| `Spike01` | `Initial Mesh Orientation` rotation = `Random Range Vector` min (0, 0, 1) max (0, 0.5, −1) — a constrained, mostly-upward fan | rotation = `Random Range Vector` **min (−1, −1, −1) max (1, 1, 1)** — spikes point in every direction |
| `Sparkles_02001` | Sphere r 100, Hemisphere Z **true** | Sphere r 100, Hemisphere Z **false**; Curl Noise Force unchanged (freq 10, strength 5000, randomization (0.65, 0.125, 0.37)) |
| `Flames` | Sphere r 50, Surface Only, Outside, `UseNonUniformScale = true`, Hemisphere Z **true**; Add Velocity = `Random Range Vector` min (−100,−100,100) max (100,100,200); `Uniform Sprite Size Min` **50** | Sphere r 50, Surface Only, Outside, `UseNonUniformScale = **false**`, Hemisphere Z **false**; **`Add Velocity from Point`, strength rand 130–200**; `Uniform Sprite Size Min` **100** |

Unchanged from Ground: `Bubble_First_Explo`, `Flare01`, `Ring`, `Glow_03`, `Glow_04`, `Raimbow`,
`Glow_01001` all spawn at Simulation Position with no velocity module; `Sparkles_02`'s
`Acceleration Force (0, 0, −4000)` is still present; `Spike01`'s `Cone Location` is still DISABLED;
`Sparkles_02_Trail`'s `Add Velocity from Point` is still DISABLED.

### 2.2 The event chain

Identical to `NS_ExplosionGround.md` §2.2, including its **RESOLVED `[corpus-v3]`** event-handler
block — read the contract there. `Sparkles_02` runs `Generate Location Event` (Every Frame, send
rate 30, unit spacing 20); `Sparkles_02_Trail` has only `Initialize Ribbon` in its spawn stack
(`Ribbon Width 10`, `Position Offset (100, 0, 0)`) because it is **event-spawned**: `LocationEvent`
from `Sparkles_02`, `executionMode = SpawnedParticles`, **1 particle per event**, unbounded
events/frame, `Receive Location Event` applying Position/Velocity/Acceleration/**Ribbon ID**.
Ribbon particle lifetime **0.2 s** (`Initialize Ribbon.Lifetime`, Direct Set).
*(Was `[unresolved: the event-handler stack is NOT exported]`.)*

### 2.3 Randomized lifetimes — RESOLVED `[corpus-v3]`

**Identical situation, identical numbers, to [NS_ExplosionGround.md](NS_ExplosionGround.md) §2.3** —
read the resolution there. Six emitters carry both a `Lifetime Min / Max` pair and an
`[override] Lifetime = dyn:Random Range Float` (0.2 / 0.4 on every one). Per [P0-D2]
`Lifetime Mode = Random` ⇒ **Min/Max DRIVES** and the override is INERT
(`lifetimeResolved.source = minmax` on all six). The live ranges are `Sparkles_02` **0.4/0.7**,
`Sparkles_01` **0.2/0.4**, `Smokes` **0.7/1.3**, `SmokesCenter` **0.7/1.3**, `Sparkles_02001`
**0.4/0.7**, `Flames` **0.35/0.7**.

**This sheet previously took the override as authoritative** (the `NS_BasicAttack.md` §2 precedent).
That is WRONG, **and here it MOVES THE CADENCE ROW** — the longest layer becomes 1.3 s, not 0.4 s.
See §6.1.

---

## 3. Mesh geometry

**Identical to [NS_ExplosionGround.md](NS_ExplosionGround.md) §3** — the same two carriers, same
measurements: `SM_VFX_Sphere01` (559 v / 960 t, radius-100 UV sphere, u longitudinal, v pole-to-pole
+Z→−Z) and `SM_VFX_Spike01` (16 v / 6 t, 4-sided pyramid, apex (0,0,200), base corners at XY radius
141.42, v = 0 at apex).

Same per-particle scaling: `Bubble_First_Explo` uniform 0.8; `Spike01` random non-uniform
min (0.2, 0.2, 0.5) / max (0.4, 0.4, 1.5).

---

## 4. Material family and per-instance deltas

**Identical to [NS_ExplosionGround.md](NS_ExplosionGround.md) §4, minus `M_VFX_DisAdd_Star04`** (which
only `Ground_Mark` used). Ten materials: nine `M_VFX_DissolveAdd` instances (`Flames01`, `Flare01`,
`Part01`, `Part04`, `Rainbow`, `Ring01`, `Smoke01`, `Star01`, `Trail03`) plus the one `M_VFX_FlatAdd`
instance (`Flat02`). Every scalar/vector/texture value is the same as in the Ground sheet's §4 tables —
**the Fire↔Omni difference is entirely in the emitters' colour curves, never in the materials.**

Texture set: same as Ground §4.3 minus `T_VFX_Star_04`. `T_VFX_LUT_Rainbow_01` (512×2, `TC_Default`,
**sRGB**) is still required by `Rainbow` and is still the one texture with no procedural path.

---

## 5. Per-emitter runtime curves — EXACT keyframes `[corpus]`

All curves sample **NormalizedAge** over that emitter's own lifetime unless a `CurveIndex` says
otherwise. `C` = constant key, `L` = linear key. Verbatim, including the source's own float noise.

**Curves IDENTICAL to `NS_ExplosionGround.md` §5** (transcribe from there — do not re-derive):
`Bubble_First_Explo`, `Flare01`, `Sparkles_02`, `Sparkles_01`, `Sparkles_02_Trail`, `Smokes`,
`Spike01`, `Raimbow`, `Glow_01001` (= Ground's `Light`), `Flames`.

The five that DIFFER, in full:

### `SmokesCenter` — colour goes HDR here
- **Color**: R (0, 1)C (0.0458799, **5**)L (0.162994, **5**)L (0.433444, 1)L (0.591609, 0)L | G (0, 1)C (0.0458799, **2.33892**)L (0.162994, **1.19199**)L (0.433444, 0)L (0.591609, 0)L | B (0, 1)C (0.0458799, 0.0649151)L (0.162994, **0.171699**)L (0.433444, 0)L (0.591609, 0)L | A (0.15575, 1)L (0.591609, 0.35)L
  *(Ground's `SmokesCenter` is the LDR variant peaking at 1.0; here it matches `Smokes`' overdrive
  except in the two mid keys — G 1.19199 vs `Smokes`' 1.0684, B 0.171699 vs 0.0149998.)*
- Velocity scale, both dynamic-parameter curves, sprite-size curve and rotation rate are unchanged
  from Ground (X, Y, Z (0,1)C (1,0.2)C; `dissolve` (0.4, −2.46502e-08)C (1, −1)C; `core_color`
  (0, −1)C (0.3, 1)C; size (0, 0.5)C (0.2, 0.9)C (1, 1)C; rotation rate rand −30..30)
- Sizes Random Uniform 100..200; `Color.Scale Alpha` 0.4

### `Ring` — cooler colour and a NEGATIVE dissolve start
- **Color**: R (0, 1)C (0.165409, 1)L (1, 1)L | G (0, 1)C (0.165409, **0.550878**)L (1, **0.389792**)L | B (0, 1)C (0.165409, **0.0261113**)L (1, **0.0328193**)L | A (0, 1)C
- **Dyn param 1 (`dissolve`)**: (0.5, **−0.1**)C (1, −1)C — Ground starts at **+0.15**; this one never
  erodes, it only intensifies
- **Scale Sprite Size**: (0, 0.5)C (0.5, 0.975)C (1, 1)C — unchanged
- Size **400** uniform (Ground: 500); `Color.Scale Alpha` 0.5; sprite rotation random 0..360°

### `Sparkles_02001` — same curves, different spawn
Every curve is identical to Ground's (`NS_ExplosionGround.md` §5.12): velocity scale
(0,1)C (0.2,0.35)C (1,0.05)C; colour = `Sparkles_01`'s; uniform size (0,0)C (0.1,1)C (1,0)C;
non-uniform Y (0.5, 1)C (1, 0.4)C. Only `Hemisphere Z` changes (§2.1).

### `Glow_03` — 2 particles, brighter, smaller, warmer
- **Scale Color** (Vector4 from Curve): R, G, B all (0, **5**)L (1, 1)L | A (0, 1)L (1, 0)L
  *(Ground: (0, 1)L (1, 1)L — no initial overdrive)*
- **Scale Sprite Size**: (0, 0.5)C (0.1, 1)L (1, 1)L — unchanged
- Initialize `Color = **RGBA(1, 0.153251, 0.0335489, 1)**` (Ground: RGBA(0.55, 0.0499629, 0.0240543, 1));
  size **1300** uniform (Ground: 1600); dyn params [1, 0, 0, 0]
- `ScaleColor.Scale RGB = (100, 100, 100)` — unchanged from Ground

### `Glow_04` — 3 particles, HDR start
- **Color**: R (0, **3**)C (1, 1)C | G (0, **2.12513**)C (1, 0.341915)C | B (0, **0.139995**)C (1, 0.109462)C | A (0, 1)C (1, 0)L
  *(Ground: R (0,1)C, G (0,0.708376)C, B (0,0.046665)C — same endpoints, ~3× brighter start)*
- **Scale Sprite Size**: (0, 0.5)C (0.1, 1)L (1, 1)L — unchanged
- Initialize `Color = RGBA(0.55, 0.0499629, 0.0240543, 1)`; size 800 uniform; dyn params [1, 0, 0, 0]

**Also different but not a curve:** `Raimbow` spawns at **t = 0.05** (Ground: 0.1) at size **600**
(Ground: 800).

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.0 Capability-gap callout

**The same five gaps as [NS_ExplosionGround.md](NS_ExplosionGround.md) §6.0** — do not re-derive them,
read that table:

| # | Gap | Present here? |
|---|---|---|
| G1 | Ribbon renderer (`Sparkles_02_Trail` + `M_VFX_DisAdd_Trail03` + `Scale Ribbon Width`) | **YES** — identical emitter |
| G2 | Light renderer (`Glow_01001`, RadiusScale 10) | **YES** — identical emitter, different name |
| G3 | Sub-UV flipbook (`Flames`, 2×2, mode Random, frames 0–3) | **YES** |
| G4 | Event generation → event-handler spawn | **YES** — pure capability gap now; the handler stack IS exported `[corpus-v3]` (§2.2) |
| G5 | ~~Per-emitter cadence divergence~~ | **NO — NOT A GAP `[corpus-v3]`**: every emitter is `Life Cycle Mode = System`, so the stored 1.0 Infinite / 0.3 / 0.4 Once rows are all inert and the system's `Once / 2.0 s` governs uniformly. `Ground_Mark`'s 1.5 s layer is absent here, so the longest layer is **1.3 s** (`Smokes` / `SmokesCenter`, §2.3 resolved) — *not the 0.4 s this sheet assumed* |

Ground §6.0's loop-authority item applies verbatim, and is **RESOLVED `[corpus-v3]`**: every emitter
is `Life Cycle Mode = System`, and the system's own rows are **`Loop Behavior = Once`,
`Loop Duration = 2.0 s`, `Loop Delay = 0`, `Inactive Response = Complete`,
`Recalculate Duration Each Loop = false`** — the authority per [P0-D1].

Also as in Ground: `Curl Noise Force` (`Sparkles_02001`) and the
`Acceleration Force (0,0,−4000)` on `Sparkles_02` are **work, not gaps** — closed-form in the `.ush`
plus its exact C++ mirror.

### 6.1 Cadence row

No existing row matches. Per [P0-D3] (loop = system loop duration, lifetime = max resolved lifetime,
burst = §2 counts):

```
{ TEXT("PS_CkParticles_Template_ExplosionOmni"), 2.0f, 1.3f, 65, Get_ExplosionOmniRendererSpecs() }
```

- `LoopDuration` **2.0** `[corpus-v3]` — the system's `Once` loop duration. *Was 1.0, taken from the
  inert emitter rows.*
- `ParticleLifetime` **1.3** `[corpus-v3]` — `Smokes` / `SmokesCenter`'s resolved `Lifetime Max`.
  *Was `[unresolved]` with 0.4 s as this sheet's working reading; §2.3's override-wins assumption is
  corrected per [P0-D2], so the longest layer is 3.25× what the sheet assumed.* Shorter layers write
  zero colour/size/scale past their own lifetime, per `NS_BasicAttack.md` §8.
- `BurstCount` **65**, layer index = `Seed % 65` (double-modulo). Ranges: 0 `Bubble`, 1 `Flare01`,
  2–8 `Sparkles_02`, 9–28 `Sparkles_01`, 29–31 `Smokes`, 32–36 `SmokesCenter`, 37–41 `Spike01`,
  42 `Ring`, 43–52 `Sparkles_02001`, 53–54 `Glow_03`, 55–57 `Glow_04`, 58 `Raimbow`, 59 `Glow_01001`,
  60–64 `Flames`.
- Late-spawn layers (0.05 / 0.1 s) hide before their delay and run curves on `(age − delay) / lifetime`.

**`NS_ExplosionIceOmni` has the IDENTICAL cadence** (2.0 s loop, 1.3 s lifetime, 65 particles — same
lifetimes bar the two 0.1 → 0.15 direct-set bumps) — one row serves both. Whether the Ground pair
(2.0 s / 1.5 s / 70 particles) shares it is a §6.6 decision; `[corpus-v3]` all four variants now
agree on the 2.0 s loop, so only lifetime and burst differ.

### 6.2 Renderer / VisTag needs

Ten distinct materials ⇒ roughly **10 row-declared renderers**, because VisTag 0/1/4 each bind exactly
one material via `User.SpriteMaterial`. Breakdown:

| Kind needed | Count | Source emitters |
|---|---|---|
| camera-facing sprite w/ explicit look | 7 | `Flare01`, `Sparkles_02`, `Smokes`, `SmokesCenter`, `Ring`, `Glow_04`, `Raimbow`, `Flames` (8 draws, 7 distinct materials — `Smokes` and `SmokesCenter` share `Smoke01`) |
| velocity-aligned sprite | 1 | `Sparkles_01`, `Sparkles_02001` (both `Part04` — ONE renderer serves both) |
| custom-facing sprite w/ explicit look | 1 | `Glow_03` |
| mesh | 2 | `Bubble_First_Explo` (sphere), `Spike01` (pyramid) — both `Flat02`, but different meshes ⇒ two renderers |
| sprite for the light emitter | 1 | `Glow_01001` (`Part01`, size 9.17 — visually negligible; a candidate to drop with G2) |

**Two new renderer kinds are required** on `ECk_ParticlesRenderer_Kind` (today only `Mesh` and
`VelocityAlignedSprite`): a **`CameraFacingSprite`** with an explicitly bound look, and a
**`CustomFacingSprite`** with an explicitly bound look. Both are small, mechanical, and reusable across
the whole batch.

### 6.3 Mesh needs

Same two procedural carriers as Ground §6.3: `SM_CkParticles_UvSphere` and `SM_CkParticles_Pyramid4`,
generated from §3's measurements. Check whether the existing `Shell` carrier already is the sphere.

### 6.4 Look needs

Nine `DissolveAdd` parameterizations + the new `FlatAdd` family look (Ground §6.4). Same three new
family parameters (`CoreIntensity`, gradient-map chain, `GradientInvert`).

### 6.5 Texture needs

Ground §4.3 minus `T_VFX_Star_04`: three already covered by existing procedural bakes
(`SoftParticle`, `SparkStreak`, `TileNoise`), six need new measurement-driven bakes, and
`T_VFX_LUT_Rainbow_01` needs a colour-LUT capability that does not exist.

### 6.6 Behavior id

**Do NOT allocate now.** Take the next free id from `ck::particles::NumBehaviors` at implementation
time. Note the family question: `NS_ExplosionIceOmni` is this system with **only colour curves and
three scalars changed** (see that sheet), which is a strong candidate for one behavior with a palette
branch rather than two ids.

### 6.7 Known deviations already implied

Same as Ground §6.7 — world→local space, `Opacty_DepthFade` dropped, HDR colour values (up to 5 on
`Smokes`/`SmokesCenter`/`Glow_03`, 3 on `Glow_04`, ×100 on `Glow_03`'s `Scale RGB`, ×1e6 on
`Glow_01001`'s) must survive the colour path unclamped.

---

## 7+. Reserved for implementation.
