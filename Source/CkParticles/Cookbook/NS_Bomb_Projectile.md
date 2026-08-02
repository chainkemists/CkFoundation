# Recipe: NS_Bomb_Projectile → CkParticles (PRE-IMPLEMENTATION TRANSLATION SHEET)

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
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Bomb_Projectile` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior id | **not allocated** — take the next free id at implementation time from `ck::particles::NumBehaviors` |
| Recreation status | not started |

Corpus evidence (all `[corpus]`, exported 2026-08-01; `Saved/` is machine-local and regenerable):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Bomb_Projectile.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/{M_VFX_DisAdd_Part01,M_VFX_DisAdd_Trail01}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/{MI_VFX_Bomb,M_VFX_FlatAdd}.json`
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_Bomb_01_Small.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/*.json`

**The source Niagara asset was not opened for editing** and must not be. Every fact below is
`[corpus]` unless tagged otherwise.

> ### A same-named sibling exists — take the right one
> `[corpus]` The pack ships a second `NS_Bomb_Projectile` at
> `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Bomb_Projectile`. **Fastest discriminator: the
> sibling declares three user parameters** (`User.Glow Color 01`, `User.Scale Overall`,
> `User.Trail Color 01`) **and renders through `MI_VFX_*` instances**
> (`MI_VFX_Glow_01`, `MI_VFX_Bomb`, `MI_VFX_Trail_01`). The `Anime_VFX/Shared/Skills` variant
> documented here has an **empty user-parameter list** and renders through `M_VFX_DisAdd_*`
> instances in `Anime_VFX/Shared/Materials/`. Emitter names and counts are otherwise near-identical,
> so the user-parameter list is the only cheap tell.

---

## 2. System anatomy `[corpus]`

**3 CPU emitters. 2 burst-at-loop-start sprite/mesh emitters (LOCAL space) + 1 distance-driven
ribbon trail (WORLD space). 4 particles per burst; the ribbon count is unbounded and movement-driven.**

| Emitter | Enabled | Sim | Space | Spawn | Count | Spawn t | Lifetime | Renderer | Mesh | Material |
|---|---|---|---|---|---|---|---|---|---|---|
| `Bomb_Glow` | yes | CPU | **Local** | Burst Instantaneous | **3** | 0 | 2.5 | Sprite, Unaligned / FaceCamera, Sort None | — | `M_VFX_DisAdd_Part01` |
| `Bomb` | yes | CPU | **Local** | Burst Instantaneous | **1** | 0 | 2.5 | Mesh, Facing **Default**, Sort None | `SM_VFX_Bomb_01_Small` | `Parents/MI_VFX_Bomb` |
| `Bomb_Trail` | yes | CPU | **World** | **Spawn Per Unit** (+ a DISABLED Spawn Rate 50) | — | — | 0.6 | **Ribbon** | — | `M_VFX_DisAdd_Trail01` |

Per-emitter `Emitter State`: `Loop Behavior = Infinite`, `Loop Duration Mode = Fixed`,
`Loop Duration = 1`, `Loop Delay = 0`, `UseLoopDelay = false`, `UseLoopCountLimit = false`
(`Loop Count Limit = 1` is therefore an inert leftover, exactly as in NS_Lightning_Range §4).
`Bounds: Dynamic` on all three. `Determinism: false`. `Inactive Response = Complete`.

> **`Life Cycle Mode = System` on all three emitters `[corpus]`.** The per-emitter Loop
> Behavior / Loop Duration values above are therefore **driven by the system**, not by the emitter,
> and the numbers stored on the module are leftovers.
>
> **System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.5 s`, `Loop Delay = 0`,
> `UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
> Per [P0-D1] this is the authority. Note **2.5 s, not the 2.0 s every other Vefects Skills system
> uses** — it matches this system's 2.5 s particle lifetimes exactly. *(Was `[unresolved]`.)*
> Spawn **times** and burst **counts** are live and are quoted as read.

Total burst particles per firing: **4** (3 glow + 1 bomb). Ribbon points are emitted per 20 units of
travel (see §5), so a projectile that never moves emits no trail at all.

`Bomb_Trail`'s disabled `Add Velocity` carries `Velocity = (-3000, 0, 0)`, `Scale Added Velocity =
(1,1,1)` — recorded because it names the intended travel axis (**−X**), not because it executes.

---

## 3. Mesh geometry `[corpus]`

### `SM_VFX_Bomb_01_Small` — the bomb prop

| Fact | Value |
|---|---|
| Verts / tris | **6066 / 3128** |
| UV channels | 2 |
| Bounds min | `(-87.26517486572266, -91.60921478271484, -81.66767120361328)` |
| Bounds max | `(87.26517486572266, 91.7042007446289, 81.66767120361328)` |
| Bounds size | `(174.5303497314453, 183.31341552734375, 163.33534240722656)` |
| uv0 range | `(0, 0.04724121)` … `(1, 0.8491211)` |
| Section 0 | slot `WorldGridMaterial` → `Parents/MI_VFX_Bomb`, 3128 tris |

OBJ characterization (measured with a throw-away parser over `v`/`vt`/`f`; per NS_BasicAttack §3
method) `[corpus, derived from the .obj]`:

- Radius-from-origin (3D): **min 69.161, max 94.084, mean 81.818** — a near-spherical body with
  surface detail, not a primitive sphere (a primitive would be constant-radius).
- `radiusXY` per Z-octile: `0:{0.0..47.5} 1:{12.8..56.9} 3:{73.9..92.5} 4:{74.3..92.4}
  5:{73.9..92.0} 7:{12.8..56.9} 8:{0.0..47.5}` — symmetric about Z, widest at the equator: a ball.
  Octiles 2 and 6 are empty of paired verts, i.e. the tessellation is banded.
- **UVs are an authored atlas, not a projection.** `corr(u, angleXY) = −0.131`,
  `corr(u, radiusXY) = −0.052`, `corr(u, Z) = 0.083`; `corr(v, radiusXY) = 0.175`,
  `corr(v, Z) = −0.058`. Every correlation is near zero — there is **no cylindrical, spherical, or
  planar UV mapping to re-derive**. `v` spans only 0.047…0.849, i.e. the shell occupies a sub-rect
  of the atlas.

**Consequence for the recreation: this mesh is NOT procedurally reproducible** the way
NS_BasicAttack's crescent was. Its silhouette is a ball (easy), but its UV atlas is hand-authored
and the bomb material (§4) samples `TextureCoordinate` three times through `Step`/`Lerp` bands —
so a re-UV'd procedural sphere would band in the wrong places. Options are named in §6.

---

## 4. Material families and per-instance deltas `[corpus]`

Three *different* parent graphs are involved. Family reference for the delta tables is
`M_VFX_DisAdd_Part01` (the same DissolveAdd family NS_BasicAttack §4 and NS_Lightning_Range §7
already characterize).

### 4.1 `M_VFX_DissolveAdd` family — `M_VFX_DisAdd_Part01`, `M_VFX_DisAdd_Trail01`

Both are instances of `/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd`.
Base properties, identical on both: `MD_Surface`, **`BLEND_Translucent`**, **`MSM_Unlit`**,
`twoSided: false`, connected outputs **`EmissiveColor` + `Opacity`** only, dynamic-parameter channel
names **`[dissolve, distortion, offset, core_color]`**.

Expression histogram (identical on both, the family fingerprint): `ScalarParameter ×41`,
`Multiply ×18`, `Add ×18`, `AppendVector ×18`, `Saturate ×12`, `DynamicParameter ×8`, `Reroute ×8`,
`TextureSampleParameter2D ×6`, `Panner ×5`, `Constant ×5`, `TextureCoordinate ×5`,
`LinearInterpolate ×4`, `Clamp ×2`, `OneMinus ×2`, `DepthFade ×1`, `ParticleColor ×1`, `Power ×1`,
`SmoothStep ×1`, `StaticBoolParameter ×1`, `StaticSwitch ×1`, `VectorParameter ×1`,
`MaterialFunctionCall ×1`, `WorldPosition ×1`.

`M_VFX_DisAdd_Part01` **is** the delta reference. Its effective values, quoted because the delta
tables in every sheet in this batch are relative to them:

| Parameter | Value |
|---|---|
| `Main_Tex` / `Color_Tex` / `Dissolve_Tex` | `T_VFX_Part_01` |
| `Distortion_Tex` / `GradientShape_Tex` | `T_VFX_Noise_02` |
| `GradientMap_Tex` | `T_VFX_WhitePixel` (1×1 — a no-op gradient map) |
| `Brightness` | 1 |
| `Opacity_Boldness` | 0.5 |
| `Distortion_Intensity` | 0 |
| `Dissolve`, `Dissolve_Invert` | 0, 0 |
| `Dissolve_Scale_X/Y`, `Dissolve_Speed_X/Y` | 1, 1 / 0, 0 |
| `MainTex_Scale_X/Y`, `_Speed_X/Y`, `_Offset_X/Y` | 1, 1 / 0, 0 / 0, 0 |
| `Color_Scale_X/Y`, `_Speed_X/Y`, `_Offset_X/Y` | 1, 1 / 0, 0 / 0, 0 |
| `Distortion_Scale_X/Y`, `_Speed_X/Y` | 1, 1 / 0, 0 |
| `GradientShape_Scale_X/Y`, `_Speed_X/Y` | 1, 1 / 0, 0 |
| `Gradient_Invert` | 0.5 |
| `GradientMap_Displacement` | 0.10000000149011612 |
| `Color_CoreDifferent`, `Core_Power`, `Core_Intensity` | 0, 1, 0 |
| `Glow_Intensity` | 1 |
| `Opacty_Step`, `Opacty_StepAdd`, `Opacty_DepthFade` | 0, 0.10000000149011612, 20 |
| `CamOffset` | 0 |
| `Color_Core` | `RGBA(1, 1, 1, 0)` |

| Instance | Delta vs `M_VFX_DisAdd_Part01` |
|---|---|
| `M_VFX_DisAdd_Part01` | *(reference)* |
| `M_VFX_DisAdd_Trail01` | `Brightness` 1 → **6**; `Color_CoreDifferent` 0 → **1**; `Core_Intensity` 0 → **2**; `Dissolve_Speed_X` 0 → **−1**; `Distortion_Intensity` 0 → **0.3**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **30**; `Main_Tex`/`Color_Tex` → **`T_VFX_Wind_02`**; `Dissolve_Tex` → **`T_VFX_Noise_06`**; `GradientMap_Tex` → **`T_VFX_LUT_Bomb_01`**; `Color_Core` → **`RGBA(0.057805, 0.313989, 0.590619, 1)`** |

`M_VFX_DisAdd_Trail01` is the only material in this batch that engages the **gradient-map** chain
with a real LUT (`T_VFX_LUT_Bomb_01`, 512×2, sRGB, `TC_Default`) instead of the white pixel. That
chain is currently unimplemented in the CkUsf DissolveAdd family (NS_Lightning_Range §13.3 dropped
it as a no-op *because* its LUT was a white pixel — here it is not).

### 4.2 `M_VFX_Bomb` family — `Parents/MI_VFX_Bomb`

| Fact | Value |
|---|---|
| Parent | `/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_Bomb` |
| Blend / shading | **`BLEND_Opaque`**, `MSM_Unlit`, `twoSided: false`, `MD_Surface` |
| Connected outputs | **`EmissiveColor` only** (no Opacity) |
| Dynamic parameters | **none** — this material does not read `Particles.DynamicMaterialParameter` |
| Expressions | `Multiply ×6`, `LinearInterpolate ×4`, `ComponentMask ×4`, `Constant ×3`, `TextureCoordinate ×3`, `VectorParameter ×5`, `Step ×2`, `ScalarParameter ×2`, `Abs ×1`, `Reroute ×1`, `Sine ×1`, `ParticleColor ×1` |
| `Glow` | 2 |
| `Shadow` (scalar) | 0.6 |
| `Color_01` | `RGBA(0.093, 0.819748, 3, 0)` |
| `Color_02` | `RGBA(0.228, 1.557, 2, 0)` |
| `Color_03` | `RGBA(0.019065, 0.021971, 0.031, 1)` |
| `Shadow` (vector) | `RGBA(0.635442, 0.724198, 1, 1)` |
| `MainColor` | `RGBA(1, 1, 1, 0)` |
| Textures | **none** — fully procedural from UV + `Step`/`Lerp`/`Sine` |

**This is a toon-shaded prop material, not a VFX additive material**: opaque, no opacity output, no
texture samples, three flat colours banded by `Step` over `TextureCoordinate` plus a `Sine`. It is
*cheap to recreate* as a CkUsf look — but it depends entirely on the mesh's authored UVs (§3), which
is the real cost.

### 4.3 `M_VFX_FlatAdd` (parent, referenced only indirectly here)

Quoted because it is the second family in this batch and other sheets reference it:
`BLEND_Translucent`, `MSM_Unlit`, outputs `EmissiveColor` + `Opacity`, **no dynamic parameters**,
**no texture parameters**; scalars `Brightness = 1`, `Opacty_DepthFade = 0`, `CamOffset = 0`;
vector `Color_Core = RGBA(1,1,1,0)`. Expressions: `Multiply ×2`, `ScalarParameter ×3`,
`VectorParameter ×1`, `ParticleColor ×1`, `DepthFade ×1`, `WorldPosition ×1`,
`MaterialFunctionCall ×1`. It is *ParticleColor × Brightness with a depth fade*, nothing more.

### 4.4 Textures referenced `[corpus]`

| Texture | Size | sRGB | Compression | Address | Source format | Used by |
|---|---|---|---|---|---|---|
| `T_VFX_Part_01` | 512×512 | false | `TC_Alpha` | `TA_Clamp`/`TA_Clamp` | `TSF_G8` | `Part01` Main/Color/Dissolve |
| `T_VFX_Noise_02` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `Part01` Distortion/GradientShape |
| `T_VFX_Wind_02` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G8` | `Trail01` Main/Color |
| `T_VFX_Noise_06` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `Trail01` Dissolve |
| `T_VFX_LUT_Bomb_01` | **512×2** | **true** | `TC_Default` | `TA_Wrap` | `TSF_BGRA8` | `Trail01` GradientMap |
| `T_VFX_WhitePixel` | 1×1 | true | `TC_Default` | `TA_Wrap` | `TSF_RGBA16` | `Part01` GradientMap (no-op) |

Existing procedural stand-ins in `CkParticles_TextureGenerator.cpp` that could cover these without a
new bake: `T_VFX_Part_01` → `SoftParticle` (NS_BasicAttack §7 measured `T_VFX_Part_01` as
"perfectly radially symmetric, `pow(1-r, 2.2)`"); `T_VFX_Noise_02` → existing `TileNoise` (measured
match, NS_BasicAttack §7). **New bakes needed:** `T_VFX_Wind_02` (a streaky trail paint — `WindBand`
is the nearest existing bake but was parameterized off `T_VFX_Wind_03`, a different asset, so it
must be re-measured before reuse), `T_VFX_Noise_06`, and a **1-D colour LUT** for
`T_VFX_LUT_Bomb_01` — the generator currently bakes only greyscale masks, so a 512×2 sRGB colour
ramp is a new *kind* of output, not just a new function.

---

## 5. Per-emitter runtime facts and exact curves `[corpus]`

`t` = NormalizedAge (0 → 1 over the particle's own lifetime). `C` = constant key, `L` = linear key.
Curves are transcribed verbatim from the corpus's rasterized key lists. Where a curve's first key is
at `t > 0`, Niagara clamps to that key's value for all earlier `t`.

### 5.1 `Bomb_Glow` — 3 sprites, lifetime 2.5 s

| Fact | Value |
|---|---|
| Spawn | Burst Instantaneous, count **3**, spawn time **0**, `Age = 0`, `Spawn Probability = 1` |
| Position | `Position Mode = Simulation Position`, `Position Offset = (0, 0, 0)`, `UsePositionOffset = false` |
| Initialize colour | `RGBA(0.0368895, 0.184475, 1, 0.5)` — `Color Mode = Direct Set`, `Color Channel Mode = Link RGB / Link A` |
| Sprite size | `Sprite Size Mode = Uniform`, `Uniform Sprite Size = 300` |
| Lifetime | `Lifetime Mode = Direct Set`, **2.5** |
| Dynamic params (Spawn AND Update) | `Index 0 Param 1 = 2`, `Param 2 = 0`, `Param 3 = 0`, `Param 4 = 0`; `Write Parameter Index 0 = true`, indices 1–3 false |
| `Scale Color` | **DISABLED** — its authored curve (`Red/Green/Blue: (0,1)L (1,1)L`, `Alpha: (0,1)L (1,0)L`) does **not** execute; `ScaleColor.Scale Alpha = 1`, `Scale RGB = (1,1,1)` |
| Update modules | 1 Particle State (`Kill Particles When Lifetime Has Elapsed = true`), 2 Scale Color *(disabled)*, 3 Dynamic Material Parameters |

**All three glow sprites are identical** — no randomness of any kind is applied. Three coincident
sprites at the same position, size and colour: the source is stacking additive alpha 3×.

### 5.2 `Bomb` — 1 mesh, lifetime 2.5 s

| Fact | Value |
|---|---|
| Spawn | Burst Instantaneous, count **1**, spawn time **0** |
| Initialize colour | `RGBA(1, 1, 1, 1)` |
| Mesh scale | `Mesh Scale Mode = Uniform`, `Mesh Uniform Scale = 0.45` |
| Lifetime | Direct Set, **2.5** |
| `Uniform Sprite Size` | 500 — inert (mesh renderer) |
| Initial Mesh Orientation | `Rotation Coordinate Space = Mesh`; `Use Orientation Vector = true`, `Use Rotation Vector = true`; `Orientation Axis = (1, 0, 0)`, `Orientation Vector = (1, 0, 0)`; `Rotation = Random Range Vector`, **min `(0, 0, -1)`, max `(0, 0, 1)`**, `Random Seed = 0` |
| Dynamic params | `Index 0 Param 1 = 1`, `2/3/4 = 0` — **inert**: `MI_VFX_Bomb` declares no dynamic parameters (§4.2) |

`Scale Color` (**ENABLED**), `Scale Mode = RGBA Together`, `ScaleRGBA = true`:

| Channel | Keys |
|---|---|
| Red | `(0.913975, 0.25)L (1, 5)L` |
| Green | `(0.913975, 0.25)L (1, 5)L` |
| Blue | `(0.913975, 0.25)L (1, 5)L` |
| Alpha | `(0, 1)L` |

Read: RGB is **held at 0.25 from t = 0 to t = 0.913975**, then ramps linearly to **5** at t = 1 —
a hard bright flash in the last ~8.6 % of the 2.5 s life (≈ the last 0.215 s). Alpha is a single
key, so constant 1. `ScaleColor.Scale Alpha = 1`, `Scale RGB = (1, 1, 1)`.

Update modules: 1 Particle State, 2 Scale Color, 3 Dynamic Material Parameters. **No mesh rotation
over life** on this emitter (unlike NS_Bomb_Spawn's `Bomb`, which adds Scale Mesh Size + Update Mesh
Orientation).

### 5.3 `Bomb_Trail` — ribbon, lifetime 0.6 s, WORLD space

| Fact | Value |
|---|---|
| Spawn module | **Spawn Per Unit** (`Use Spawn Probability = false`) |
| `Spawn Spacing` | **20** |
| `Movement Tolerance` | 0.5 |
| `Max Movement Threshold` | 5000 |
| `Spawn Probability` | 0.5 *(stored; `Use Spawn Probability = false`, so inert)* |
| Second spawn module | **Spawn Rate 50 — DISABLED** (`SpawnRate.SpawnRate = 50`, `Spawn Probability = 1`) |
| Initialize Ribbon | `Color Channel Mode = Link RGBA`, `Color Mode = Direct Set`, colour `RGBA(0.0865005, 0.254152, 0.838799, 1)`; `Lifetime Mode = Direct Set`, **0.6**; `Ribbon Width Mode = Direct Set`, **`Ribbon Width = 200`**; `Position Mode = Simulation Position`, offset `(0,0,0)` |
| `Add Velocity` (Spawn) | **DISABLED** — `Velocity = (-3000, 0, 0)`, `Scale Added Velocity = (1,1,1)` |
| Solve Forces and Velocity | `Clamp Velocity = false`, `Limit Acceleration = false`, `Rotational Solver Is Enabled = true`, `Write to Presolve Properties = true`; `Acceleration Limit = 9999`, `Speed Limit = 1000` (neither binds — no forces act) |
| Dynamic params | `Write Parameter Index 0 = true`; `Param 2/3/4 = 0` |

`Dynamic Material Parameters → Index 0 Param 1` (**`dissolve`** channel):
`Float from Curve` — `(0, 0.5)C (1, -0.5)C`. `FloatFromCurve.Scale Curve = 1`.

`Scale Color`, `Scale Mode = RGBA Together`, `ScaleRGBA = true`:

| Channel | Keys |
|---|---|
| Red | `(0, 1)L (1, 1)L` |
| Green | `(0, 1)L (1, 1)L` |
| Blue | `(0, 1)L (1, 1)L` |
| Alpha | **`(0, 0.2)L (1, 0)L`** |

Update module order: 1 Solve Forces and Velocity, 2 Particle State, 3 Dynamic Material Parameters,
4 Scale Color. There is **no** Scale Ribbon Width module on this emitter — the ribbon holds width
200 for its whole 0.6 s life and fades only in alpha, from 0.2 to 0.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new row in `ck::particles::Get_TemplateSpecs()` is required** — no existing row matches.

| Field | Value | Why |
|---|---|---|
| Loop duration | **2.5 s** `[corpus-v3]` | the system's `Once` loop duration ([P0-D3]); the exported per-emitter `Loop Duration = 1` is a leftover. *Was `[unresolved]`.* |
| Particle lifetime | **2.5 s** | max resolved lifetime — both burst emitters use 2.5 |
| Burst count | **4** | 3 `Bomb_Glow` + 1 `Bomb` (§2) |

Layer partition would be `Seed % 4` (0–2 = glow, 3 = bomb), exactly the NS_BasicAttack §8 pattern.
**Do not approximate onto `PS_CkParticles_Template_Burst` (1.2 / 1.2 / 96)** — wrong in all three
numbers, the same mistake NS_BasicAttack §14.1 records.

The loop duration is now resolved mechanically `[corpus-v3]` — the v3 exporter emits the System
Update stack's `System State` module (Phase 0, unit 0.1). No `[EDITOR-VERIFY]` needed. Note the
system loop (2.5 s) and the particle lifetime (2.5 s) coincide here, so a bomb that has not exploded
by the end of its flight is killed by lifetime and by loop end at the same instant.

### 6.2 VisTag / renderer needs

| Source emitter | Needs | Shared set (0–4) covers it? |
|---|---|---|
| `Bomb_Glow` | camera-facing sprite with a DissolveAdd look | **VisTag 0** covers the renderer, but the look must bind per-behavior; `Get_BehaviorLookName` can carry ONE look, and this effect needs two (glow + bomb) → row-declared renderers |
| `Bomb` | **mesh** renderer, one generated mesh + one look, `Facing: Default` | Row-declared `ECk_ParticlesRenderer_Kind::Mesh` — the NS_BasicAttack capability, reusable as-is |
| `Bomb_Trail` | **ribbon** renderer | **NO — see §6.5** |

Two row-declared renderers (one `Mesh`, one sprite) would cover the burst half. VisTag ids are
allocated at implementation time from `ck::particles::Get_RosterVisTag_Max()`; **do not allocate
them now.**

**Note the sprite-kind gap.** `FCk_ParticlesRendererSpec` today has exactly two kinds — `Mesh` and
`VelocityAlignedSprite` (verified in `CkParticles_ScriptDefinition_Naming.h`). There is no
row-declared *camera-facing* sprite kind. `Bomb_Glow` is `Unaligned`/`FaceCamera`, which the shared
VisTag 0 renderer draws — but VisTag 0's material comes from `User.SpriteMaterial`, and this effect
already needs that channel free. **Adding a `CameraFacingSprite` kind to the renderer-spec enum is
the cheapest fix** and it generalizes: every other sheet in this batch hits the same wall.

### 6.3 Mesh needs

`SM_VFX_Bomb_01_Small` — **the hard problem of this effect.** §3 proves the UVs are a hand-authored
atlas with no re-derivable projection, and §4.2 proves `MI_VFX_Bomb` bands its three flat colours by
`Step` over those exact UVs. Three honest options, in increasing fidelity:

1. **Procedural sphere + re-authored banding.** Generate a UV sphere (the generator already builds
   MeshDescription carriers) and rewrite the look to band by *object-space Z* instead of by `v`.
   Loses the atlas detail; gains zero pack dependency. Cheapest, and visibly different.
2. **Import the mesh**, as NS_Lightning_Range §10 imported two textures — skip-if-present,
   skip-if-source-absent, into `/CkFoundation/CkParticles/Imported/Vefects/NS_Bomb_Projectile/`.
   Full fidelity; costs a pack dependency on dev hosts and a licensing posture decision.
3. **Defer**: ship the effect without the bomb prop (glow + trail only) and record it in §13.

This is a **maintainer decision**, exactly like the NS_BasicAttack §6.5 texture decision. It must be
made before the implementation session starts, not during it.

### 6.4 Look / texture needs

| Look (proposed) | Family entry point | Notes |
|---|---|---|
| `BombGlowDisAdd01` | `CkUsf_Look_DissolveAdd` (existing) | pure parameterization of §4.1's Part01 reference values; **no shader work** |
| `BombTrailDisAdd01` | `CkUsf_Look_DissolveAdd` + **new gradient-map chain** | needs `Core_Intensity`, `Color_CoreDifferent` and the `GradientMap_Tex` LUT plumbed (see below) |
| `BombProp` | **new** — a `M_VFX_Bomb` mini-family shader | opaque-ish 3-band toon; no textures; trivial HLSL, but see §6.3 |

**Two DissolveAdd family parameters are not plumbed today and this effect needs both:**
`Core_Intensity` / `Color_CoreDifferent` (Trail01 sets 2 and 1) and the **gradient-map LUT chain**
(Trail01 points it at a real 512×2 ramp, not the white pixel). NS_Lightning_Range §13.3 dropped the
gradient chain *because its LUT was a white pixel* — that justification does not transfer here.

Textures: `SoftParticle` and `TileNoise` already cover Part01's samplers. New work: a re-measured
streak bake for `T_VFX_Wind_02`, a noise bake for `T_VFX_Noise_06`, and **a new colour-LUT output
kind** for `T_VFX_LUT_Bomb_01` (the generator bakes greyscale masks only).

### 6.5 Capability gap callout — be conservative, this is where sessions are lost

| # | Gap | Severity |
|---|---|---|
| 1 | **Ribbon renderer does not exist in CkParticles.** The DI writes sprite/mesh attributes; the template builder emits sprite and mesh renderers only; the row-renderer spec has no ribbon kind; and CkUsf deliberately does **not** ship a Niagara *ribbon* usage flag (NS_Lightning_Range §9: "Ribbon and mesh-particle usages were deliberately not added"). `Bomb_Trail` is 1 of 3 emitters and is the projectile's signature read. **There is no approximation that is honest** — a chain of velocity-aligned sprites is a different effect and should be labelled as such if chosen. | **BLOCKING** |
| 2 | **`Spawn Per Unit` (distance-driven spawn) does not exist.** Every CkParticles cadence row is time-driven (loop / lifetime / burst, or a continuous rate). A trail that emits per 20 units of *travel* cannot be expressed; at constant speed it degenerates to a rate, but the source's whole point is that a stationary projectile emits nothing. | **BLOCKING** for the trail |
| 3 | **World-space emitter.** `Bomb_Trail` is `LocalSpace: false` while the other two are `true`. The CkParticles template is local-space for the whole system (NS_BasicAttack §13.2). A trail that must stay behind a moving projectile is precisely the case where that deviation is visible. | **BLOCKING** for the trail |
| 4 | **System-level loop parameters are not in the corpus** (§2). Cadence cannot be finalized from the corpus alone. | **Prerequisite** |
| 5 | **No row-declared camera-facing sprite kind** (§6.2). Small, additive fix. | Medium |
| 6 | **Gradient-map LUT chain unimplemented in the CkUsf DissolveAdd family**, and the texture generator has no colour-LUT bake (§6.4). | Medium |
| 7 | **Mesh renderer facing modes are not expressible** on a row-declared `Mesh` renderer — the spec carries `Kind`, `VisTag`, `MeshName`, `LookName` and nothing else. `Bomb` uses `Facing: Default`, which happens to be the builder's default, so **this effect is not blocked by it** — but the sibling sheets in this batch are. | None *here* |
| 8 | **Hand-authored prop mesh with a non-derivable UV atlas** (§3, §6.3). Not a pipeline gap — a content decision. | Decision required |

**Complexity tier: L.** The burst half (glow + bomb) is an S/M-tier job; the ribbon trail needs a
renderer kind, a spawn model and a coordinate space the pipeline does not have. Do not scope this as
"one behavior" — scope it as *the burst half now, the trail behind a ribbon capability*.

### 6.6 Behavior id

**Do not allocate an id in this document.** At implementation time take the next free id from
`ck::particles::NumBehaviors` in `CkParticles_ScriptDefinition_Naming.h`, bump `NumBehaviors`, and
add the id to the roster paragraph in `CkParticles/CLAUDE.md`. Four sibling sheets in this batch
(NS_FireBall_Cast / _Hit / _Projectile, NS_Bomb_Spawn, NS_Bomb_Explosion) were written in the same
pass and none of them allocates an id either — allocate them in one ordered pass so they cannot
collide.

---

## 7+. Reserved for implementation

Sections 7–14 of the recipe schema ([README.md](README.md)) are intentionally absent and are to be
written by the implementation session, from what actually happened.
