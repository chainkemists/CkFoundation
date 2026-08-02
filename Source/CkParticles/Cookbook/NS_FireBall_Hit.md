# Recipe: NS_FireBall_Hit → CkParticles (PRE-IMPLEMENTATION TRANSLATION SHEET)

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
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_FireBall_Hit` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior id | **not allocated** — take the next free id at implementation time |
| Recreation status | not started |

Corpus evidence (all `[corpus]`, exported 2026-08-01):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_FireBall_Hit.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part01_Bright,Part03_Bright,Part04,Rainbow,Ring01,Star01,Star02,Wind01,Wind02,LightStrip,Flat02,Flames01,Smoke01,Flare01,Impact01}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_FlatAdd.json`
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_{Spike01,Plane01,Ring01}.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/*.json`

**The source Niagara asset was not opened for editing.**

> ### A same-named sibling exists — and the casing differs
> `[corpus]` `Vefects/Anime_Stylized_VFX/VFX/Particles/**NS_Fireball_Hit**` — lowercase **b**.
> **Fastest robust discriminator: the sibling declares 8+ user parameters** (`User.Flames Color 01`,
> `User.Flare Color 01/02`, `User.Flash Color 01`, `User.Glow Color 01…04`, several of them
> `NiagaraDataInterfaceColorCurve`) and renders through `MI_VFX_*` instances (`MI_VFX_Glow_01`,
> `MI_VFX_Impact_01`, `MI_VFX_Light_Strip_01`, `MI_VFX_Lens_Rainbow_01`, …). The
> `Anime_VFX/Shared/Skills` variant documented here has an **empty user-parameter list** and renders
> through `M_VFX_DisAdd_*`.

**NS_FireBall_Hit is a near-sibling of [NS_FireBall_Cast.md](NS_FireBall_Cast.md)** — 14 emitter
names are shared and several curves are byte-identical. It is **not** a subset: the spawn times,
lifetimes, sizes and velocity models all differ, and `LightningStrip` uses a *different renderer
kind*. Read this sheet, do not diff the two by eye.

---

## 2. System anatomy `[corpus]`

**20 CPU emitters — 17 enabled, 3 DISABLED. All `Spawn Burst Instantaneous`. 47 particles per loop
from the enabled set. 13 sprite renderers + 4 mesh renderers (enabled).**

| # | Emitter | Enabled | Space | Count | Spawn t | Lifetime | Renderer | Mesh | Material |
|---|---|---|---|---|---|---|---|---|---|
| 0 | `Raimbow` | yes | World | 1 | 0 | 0.2 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Rainbow` |
| 1 | `Ring` | yes | World | 1 | 0 | 0.3 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Ring01` |
| 2 | `Sparkles` | yes | World | **7** | 0.05 | rand | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01_Bright` |
| 3 | `Glow_01` | yes | World | 1 | 0 | **0.1** | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 4 | `Sparkles_Stretched` | yes | World | **5** | 0.05 | rand | Sprite, **VelocityAligned**/FaceCamera | — | `M_VFX_DisAdd_Part04` |
| 5 | `Star_02` | **DISABLED** | World | 1 | 0.5 | 0.1 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Star02` |
| 6 | `Star_01` | yes | World | 1 | 0.05 | 0.3 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Star01` |
| 7 | `Glow_02` | yes | World | 1 | 0 | **0.1** | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 8 | `SecondGlow` | yes | World | 1 | 0 | **0.1** | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part03_Bright` |
| 9 | `Wind_01` | **DISABLED** | Local | 1 | 0.55 | 1.5 | Mesh, Facing Default, renderer scale `(1,1,5)` | `SM_VFX_Ring01` | `M_VFX_DisAdd_Wind02` |
| 10 | `Wind_02` | **DISABLED** | Local | 5 | 0.55 | 1.5 | Sprite, Unaligned/FaceCamera, SubUV 2×2 | — | `M_VFX_DisAdd_Wind01` |
| 11 | `Spike01` | yes | **Local** | **3** | 0.05 | rand 0.1–0.15 | **Mesh**, Facing **Velocity** | `SM_VFX_Spike01` | `M_VFX_DisAdd_Flat02` |
| 12 | `Flames01` | yes | World | **5** | 0 | rand | Sprite, Unaligned/FaceCamera, **SubUV 2×2** | — | `M_VFX_DisAdd_Flames01` |
| 13 | `Smokes` | yes | World | **5** | **0.04** | rand | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Smoke01` |
| 14 | `Flare01` | yes | World | **2** | 0 | 0.2 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Flare01` |
| 15 | `FirstFlash` | yes | World | **4** | **0.04** | 0.1 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part03_Bright` |
| 16 | `FirstGlow` | yes | World | 1 | 0.05 | 0.2 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 17 | `FlareImpact` | yes | **Local** | 1 | 0.05 | **0.05** | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Impact01` |
| 18 | `LightningStrip` | yes | **Local** | **5** | 0 | rand 0.1–0.2 | **Mesh**, Facing **Velocity** | `SM_VFX_Plane01` | `M_VFX_DisAdd_LightStrip` |
| 19 | `Spike02` | yes | **Local** | **3** | 0.05 | rand 0.1–0.15 | **Mesh**, Facing **Velocity** | `SM_VFX_Spike01` | `M_VFX_DisAdd_Flat02` |

**Total per loop (enabled): 47 particles.** The three disabled emitters would add 7 more (1 + 1 + 5).
`Bounds: Dynamic`, `Determinism: false` everywhere.

**Two spawn beats, 50 ms apart** — and unlike NS_FireBall_Cast there is **no charge-up act**:

| Beat | Spawn t | Emitters | Particles |
|---|---|---|---|
| Impact | **0** | `Raimbow`, `Ring`, `Glow_01`, `Glow_02`, `SecondGlow`, `Flames01`, `Flare01`, `LightningStrip` | 16 |
| Aftermath | **0.04 / 0.05** | `Smokes`, `FirstFlash` (0.04); `Sparkles`, `Sparkles_Stretched`, `Star_01`, `Spike01`, `FirstGlow`, `FlareImpact`, `Spike02` (0.05) | 31 |

**The three disabled emitters are recorded, not deleted** (README §3): `Star_02`, `Wind_01`,
`Wind_02`. Their absence from the recreation is then a decision, not an oversight — and note
**both wind emitters are disabled here while both are enabled in NS_FireBall_Cast.** The hit has no
wind.

> **`Life Cycle Mode = System` on all 20 emitters `[corpus]`.** The stored per-emitter Loop
> Behavior / Loop Duration are **inert leftovers** — including `Loop Duration = 5.0` on
> `Sparkles_Stretched`, `0.3` on `Star_01`/`Star_02`/`Flames01`/`Smokes`, and the
> `Loop Behavior = Once` on those five. `[unresolved: the system-level System State stack is NOT
> exported by CkAssetExporter — only Emitter Update / Particle Spawn / Particle Update. The
> system's loop duration and loop behavior are unknown from the corpus.]` Spawn **times** and burst
> **counts** are live.

---

## 3. Mesh geometry `[corpus]`

Three meshes; the third (`SM_VFX_Ring01`) is referenced only by the **disabled** `Wind_01`.

### 3.1 `SM_VFX_Spike01` — 16 verts / 6 tris — `Spike01` + `Spike02` (6 particles)

Bounds `X −100…100`, `Y −100…100`, `Z 0…200`; uv0 covers `0…1`. **A square pyramid**: apex
`(0, 0, 200)`, base square `(±100, ±100, 0)`; 4 side + 2 base triangles. Radius-from-origin min
141.421, max 200.

UVs: `corr(v, Z) = −1.000` and `corr(v, radiusXY) = 1.000`; `corr(u, Y) = −1.000`.
**v = 0 at the tip, v = 1 at the base**; u across (0 on +Y, 1 on −Y, 0.5 at the apex). UV list
verbatim: `(0,1) (0.5,0) (0,1) (0,1) (0.5,0) (1,1) (1,1) (0.5,0) (0,1) (1,1) (0.5,0) (1,1) (0,1)
(0,1) (1,1) (1,1)`. **Trivially procedural.**

### 3.2 `SM_VFX_Plane01` — 8 verts / 4 tris — `LightningStrip` (5 particles)

Bounds `X −100…100`, `Y −0.0643…0`, `Z −0…200`; uv0 `0…1`. **Two coincident quads in the XZ
plane**, offset 0.0643 in Y — a paper-thin double-sided card. Verts `(±100, 0, 0)`,
`(±100, 0, 200)` and the same four at `Y = −0.0643`.

UVs: `corr(u, X) = 1.000`, `corr(v, Z) = −1.000` — **u runs −X→+X, v runs top (0) → bottom (1)**.

**Trivially procedural.** The 0.0643 Y offset is 0.03 % of the 200-unit span; collapse to one sheet
and make the look `_TwoSided`, exactly as NS_BasicAttack §13.5 did for the crescent.

### 3.3 `SM_VFX_Ring01` — 132 verts / 128 tris — `Wind_01` (**DISABLED**)

Bounds `(-100, -100, 0)` … `(100, 100, 50)`. **Only two distinct radii (99.5, 100.0) and two
distinct Z (0, 50)** — a thin double-walled open tube, 64 segments around.
UVs: `corr(v, Z) = −1.000`, `v ∈ {0, 1}` with **v = 0 at the TOP (Z = 50)**;
**`u = 0.75 + angle/360` mod 1**, seam at ±180°. The renderer applies `scale: (1, 1, 5)`.
**Recorded for completeness — not needed unless `Wind_01` is re-enabled.**

All three meshes report section 0's material as `M_VFX_DisAdd_Slash01`; that is the mesh's default
slot and is **overridden by every emitter's renderer**. Do not chase it.

---

## 4. Material families and per-instance deltas `[corpus]`

**Two parent graphs**: `M_VFX_DissolveAdd` (15 instances) and `M_VFX_FlatAdd` (1 instance).

### 4.1 `M_VFX_DissolveAdd` family

Instances: `Part01`, `Part01_Bright`, `Part03_Bright`, `Part04`, `Rainbow`, `Ring01`, `Star01`,
`Star02`*, `Wind01`*, `Wind02`*, `LightStrip`, `Flames01`, `Smoke01`, `Flare01`, `Impact01`
(`*` = referenced only by a disabled emitter). Identical base properties on all: `MD_Surface`,
**`BLEND_Translucent`**, **`MSM_Unlit`**, `twoSided: false`, outputs **`EmissiveColor` + `Opacity`**,
dynamic-parameter channels **`[dissolve, distortion, offset, core_color]`**, and the family
expression histogram (`ScalarParameter ×41`, `Multiply/Add/AppendVector ×18` each, `Saturate ×12`,
`DynamicParameter ×8`, `Reroute ×8`, `TextureSampleParameter2D ×6`, `Panner ×5`, `Constant ×5`,
`TextureCoordinate ×5`, `LinearInterpolate ×4`, `Clamp ×2`, `OneMinus ×2`, one each of `DepthFade`,
`ParticleColor`, `Power`, `SmoothStep`, `StaticBoolParameter`, `StaticSwitch`, `VectorParameter`,
`MaterialFunctionCall`, `WorldPosition`).

Reference = `M_VFX_DisAdd_Part01`: all texture slots `T_VFX_Part_01` except
`Distortion_Tex`/`GradientShape_Tex` = `T_VFX_Noise_02` and `GradientMap_Tex` = `T_VFX_WhitePixel`;
`Brightness` 1; `Opacity_Boldness` 0.5; `Distortion_Intensity` 0; `Dissolve` 0; `Dissolve_Invert` 0;
all `*_Scale_X/Y` 1; all `*_Speed_X/Y` and `*_Offset_X/Y` 0; `Gradient_Invert` 0.5;
`GradientMap_Displacement` 0.10000000149011612; `Color_CoreDifferent` 0; `Core_Power` 1;
`Core_Intensity` 0; `Glow_Intensity` 1; `Opacty_Step` 0; `Opacty_StepAdd` 0.10000000149011612;
`Opacty_DepthFade` 20; `CamOffset` 0; `Color_Core` `RGBA(1, 1, 1, 0)`.

| Instance | Delta vs `Part01` |
|---|---|
| `M_VFX_DisAdd_Part01` | *(reference)* — `Glow_01`, `Glow_02`, `FirstGlow` |
| `M_VFX_DisAdd_Part01_Bright` | `Brightness` → **10**; `Core_Intensity` → **1**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Part_02`** |
| `M_VFX_DisAdd_Part03_Bright` | `Brightness` → **10**; **`CamOffset` 0 → 50**; `Core_Intensity` → **1**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Part_03`** |
| `M_VFX_DisAdd_Part04` | `Brightness` → **6**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; `Opacty_DepthFade` 20 → **30**; Main/Color/Dissolve → **`T_VFX_Part_04`** |
| `M_VFX_DisAdd_Rainbow` | `GradientMap_Displacement` 0.1 → **0.9**; `Gradient_Invert` 0.5 → **2**; `Opacity_Boldness` 0.5 → **1.5**; `Opacty_StepAdd` 0.1 → **0.3**; **`GradientMap_Tex` → `T_VFX_LUT_Rainbow_01`**; `GradientShape_Tex` → `T_VFX_Part_01`; `Main_Tex` → **`T_VFX_Ring_02`** |
| `M_VFX_DisAdd_Ring01` | `Brightness` → **10**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Ring_01`** |
| `M_VFX_DisAdd_Star01` | `Brightness` → **6**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; `Opacty_DepthFade` 20 → **10**; Main/Color/Dissolve → **`T_VFX_Star_01`** |
| `M_VFX_DisAdd_Star02`* | `Brightness` → **6**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; `Opacty_DepthFade` 20 → **10**; Main/Color/Dissolve → **`T_VFX_Star_02`** |
| `M_VFX_DisAdd_Wind01`* | `Brightness` → **3**; `Core_Intensity` → **1**; `Dissolve_Speed_Y` 0 → **−0.15**; `Distortion_Intensity` 0 → **0.5**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; Main/Color → **`T_VFX_Wind_01`**; `Dissolve_Tex` → **`T_VFX_Noise_02`** |
| `M_VFX_DisAdd_Wind02`* | `Brightness` → **7**; `Color_Speed_X` 0 → **−0.3**; `Dissolve_Scale_X/Y` → **0.7 / 0.95**; `Dissolve_Speed_X` → **−0.1**; `Distortion_Intensity` 0 → **1**; `Distortion_Scale_Y` → **0.6**; `Distortion_Speed_X/Y` → **0.1 / 0.1**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Wind_02`** |
| `M_VFX_DisAdd_LightStrip` | `Brightness` → **7**; **`Core_Power` 1 → 0**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_LightStrip_01`** |
| `M_VFX_DisAdd_Flames01` | `Brightness` → **10**; `Core_Intensity` → **1**; **`Dissolve` 0 → −0.1**; `Dissolve_Scale_X/Y` → **2 / 2**; **`Distortion_Intensity` 0 → 0.5**; `Distortion_Scale_X/Y` → **2 / 2**; `Distortion_Speed_X/Y` → **−0.3 / −0.3**; **`Glow_Intensity` 1 → 2**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; Main/Color → **`T_VFX_Wind_01`**; Dissolve/Distortion → **`T_VFX_Noise_04`**; `Color_Core` → **`RGBA(0.015996, 0.014444, 0.014444, 1)`** |
| `M_VFX_DisAdd_Smoke01` | `Brightness` → **10**; `Core_Intensity` → **1**; **`Dissolve` 0 → −0.1**; **`Distortion_Intensity` 0 → 0.4**; `Distortion_Speed_X/Y` → **0.1 / 0.1**; `GradientMap_Displacement` 0.1 → **0.75**; `Gradient_Invert` → **0**; **`Opacity_Boldness` 0.5 → 3**; `Color_Tex` → **`T_VFX_Cloud_04`**; `Main_Tex` → **`T_VFX_Cloud_05`**; `Dissolve_Tex` → **`T_VFX_Noise_07`**; `Distortion_Tex` → **`T_VFX_Noise_04`**; `Color_Core` → **`RGBA(0.001, 0.001, 0.001, 1)`** |
| `M_VFX_DisAdd_Flare01` | `Brightness` → **2**; **`Gradient_Invert` 0.5 → 0.847619**; `Opacity_Boldness` → **1**; `Main_Tex`/`Color_Tex` → **`T_VFX_Ring_02`**; `GradientShape_Tex` → **`T_VFX_Part_01`** |
| `M_VFX_DisAdd_Impact01` | `Brightness` → **12**; **`Core_Power` 1 → 0**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Impact_01`** |

### 4.2 `M_VFX_FlatAdd` family — `M_VFX_DisAdd_Flat02` (`Spike01`, `Spike02`)

`BLEND_Translucent`, `MSM_Unlit`, `twoSided: false`, outputs `EmissiveColor` + `Opacity`,
**no dynamic parameters, no texture parameters**; `Brightness` **10**, `Opacty_DepthFade` 0,
`CamOffset` 0, `Color_Core` `RGBA(1,1,1,0)`. Expressions: `Multiply ×2`, `ScalarParameter ×3`,
`VectorParameter ×1`, `ParticleColor ×1`, `DepthFade ×1`, `WorldPosition ×1`,
`MaterialFunctionCall ×1`. **`ParticleColor × 10`, depth-faded.**

### 4.3 Textures referenced `[corpus]`

Same set as [NS_FireBall_Cast.md](NS_FireBall_Cast.md) §4.3 minus `T_VFX_Star_03`, plus
`T_VFX_Impact_01`. All 512×512, `sRGB: false`, `TC_Alpha`, `TEXTUREGROUP_World`, except where noted:

| Texture | Address | Format | Used by |
|---|---|---|---|
| `T_VFX_Part_01` | `TA_Clamp` | `TSF_G8` | `Part01`; `Rainbow`/`Flare01` GradientShape |
| `T_VFX_Part_02` | `TA_Clamp` | `TSF_G8` | `Part01_Bright` |
| `T_VFX_Part_03` | `TA_Wrap` | `TSF_G8` | `Part03_Bright` |
| `T_VFX_Part_04` | `TA_Wrap` | `TSF_G16` | `Part04` |
| `T_VFX_Ring_01` | `TA_Wrap` | `TSF_G16` | `Ring01` |
| `T_VFX_Ring_02` | `TA_Wrap` | `TSF_G16` | `Rainbow`, `Flare01` Main/Color |
| `T_VFX_Star_01` | `TA_Wrap` | `TSF_G16` | `Star01` |
| `T_VFX_Star_02` | `TA_Wrap` | `TSF_G16` | `Star02` *(disabled emitter)* |
| `T_VFX_Wind_01` | `TA_Wrap` | `TSF_G16` | `Flames01` Main/Color; `Wind01`* |
| `T_VFX_Wind_02` | `TA_Wrap` | `TSF_G8` | `Wind02`* |
| `T_VFX_LightStrip_01` | `TA_Wrap` | `TSF_G16` | `LightStrip` |
| `T_VFX_Impact_01` | `TA_Wrap` | `TSF_G16` | `Impact01` |
| `T_VFX_Cloud_04` | `TA_Wrap` | `TSF_G16` | `Smoke01` Color_Tex |
| `T_VFX_Cloud_05` | `TA_Wrap` | `TSF_G16` | `Smoke01` Main_Tex |
| `T_VFX_Noise_02` | `TA_Wrap` | `TSF_G16` | Distortion / GradientShape on most |
| `T_VFX_Noise_04` | `TA_Wrap` | `TSF_G16` | `Flames01`/`Smoke01` Dissolve/Distortion |
| `T_VFX_Noise_07` | `TA_Wrap` | `TSF_G16` | `Smoke01` Dissolve |
| `T_VFX_LUT_Rainbow_01` | **512×2**, sRGB **true**, `TC_Default` | `TSF_BGRA8` | `Rainbow` GradientMap |
| `T_VFX_WhitePixel` | 1×1, sRGB true, `TC_Default` | `TSF_RGBA16` | GradientMap no-op |

Existing procedural stand-ins usable without new work: `T_VFX_Part_01` → `SoftParticle`,
`T_VFX_Part_04` → `SparkStreak`, `T_VFX_Noise_02` → `TileNoise` (measured in NS_BasicAttack §7).
Measure before assuming: `T_VFX_Wind_01` vs `WindBand`, `T_VFX_Noise_04`/`_07` vs `TileNoise`,
`T_VFX_Ring_01` vs the SDF `Ring` bake, `T_VFX_LightStrip_01` vs `Streak`, `T_VFX_Star_01` vs
`Flare`. **New bakes at minimum:** `T_VFX_Part_02`, `T_VFX_Part_03`, `T_VFX_Ring_02`,
`T_VFX_Impact_01`, `T_VFX_Cloud_04`, `T_VFX_Cloud_05`; plus a **colour-LUT output kind** for
`T_VFX_LUT_Rainbow_01` and a **2×2 sub-UV atlas** shape for `Flames01`.

---

## 5. Per-emitter runtime facts and exact curves `[corpus]`

`t` = NormalizedAge over each emitter's own lifetime. `C` = constant key, `L` = linear key.
Where a curve's first key is at `t > 0`, Niagara clamps to that key's value for all earlier `t`.

Shared boilerplate unless contradicted: `Color Mode = Direct Set`, `Position Mode = Simulation
Position`, **`Position Offset = (0,0,0)` on every emitter in this system**,
`Particle State → Kill Particles When Lifetime Has Elapsed = true`, `Write Parameter Index 0 = true`
(1–3 false), `Dyn Param 2/3/4 = 0`, `ScaleColor.Scale Alpha = 1`, `ScaleColor.Scale RGB = (1,1,1)`,
`ScaleSpriteSize.Initial Sprite Size = (0,0)`, `SolveForcesAndVelocity.Acceleration Limit = 9999`,
`Speed Limit = 1000`, `VectorFromCurve.Scale Curve = (1,1,1)`, `FloatFromCurve.Scale Curve = 1`.

**The velocity model differs from NS_FireBall_Cast**: this system uses **`Add Velocity from Point`**
(a radial burst from the impact point) where the cast uses `Add Velocity` with a directional
`Random Range Vector`. That is the single biggest behavioural difference between the two sheets.

### 5.1 `Raimbow` — 1 sprite, spawn 0, lifetime 0.2, size **250**

Init colour `RGBA(0.913099, 0.913099, 0.913099, 0.2)`; `Sprite Size Mode = Uniform`,
`Uniform Sprite Size = 250`; `Sprite Rotation Mode = Random`, 0…360; dyn `Param 1 = 0.5`.

`Scale Color` (`RGBA Together`) — identical to the Cast's:
`Red/Green/Blue: (0.328403, 0.5)L`; **`Alpha: (0, 0)L (0.328403, 1)L (1, 0)L`**.
`Uniform Curve Sprite Scale`: `(0, 0.5)C (0.2, 0.9)C (1, 1)L`.
Update order: 1 Particle State, 2 Scale Color, 3 Dynamic Material Parameters, 4 Scale Sprite Size.

### 5.2 `Ring` — 1 sprite, spawn 0, lifetime 0.3, size **150**

Init colour `RGBA(0.913099, 0.191202, 1, 0.608)`; module-level **`Color.Scale Alpha = 0.5`**;
`Uniform Sprite Size = 150` (`Min/Max 150/160` stored, unused); `Lifetime` Direct Set 0.3
(`Min/Max 0.3/0.7` stored, unused); `Sprite Rotation Mode = Random`, 0…360.

`Dynamic Material Parameters → Index 0 Param 1` (**`dissolve`**): **`(0, 0)C (1, -1)C`**.

`Color` (`Color from Curve`) — **byte-identical to the Cast's `Ring`**:

| Channel | Keys |
|---|---|
| Red | `(0, 1)C (0.118322, 1)L (0.295804, 1)L (0.542107, 0.391573)C (0.843948, 0.009134)L` |
| Green | `(0, 1)C (0.118322, 0.693872)L (0.295804, 0.040915)L (0.542107, 0.003677)C (0.843948, 0.004025)L` |
| Blue | `(0, 1)C (0.118322, 0.147027)L (0.295804, 0.045186)L (0.542107, 0.022174)C (0.843948, 0.006995)L` |
| Alpha | `(0, 1)C` |

`Uniform Curve Sprite Scale`: **`(0, 0)C (0.25, 0.9)C (1, 1)C`** *(differs from the Cast's
`(0, 0.5)C (0.1, 0.9)C (1, 1)C` — the hit's ring starts from zero and grows slower)*.
Update order: 1 Particle State, 2 Dynamic Material Parameters, 3 Scale Sprite Size, 4 Color.

### 5.3 `Sparkles` — 7 sprites, spawn 0.05, WORLD

| Fact | Value |
|---|---|
| Lifetime | `Random`; `Lifetime Min/Max = **0.3 / 0.6**`; pin override `Random Range Float` min **0.2** max **0.4** `[unresolved: which is live — the pin override normally wins]` |
| Spawn shape | Sphere Location, `Sphere Radius = 20`, `Sphere Orientation Axis = (1,0,0)`, `Non Uniform Scale = (1,1,1)`, `Surface Only = false`, `Sphere Distribution = Random`, `Random Seed = 0` |
| Velocity | **`Add Velocity from Point`**, `Origin Offset = (0,0,0)`, `Velocity Falloff Distance = 100`, `Velocity Strength` ← `Random Range Float 001` **min 200, max 1200** |
| Size | `Random Uniform`, min **10**, max **20** |
| Sprite rotation | Random, 0…360 |
| Init colour | `RGBA(1, 1, 1, 1)`; `Color.Scale Alpha = 1` |
| Dyn `Param 1` | 1 |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`**.

`Color` (`Color from Curve`) — identical to the Cast's `Sparkles`:
`Red: (0.615756, 1)C | Green: (0.615756, 0.563224)C | Blue: (0.615756, 0.224)C |
Alpha: (0.613341, 1)C (1, 0)L`.

`Uniform Curve Sprite Scale`: `(0, 0)C (0.1, 1)C (1, 0)C`.
Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Particle State, 4 Color,
5 Dynamic Material Parameters, 6 Scale Sprite Size.

### 5.4 `Sparkles_Stretched` — 5 velocity-aligned sprites, spawn 0.05, WORLD

| Fact | Value |
|---|---|
| Lifetime | `Random`; `Lifetime Min/Max = **0.3 / 0.5**`; pin override `Random Range Float` min 0.2 max 0.4 `[unresolved]` |
| Spawn shape | Sphere Location, `Sphere Radius = 10`, `Surface Only = false` |
| Velocity | **`Add Velocity from Point`**, `Velocity Falloff Distance = 100`, `Velocity Strength` ← `Random Range Float 001` **min 800, max 2000** |
| Size | `Random Non-Uniform`, **min `(25, 70)`, max `(40, 60)`** *(min.y > max.y; per-axis lo/hi)* |
| Init colour | `RGBA(1, 1, 1, 1)`; module-level **`Color.Scale Alpha = 0.6`** |
| Dyn `Param 1` | 0 |
| Extra module | **`Scale Sprite Size by Speed`** — `Min Scale Factor = (1, 1)`, `Max Scale Factor = (1, 2)`, `Velocity Threshold = 1000`, `Scale Factor Curve = (0, 0)L (1, 1)L` |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (0.2, 0.2)C (1, -9.09372e-09)C`**
*(the Cast's is `0.2 → 0.25`; this one decays harder)*.

`Color` (`Color from Curve`) — identical to the Cast's:
`Red: (0, 1)C (0.088138, 1)L (0.615756, 1)L (0.98038, 0.391573)C |
Green: (0, 1)C (0.088138, 0.671611)L (0.615756, 0.025)L (0.98038, 0.003677)C |
Blue: (0, 1)C (0.088138, 0.085)L (0.615756, 0.0293413)L (0.98038, 0.022174)C |
Alpha: (0, 1)C (1, 0)C`

Two `Scale Sprite Size` modules:
- module 6: `Uniform: (0, 0)C (0.1, 1)C (1, 0)C`; `Non-Uniform: X: (0,0)L (1,1)L | Y: (0,0)L (1,1)L`
- module 7 (`001`): `Uniform: (0, 0)L (1, 1)L`;
  `Non-Uniform: X: (1, 1)L | Y: (0, 1)C (**0.3, 0.35**)C (1, 0.2)C` *(the Cast's mid key is 0.25)*

Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Color, 4 Particle State,
5 Dynamic Material Parameters, 6 Scale Sprite Size, 7 Scale Sprite Size 001,
8 Scale Sprite Size by Speed.

### 5.5 `Star_01` (and `Star_02`, DISABLED)

| | `Star_01` | `Star_02` *(DISABLED)* |
|---|---|---|
| Count / spawn t | 1 / **0.05** | 1 / 0.5 |
| Lifetime | 0.3 | 0.1 |
| `Uniform Sprite Size` | **20** | **70** |
| Init colour | `RGBA(1, 0.184475, 0.386429, 0.4)` | identical |
| Sprite rotation | angle **0.1** | `Direct Angle (Degrees)`, angle 0 |
| Dyn `Param 1` | 0 | 0 |
| `Uniform Curve Sprite Scale` | **`(0, 1.36552e-08)C (0.2, 1)C (1, 0)C`** | `(0, 1)C (1, 0)C` |
| `Color` module | present | **absent** |

`Star_01` `Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0, 1)C (0.115907, 1)L (0.636281, 1)L (0.9345, 0.391573)C` |
| Green | `(0, 1)C (0.115907, 0.693872)L (0.636281, 0.040915)L (0.9345, 0.003677)C` |
| Blue | `(0, 1)C (0.115907, 0.147027)L (0.636281, 0.045186)L (0.9345, 0.022174)C` |
| Alpha | `(0, 1)C` |

*(Note: the Cast's `Star_01` starts at RGB 2 — this one starts at 1.)*

Update order — `Star_01`: 1 Particle State, 2 Dynamic Material Parameters, 3 Scale Sprite Size,
4 Color. `Star_02`: 1 Particle State, 2 Dynamic Material Parameters, 3 Scale Sprite Size.

### 5.6 The impact glows — `Glow_01`, `Glow_02`, `SecondGlow`, `FirstGlow`

| | `Glow_01` | `Glow_02` | `SecondGlow` | `FirstGlow` |
|---|---|---|---|---|
| Count / spawn t | 1 / 0 | 1 / 0 | 1 / 0 | 1 / **0.05** |
| Lifetime | **0.1** | **0.1** | **0.1** | **0.2** |
| `Uniform Sprite Size` | **1000** | **100** | **170** | **1000** |
| Init colour | `RGBA(0.102242, 0.658375, 1, 0.2)` | same | same | same |
| Module alpha scale | **0.6** | **1** | **1** | **0.6** |
| Dyn `Param 1` | 1 | 1 | **3.5** | 1 |
| Material | `Part01` | `Part01` | `Part03_Bright` | `Part01` |
| `Uniform Curve Sprite Scale` | `(0, 0.5)C (0.1, 0.9)C (1, 1)C` | same | **`(0, 0.5)C (1, 1)C`** | `(0, 0.5)C (0.1, 0.9)C (1, 1)C` |
| `Non-Uniform Curve Sprite Scale` | — | — | `X: (0,1)C (1,0.4)C \| Y: (0,1)C (1,0)C` | same as `SecondGlow` |

`Glow_01` `Color` — identical to the Cast's:
`Red: (0.0881376, 1)L (0.377905, 1)L (0.9345, 0.391573)C |
Green: (0.0881376, 0.693872)L (0.377905, 0.0409152)L (0.9345, 0.00367651)C |
Blue: (0.0881376, 0.147027)L (0.377905, 0.0451862)L (0.9345, 0.0221739)C |
Alpha: (0, 1)C (1, 0)C`

`Glow_02` `Color` — identical to the Cast's:
`Red: (0.214911, 1)L (0.817386, 1)L | Green: (0.214911, 0.693872)L (0.817386, 0.0409152)L |
Blue: (0.214911, 0.147027)L (0.817386, 0.0451862)L | Alpha: (0.406882, 1)C (1, 0)C`

`SecondGlow` `Color` — **over-1, and different from the Cast's**:
`Red: (0.618171, 2)L (0.963477, 3)L | Green: (0.618171, 0.683829)L (0.963477, 2.25883)L |
Blue: (0.618171, 0.218923)L (0.963477, 0.328385)L | Alpha: (0, 0)C (0.246302, 1)C`

`FirstGlow` `Color`:
`Red: (0, 1)C (0.111078, 1)L (0.322366, 1)L (0.96227, 0.913099)L |
Green: (0, 0.938686)C (0.111078, 0.752942)L (0.322366, 0.341915)L (0.96227, 0.0241576)L |
Blue: (0, 0.791298)C (0.111078, 0.109462)L (0.322366, 0.109462)L (0.96227, 0.0241576)L |
Alpha: (0, 1)C (1, 0)C`

Update order on all four: 1 Particle State, 2 Dynamic Material Parameters, 3 Scale Sprite Size,
4 Color.

### 5.7 `FirstFlash` — 4 sprites, spawn 0.04, lifetime 0.1, size 300

Init colour `RGBA(0.102242, 0.658375, 1, 0.2)`; module-level **`Color.Scale Alpha = 1`**;
dyn `Param 1 = 1`. `Uniform Curve Sprite Scale`: `(0, 0.5)C (0.1, 1)L (1, 1)L`.

`Color` (`Color from Curve`) — identical to the Cast's `FirstFlash`:

| Channel | Keys |
|---|---|
| Red | `(0.0374283, 0.715694)C (0.16058, 1)L (0.312708, 1)L (0.659221, 0.913099)L (0.96227, 0.0137021)C` |
| Green | `(0.0374283, 0.89627)C (0.16058, 0.752942)L (0.312708, 0.341915)L (0.659221, 0.0241576)L (0.96227, 0.00182116)C` |
| Blue | `(0.0374283, 1)C (0.16058, 0.109462)L (0.312708, 0.109462)L (0.659221, 0.0241576)L (0.96227, 0.00802319)C` |
| Alpha | `(0, 1)C` |

Update order: 1 Particle State, 2 Dynamic Material Parameters, 3 Scale Sprite Size, 4 Color.

### 5.8 `Flare01` — 2 sprites, spawn 0, lifetime 0.2, size 120

Init colour `RGBA(0.913099, 0.0193824, 0.130136, 0.4)`; module-level **`Color.Scale Alpha = 0.6`**;
`Sprite Rotation Mode = Random`, 0…360.

`Dynamic Material Parameters → Index 0 Param 1` (**`dissolve`**): **`(0, 1)C (1, -1)C`**.

`Color` (`Color from Curve`) — **starts at RGB 2, unlike the Cast's which starts at 1**:

| Channel | Keys |
|---|---|
| Red | `(0, 2)C (0.142469, 2)L (0.47208, 1)L (0.9345, 0.391573)C` |
| Green | `(0, 2)C (0.142469, 1.38774)L (0.47208, 0.040915)L (0.9345, 0.003677)C` |
| Blue | `(0, 2)C (0.142469, 0.294054)L (0.47208, 0.045186)L (0.9345, 0.022174)C` |
| Alpha | `(0, 0)C (0.214911, 1)L (1, 0)C` |

`Uniform Curve Sprite Scale`: `(0, 0.5)C (0.1, 1)L (1, 1)L`.
Update order: 1 Particle State, 2 Dynamic Material Parameters, 3 Scale Sprite Size, 4 Color.

### 5.9 `FlareImpact` — 1 sprite, spawn 0.05, lifetime **0.05 s**, LOCAL, size 150

The shortest-lived particle in the batch. Init colour `RGBA(0.644888, 0.2, 1, 1)`;
`Color.Scale Alpha = 1`.

`Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0, 1)C (0.131603, 1)L (0.601268, 1)L (0.828252, 0.391573)C (0.997283, 0.009134)L` |
| Green | `(0, 1)C (0.131603, 0.693872)L (0.601268, 0.040915)L (0.828252, 0.003677)C (0.997283, 0.004025)L` |
| Blue | `(0, 1)C (0.131603, 0.147027)L (0.601268, 0.045186)L (0.828252, 0.022174)C (0.997283, 0.006995)L` |
| Alpha | `(0, 1)C` |

`Dynamic Material Parameters → Index 0 Param 1` (**`dissolve`**): **`(0, 0.5)C (1, -1)C`**.
`Uniform Curve Sprite Scale`: `(0, 0.5)C (0.1, 0.9)C (1, 1)C`.
Update order: 1 Particle State, 2 Color, 3 Dynamic Material Parameters, 4 Scale Sprite Size.

### 5.10 `Spike01` / `Spike02` — 6 pyramid meshes, spawn 0.05, LOCAL, Facing **Velocity**

Both: burst **3**; `Lifetime Mode = Random` **min 0.1, max 0.15** (`InitializeParticle.Lifetime =
0.1` stored, unused); `Mesh Scale Mode = Random Non-Uniform`; Sphere Location with
`Sphere Radius = 10`, **`Non Uniform Scale = (0.1, 0.1, 0.1)`**, **`Hemisphere X = false`**,
`Surface Only = false`; **`Add Velocity from Point`** with `Velocity Strength = **10**` (static),
`Velocity Falloff Distance = 100`; `Initial Mesh Orientation` with `Orientation Axis = (0, 0, 1)`,
`Orientation Vector = (1, 0, 0)`, `Rotation` ← `Random Range Vector` **min `(0, 0, 1)`,
max `(0, 0.5, −1)`**; init colour `RGBA(1, 0.184475, 0.386429, 1)`, `Color.Scale Alpha = 1`;
`Uniform Sprite Size = 500` (inert); **no dynamic parameters written**.

| | `Spike01` | `Spike02` |
|---|---|---|
| `Mesh Scale Min` | `(0.2, 0.2, 0.4)` | **`(0.3, 0.3, 0.6)`** |
| `Mesh Scale Max` | `(0.2, 0.2, 0.7)` | **`(0.3, 0.3, 1)`** |

`Color` (`Color from Curve`) — **identical on both**:

| Channel | Keys |
|---|---|
| Red | `(0, 1)C (0.118322, 1)L (0.515545, 1)L (0.671295, 0.391573)C (0.843948, 0.009134)L` |
| Green | `(0, 1)C (0.118322, 0.693872)L (0.515545, 0.040915)L (0.671295, 0.003677)C (0.843948, 0.004025)L` |
| Blue | `(0, 1)C (0.118322, 0.147027)L (0.515545, 0.045186)L (0.671295, 0.022174)C (0.843948, 0.006995)L` |
| Alpha | `(0, 1)C` |

`Scale Mesh Size` → `Scale Factor` (`Scale Float by Curve`) — **identical on both**:
**`X: (0, 0)C (0.2, 0.5)C (1, 4.17233e-08)C | Y: (0, 0)C (0.2, 0.4)C (1, 5.66244e-08)C |
Z: (0, 0)C (0.2, 1)C`**.

Update order on both: 1 Solve Forces and Velocity, 2 Particle State, 3 Color, 4 Scale Mesh Size.
`ScaleFloatByCurve.InitialValue = (1,1,1)`.

### 5.11 `LightningStrip` — 5 card meshes, spawn 0, LOCAL, Facing **Velocity**

**This is a MESH renderer here** (`SM_VFX_Plane01`), unlike NS_FireBall_Cast where the same-named
emitter is a velocity-aligned sprite. Same as [NS_Bomb_Explosion.md](NS_Bomb_Explosion.md) §5.6.

| Fact | Value |
|---|---|
| Lifetime | `Direct Set` **0.2** (`Min/Max 0.1/0.2` stored, unused) |
| Mesh scale | `Mesh Scale Mode = Non-Uniform`, `Mesh Scale` ← `Random Range Vector 001` **min `(0.3, 0.3, 1.5)`, max `(0.7, 0.7, 3)`** |
| Spawn shape | **Sphere Location — DISABLED** (`Sphere Radius = 50`, `Hemisphere Z = true`, `Surface Only = false`); so all 5 spawn at the origin |
| Velocity | **`Add Velocity from Point`**, `Velocity Strength = **500**` (static), `Velocity Falloff Distance = 100`, `Origin Offset = (0,0,0)` |
| Initial Mesh Orientation | `Orientation Axis = (0, 0, 1)`, `Orientation Vector = (1, 0, 0)`; `Rotation` ← `Random Range Vector` **min `(0, 0, 1)`, max `(0, 0.5, −1)`** |
| Init colour | `RGBA(0.341915, 0.184475, 1, 1)`; module-level **`Color.Scale Alpha = 0.3`** |
| Dyn `Param 1/2/3/4` | 0 / 0 / 0 / 0 |
| Sprite size values | `Sprite Size = (200, 180)`, `Uniform Sprite Size = 50` etc. — **inert** on a mesh renderer |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`**.

`Color` (`Color from Curve`) — identical to the Cast's and to NS_Bomb_Explosion's:

| Channel | Keys |
|---|---|
| Red | `(0.276487, 1)L` |
| Green | `(0.276487, 0.366253)L` |
| Blue | `(0.276487, 0.184475)L` |
| Alpha | `(0, 0)L (0.274072, 1)C (1, 0)C` |

`Scale Mesh Size` → `Scale Factor` (`Vector from Curve 001`):
**`X: (0, 0.5)C (0.2, 1)C (1, -1.44926e-08)C | Y: (0, -2.4747e-08)C (1, 1)C |
Z: (0, -5.68323e-08)C (0.2, 0.75)C (1, 1)C`**.

Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Color, 4 Particle State,
5 Dynamic Material Parameters, 6 Scale Mesh Size.

### 5.12 `Flames01` — 5 sprites, spawn 0, WORLD, **SubUV 2×2**

| Fact | Value |
|---|---|
| Lifetime | `Random`; `Lifetime Min/Max = **0.2 / 0.7**`; pin override `Random Range Float` min 0.2 max 0.4 `[unresolved]` |
| Spawn shape | Sphere Location, `Sphere Radius = 20`, `Surface Only = true` (`Surface Expansion Mode = Outside`, `Band Thickness = 0`), **`Hemisphere Z = false`** *(the Cast's is `true`)*, `Radius Position = 1`, `U Position = 0`, `V Position = 0.5`, `Uniform Distribution = 1`, `Uniform Spiral Amount = 1` |
| Velocity | **`Add Velocity from Point`**, `Velocity Strength` ← `Random Range Float 002` **min 50, max 300** |
| Size | `Random Uniform`, min **50**, max **100** |
| Sprite rotation | Random 0…360; **`Sprite Rotation Rate` ← `Random Range Float 001`, min −30, max 30** |
| **Sub UV** | `Start Frame = 0`, `End Frame = 3`, `SubUV Loop Count = 1`; renderer `SubUV: 2x2` |
| Init colour | `RGBA(1, 1, 1, 1)`; `Color.Scale Alpha = 1` |
| Dyn params | `Index 0 Param 2 = **5**`, `3/4 = 0`; **`Param3WriteEnabled = true`** |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (1, 0.2)C`**.

`Color` (`Color from Curve`) — identical to the Cast's `Flames`:

| Channel | Keys |
|---|---|
| Red | `(0.0796861, 5)L (0.368246, 3)L (0.738907, 0.250158)L` |
| Green | `(0.0796861, 3.43343)L (0.368246, 0.67227)L (0.738907, 0.00749903)L` |
| Blue | `(0.0796861, 0.115767)L (0.368246, 0.0841786)L (0.738907, 0.00749903)L` |
| Alpha | `(0, 0)L (0.303049, 1)L (0.992454, 0)L` |

`Dynamic Material Parameters → Index 0 Param 1`: **`(0, -2.46502e-08)C (1, -1)C`**.
`Uniform Curve Sprite Scale`: `(0, 0.5)C (0.2, 0.9)C (1, 1)C`.
Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Particle State, 4 Color,
5 Dynamic Material Parameters, 6 Scale Sprite Size, 7 Sprite Rotation Rate.

### 5.13 `Smokes` — 5 sprites, spawn 0.04, WORLD

| Fact | Value |
|---|---|
| Lifetime | `Random`; `Lifetime Min/Max = **0.7 / 1.3**`; pin override `Random Range Float` min 0.2 max 0.4 `[unresolved]` |
| Spawn shape | Sphere Location, `Sphere Radius = 20`, **`Non Uniform Scale = (1, 1, 0)`**, `Surface Only = true` / `Outside`, **`Hemisphere Z = false`** *(the Cast's is `true`)* |
| Velocity | **`Add Velocity from Point`**, `Velocity Strength` ← `Random Range Float 002` **min 50, max 200** |
| Size | `Random Uniform`, min **100**, max **200** |
| Sprite rotation | Random 0…360; `Sprite Rotation Rate` ← `Random Range Float 001`, min −30, max 30 |
| Init colour | `RGBA(1, 1, 1, 1)`; module-level **`Color.Scale Alpha = 0.3`** |
| Dyn `Param 2/3` | 0 / 0 |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (0.2, 0.2)C (1, 0.1)C`**
*(the Cast's mid key is 0.3)*.

`Color` (`Color from Curve`) — note the duplicated t = 0 keys, verbatim; **this is the
NS_FireBall_Projectile variant of the curve (mid key at 0.22457 / 0.363417), not the Cast's**:

| Channel | Keys |
|---|---|
| Red | `(0, 0.009134)L (0, 1)C (0.0603682, 1)L (0.22457, 1)L (0.363417, 0.391573)C (0.527618, 0)L` |
| Green | `(0, 0.004025)L (0, 1)C (0.0603682, 0.693872)L (0.22457, 0.040915)L (0.363417, 0.003677)C (0.527618, 0)L` |
| Blue | `(0, 0.006995)L (0, 1)C (0.0603682, 0.147027)L (0.22457, 0.045186)L (0.363417, 0.022174)C (0.527618, 0)L` |
| Alpha | `(0.126773, 1)C (0.514337, 0.35)L` |

`Dynamic Material Parameters`:
- `Index 0 Param 1` (**`dissolve`**): `(0, -2.46502e-08)C (1, -1)C`
- `Index 0 Param 4` (**`core_color`**): **`(0, -1)C (0.25, 1)C`** *(the Cast's second key is at 0.4)*

`Uniform Curve Sprite Scale`: `(0, 0.5)C (0.2, 0.9)C (1, 1)C`.
Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Particle State, 4 Color,
5 Dynamic Material Parameters, 6 Scale Sprite Size, 7 Sprite Rotation Rate.

### 5.14 Disabled emitters — recorded, not implemented

`Wind_01` (mesh `SM_VFX_Ring01`, renderer scale `(1,1,5)`, lifetime 1.5, `Add Velocity (-200,0,0)`,
`Color.Scale Alpha 0.7`, near-black colour with alpha envelope `(0,0)L (0.240266,1)C (0.676124,1)L
(1,0)L`, `Scale Mesh Size X/Y: (0,1.5)C (0.2,2)C | Z: (0,0.5)C (0.4,3)C (0.7,4.75)C (1,5)C`,
`Update Mesh Orientation Rotation Rate 0.3` about `(1,0,0)`, dyn `Param 1` curve
`(0,-0.2)C (1,-1)C`) and `Wind_02` (5 sprites, SubUV 2×2, size Random Uniform 130…230,
`Add Velocity` RRV min `(-100,-20,-20)` max `(-700,20,20)`, dyn `Param 1` curve
`(0,-5.88215e-08)C (1,-1)C`) are **byte-identical to NS_FireBall_Cast's enabled `Wind_01`/`Wind_02`**
— see [NS_FireBall_Cast.md](NS_FireBall_Cast.md) §5.9 / §5.10 for the full transcription.
`Star_02` is covered in §5.5.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new row in `ck::particles::Get_TemplateSpecs()` is required.**

| Field | Value | Why |
|---|---|---|
| Loop duration | `[unresolved: system-level loop, §2]` — **resolve before writing the row** | `Life Cycle Mode = System` |
| Particle lifetime | **≥ 1.34 s** | the longest layer is `Smokes` at up to 1.3 s from a spawn at t = 0.04 → alive to t ≈ 1.34 s. **If the `[unresolved]` lifetime override (0.2–0.4) is the live one, this drops to ≈ 0.44 s** — resolve §5.13 first, the answer changes the row by 3× |
| Burst count | **47** (enabled only) | the §2 total; 54 if the three disabled emitters are restored |

Layer partition `Seed % 47` with the layer→emitter map from §2. The 0 / 0.04 / 0.05 spawn beats are
reproduced by hiding a layer for `Age < SpawnDelay` and running its curves on
`(Age − SpawnDelay) / Lifetime`, per NS_BasicAttack §5.

**The disabled emitters must be a recorded decision.** If the recreation ships 47 layers, say so;
if it ships 54 and re-enables the wind, say that. Silently matching the enabled set is fine —
silently *not saying which* is what README §3 forbids.

### 6.2 VisTag / renderer needs

| Source emitters | Renderer needed | Available today? |
|---|---|---|
| 12 camera-facing sprites across **8 distinct materials** (`Part01` ×3, `Part03_Bright` ×2, `Part01_Bright`, `Rainbow`, `Ring01`, `Star01`, `Flare01`, `Impact01`, `Smoke01`) | camera-facing sprite, per-look | **NO — gap 1** |
| `Sparkles_Stretched` | velocity-aligned sprite, `Part04` | **YES** — shared VisTag 1 or one row-declared `VelocityAlignedSprite` |
| `Spike01`, `Spike02`, `LightningStrip` | mesh, Facing **Velocity**, 2 meshes / 2 looks | **partial — gap 3**; Default-facing mesh row renderer exists, velocity facing does not |
| `Flames01` | camera-facing sprite **with SubUV 2×2** | **NO — gap 2** |

### 6.3 Mesh needs

Both required meshes are procedurally reproducible (§3), and **both are already required by sibling
sheets** — build each once:

| Generated mesh (proposed) | Reproduces | Also needed by |
|---|---|---|
| `Spike` | `SM_VFX_Spike01` — square pyramid, base ±100 XY at Z = 0, apex `(0,0,200)`, 6 tris; **v = 0 tip → 1 base** | [NS_Bomb_Explosion.md](NS_Bomb_Explosion.md) §6.3, [NS_FireBall_Cast.md](NS_FireBall_Cast.md) §6.3 |
| `Card` | `SM_VFX_Plane01` — flat quad in XZ, `X ±100`, `Z 0…200`; u = −X→+X, **v = top (0) → bottom (1)**; drop the 0.0643 Y offset, make the look `_TwoSided` | [NS_Bomb_Explosion.md](NS_Bomb_Explosion.md) §6.3 |

`TubeBand` (`SM_VFX_Ring01`) is needed **only if `Wind_01` is re-enabled**.

### 6.4 Look / texture needs

Ten to twelve looks, **all parameterizations of the existing `CkUsf_Look_DissolveAdd`** except the
spikes' tiny `M_VFX_FlatAdd` shader (shared with NS_Bomb_Explosion and NS_FireBall_Cast). CkUsf
family parameters **not plumbed today** that this effect needs:

| Parameter | Used by |
|---|---|
| `CamOffset` (50) | `Part03_Bright` (`SecondGlow`, `FirstFlash`) |
| `Core_Intensity` (1) | `Part01_Bright`, `Part03_Bright`, `Flames01`, `Smoke01` |
| `Core_Power` (0) | `LightStrip`, `Impact01` |
| `Glow_Intensity` (2) | `Flames01` |
| `Gradient_Invert` (0.847619 / 2 / 0) | `Flare01`, `Rainbow`, others |
| **gradient-map LUT chain** | `Rainbow` |
| separate `GradientShape_Tex` | `Rainbow`, `Flare01` |
| separate `Main_Tex` vs `Color_Tex` | `Smoke01` |

Textures: 17 distinct in the enabled set (§4.3). Reuse `SoftParticle` / `SparkStreak` / `TileNoise`;
measure the six "candidate" mappings before assuming; **new bakes** for six, plus a colour-LUT kind
and a 2×2 sub-UV atlas shape.

### 6.5 Capability gap callout

| # | Gap | Severity |
|---|---|---|
| 1 | **No row-declared camera-facing sprite kind.** 12 of the 17 enabled emitters are `Unaligned`/`FaceCamera` across **8 materials**; one `User.SpriteMaterial` binding cannot carry them. Additive fix to `ECk_ParticlesRenderer_Kind`; shared with every sheet in this batch. | **BLOCKING** |
| 2 | **No sub-UV / flipbook support anywhere in CkParticles.** `Flames01` renders `SubUV: 2x2` driven by `Sub UVAnimation` (frames 0–3, 1 loop). The DI writes no sub-image index, the builder sets no `SubImageSize`, the generator bakes no atlases. Without it the flames read as static sprites. | **BLOCKING** for 1 emitter |
| 3 | **Mesh renderer facing mode is not expressible** on a row-declared `Mesh` renderer (`Kind`/`VisTag`/`MeshName`/`LookName` only). Three emitters (`Spike01`, `Spike02`, `LightningStrip`) need `Facing: Velocity`. **Workaroundable** — the behavior owns the velocity and can write an `Orientation` quat from it. | Medium |
| 4 | **System-level loop parameters absent from the corpus** (§2). Cadence cannot be finalized. | **Prerequisite** |
| 5 | **Four `[unresolved]` lifetime ranges** (§5.3, §5.4, §5.12, §5.13). `Smokes`' resolution changes the template lifetime by 3× (§6.1) — this is the highest-leverage unresolved item in the sheet. | **Prerequisite** |
| 6 | **World space on 12 of 17 enabled emitters**; the template is local-space (NS_BasicAttack §13.2). A hit effect fires at a fixed impact point, so risk is **low** — unlike the projectile sheet. | Low |
| 7 | **Eight unplumbed CkUsf family parameters** (§6.4), including the gradient-map LUT chain and a separate `GradientShape_Tex`. | Medium |
| 8 | **A colour-LUT texture output kind** and a **2×2 sub-UV atlas shape** are new *kinds* of bake, not new functions. | Medium |
| 9 | **Three disabled emitters** must be a recorded decision, not a silent omission (§6.1). | Discipline |
| 10 | **`LightningStrip` is a different renderer kind here than in NS_FireBall_Cast** (mesh vs velocity-aligned sprite), despite the identical emitter name and an identical colour curve. Copying the translation between sheets is the trap. | None (trap) |
| 11 | **Inert modules that look load-bearing.** `LightningStrip`'s sprite-size values on a mesh renderer; the DISABLED `Sphere Location` on the same emitter (so all 5 spawn at the origin). Do not implement them. | None (trap) |
| 12 | **`Opacty_DepthFade`** 20 / 30 / 10 / 0 across instances; CkUsf surface looks do not wire scene depth (known gap, NS_Lightning_Range §13.4). | Low (known) |

**Complexity tier: M**, conditional on gap 1 landing and gap 3 being solved by the
orientation-from-velocity workaround. This is **the most tractable of the three FireBall sheets**:
no ribbons, no continuous-rate emitters, no hand-authored meshes, only one sub-UV emitter, a single
short cadence, and a fixed impact point that makes the local-space deviation nearly free.
**If gap 2 is descoped** (ship `Flames01` as a static sprite and record it as a fidelity gap in
§13), 16 of 17 emitters are reachable and the tier holds at **M**.

### 6.6 Behavior id

**Do not allocate an id in this document.** Take the next free id from
`ck::particles::NumBehaviors` at implementation time, bump it, and update the roster paragraph in
`CkParticles/CLAUDE.md`. Five sibling sheets were written in the same pass and none allocates an id
— allocate them in one ordered pass so they cannot collide.

---

## 7+. Reserved for implementation

Sections 7–14 of the recipe schema ([README.md](README.md)) are intentionally absent and are to be
written by the implementation session, from what actually happened.
