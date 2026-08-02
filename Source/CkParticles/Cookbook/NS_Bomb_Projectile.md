# Recipe: NS_Bomb_Projectile → CkParticles (PRE-IMPLEMENTATION TRANSLATION SHEET)

Schema and evidence-tag conventions: [README.md](README.md).
Exemplars this sheet copies: [NS_BasicAttack.md](NS_BasicAttack.md) §1–6, [NS_Lightning_Range.md](NS_Lightning_Range.md) §1–5.

## Completion state — READ FIRST

**Status: IMPLEMENTATION-COMPLETE (2026-08-02, Phase 3 batch F). Behavior id 37. Not yet A/B'd.**

`Behavior_BombProjectile.ush` + `ExecuteStage_CPU` case 37, the `PS_CkParticles_Template_BombProjectile`
cadence row (2.5 s / 2.5 s / burst 4, plus a ribbon emitter whose 17-point burst is placed by ARC LENGTH),
one new CkUsf look (`TrailDisAdd01`, ribbon-drawn), one new texture (`TileNoiseSparse`), zero new meshes,
`Test_Particles_BombProjectileBehavior.cpp`, and a VfxExamples gym pair. **Read §13.1 first: the trail is
structurally present and draws nothing, because the source draws nothing either at a stationary spawn
point.** Nothing has been rendered or visually compared — §12 is open.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Bomb_Projectile` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior id | **37** (`BombProjectile`) |
| Recreation status | implementation-complete; `[HUMAN-VERIFY]` open |

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

`SM_VFX_Bomb_01_Small` — **RULED and already SHIPPED.** [C-D2] chose option 1: a procedural stylized
stand-in (a sphere with a fuse) under the toon-banded material, no mesh import. NS_Bomb_Spawn (behavior
25) built it — `SM_CkParticles_Bomb` plus the `BombToon` look — from the same source mesh and the same
`MI_VFX_Bomb` instance, so this port reuses both unchanged and adds no mesh work at all. The shape gap
(§3's authored UV atlas, which no re-UV'd sphere can band in the same places) is recorded in §13.

The three options this sheet listed are kept below for the record; only the first was taken.

1. *(TAKEN)* Procedural sphere + re-authored banding — banded by object-space Z rather than by `v`.
2. Import the mesh — rejected by [C-D2] (pack dependency + licensing posture).
3. Defer the prop entirely — rejected; the prop is the effect's subject.

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
| 1 | ~~Ribbon renderer does not exist~~ — **CLOSED by C6a/C6b** ([P3-D1]): a ribbon-bearing row declares a SECOND emitter carrying the ribbon spawn stack and `Ribbon`-kind renderers, and `_UsedWithNiagaraRibbons` is a real CkUsf usage flag. | Closed |
| 2 | ~~`Spawn Per Unit` does not exist~~ — **CLOSED by C11**: per-unit spawn is arc-length placement along the leader's path (`CkParticles_ArcLengthTable` / `ArcLengthTime`). Its LAST clause is exactly right and survives: a stationary projectile emits nothing, and this recreation reproduces that rather than papering over it. §13. | Closed |
| 3 | **World-space emitter** — the C12 non-goal. `Bomb_Trail` is `LocalSpace: false` while the other two are `true`, and its points are deposited where the projectile *was*. At a STATIONARY pedestal the difference is unobservable, because a stationary emitter's local and world frames coincide and Spawn Per Unit fires in neither. §13. | Known difference |
| 4 | ~~System-level loop parameters are not in the corpus~~ — **CLOSED `[corpus-v3]`**: the v3 exporter dumps the System Update stack, giving `Loop Once / 2.5 s` (§2). | Closed |
| 5 | ~~No row-declared camera-facing sprite kind~~ — **CLOSED by C1** (`ECk_ParticlesRenderer_Kind::CameraFacingSprite`). | Closed |
| 6 | **Gradient-map LUT chain**: the CHAIN is implemented (C3) and the generator bakes `ColorLut` textures, but `T_VFX_LUT_Bomb_01` itself is not baked — the look holds the family's white ramp pending **[P1-D1]**, the open `Gradient_Invert` remap question, exactly as `RainbowDisAdd` does. §13. | Medium (deferred) |
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

## 7. Textures — ONE new bake

| Source texture | Bake | Status |
|---|---|---|
| `T_VFX_Part_01` | `SoftParticle` | existing (NS_BasicAttack §7) |
| `T_VFX_Noise_02` | `TileNoise` | existing (NS_BasicAttack §7) |
| `T_VFX_Wind_02` | `WindBandMid` | existing (NS_Arrow_Cast §7.3) |
| `T_VFX_Noise_06` | **`TileNoiseSparse`** | **new** |
| `T_VFX_LUT_Bomb_01` | `LutWhite` (held) | deferred behind **[P1-D1]**, §13 |
| `T_VFX_WhitePixel` | `LutWhite` | existing |

### 7.1 `T_VFX_Wind_02` — measured, and it is not a new paint

Measured independently at implementation, reproducing NS_Arrow_Cast §7.3's finding from scratch: rolling
`T_VFX_Wind_03` **down by 141 of its 512 rows** reproduces `T_VFX_Wind_02` to a **maximum absolute
difference of 0.0039** (one 8-bit quantum) and a mean absolute difference of 1e-6, with a Pearson
correlation of **1.00000**. Their column-mean profiles and their coverage above 0.5 are identical to four
decimals. `WindBandMid` already IS that roll (`Px_WindBandAt(U, frac(V − 141/512))`), so the shape paint
needed no work.

Their unrolled correlation is **−0.31**, so a naive reuse of `WindBand` — the same paint at its original
phase — would have been *worse than uncorrelated*. Measuring the roll is what turns a rejection into an
exact reuse.

### 7.2 `TileNoiseSparse` — measured from `T_VFX_Noise_06`

Rejected against all four existing noise bakes first: the best roll-aligned correlation to any of them is
**0.11** (`TileNoiseBanded`), against 1.000 for the Wind pair. What sets this paint apart is not its
spectrum but a hard **floor**: 42.8 % of it is exactly black, so a dissolve driven by it clears in
isolated patches instead of eroding everywhere at once — and its p99 sits at 0.84 where every other noise
in the pack runs to white.

| Measured on `T_VFX_Noise_06` | Source | Bake |
|---|---|---|
| fraction exactly 0 | 0.4283 | 0.4345 |
| mean | 0.1354 | 0.1525 |
| p75 / p90 / p95 | 0.208 / 0.459 / 0.612 | 0.264 / 0.457 / 0.571 |
| coverage > 0.25 / > 0.5 | 0.217 / 0.085 | 0.265 / 0.079 |
| correlation half-decay (u / v, px) | 7 / 11 | 9 / 8 |

The bake is the library's ordinary tileable value-noise Fbm at **32 tiles, 2 octaves**, cut at a
**threshold of 0.47** and re-gained by **1.90** — both constants fitted to the distribution above, not
chosen.

---

## 8. Mesh — reused, not rebuilt

`SM_CkParticles_Bomb` and the `BombToon` look already exist: NS_Bomb_Spawn (behavior 25) built them from
the SAME source mesh (`SM_VFX_Bomb_01_Small`) and the SAME material instance (`Parents/MI_VFX_Bomb`) under
[C-D2]. Every one of `BombToon`'s nine parameter defaults was checked against §4.2's table before reuse
and matches exactly. This port adds no mesh work and no material work for the prop.

The shape gap §3 identified — the source's hand-authored UV atlas, which `MI_VFX_Bomb` bands by `Step`
over `TextureCoordinate` and which no re-UV'd procedural sphere can reproduce in the same places — carries
over unchanged from NS_Bomb_Spawn §13 and is restated in §13 below.

---

## 9. The behavior — `Behavior_BombProjectile.ush` + `ExecuteStage_CPU` case 37

### 9.1 The row

`PS_CkParticles_Template_BombProjectile`: **loop 2.5 s / lifetime 2.5 s / burst 4**, and **no spawn
rate** — nothing in this source streams. 2.5 s is the only such loop in the cookbook and it is the
system's own `Loop Once` duration `[corpus-v3]`, which coincides exactly with both burst emitters'
lifetimes.

Partition is a plain `Seed % 4`: slots 0–2 are `Bomb_Glow`, slot 3 is `Bomb`. With no rate stack there is
one population, so the behavior reads **no spawn phase and no emitter clock** — asserted, because
`RosterSanity`'s derived rule says a behavior on a burst-only row must be clock-independent.

### 9.2 The fuse

`Bomb`'s `Scale Color` (RGBA Together) holds RGB at **0.25** from t = 0 to **t = 0.9139752** and then ramps
to **5.0** at t = 1 — a hard bright flash in the last ~0.215 s of the 2.5 s flight, over a base colour of
white. Alpha is a single key at 1 and never moves. Reading this as a fade would invert the entire effect;
the test pins 0.25 at the key, 5.0 at the end, and 2.2391682 at t = 0.95 so a step cannot pass for a ramp.

`Bomb_Glow`'s `Scale Color` is **DISABLED**, so all three shells hold `RGBA(0.0368895, 0.184475, 1, 0.5)`
at 300 units for the entire 2.5 s. Its authored curve fades alpha 1 → 0; running it would dissolve the
glow the source keeps constant. Asserted at both ends of life.

### 9.3 Orientation

`Initial Mesh Orientation` randomizes Z through a `Random Range Vector` of −1 … 1, read as **turns** by the
NS_Bomb_Spawn precedent (the same module on the same prop), so the yaw is `TAU × lerp(−1, 1, Rand(Seed, 3))`.
Unlike its Bomb_Spawn twin this emitter carries **no** `Update Mesh Orientation`, so the prop never spins —
asserted by comparing the quaternion at two ages.

### 9.4 The trail — C11, and why it draws nothing here

`Bomb_Trail` is the cookbook's only `Spawn Per Unit` emitter: one ribbon point per **20 units of travel**,
held in world space for 0.6 s at a constant 200-unit width, alpha 0.2 → 0, dissolve +0.5 → −0.5. Its
`Add Velocity` is DISABLED and no force acts on it, so a trail point never moves after it is deposited.

C11 expresses that exactly: the point at arc-length fraction `i/16` of the leader's path holds the
**leader's position at the time the leader had covered that fraction**. The behavior builds the leader's
17-slot path, takes `CkParticles_ArcLengthTable` / `CkParticles_ArcLengthTime`, and evaluates
`LeaderPos(SpawnTime)`. Verified against a control leader travelling at a constant 800 units/s: the 17
placements come out **exactly equidistant** (spacing spread 0.00e+00 over a 2000-unit path).

**And in this recreation the leader is stationary, so the trail places nothing — which is what the SOURCE
does under the same conditions.** `Spawn Per Unit` gates on the emitter's own movement delta against a
0.5-unit `Movement Tolerance`; at a stationary spawn point the original emits zero trail particles too. A
world-space trail behind a moving projectile is the C12 non-goal, and the campaign's standing ruling is
that at a stationary A/B pedestal the difference is unobservable — here it is unobservable in the strong
sense that **both sides draw nothing**. Placing a synthetic trail would therefore be a parity *regression*,
not a fix: our pedestal would show a streamer the original does not.

The behavior is written against a general path and the degenerate case is a guard, not a special case:
`CkParticles_BombProjectile_LeaderPos` is a single function, and a leader with real travel lights the whole
branch up without touching it. The ribbon emitter, renderer and look all ship for the same reason — the
source has them, and the structure is what makes the difference recoverable later.

---

## 10. Looks and renderers

**Two row renderers on the main emitter, one on the ribbon emitter. ONE new look.**

| VisTag | Kind | Look | Source emitter |
|---|---|---|---|
| 164 | CameraFacingSprite | `PartDisAdd01` | `Bomb_Glow` |
| 165 | Mesh (`Bomb`) | `BombToon` | `Bomb` |
| 166 | **Ribbon** | `TrailDisAdd01` *(new)* | `Bomb_Trail` |

`PartDisAdd01` and `BombToon` are exact reuses — the same source instances, checked value-by-value.

`TrailDisAdd01` is `M_VFX_DisAdd_Trail01`: Brightness 6, `Dissolve_Speed_X = −1` (the only look in the
cookbook that pans its dissolve at unit speed), `Distortion_Intensity 0.3` LIVE, `Gradient_Invert 0`,
`Opacity_Boldness 1`, shape `WindBandMid`, dissolve `TileNoiseSparse`, distortion `TileNoise` (the
instance's `Distortion_Tex` is `T_VFX_Noise_02`, inherited from the reference and distinct from its
dissolve paint). It opts into `_UsedWithNiagaraRibbons`.

---

## 11. Tests

`Test_Particles_BombProjectileBehavior.cpp` — `CkTests.UnitTests.CkParticles.BombProjectileBehavior`. CPU
mirror only. It asserts:

- the cadence row's four numbers, that the row declares **no** spawn rate, and the ribbon emitter's
  17-point burst, its zero rate, its single `Ribbon`-kind renderer, its look name and its VisTag;
- the 3:1 burst partition, over one modulus and over 500;
- `Bomb_Glow` is constant at three ages — colour, alpha 0.5, size 300 and dissolve 2 — which is the
  disabled-`Scale Color` claim, and would fail against its authored 1 → 0 alpha curve;
- the FUSE at both ends and on the ramp: 0.25 at t = 0 and at t = 0.9139752, **2.2391682** at t = 0.95,
  **5.0** at t = 1, alpha pinned at 1;
- the prop's mesh index, its 0.45 uniform scale, a normalized orientation, no sprite quad, and a yaw that
  does NOT change between two ages;
- both layers dying at 2.5 s;
- the trail branch is REACHED (every ribbon-bank id writes VisTag 166 and nothing else) and produces
  nothing at five ages across 51 ids — the "no travel, no trail" contract, stated as an assertion rather
  than left as an absence;
- **emitter-clock independence**: 400 seeds × 3 clocks, zero movement. The row declares one population, so
  reading the clock at all would be a defect.

---

## 12. Verification — A/B protocol

`[HUMAN-VERIFY]` Open the **VfxExamples** gym, cycle to **BOMB PROJECTILE**, and run
`Ck_GymVfxExamples_RestartAll`. In order:

a. **A toon bomb hanging in the air**, banded into flat colour zones, at a random yaw that does not change.
b. **A soft blue halo around it** that does NOT fade — three coincident 300-unit shells stacking additive
   alpha, constant for the whole 2.5 s. If it fades out, the disabled `Scale Color` was transcribed anyway.
c. **A bright white flash on the prop in the last fifth of a second**, from a quarter brightness straight
   to 5×. This is the whole point of the effect and it is easy to miss if you blink; restart and watch the
   end of the cycle specifically.
d. **NEITHER side draws a trail.** The source's trail is spawned per unit of travel and the pedestal does
   not move. If the ORIGINAL shows one, the pedestal is moving and §13.1 needs revisiting; if OURS shows
   one and the original does not, that is a defect.
e. The bomb's silhouette against the original's: ours is a stylized stand-in (sphere + fuse) and the
   banding lands in different places (§13.2). Judge the *read*, not the geometry.

## 13. Confirmed fidelity differences or intentional deviations

**The visual gate in §12 has NOT been executed.** Everything below is a *known* difference derived from
the source data, or *unverified*.

### Known differences — deliberate

1. **The trail is structurally present and visually absent.** The source spawns it per 20 units of TRAVEL;
   this recreation renders at a fixed point, so the leader path has zero length and no point is placed.
   The original behaves identically at a stationary spawn point, so this is parity rather than a gap — but
   it means the C11 arc-length machinery, though correct and exercised by its own control, contributes
   nothing to the rendered image of THIS port. A leader with real travel (the C12 non-goal, or a future
   moving-spawner capability) lights it up with no change to the trail branch.
2. **The bomb prop is a procedural stand-in** ([C-D2]) — a sphere with a fuse, banded by object-space Z.
   The source mesh is a 6066-vertex sculpt whose UVs are a hand-authored atlas with no re-derivable
   projection (§3), and `MI_VFX_Bomb` bands its three flat colours by `Step` over exactly those UVs, so the
   bands land in different places. Same gap NS_Bomb_Spawn §13 records.
3. **`MI_VFX_Bomb`'s band EDGES are not in the corpus** (unnamed graph constants), so `BombToon` starts at
   an even three-way split. Inherited from NS_Bomb_Spawn.
4. **The gradient-map LUT is held white.** `M_VFX_DisAdd_Trail01` is the only instance in the cookbook that
   points `GradientMap_Tex` at a REAL 512×2 ramp (`T_VFX_LUT_Bomb_01`) rather than the family's white
   pixel. The chain is implemented (C3) and the generator can bake a `ColorLut`, but **[P1-D1]** — the
   exact `Gradient_Invert` remap, unrecoverable from the corpus — is still open, and the campaign's
   standing rule holds every real ramp at `LutWhite` until it is ruled. `RainbowDisAdd` is held the same
   way. Inert today; a colour shift on the trail once the trail draws.
5. **`Core_Intensity` (2), `Color_CoreDifferent` (1) and the `Color_Core` tint
   `RGBA(0.057805, 0.313989, 0.590619, 1)` are not plumbed.** The family helper pins `CoreColor` white.
   Trail01 drives all three; every other DissolveAdd instance in the cookbook leaves them at the reference.
6. **`Opacty_DepthFade` (20 on Part01, 30 on Trail01) is not wired.** CkUsf surface looks have no
   scene-depth input.
7. **The shape paint is an exact reuse, but the family's `MainTex_Offset` is still unplumbed.** It is
   (0, 0) on this instance, so nothing is lost here — but the reason `WindBandMid` exists as a second bake
   at all is that the look cannot offset a texture, so a third phase of the same paint would cost a third
   bake rather than a parameter.
8. **World space on 1 of 3 emitters** (`Bomb_Trail`) — the C12 non-goal, subsumed by (1).
9. **`Bomb`'s `Index 0 Param 1 = 1` is dropped**: `MI_VFX_Bomb` declares no dynamic parameters, so the
   write is inert in the source too.
10. **The recreation loops; the source completes.** The row loops its 2.5 s forever; the source's
    `Inactive Response = Complete` lets it die and the harness re-arms it.

### Unverified

- Every visual criterion in §12. Nothing has been rendered.
- Whether the stand-in prop reads as the same object at gym distance. NS_Bomb_Spawn's identical prop has
  not been A/B'd either.
- Whether `TrailDisAdd01` renders correctly under a ribbon renderer at all — nothing in this port makes it
  draw, so its first real exercise will be a future port or a moving leader.

---

## 14. Reusable lessons

1. **"The source draws nothing here" can be the correct implementation.** A distance-driven spawn at a
   stationary pedestal emits zero particles on BOTH sides. Synthesizing a trail to make the port look
   complete would have introduced a visible A/B mismatch in the name of fixing an invisible one. Check what
   the ORIGINAL does under the gym's own conditions before deciding a branch is missing.
2. **Write the degenerate case as a guard on a general function, not as a special case.** The whole trail
   branch is written against `LeaderPos(t)`; the only thing that makes it inert is that this port's leader
   returns a constant. That keeps the C11 code honest, testable and one edit away from live.
3. **Measure the ROLL, not just the correlation.** Two paints correlating at −0.31 can be the same image
   at a different phase. A phase sweep turned a rejection into an exact reuse and saved a bake — and the
   same sweep is what proves a genuinely different paint (`T_VFX_Noise_06`, best 0.11 at any roll) really
   does need one.
4. **A hard floor is a property a percentile fit will miss.** `T_VFX_Noise_06`'s defining feature is that
   43 % of it is exactly zero, not where its median sits. Fitting a threshold-and-gain to the zero fraction
   first, and the percentiles second, reproduces the paint's *behaviour* under a dissolve rather than its
   histogram.
5. **A shipped look is a shipped asset, independent of whether the port that introduced it draws it.**
   `TrailDisAdd01` is invisible in this port and still had to be right — including its dissolve paint, its
   distortion texture and the [P1-D1] deferral — because the next consumer inherits it.
