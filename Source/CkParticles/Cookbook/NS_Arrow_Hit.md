# Recipe: NS_Arrow_Hit → CkParticles (PLANNING SHEET)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior id is allocated, no `.ush` exists, no look exists, no cadence row exists, no mesh or texture
has been baked, and nothing has ever been rendered. Sections 7+ are reserved for the implementation
session.

**Read [NS_Arrow_Cast.md](NS_Arrow_Cast.md) first.** `NS_Arrow_Hit` is the *same construction* with two
emitters removed, one added, and a dozen numbers changed. This sheet carries the full data but points
at the Cast recipe for the shared material/mesh archaeology rather than duplicating it.

**The two removals matter for scheduling: `NS_Arrow_Hit` has NO Wind layers, therefore NO sub-UV
flipbook and NO tube mesh.** It is materially cheaper than `NS_Arrow_Cast` (§6.7).

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Arrow_Hit` |
| Pack | Vefects — *Anime VFX* |
| Role in the pack | the impact burst of the arrow ability |

Corpus evidence:

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Arrow_Hit.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part01_Bright,Part02,Part04,Rainbow,Ring01,Impact01,Star01,Star02,LightStrip,Flat02}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/{M_VFX_DissolveAdd,M_VFX_FlatAdd}.json`
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_{Spike01,Plane01}.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Part_04,Ring_01,Ring_02,Impact_01,Star_01,Star_02,LightStrip_01,LUT_Rainbow_01,Noise_02,WhitePixel}.json`

**The source Niagara asset was never opened.**

> ### Two systems share this name — take the right one
> `[corpus]` A second `NS_Arrow_Hit` lives at `Vefects/Anime_Stylized_VFX/VFX/Particles/`, also with
> **14 emitters** — emitter count does NOT discriminate them.
>
> **Fastest discriminator: the user-parameter list.** This system's is **empty**. The Stylized
> sibling exposes thirteen: `User.Flare Impact Color 01`, `User.Glow Color 01`–`05`,
> `User.Lightning Strip Color 01`, `User.Rainbow Color 01`, `User.Ring Color 01`,
> `User.Scale Overall`, `User.Sparkles Color 01`, `User.Spike Color 01`, `User.Star Color 01`.
> Second discriminator: the sibling renders through `MI_VFX_*` instances; this one through
> `M_VFX_DisAdd_*`.

---

## 2. System anatomy `[corpus]`

**14 CPU emitters, all enabled, all `LocalSpace: true`, `Determinism: false`, `Bounds: Dynamic`,
no user parameters.** Every emitter: `Life Cycle Mode = System`, a stored Loop Behavior **Infinite** /
Loop Duration **1.0 s**, one `Spawn Burst Instantaneous` with `UseLoopCountLimit = false` (stored
`Loop Count Limit = 1` inert on all 14).

**System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
Per [P0-D1] this RULES all 14 emitters — the stored per-emitter `Infinite / 1.0 s` rows are inert.
*(Was read as a 1.0 s infinite loop.)*

**34 particles per burst. Longest lifetime 0.5 s; last spawn at t = 0.1 s — so the whole effect is
over by t ≈ 0.55 s, well inside the 2.0 s `Once` system loop, and generations NEVER overlap.**

| # | Emitter | Count | Spawn t | Lifetime | Renderer | Alignment / Facing | Material | Size / Scale |
|---|---|---|---|---|---|---|---|---|
| 0 | `Glow_01` | 1 | 0 | 0.1 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | Uniform **1000** |
| 1 | `Glow_02` | 1 | 0 | 0.1 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | Uniform **200** |
| 2 | `Glow_03` | 5 | **0.04** | 0.05 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part02` | Uniform **150** |
| 3 | `Raimbow` (sic) | 1 | 0.05 | 0.1 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Rainbow` | Uniform **200**, rotation random 0–360 |
| 4 | `Sparkles_01` | 5 | 0.05 | **rand 0.2–0.4** | Sprite | **`VelocityAligned`** / `FaceCamera` | `M_VFX_DisAdd_Part04` | rand non-uniform **(35,80)–(50,90)** |
| 5 | `Ring_01` | 1 | 0.05 | 0.5 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Ring01` | Uniform **60**, rotation random 0–360 |
| 6 | `Glow_04` | 5 | 0.05 | 0.2 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | Uniform **1000** |
| 7 | `Glow_05` | **1** | 0.05 | **0.07** | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01_Bright` | Uniform **50** |
| 8 | `FlareImpact` | 1 | 0.05 | 0.05 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Impact01` | Uniform **150** |
| 9 | `Spike01` | 5 | 0.05 | 0.1 | **Mesh** `SM_VFX_Spike01` | **Facing `Velocity`** | `M_VFX_DisAdd_Flat02` (renderer override) | mesh scale rand **(0.1,0.1,0.2)–(0.05,0.2,0.5)** |
| 10 | `LightningStrip` | 5 | 0.05 | **rand 0.1–0.2** | **Mesh** `SM_VFX_Plane01` | **Facing `Velocity`** | `M_VFX_DisAdd_LightStrip` (renderer override) | mesh scale rand **(0.5,0.5,1.5)–(1,1,2)** |
| 11 | `Star01` | 1 | 0.05 | 0.3 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Star01` | Uniform **20** |
| 12 | `Star02` | 1 | **0.1** | 0.3 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Star02` | Uniform **70** |
| 13 | `Ring_02` | 1 | 0.05 | 0.5 | Sprite | **`CustomAlignment` / `CustomFacingVector`** | `M_VFX_DisAdd_Ring01` | Uniform **60**, rotation random 0–360 |

`Sort: None` everywhere. `Position Mode = Simulation Position` everywhere.
`UsePositionOffset` is false on every emitter except `Sparkles_01` and `LightningStrip`, both of which
carry `Position Offset = (0,0,0)` — **no layer is offset from the origin.**

### Delta table vs `NS_Arrow_Cast` `[corpus]`

Emitters that are **byte-identical** between the two systems: `Glow_03`, `Raimbow`, `Sparkles_01`,
`FlareImpact`, `Star01`, `Star02`. `Ring_01` is `NS_Arrow_Cast`'s `Ring` renamed, values unchanged.

| Emitter | Change vs `NS_Arrow_Cast` |
|---|---|
| `Glow_01` | colour curve's t=0 keys change: `G 1 → 0.947307`, `B 1 → 0.665387` (everything else identical) |
| `Glow_02` | size `300 → **200**`; the `Scale Color` module is **replaced by a `Color` module** with a full colour curve (§5) |
| `Glow_04` | `Color.Scale Alpha 0.3 → **0.2**` |
| `Glow_05` | count `3 → **1**`; lifetime `0.1 → **0.07**`; colour curve `G 0.637597 → 0.80876`, `B 0.152926 → 0.553` |
| `Spike01` | mesh scale `min (0.1,0.1,0.1) max (0.3,0.3,0.3)` → `min (0.1,0.1,0.2) max (0.05,0.2,0.5)`; `Scale Mesh Size` curve's 0.2-key `X 0.5 / Y 0.4 / Z 1` → `1.5 / 1.5 / 1.5` |
| `LightningStrip` | `Color.Scale Alpha 0.4 → **0.15**`; mesh-scale random range `min (0.5,0.5,1) max (1.5,1.5,2)` → `min (0.5,0.5,1.5) max (1,1,2)` |
| **`Wind_01`, `Wind_02`** | **REMOVED** — no mesh tube, no sub-UV sprite |
| **`Ring_02`** | **ADDED** — a second ring sprite, custom-aligned rather than billboarded |

> **`Spike01`'s mesh-scale range is inverted on X** in this system: min `0.1` > max `0.05`. That is
> what the corpus records and it is **not** a transcription error — it is copied verbatim. Whether
> Niagara swaps them or produces a degenerate range is `[unresolved]`; the same inversion appears in
> `NS_Gunshot_Hit`'s `Spike01`.

---

## 3. Mesh geometry `[corpus, measured from the .obj]`

Two carrier meshes, both also used by `NS_Arrow_Cast` — full measurements are in
[NS_Arrow_Cast.md](NS_Arrow_Cast.md) §3.1–3.2. Restated in brief so this sheet is usable alone:

- **`SM_VFX_Spike01`** — 16 verts / 6 tris. A square pyramid: base `(±100, ±100, 0)`, apex
  `(0, 0, 200)`; bounds `(-100,-100,0)`→`(100,100,200)`. UV0 has three values only: apex `(0.5, 0)`,
  `+Y` base corners `(0, 1)`, `−Y` base corners `(1, 1)`. `corr(v, z) = -1.000` — **v runs tip(0) →
  base(1)**; u only splits the two halves. Section material slot `"Material"`, asset-level default
  `M_VFX_DisAdd_Slash01` — **overridden at the renderer to `M_VFX_DisAdd_Flat02`**, so the Slash01
  reference must not be ported.
- **`SM_VFX_Plane01`** — 8 verts / 4 tris. A double-sided flat sheet: two coincident quads at
  `y ≈ 0` and `y ≈ -0.0643`, each X `-100..100` × Z `0..200`. `corr(u, x) = 1.000`,
  `corr(v, z) = -1.000` — **u across (−X→+X), v down (Z=200 → Z=0)**. Section slot
  `"WorldGridMaterial"`, asset default `M_VFX_DisAdd_Slash01` — **overridden to
  `M_VFX_DisAdd_LightStrip`**.

**`SM_VFX_Ring01` is NOT used by this system** (it belongs to `Wind_01`, which `NS_Arrow_Hit` does not
have).

---

## 4. Material families and per-instance deltas `[corpus]`

**Two families**, exactly as in `NS_Arrow_Cast` §4 — minus the two Wind instances.

### 4.1 `Parents/M_VFX_DissolveAdd` — 10 instances

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
| `Part01_Bright` | `Glow_05` | `Brightness` **10**; `Core_Intensity` **1**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_Part_02`** |
| `Part02` | `Glow_03` | **`Glow_Intensity` 0.3**; Main/Color/Dissolve tex → **`T_VFX_Part_02`** |
| `Part04` | `Sparkles_01` | `Brightness` **6**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; `Opacty_DepthFade` **30**; Main/Color/Dissolve tex → **`T_VFX_Part_04`** |
| `Rainbow` | `Raimbow` | **`GradientMap_Displacement` 0.9**; **`Gradient_Invert` 2**; **`Opacity_Boldness` 1.5**; **`Opacty_StepAdd` 0.3**; **`GradientMap_Tex` → `T_VFX_LUT_Rainbow_01`**; `GradientShape_Tex` → `T_VFX_Part_01`; `Main_Tex` → **`T_VFX_Ring_02`** |
| `Ring01` | `Ring_01`, **`Ring_02`** | `Brightness` **10**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_Ring_01`** |
| `Impact01` | `FlareImpact` | `Brightness` **12**; **`Core_Power` 0**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_Impact_01`** |
| `Star01` | `Star01` | `Brightness` **6**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; `Opacty_DepthFade` **10**; Main/Color/Dissolve tex → **`T_VFX_Star_01`** |
| `Star02` | `Star02` | `Brightness` **6**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; `Opacty_DepthFade` **10**; Main/Color/Dissolve tex → **`T_VFX_Star_02`** |
| `LightStrip` | `LightningStrip` | `Brightness` **7**; **`Core_Power` 0**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_LightStrip_01`** |

**`Distortion_Intensity` is 0 on every instance in this system** (the two nonzero ones, `Wind01` at
0.5 and `Wind02` at 1, belong to emitters `NS_Arrow_Hit` does not have). **The distortion branch is
entirely dead here**, and `T_VFX_Noise_02` is therefore not required.

### 4.2 `Parents/M_VFX_FlatAdd` — one instance

`M_VFX_DisAdd_Flat02` (the `Spike01` renderer override) is **not** a DissolveAdd instance despite its
name. Parent `Parents/M_VFX_FlatAdd`; `BLEND_Translucent` / `MSM_Unlit` / `MD_Surface`,
`twoSided: false`, outputs `EmissiveColor` + `Opacity`, **no dynamic parameters, no texture
parameters**. Scalars: `Brightness` **10** (parent 1), `Opacty_DepthFade` 0, `CamOffset` 0;
vector `Color_Core = RGBA(1,1,1,0)`. Expression histogram: `ScalarParameter ×3`, `Multiply ×2`,
`DepthFade ×1`, `ParticleColor ×1`, `WorldPosition ×1`, `VectorParameter ×1`,
`MaterialFunctionCall ×1` — i.e. `ParticleColor × Brightness` and nothing else.

### 4.3 Textures `[corpus]` — 512×512, `sRGB: false`, `TC_Alpha` unless noted

| Texture | Format | Address | Consumer | Recreation verdict |
|---|---|---|---|---|
| `T_VFX_Part_01` | `TSF_G8` | Clamp | `Part01`; `Rainbow` Color/Dissolve/GradientShape | **required** — existing stand-in `T_CkParticles_SoftParticle` |
| `T_VFX_Part_02` | `TSF_G8` | Clamp | `Part01_Bright`, `Part02` | **required** — **NEW bake, unmeasured** |
| `T_VFX_Part_04` | `TSF_G16` | Wrap | `Part04` | **required** — existing stand-in `T_CkParticles_SparkStreak` |
| `T_VFX_Ring_01` | `TSF_G16` | Wrap | `Ring01` | **required** — existing `Ring` SDF bake is a candidate |
| `T_VFX_Ring_02` | `TSF_G16` | Wrap | `Rainbow.Main_Tex` | **required** — **NEW bake, unmeasured** |
| `T_VFX_Impact_01` | `TSF_G16` | Wrap | `Impact01` | **required** — **NEW bake, unmeasured** |
| `T_VFX_Star_01` | `TSF_G16` | Wrap | `Star01` | **required** — existing `Flare` bake is a candidate |
| `T_VFX_Star_02` | `TSF_G16` | Wrap | `Star02` | **required** — **NEW bake or a `Flare` parameterization** |
| `T_VFX_LightStrip_01` | `TSF_G16` | Wrap | `LightStrip` | **required** — existing `Streak` bake is a candidate |
| `T_VFX_LUT_Rainbow_01` | **512×2**, `TSF_BGRA8`, `TC_Default`, **sRGB true** | Wrap | `Rainbow.GradientMap_Tex` | **required, a COLOUR LUT not a mask** — §6.7 |
| `T_VFX_Noise_02` | `TSF_G16` | Wrap | every `Distortion_Tex` | **not needed** — `Distortion_Intensity = 0` on every instance here |
| `T_VFX_WhitePixel` | **1×1**, `TSF_RGBA16`, `TC_Default`, sRGB true | Wrap | `GradientMap_Tex` on all but `Rainbow` | not needed — no-op |

**No "candidate" reuse above has been verified against the corpus PNG.** Measure before trusting.

---

## 5. Per-layer runtime curves `[corpus]`

`C` = constant key, `L` = linear key; `t` = NormalizedAge over that layer's own lifetime. Values are
verbatim, including the corpus's own float-noise near-zero endpoints (authored zeros).

> **CURVE-A ("the sparkle ramp")**, shared by `Sparkles_01`, `Ring_01`, `Ring_02`, `FlareImpact`,
> `Spike01`, `Star01`, `Star02`:
> `R: (0, 1)C (0.46725, 1)L (1, 1)C` · `G: (0, 1)C (0.46725, 0.693872)L (1, 0.450786)C` ·
> `B: (0, 1)C (0.46725, 0.147027)L (1, 0.040915)C` · `A: (0.466043, 1)C`

### Layer 0 — `Glow_01` (1, t=0, life 0.1)

- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 0.4)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 0.3`, `Color.Scale Color = (1, 1, 1)`.
- **Color from Curve:** `R: (0, 1)C (0.447932, 1)L (1, 1)C` ·
  **`G: (0, 0.947307)C (0.447932, 0.693872)L (1, 0.450786)C`** ·
  **`B: (0, 0.665387)C (0.447932, 0.147027)L (1, 0.0409152)C`** · `A: (0, 1)C (1, 0)C`
- **Scale Sprite Size** (Uniform Curve): `None: (0, 0.5)C (0.1, 0.9)C (1, 1)C`
- Dynamic params `[dissolve, distortion, offset, core_color] = [1, 0, 0, 0]`.

### Layer 1 — `Glow_02` (1, t=0, life 0.1)

- `InitializeParticle.Color = RGBA(1, 0.899192, 0.548, 1)`, `Color Mode = Direct Set`,
  `Color.Scale Alpha = 1`, `Color.Scale Color = (1, 1, 1)`, size Uniform **200**.
- **Color from Curve** (this replaces the Cast variant's `Scale Color` module):
  `R: (0, 0.318547)C (0.0965892, 1)L (1, 1)C` ·
  `G: (0, 0.955974)C (0.0965892, 1)L (1, 0.863157)C` ·
  `B: (0, 1)C (0.0965892, 1)L (1, 0.38643)C` · `A: (0.143676, 1)C (1, 0)C`
  — a cool blue-white that snaps to pure white by t≈0.097, then warms slightly.
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 1)L (1, 1)L`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 2 — `Glow_03` (5, t=0.04, life 0.05)

- `InitializeParticle.Color = RGBA(0.313989, 0, 0.00227652, 0.483)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`.
- **Color from Curve:** `R: (0, 1)C (1, 1)C` · `G: (0, 1)C (1, 0.450786)C` ·
  `B: (0, 1)C (1, 0.0409152)C` · `A: (0.485361, 1)C`
- **No size curve.** Dynamic params `[0, 0, 0, 0]`.

### Layer 3 — `Raimbow` (1, t=0.05, life 0.1)

- `InitializeParticle.Color = RGBA(0.913099, 0.913099, 0.913099, 0.2)`, `Color Mode = Direct Set`,
  `Sprite Rotation Mode = Random` 0–360.
- **`Color` module with NO curve override** — writes `Color.Color = RGBA(1, 1, 1, 1)`, replacing the
  Initialize colour with white.
- **Scale Color** (RGBA Together): `R: (0, 0.5)L` · `G: (0, 0.5)L` · `B: (0, 0.5)L` ·
  `A: (0, 1)L (1, 0)L`
- **Scale Sprite Size**: `None: (0, 0.5)C (0.2, 0.9)C (1, 1)L`
- Dynamic params `[0.5, 0, 0, 0]`.

### Layer 4 — `Sparkles_01` (5, t=0.05, life **random 0.2–0.4**)

- `Lifetime Mode = Random` via `Random Range Float` **min 0.2 / max 0.4**.
- `Sphere Location`: **radius 0.1**, `Random` distribution, `Surface Only = false`,
  `Non Uniform Scale = (1,1,1)`, `Offset = (0,0,0)` — a point source.
- `Add Velocity from Point`: `Velocity Strength = Random Range Float 001` **min 1300 / max 2000**,
  `Origin Offset = (0,0,0)`, `Velocity Falloff Distance = 100`.
- `Sprite Size Mode = Random Non-Uniform`, **min (35, 80) / max (50, 90)**.
- `InitializeParticle.Color = RGBA(1, 1, 1, 1)`; colour = **CURVE-A**.
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (0.2, 0.35)C (1, 0.05)C`
- **Scale Sprite Size** (Uniform): `None: (0, 0)C (0.1, 1)C (1, 0)C`
- **Scale Sprite Size 001** (Non-Uniform): `X: (1, 1)L` · `Y: (0, 1)C (1, 0.6)C`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 5 — `Ring_01` (1, t=0.05, life 0.5)

- `InitializeParticle.Color = RGBA(1, 1, 1, 1)`, `Sprite Rotation Mode = Random` 0–360,
  `Sprite Size Mode = Uniform` **60** (`Uniform Sprite Size Min/Max = 150/160` **inert**).
- Colour = **CURVE-A**.
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, -0.325)C (1, -1)C`. Params 2–4 are `0`.
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 0.9)C (1, 1)C`

### Layer 6 — `Glow_04` (5, t=0.05, life 0.2)

- `InitializeParticle.Color = RGBA(0.313989, 0, 0.00227652, 0.483)`, `Color Mode = Unset`,
  **`Color.Scale Alpha = 0.2`**.
- **Color from Curve:** `R: (0.315122, 1)C (1, 1)C` · `G: (0.315122, 0.664854)C (1, 0.450786)C` ·
  `B: (0.315122, 0.254571)C (1, 0.0409152)C` · `A: (0, 1)C (0.992454, 0)L`
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 1)L (1, 1)L`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 7 — `Glow_05` (**1**, t=0.05, life **0.07**)

- `InitializeParticle.Color = RGBA(0.313989, 0, 0.00227652, 0.483)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`.
- **Color from Curve:** `R: (0.315122, 1)C (1, 1)C` · **`G: (0.315122, 0.80876)C (1, 0.450786)C`** ·
  **`B: (0.315122, 0.553)C (1, 0.0409152)C`** · `A: (0.312708, 1)C (0.992454, 0)L`
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 1)L (1, 1)L`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 8 — `FlareImpact` (1, t=0.05, life 0.05)

- `InitializeParticle.Color = RGBA(0.644888, 0.2, 1, 1)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`. Colour = **CURVE-A**.
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, 0.5)C (1, -1)C`. Params 2–4 are `0`.
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 0.9)C (1, 1)C`

### Layer 9 — `Spike01` (5, t=0.05, life 0.1) — MESH

- `Sphere Location`: **radius 20, `Surface Only = true`**, `Non Uniform Scale = (1,1,1)`.
- `Add Velocity from Point`: `Velocity Strength = 10`, falloff distance 100.
- `Initial Mesh Orientation`: coordinate space **Mesh**, `Orientation Axis = (0, 0, 1)`,
  `Orientation Vector = (1, 0, 0)`, **`Rotation = Random Range Vector` min `(0, 0, 1)` max
  `(0, 0.5, -1)`**.
- `Mesh Scale Mode = Random Non-Uniform`, **min `(0.1, 0.1, 0.2)` / max `(0.05, 0.2, 0.5)`** — the
  X component's min exceeds its max; see the §2 note.
- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 1)`, `Color.Scale Alpha = 1`.
  Colour = **CURVE-A**.
- **Scale Mesh Size** (Scale Float by Curve, per-axis):
  `X: (0, 0)C (0.2, 1.5)C (1, 4.17233e-08)C` ·
  `Y: (0, 0)C (0.2, 1.5)C (1, 5.66244e-08)C` ·
  `Z: (0, 0)C (0.2, 1.5)C`
  — a **uniform** 1.5× pop at t=0.2 (the Cast variant's is anisotropic 0.5 / 0.4 / 1).
- `M_VFX_DisAdd_Flat02` has no dynamic parameters — nothing is written to `Dynamic`.

### Layer 10 — `LightningStrip` (5, t=0.05, life **random 0.1–0.2**) — MESH

- `Lifetime Mode = Random`, `Lifetime Min/Max = 0.1 / 0.2` (Direct-Set `Lifetime = 0.3` **inert**).
- **`Sphere Location` is DISABLED** (radius 50 and `Hemisphere Z = true` never apply) — all five
  particles **spawn at the origin**; `Position Offset = (0,0,0)`, `UsePositionOffset = true`.
- `Add Velocity from Point`: `Velocity Strength = 500`, falloff distance 100. See §6.7 #6 for the
  zero-distance ambiguity.
- `Initial Mesh Orientation`: coordinate space **Mesh**, axis `(0, 0, 1)`, vector `(1, 0, 0)`,
  **`Rotation = Random Range Vector` min `(0, 0, 1)` max `(0, 0.5, -1)`**.
- `Mesh Scale Mode = Non-Uniform` via **`Random Range Vector 001` min `(0.5, 0.5, 1.5)` max
  `(1, 1, 2)`**.
- `InitializeParticle.Color = RGBA(0.341915, 0.184475, 1, 1)`, `Color Mode = Direct Set`,
  **`Color.Scale Alpha = 0.15`**.
- **Color from Curve:** `R: (0, 1)C (0.319952, 1)L (1, 1)C` ·
  `G: (0, 0.913099)C (0.319952, 0.693872)L (1, 0.450786)C` ·
  `B: (0, 0.715694)C (0.319952, 0.147027)L (1, 0.040915)C` · `A: (0.327196, 1)C (1, 0)C`
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`
- **Scale Mesh Size** (Vector from Curve 001):
  `X: (0, 0.5)C (0.2, 1)C (1, -1.44926e-08)C` · `Y: (0, -2.4747e-08)C (1, 1)C` ·
  `Z: (0, -5.68323e-08)C (0.2, 0.75)C (1, 1)C`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 11 — `Star01` (1, t=0.05, life 0.3)

- `InitializeParticle.Color = RGBA(1, 0.637597, 0.152926, 0.2)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`. Colour = **CURVE-A**.
- **Scale Sprite Size**: `None: (0, 0)C (0.4, 1)C (1, 0)C`
- Dynamic params `[1, 0, 0, 0]`.

### Layer 12 — `Star02` (1, t=**0.1**, life 0.3)

- `InitializeParticle.Color = RGBA(1, 0.637597, 0.152926, 0.2)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`. Colour = **CURVE-A**.
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, -0.125)C (1, -1)C`. Params 2–4 are `0`.
- **Scale Sprite Size**: `None: (0, 0)C (0.4, 1)C (1, 0)C`

### Layer 13 — `Ring_02` (1, t=0.05, life 0.5) — **CUSTOM-FACING**

The one emitter with no counterpart in `NS_Arrow_Cast`.

- Renderer: `Alignment = **CustomAlignment**`, `Facing = **CustomFacingVector**` — a quad fixed in sim
  space, not billboarded. This is exactly the pair CkParticles' **VisTag 4** was added for
  (NS_Lightning_Range §8).
- `Align Sprite to Mesh Orientation` module values `[corpus]`:
  `Mesh Orientation Relative Sprite Alignment Vector = (0, 0, 1)` ·
  **`Mesh Orientation Relative Sprite Facing Vector = (0, 10, 0)`** ·
  `Orientation Quaternion = quat(0, 0, 0, 1)` (identity).
  The facing vector's magnitude 10 is not unit — normalize before use `[inferred: Niagara normalizes
  facing vectors; the 10 is an authoring artifact]`.
- `Initial Mesh Orientation`: coordinate space **Mesh**, `Orientation Axis = (1, 0, 0)`,
  `Orientation Vector = (1, 0, 0)`, **`Rotation = (0.25, 0, 0)`** — a fixed quarter-turn about X
  (units appear to be turns, matching this pack's `Random Range Vector` magnitudes `[inferred]`).
- `InitializeParticle.Color = RGBA(1, 1, 1, 1)`, `Sprite Rotation Mode = Random` 0–360,
  `Sprite Size Mode = Uniform` **60** (`Min/Max 150/160` inert).
- Colour = **CURVE-A**.
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, -0.325)C (1, -1)C`. Params 2–4 are `0`.
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 0.9)C (1, 1)C`

**`Ring_01` and `Ring_02` are the same ring drawn twice** — same material, same size, same colour
curve, same dissolve curve — with one billboarded and one locked to a plane. That pairing is the whole
point of the emitter and must survive the port.

### Inert values recorded so they are not implemented `[corpus]`

- `Sprite Rotation Mode = Unset` on layers 0, 1, 2, 4, 6, 7, 8, 9, 10, 11, 12 → their
  `Sprite Rotation Angle 90 / Min 0 / Max 360` never apply. Only `Raimbow`, `Ring_01`, `Ring_02`
  randomize rotation.
- `Lifetime Mode = Direct Set` on every layer except `Sparkles_01` and `LightningStrip`.
- `Sprite Size Mode = Uniform` on layers 0, 1, 2, 3, 5, 6, 7, 8, 11, 12, 13 → their `Sprite Size`
  pairs and `Sprite Size Min/Max` never apply.
- `Clamp Velocity = false`, `Limit Acceleration = false` everywhere → `Speed Limit 1000` /
  `Acceleration Limit 9999` never bind.
- `LightningStrip`'s `Sphere Location` module (and its `Hemisphere Z` override) is **DISABLED**.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**New row `[corpus-v3]`, per [P0-D3]: loop 2.0 s, particle lifetime 0.5 s, burst 34.**
Loop = the system's `Once` loop duration (*was 1.0 s, taken from the inert emitter rows*); lifetime =
max resolved emitter lifetime (0.5 s); burst = the §2 count.

Lifetime 0.5 s covers the longest source layer (`Ring_01` / `Ring_02`), and **the last layer dies at
t ≈ 0.55 s, well inside the 2.0 s loop, so generations never overlap** — a materially simpler cadence
than the Cast variant's.

`PS_CkParticles_Template_Slash` is **1.0 / 0.5 / 19** — same lifetime, different loop and burst; a
`2.0 / 0.5 / 34` row is a near sibling of it, not a novel shape.

Layer index = **`Seed % 34`**. Partition:

| Layer band | Source emitter | Count |
|---|---|---|
| 0 | `Glow_01` | 1 |
| 1 | `Glow_02` | 1 |
| 2–6 | `Glow_03` | 5 |
| 7 | `Raimbow` | 1 |
| 8–12 | `Sparkles_01` | 5 |
| 13 | `Ring_01` | 1 |
| 14–18 | `Glow_04` | 5 |
| 19 | `Glow_05` | 1 |
| 20 | `FlareImpact` | 1 |
| 21–25 | `Spike01` | 5 |
| 26–30 | `LightningStrip` | 5 |
| 31 | `Star01` | 1 |
| 32 | `Star02` | 1 |
| 33 | `Ring_02` | 1 |

Per-layer spawn delay (0 / 0.04 / 0.05 / 0.1 s) handled as in NS_BasicAttack: hide while
`Age < delay`, run the curves on `(Age - delay) / layerLifetime`.

### 6.2 Renderers / VisTag

| Kind | Look | Layers |
|---|---|---|
| camera sprite `[needs a new kind — §6.7 #1]` | `PartDisAdd01` | 0, 1, 14–18 |
| camera sprite | `PartDisAdd02` | 2–6 |
| camera sprite | `RainbowDisAdd` | 7 |
| `VelocityAlignedSprite` *(kind exists)* | `PartDisAdd04` *(look exists)* | 8–12 |
| camera sprite | `RingDisAdd01` | 13 |
| camera sprite | `PartDisAdd01Bright` | 19 |
| camera sprite | `ImpactDisAdd01` | 20 |
| `Mesh` `Spike` | `FlatAdd02` | 21–25 |
| `Mesh` `Sheet` | `LightStripDisAdd` | 26–30 |
| camera sprite | `StarDisAdd01` | 31 |
| camera sprite | `StarDisAdd02` | 32 |
| **shared VisTag 4** (custom-facing sprite) | `RingDisAdd01` via `User.SpriteMaterial` | 33 |

Layer 33 is the one layer this system can draw through the **existing shared renderer set**: VisTag 4
already implements `CustomAlignment` + `CustomFacingVector` and shares `User.SpriteMaterial` with
VisTag 0. But VisTag 4's material comes from `Get_BehaviorLookName`, and this behavior needs eleven
other looks on row renderers — so binding `RingDisAdd01` there is possible *and* means
`Get_BehaviorLookName` is **not** `NAME_None` for this behavior. That combination (row renderers **and**
a bound look) has never been exercised; verify it before relying on it, or row-declare a
custom-facing renderer instead once §6.7 #1 adds sprite kinds to the row spec.

VisTags allocate above `SharedRendererVisTag_Max`, read via `Get_RosterVisTag_Max()` — never a literal.

### 6.3 CkUsf looks

**One new family + ten new looks.** Identical set to `NS_Arrow_Cast` §6.3 **minus** `WindDisAdd01`
and `WindDisAdd02Mesh`:

- **`FlatAdd` family (NEW)** — `EmissiveColor = ParticleColor.rgb × Brightness`,
  `Opacity = ParticleColor.a`. One look `FlatAdd02`, `Brightness = 10`.
- **`DissolveAdd` family** — `PartDisAdd01`, `PartDisAdd02`, `PartDisAdd01Bright`, `RainbowDisAdd`,
  `RingDisAdd01`, `ImpactDisAdd01`, `StarDisAdd01`, `StarDisAdd02`, `LightStripDisAdd`
  (`PartDisAdd04` exists).

If `NS_Arrow_Cast` is implemented first, **nine of the ten looks are already done** and this recipe
adds none. That sequencing is worth taking.

### 6.4 Mesh / texture needs

**Two new procedural meshes** (both shared with `NS_Arrow_Cast`):
`SM_CkParticles_Spike` (square pyramid per §3) and `SM_CkParticles_Sheet` (single flat quad,
`_TwoSided` look instead of the doubled sheet). **No tube** — `SM_VFX_Ring01` is not used here.

**Textures:** four new bakes (`T_VFX_Part_02`, `T_VFX_Ring_02`, `T_VFX_Impact_01`, `T_VFX_Star_02`)
plus the `T_VFX_LUT_Rainbow_01` colour ramp; four candidate reuses (`SoftParticle`, `SparkStreak`,
`Ring`, `Flare`, `Streak`) that **must be measured before being trusted**. One fewer new bake than
`NS_Arrow_Cast` (no `T_VFX_Wind_01` sub-UV sheet).

### 6.5 Behavior id

**Do not allocate now.** `ck::particles::NumBehaviors` read 18 at the time of writing; bump whatever it
reads at implementation time.

### 6.6 Stage outputs

`Position`, `Velocity`, `Color`, `Size`, `Scale` + `Orientation` (mesh layers 21–30), `Dynamic`,
`Rotation` (layers 7, 13, 33), and — for layer 33 only — **`SpriteAlignment` `(0,0,1)` and
`SpriteFacing` `(0,1,0)`** (the source's `(0,10,0)` normalized). `MeshIndex` stays 0.

Engine trap to carry forward from NS_Lightning_Range §8: **a missing `Particles.SpriteAlignment`
makes `CustomAlignment` silently fall back to Unaligned**, so both vectors must always be written and
must never be degenerate.

### 6.7 CAPABILITY GAPS — what the current pipeline cannot express

1. **No camera-facing sprite kind on the row-renderer spec.** `ECk_ParticlesRenderer_Kind` is
   `{ Mesh, VelocityAlignedSprite }`; ten of this system's twelve sprite layers are
   `Unaligned` / `FaceCamera` needing *different* looks, and `User.SpriteMaterial` carries one.
   **A `CameraSprite` kind must be added** (small, additive) or the effect cannot be drawn correctly.
   Same gap as `NS_Arrow_Cast` #1 — fix it once for the whole batch.
2. **Mesh renderer `Facing: Velocity` is not expressible on a row spec.** `Spike01` and
   `LightningStrip` use it. Workaround: the behavior authors the velocity, so it can write an
   `Orientation` quaternion carrying the mesh's local axis onto the velocity direction and leave the
   renderer at `Default`. Math, not a pipeline change — but do it deliberately.
3. **The gradient-map LUT chain is not implemented and is LOAD-BEARING here.**
   `M_VFX_DisAdd_Rainbow` uses a **512×2 BGRA8 sRGB colour ramp** with
   `GradientMap_Displacement 0.9` and `Gradient_Invert 2`. Prior recipes could omit the chain because
   their gradient map was a white pixel; that argument does not transfer. Needs a family parameter,
   a **colour** texture (the first non-greyscale one in the CkParticles library), and shader work —
   or the rainbow layer is dropped as a recorded deviation (1 of 34 particles).
4. **Five DissolveAdd parameters this system drives are not in the family signature:**
   `Glow_Intensity` (**0.3** on `Part02`), `Core_Intensity` (**1** on `Part01_Bright`),
   `Core_Power` (**0** on `Impact01` and `LightStrip`), `Gradient_Invert` (0 / 2),
   `Opacty_StepAdd` (**0.3** on `Rainbow`). Several are NOT at inert values.
5. **`Opacty_DepthFade` (10 / 20 / 30) is not wired.** Known, pre-existing gap.
6. **`[unresolved]` — `Add Velocity from Point` at zero offset.** `LightningStrip` spawns all five at
   the origin (its `Sphere Location` is disabled) and then applies a from-point velocity of strength
   500 from that same origin. The direction Niagara produces at zero distance is not recoverable from
   the corpus. Resolve by observation before implementing.
7. **`[unresolved]` — `Spike01`'s inverted X mesh-scale range** (min 0.1 > max 0.05). Copied verbatim
   from the corpus; whether Niagara swaps, clamps, or degenerates is not recorded.
8. **A second material family (`FlatAdd`) must be authored** — not a capability gap, but the first
   time the cookbook ships two families, and `M_VFX_DisAdd_Flat02` is a naming trap (it is *not*
   DisAdd).
9. **Local space matches** the CkParticles template on every emitter — **no gap**.

**Not needed by this system, unlike `NS_Arrow_Cast`: sub-UV flipbooks, a tube mesh, renderer-level
mesh scale.** Nothing here needs ribbons, GPU simulation, collision, events, light renderers, or user
parameters.

---

## 7+. Reserved for implementation
