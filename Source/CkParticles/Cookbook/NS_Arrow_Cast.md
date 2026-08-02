# Recipe: NS_Arrow_Cast → CkParticles (PLANNING SHEET)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior id is allocated, no `.ush` exists, no look exists, no cadence row exists, no mesh or texture
has been baked, and nothing has ever been rendered. Every section below is archaeology out of the
extracted corpus plus a translation *plan*. Sections 7+ are reserved for the implementation session.

**This is the largest system in the Arrow/Gunshot batch: 15 emitters, 42 particles per loop, three
renderer kinds, and at least three capability gaps that do not exist in the pipeline today
(§6.7).** Read §6.7 before estimating it.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Arrow_Cast` |
| Pack | Vefects — *Anime VFX* |
| Role in the pack | the cast/charge flash at the bow (paired with `NS_Arrow_Projectile`, `NS_Arrow_Hit`) |

Corpus evidence:

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Arrow_Cast.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part01_Bright,Part02,Part04,Rainbow,Ring01,Impact01,Star01,Star02,Wind01,Wind02,LightStrip,Flat02}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/{M_VFX_DissolveAdd,M_VFX_FlatAdd}.json`
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_{Spike01,Plane01,Ring01}.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Part_04,Ring_01,Ring_02,Impact_01,Star_01,Star_02,Wind_01,Wind_02,LightStrip_01,LUT_Rainbow_01,Noise_02,WhitePixel}.json`

**The source Niagara asset was never opened.**

> ### Two systems share this name — take the right one
> `[corpus]` A second `NS_Arrow_Cast` lives at `Vefects/Anime_Stylized_VFX/VFX/Particles/`, also with
> **15 emitters** — so emitter count does NOT discriminate them.
>
> **Fastest discriminator: the user-parameter list.** This system's is **empty**. The Stylized
> sibling exposes fifteen: `User.Flare Impact Color 01`, `User.Glow Color 01`–`05`,
> `User.Lightning Stip Color 01` (sic), `User.Rainbow Color 01`, `User.Ring Color 01`,
> `User.Scale Overall`, `User.Sparkles Color 01`, `User.Spike Color 01`, `User.Star Color 01`,
> `User.Wind Color 01`, `User.Wind Color 02`.
> Second discriminator: the sibling renders through `MI_VFX_*` instances
> (`MI_VFX_Glow_01`, `MI_VFX_Ring_01`, `MI_VFX_Lens_Rainbow_01`, …); this one through
> `M_VFX_DisAdd_*`.
>
> This recipe documents the **`Anime_VFX/Shared/Skills`** variant only.

---

## 2. System anatomy `[corpus]`

**15 CPU emitters, all enabled, all `LocalSpace: true`, `Determinism: false`, `Bounds: Dynamic`,
no user parameters.** Every emitter runs `Emitter State` with Loop Behavior **Infinite**, Loop Duration
Mode **Fixed**, **Loop Duration 1.0 s**, and one `Spawn Burst Instantaneous`
(`UseLoopCountLimit = false`, so the stored `Loop Count Limit = 1` is inert on all 15).

**42 particles per loop. Longest lifetime 1.5 s > the 1.0 s loop, so generations overlap.**

| # | Emitter | Count | Spawn t | Lifetime | Renderer | Alignment / Facing | Material | Size / Scale |
|---|---|---|---|---|---|---|---|---|
| 0 | `Glow_01` | 1 | 0 | 0.1 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | Uniform **1000** |
| 1 | `Glow_02` | 1 | 0 | 0.1 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | Uniform **300** |
| 2 | `Glow_03` | 5 | **0.04** | 0.05 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part02` | Uniform **150** |
| 3 | `Raimbow` (sic) | 1 | 0.05 | 0.1 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Rainbow` | Uniform **200**, rotation random 0–360 |
| 4 | `Sparkles_01` | 5 | 0.05 | **rand 0.2–0.4** | Sprite | **`VelocityAligned`** / `FaceCamera` | `M_VFX_DisAdd_Part04` | rand non-uniform **(35,80)–(50,90)** |
| 5 | `Ring` | 1 | 0.05 | 0.5 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Ring01` | Uniform **60**, rotation random 0–360 |
| 6 | `Glow_04` | 5 | 0.05 | 0.2 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | Uniform **1000** |
| 7 | `Glow_05` | 3 | 0.05 | 0.1 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01_Bright` | Uniform **50** |
| 8 | `FlareImpact` | 1 | 0.05 | 0.05 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Impact01` | Uniform **150** |
| 9 | `Spike01` | 5 | 0.05 | 0.1 | **Mesh** `SM_VFX_Spike01` | **Facing `Velocity`** | `M_VFX_DisAdd_Flat02` (renderer override) | mesh scale rand **(0.1,0.1,0.1)–(0.3,0.3,0.3)** |
| 10 | `LightningStrip` | 5 | 0.05 | **rand 0.1–0.2** | **Mesh** `SM_VFX_Plane01` | **Facing `Velocity`** | `M_VFX_DisAdd_LightStrip` (renderer override) | mesh scale rand **(0.5,0.5,1)–(1.5,1.5,2)** |
| 11 | `Star01` | 1 | 0.05 | 0.3 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Star01` | Uniform **20** |
| 12 | `Star02` | 1 | **0.1** | 0.3 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Star02` | Uniform **70** |
| 13 | `Wind_01` | 1 | 0.05 | **1.5** | **Mesh** `SM_VFX_Ring01` (renderer mesh scale **(1,1,5)**) | **Facing `Default`** | `M_VFX_DisAdd_Wind02` (renderer override) | mesh uniform scale **0.3** |
| 14 | `Wind_02` | 6 | 0.05 | **1.5** | Sprite, **SubUV 2×2** | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Wind01` | rand uniform **130–230**, rotation random 0–360 |

`Sort: None` on every renderer. `Position Mode = Simulation Position` on every emitter.
No emitter is disabled in this system.

**Spawn-time structure.** `Glow_01` and `Glow_02` fire at t=0; `Glow_03` at t=0.04; everything else at
t=0.05 except `Star02` at t=0.10. Within one 1.0 s loop the whole effect is therefore over in
0.05 + 1.5 = 1.55 s, i.e. **the two Wind layers survive past the next loop start** and every other
layer is dead by t ≈ 0.55 s.

**Position offsets.** `UsePositionOffset` is **false** on every emitter except `Sparkles_01` and
`LightningStrip`, and both of those carry `Position Offset = (0,0,0)`. **No layer in this system is
offset from the origin** — a fact worth keeping, because the sibling `NS_Gunshot_Cast` is full of
non-zero offsets.

---

## 3. Mesh geometry `[corpus, measured from the .obj]`

Three carrier meshes. All three declare `M_VFX_DisAdd_Slash01` as their *asset-level* section
material, and **all three are overridden at the renderer** — so that Slash01 reference is a red herring
and must not be ported.

### 3.1 `SM_VFX_Spike01` — 16 verts / 6 tris / 2 UV sets

A **square pyramid**. Bounds `(-100,-100,0)` → `(100,100,200)`, size `(200,200,200)`.
Base is the square `(±100, ±100, 0)`; apex is `(0, 0, 200)`. Six triangles = four side faces plus a
two-triangle base quad. Section material slot `"Material"`.

UV0 covers 0..1 fully and is a **three-value** layout, not a continuous parameterization:

| Vertex | uv0 |
|---|---|
| apex `(0, 0, 200)` | `(0.5, 0.0)` |
| base `(+100, +100, 0)` and `(-100, +100, 0)` | `(0.0, 1.0)` |
| base `(-100, -100, 0)` and `(+100, -100, 0)` | `(1.0, 1.0)` |

Measured correlations: `corr(v, z) = -1.000`, `corr(v, radius) = +1.000`, `corr(u, angle) = -0.894`.
Reading: **v runs along the spike, 0 at the TIP and 1 at the BASE**; **u splits the base by the sign
of Y** (+Y → u=0, −Y → u=1) and is 0.5 at the apex. A texture on this carrier therefore reads as a
tip-to-base gradient in v, with u only distinguishing the two halves. `M_VFX_DisAdd_Flat02` samples no
texture at all (§4), so **for this system the UV is irrelevant** — but it is recorded because
`NS_Gunshot_Cast` and both Hit systems reuse the same mesh.

### 3.2 `SM_VFX_Plane01` — 8 verts / 4 tris / 2 UV sets

A **double-sided flat sheet**: two coincident quads at `y ≈ 0` and `y ≈ -0.0643`, each spanning
X `-100..+100` and Z `0..200`. Bounds `(-100, -0.06429117918014526, 0)` →
`(100, 5.9604644775390625e-06, 200)`. Section material slot `"WorldGridMaterial"`.

UV0: `corr(u, x) = 1.000`, `corr(v, z) = -1.000`. Exactly:

| Vertex | uv0 |
|---|---|
| `x = -100, z = 200` | `(0.0, 0.0)` |
| `x = +100, z = 200` | `(1.0, 0.0)` |
| `x = -100, z = 0` | `(0.0, 1.0)` |
| `x = +100, z = 0` | `(1.0, 1.0)` |

**u runs across the sheet (−X → +X), v runs down it (top Z=200 → bottom Z=0).** The 0.0643-unit
Y separation between the two sheets is 0.03 % of the 200-unit span — it is a doubled sheet for
two-sided rendering, not a volume.

### 3.3 `SM_VFX_Ring01` — 132 verts / 128 tris / 2 UV sets

An **open, thin-walled cylinder (a tube)**. Bounds `(-100,-100,0)` → `(100,100,50)`.
Two coaxial walls at radius **99.5** and **100.0** (0.5 thick), height Z **0..50**, **32
circumferential segments** (33 distinct u values including the duplicated seam).
32 segments × 2 walls × 2 triangles = 128. Section material slot `"WorldGridMaterial"`.

UV0, measured exactly (`corr(v, z) = -1.000`, `corr(v, radius) = 0.000`):

- **v = 1 at Z = 0 (bottom), v = 0 at Z = 50 (top).** Only those two values exist.
- **u wraps once around the circumference and DECREASES with increasing polar angle** (clockwise seen
  from +Z), in 1/32 steps. Anchor values: `u = 0.75` at angle 0°, `u = 0.5` at +90°, `u = 0.25` at
  ±180°, and the **seam u = 0 / 1 sits at angle −90°**. Closed form:
  **`u = frac(0.75 - angle_deg / 360)`**.

**The renderer applies a mesh scale of `(1, 1, 5)`** on top of `Particles.Scale`, so the drawn tube is
100-radius × **250** tall before the emitter's uniform 0.3 and its `Scale Mesh Size` curve (§5). Final
Z extent at t=1 is `50 × 5 × 0.3 × 5 = 375` units `[corpus arithmetic]`.

---

## 4. Material families and per-instance deltas `[corpus]`

**Two families, not one.**

### 4.1 `Parents/M_VFX_DissolveAdd` — 12 of the 13 instances

Base properties, identical on every instance: `MD_Surface`, **`BLEND_Translucent`**, **`MSM_Unlit`**,
`twoSided: false`, connected outputs **`EmissiveColor` + `Opacity`** only, dynamic-parameter channel
names **`dissolve`, `distortion`, `offset`, `core_color`**.

Reference row `M_VFX_DisAdd_Part01` (the family's own defaults): `Brightness 1`,
`Opacity_Boldness 0.5`, `Glow_Intensity 1`, `Core_Power 1`, `Core_Intensity 0`, `Gradient_Invert 0.5`,
`GradientMap_Displacement 0.1`, `Opacty_Step 0`, `Opacty_StepAdd 0.1`, `Opacty_DepthFade 20`,
`CamOffset 0`, `Dissolve 0`, `Dissolve_Invert 0`, `Dissolve_Scale_X/Y 1`, `Dissolve_Speed_X/Y 0`,
`Dissolve_Offset_X/Y 0`, `Distortion_Intensity 0`, `Distortion_Scale_X/Y 1`, `Distortion_Speed_X/Y 0`,
`MainTex_Scale_X/Y 1`, `MainTex_Speed_X/Y 0`, `MainTex_Offset_X/Y 0`, `Color_Scale_X/Y 1`,
`Color_Speed_X/Y 0`, `Color_Offset_X/Y 0`, `GradientShape_Scale_X/Y 1`, `GradientShape_Speed_X/Y 0`,
`Color_Core = RGBA(1, 1, 1, 0)`; `Main_Tex`/`Color_Tex`/`Dissolve_Tex = T_VFX_Part_01`,
`Distortion_Tex`/`GradientShape_Tex = T_VFX_Noise_02`, `GradientMap_Tex = T_VFX_WhitePixel`.

| Material | Used by | Deltas vs `Part01` |
|---|---|---|
| `Part01` (ref) | `Glow_01`, `Glow_02`, `Glow_04` | — |
| `Part01_Bright` | `Glow_05` | `Brightness` **10**; `Core_Intensity` **1**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_Part_02`** |
| `Part02` | `Glow_03` | **`Glow_Intensity` 0.3**; Main/Color/Dissolve tex → **`T_VFX_Part_02`** |
| `Part04` | `Sparkles_01` | `Brightness` **6**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; `Opacty_DepthFade` **30**; Main/Color/Dissolve tex → **`T_VFX_Part_04`** |
| `Rainbow` | `Raimbow` | **`GradientMap_Displacement` 0.9**; **`Gradient_Invert` 2**; **`Opacity_Boldness` 1.5**; **`Opacty_StepAdd` 0.3**; **`GradientMap_Tex` → `T_VFX_LUT_Rainbow_01`**; `GradientShape_Tex` → `T_VFX_Part_01`; `Main_Tex` → **`T_VFX_Ring_02`** (Color_Tex and Dissolve_Tex stay `T_VFX_Part_01`) |
| `Ring01` | `Ring` | `Brightness` **10**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_Ring_01`** |
| `Impact01` | `FlareImpact` | `Brightness` **12**; **`Core_Power` 0**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_Impact_01`** |
| `Star01` | `Star01` | `Brightness` **6**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; `Opacty_DepthFade` **10**; Main/Color/Dissolve tex → **`T_VFX_Star_01`** |
| `Star02` | `Star02` | `Brightness` **6**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; `Opacty_DepthFade` **10**; Main/Color/Dissolve tex → **`T_VFX_Star_02`** |
| `Wind01` | `Wind_02` (sprite) | `Brightness` **3**; `Core_Intensity` **1**; **`Dissolve_Speed_Y` −0.15**; **`Distortion_Intensity` 0.5**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; `Main_Tex`/`Color_Tex` → **`T_VFX_Wind_01`**; `Dissolve_Tex` → **`T_VFX_Noise_02`** |
| `Wind02` | `Wind_01` (mesh) | `Brightness` **7**; **`Color_Speed_X` −0.3**; **`Dissolve_Scale_X` 0.7**, **`Dissolve_Scale_Y` 0.95**; **`Dissolve_Speed_X` −0.1**; **`Distortion_Intensity` 1**; **`Distortion_Scale_Y` 0.6**; **`Distortion_Speed_X/Y` 0.1 / 0.1**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_Wind_02`** |
| `LightStrip` | `LightningStrip` | `Brightness` **7**; **`Core_Power` 0**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_LightStrip_01`** |

Family expression histogram `[corpus]`: `ScalarParameter ×41`, `Add ×18`, `AppendVector ×18`,
`Multiply ×18`, `Saturate ×12`, `DynamicParameter ×8`, `Reroute ×8`, `TextureSampleParameter2D ×6`,
`Panner ×5`, `TextureCoordinate ×5`, `Constant ×5`, `LinearInterpolate ×4`, `Clamp ×2`, `OneMinus ×2`,
`DepthFade ×1`, `SmoothStep ×1`, `Power ×1`, `ParticleColor ×1`, `WorldPosition ×1`,
`VectorParameter ×1`, `StaticSwitch ×1`, `StaticBoolParameter ×1`, `MaterialFunctionCall ×1`.

### 4.2 `Parents/M_VFX_FlatAdd` — a SECOND family, one instance

`M_VFX_DisAdd_Flat02` (used by the `Spike01` mesh renderer) is **not** a DissolveAdd instance. Its
parent is `Parents/M_VFX_FlatAdd`, and the two are nearly the same object `[corpus]`:

| | `M_VFX_FlatAdd` (parent) | `M_VFX_DisAdd_Flat02` |
|---|---|---|
| Blend / shading / domain | `BLEND_Translucent` / `MSM_Unlit` / `MD_Surface` | same |
| `twoSided` | false | false |
| Connected outputs | `EmissiveColor`, `Opacity` | same |
| Dynamic parameters | **none** | **none** |
| Texture parameters | **none** | **none** |
| `Brightness` | 1 | **10** |
| `Opacty_DepthFade` | 0 | 0 |
| `CamOffset` | 0 | 0 |
| `Color_Core` | `RGBA(1,1,1,0)` | `RGBA(1,1,1,0)` |
| Expressions | `ScalarParameter ×3`, `Multiply ×2`, `DepthFade ×1`, `ParticleColor ×1`, `WorldPosition ×1`, `VectorParameter ×1`, `MaterialFunctionCall ×1` | identical histogram |

**This is a trivially small shader: `ParticleColor × Brightness` into Emissive, `ParticleColor.a` into
Opacity, plus a depth fade that resolves to 0.** It is a NEW CkUsf look family — the cookbook has only
`DissolveAdd` today — but the cheapest one it will ever add. Note the naming trap: the asset is called
`M_VFX_DisAdd_Flat02` yet is **not** in the DisAdd family. Do not group it by name.

### 4.3 Textures `[corpus]` — all 512×512 unless noted, `sRGB: false` and `TC_Alpha` unless noted

| Texture | Format | Address | Consumer | Recreation verdict |
|---|---|---|---|---|
| `T_VFX_Part_01` | `TSF_G8` | Clamp/Clamp | `Part01`, `Part01_Bright`(no), `Rainbow` (GradientShape + Color + Dissolve) | **required** — existing stand-in `T_CkParticles_SoftParticle` |
| `T_VFX_Part_02` | `TSF_G8` | Clamp/Clamp | `Part01_Bright`, `Part02` | **required** — **NEW bake needed** (no existing stand-in) |
| `T_VFX_Part_04` | `TSF_G16` | Wrap/Wrap | `Part04` | **required** — existing stand-in `T_CkParticles_SparkStreak` |
| `T_VFX_Ring_01` | `TSF_G16` | Wrap/Wrap | `Ring01` | **required** — the existing `Ring` SDF bake is a candidate; verify the line width before reusing |
| `T_VFX_Ring_02` | `TSF_G16` | Wrap/Wrap | `Rainbow.Main_Tex` | **required** — **NEW bake needed** |
| `T_VFX_Impact_01` | `TSF_G16` | Wrap/Wrap | `Impact01` | **required** — **NEW bake needed** |
| `T_VFX_Star_01` | `TSF_G16` | Wrap/Wrap | `Star01` | **required** — the existing `Flare` (star) bake is a candidate |
| `T_VFX_Star_02` | `TSF_G16` | Wrap/Wrap | `Star02` | **required** — **NEW bake or a `Flare` parameterization** |
| `T_VFX_Wind_01` | `TSF_G16` | Wrap/Wrap | `Wind01` | **required, and it is a 2×2 SUB-UV SHEET** (§6.7) — **NEW bake needed** |
| `T_VFX_Wind_02` | `TSF_G8` | Wrap/Wrap | `Wind02` | **required** — the existing `WindBand` bake is a candidate |
| `T_VFX_LightStrip_01` | `TSF_G16` | Wrap/Wrap | `LightStrip` | **required** — the existing `Streak` bake is a candidate |
| `T_VFX_LUT_Rainbow_01` | **512×2**, `TSF_BGRA8`, `TC_Default`, **sRGB true** | Wrap/Wrap | `Rainbow.GradientMap_Tex` | **required and it is a COLOUR LUT, not a mask** — see §6.7 |
| `T_VFX_Noise_02` | `TSF_G16` | Wrap/Wrap | every instance's `Distortion_Tex`/`GradientShape_Tex`; `Wind01.Dissolve_Tex` | **required for `Wind01` only** (`Distortion_Intensity 0.5`); existing stand-in `T_CkParticles_TileNoise` |
| `T_VFX_WhitePixel` | **1×1**, `TSF_RGBA16`, `TC_Default`, sRGB true | Wrap/Wrap | `GradientMap_Tex` on all but `Rainbow` | not needed — a no-op gradient map |

**None of the "NEW bake needed" rows has been measured.** The measurement pass (structure tensor,
32-bin per-axis profiles, zero-crossing counts, radial ring means — NS_BasicAttack §7's method) is
implementation work and has not been done. Do not assume a bake is cheap until its PNG is measured.

---

## 5. Per-layer runtime curves `[corpus]`

Conventions as in the other recipes: `C` = constant key, `L` = linear key; `t` = NormalizedAge over
that layer's own lifetime. Curve keys are transcribed verbatim from the corpus, including its own
precision and its float-noise near-zero endpoints (e.g. `4.17233e-08`) — those are authored zeros.

Five colour curves recur across many emitters. Naming them once avoids ten transcriptions:

> **CURVE-A ("the sparkle ramp")** — `R: (0, 1)C (0.46725, 1)L (1, 1)C` ·
> `G: (0, 1)C (0.46725, 0.693872)L (1, 0.450786)C` ·
> `B: (0, 1)C (0.46725, 0.147027)L (1, 0.040915)C` · `A: (0.466043, 1)C`
> — white → warm orange, alpha a single constant key at 1.
> Used by: `Sparkles_01`, `Ring`, `FlareImpact`, `Spike01`, `Star01`, `Star02`.

### Layer 0 — `Glow_01` (1 particle, t=0, life 0.1)

- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 0.4)`, `Color Mode = Unset`
  (so the update curve is the only colour source), `Color.Scale Alpha = 0.3`,
  `Color.Scale Color = (1, 1, 1)`.
- **Color from Curve:** `R: (0, 1)C (0.447932, 1)L (1, 1)C` ·
  `G: (0, 1)C (0.447932, 0.693872)L (1, 0.450786)C` ·
  `B: (0, 1)C (0.447932, 0.147027)L (1, 0.0409152)C` · `A: (0, 1)C (1, 0)C`
- **Scale Sprite Size** (Uniform Curve): `None: (0, 0.5)C (0.1, 0.9)C (1, 1)C`
- Dynamic params `[dissolve, distortion, offset, core_color] = [1, 0, 0, 0]` (constants).

### Layer 1 — `Glow_02` (1, t=0, life 0.1)

- `InitializeParticle.Color = RGBA(1, 0.899192, 0.548, 1)`, `Color Mode = Direct Set`.
- **Scale Color** (`Scale Mode = RGBA Together`, Vector4-from-Curve):
  `R: (0, 1)L (1, 1)L` · `G: (0, 1)L (1, 1)L` · `B: (0, 1)L (1, 1)L` · **`A: (0, 1)L (1, 0)L`**
  — i.e. RGB untouched, alpha a straight 1→0 fade.
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 1)L (1, 1)L`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 2 — `Glow_03` (5, t=0.04, life 0.05)

- `InitializeParticle.Color = RGBA(0.313989, 0, 0.00227652, 0.483)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`.
- **Color from Curve:** `R: (0, 1)C (1, 1)C` · `G: (0, 1)C (1, 0.450786)C` ·
  `B: (0, 1)C (1, 0.0409152)C` · `A: (0.485361, 1)C`
- **No size curve** (no `Scale Sprite Size` module).
- Dynamic params `[0, 0, 0, 0]`.

### Layer 3 — `Raimbow` (1, t=0.05, life 0.1)

- `InitializeParticle.Color = RGBA(0.913099, 0.913099, 0.913099, 0.2)`, `Color Mode = Direct Set`,
  `Sprite Rotation Mode = Random` over `0..360`.
- **`Color` module present with NO curve override** — it writes its own
  `Color.Color = RGBA(1, 1, 1, 1)`, i.e. it *replaces* the Initialize colour with white.
- **Scale Color** (RGBA Together): `R: (0, 0.5)L` · `G: (0, 0.5)L` · `B: (0, 0.5)L` ·
  `A: (0, 1)L (1, 0)L` — RGB scaled to a constant 0.5, alpha 1→0.
- **Scale Sprite Size**: `None: (0, 0.5)C (0.2, 0.9)C (1, 1)L`
- Dynamic params `[0.5, 0, 0, 0]` — `dissolve = 0.5` constant.

### Layer 4 — `Sparkles_01` (5, t=0.05, life **random 0.2–0.4**)

- `Lifetime Mode = Random` driven by `Random Range Float` **min 0.2 / max 0.4**.
- `Sphere Location`: **radius 0.1**, `Sphere Distribution = Random`, `Surface Only = false`,
  `Non Uniform Scale = (1,1,1)`, `Offset = (0,0,0)` — effectively a point source.
- `Add Velocity from Point`: `Velocity Strength = Random Range Float 001` **min 1300 / max 2000**,
  `Origin Offset = (0,0,0)`, `Velocity Falloff Distance = 100`. Radially outward from the origin.
- `Sprite Size Mode = Random Non-Uniform`, **min (35, 80) / max (50, 90)**.
- `InitializeParticle.Color = RGBA(1, 1, 1, 1)`; colour comes from **CURVE-A**.
- **Scale Velocity** (Vector from Curve): `X, Y, Z all: (0, 1)C (0.2, 0.35)C (1, 0.05)C`
- **Scale Sprite Size** (Uniform): `None: (0, 0)C (0.1, 1)C (1, 0)C`
- **Scale Sprite Size 001** (Non-Uniform): `X: (1, 1)L` · `Y: (0, 1)C (1, 0.6)C`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 5 — `Ring` (1, t=0.05, life 0.5)

- `InitializeParticle.Color = RGBA(1, 1, 1, 1)`, `Sprite Rotation Mode = Random` 0..360,
  `Sprite Size Mode = Uniform` **60** (the `Uniform Sprite Size Min/Max = 150/160` are **inert**).
- Colour = **CURVE-A**.
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, -0.325)C (1, -1)C` — starts partly eroded
  and becomes fully intact. Params 2–4 are `0`.
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 0.9)C (1, 1)C`

### Layer 6 — `Glow_04` (5, t=0.05, life 0.2)

- `InitializeParticle.Color = RGBA(0.313989, 0, 0.00227652, 0.483)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 0.3`.
- **Color from Curve:** `R: (0.315122, 1)C (1, 1)C` · `G: (0.315122, 0.664854)C (1, 0.450786)C` ·
  `B: (0.315122, 0.254571)C (1, 0.0409152)C` · `A: (0, 1)C (0.992454, 0)L`
  — note the RGB curves' first key is at **t = 0.315122**, not 0; before it the constant-mode key
  holds its own value.
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 1)L (1, 1)L`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 7 — `Glow_05` (3, t=0.05, life 0.1)

- `InitializeParticle.Color = RGBA(0.313989, 0, 0.00227652, 0.483)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`.
- **Color from Curve:** `R: (0.315122, 1)C (1, 1)C` · `G: (0.315122, 0.637597)C (1, 0.450786)C` ·
  `B: (0.315122, 0.152926)C (1, 0.0409152)C` · `A: (0.312708, 1)C (0.992454, 0)L`
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 1)L (1, 1)L`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 8 — `FlareImpact` (1, t=0.05, life 0.05)

- `InitializeParticle.Color = RGBA(0.644888, 0.2, 1, 1)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`. Colour = **CURVE-A**.
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, 0.5)C (1, -1)C` — starts eroded, ends
  intact. Params 2–4 are `0`.
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 0.9)C (1, 1)C`

### Layer 9 — `Spike01` (5, t=0.05, life 0.1) — MESH

- `Sphere Location`: **radius 20, `Surface Only = true`**, `Non Uniform Scale = (1,1,1)`.
- `Add Velocity from Point`: `Velocity Strength = 10` (constant), falloff distance 100.
- `Initial Mesh Orientation`: `Rotation Coordinate Space = **Mesh**`,
  `Orientation Axis = (0, 0, 1)`, `Orientation Vector = (1, 0, 0)`,
  **`Rotation = Random Range Vector` min `(0, 0, 1)` max `(0, 0.5, -1)`**.
- `Mesh Scale Mode = Random Non-Uniform`, **min (0.1, 0.1, 0.1) / max (0.3, 0.3, 0.3)**.
- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 1)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`. Colour = **CURVE-A**.
- **Scale Mesh Size** (Scale Float by Curve, per-axis):
  `X: (0, 0)C (0.2, 0.5)C (1, 4.17233e-08)C` ·
  `Y: (0, 0)C (0.2, 0.4)C (1, 5.66244e-08)C` ·
  `Z: (0, 0)C (0.2, 1)C`
  — a grow-then-collapse in X/Y and a grow-and-hold in Z: the spike shoots out and thins.
- `M_VFX_DisAdd_Flat02` has **no dynamic parameters**, so nothing is written to `Dynamic` here.

### Layer 10 — `LightningStrip` (5, t=0.05, life **random 0.1–0.2**) — MESH

- `Lifetime Mode = Random`, `Lifetime Min/Max = 0.1 / 0.2` (the Direct-Set `Lifetime = 0.3` is
  **inert**).
- **`Sphere Location` is DISABLED** (its radius 50 and `Hemisphere Z = true` never apply) — so all
  five particles **spawn at the origin**. `Position Offset = (0,0,0)` with `UsePositionOffset = true`.
- `Add Velocity from Point`: `Velocity Strength = 500` (constant), falloff distance 100.
  With every particle at the origin the direction is whatever the module resolves at zero distance —
  `[unresolved: the exact direction Niagara's Add-Velocity-from-Point produces at zero offset; the
  corpus records the module and its parameters, not its degenerate-case behaviour]`.
- `Initial Mesh Orientation`: coordinate space **Mesh**, axis `(0, 0, 1)`, vector `(1, 0, 0)`,
  **`Rotation = Random Range Vector` min `(0, 0, 1)` max `(0, 0.5, -1)`**.
- `Mesh Scale Mode = Non-Uniform` driven by **`Random Range Vector 001` min `(0.5, 0.5, 1)` max
  `(1.5, 1.5, 2)`**.
- `InitializeParticle.Color = RGBA(0.341915, 0.184475, 1, 1)`, `Color Mode = Direct Set`,
  **`Color.Scale Alpha = 0.4`**.
- **Color from Curve:** `R: (0, 1)C (0.319952, 1)L (1, 1)C` ·
  `G: (0, 0.913099)C (0.319952, 0.693872)L (1, 0.450786)C` ·
  `B: (0, 0.715694)C (0.319952, 0.147027)L (1, 0.040915)C` · `A: (0.327196, 1)C (1, 0)C`
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`
- **Scale Mesh Size** (Vector from Curve 001):
  `X: (0, 0.5)C (0.2, 1)C (1, -1.44926e-08)C` ·
  `Y: (0, -2.4747e-08)C (1, 1)C` ·
  `Z: (0, -5.68323e-08)C (0.2, 0.75)C (1, 1)C`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 11 — `Star01` (1, t=0.05, life 0.3)

- `InitializeParticle.Color = RGBA(1, 0.637597, 0.152926, 0.2)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`. Colour = **CURVE-A**.
- **Scale Sprite Size**: `None: (0, 0)C (0.4, 1)C (1, 0)C`
- Dynamic params `[1, 0, 0, 0]` — `dissolve = 1` constant.

### Layer 12 — `Star02` (1, t=**0.1**, life 0.3)

- `InitializeParticle.Color = RGBA(1, 0.637597, 0.152926, 0.2)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`. Colour = **CURVE-A**.
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, -0.125)C (1, -1)C`. Params 2–4 are `0`.
- **Scale Sprite Size**: `None: (0, 0)C (0.4, 1)C (1, 0)C`

### Layer 13 — `Wind_01` (1, t=0.05, life **1.5**) — MESH, tube

- `Mesh Scale Mode = Uniform` **0.3**.
- `Initial Mesh Orientation`: coordinate space **Mesh**, `Orientation Axis = (1, 0, 0)`,
  `Orientation Vector = (1, 0, 0)`, **`Rotation = (0, 0.25, 0)`** (a fixed quarter-turn about Y —
  note the value is in turns, matching the `Random Range Vector` ranges elsewhere in this pack
  `[inferred from the 0..1 magnitudes]`).
- `Add Velocity = (-200, 0, 0)`, `Scale Added Velocity = (1,1,1)` — **the tube travels −X.**
- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 1)`, `Color Mode = Unset`,
  **`Color.Scale Alpha = 0.5`**.
- **Color from Curve** — RGB are single constant keys, alpha is a full envelope:
  `R: (0, 0.0451862)C` · `G: (0, 0.0413334)C` · `B: (0, 0.0363297)C` ·
  **`A: (0, 0)L (0.240266, 1)C (0.676124, 1)L (1, 0)L`**
  (fade in to t=0.24, hold to t=0.676, fade out by t=1)
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (0.2, 0.15)C (1, 0)C`
- **Scale Mesh Size:** `X: (0, 1.5)C (0.2, 2)C` · `Y: (0, 1.5)C (0.2, 2)C` ·
  **`Z: (0, 0.5)C (0.4, 3)C (0.7, 4.75)C (1, 5)C`** — the tube stretches to 5× along its own axis.
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, -0.2)C (1, -1)C`. Params 2–4 are `0`.
- **`Update Mesh Orientation`**: `Rotation Coordinate Space = Simulation`,
  `Rotation Rate = 0.3`, `Rotation Vector = (1, 0, 0)` — a continuous spin about world X.

### Layer 14 — `Wind_02` (6, t=0.05, life **1.5**) — SubUV sprite

- `Sprite Size Mode = Random Uniform`, **min 130 / max 230**; `Sprite Rotation Mode = Random` 0..360.
- **`Sub UVAnimation`**: `Start Frame 0`, `End Frame 3`, `SubUV Loop Count 1` over a **2×2** sub-image
  sheet declared on the renderer.
- `Add Velocity = Random Range Vector 001` **min `(-100, -20, -20)` max `(-700, 20, 20)`** — a
  −X-dominant spray.
- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 1)`, `Color Mode = Unset`,
  **`Color.Scale Alpha = 0.3`**.
- **Color from Curve** — RGB single constant keys, alpha the same envelope as `Wind_01`:
  `R: (0, 0.5)C` · `G: (0, 0.447431)C` · `B: (0, 0.375)C` ·
  `A: (0, 0)L (0.240266, 1)C (0.676124, 1)L (1, 0)L`
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (0.2, 0.15)C (1, 9.74764e-10)C`
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, -5.88215e-08)C (1, -1)C`
  (the first key is an authored zero). Params 2–4 are `0`.
- **Scale Sprite Size**: `None: (0, 0.5)C (1, 1)C`

### Inert values recorded so they are not implemented `[corpus]`

- `Sprite Rotation Mode = Unset` on layers 0, 1, 2, 4, 6, 7, 8, 9, 10, 11, 12, 13 → their
  `Sprite Rotation Angle 90 / Min 0 / Max 360` never apply. Only `Raimbow`, `Ring` and `Wind_02`
  actually randomize rotation.
- `Lifetime Mode = Direct Set` on every layer except `Sparkles_01` and `LightningStrip` → the
  `Lifetime Min/Max` values on those layers never apply.
- `Sprite Size Mode = Uniform` on layers 0, 1, 2, 3, 5, 6, 7, 8, 11, 12 → their `Sprite Size`
  pairs and `Sprite Size Min/Max` never apply; `Ring`'s `Uniform Sprite Size Min/Max = 150/160`
  likewise never apply.
- `Solve Forces and Velocity` runs with `Clamp Velocity = false` and `Limit Acceleration = false`
  everywhere, so `Speed Limit 1000` / `Acceleration Limit 9999` never bind.
- `LightningStrip`'s `Sphere Location` module and its `Hemisphere Z = true` override are **DISABLED**.
- `Wind_01` and `Wind_02` carry `Mesh Scale Min/Max` and `Mesh Uniform Scale Min/Max` values their
  modes do not select.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**New row required: loop 1.0 s, particle lifetime 1.5 s, burst 42.** No existing row is close
(`_Burst` is 1.2/1.2/96, `_Slash` 1.0/0.5/19, `_Single` 1.0/1.1/1).

Lifetime **1.5 s**, not 0.55 s: the template particle must outlive the longest source layer
(`Wind_01`/`Wind_02` at 1.5 s), and every shorter layer hides itself past its own lifetime by writing
zero colour/size/scale — exactly the discipline NS_BasicAttack §8 established.

Layer index = **`Seed % 42`** (burst UniqueIDs are sequential, so one loop draws exactly one particle
per source emitter particle). Partition:

| Layer band | Source emitter | Count |
|---|---|---|
| 0 | `Glow_01` | 1 |
| 1 | `Glow_02` | 1 |
| 2–6 | `Glow_03` | 5 |
| 7 | `Raimbow` | 1 |
| 8–12 | `Sparkles_01` | 5 |
| 13 | `Ring` | 1 |
| 14–18 | `Glow_04` | 5 |
| 19–21 | `Glow_05` | 3 |
| 22 | `FlareImpact` | 1 |
| 23–27 | `Spike01` | 5 |
| 28–32 | `LightningStrip` | 5 |
| 33 | `Star01` | 1 |
| 34 | `Star02` | 1 |
| 35 | `Wind_01` | 1 |
| 36–41 | `Wind_02` | 6 |

Per-layer **spawn delay** (0, 0.04, 0.05 or 0.1 s) is handled the way NS_BasicAttack handled its
0.06 s spark delay: the layer hides for `Age < delay` and runs its curves on
`(Age - delay) / layerLifetime`.

### 6.2 Renderers / VisTag

The shared set (0–4) cannot express this system. **Twelve row-declared renderers are needed** — one
per distinct (kind × look) pair:

| Kind | Look | Layers |
|---|---|---|
| camera sprite `[needs a new kind — see §6.7 #1]` | `PartDisAdd01` | 0, 1, 14–18 |
| camera sprite | `PartDisAdd02` | 2–6 |
| camera sprite | `RainbowDisAdd` | 7 |
| `VelocityAlignedSprite` | `PartDisAdd04` *(exists)* | 8–12 |
| camera sprite | `RingDisAdd01` | 13 |
| camera sprite | `PartDisAdd01Bright` | 19–21 |
| camera sprite | `ImpactDisAdd01` | 22 |
| `Mesh` `Spike` | `FlatAdd02` | 23–27 |
| `Mesh` `Sheet` | `LightStripDisAdd` | 28–32 |
| camera sprite | `StarDisAdd01` | 33 |
| camera sprite | `StarDisAdd02` | 34 |
| `Mesh` `Tube` | `WindDisAdd02Mesh` | 35 |
| camera sprite, **SubUV 2×2** `[not expressible — §6.7 #2]` | `WindDisAdd01` | 36–41 |

That is **13 rows counting the SubUV one**, of which ten need a renderer kind the row spec does not
have. This is by far the heaviest renderer demand in the batch and it is the main reason this effect
is tier **L**.

Alternative worth pricing at implementation time: **split the effect across two or three behaviors**
sharing one cadence row, so no single row carries 13 renderers. The source is genuinely three
sub-effects (the glow stack, the spike/lightning debris, the wind) that happen to ship in one asset.

### 6.3 CkUsf looks

**One new family + eleven new looks.**

- **`FlatAdd` family (NEW)** — a second `.ush` beside `DissolveAdd.ush`. Signature is tiny:
  `Brightness`, `CoreColor`, and nothing else; `EmissiveColor = ParticleColor.rgb × Brightness`,
  `Opacity = ParticleColor.a`. One look, `FlatAdd02`, with `Brightness = 10`.
- **`DissolveAdd` family** — eleven new parameterizations, values straight off §4.1:
  `PartDisAdd01`, `PartDisAdd02`, `PartDisAdd01Bright`, `RainbowDisAdd`, `RingDisAdd01`,
  `ImpactDisAdd01`, `StarDisAdd01`, `StarDisAdd02`, `WindDisAdd01`, `WindDisAdd02Mesh`,
  `LightStripDisAdd`. (`PartDisAdd04` already exists.)

Naming caution: a look named `WindDisAdd02` **already exists** (NS_BasicAttack, from
`M_VFX_DisAdd_Pan_Wind02`). This system's `M_VFX_DisAdd_Wind02` is a **different material** — pick a
distinct look name (`WindDisAdd02Mesh` above) rather than colliding.

Family parameters this system needs that the current 15-param `CkUsf_Look_DissolveAdd` signature does
**not** expose: `Glow_Intensity` (0.3 on `Part02`), `Core_Intensity` (1 on `Part01_Bright` and
`Wind01`), `Core_Power` (0 on `Impact01` and `LightStrip`), `Gradient_Invert` (0 / 2),
`GradientMap_Displacement` (0.9 on `Rainbow`), `Opacty_StepAdd` (0.3 on `Rainbow`), `Color_Speed_X`
(−0.3 on `Wind02`), and the whole gradient-map LUT chain. See §6.7.

### 6.4 Mesh / texture needs

**Three new procedural meshes**, all buildable from §3's measurements with the existing
MeshDescription generator:

| Generated name | From | Build |
|---|---|---|
| `SM_CkParticles_Spike` | `SM_VFX_Spike01` | square pyramid, base `(±100, ±100, 0)`, apex `(0,0,200)`, 6 tris, UV per §3.1 |
| `SM_CkParticles_Sheet` | `SM_VFX_Plane01` | single flat quad X `−100..100` × Z `0..200`, u across, v top→bottom; drop the doubled sheet and make the look `_TwoSided` (the same simplification NS_BasicAttack §13.5 made) |
| `SM_CkParticles_Tube` | `SM_VFX_Ring01` | open cylinder r=100, Z `0..50`, 32 segments; `u = frac(0.75 - angle/360)`, `v = 1` at Z=0. Bake the renderer's `(1,1,5)` scale either into the mesh (Z `0..250`) or into the behavior's `Scale` — **decide once and record it**, because doing both is a 5× error |

**Textures: six or seven new procedural bakes, none of them measured yet.** `T_VFX_Part_02`,
`T_VFX_Ring_02`, `T_VFX_Impact_01`, `T_VFX_Star_02`, `T_VFX_Wind_01` (a 2×2 sheet), and the
`T_VFX_LUT_Rainbow_01` colour ramp. Candidates for reuse without a new bake: `T_VFX_Part_01` →
`SoftParticle`, `T_VFX_Part_04` → `SparkStreak`, `T_VFX_Noise_02` → `TileNoise`, `T_VFX_Ring_01` →
the existing `Ring` SDF, `T_VFX_Star_01` → the existing `Flare`, `T_VFX_Wind_02` → the existing
`WindBand`, `T_VFX_LightStrip_01` → the existing `Streak`. **Every one of those reuses is a guess
until the corpus PNG is measured** (NS_BasicAttack §7's method).

### 6.5 Behavior id

**Do not allocate now.** `ck::particles::NumBehaviors` read 18 at the time of writing; take whatever it
reads at implementation time and bump it. If §6.2's split option is taken, this effect consumes more
than one id.

### 6.6 Stage outputs

`Position` (sphere-location spawn points + integrated velocity), `Velocity`, `Color`, `Size`, `Scale`
(mesh layers), `Orientation` (mesh layers), `Dynamic`, `Rotation` (layers 3, 5, 14). `MeshIndex` stays
0 — each row renderer carries a single mesh. `SpriteAlignment` / `SpriteFacing` are not used: no layer
in this system is custom-facing.

**Spike01 and LightningStrip need `Orientation` computed from velocity**, because their renderers use
Niagara's mesh `Facing: Velocity` (§6.7 #3).

### 6.7 CAPABILITY GAPS — what the current pipeline cannot express

Be conservative reading this list. Items 1–4 are real pipeline work, not data edits.

1. **No camera-facing sprite kind on the row-renderer spec.** `ECk_ParticlesRenderer_Kind` is
   `{ Mesh, VelocityAlignedSprite }`. Ten of this system's fourteen sprite layers are
   `Unaligned` / `FaceCamera`, each needing a *different* look — and `User.SpriteMaterial` can carry
   exactly one. **A `CameraSprite` kind must be added to the row spec** (small, additive, mirrors
   `VelocityAlignedSprite`) or the effect cannot be drawn correctly.
2. **Sub-UV flipbooks are not supported at all.** `Wind_02` renders through a **2×2 SubUV** sheet
   driven by a `Sub UVAnimation` module (frames 0→3, loop count 1). `FCkParticles_StageOutput` has no
   `SubImageIndex` field, the DI writes no `Particles.SubImageIndex`, and no renderer declares
   `SubImageSize`. **This needs a DI output + a renderer property + a CPU/GPU mirror** — the largest
   single gap in this recipe. Workaround if it is deferred: bake one frame and accept a static wind
   puff, recorded as a fidelity deviation.
3. **Mesh renderer `Facing` modes are not expressible on a row spec.** `FCk_ParticlesRendererSpec`
   carries `Kind`/`VisTag`/`MeshName`/`LookName` — no facing mode. `Spike01` and `LightningStrip` use
   `Facing: Velocity`; `Wind_01` uses `Facing: Default`. **Workaround that probably suffices:** the
   behavior authors the velocity itself, so it can write an `Orientation` quaternion that carries the
   mesh's local axis onto the velocity direction, leaving the renderer at `Default`. That is math, not
   a pipeline change — but it must be done deliberately, and it is not what "Facing: Velocity" means
   under sub-frame velocity changes `[inferred]`.
4. **The gradient-map LUT chain is not implemented and here it is LOAD-BEARING.**
   `M_VFX_DisAdd_Rainbow` sets `GradientMap_Tex = T_VFX_LUT_Rainbow_01` (a **512×2 BGRA8 sRGB colour
   ramp**, not a greyscale mask), `GradientMap_Displacement = 0.9`, `Gradient_Invert = 2`. Prior
   recipes could omit this chain because their gradient map was a 1×1 white pixel; that argument does
   not transfer. This needs a new family parameter, a **colour** texture (the first non-greyscale
   texture the CkParticles library would carry), and a shader branch. Alternatively the rainbow layer
   is dropped and recorded as a deviation — it is 1 of 42 particles.
5. **Renderer-level mesh scale `(1,1,5)` has no home on the row spec.** Fold it into the generated
   tube or into `Scale`; see §6.4.
6. **Six DissolveAdd parameters this system drives are not in the family signature**:
   `Glow_Intensity`, `Core_Intensity`, `Core_Power`, `Gradient_Invert`, `Opacty_StepAdd`,
   `Color_Speed_X`. Unlike prior recipes, several are **not** at inert values here
   (`Glow_Intensity 0.3`, `Core_Power 0`, `Core_Intensity 1`). Each is an added parameter plus shader
   work, or a recorded deviation.
7. **`Opacty_DepthFade` (10 / 20 / 30 across the instances) is not wired.** Known, pre-existing gap.
8. **A second material family (`FlatAdd`) must be authored.** Not a gap in capability — CkUsf handles
   it — but it is the first time the cookbook ships two families, and the naming trap in §4.2
   (`M_VFX_DisAdd_Flat02` is *not* DisAdd) is a live footgun.
9. **`[unresolved]` — `Add Velocity from Point` at zero offset.** `LightningStrip` spawns all five
   particles at the origin with its `Sphere Location` disabled, then applies a from-point velocity of
   strength 500 from that same origin. What direction Niagara produces at zero distance is not
   recoverable from the corpus. Resolve by observation before implementing, or the five lightning
   sheets will fly the wrong way (or not at all).
10. **Local space matches** — every emitter is `LocalSpace: true`, like the CkParticles template.
    No gap, unlike the Slash recipe.

Nothing here needs ribbons, GPU simulation, collision, events, light renderers, or user parameters.

---

## 7+. Reserved for implementation
