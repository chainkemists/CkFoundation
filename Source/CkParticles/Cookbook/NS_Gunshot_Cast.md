# Recipe: NS_Gunshot_Cast → CkParticles (PLANNING SHEET)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior id is allocated, no `.ush` exists, no look exists, no cadence row exists, no mesh or texture
has been baked, and nothing has ever been rendered. Sections 7+ are reserved for the implementation
session.

**This system carries the batch's two hardest capability gaps: THREE sub-UV flipbook emitters, and one
emitter whose Loop Behavior is `Once` with its own 0.3 s loop duration while the other thirteen loop
infinitely at 1.0 s.** Neither is expressible today. Read §6.7 before estimating.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Gunshot_Cast` |
| Pack | Vefects — *Anime VFX* |
| Role in the pack | the muzzle flash (paired with `NS_Gunshot_Projectile` and `NS_Gunshot_Hit`) |

Corpus evidence:

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Gunshot_Cast.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part01_Bright,Part02,Part03_Bright,Part04,Star01,Star02,Wind01,Wind02,LightStrip,Impact02,Flat02}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/{M_VFX_DissolveAdd,M_VFX_FlatAdd}.json`
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_{Spike01,Ring01}.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Part_03,Part_04,Star_01,Star_02,Wind_01,Wind_02,Impact_02,LightStrip_01,Noise_02,WhitePixel}.json`

**The source Niagara asset was never opened.**

> ### Two systems share this name — take the right one
> `[corpus]` A second `NS_Gunshot_Cast` lives at `Vefects/Anime_Stylized_VFX/VFX/Particles/`, with
> **14** emitters against this one's **15** — close enough that emitter count is a poor test.
>
> **Fastest discriminator: the user-parameter list.** This system's is **empty**. The Stylized
> sibling exposes thirteen: `User.Glow Color 01`–`05`, `User.Impact Color 01`,
> `User.Lightning Strip Color 01`, `User.Scale Overall`, `User.Sparkles Color 01`,
> `User.Sparkles Color 02`, `User.Spike Color 01`, `User.Star Color 01`, `User.Wind Color 01`.
> Second discriminator: the sibling renders through `MI_VFX_*` instances
> (`MI_VFX_Glow_03_Bright`, `MI_VFX_Impact_02`, …); this one through `M_VFX_DisAdd_*`.

---

## 2. System anatomy `[corpus]`

**15 CPU emitters (14 enabled, 1 disabled), all `LocalSpace: true`, `Determinism: false`,
`Bounds: Dynamic`, no user parameters.** Fourteen emitters are `Life Cycle Mode = System` and store
Loop Behavior **Infinite** / **Loop Duration 1.0 s** — all inert per [P0-D1]. **`Sparkles_01` is the
one `Life Cycle Mode = Self` emitter**, so its `Loop Behavior = Once` / `Loop Duration 0.3 s` is
LIVE. `UseLoopCountLimit = false` everywhere, so every stored `Loop Count Limit = 1` is inert.

**System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**

**33 particles per firing, plus the 7 one-shot `Sparkles_01`. Longest lifetime 1.5 s from a spawn at
t = 0.05 → over by t ≈ 1.55 s, inside the 2.0 s `Once` loop, so generations do NOT overlap.**
*(Was "1.5 s > the 1.0 s loop, so the three Wind layers overlap the next loop" — an artefact of the
inert emitter rows.)*

| # | Emitter | Count | Spawn t | Lifetime | Renderer | Alignment / Facing | Material | Size / Scale |
|---|---|---|---|---|---|---|---|---|
| 0 | `Glow_01` | 1 | 0 | 0.1 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | Uniform **700** |
| 1 | `Glow_02` | 1 | 0 | 0.1 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | Uniform **300** |
| 2 | `Glow_03` | 5 | **0.04** | 0.05 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part02` | Uniform **150** |
| 3 | `Sparkles_02` | 3 | 0.05 | **rand 0.1–0.2** | Sprite | **`VelocityAligned`** | `M_VFX_DisAdd_Part04` | rand non-uniform **(20,130)–(25,150)** |
| 4 | `Glow_04` | 5 | 0.05 | 0.2 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | Uniform **800** |
| 5 | `Glow_05` | 3 | 0.05 | 0.1 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part03_Bright` | Uniform **250** |
| 6 | `Spike01` | 3 | 0.05 | **rand 0.1–0.15** | **Mesh** `SM_VFX_Spike01` | **Facing `Velocity`** | `M_VFX_DisAdd_Flat02` (renderer override) | mesh scale rand **(0.2,0.2,0.4)–(0.2,0.2,0.7)** |
| 7 | `LightningStrip` | 1 | 0.05 | **rand 0.1–0.2** | **Sprite** (not a mesh — see below) | **`VelocityAligned`** | `M_VFX_DisAdd_LightStrip` | Non-Uniform **(100, 800)**, offset **(354.572, 0, 0)** |
| 8 | `Star01` | 1 | 0.05 | 0.2 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Star01` | Uniform **20** |
| 9 | `Star02` **(DISABLED)** | 1 | 0.1 | 0.3 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Star02` | Uniform **70** |
| 10 | `Wind_01` | 1 | 0.05 | **1.5** | **Mesh** `SM_VFX_Ring01` (renderer mesh scale **(1,1,5)**) | **Facing `Default`** | `M_VFX_DisAdd_Wind02` (renderer override) | mesh uniform scale **0.3** |
| 11 | `Wind_02` | 6 | 0.05 | **1.5** | Sprite, **SubUV 2×2** | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Wind01` | rand uniform **130–230**, rotation random 0–360 |
| 12 | `Sparkles_01` | 7 | 0.05 | **rand 0.3–0.6** `[corpus-v3]` | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01_Bright` | rand uniform **6–10** |
| 13 | `Wind_03` | 2 | 0.05 | **1.5** | Sprite, **SubUV 2×2** | **`VelocityAligned`** | `M_VFX_DisAdd_Wind01` | rand non-uniform **(60,400)–(80,500)**, offset **(70, 0, 0)** |
| 14 | `Impact_01` | 1 | 0.05 | 0.2 | Sprite, **SubUV 2×2** | **`VelocityAligned`** | `M_VFX_DisAdd_Impact02` | rand non-uniform **(100,170)–(120,200)**, offset **(132.846, 0, 0)**, rotation random 0–360 |

`Sort: None` everywhere. `Position Mode = Simulation Position` everywhere.

**`Star02` is DISABLED and is listed so its absence from the recreation is a recorded decision, not an
oversight.** Its authored values are transcribed in §5 for completeness.

**`LightningStrip` is a SPRITE here, a MESH in the Arrow systems.** Same material name
(`M_VFX_DisAdd_LightStrip`), same idea, entirely different renderer: `NS_Arrow_Cast`/`NS_Arrow_Hit`
draw `SM_VFX_Plane01` with `Facing: Velocity`; this one draws a `(100, 800)` velocity-aligned sprite
offset 354.572 units down +X. **Do not assume the Arrow port's carrier transfers.**

**Three emitters offset along +X**: `LightningStrip` 354.572, `Impact_01` 132.846, `Wind_03` 70. Every
other emitter has `UsePositionOffset = false` (or a zero offset). The pipeline's existing
`MuzzleFlash` / `Tracer` convention is already "forward = +X", so the axis agrees.

**`Sparkles_01` is a one-shot inside a looping system.** `Loop Behavior = Once`,
`Loop Duration = 0.3 s`, 7 particles at t = 0.05. On a component that is spawned per shot, it fires
once; on a looping preview, everything else re-fires every second and it does not. §6.7 #2.

---

## 3. Mesh geometry `[corpus, measured from the .obj]`

Two carrier meshes. Both declare `M_VFX_DisAdd_Slash01` as their asset-level section material and both
are **overridden at the renderer** — that Slash01 reference is a red herring and must not be ported.

### 3.1 `SM_VFX_Spike01` — 16 verts / 6 tris / 2 UV sets

A **square pyramid**: base `(±100, ±100, 0)`, apex `(0, 0, 200)`; bounds
`(-100,-100,0)` → `(100,100,200)`, size `(200,200,200)`. Six triangles = four sides + a two-triangle
base quad. Section material slot `"Material"`.

UV0 is a three-value layout: apex `(0.5, 0.0)`; `+Y` base corners `(0.0, 1.0)`; `−Y` base corners
`(1.0, 1.0)`. Measured `corr(v, z) = -1.000`, `corr(v, radius) = +1.000`, `corr(u, angle) = -0.894`
— **v runs tip(0) → base(1)**; u only distinguishes the two halves. Since `M_VFX_DisAdd_Flat02`
samples **no texture at all** (§4.2), the UV is irrelevant for this system — recorded because the Hit
systems reuse the mesh.

### 3.2 `SM_VFX_Ring01` — 132 verts / 128 tris / 2 UV sets

An **open, thin-walled cylinder**. Bounds `(-100,-100,0)` → `(100,100,50)`. Two coaxial walls at
radius **99.5** and **100.0**, height Z **0..50**, **32 circumferential segments** (33 distinct u
values including the duplicated seam); 32 × 2 walls × 2 triangles = 128. Section slot
`"WorldGridMaterial"`.

UV0, measured (`corr(v, z) = -1.000`, `corr(v, radius) = 0.000`):

- **v = 1 at Z = 0 (bottom), v = 0 at Z = 50 (top)** — only those two values exist.
- **u wraps once around the circumference and DECREASES with increasing polar angle** (clockwise from
  +Z), in 1/32 steps. Anchors: `u = 0.75` at 0°, `0.5` at +90°, `0.25` at ±180°, seam `u = 0/1` at
  **−90°**. Closed form: **`u = frac(0.75 - angle_deg / 360)`**.

**The renderer applies mesh scale `(1, 1, 5)`** on top of `Particles.Scale`, so the drawn tube is
100-radius × **250** tall before the emitter's uniform 0.3 and its `Scale Mesh Size` curve. Final Z
extent at t=1 is `50 × 5 × 0.3 × 3 = 225` units `[corpus arithmetic; the Z curve peaks at 3 here, not
the Arrow variant's 5]`.

**`SM_VFX_Plane01` is NOT used by this system** — its `LightningStrip` is a sprite.

---

## 4. Material families and per-instance deltas `[corpus]`

### 4.1 `Parents/M_VFX_DissolveAdd` — 10 instances (one on the disabled emitter)

Base properties on every instance: `MD_Surface`, **`BLEND_Translucent`**, **`MSM_Unlit`**,
`twoSided: false`, outputs **`EmissiveColor` + `Opacity`** only, dynamic channels
**`dissolve`, `distortion`, `offset`, `core_color`**.

Reference row `M_VFX_DisAdd_Part01` (family defaults): `Brightness 1`, `Opacity_Boldness 0.5`,
`Glow_Intensity 1`, `Core_Power 1`, `Core_Intensity 0`, `Gradient_Invert 0.5`,
`GradientMap_Displacement 0.1`, `Opacty_Step 0`, `Opacty_StepAdd 0.1`, `Opacty_DepthFade 20`,
`CamOffset 0`, `Dissolve 0`, `Dissolve_Invert 0`, `Dissolve_Scale_X/Y 1`, `Dissolve_Speed_X/Y 0`,
`Dissolve_Offset_X/Y 0`, `Distortion_Intensity 0`, `Distortion_Scale_X/Y 1`, `Distortion_Speed_X/Y 0`,
`MainTex_Scale_X/Y 1`, `MainTex_Speed_X/Y 0`, `MainTex_Offset_X/Y 0`, `Color_Scale_X/Y 1`,
`Color_Speed_X/Y 0`, `Color_Offset_X/Y 0`, `GradientShape_Scale_X/Y 1`, `GradientShape_Speed_X/Y 0`,
`Color_Core = RGBA(1,1,1,0)`; `Main_Tex`/`Color_Tex`/`Dissolve_Tex = T_VFX_Part_01`,
`Distortion_Tex`/`GradientShape_Tex = T_VFX_Noise_02`, `GradientMap_Tex = T_VFX_WhitePixel`.

| Material | Used by | Deltas vs `Part01` |
|---|---|---|
| `Part01` (ref) | `Glow_01`, `Glow_02`, `Glow_04` | — |
| `Part01_Bright` | `Sparkles_01` | `Brightness` **10**; `Core_Intensity` **1**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_Part_02`** |
| `Part02` | `Glow_03` | **`Glow_Intensity` 0.3**; Main/Color/Dissolve tex → **`T_VFX_Part_02`** |
| `Part03_Bright` | `Glow_05` | `Brightness` **10**; **`CamOffset` 50**; `Core_Intensity` **1**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_Part_03`** |
| `Part04` | `Sparkles_02` | `Brightness` **6**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; `Opacty_DepthFade` **30**; Main/Color/Dissolve tex → **`T_VFX_Part_04`** |
| `Star01` | `Star01` | `Brightness` **6**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; `Opacty_DepthFade` **10**; Main/Color/Dissolve tex → **`T_VFX_Star_01`** |
| `Star02` | `Star02` *(DISABLED emitter)* | `Brightness` **6**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; `Opacty_DepthFade` **10**; Main/Color/Dissolve tex → **`T_VFX_Star_02`** |
| `Wind01` | `Wind_02`, `Wind_03` | `Brightness` **3**; `Core_Intensity` **1**; **`Dissolve_Speed_Y` −0.15**; **`Distortion_Intensity` 0.5**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; `Main_Tex`/`Color_Tex` → **`T_VFX_Wind_01`**; `Dissolve_Tex` → **`T_VFX_Noise_02`** |
| `Wind02` | `Wind_01` (mesh) | `Brightness` **7**; **`Color_Speed_X` −0.3**; **`Dissolve_Scale_X` 0.7**, **`Dissolve_Scale_Y` 0.95**; **`Dissolve_Speed_X` −0.1**; **`Distortion_Intensity` 1**; **`Distortion_Scale_Y` 0.6**; **`Distortion_Speed_X/Y` 0.1 / 0.1**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_Wind_02`** |
| `LightStrip` | `LightningStrip` | `Brightness` **7**; **`Core_Power` 0**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_LightStrip_01`** |
| `Impact02` | `Impact_01` | `Brightness` **15**; `Core_Intensity` **2**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_Impact_02`** |

**No `Rainbow` instance in this system** — the gradient-map LUT gap that dominates the Arrow recipes
does **not** apply here. That is the one way `NS_Gunshot_Cast` is cheaper than `NS_Arrow_Cast`.

**`CamOffset = 50` on `Part03_Bright` is new.** It is a camera-toward depth offset in the family's
`WorldPosition` chain; every other instance in this batch resolves it to 0. §6.7.

Family expression histogram `[corpus]`: `ScalarParameter ×41`, `Add ×18`, `AppendVector ×18`,
`Multiply ×18`, `Saturate ×12`, `DynamicParameter ×8`, `Reroute ×8`, `TextureSampleParameter2D ×6`,
`Panner ×5`, `TextureCoordinate ×5`, `Constant ×5`, `LinearInterpolate ×4`, `Clamp ×2`, `OneMinus ×2`,
`DepthFade ×1`, `SmoothStep ×1`, `Power ×1`, `ParticleColor ×1`, `WorldPosition ×1`,
`VectorParameter ×1`, `StaticSwitch ×1`, `StaticBoolParameter ×1`, `MaterialFunctionCall ×1`.

### 4.2 `Parents/M_VFX_FlatAdd` — one instance

`M_VFX_DisAdd_Flat02` (the `Spike01` renderer override) is **not** a DissolveAdd instance despite the
name. Parent `Parents/M_VFX_FlatAdd`; `BLEND_Translucent` / `MSM_Unlit` / `MD_Surface`,
`twoSided: false`, outputs `EmissiveColor` + `Opacity`, **no dynamic parameters, no texture
parameters**. Scalars: `Brightness` **10** (parent 1), `Opacty_DepthFade` 0, `CamOffset` 0; vector
`Color_Core = RGBA(1,1,1,0)`. Histogram: `ScalarParameter ×3`, `Multiply ×2`, `DepthFade ×1`,
`ParticleColor ×1`, `WorldPosition ×1`, `VectorParameter ×1`, `MaterialFunctionCall ×1` — i.e.
`ParticleColor × Brightness` and nothing else.

### 4.3 Textures `[corpus]` — 512×512, `sRGB: false`, `TC_Alpha` unless noted

| Texture | Format | Address | Consumer | Recreation verdict |
|---|---|---|---|---|
| `T_VFX_Part_01` | `TSF_G8` | Clamp | `Part01` | **required** — existing stand-in `T_CkParticles_SoftParticle` |
| `T_VFX_Part_02` | `TSF_G8` | Clamp | `Part01_Bright`, `Part02` | **required** — **NEW bake, unmeasured** |
| `T_VFX_Part_03` | `TSF_G8` | **Wrap** | `Part03_Bright` | **required** — **NEW bake, unmeasured** |
| `T_VFX_Part_04` | `TSF_G16` | Wrap | `Part04` | **required** — existing stand-in `T_CkParticles_SparkStreak` |
| `T_VFX_Star_01` | `TSF_G16` | Wrap | `Star01` | **required** — existing `Flare` bake is a candidate |
| `T_VFX_Star_02` | `TSF_G16` | Wrap | `Star02` *(disabled emitter)* | **not required** |
| `T_VFX_Wind_01` | `TSF_G16` | Wrap | `Wind01` | **required, and it is a 2×2 SUB-UV SHEET** — §6.7 #1 |
| `T_VFX_Wind_02` | `TSF_G8` | Wrap | `Wind02` | **required** — existing `WindBand` bake is a candidate |
| `T_VFX_Impact_02` | **`TSF_BGRA8`** `TC_Alpha` | Wrap | `Impact02` | **required, and it is a 2×2 SUB-UV SHEET** — §6.7 #1. Note the **BGRA8 source** against `TC_Alpha` compression: the colour channels are authored but discarded, so it still resolves to a mask `[inferred]` |
| `T_VFX_LightStrip_01` | `TSF_G16` | Wrap | `LightStrip` | **required** — existing `Streak` bake is a candidate |
| `T_VFX_Noise_02` | `TSF_G16` | Wrap | `Wind01.Dissolve_Tex`; every `Distortion_Tex` | **required** — `Distortion_Intensity` is **0.5** on `Wind01` and **1** on `Wind02`, so the distortion branch is LIVE here (unlike the Hit systems). Existing stand-in `T_CkParticles_TileNoise` |
| `T_VFX_WhitePixel` | **1×1**, `TSF_RGBA16`, `TC_Default`, sRGB true | Wrap | `GradientMap_Tex` on every instance | not needed — a no-op gradient map on all of them |

**No "candidate" reuse above has been verified against the corpus PNG.** Measure first
(NS_BasicAttack §7's method).

---

## 5. Per-layer runtime curves `[corpus]`

`C` = constant key, `L` = linear key; `t` = NormalizedAge over that layer's own lifetime. Values are
verbatim, including the corpus's own float-noise near-zero endpoints (authored zeros) and its
**above-1 HDR colour keys**, which are deliberate.

Two curves recur; naming them once avoids re-transcription:

> **CURVE-A ("the sparkle ramp")** — `R: (0, 1)C (0.46725, 1)L (1, 1)C` ·
> `G: (0, 1)C (0.46725, 0.693872)L (1, 0.450786)C` ·
> `B: (0, 1)C (0.46725, 0.147027)L (1, 0.040915)C` · `A: (0.466043, 1)C`
> — used by `Star01` and (disabled) `Star02`.

> **CURVE-W ("the wind envelope")** — RGB single constant keys
> `R: (0, 0.846873)C` · `G: (0, 0.921582)C` · `B: (0, 1)C` ·
> `A: (0, 0)L (0.240266, 1)C (0.676124, 1)L (1, 0)L`
> — a cool blue-white held flat while alpha fades in to t=0.24, holds to t=0.676, then fades out.
> Used by all three Wind layers (`Wind_01`, `Wind_02`, `Wind_03`).

### Layer 0 — `Glow_01` (1, t=0, life 0.1)

- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 0.4)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 0.3`, `Color.Scale Color = (1, 1, 1)`; size Uniform **700**.
- **Color from Curve:** `R: (0, 1)C (0.415334, 1)L (0.810142, 1)C` ·
  `G: (0, 1)C (0.415334, 0.693872)L (0.810142, 0.0409152)C` ·
  `B: (0, 1)C (0.415334, 0.147027)L (0.810142, 0.0451862)C` · `A: (0, 1)C (1, 0)C`
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 0.9)C (1, 1)C`
- Dynamic params `[dissolve, distortion, offset, core_color] = [1, 0, 0, 0]`.

### Layer 1 — `Glow_02` (1, t=0, life 0.1)

- `InitializeParticle.Color = RGBA(1, 0.850349, 0.329, 1)`, `Color Mode = Direct Set`;
  size Uniform **300**.
- **Scale Color** (`Scale Mode = RGBA Together`, Vector4-from-Curve):
  `R: (0, 1)L (1, 1)L` · `G: (0, 1)L (1, 1)L` · `B: (0, 1)L (1, 1)L` · **`A: (0, 1)L (1, 0)L`**
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 1)L (1, 1)L`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 2 — `Glow_03` (5, t=0.04, life 0.05)

- `InitializeParticle.Color = RGBA(0.313989, 0, 0.00227652, 0.483)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`.
- **Color from Curve:** `R: (0, 1)C (0.437066, 1)C` · `G: (0, 1)C (0.437066, 0.945625)C` ·
  `B: (0, 1)C (0.437066, 0.643)C` · `A: (0.485361, 1)C`
  — near-white throughout, cooling only slightly. (The Arrow variant's `Glow_03` goes deep orange.)
- **No size curve.** Dynamic params `[0, 0, 0, 0]`.

### Layer 3 — `Sparkles_02` (3, t=0.05, life **random 0.1–0.2**)

- `Lifetime Mode = Random` via `Random Range Float`; `Lifetime Min/Max = 0.1 / 0.2`.
- `Sphere Location`: **radius 100**, `Non Uniform Scale = (0.1, 0.1, 0.1)`,
  **`Hemisphere X = true`** (an override), `Surface Only = false`, `Offset = (0,0,0)`. Net: a flat
  disc-ish half-volume 10 units thick in Y and Z, 100 in X `[inferred from radius × non-uniform
  scale]`.
- `Add Velocity` = **`Random Range Vector` min `(2000, -100, -100)` max `(7000, 100, 100)`** — a hard
  +X cone.
- `Sprite Size Mode = Random Non-Uniform`, **min (20, 130) / max (25, 150)**.
- `InitializeParticle.Color = RGBA(1, 1, 1, 1)`, `Color.Scale Alpha = 1`.
- **Color from Curve:**
  `R: (0, 1)C (0.297012, 1)L (0.671295, 1)L (0.910353, 0.381326)C` ·
  `G: (0, 0.913099)C (0.297012, 0.493097)L (0.671295, 0.24027)L (0.910353, 0.042927)C` ·
  `B: (0, 0.584079)C (0.297012, 0.0409999)L (0.671295, 0.0839999)L (0.910353, 0.0366073)C` ·
  `A: (0.243888, 1)C (1, 0)L`
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (0.2, 0.35)C (1, 0.05)C`
- **Scale Sprite Size** (Uniform): `None: (0, 0)C (0.1, 1)C (1, 0)C`
- **Scale Sprite Size 001** (Non-Uniform): `X: (1, 1)L` · `Y: (0, 1)C (1, 0.6)C`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 4 — `Glow_04` (5, t=0.05, life 0.2)

- `InitializeParticle.Color = RGBA(0.313989, 0, 0.00227652, 0.483)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 0.3`; size Uniform **800**.
- **Color from Curve:** `R: (0, 1)C (0.414126, 1)L (0.926049, 1)C` ·
  `G: (0, 0.947307)C (0.414126, 0.693872)L (0.926049, 0.450786)C` ·
  `B: (0, 0.665387)C (0.414126, 0.147027)L (0.926049, 0.0409152)C` · `A: (0, 1)C (0.992454, 0)L`
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 1)L (1, 1)L`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 5 — `Glow_05` (3, t=0.05, life 0.1) — **HDR**

- `InitializeParticle.Color = RGBA(0.313989, 0, 0.00227652, 0.483)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`; size Uniform **250**.
- **Color from Curve — the R and G keys exceed 1:**
  **`R: (0.315122, 3)C (1, 1)C`** · **`G: (0.315122, 1.95267)C (1, 0.637597)C`** ·
  `B: (0.315122, 0.552)C (1, 0.152926)C` · `A: (0.312708, 1)C (0.992454, 0)L`
  — a blown-out white-hot core that decays to orange. Combined with `Part03_Bright`'s
  `Brightness 10`, this is the muzzle flash's brightest element.
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 1)L (1, 1)L`
- Dynamic params `[1, 0, 0, 0]`.

### Layer 6 — `Spike01` (3, t=0.05, life **random 0.1–0.15**) — MESH

- `Lifetime Mode = Random`, `Lifetime Min/Max = 0.1 / 0.15` (Direct-Set `Lifetime = 0.1` inert).
- `Sphere Location`: **radius 10**, `Non Uniform Scale = (0.1, 0.1, 0.1)`,
  **`Hemisphere X = true`**, `Surface Only = false`.
- `Add Velocity` = **`Random Range Vector 001` min `(10, -10, -10)` max `(50, 10, 10)`**.
- `Initial Mesh Orientation`: coordinate space **Mesh**, `Orientation Axis = (0, 0, 1)`,
  `Orientation Vector = (1, 0, 0)`, **`Rotation = Random Range Vector` min `(0, 0, 1)` max
  `(0, 0.5, -1)`**.
- `Mesh Scale Mode = Random Non-Uniform`, **min `(0.2, 0.2, 0.4)` / max `(0.2, 0.2, 0.7)`**.
- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 1)`, `Color.Scale Alpha = 1`.
- **Color from Curve:**
  `R: (0, 0.715694)C (0.0941745, 1)L (0.21974, 1)L (0.440688, 0.854993)L (0.707516, 0.0103298)C` ·
  `G: (0, 0.89627)C (0.0941745, 0.752942)L (0.21974, 0.341915)L (0.440688, 0.135633)L (0.707516, 0.00749903)C` ·
  `B: (0, 1)C (0.0941745, 0.109462)L (0.21974, 0.109462)L (0.440688, 0.0512695)L (0.707516, 0.00604883)C` ·
  `A: (0.466043, 1)C`
  — starts cool blue-white, snaps warm by t≈0.094, crushes to near-black by t≈0.708.
- **Scale Mesh Size** (Scale Float by Curve, per-axis):
  `X: (0, 0)C (0.2, 0.5)C (1, 4.17233e-08)C` · `Y: (0, 0)C (0.2, 0.4)C (1, 5.66244e-08)C` ·
  `Z: (0, 0)C (0.2, 1)C`
- `M_VFX_DisAdd_Flat02` has no dynamic parameters — nothing is written to `Dynamic`.

### Layer 7 — `LightningStrip` (1, t=0.05, life **random 0.1–0.2**) — SPRITE

- `Lifetime Mode = Direct Set`? **No — `Lifetime Mode = Direct Set` with `Lifetime = 0.2`, and
  `Lifetime Min/Max = 0.1 / 0.2` are inert** `[corpus]`. Effective lifetime **0.2**.
- `Sprite Size Mode = Non-Uniform` **(100, 800)**; `Position Offset = (354.572, 0, 0)`,
  `UsePositionOffset = true`.
- `Add Velocity = (0.1, 0, 0)`, `Scale Added Velocity = (1,1,1)` — 0.1 units/s, present to give
  `VelocityAligned` its +X axis (the same idiom as the Projectile systems).
- `Mesh Scale = Random Range Vector 001` min `(0.5,0.5,1)` max `(1.5,1.5,2)` — **inert**
  (`Mesh Scale Mode = Unset`, and the renderer is a sprite).
- `InitializeParticle.Color = RGBA(1, 1, 1, 1)`, `Color Mode = Direct Set`,
  **`Color.Scale Alpha = 0.1`**.
- **Color from Curve:** `R: (0.319952, 1)L (1, 1)C` · `G: (0.319952, 0.366253)L (1, 0.450786)C` ·
  `B: (0.319952, 0.184475)L (1, 0.040915)C` · `A: (0.327196, 1)C (1, 0)C`
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`
- **Scale Sprite Size** (`Scale Sprite Size Mode = Non-Uniform Curve`):
  `X: (0, 0.5)C (0.2, 1)C (1, 0.4)C` · `Y: (0.3, 1)C`
  (its uniform companion `None: (0, 0)L (1, 1)L` is unused in Non-Uniform mode)
- Dynamic params `[0, 0, 0, 0]`.

### Layer 8 — `Star01` (1, t=0.05, life 0.2)

- `InitializeParticle.Color = RGBA(1, 0.637597, 0.152926, 0.2)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`; size Uniform **20**. Colour = **CURVE-A**.
- **Scale Sprite Size**: `None: (0, 0)C (0.4, 1)C (1, 0)C`
- Dynamic params `[1, 0, 0, 0]`.

### Layer 9 — `Star02` — **DISABLED, transcribed for the record only**

1 particle at t=0.1, life 0.3, `M_VFX_DisAdd_Star02`, size Uniform 70,
`InitializeParticle.Color = RGBA(1, 0.637597, 0.152926, 0.2)`, colour = **CURVE-A**,
**dynamic param 1 from curve** `None: (0, -0.125)C (1, -1)C`,
**Scale Sprite Size** `None: (0, 0)C (0.4, 1)C (1, 0)C`.
**Not part of the 33-particle count and not to be recreated.**

### Layer 10 — `Wind_01` (1, t=0.05, life **1.5**) — MESH tube

- `Mesh Scale Mode = Uniform` **0.3**.
- `Initial Mesh Orientation`: coordinate space **Mesh**, `Orientation Axis = (1, 0, 0)`,
  `Orientation Vector = (1, 0, 0)`, **`Rotation = (0, 0.25, 0)`** (a fixed quarter-turn about Y;
  units appear to be turns `[inferred from the pack's 0..1 magnitudes]`).
- **`Add Velocity = (150, 0, 0)`** — the tube travels **+X** (the Arrow variant's travels −200 along
  −X; the muzzle blast points forward, the bow's backward).
- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 1)`, `Color Mode = Unset`,
  **`Color.Scale Alpha = 0.02`** — barely visible; this layer is a whisper of haze.
- Colour = **CURVE-W**.
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (0.2, 0.25)C (1, 0.1)C`
- **Scale Mesh Size:** `X: (0, 1.5)C (0.4, 2)C` · `Y: (0, 1.5)C (0.4, 2)C` ·
  **`Z: (0, 0.5)C (0.4, 1.5)C (0.7, 2.625)C (1, 3)C`**
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, -0.2)C (1, -1)C`
- **Dynamic param 2 (`distortion`) from Curve:** **`None: (0, 0)C (0.9, 0.2)C`** — the only
  distortion-channel *curve* in this batch. `M_VFX_DisAdd_Wind02` has `Distortion_Intensity = 1`, so
  it is live. Params 3–4 are `0`.
- **`Update Mesh Orientation`**: `Rotation Coordinate Space = Simulation`, `Rotation Rate = 0.3`,
  `Rotation Vector = (1, 0, 0)` — a continuous spin about world X.

### Layer 11 — `Wind_02` (6, t=0.05, life **1.5**) — SubUV sprite

- `Sprite Size Mode = Random Uniform`, **min 130 / max 230**; `Sprite Rotation Mode = Random` 0–360.
- **`Sub UVAnimation`**: `Start Frame 0`, `End Frame 3`, `SubUV Loop Count 1`, on a **2×2** sheet.
- `Add Velocity = Random Range Vector 001` **min `(200, -20, -20)` max `(1000, 20, 20)`** — a
  **+X** spray (Arrow's is −X).
- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 1)`, `Color Mode = Unset`,
  **`Color.Scale Alpha = 0.15`**. Colour = **CURVE-W**.
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (0.2, 0.15)C (1, 9.74764e-10)C`
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, -5.88215e-08)C (1, -1)C`. Params 2–4 `0`.
- **Scale Sprite Size**: `None: (0, 0.5)C (1, 1)C`

### Layer 12 — `Sparkles_01` (7, t=0.05, life **random 0.3–0.6** `[corpus-v3]`) — **ONE-SHOT**

*Lifetime was misread as 0.2–0.4 under the override-wins assumption; `Lifetime Mode = Random` ⇒
`Lifetime Min 0.3 / Max 0.6` drives ([P0-D2]), the `Random Range Float` override is inert.*

- **`Emitter State`: Loop Behavior `Once`, Loop Duration Mode `Fixed`, Loop Duration `0.3`.**
  Every other emitter in the system is Infinite / 1.0. §6.7 #2.
- `Lifetime Mode = Random` via `Random Range Float` **min 0.2 / max 0.4** (the module's own
  `Lifetime Min/Max = 0.3 / 0.6` are inert — the dynamic input wins).
- `Sphere Location`: **radius 100**, `Non Uniform Scale = (0.1, 0.1, 0.1)`,
  **`Hemisphere X = true`**, `Surface Only = false`.
- `Add Velocity` = **`Random Range Vector` min `(500, -50, -50)` max `(4000, 50, 50)`**.
- `Sprite Size Mode = Random Uniform`, **min 6 / max 10** (the `Sprite Size Min/Max` `(35,25)`/`(50,35)`
  are inert).
- `InitializeParticle.Color = RGBA(1, 1, 1, 1)`, **`Color.Scale Alpha = 0.15`**.
- **Color from Curve:** `R: (0.562632, 1)C (0.997283, 0.112)L` ·
  `G: (0.562632, 0.603828)C (0.997283, 0.0676287)L` ·
  `B: (0.562632, 0.296138)C (0.997283, 0.0331675)L` · `A: (0.562632, 1)L (1, 0)C`
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (**0.1**, 0.15)C (1, -9.09372e-09)C`
  — note the knee is at t=0.1, not the 0.2 every other layer uses.
- **Scale Sprite Size** (Uniform): `None: (0, 0)C (0.1, 1)C (1, 0)C`
- **Dynamic params `[3, 0, 0, 0]`** — `dissolve = 3`, the largest constant dissolve in the batch.

### Layer 13 — `Wind_03` (2, t=0.05, life **1.5**) — SubUV, velocity-aligned

- `Sprite Size Mode = Random Non-Uniform`, **min (60, 400) / max (80, 500)**;
  `Sprite Rotation Mode = Random` 0–360; `Position Offset = (70, 0, 0)`, `UsePositionOffset = true`.
- **`Initial Mesh Orientation` is DISABLED** (its `Random Range Vector` min `(0,0.22,0)` max
  `(0,0.28,0)` never applies).
- **`Sub UVAnimation`**: `Start Frame 0`, **`End Frame 4`**, `SubUV Loop Count 1`, on a **2×2** sheet
  — see the `[unresolved]` note in §6.7 #1.
- `Add Velocity = Random Range Vector 001` **min `(250, -20, -20)` max `(500, 20, 20)`**.
- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 1)`, `Color Mode = Unset`,
  **`Color.Scale Alpha = 0.08`**. Colour = **CURVE-W**.
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (0.2, 0.15)C (1, 9.74764e-10)C`
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, 0.5)C (1, -1)C`. Params 2–4 `0`.
- **Scale Sprite Size**: `None: (0, 0.5)C (1, 1)C`

### Layer 14 — `Impact_01` (1, t=0.05, life 0.2) — SubUV, velocity-aligned, **HDR**

- `Sprite Size Mode = Random Non-Uniform`, **min (100, 170) / max (120, 200)**;
  `Sprite Rotation Mode = Random` 0–360; `Position Offset = (132.846, 0, 0)`,
  `UsePositionOffset = true`.
- **`Initial Mesh Orientation` is DISABLED.**
- **`Sub UVAnimation`**: `Start Frame 0`, **`End Frame 4`**, `SubUV Loop Count 1`, **2×2** sheet.
- `Add Velocity = Random Range Vector 001` **min `(250, -20, -20)` max `(500, 20, 20)`**.
- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 1)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`.
- **Color from Curve — the first three keys are strongly HDR:**
  **`R: (0, 3.57847)C (0.094174, 3)L (0.315122, 1)L (0.447932, 0.854993)L (0.606097, 0.01033)C`** ·
  **`G: (0, 4.48135)C (0.094174, 2.25883)L (0.315122, 0.341915)L (0.447932, 0.135633)L (0.606097, 0.007499)C`** ·
  **`B: (0, 5)C (0.094174, 0.328386)L (0.315122, 0.109462)L (0.447932, 0.051269)L (0.606097, 0.006049)C`** ·
  `A: (0.466043, 1)C`
  — a 5× blue-white flashbulb that collapses to near-black by t≈0.606. With `Impact02`'s
  `Brightness 15` this is the single brightest thing in the system.
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (0.2, 0.15)C (1, 9.74764e-10)C`
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, 1)C (1, -1)C`. Params 2–4 `0`.
- **Scale Sprite Size** (`Non-Uniform Curve` mode): `X: (0, 0.5)C (0.2, 1)C (1, 0.4)C` ·
  `Y: (0.3, 1)C`

### Inert values recorded so they are not implemented `[corpus]`

- `Sprite Rotation Mode = Unset` on layers 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12 → their
  `Sprite Rotation Angle 90 / Min 0 / Max 360` never apply. Only `Wind_02`, `Wind_03` and `Impact_01`
  randomize rotation.
- `Sprite Size Mode = Uniform` on layers 0, 1, 2, 4, 5, 8, 9 → their `Sprite Size` pairs never apply.
- `Lifetime Mode = Direct Set` on every layer except `Sparkles_02`, `Spike01` and `Sparkles_01` → the
  `Lifetime Min/Max` on the others never apply, **including `LightningStrip`'s**.
- `Clamp Velocity = false`, `Limit Acceleration = false` everywhere → `Speed Limit 1000` /
  `Acceleration Limit 9999` never bind.
- `Wind_03` and `Impact_01` carry a **DISABLED** `Initial Mesh Orientation` module.
- `LightningStrip` carries a `Mesh Scale` random range that its `Mesh Scale Mode = Unset` and its
  sprite renderer both ignore.
- Every emitter's `GradientMap_Tex` is `T_VFX_WhitePixel` — the gradient chain is a no-op system-wide.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**New row `[corpus-v3]`, per [P0-D3]: loop 2.0 s, particle lifetime 1.5 s, burst 33.**
Loop = the system's `Once` loop duration (*was 1.0 s, from the inert emitter rows*); lifetime = max
resolved emitter lifetime (the three Wind layers); burst = the §2 count. Every shorter layer hides
past its own lifetime (NS_BasicAttack §8's rule).

**The 7 one-shot `Sparkles_01` particles are NOT in the 33.** Options, decide explicitly:

- **(a)** Fold them in — burst 40. `[corpus-v3]` the re-fire cost is smaller than the sheet assumed:
  the SYSTEM is `Loop Once`, so on a single firing every layer including `Sparkles_01` fires exactly
  once; the deviation only shows on the gym's looping re-arm.
- **(b)** Drop them and record the deviation. Loses the fine gold spark spray.

Either way it is a **recorded deviation, not a silent one** — §6.7 #2.

Layer index = **`Seed % 33`** (or 40 under option (a)). Partition under option (b):

| Layer band | Source emitter | Count |
|---|---|---|
| 0 | `Glow_01` | 1 |
| 1 | `Glow_02` | 1 |
| 2–6 | `Glow_03` | 5 |
| 7–9 | `Sparkles_02` | 3 |
| 10–14 | `Glow_04` | 5 |
| 15–17 | `Glow_05` | 3 |
| 18–20 | `Spike01` | 3 |
| 21 | `LightningStrip` | 1 |
| 22 | `Star01` | 1 |
| 23 | `Wind_01` | 1 |
| 24–29 | `Wind_02` | 6 |
| 30–31 | `Wind_03` | 2 |
| 32 | `Impact_01` | 1 |

Per-layer spawn delay (0 / 0.04 / 0.05 s) handled as in NS_BasicAttack: hide while `Age < delay`, run
the curves on `(Age - delay) / layerLifetime`.

### 6.2 Renderers / VisTag

| Kind | Look | Layers |
|---|---|---|
| camera sprite `[needs a new kind — §6.7 #3]` | `PartDisAdd01` | 0, 1, 10–14 |
| camera sprite | `PartDisAdd02` | 2–6 |
| `VelocityAlignedSprite` *(exists)* | `PartDisAdd04` *(look exists)* | 7–9 |
| camera sprite | `PartDisAdd03Bright` | 15–17 |
| `Mesh` `Spike` | `FlatAdd02` | 18–20 |
| `VelocityAlignedSprite` | `LightStripDisAdd` | 21 |
| camera sprite | `StarDisAdd01` | 22 |
| `Mesh` `Tube` | `WindDisAdd02Mesh` | 23 |
| camera sprite **+ SubUV 2×2** `[not expressible — §6.7 #1]` | `WindDisAdd01` | 24–29 |
| `VelocityAlignedSprite` **+ SubUV 2×2** `[not expressible]` | `WindDisAdd01` | 30–31 |
| `VelocityAlignedSprite` **+ SubUV 2×2** `[not expressible]` | `ImpactDisAdd02` | 32 |

Eleven renderers, **three of them sub-UV**. If §6.2's option (a) is taken, add a twelfth
(camera sprite, `PartDisAdd01Bright`, layers 33–39).

VisTags allocate above `SharedRendererVisTag_Max`, read via `Get_RosterVisTag_Max()` — never a literal.

### 6.3 CkUsf looks

**One new family + eight new looks.**

- **`FlatAdd` family (NEW)** — `EmissiveColor = ParticleColor.rgb × Brightness`,
  `Opacity = ParticleColor.a`; one look `FlatAdd02`, `Brightness = 10`. Shared with all three other
  Arrow/Gunshot Cast/Hit recipes.
- **`DissolveAdd` family** — `PartDisAdd01`, `PartDisAdd02`, `PartDisAdd03Bright`, `StarDisAdd01`,
  `WindDisAdd01`, `WindDisAdd02Mesh`, `LightStripDisAdd`, `ImpactDisAdd02`
  (+ `PartDisAdd01Bright` under option (a)). `PartDisAdd04` already exists.

Naming caution: a look called `WindDisAdd02` **already exists** (NS_BasicAttack, from
`M_VFX_DisAdd_Pan_Wind02`) and is a *different* material from this system's `M_VFX_DisAdd_Wind02`.
Pick a distinct name (`WindDisAdd02Mesh` above).

Overlap with the Arrow recipes: `PartDisAdd01`, `PartDisAdd02`, `StarDisAdd01`, `LightStripDisAdd`,
`WindDisAdd01`, `WindDisAdd02Mesh` and `FlatAdd02` are all shared. Only `PartDisAdd03Bright` and
`ImpactDisAdd02` are unique to the Gunshot systems.

### 6.4 Mesh / texture needs

**Two new procedural meshes** (both shared with other recipes in the batch):

| Generated name | From | Build |
|---|---|---|
| `SM_CkParticles_Spike` | `SM_VFX_Spike01` | square pyramid, base `(±100, ±100, 0)`, apex `(0,0,200)`, 6 tris, UV per §3.1 |
| `SM_CkParticles_Tube` | `SM_VFX_Ring01` | open cylinder r=100, Z `0..50`, 32 segments; `u = frac(0.75 - angle/360)`, `v = 1` at Z=0. Bake the renderer's `(1,1,5)` into the mesh **or** into `Scale` — **never both** |

**No `Sheet` mesh** — this system's `LightningStrip` is a sprite.

**Textures: three new bakes plus two sub-UV sheets.** New and unmeasured: `T_VFX_Part_02`,
`T_VFX_Part_03`, and the two 2×2 sheets `T_VFX_Wind_01` and `T_VFX_Impact_02`. Candidate reuses:
`T_VFX_Part_01` → `SoftParticle`, `T_VFX_Part_04` → `SparkStreak`, `T_VFX_Star_01` → `Flare`,
`T_VFX_Wind_02` → `WindBand`, `T_VFX_LightStrip_01` → `Streak`, `T_VFX_Noise_02` → `TileNoise`.
**Every reuse is a guess until the corpus PNG is measured.**

**The two sub-UV sheets are a different kind of bake**: four sub-images laid out 2×2 in one 512²
texture, each a distinct frame of an animation. The generator has no concept of a sheet layout today.

### 6.5 Behavior id

**Do not allocate now.** `ck::particles::NumBehaviors` read 18 at the time of writing; bump whatever it
reads at implementation time.

### 6.6 Stage outputs

`Position` (offsets + sphere-location spawn + integrated velocity), `Velocity`, `Color`, `Size`,
`Scale` + `Orientation` (mesh layers 18–20, 23), `Dynamic` (including the `distortion` channel curve on
layer 23), `Rotation` (layers 24–32). `MeshIndex` stays 0. `SpriteAlignment` / `SpriteFacing` unused —
no layer is custom-facing.

**A `SubImageIndex` output does not exist and would be required for layers 24–32.**

### 6.7 CAPABILITY GAPS — what the current pipeline cannot express

Items 1–3 are pipeline work, not data edits. This is the heaviest gap list in the batch.

1. **Sub-UV flipbooks are not supported at all, and THREE emitters need them** (`Wind_02`, `Wind_03`,
   `Impact_01` — 9 of 33 particles). `FCkParticles_StageOutput` has no `SubImageIndex` field, the DI
   writes no `Particles.SubImageIndex`, and no renderer declares `SubImageSize`. This needs a DI
   output, a renderer property on the row spec, a texture-generator sheet layout, and a CPU/GPU
   mirror. **Deferral workaround:** bake one representative frame per sheet and accept static puffs
   and a static impact flash — a large, visible fidelity deviation on the muzzle flash's most
   prominent element.
   `[unresolved: Wind_03 and Impact_01 declare `End Frame = 4` on a 2×2 sheet whose valid indices are
   0–3. Whether Niagara clamps, wraps, or samples a fifth non-existent frame is not recoverable from
   the corpus.]`
2. **A per-emitter loop behavior that differs from the system's cannot be expressed.**
   `Sparkles_01` is `Loop Behavior = Once` with its own `Loop Duration = 0.3` while the other thirteen
   emitters are `Infinite` / `1.0`. A CkParticles cadence row is ONE (loop, lifetime, burst) triple
   for the whole template. Options in §6.1; **whichever is chosen must be recorded as a deviation.**
3. **No camera-facing sprite kind on the row-renderer spec.** `ECk_ParticlesRenderer_Kind` is
   `{ Mesh, VelocityAlignedSprite }`; five of this system's layer groups are `Unaligned` /
   `FaceCamera` needing different looks, and `User.SpriteMaterial` carries one. Add a `CameraSprite`
   kind (small, additive) — the same fix all four Cast/Hit recipes in this batch need.
4. **Mesh renderer `Facing: Velocity` is not expressible on a row spec.** `Spike01` uses it
   (`Wind_01` uses `Default`, which is what the row spec already does). Workaround: the behavior
   authors the velocity, so it can write an `Orientation` quaternion carrying the mesh's local axis
   onto the velocity direction. Math, not a pipeline change — but do it deliberately.
5. **Renderer-level mesh scale `(1,1,5)` has no home on the row spec.** Fold it into the generated
   tube or into `Scale`; §6.4.
6. **Five DissolveAdd parameters this system drives are not in the family signature:**
   `Glow_Intensity` (**0.3** on `Part02`), `Core_Intensity` (**1** on `Part01_Bright`/`Wind01`,
   **2** on `Impact02`), `Core_Power` (**0** on `LightStrip`), `Gradient_Invert` (0), `Color_Speed_X`
   (**−0.3** on `Wind02`). Several are **not** at inert values.
7. **`CamOffset = 50` on `M_VFX_DisAdd_Part03_Bright` is not wired.** It is a camera-toward world-position
   offset in the family's `WorldPosition` chain; CkUsf surface looks expose no WorldPositionOffset hook
   on this path. `Glow_05` will sort against nearby geometry differently from the source
   `[inferred from the parameter's name and the `WorldPosition ×1` expression; the exact chain is not
   reconstructible from a histogram]`.
8. **`Opacty_DepthFade` (10 / 20 / 30) is not wired.** Known, pre-existing gap.
9. **HDR colour keys above 1** (`Glow_05` R=3 / G=1.95; `Impact_01` R=3.58 / G=4.48 / B=5) must survive
   the port. `FCkParticles_StageOutput.Color` is a `float4` so the values pass, and the roster test
   deliberately does not assert an alpha upper bound — but any clamp added later would silently flatten
   this system's two brightest layers.
10. **A second material family (`FlatAdd`) must be authored.** Not a capability gap; note the naming
    trap — `M_VFX_DisAdd_Flat02` is **not** in the DisAdd family.
11. **Local space matches** the CkParticles template on every emitter — **no gap**.

**No gradient-map LUT gap here** (no `Rainbow` instance), unlike the two Arrow recipes.
Nothing here needs ribbons, GPU simulation, collision, events, light renderers, or user parameters.

---

## 7+. Reserved for implementation
