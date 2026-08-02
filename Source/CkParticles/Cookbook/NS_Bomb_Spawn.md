# Recipe: NS_Bomb_Spawn → CkParticles (PRE-IMPLEMENTATION TRANSLATION SHEET)

Schema and evidence-tag conventions: [README.md](README.md).
Exemplars this sheet copies: [NS_BasicAttack.md](NS_BasicAttack.md) §1–6, [NS_Lightning_Range.md](NS_Lightning_Range.md) §1–5.

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior, no `.ush`, no CPU mirror, no look, no mesh, no texture, no cadence row, no test, no gym
station exists for this effect. No behavior id has been allocated. Nothing has been rendered or
visually compared. Sections 1–6 are archaeology and a plan; everything below `## 7+` is reserved for
the implementation session.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Bomb_Spawn` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior id | **not allocated** — take the next free id at implementation time |
| Recreation status | not started |

Corpus evidence (all `[corpus]`, exported 2026-08-01):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Bomb_Spawn.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part01_Bright,Part02,Part03,Part03_Bright,Rainbow,Ring01,Star03}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/MI_VFX_Bomb.json`
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_Bomb_01_Small.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/*.json`

**The source Niagara asset was not opened for editing.**

> ### A same-named sibling exists — take the right one
> `[corpus]` `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Bomb_Spawn` is a **different,
> parameterized** system. **Fastest discriminator: it declares 8+ user parameters** (`User.Bomb Glow
> Color 01`, `User.Flare Color 01…04`, `User.Flash Color 01/02`, `User.Glow Color 01`) and renders
> through `MI_VFX_*` instances (`MI_VFX_Glow_01`, `MI_VFX_Glow_02`, `MI_VFX_Glow_03`,
> `MI_VFX_Star_03`, `MI_VFX_Lens_Rainbow_01`, `MI_VFX_Ring_01`, `MI_VFX_Bomb`). The
> `Anime_VFX/Shared/Skills` variant documented here has an **empty user-parameter list** and renders
> through `M_VFX_DisAdd_*`.

---

## 2. System anatomy `[corpus]`

**14 CPU emitters, ALL world-space (`LocalSpace: false`), ALL `Spawn Burst Instantaneous`, no
continuous rate anywhere. 27 particles per loop. 13 sprite renderers + 1 mesh renderer.**

| # | Emitter | Count | Spawn t | Lifetime | Sprite size | Renderer | Material |
|---|---|---|---|---|---|---|---|
| 0 | `Glow_01` | 1 | 0 | 1 | Uniform **500** | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Part01` |
| 1 | `Glow_02` | 1 | 0 | 1 | Uniform **250** | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Part01` |
| 2 | `Glow_03` | **3** | **0.05** | 1 | Uniform **200** | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Part01` |
| 3 | `Raimbow` | 1 | **0.1** | 0.3 | Uniform **300** | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Rainbow` |
| 4 | `Sparkles` | **10** | **0.05** | rand **0.5–1.0** `[corpus-v3]` | Random Uniform **7–10** | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Part01_Bright` |
| 5 | `Ring01` | 1 | **0.05** | 0.75 | Uniform **170** | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Ring01` |
| 6 | `Flash_Glow_01` | 1 | **0.1** | 0.5 | Uniform **30** | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Part02` |
| 7 | `Flash_Glow_02` | 1 | **0.1** | 0.5 | Uniform **50** | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Part02` |
| 8 | `Bomb_Glow` | **3** | **0.05** | 1 | Uniform **300** | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Part01` |
| 9 | `Bomb` | 1 | 0 | 1 | mesh scale **0.45** | **Mesh**, Facing Default | `Parents/MI_VFX_Bomb` on `SM_VFX_Bomb_01_Small` |
| 10 | `Flare_Stretched_04` | 1 | 0 | 0.3 | Non-Uniform **(400, 50)** | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Star03` |
| 11 | `Flare_Stretched_03` | **2** | 0 | 0.7 | Non-Uniform **(1400, 200)** | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Part03` |
| 12 | `Flare_Stretched_02` | 1 | 0 | 0.5 | Non-Uniform **(500, 100)** | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Part03_Bright` |
| 13 | `Flare_Stretched_01` | 1 | 0 | 0.5 | Non-Uniform **(500, 100)** | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Part01` |

**Total per firing: 27 particles** (1+1+3+1+10+1+1+1+3+1+1+2+1+1).

Per-emitter `Emitter State` on all 14: `Loop Behavior = Infinite`, `Loop Duration Mode = Fixed`,
`Loop Duration = 1`, `Loop Delay = 0`, `UseLoopDelay = false`, `UseLoopCountLimit = false`
(`Loop Count Limit = 1` inert). `Bounds: Dynamic`, `Determinism: false`,
`Inactive Response = Complete`. Every emitter `Position Mode = Simulation Position`,
`Position Offset = (0,0,0)`, `UsePositionOffset = false` — **nothing is displaced at spawn** except
`Sparkles` (Sphere Location, §5.5).

> **`Life Cycle Mode = System` on all 14 emitters `[corpus]`.** The stored per-emitter Loop
> Behavior / Loop Duration are therefore **inert leftovers**; the system drives the cadence.
>
> **System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
> `UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
> Per [P0-D1] this is the authority — the effect fires ONE 27-particle burst over a single 2.0 s
> cycle, it does not re-burst every second. *(Was `[unresolved]`.)* Spawn **times** and burst
> **counts** are live.

**Spawn-time clustering is the effect's structure**: t = 0 (Glow_01, Glow_02, Bomb, all four
Flare_Stretched), t = 0.05 (Glow_03, Sparkles, Ring01, Bomb_Glow), t = 0.1 (Raimbow,
Flash_Glow_01, Flash_Glow_02). Three beats, 50 ms apart.

---

## 3. Mesh geometry `[corpus]`

### `SM_VFX_Bomb_01_Small`

| Fact | Value |
|---|---|
| Verts / tris | **6066 / 3128** |
| UV channels | 2 |
| Bounds min | `(-87.26517486572266, -91.60921478271484, -81.66767120361328)` |
| Bounds max | `(87.26517486572266, 91.7042007446289, 81.66767120361328)` |
| Bounds size | `(174.5303497314453, 183.31341552734375, 163.33534240722656)` |
| uv0 range | `(0, 0.04724121)` … `(1, 0.8491211)` |
| Section 0 | slot `WorldGridMaterial` → `Parents/MI_VFX_Bomb`, 3128 tris |

OBJ characterization `[corpus, derived from the .obj]`:

- Radius-from-origin (3D): **min 69.161, max 94.084, mean 81.818** — a detailed ball, not a
  constant-radius primitive.
- `radiusXY` per Z-octile: `0:{0.0..47.5} 1:{12.8..56.9} 3:{73.9..92.5} 4:{74.3..92.4}
  5:{73.9..92.0} 7:{12.8..56.9} 8:{0.0..47.5}` — symmetric about Z, widest at the equator.
- **UVs are an authored atlas.** `corr(u, angleXY) = −0.131`, `corr(u, radiusXY) = −0.052`,
  `corr(u, Z) = 0.083`; `corr(v, radiusXY) = 0.175`, `corr(v, Z) = −0.058`. Every correlation is
  near zero: **no cylindrical / spherical / planar projection to re-derive.** `v` spans only
  0.047…0.849.

Consequence: the silhouette is trivially reproducible; the **UV layout is not**, and
`MI_VFX_Bomb` (§4.2) bands its three flat colours by `Step` over exactly these UVs. See §6.3.

---

## 4. Material families and per-instance deltas `[corpus]`

### 4.1 `M_VFX_DissolveAdd` family — 8 of the 9 materials

All of `M_VFX_DisAdd_{Part01, Part01_Bright, Part02, Part03, Part03_Bright, Rainbow, Ring01,
Star03}` are instances of `/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd`.
Identical base properties on all eight: `MD_Surface`, **`BLEND_Translucent`**, **`MSM_Unlit`**,
`twoSided: false`, connected outputs **`EmissiveColor` + `Opacity`**, dynamic-parameter channel
names **`[dissolve, distortion, offset, core_color]`**, and the same expression histogram
(`ScalarParameter ×41`, `Multiply/Add/AppendVector ×18` each, `Saturate ×12`, `DynamicParameter ×8`,
`TextureSampleParameter2D ×6`, `Panner ×5`, `Constant ×5`, `TextureCoordinate ×5`,
`LinearInterpolate ×4`, `Clamp ×2`, `OneMinus ×2`, `DepthFade/ParticleColor/Power/SmoothStep/
StaticBoolParameter/StaticSwitch/VectorParameter/MaterialFunctionCall/WorldPosition ×1` each).

This is the family the CkUsf look `CkUsf_Look_DissolveAdd` already implements (NS_BasicAttack §9).

Reference instance = `M_VFX_DisAdd_Part01`: `Main_Tex`/`Color_Tex`/`Dissolve_Tex` = `T_VFX_Part_01`;
`Distortion_Tex`/`GradientShape_Tex` = `T_VFX_Noise_02`; `GradientMap_Tex` = `T_VFX_WhitePixel`;
`Brightness` 1; `Opacity_Boldness` 0.5; `Distortion_Intensity` 0; `Dissolve` 0; `Dissolve_Invert` 0;
all `*_Scale_X/Y` 1; all `*_Speed_X/Y` and `*_Offset_X/Y` 0; `Gradient_Invert` 0.5;
`GradientMap_Displacement` 0.10000000149011612; `Color_CoreDifferent` 0; `Core_Power` 1;
`Core_Intensity` 0; `Glow_Intensity` 1; `Opacty_Step` 0; `Opacty_StepAdd` 0.10000000149011612;
`Opacty_DepthFade` 20; `CamOffset` 0; `Color_Core` `RGBA(1, 1, 1, 0)`.

| Instance | Delta vs `Part01` |
|---|---|
| `M_VFX_DisAdd_Part01` | *(reference)* — used by `Glow_01`, `Glow_02`, `Glow_03`, `Bomb_Glow`, `Flare_Stretched_01` |
| `M_VFX_DisAdd_Part01_Bright` | `Brightness` → **10**; `Core_Intensity` → **1**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Part_02`** |
| `M_VFX_DisAdd_Part02` | `Glow_Intensity` 1 → **0.3**; Main/Color/Dissolve → **`T_VFX_Part_02`** |
| `M_VFX_DisAdd_Part03` | `Brightness` → **3**; `Core_Intensity` → **1**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Part_03`** |
| `M_VFX_DisAdd_Part03_Bright` | `Brightness` → **10**; **`CamOffset` 0 → 50**; `Core_Intensity` → **1**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Part_03`** |
| `M_VFX_DisAdd_Rainbow` | `GradientMap_Displacement` 0.1 → **0.9**; `Gradient_Invert` 0.5 → **2**; `Opacity_Boldness` 0.5 → **1.5**; `Opacty_StepAdd` 0.1 → **0.3**; **`GradientMap_Tex` → `T_VFX_LUT_Rainbow_01`**; `GradientShape_Tex` → `T_VFX_Part_01`; `Main_Tex` → **`T_VFX_Ring_02`** (`Color_Tex`/`Dissolve_Tex` stay `T_VFX_Part_01`) |
| `M_VFX_DisAdd_Ring01` | `Brightness` → **10**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Ring_01`** |
| `M_VFX_DisAdd_Star03` | `Brightness` → **10**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; `Opacty_DepthFade` 20 → **10**; Main/Color/Dissolve → **`T_VFX_Star_03`** |

**`M_VFX_DisAdd_Rainbow` is the outlier and the interesting one.** It is the only instance in this
system that engages the **gradient-map LUT chain** with a real ramp (`T_VFX_LUT_Rainbow_01`,
512×2, sRGB) rather than the white pixel, and it is also the only one whose three texture slots
disagree. NS_Lightning_Range §13.3 dropped the gradient chain *because its LUT was a white pixel* —
that justification does not transfer.

**`CamOffset = 50` on `Part03_Bright`** is the second unimplemented parameter: a camera-toward
world-position offset (the family's `WorldPosition` + `CamOffset` chain). Unplumbed in CkUsf today.

### 4.2 `M_VFX_Bomb` family — `Parents/MI_VFX_Bomb`

| Fact | Value |
|---|---|
| Parent | `Parents/M_VFX_Bomb` |
| Blend / shading | **`BLEND_Opaque`**, `MSM_Unlit`, `twoSided: false`, `MD_Surface` |
| Connected outputs | **`EmissiveColor` only** |
| Dynamic parameters | **none** |
| Expressions | `Multiply ×6`, `LinearInterpolate ×4`, `ComponentMask ×4`, `Constant ×3`, `TextureCoordinate ×3`, `VectorParameter ×5`, `Step ×2`, `ScalarParameter ×2`, `Abs ×1`, `Reroute ×1`, `Sine ×1`, `ParticleColor ×1` |
| `Glow` / `Shadow` (scalars) | 2 / 0.6 |
| `Color_01` | `RGBA(0.093, 0.819748, 3, 0)` |
| `Color_02` | `RGBA(0.228, 1.557, 2, 0)` |
| `Color_03` | `RGBA(0.019065, 0.021971, 0.031, 1)` |
| `Shadow` (vector) | `RGBA(0.635442, 0.724198, 1, 1)` |
| `MainColor` | `RGBA(1, 1, 1, 0)` |
| Textures | **none** |

A toon prop material: three flat colours banded by `Step` over `TextureCoordinate`, plus a `Sine`,
modulated by `ParticleColor`. The `Bomb` emitter's `Index 0 Param 1 = 1` is therefore **inert** —
this material reads no dynamic parameters.

### 4.3 Textures referenced `[corpus]`

| Texture | Size | sRGB | Compression | Address | Format | Role |
|---|---|---|---|---|---|---|
| `T_VFX_Part_01` | 512×512 | false | `TC_Alpha` | `TA_Clamp` | `TSF_G8` | Part01 Main/Color/Dissolve; Rainbow GradientShape + Color/Dissolve |
| `T_VFX_Part_02` | 512×512 | false | `TC_Alpha` | `TA_Clamp` | `TSF_G8` | Part01_Bright, Part02 |
| `T_VFX_Part_03` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G8` | Part03, Part03_Bright |
| `T_VFX_Ring_01` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | Ring01 |
| `T_VFX_Ring_02` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | Rainbow Main_Tex |
| `T_VFX_Star_03` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | Star03 |
| `T_VFX_Noise_02` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | Distortion / GradientShape on most |
| `T_VFX_LUT_Rainbow_01` | **512×2** | **true** | `TC_Default` | `TA_Wrap` | `TSF_BGRA8` | Rainbow GradientMap |
| `T_VFX_WhitePixel` | 1×1 | true | `TC_Default` | `TA_Wrap` | `TSF_RGBA16` | GradientMap no-op elsewhere |

Existing procedural stand-ins that fit without new work: `T_VFX_Part_01` → `SoftParticle`
(NS_BasicAttack §7 measured it as perfectly radially symmetric, `pow(1-r, 2.2)`); `T_VFX_Noise_02` →
`TileNoise` (measured match, same §7); `T_VFX_Ring_01` → the existing SDF `Ring` bake **is a
candidate but was never measured against this asset** — measure before claiming it.
**New bakes needed:** `T_VFX_Part_02`, `T_VFX_Part_03`, `T_VFX_Ring_02`, `T_VFX_Star_03`, and a
**new colour-LUT output kind** for `T_VFX_LUT_Rainbow_01` (the generator bakes greyscale masks
only — a 512×2 sRGB colour ramp is a new kind of output, not just a new function).

---

## 5. Per-emitter runtime facts and exact curves `[corpus]`

`t` = NormalizedAge over each emitter's own lifetime. `C` = constant key, `L` = linear key.
Where a curve's first key is at `t > 0`, Niagara clamps to that key's value for all earlier `t`.

Shared boilerplate on every emitter unless contradicted: `Color Mode = Direct Set`,
`Color Channel Mode = Link RGB / Link A`, `Position Mode = Simulation Position`,
`Position Offset = (0,0,0)`, `Particle State → Kill Particles When Lifetime Has Elapsed = true`,
`Write Parameter Index 0 = true` with indices 1–3 false, `ScaleColor.Scale Alpha = 1`,
`ScaleColor.Scale RGB = (1,1,1)`, `ScaleSpriteSize.Initial Sprite Size = (0,0)`,
`Uniform Curve Index/Scale = 0/1`.

### 5.1 `Glow_01` / `Glow_02` / `Bomb_Glow` / `Glow_03` — the four Part01 glows

| | `Glow_01` | `Glow_02` | `Glow_03` | `Bomb_Glow` |
|---|---|---|---|---|
| Count / spawn t | 1 / 0 | 1 / 0 | **3** / **0.05** | **3** / **0.05** |
| Lifetime | 1 | 1 | 1 | 1 |
| `Uniform Sprite Size` | **500** | **250** | **200** | **300** |
| Init colour | `RGBA(0.0368895, 0.184475, 1, 0.5)` | `RGBA(0.0193824, 0.0451862, 0.913099, 1)` | `RGBA(0.0466651, 0.491021, 1, 1)` | `RGBA(0.0368895, 0.184475, 1, 0.5)` |
| Dyn `Index 0 Param 1` | **1** | **0** | **2** | **2** |
| Dyn Param 2/3/4 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 |
| `Scale Color` module | enabled | enabled | enabled | **DISABLED** |
| Scale RGBA curve | R/G/B `(0,1)L (1,1)L`; **A `(0,1)L (1,0)L`** | same | same | same *(does not execute)* |
| `Uniform Curve Sprite Scale` | `(0, 0.5)C (0.1, 1)L (1, 1)L` | `(0, 0.5)C (0.1, 1)L (1, 1)L` | **`(0, 0)C (0.2, 1)L (1, 1)L`** | **`(0, 0)C (0.2, 1)L (1, 1)L`** |
| Update module order | 1 Particle State, 2 Scale Color, 3 Dynamic Material Parameters, 4 Scale Sprite Size | same | same | same (2 disabled) |

**`Bomb_Glow`'s three sprites are identical and its alpha fade is disabled** — three coincident
sprites holding alpha 0.5 for the full second, growing from 0 to full size over the first 20 %.

### 5.2 `Raimbow` — 1 sprite, lifetime 0.3 s, spawn t = 0.1

| Fact | Value |
|---|---|
| Init colour | `RGBA(0.913099, 0.913099, 0.913099, 0.15)` |
| Size | `Sprite Size Mode = Uniform`, `Uniform Sprite Size = 300` |
| Sprite rotation | `Sprite Rotation Mode = Random`, angle min **0** max **360** |
| Dyn params | `Index 0 Param 1 = 0.5`, `2/3/4 = 0` |

`Scale Color` (`RGBA Together`):

| Channel | Keys |
|---|---|
| Red | `(0, 0.5)L` |
| Green | `(0, 0.5)L` |
| Blue | `(0, 0.5)L` |
| Alpha | `(0, 1)L (1, 0)L` |

`Uniform Curve Sprite Scale`: `(0, 0.5)C (0.2, 0.9)C (1, 1)L`.
Update order: 1 Particle State, 2 Scale Color, 3 Dynamic Material Parameters, 4 Scale Sprite Size.

### 5.3 `Ring01` — 1 sprite, lifetime 0.75 s, spawn t = 0.05

| Fact | Value |
|---|---|
| Init colour | `RGBA(1, 1, 1, 1)`; module-level **`Color.Scale Alpha = 1`**, `Scale Color = (1,1,1)` |
| Lifetime | Direct Set **0.75** (`Lifetime Min/Max = 0.3/0.7` stored but unused — mode is Direct Set) |
| Size | Uniform **170** (`Uniform Sprite Size Min/Max = 150/160` stored, unused) |
| Sprite rotation | Random, 0…360 |
| Dyn Param 2/3/4 | 0 / 0 / 0 |

`Dynamic Material Parameters → Index 0 Param 1` (**`dissolve`**), `Float from Curve`:
**`(0, 1)C (1, -1)C`**. `FloatFromCurve.Scale Curve = 1`.

`Color` (`Color from Curve`) — this emitter uses the **Color** module, not Scale Color:

| Channel | Keys |
|---|---|
| Red | `(0, 0.191202)L (0.405675, 0.191202)L (1, 0.79)C` |
| Green | `(0, 0.318547)L (0.405675, 0.318547)L (1, 0.823064)C` |
| Blue | `(0, 1)L (0.405675, 1)L (1, 1)C` |
| Alpha | `(0, 1)C` |

`Uniform Curve Sprite Scale`: `(0, 0)C (0.2, 0.7)C (1, 1)C`.
Update order: 1 Particle State, 2 Dynamic Material Parameters, 3 Color, 4 Scale Sprite Size.

### 5.4 `Flash_Glow_01` / `Flash_Glow_02` — 1 sprite each, lifetime 0.5 s, spawn t = 0.1

| | `Flash_Glow_01` | `Flash_Glow_02` |
|---|---|---|
| `Uniform Sprite Size` | **30** | **50** |
| Init colour | `RGBA(0.102242, 0.658375, 1, 0.2)` | `RGBA(0.102242, 1, 0.838799, 0.3)` |
| Dyn `Param 1` | 1 | 1 |
| Scale RGBA | R/G/B `(0,1)L (1,1)L`; A `(0,1)L (1,0)L` | identical |
| `Uniform Curve Sprite Scale` | `(0, 0.5)C (0.1, 1)L (1, 1)L` | identical |

Update order on both: 1 Particle State, 2 Scale Color, 3 Dynamic Material Parameters,
4 Scale Sprite Size.

### 5.5 `Sparkles` — 10 sprites, lifetime rand 0.5–1.0 s, spawn t = 0.05

| Fact | Value |
|---|---|
| Lifetime | `[corpus-v3]` `Lifetime Mode = Random` ⇒ **`Lifetime Min 0.5 / Max 1.0` DRIVES** (`lifetimeResolved.source = minmax`). The `dyn:Random Range Float` override (0.2 / 0.4) sits on the unselected Direct-Set pin and is INERT (`inertOverrides`). *The sheet's parenthetical was right and its `[unresolved]` note — "the override wins in Niagara, so 0.2–0.4 is the more likely live range" — was WRONG; corrected per [P0-D2].* |
| Spawn shape | **Sphere Location**, `Sphere Radius = 0.5`, `Sphere Orientation Axis = (1,0,0)`, `Non Uniform Scale = (1,1,1)`, `Offset = (0,0,0)`, `Surface Only = false`, `Sphere Distribution = Random`, `Random Seed = 0` — effectively a **point** |
| Velocity | **Add Velocity from Point**, `Origin Offset = (0,0,0)`, `Velocity Falloff Distance = 100`, `Velocity Strength = Random Range Float 001` **min 350, max 500** |
| Size | `Sprite Size Mode = Random Uniform`, min **7**, max **10** |
| Sprite rotation | Random, 0…360 |
| Init colour | `RGBA(1, 1, 1, 1)`; `Color.Scale Alpha = 1` |
| Dyn `Param 1` | 1; `2/3/4 = 0` |
| Solver | `Acceleration Limit = 9999`, `Speed Limit = 1000` — the 1000 limit **does bind** at spawn (strength 350–500 is under it; it does not bind) |

`Scale Velocity` → `Velocity Scale` (`Vector from Curve`), all three axes identical:
**X/Y/Z: `(0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`**. `VectorFromCurve.Scale Curve = (1,1,1)`.

`Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0.615756, 0.296138)C (0.936915, 0)C` |
| Green | `(0.615756, 0.571125)C (0.936915, 0.0896671)C` |
| Blue | `(0.615756, 1)C (0.936915, 1)C` |
| Alpha | `(0.613341, 1)C (1, 0)L` |

`Uniform Curve Sprite Scale`: `(0, 0)C (0.1, 1)C (1, 0)C`.
Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Particle State, 4 Color,
5 Dynamic Material Parameters, 6 Scale Sprite Size.

### 5.6 `Bomb` — 1 mesh, lifetime 1 s, spawn t = 0

| Fact | Value |
|---|---|
| Init colour | `RGBA(1, 1, 1, 1)` |
| Mesh scale | `Mesh Scale Mode = Uniform`, `Mesh Uniform Scale = 0.45` |
| Initial Mesh Orientation | `Orientation Axis = (1,0,0)`, `Orientation Vector = (1,0,0)`; `Rotation = Random Range Vector` **min `(0,0,-1)` max `(0,0,1)`**, `Random Seed = 0` |
| Dyn `Param 1` | 1 — **inert** (`MI_VFX_Bomb` reads no dynamic parameters, §4.2) |
| `Uniform Sprite Size` | 500 — inert (mesh renderer) |

`Scale Color` (`RGBA Together`):

| Channel | Keys |
|---|---|
| Red | **`(0.15, 5)L (0.255961, 0.25)L`** |
| Green | **`(0.15, 5)L (0.255961, 0.25)L`** |
| Blue | **`(0.15, 5)L (0.255961, 0.25)L`** |
| Alpha | `(0, 1)L` |

Read: RGB is **held at 5 from t = 0 to t = 0.15** (an early over-bright flash), falls linearly to
**0.25** by t = 0.255961, and holds 0.25 to death. Alpha constant 1.

`Scale Mesh Size` → `Scale Factor` (`Vector from Curve`), all three axes:
**X/Y/Z: `(0, 0)C (0.2, 1)C`** — pops from zero to full over the first 20 %.

`Update Mesh Orientation` → `Rotation Rate` (`Float from Curve`): **`(0, 5)C (0.5, 0)C`**;
`UpdateMeshOrientation.Rotation Vector = (0, 0, 1)`. The bomb **spins about world +Z**, fast at
spawn, stopped by half-life.

Update order: 1 Particle State, 2 Scale Color, 3 Dynamic Material Parameters, 4 Scale Mesh Size,
5 Update Mesh Orientation. `VectorFromCurve.Scale Curve = (1,1,1)`, `FloatFromCurve.Scale Curve = 1`.

### 5.7 `Flare_Stretched_01…04` — 5 sprites total, all `Sprite Size Mode = Non-Uniform`

All four share one `Scale Color` curve and one `Uniform Curve Sprite Scale`; they differ in colour,
lifetime, sprite size, count, and the tail of the non-uniform scale curve.

Shared `Scale Color` (`RGBA Together`):

| Channel | Keys |
|---|---|
| Red | `(0.218533, 1)L (1, 1)L` |
| Green | `(0.218533, 1)L (1, 1)L` |
| Blue | `(0.218533, 1)L (1, 1)L` |
| Alpha | `(0.225777, 1)L (1, 0)L` |

Shared `Uniform Curve Sprite Scale`: `(0, 0.5)C (0.1, 1)L (1, 1)L`.
Shared X of `Non-Uniform Curve Sprite Scale`: `(0, 1)C (0.9, 2.30968e-08)C` — the streak's **length
collapses to zero by t = 0.9**.

| | `_01` | `_02` | `_03` | `_04` |
|---|---|---|---|---|
| Count / spawn t | 1 / 0 | 1 / 0 | **2** / 0 | 1 / 0 |
| Lifetime | 0.5 | 0.5 | **0.7** | **0.3** |
| `Sprite Size` | `(500, 100)` | `(500, 100)` | **`(1400, 200)`** | **`(400, 50)`** |
| Init colour | `RGBA(0.00182116, 0.0561285, 1, 0.5)` | `RGBA(0.238, 0.502791, 1, 1)` | `RGBA(0.00402472, 0.0295568, 0.130136, 1)` | `RGBA(0.508881, 0.679543, 1, 1)` |
| Dyn `Param 1` | 1 | 1 | **0.5** | 1 |
| Non-Uniform **Y** curve | `(0, 0.3)C (0.2, 1)C` | `(0, 0.3)C (0.2, 1)C (0.9, 0.2)L` | `(0, 0.3)C (0.2, 1)C` | `(0, 0.3)C (0.2, 1)C (0.9, 0.0999999)L` |
| Material | `Part01` | `Part03_Bright` | `Part03` | `Star03` |
| `Uniform Sprite Size` (inert) | 550 | 550 | 550 | 550 |

Update order on all four: 1 Particle State, 2 Scale Color, 3 Dynamic Material Parameters,
4 Scale Sprite Size (carrying both the uniform and non-uniform curves).

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new row in `ck::particles::Get_TemplateSpecs()` is required.**

| Field | Value | Why |
|---|---|---|
| Loop duration | **2.0 s** `[corpus-v3]` | the system's `Once` loop duration ([P0-D3]); the exported `Loop Duration = 1` is a leftover. *Was `[unresolved]`.* |
| Particle lifetime | **1.0 s** | max resolved lifetime — `Glow_01/02/03`, `Bomb_Glow`, `Bomb` at 1.0, and `Sparkles`' resolved `Lifetime Max` is also 1.0 (§5.5) |
| Burst count | **27** | the §2 total |

Layer partition `Seed % 27` — the NS_BasicAttack §8 pattern, with the layer→emitter map straight off
the §2 table (indices 0…26 in emitter order). Layers past their **own** lifetime write zero colour,
zero size and zero scale, exactly as NS_BasicAttack §8 requires; the three spawn beats (0, 0.05,
0.1 s) are reproduced by hiding a layer for `Age < SpawnDelay` and running its curves on
`(Age − SpawnDelay) / Lifetime` — the same mechanism NS_BasicAttack used for its 0.06 s spark delay.

**Do not approximate onto `PS_CkParticles_Template_Burst`** (1.2 / 1.2 / 96).

### 6.2 VisTag / renderer needs

Every one of the 13 sprite emitters is `Alignment: Unaligned` / `Facing: FaceCamera` — a plain
camera-facing billboard. But they use **eight different materials**, and `Get_BehaviorLookName`
carries exactly one look, bound through `User.SpriteMaterial`. So:

| Need | Kind | Count |
|---|---|---|
| Camera-facing sprite, per-look | **new `CameraFacingSprite` row-renderer kind** (see gap 1) | 8 distinct looks |
| Mesh, `Facing: Default`, one mesh + one look | existing `ECk_ParticlesRenderer_Kind::Mesh` | 1 |

Eight row renderers is a lot, but it is *data*, and the NS_BasicAttack §8.1 capability already emits
row-declared renderers for one row only. Consolidation is possible where two emitters share a
material (`Part01` serves five emitters, `Part02` serves two) — **5 sprite looks + 1 mesh look = 6
row renderers** is the honest floor:

| Row renderer | Kind | Look (proposed) | Serves |
|---|---|---|---|
| a | CameraFacingSprite | `BombGlowDisAdd01` | `Glow_01`, `Glow_02`, `Glow_03`, `Bomb_Glow`, `Flare_Stretched_01` |
| b | CameraFacingSprite | `BombSparkDisAdd01B` | `Sparkles` |
| c | CameraFacingSprite | `BombFlashDisAdd02` | `Flash_Glow_01`, `Flash_Glow_02` |
| d | CameraFacingSprite | `BombRingDisAdd01` | `Ring01` |
| e | CameraFacingSprite | `BombRainbowDisAdd` | `Raimbow` |
| f | CameraFacingSprite | `BombFlareDisAdd03` (+ `…03B`, `…Star03`) | `Flare_Stretched_02/03/04` — three distinct materials, so **three** renderers unless the Brightness/texture deltas are folded into a dynamic-parameter channel |
| g | Mesh | `BombProp` on a bomb mesh | `Bomb` |

That is **6–8 row renderers**. VisTag ids are allocated at implementation time from
`ck::particles::Get_RosterVisTag_Max()`; **do not allocate them now.**

### 6.3 Mesh needs

Same decision as [NS_Bomb_Projectile.md](NS_Bomb_Projectile.md) §6.3, and it should be made **once**
for both sheets: `SM_VFX_Bomb_01_Small` has a non-derivable UV atlas (§3) that `MI_VFX_Bomb` bands
by `Step` (§4.2). Options: (1) procedural sphere + re-authored object-space banding, (2) import the
mesh skip-if-present into `/CkFoundation/CkParticles/Imported/Vefects/`, (3) ship without the prop.
**Maintainer decision, required before the implementation session.**

### 6.4 Look / texture needs

Six to eight CkUsf looks, **all parameterizations of the existing `CkUsf_Look_DissolveAdd`** except
`BombProp` (a new small `M_VFX_Bomb` family shader). Two family parameters must be plumbed first:

- **`CamOffset`** (50 on `Part03_Bright`) — a world-position offset toward the camera. Unimplemented.
- **The gradient-map LUT chain** (`Rainbow` uses a real 512×2 ramp). Unimplemented; NS_Lightning_Range
  §13.3's "it's a white pixel, so it's a no-op" justification does not apply here.

Textures: `SoftParticle` and `TileNoise` cover `T_VFX_Part_01` / `T_VFX_Noise_02` from prior
measurements. **New bakes: `T_VFX_Part_02`, `T_VFX_Part_03`, `T_VFX_Ring_02`, `T_VFX_Star_03`**
(measure each PNG per the NS_BasicAttack §7 method — 32-bin per-axis profiles, structure tensor,
mean absolute difference, zero-crossings, radial ring means — then bake from the numbers, never
copy pixels), a **check** of whether the existing SDF `Ring` bake matches `T_VFX_Ring_01`, and a
**new colour-LUT output kind** for `T_VFX_LUT_Rainbow_01`.

### 6.5 Capability gap callout

| # | Gap | Severity |
|---|---|---|
| 1 | **No row-declared camera-facing sprite kind.** `FCk_ParticlesRendererSpec` has exactly `Mesh` and `VelocityAlignedSprite`. All 13 sprite emitters here are `Unaligned`/`FaceCamera` and need **five to eight different looks**, which one `User.SpriteMaterial` binding cannot carry. Additive fix, but this effect **cannot ship without it**. | **BLOCKING** |
| 2 | **System-level loop parameters absent from the corpus** (§2). Cadence cannot be finalized. Fix by extending CkAssetExporter to emit the System Spawn / System Update stacks, or by an `[EDITOR-VERIFY]` read. | **Prerequisite** |
| 3 | **Gradient-map LUT chain unimplemented in CkUsf's DissolveAdd family**, and the texture generator has **no colour-LUT bake kind** (greyscale masks only). `Raimbow` is one of 14 emitters, so the effect degrades rather than fails — but the "rainbow" is literally the LUT. | Medium |
| 4 | **`CamOffset` unimplemented** (`Part03_Bright`, 50). A depth-sorting nudge; visually minor. | Low |
| 5 | **World space.** All 14 emitters are `LocalSpace: false`; the CkParticles template is local-space (NS_BasicAttack §13.2). Visible only if the spawning actor moves during the 1 s life. For a "bomb spawn" one-shot at a fixed point, low risk. | Low |
| 6 | **Hand-authored prop mesh with a non-derivable UV atlas** (§3, §6.3). Content decision, not a pipeline gap. | Decision required |
| 7 | **`Opacty_DepthFade`** is 20 on most instances and 10 on `Star03`. CkUsf surface looks do not wire scene depth — the same gap NS_Lightning_Range §13.4 and NS_BasicAttack §13.3 already record. | Low (known) |

**Complexity tier: M**, conditional on gap 1 landing. Nothing here needs a renderer *class* the
pipeline lacks (no ribbons, no sub-UV, no GPU sim, no collision, no events, no user parameters, no
light renderers) — it needs one new *kind* in an existing enum, a cadence row, ~7 looks, 4 texture
bakes, and one content decision about the bomb mesh. **If gap 1 is treated as out of scope, the
tier is L**, because there is no honest way to draw eight materials through one binding.

### 6.6 Behavior id

**Do not allocate an id in this document.** Take the next free id from
`ck::particles::NumBehaviors` at implementation time, bump it, and update the roster paragraph in
`CkParticles/CLAUDE.md`. Five sibling sheets were written in the same pass and none allocates an id
— allocate them in one ordered pass so they cannot collide.

---

## 7+. Reserved for implementation

Sections 7–14 of the recipe schema ([README.md](README.md)) are intentionally absent and are to be
written by the implementation session, from what actually happened.
