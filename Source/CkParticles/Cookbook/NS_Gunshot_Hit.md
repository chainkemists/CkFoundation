# Recipe: NS_Gunshot_Hit → CkParticles (PLANNING SHEET)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior id is allocated, no `.ush` exists, no look exists, no cadence row exists, no mesh or texture
has been baked, and nothing has ever been rendered. Sections 7+ are reserved for the implementation
session.

**Read [NS_Gunshot_Cast.md](NS_Gunshot_Cast.md) alongside this.** `NS_Gunshot_Hit` is the same
construction with the three Wind layers and the `LightningStrip` removed, `FlareImpact` added, and
every `Add Velocity` replaced by `Add Velocity from Point`.

**This is the cheapest of the four Cast/Hit systems: one sub-UV emitter instead of three, one mesh
instead of two, no gradient-map LUT, no renderer-level mesh scale.**

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Gunshot_Hit` |
| Pack | Vefects — *Anime VFX* |
| Role in the pack | the bullet impact burst |

Corpus evidence:

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Gunshot_Hit.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part01_Bright,Part02,Part03_Bright,Part04,Star01,Impact01,Impact02,Flat02}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/{M_VFX_DissolveAdd,M_VFX_FlatAdd}.json`
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_Spike01.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Part_03,Part_04,Star_01,Impact_01,Impact_02,Noise_02,WhitePixel}.json`

**The source Niagara asset was never opened.**

> ### Two systems share this name — take the right one
> `[corpus]` A second `NS_Gunshot_Hit` lives at `Vefects/Anime_Stylized_VFX/VFX/Particles/`, also
> with **11 emitters** — emitter count does NOT discriminate them.
>
> **Fastest discriminator: the user-parameter list.** This system's is **empty**. The Stylized
> sibling exposes twelve: `User.Flare Impact Color 01`, `User.Glow Color 01`–`05`,
> `User.Impact Color 01`, `User.Scale Overall`, `User.Sparkles Color 01`, `User.Sparkles Color 02`,
> `User.Spike Color 01`, `User.Stars Color 01`.
> Second discriminator: the sibling renders through `MI_VFX_*` instances; this one through
> `M_VFX_DisAdd_*`.

---

## 2. System anatomy `[corpus]`

**11 CPU emitters, all enabled, all `LocalSpace: true`, `Determinism: false`, `Bounds: Dynamic`,
no user parameters.** Ten emitters run Loop Behavior **Infinite** / Loop Duration Mode **Fixed** /
**Loop Duration 1.0 s**; **`Sparkles_01` runs Loop Behavior `Once` with Loop Duration 0.3 s**.
`UseLoopCountLimit = false` everywhere (every `Loop Count Limit = 1` is inert).

**33 particles per loop, plus a one-time 7 on the first activation. Longest lifetime 0.4 s and the
last spawn is at t = 0.05 s — the whole effect is over by t ≈ 0.45 s and generations NEVER overlap.**

| # | Emitter | Count | Spawn t | Lifetime | Renderer | Alignment / Facing | Material | Size / Scale |
|---|---|---|---|---|---|---|---|---|
| 0 | `Glow_01` | 1 | 0 | 0.1 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | Uniform **700** |
| 1 | `Glow_02` | 1 | 0 | 0.1 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | Uniform **300** |
| 2 | `Glow_03` | 5 | **0.04** | 0.05 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part02` | Uniform **150** |
| 3 | `Sparkles_02` | 5 | 0.05 | **rand 0.2–0.4** | Sprite | **`VelocityAligned`** | `M_VFX_DisAdd_Part04` | rand non-uniform **(20,130)–(25,150)** |
| 4 | `Glow_04` | 5 | 0.05 | 0.2 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | Uniform **800** |
| 5 | `Glow_05` | 3 | 0.05 | 0.1 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part03_Bright` | Uniform **250** |
| 6 | `Star01` | 1 | 0.05 | 0.2 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Star01` | Uniform **20** |
| 7 | `Sparkles_01` | 7 | 0.05 | **rand 0.2–0.4** | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01_Bright` | rand uniform **6–10** |
| 8 | `Impact_01` | **6** | 0.05 | 0.2 | Sprite, **SubUV 2×2** | **`VelocityAligned`** | `M_VFX_DisAdd_Impact02` | rand non-uniform **(60,110)–(80,160)** |
| 9 | `FlareImpact` | 1 | 0.05 | 0.05 | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Impact01` | Uniform **150** |
| 10 | `Spike01` | 5 | 0.05 | 0.1 | **Mesh** `SM_VFX_Spike01` | **Facing `Velocity`** | `M_VFX_DisAdd_Flat02` (renderer override) | mesh scale rand **(0.1,0.1,0.2)–(0.05,0.2,0.5)** |

`Sort: None` everywhere. `Position Mode = Simulation Position` everywhere. **No emitter is disabled.**

**Every position offset in this system is inert.** `UsePositionOffset` is `false` on all eleven
emitters, including `Impact_01`, whose stored `Position Offset = (80, 0, 0)` therefore **does not
apply** — a direct contrast with `NS_Gunshot_Cast`, where three emitters really are offset along +X.
Everything here spawns at the impact point.

### Delta table vs `NS_Gunshot_Cast` `[corpus]`

Byte-identical emitters: `Glow_01`, `Glow_02`, `Star01`.

| Emitter | Change vs `NS_Gunshot_Cast` |
|---|---|
| `Glow_03` | `Color.Scale Alpha 1 → **4**`; the colour curve is replaced by **CURVE-S** (§5) |
| `Sparkles_02` | count `3 → **5**`; lifetime `rand 0.1–0.2 → **rand 0.2–0.4**`; **`Add Velocity` (a +X cone) → `Add Velocity from Point`** (radial, strength rand 1300–2000); `Sphere Location` radius `100 → **0.1**`, non-uniform scale `(0.1,0.1,0.1) → (1,1,1)`, `Hemisphere X` **removed**; `Scale Velocity` knee `0.35 → 0.2`; the `Y` taper curve gains a middle key |
| `Glow_04` | `Color.Scale Alpha 0.3 → **0.2**`; colour curve `G` and `B` start much darker (§5) |
| `Glow_05` | `Color.Scale Alpha 1 → **0.8**`; colour curve collapses to single constant keys (§5) |
| `Sparkles_01` | **`Add Velocity` → `Add Velocity from Point`** (strength rand **800–1700**); `Sphere Location` radius `100 → **0.1**`, non-uniform `(0.1,0.1,0.1) → (1,1,1)`, `Hemisphere X` **removed** |
| `Impact_01` | count `1 → **6**`; `UsePositionOffset` `true → **false**` (the 80-unit offset goes inert); `Sprite Rotation Mode` `Random → **Unset**`; size range `(100,170)–(120,200) → **(60,110)–(80,160)**`; **`Add Velocity` → `Sphere Location` (radius 10) + `Add Velocity from Point` (strength 10)** |
| `Spike01` | count `3 → **5**`; `Lifetime Mode Random → **Direct Set** 0.1`; `Surface Only false → **true**`; `Sphere Location` radius `10 → **20**`, non-uniform `(0.1,0.1,0.1) → (1,1,1)`, `Hemisphere X` **removed**; **`Add Velocity` → `Add Velocity from Point`** (strength 10); mesh scale `(0.2,0.2,0.4)–(0.2,0.2,0.7) → **(0.1,0.1,0.2)–(0.05,0.2,0.5)**`; `Color.Scale Alpha 1 → **4**`; `Scale Mesh Size` 0.2-key `0.5/0.4/1 → **1.5/1.5/1.5**` |
| **`LightningStrip`, `Wind_01`, `Wind_02`, `Wind_03`** | **REMOVED** |
| **`FlareImpact`** | **ADDED** |
| **`Star02`** | not present (it exists but disabled in the Cast variant) |

**The single structural difference is directionality.** `NS_Gunshot_Cast` is a *directed* muzzle
blast: hemisphere-X spawn volumes, +X velocity cones up to 7000 units/s, +X position offsets.
`NS_Gunshot_Hit` is *omnidirectional*: point-source sphere locations (radius 0.1), radial
`Add Velocity from Point`, no hemispheres, no offsets. Same palette, opposite topology.

> **`Spike01`'s mesh-scale range is inverted on X**: min `0.1` > max `0.05`. Copied verbatim from the
> corpus — the identical inversion appears in `NS_Arrow_Hit`. `[unresolved: whether Niagara swaps,
> clamps, or degenerates.]`

---

## 3. Mesh geometry `[corpus, measured from the .obj]`

**One carrier mesh.**

### `SM_VFX_Spike01` — 16 verts / 6 tris / 2 UV sets

A **square pyramid**: base `(±100, ±100, 0)`, apex `(0, 0, 200)`; bounds
`(-100,-100,0)` → `(100,100,200)`, size `(200,200,200)`. Six triangles = four side faces plus a
two-triangle base quad. Section material slot `"Material"`, asset-level default
`M_VFX_DisAdd_Slash01` — **overridden at the renderer to `M_VFX_DisAdd_Flat02`**, so the Slash01
reference is a red herring and must not be ported.

UV0 covers 0..1 and is a three-value layout, not a continuous parameterization:

| Vertex | uv0 |
|---|---|
| apex `(0, 0, 200)` | `(0.5, 0.0)` |
| base `(+100, +100, 0)` and `(-100, +100, 0)` | `(0.0, 1.0)` |
| base `(-100, -100, 0)` and `(+100, -100, 0)` | `(1.0, 1.0)` |

Measured: `corr(v, z) = -1.000`, `corr(v, radius) = +1.000`, `corr(u, angle) = -0.894`.
**v runs tip(0) → base(1)**; u only distinguishes the `+Y` and `−Y` halves.
`M_VFX_DisAdd_Flat02` samples **no texture at all** (§4.2), so the UV is irrelevant here — recorded
because the other three Cast/Hit systems reuse the same mesh.

**Neither `SM_VFX_Plane01` nor `SM_VFX_Ring01` is used by this system.**

---

## 4. Material families and per-instance deltas `[corpus]`

### 4.1 `Parents/M_VFX_DissolveAdd` — 8 instances

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
| `Impact01` | `FlareImpact` | `Brightness` **12**; **`Core_Power` 0**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_Impact_01`** |
| `Impact02` | `Impact_01` | `Brightness` **15**; `Core_Intensity` **2**; `Gradient_Invert` **0**; `Opacity_Boldness` **1**; Main/Color/Dissolve tex → **`T_VFX_Impact_02`** |

**`Distortion_Intensity` is 0 on every instance in this system** — the two nonzero ones (`Wind01`
0.5, `Wind02` 1) belong to emitters `NS_Gunshot_Hit` does not have. **The distortion branch is
entirely dead**, and `T_VFX_Noise_02` is not required.

**No `Rainbow` instance** — the gradient-map LUT gap that dominates the Arrow recipes does not apply.

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
| `T_VFX_Impact_01` | `TSF_G16` | Wrap | `Impact01` | **required** — **NEW bake, unmeasured** |
| `T_VFX_Impact_02` | **`TSF_BGRA8`** `TC_Alpha` | Wrap | `Impact02` | **required, and it is a 2×2 SUB-UV SHEET** — §6.7 #1. The BGRA8 source against `TC_Alpha` compression means the colour channels are authored but discarded, so it still resolves to a mask `[inferred]` |
| `T_VFX_Noise_02` | `TSF_G16` | Wrap | every `Distortion_Tex` / `GradientShape_Tex` | **not needed** — `Distortion_Intensity = 0` on every instance here |
| `T_VFX_WhitePixel` | **1×1**, `TSF_RGBA16`, `TC_Default`, sRGB true | Wrap | `GradientMap_Tex` on every instance | not needed — no-op |

**No "candidate" reuse above has been verified against the corpus PNG.** Measure first
(NS_BasicAttack §7's method).

---

## 5. Per-layer runtime curves `[corpus]`

`C` = constant key, `L` = linear key; `t` = NormalizedAge over that layer's own lifetime. Verbatim,
including the corpus's float-noise near-zero endpoints and its above-1 HDR keys.

Two curves recur; naming them avoids re-transcription:

> **CURVE-A ("the sparkle ramp")** — `R: (0, 1)C (0.46725, 1)L (1, 1)C` ·
> `G: (0, 1)C (0.46725, 0.693872)L (1, 0.450786)C` ·
> `B: (0, 1)C (0.46725, 0.147027)L (1, 0.040915)C` · `A: (0.466043, 1)C`
> — used by `Star01` only in this system.

> **CURVE-S ("the impact ramp")** —
> `R: (0, 0.715694)C (0.094174, 1)L (0.21974, 1)L (0.440688, 0.854993)L (0.707516, 0.01033)C` ·
> `G: (0, 0.89627)C (0.094174, 0.752942)L (0.21974, 0.341915)L (0.440688, 0.135633)L (0.707516, 0.007499)C` ·
> `B: (0, 1)C (0.094174, 0.109462)L (0.21974, 0.109462)L (0.440688, 0.051269)L (0.707516, 0.006049)C` ·
> `A: (0.466043, 1)C`
> — cool blue-white, snapping warm by t≈0.094 and crushing to near-black by t≈0.708.
> **Used by three emitters here: `Glow_03`, `FlareImpact`, `Spike01`.** In `NS_Gunshot_Cast` only
> `Spike01` uses it; this system's promotion of it to three layers is what makes the Hit read as one
> unified flash rather than a layered muzzle blast.

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
  **`Color.Scale Alpha = 4`**; size Uniform **150**.
- Colour = **CURVE-S**.
- **No size curve.** Dynamic params `[0, 0, 0, 0]`.

**`Color.Scale Alpha = 4` is a 4× alpha multiplier on top of a curve that already reaches 1.** Three
emitters in this system do it (`Glow_03`, `FlareImpact`, `Spike01`); no other system in the batch
exceeds 1. It is a deliberate over-brighten on a translucent additive-reading material, and any clamp
introduced in the port would flatten it.

### Layer 3 — `Sparkles_02` (5, t=0.05, life **random 0.2–0.4**)

- `Lifetime Mode = Random` via `Random Range Float` **min 0.2 / max 0.4**.
- `Sphere Location`: **radius 0.1**, `Non Uniform Scale = (1,1,1)`, `Surface Only = false`,
  `Offset = (0,0,0)`, **no hemisphere override** — a point source.
- `Add Velocity from Point`: `Velocity Strength = Random Range Float 001` **min 1300 / max 2000**,
  `Origin Offset = (0,0,0)`, `Velocity Falloff Distance = 100` — radially outward.
- `Sprite Size Mode = Random Non-Uniform`, **min (20, 130) / max (25, 150)**.
- `InitializeParticle.Color = RGBA(1, 1, 1, 1)`, `Color.Scale Alpha = 1`.
- **Color from Curve:**
  `R: (0, 1)C (0.297012, 1)L (0.671295, 1)L (0.910353, 0.381326)C` ·
  `G: (0, 0.913099)C (0.297012, 0.493097)L (0.671295, 0.24027)L (0.910353, 0.042927)C` ·
  `B: (0, 0.584079)C (0.297012, 0.0409999)L (0.671295, 0.0839999)L (0.910353, 0.0366073)C` ·
  `A: (0.243888, 1)C (1, 0)L`
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (**0.2**, **0.2**)C (1, 0.05)C`
- **Scale Sprite Size** (Uniform): `None: (0, 0)C (0.1, 1)C (1, 0)C`
- **Scale Sprite Size 001** (Non-Uniform): `X: (1, 1)L` ·
  **`Y: (0, 1)C (0.3, 0.35)C (1, 0.2)C`** (the Cast variant's is the simpler `(0, 1)C (1, 0.6)C`)
- Dynamic params `[0, 0, 0, 0]`.

### Layer 4 — `Glow_04` (5, t=0.05, life 0.2)

- `InitializeParticle.Color = RGBA(0.313989, 0, 0.00227652, 0.483)`, `Color Mode = Unset`,
  **`Color.Scale Alpha = 0.2`**; size Uniform **800**.
- **Color from Curve:** `R: (0, 1)C (0.414126, 1)L (0.926049, 1)C` ·
  **`G: (0, 0.349312)C (0.414126, 0.494059)L (0.926049, 0.450786)C`** ·
  **`B: (0, 0.0970486)C (0.414126, 0.124008)L (0.926049, 0.0409152)C`** ·
  `A: (0, 1)C (0.992454, 0)L`
  — a deep red halo that only *warms* toward the middle of its life, where the Cast variant's starts
  near-white.
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 1)L (1, 1)L`
- Dynamic params `[0, 0, 0, 0]`.

### Layer 5 — `Glow_05` (3, t=0.05, life 0.1) — **HDR**

- `InitializeParticle.Color = RGBA(0.313989, 0, 0.00227652, 0.483)`, `Color Mode = Unset`,
  **`Color.Scale Alpha = 0.8`**; size Uniform **250**.
- **Color from Curve — RGB are single constant keys, all above or near 1:**
  **`R: (0.315122, 3)C`** · **`G: (0.315122, 1.49184)C`** · `B: (0.315122, 0.509197)C` ·
  `A: (0.312708, 1)C (0.992454, 0)L`
  — a constant 3× white-hot core whose only animation is the alpha fade. (The Cast variant's decays
  in RGB as well.)
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 1)L (1, 1)L`
- Dynamic params `[1, 0, 0, 0]`.

### Layer 6 — `Star01` (1, t=0.05, life 0.2)

- `InitializeParticle.Color = RGBA(1, 0.637597, 0.152926, 0.2)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`; size Uniform **20**. Colour = **CURVE-A**.
- **Scale Sprite Size**: `None: (0, 0)C (0.4, 1)C (1, 0)C`
- Dynamic params `[1, 0, 0, 0]`.

### Layer 7 — `Sparkles_01` (7, t=0.05, life **random 0.2–0.4**) — **ONE-SHOT**

- **`Emitter State`: Loop Behavior `Once`, Loop Duration Mode `Fixed`, Loop Duration `0.3`.**
  Every other emitter is Infinite / 1.0. §6.7 #2.
- `Lifetime Mode = Random` via `Random Range Float` **min 0.2 / max 0.4** (the module's own
  `Lifetime Min/Max = 0.3 / 0.6` are inert — the dynamic input wins).
- `Sphere Location`: **radius 0.1**, `Non Uniform Scale = (1,1,1)`, `Surface Only = false`,
  no hemisphere — a point source.
- `Add Velocity from Point`: `Velocity Strength = Random Range Float 001` **min 800 / max 1700**,
  `Origin Offset = (0,0,0)`, `Velocity Falloff Distance = 100`.
- `Sprite Size Mode = Random Uniform`, **min 6 / max 10** (`Sprite Size Min/Max` `(35,25)`/`(50,35)`
  are inert).
- `InitializeParticle.Color = RGBA(1, 1, 1, 1)`, **`Color.Scale Alpha = 0.15`**.
- **Color from Curve:** `R: (0.562632, 1)C (0.997283, 0.112)L` ·
  `G: (0.562632, 0.603828)C (0.997283, 0.0676287)L` ·
  `B: (0.562632, 0.296138)C (0.997283, 0.0331675)L` · `A: (0.562632, 1)L (1, 0)C`
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (**0.1**, 0.15)C (1, -9.09372e-09)C`
  — knee at t=0.1, not the 0.2 every other layer uses.
- **Scale Sprite Size** (Uniform): `None: (0, 0)C (0.1, 1)C (1, 0)C`
- **Dynamic params `[3, 0, 0, 0]`** — `dissolve = 3`, the largest constant dissolve in the batch.

### Layer 8 — `Impact_01` (6, t=0.05, life 0.2) — SubUV, velocity-aligned, **HDR**

- `Sprite Size Mode = Random Non-Uniform`, **min (60, 110) / max (80, 160)**;
  `Sprite Rotation Mode = **Unset**`; **`UsePositionOffset = false`**, so the stored
  `Position Offset = (80, 0, 0)` is **inert**.
- **`Initial Mesh Orientation` is DISABLED** (its `Random Range Vector` min `(0,0.22,0)` max
  `(0,0.28,0)` never applies).
- `Sphere Location`: **radius 10**, `Non Uniform Scale = (1,1,1)`, `Surface Only = false`.
- `Add Velocity from Point`: `Velocity Strength = 10` (constant), falloff distance 100.
- **`Sub UVAnimation`**: `Start Frame 0`, **`End Frame 4`**, `SubUV Loop Count 1`, on a **2×2**
  sheet — see §6.7 #1's `[unresolved]`.
- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 1)`, `Color Mode = Unset`,
  `Color.Scale Alpha = 1`.
- **Color from Curve — the first keys are strongly HDR:**
  **`R: (0, 3.57847)C (0.094174, 3)L (0.315122, 1)L (0.447932, 0.854993)L (0.606097, 0.01033)C`** ·
  **`G: (0, 4.48135)C (0.094174, 2.25883)L (0.315122, 0.341915)L (0.447932, 0.135633)L (0.606097, 0.007499)C`** ·
  **`B: (0, 5)C (0.094174, 0.328386)L (0.315122, 0.109462)L (0.447932, 0.051269)L (0.606097, 0.006049)C`** ·
  `A: (0.466043, 1)C`
  — a 5× blue-white flashbulb collapsing to near-black by t≈0.606. With `Impact02`'s `Brightness 15`
  and **six** particles (against the Cast variant's one), this is the dominant element of the effect.
- **Scale Velocity:** `X, Y, Z all: (0, 1)C (0.2, 0.15)C (1, 9.74764e-10)C`
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, 1)C (1, -1)C`. Params 2–4 `0`.
- **Scale Sprite Size** (`Non-Uniform Curve` mode): `X: (0, 0.5)C (0.2, 1)C (1, 0.4)C` ·
  `Y: (0.3, 1)C` (its uniform companion `None: (0, 0)L (1, 1)L` is unused in this mode)

### Layer 9 — `FlareImpact` (1, t=0.05, life 0.05)

- `InitializeParticle.Color = RGBA(0.644888, 0.2, 1, 1)`, `Color Mode = Unset`,
  **`Color.Scale Alpha = 4`**; size Uniform **150**. Colour = **CURVE-S**.
- **Dynamic param 1 (`dissolve`) from Curve:** `None: (0, 0.5)C (1, -1)C`. Params 2–4 `0`.
- **Scale Sprite Size**: `None: (0, 0.5)C (0.1, 0.9)C (1, 1)C`

### Layer 10 — `Spike01` (5, t=0.05, life 0.1) — MESH

- `Lifetime Mode = **Direct Set** 0.1` (the Cast variant randomizes 0.1–0.15).
- `Sphere Location`: **radius 20, `Surface Only = true`**, `Non Uniform Scale = (1,1,1)`,
  no hemisphere.
- `Add Velocity from Point`: `Velocity Strength = 10` (constant), falloff distance 100.
- `Initial Mesh Orientation`: coordinate space **Mesh**, `Orientation Axis = (0, 0, 1)`,
  `Orientation Vector = (1, 0, 0)`, **`Rotation = Random Range Vector` min `(0, 0, 1)` max
  `(0, 0.5, -1)`**.
- `Mesh Scale Mode = Random Non-Uniform`, **min `(0.1, 0.1, 0.2)` / max `(0.05, 0.2, 0.5)`** — the X
  component's min exceeds its max; see the §2 note.
- `InitializeParticle.Color = RGBA(1, 0.184475, 0.386429, 1)`, **`Color.Scale Alpha = 4`**.
  Colour = **CURVE-S**.
- **Scale Mesh Size** (Scale Float by Curve, per-axis):
  `X: (0, 0)C (0.2, 1.5)C (1, 4.17233e-08)C` · `Y: (0, 0)C (0.2, 1.5)C (1, 5.66244e-08)C` ·
  `Z: (0, 0)C (0.2, 1.5)C`
  — a **uniform** 1.5× pop at t=0.2 (the Cast variant's is anisotropic 0.5 / 0.4 / 1).
- `M_VFX_DisAdd_Flat02` has no dynamic parameters — nothing is written to `Dynamic`.

### Inert values recorded so they are not implemented `[corpus]`

- **Every `Position Offset` in this system is inert** (`UsePositionOffset = false` on all eleven
  emitters), including `Impact_01`'s `(80, 0, 0)`.
- `Sprite Rotation Mode = Unset` on **every** emitter → no layer randomizes sprite rotation, and all
  the `Sprite Rotation Angle 90 / Min 0 / Max 360` values are dead. (`NS_Gunshot_Cast` randomizes on
  three layers.)
- `Sprite Size Mode = Uniform` on layers 0, 1, 2, 4, 5, 6 → their `Sprite Size` pairs never apply.
- `Lifetime Mode = Direct Set` on every layer except `Sparkles_02` and `Sparkles_01`.
- `Clamp Velocity = false`, `Limit Acceleration = false` everywhere → `Speed Limit 1000` /
  `Acceleration Limit 9999` never bind.
- `Impact_01` carries a **DISABLED** `Initial Mesh Orientation` module.
- Every `GradientMap_Tex` is `T_VFX_WhitePixel` — the gradient chain is a no-op system-wide.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**New row: loop 1.0 s, particle lifetime 0.4 s, burst 33.** The longest source layer is 0.4 s
(`Sparkles_01` / `Sparkles_02` at the top of their random range), the last spawn is t = 0.05 s, and
**nothing survives past t ≈ 0.45 s — generations never overlap.** Shorter layers hide past their own
lifetime (NS_BasicAttack §8's rule).

**The 7 one-shot `Sparkles_01` particles are NOT in the 33.** Same fork as `NS_Gunshot_Cast` §6.1:
(a) fold them in at burst 40 and accept a per-loop re-fire, or (b) drop them and record the deviation.
**Whichever is chosen must be recorded, not silent.**

Layer index = **`Seed % 33`** (or 40 under option (a)). Partition under option (b):

| Layer band | Source emitter | Count |
|---|---|---|
| 0 | `Glow_01` | 1 |
| 1 | `Glow_02` | 1 |
| 2–6 | `Glow_03` | 5 |
| 7–11 | `Sparkles_02` | 5 |
| 12–16 | `Glow_04` | 5 |
| 17–19 | `Glow_05` | 3 |
| 20 | `Star01` | 1 |
| 21–26 | `Impact_01` | 6 |
| 27 | `FlareImpact` | 1 |
| 28–32 | `Spike01` | 5 |

Per-layer spawn delay (0 / 0.04 / 0.05 s) handled as in NS_BasicAttack: hide while `Age < delay`, run
the curves on `(Age - delay) / layerLifetime`.

### 6.2 Renderers / VisTag

| Kind | Look | Layers |
|---|---|---|
| camera sprite `[needs a new kind — §6.7 #3]` | `PartDisAdd01` | 0, 1, 12–16 |
| camera sprite | `PartDisAdd02` | 2–6 |
| `VelocityAlignedSprite` *(exists)* | `PartDisAdd04` *(look exists)* | 7–11 |
| camera sprite | `PartDisAdd03Bright` | 17–19 |
| camera sprite | `StarDisAdd01` | 20 |
| `VelocityAlignedSprite` **+ SubUV 2×2** `[not expressible — §6.7 #1]` | `ImpactDisAdd02` | 21–26 |
| camera sprite | `ImpactDisAdd01` | 27 |
| `Mesh` `Spike` | `FlatAdd02` | 28–32 |

Eight renderers, **one of them sub-UV** (against `NS_Gunshot_Cast`'s eleven with three sub-UV). Under
option (a), add a ninth (camera sprite, `PartDisAdd01Bright`, layers 33–39).

VisTags allocate above `SharedRendererVisTag_Max`, read via `Get_RosterVisTag_Max()` — never a literal.

### 6.3 CkUsf looks

**One new family + six new looks.**

- **`FlatAdd` family (NEW)** — `EmissiveColor = ParticleColor.rgb × Brightness`,
  `Opacity = ParticleColor.a`; one look `FlatAdd02`, `Brightness = 10`. Shared with the other three
  Cast/Hit recipes.
- **`DissolveAdd` family** — `PartDisAdd01`, `PartDisAdd02`, `PartDisAdd03Bright`, `StarDisAdd01`,
  `ImpactDisAdd01`, `ImpactDisAdd02` (+ `PartDisAdd01Bright` under option (a)).
  `PartDisAdd04` already exists.

**Every one of those six is shared with another recipe in this batch** (`ImpactDisAdd01` with the two
Arrow systems; the rest with `NS_Gunshot_Cast`). If either sibling ships first, this recipe adds
**zero** new looks.

### 6.4 Mesh / texture needs

**One new procedural mesh:** `SM_CkParticles_Spike` — square pyramid, base `(±100, ±100, 0)`, apex
`(0, 0, 200)`, 6 triangles, UV per §3. Shared with all three sibling recipes.

**Textures: three new bakes plus one sub-UV sheet.** New and unmeasured: `T_VFX_Part_02`,
`T_VFX_Part_03`, `T_VFX_Impact_01`, and the 2×2 sheet `T_VFX_Impact_02`. Candidate reuses:
`T_VFX_Part_01` → `SoftParticle`, `T_VFX_Part_04` → `SparkStreak`, `T_VFX_Star_01` → `Flare`.
**Every reuse is a guess until the corpus PNG is measured.**

### 6.5 Behavior id

**Do not allocate now.** `ck::particles::NumBehaviors` read 18 at the time of writing; bump whatever it
reads at implementation time.

### 6.6 Stage outputs

`Position` (point-source and radius-10/20 sphere spawn points + integrated velocity), `Velocity`,
`Color`, `Size`, `Scale` + `Orientation` (mesh layers 28–32), `Dynamic`. **`Rotation` is NOT written
by any layer** — every emitter's `Sprite Rotation Mode` is `Unset`. `MeshIndex` stays 0.
`SpriteAlignment` / `SpriteFacing` unused — no layer is custom-facing.

**A `SubImageIndex` output does not exist and would be required for layers 21–26.**

### 6.7 CAPABILITY GAPS — what the current pipeline cannot express

1. **Sub-UV flipbooks are not supported at all, and `Impact_01` needs one** (2×2 sheet, 6 of 33
   particles — and the visually dominant ones). `FCkParticles_StageOutput` has no `SubImageIndex`
   field, the DI writes no `Particles.SubImageIndex`, and no renderer declares `SubImageSize`. Needs
   a DI output, a renderer property on the row spec, a texture-generator sheet layout, and a CPU/GPU
   mirror. **Deferral workaround:** bake one representative frame and accept a static impact flash.
   `[unresolved: `End Frame = 4` on a 2×2 sheet whose valid indices are 0–3. Whether Niagara clamps,
   wraps, or samples a fifth non-existent frame is not recoverable from the corpus. The same value
   appears on two `NS_Gunshot_Cast` emitters.]`
2. **A per-emitter loop behavior that differs from the system's cannot be expressed.**
   `Sparkles_01` is `Loop Behavior = Once` with `Loop Duration = 0.3` while the other ten are
   `Infinite` / `1.0`. A CkParticles cadence row is ONE (loop, lifetime, burst) triple for the whole
   template. Options in §6.1; record whichever is chosen as a deviation.
3. **No camera-facing sprite kind on the row-renderer spec.** `ECk_ParticlesRenderer_Kind` is
   `{ Mesh, VelocityAlignedSprite }`; five of this system's layer groups are `Unaligned` /
   `FaceCamera` needing different looks, and `User.SpriteMaterial` carries one. Add a `CameraSprite`
   kind — the same fix all four Cast/Hit recipes need.
4. **Mesh renderer `Facing: Velocity` is not expressible on a row spec.** `Spike01` uses it.
   Workaround: the behavior authors the velocity, so it can write an `Orientation` quaternion carrying
   the mesh's local axis onto the velocity direction. Math, not a pipeline change — but deliberate.
5. **Four DissolveAdd parameters this system drives are not in the family signature:**
   `Glow_Intensity` (**0.3** on `Part02`), `Core_Intensity` (**1** on `Part01_Bright`/`Part03_Bright`,
   **2** on `Impact02`), `Core_Power` (**0** on `Impact01`), `Gradient_Invert` (0). Several are
   **not** at inert values.
6. **`CamOffset = 50` on `M_VFX_DisAdd_Part03_Bright` is not wired.** A camera-toward world-position
   offset in the family's `WorldPosition` chain; CkUsf surface looks expose no WorldPositionOffset hook
   here, so `Glow_05` will sort against nearby geometry differently from the source
   `[inferred from the parameter name and the `WorldPosition ×1` expression; the exact chain is not
   reconstructible from a histogram]`.
7. **`Opacty_DepthFade` (10 / 20 / 30) is not wired.** Known, pre-existing gap.
8. **Colour values above 1 must survive the port** — both the HDR curve keys (`Glow_05` R=3 / G=1.49;
   `Impact_01` R=3.58 / G=4.48 / B=5) **and** the `Color.Scale Alpha = 4` multipliers on `Glow_03`,
   `FlareImpact` and `Spike01`. `FCkParticles_StageOutput.Color` is a `float4` so the values pass, and
   the roster test deliberately does not assert an alpha upper bound — but any clamp added later would
   silently flatten the three brightest layers of this system.
9. **`[unresolved]` — `Spike01`'s inverted X mesh-scale range** (min 0.1 > max 0.05). Verbatim from
   the corpus; the identical inversion appears in `NS_Arrow_Hit`.
10. **A second material family (`FlatAdd`) must be authored.** Not a capability gap; note the naming
    trap — `M_VFX_DisAdd_Flat02` is **not** in the DisAdd family.
11. **Local space matches** the CkParticles template on every emitter — **no gap**.

**Absent here, unlike the siblings:** no gradient-map LUT (no `Rainbow`), no renderer-level mesh
scale, no tube or sheet mesh, no distortion branch, no sprite-rotation output. Nothing needs ribbons,
GPU simulation, collision, events, light renderers, or user parameters.

---

## 7+. Reserved for implementation
