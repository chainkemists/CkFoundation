# Recipe: NS_FireBall_Cast → CkParticles (PRE-IMPLEMENTATION TRANSLATION SHEET)

Schema and evidence-tag conventions: [README.md](README.md).
Exemplars this sheet copies: [NS_BasicAttack.md](NS_BasicAttack.md) §1–6, [NS_Lightning_Range.md](NS_Lightning_Range.md) §1–5.

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior, no `.ush`, no CPU mirror, no look, no mesh, no texture, no cadence row, no test, no gym
station exists for this effect. No behavior id has been allocated. Nothing has been rendered or
visually compared. Sections 1–6 are archaeology and a plan; everything below `## 7+` is reserved for
the implementation session.

**This is the largest system in the batch by emitter count (26) and the most heterogeneous.** Read
§6.5 before scoping it.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_FireBall_Cast` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior id | **not allocated** — take the next free id at implementation time |
| Recreation status | not started |

Corpus evidence (all `[corpus]`, exported 2026-08-01):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_FireBall_Cast.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part01_Bright,Part03_Bright,Part04,Rainbow,Ring01,Star01,Star02,Star03,Wind01,Wind02,LightStrip,Flat02,Flames01,Smoke01,Flare01}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_FlatAdd.json`
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_{Ring01,Spike01}.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/*.json`

**The source Niagara asset was not opened for editing.**

> ### A same-named sibling exists — and the casing differs
> `[corpus]` `Vefects/Anime_Stylized_VFX/VFX/Particles/**NS_Fireball_Cast**` — lowercase **b**.
> **Fastest robust discriminator: the sibling declares 8+ user parameters** (`User.First Flash
> Color 01`, `User.Flames Color 01`, `User.Flare Color 01`, `User.Flare Stretched Color 01…04`,
> `User.Glow Color 01`, several of them `NiagaraDataInterfaceColorCurve`) and renders through
> `MI_VFX_*` instances (`MI_VFX_Glow_01`, `MI_VFX_Star_01…03`, `MI_VFX_Lens_Rainbow_01`,
> `MI_VFX_Wind_01/02`, `MI_VFX_Flat_01`, …). The `Anime_VFX/Shared/Skills` variant documented here
> has an **empty user-parameter list** and renders through `M_VFX_DisAdd_*`.

---

## 2. System anatomy `[corpus]`

**26 CPU emitters, all enabled, all `Spawn Burst Instantaneous`. 50 particles per loop.
24 sprite renderers + 2 mesh renderers.**

| # | Emitter | Space | Count | Spawn t | Lifetime | Renderer | Mesh | Material |
|---|---|---|---|---|---|---|---|---|
| 0 | `Raimbow` | World | 1 | **0.5** | 0.2 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Rainbow` |
| 1 | `Ring` | World | 1 | **0.5** | 0.4 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Ring01` |
| 2 | `Sparkles` | World | **10** | **0.55** | rand | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01_Bright` |
| 3 | `Glow_01` | World | 1 | 0 | 0.5 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 4 | `Sparkles_Stretched` | World | **3** | **0.55** | rand | Sprite, **VelocityAligned**/FaceCamera | — | `M_VFX_DisAdd_Part04` |
| 5 | `Flare_Stretched_01` | World | 1 | 0 | 0.5 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 6 | `Flare_Stretched_02` | World | 1 | 0 | 0.5 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part03_Bright` |
| 7 | `Flare_Stretched_03` | World | 1 | 0 | 0.5 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part03_Bright` |
| 8 | `Flare_Stretched_04` | World | 1 | 0 | 0.5 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Star03` |
| 9 | `Star_02` | World | 1 | **0.5** | 0.2 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Star02` |
| 10 | `Star_01` | World | 1 | **0.55** | 0.3 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Star01` |
| 11 | `Glow_02` | World | 1 | 0 | 0.5 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 12 | `FirstGlow` | World | 1 | 0 | 0.5 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 13 | `SecondGlow` | World | 1 | 0 | 0.5 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part03_Bright` |
| 14 | `ShootFlash_01` | World | 1 | **0.5** | 0.1 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 15 | `ShootFlash_02` | World | 1 | **0.5** | 0.1 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 16 | `FirstFlash` | World | **4** | **0.54** | 0.1 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part03_Bright` |
| 17 | `SecondFlash_01` | World | 1 | **0.55** | 0.2 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 18 | `SecondFlash_02` | World | 1 | **0.5** | 0.1 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01_Bright` |
| 19 | `Wind_01` | **Local** | 1 | **0.55** | **1.5** | **Mesh**, Facing Default, renderer **scale (1, 1, 5)** | `SM_VFX_Ring01` | `M_VFX_DisAdd_Wind02` |
| 20 | `Wind_02` | **Local** | **5** | **0.55** | **1.5** | Sprite, Unaligned/FaceCamera, **SubUV 2×2** | — | `M_VFX_DisAdd_Wind01` |
| 21 | `LightningStrip` | **Local** | 1 | **0.55** | 0.2 | Sprite, **VelocityAligned**/FaceCamera | — | `M_VFX_DisAdd_LightStrip` |
| 22 | `Spike01` | **Local** | **3** | **0.55** | rand 0.1–0.15 | **Mesh**, Facing **Velocity** | `SM_VFX_Spike01` | `M_VFX_DisAdd_Flat02` |
| 23 | `Flames` | World | **4** | **0.5** | rand | Sprite, Unaligned/FaceCamera, **SubUV 2×2** | — | `M_VFX_DisAdd_Flames01` |
| 24 | `Smokes` | World | **2** | **0.5** | rand | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Smoke01` |
| 25 | `Flare01` | World | 1 | **0.5** | 0.1 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Flare01` |

**Total per loop: 50 particles.** `Bounds: Dynamic`, `Determinism: false` everywhere.

**The effect is two acts, and the split is the structure:**

| Act | Spawn t | Emitters | Particles |
|---|---|---|---|
| **Charge-up** | **0** | `Glow_01`, `Flare_Stretched_01…04`, `Glow_02`, `FirstGlow`, `SecondGlow` | 8 |
| **Release** | **0.5 – 0.55** | everything else (18 emitters) | 42 |

Half a second of gathering glow and stretched flares, then a 42-particle discharge over a 50 ms
window. **`FirstFlash` at 0.54 s is the only spawn time between 0.5 and 0.55** — it is the transient
between the two release beats.

> **`Life Cycle Mode = System` on all 26 emitters `[corpus]`.** The stored per-emitter Loop
> Behavior / Loop Duration are therefore **inert leftovers** — including the three distinct values
> the corpus exports (1.0 s on 21 emitters, **5.0 s** on `Sparkles_Stretched`, **0.3 s** on
> `Star_01`, `Star_02`, `Flames`, `Smokes`) and the `Loop Behavior = Once` on those same five.
> **System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
> `UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
> Per [P0-D1] this is the authority. *(Was `[unresolved]`.)*
>
> **This resolves an apparent contradiction, and it is worth stating explicitly**: `Star_01` spawns
> at t = 0.55 while carrying `Loop Duration = 0.3`, and `Flames`/`Smokes` spawn at 0.5 with the
> same 0.3. Read at face value those bursts could never fire. They fire because the emitter's own
> loop settings are inert under `Life Cycle Mode = System`. **Do not "fix" the spawn times.**

---

## 3. Mesh geometry `[corpus]`

### 3.1 `SM_VFX_Spike01` — 16 verts / 6 tris — `Spike01` (3 particles)

| Fact | Value |
|---|---|
| Verts / tris | 16 / 6 |
| Bounds | `X −100…100`, `Y −100…100`, `Z 0…200` |
| uv0 | `(0,0)` … `(1,1)` |
| Section 0 | slot `Material` → `M_VFX_DisAdd_Slash01` *(the mesh's default slot; **overridden** by the emitter's renderer)* |

Exact geometry `[corpus, from the .obj]` — **a square pyramid**: apex `(0, 0, 200)`, base square
`(±100, ±100, 0)`; 4 side triangles + 2 base triangles. Radius-from-origin min 141.421 (base
corners), max 200 (apex).

UVs: `corr(v, Z) = −1.000` exactly and `corr(v, radiusXY) = 1.000`; `corr(u, Y) = −1.000`.
**v = 0 at the tip, v = 1 at the base**; u runs across the spike (0 on the +Y side, 1 on the −Y
side, 0.5 at the apex). UV list verbatim: `(0,1) (0.5,0) (0,1) (0,1) (0.5,0) (1,1) (1,1) (0.5,0)
(0,1) (1,1) (0.5,0) (1,1) (0,1) (0,1) (1,1) (1,1)`.

**Trivially procedural.** Six triangles of generator code.

### 3.2 `SM_VFX_Ring01` — 132 verts / 128 tris — `Wind_01` (1 particle)

| Fact | Value |
|---|---|
| Verts / tris | 132 / 128 |
| Bounds min / max | `(-100, -100, 0)` / `(100, 100, 50)` |
| Bounds size | `(200, 200, 50)` |
| uv0 | `(0,0)` … `(1,1)` |
| Section 0 | slot `WorldGridMaterial` → `M_VFX_DisAdd_Slash01` *(default slot, overridden)* |

OBJ characterization `[corpus, from the .obj]`:

- **Only two distinct radii, 99.5 and 100.0, and only two distinct Z, 0 and 50** — this is a
  **thin open cylinder (a tube band)**, double-walled: an outer wall at r = 100 and an inner wall at
  r = 99.5, 50 units tall, 64 segments around (angle step 11.25°).
- **UVs**: `corr(v, Z) = −1.000` exactly, and `v` takes only the values `{0, 1}` — **v = 0 at the
  TOP (Z = 50), v = 1 at the BOTTOM (Z = 0)**. `u` wraps once around the circumference:
  `u = 0.25` at −180°, `u = 0.75` at 0°, `u = 0.2812` at +168.75°, `u = 0.7812` at −11.25° →
  **`u = 0.75 + angle/360` mod 1, seam at ±180°, increasing clockwise from +X**. `corr(u, X) = 0.742`
  (the imperfect value is the seam wrap, not an irregularity).
- The renderer applies **`scale: (1, 1, 5)`** on top, so the drawn tube is **250 units tall** before
  the particle's own `Mesh Uniform Scale = 0.3` and its `Scale Mesh Size` curve (§5.20).

**Procedural.** A 64-segment, 2-radius, 2-height tube with that exact UV convention. The existing
`Tube` carrier in `CkParticles_MeshGenerator.cpp` may already be this shape — **check its UV
convention against the measurements above before reusing it.**

---

## 4. Material families and per-instance deltas `[corpus]`

**Two parent graphs**: `M_VFX_DissolveAdd` (15 instances) and `M_VFX_FlatAdd` (1 instance).

### 4.1 `M_VFX_DissolveAdd` family — 15 of the 16 materials

`M_VFX_DisAdd_{Part01, Part01_Bright, Part03_Bright, Part04, Rainbow, Ring01, Star01, Star02,
Star03, Wind01, Wind02, LightStrip, Flames01, Smoke01, Flare01}`. Identical base properties on all
fifteen: `MD_Surface`, **`BLEND_Translucent`**, **`MSM_Unlit`**, `twoSided: false`, outputs
**`EmissiveColor` + `Opacity`**, dynamic-parameter channels
**`[dissolve, distortion, offset, core_color]`**, and the family expression histogram
(`ScalarParameter ×41`, `Multiply/Add/AppendVector ×18` each, `Saturate ×12`, `DynamicParameter ×8`,
`Reroute ×8`, `TextureSampleParameter2D ×6`, `Panner ×5`, `Constant ×5`, `TextureCoordinate ×5`,
`LinearInterpolate ×4`, `Clamp ×2`, `OneMinus ×2`, one each of `DepthFade`, `ParticleColor`,
`Power`, `SmoothStep`, `StaticBoolParameter`, `StaticSwitch`, `VectorParameter`,
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
| `M_VFX_DisAdd_Part01` | *(reference)* — `Glow_01`, `Glow_02`, `FirstGlow`, `Flare_Stretched_01`, `ShootFlash_01/02`, `SecondFlash_01` |
| `M_VFX_DisAdd_Part01_Bright` | `Brightness` → **10**; `Core_Intensity` → **1**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Part_02`** |
| `M_VFX_DisAdd_Part03_Bright` | `Brightness` → **10**; **`CamOffset` 0 → 50**; `Core_Intensity` → **1**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Part_03`** |
| `M_VFX_DisAdd_Part04` | `Brightness` → **6**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; `Opacty_DepthFade` 20 → **30**; Main/Color/Dissolve → **`T_VFX_Part_04`** |
| `M_VFX_DisAdd_Rainbow` | `GradientMap_Displacement` 0.1 → **0.9**; `Gradient_Invert` 0.5 → **2**; `Opacity_Boldness` 0.5 → **1.5**; `Opacty_StepAdd` 0.1 → **0.3**; **`GradientMap_Tex` → `T_VFX_LUT_Rainbow_01`**; `GradientShape_Tex` → `T_VFX_Part_01`; `Main_Tex` → **`T_VFX_Ring_02`** |
| `M_VFX_DisAdd_Ring01` | `Brightness` → **10**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Ring_01`** |
| `M_VFX_DisAdd_Star01` | `Brightness` → **6**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; `Opacty_DepthFade` 20 → **10**; Main/Color/Dissolve → **`T_VFX_Star_01`** |
| `M_VFX_DisAdd_Star02` | `Brightness` → **6**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; `Opacty_DepthFade` 20 → **10**; Main/Color/Dissolve → **`T_VFX_Star_02`** |
| `M_VFX_DisAdd_Star03` | `Brightness` → **10**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; `Opacty_DepthFade` 20 → **10**; Main/Color/Dissolve → **`T_VFX_Star_03`** |
| `M_VFX_DisAdd_Wind01` | `Brightness` → **3**; `Core_Intensity` → **1**; **`Dissolve_Speed_Y` 0 → −0.15**; **`Distortion_Intensity` 0 → 0.5**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; Main/Color → **`T_VFX_Wind_01`**; `Dissolve_Tex` → **`T_VFX_Noise_02`** |
| `M_VFX_DisAdd_Wind02` | `Brightness` → **7**; **`Color_Speed_X` 0 → −0.3**; `Dissolve_Scale_X/Y` 1 → **0.7 / 0.95**; `Dissolve_Speed_X` 0 → **−0.1**; **`Distortion_Intensity` 0 → 1**; `Distortion_Scale_Y` 1 → **0.6**; `Distortion_Speed_X/Y` 0 → **0.1 / 0.1**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Wind_02`** |
| `M_VFX_DisAdd_LightStrip` | `Brightness` → **7**; **`Core_Power` 1 → 0**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_LightStrip_01`** |
| `M_VFX_DisAdd_Flames01` | `Brightness` → **10**; `Core_Intensity` → **1**; **`Dissolve` 0 → −0.1**; `Dissolve_Scale_X/Y` → **2 / 2**; **`Distortion_Intensity` 0 → 0.5**; `Distortion_Scale_X/Y` → **2 / 2**; `Distortion_Speed_X/Y` → **−0.3 / −0.3**; **`Glow_Intensity` 1 → 2**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; Main/Color → **`T_VFX_Wind_01`**; Dissolve/Distortion → **`T_VFX_Noise_04`**; `Color_Core` → **`RGBA(0.015996, 0.014444, 0.014444, 1)`** |
| `M_VFX_DisAdd_Smoke01` | `Brightness` → **10**; `Core_Intensity` → **1**; **`Dissolve` 0 → −0.1**; **`Distortion_Intensity` 0 → 0.4**; `Distortion_Speed_X/Y` → **0.1 / 0.1**; `GradientMap_Displacement` 0.1 → **0.75**; `Gradient_Invert` → **0**; **`Opacity_Boldness` 0.5 → 3**; `Color_Tex` → **`T_VFX_Cloud_04`**; `Main_Tex` → **`T_VFX_Cloud_05`**; `Dissolve_Tex` → **`T_VFX_Noise_07`**; `Distortion_Tex` → **`T_VFX_Noise_04`**; `Color_Core` → **`RGBA(0.001, 0.001, 0.001, 1)`** |
| `M_VFX_DisAdd_Flare01` | `Brightness` → **2**; **`Gradient_Invert` 0.5 → 0.847619**; `Opacity_Boldness` → **1**; `Main_Tex`/`Color_Tex` → **`T_VFX_Ring_02`**; `GradientShape_Tex` → **`T_VFX_Part_01`** |

**`Wind02` is the family's heaviest parameterization in the whole batch** — it turns on distortion
at intensity 1 with panning speeds, a non-unit dissolve scale, and a `Color_Speed_X` pan. Everything
it needs is already a declared CkUsf family parameter (NS_BasicAttack §9, params 10–15) **except**
`Color_Speed_X` (the family collapses Main/Color into one `ShapeTex` with one pan).

### 4.2 `M_VFX_FlatAdd` family — `M_VFX_DisAdd_Flat02` (`Spike01`)

`BLEND_Translucent`, `MSM_Unlit`, `twoSided: false`, outputs `EmissiveColor` + `Opacity`,
**no dynamic parameters, no texture parameters**; `Brightness` **10** (parent default 1),
`Opacty_DepthFade` 0, `CamOffset` 0, `Color_Core` `RGBA(1,1,1,0)`. Expressions: `Multiply ×2`,
`ScalarParameter ×3`, `VectorParameter ×1`, `ParticleColor ×1`, `DepthFade ×1`, `WorldPosition ×1`,
`MaterialFunctionCall ×1`. **`ParticleColor × 10`, depth-faded — the cheapest look in the batch.**

### 4.3 Textures referenced `[corpus]`

| Texture | Size | sRGB | Compression | Address | Format |
|---|---|---|---|---|---|
| `T_VFX_Part_01` | 512×512 | false | `TC_Alpha` | `TA_Clamp` | `TSF_G8` |
| `T_VFX_Part_02` | 512×512 | false | `TC_Alpha` | `TA_Clamp` | `TSF_G8` |
| `T_VFX_Part_03` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G8` |
| `T_VFX_Part_04` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` |
| `T_VFX_Ring_01` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` |
| `T_VFX_Ring_02` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` |
| `T_VFX_Star_01` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` |
| `T_VFX_Star_02` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` |
| `T_VFX_Star_03` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` |
| `T_VFX_Wind_01` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` |
| `T_VFX_Wind_02` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G8` |
| `T_VFX_LightStrip_01` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` |
| `T_VFX_Cloud_04` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` |
| `T_VFX_Cloud_05` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` |
| `T_VFX_Noise_02` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` |
| `T_VFX_Noise_04` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` |
| `T_VFX_Noise_07` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` |
| `T_VFX_LUT_Rainbow_01` | **512×2** | **true** | `TC_Default` | `TA_Wrap` | `TSF_BGRA8` |
| `T_VFX_WhitePixel` | 1×1 | true | `TC_Default` | `TA_Wrap` | `TSF_RGBA16` |

**Nineteen distinct textures — the largest dependency set in the batch.** Existing procedural
stand-ins usable without new work: `T_VFX_Part_01` → `SoftParticle`, `T_VFX_Part_04` →
`SparkStreak`, `T_VFX_Noise_02` → `TileNoise` (all measured in NS_BasicAttack §7). Measure before
assuming: `T_VFX_Wind_01`/`_02` against `WindBand` (which was parameterized off `T_VFX_Wind_03`, a
different asset), `T_VFX_Noise_04`/`_07` against `TileNoise`, `T_VFX_Ring_01` against the SDF `Ring`
bake, `T_VFX_LightStrip_01` against `Streak`, `T_VFX_Star_01…03` against `Flare` (the existing star
bake). **New bakes at minimum:** `T_VFX_Part_02`, `T_VFX_Part_03`, `T_VFX_Ring_02`, `T_VFX_Cloud_04`,
`T_VFX_Cloud_05`, plus a **new colour-LUT output kind** for `T_VFX_LUT_Rainbow_01` and a **2×2
sub-UV atlas** shape for the two flipbook emitters.

---

## 5. Per-emitter runtime facts and exact curves `[corpus]`

`t` = NormalizedAge over each emitter's own lifetime. `C` = constant key, `L` = linear key.
Where a curve's first key is at `t > 0`, Niagara clamps to that key's value for all earlier `t`.

Shared boilerplate unless contradicted: `Color Mode = Direct Set`, `Position Mode = Simulation
Position`, `Position Offset = (0,0,0)`, `Particle State → Kill Particles When Lifetime Has Elapsed
= true`, `Write Parameter Index 0 = true` (1–3 false), `Dyn Param 2/3/4 = 0`,
`ScaleColor.Scale Alpha = 1`, `ScaleColor.Scale RGB = (1,1,1)`,
`ScaleSpriteSize.Initial Sprite Size = (0,0)`, `SolveForcesAndVelocity.Acceleration Limit = 9999`,
`Speed Limit = 1000`, `VectorFromCurve.Scale Curve = (1,1,1)`, `FloatFromCurve.Scale Curve = 1`.

### 5.1 `Raimbow` — 1 sprite, spawn 0.5, lifetime 0.2, size 354.039

Init colour `RGBA(0.913099, 0.913099, 0.913099, 0.2)`; `Sprite Size Mode = Uniform`,
`Uniform Sprite Size = **354.039**`; `Sprite Rotation Mode = Random`, angle 0…360; dyn `Param 1 = 0.5`.

`Scale Color` (`RGBA Together`):

| Channel | Keys |
|---|---|
| Red / Green / Blue | `(0.328403, 0.5)L` *(single key → constant 0.5)* |
| Alpha | **`(0, 0)L (0.328403, 1)L (1, 0)L`** |

`Uniform Curve Sprite Scale`: `(0, 0.5)C (0.2, 0.9)C (1, 1)L`.
Update order: 1 Particle State, 2 Scale Color, 3 Dynamic Material Parameters, 4 Scale Sprite Size.

### 5.2 `Ring` — 1 sprite, spawn 0.5, lifetime 0.4, size 100

Init colour `RGBA(0.913099, 0.191202, 1, 0.608)`; module-level **`Color.Scale Alpha = 0.5`**;
`Uniform Sprite Size = 100` (`Min/Max 150/160` stored, unused); `Lifetime` Direct Set 0.4
(`Min/Max 0.3/0.7` stored, unused); `Sprite Rotation Mode = Random`, 0…360.

`Dynamic Material Parameters → Index 0 Param 1` (**`dissolve`**): **`(0, 0)C (1, -1)C`**.

`Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0, 1)C (0.118322, 1)L (0.295804, 1)L (0.542107, 0.391573)C (0.843948, 0.009134)L` |
| Green | `(0, 1)C (0.118322, 0.693872)L (0.295804, 0.040915)L (0.542107, 0.003677)C (0.843948, 0.004025)L` |
| Blue | `(0, 1)C (0.118322, 0.147027)L (0.295804, 0.045186)L (0.542107, 0.022174)C (0.843948, 0.006995)L` |
| Alpha | `(0, 1)C` |

`Uniform Curve Sprite Scale`: `(0, 0.5)C (0.1, 0.9)C (1, 1)C`.
Update order: 1 Particle State, 2 Dynamic Material Parameters, 3 Scale Sprite Size, 4 Color.

### 5.3 `Sparkles` — 10 sprites, spawn 0.55, WORLD

| Fact | Value |
|---|---|
| Lifetime | `[corpus-v3]` `Lifetime Mode = Random` ⇒ **`Lifetime Min/Max = 0.3 / 0.6` DRIVES** (`lifetimeResolved.source = minmax`); the `Random Range Float` override (0.2 / 0.4) sits on the unselected Direct-Set pin and is INERT. *Was read as "the pin override normally wins"; corrected per [P0-D2].* |
| Spawn shape | Sphere Location, `Sphere Radius = 20`, `Sphere Orientation Axis = (1,0,0)`, `Non Uniform Scale = (1,1,1)`, `Offset = (0,0,0)`, `Surface Only = false`, `Sphere Distribution = Random`, `Random Seed = 0` |
| Velocity | `Add Velocity` ← `Random Range Vector` **min `(500, −300, −300)`, max `(2500, 300, 300)`** — a strong **+X** cone |
| Size | `Sprite Size Mode = Random Uniform`, min **10**, max **20** |
| Sprite rotation | Random, 0…360 |
| Init colour | `RGBA(1, 1, 1, 1)`; `Color.Scale Alpha = 1` |
| Dyn `Param 1` | 1 |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`**.

`Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0.615756, 1)C` |
| Green | `(0.615756, 0.563224)C` |
| Blue | `(0.615756, 0.224)C` |
| Alpha | `(0.613341, 1)C (1, 0)L` |

`Uniform Curve Sprite Scale`: `(0, 0)C (0.1, 1)C (1, 0)C`.
Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Particle State, 4 Color,
5 Dynamic Material Parameters, 6 Scale Sprite Size.

### 5.4 `Sparkles_Stretched` — 3 velocity-aligned sprites, spawn 0.55, WORLD

| Fact | Value |
|---|---|
| Lifetime | `[corpus-v3]` `Lifetime Mode = Random` ⇒ **`Lifetime Min/Max = 0.1 / 0.2` DRIVES** (`lifetimeResolved.source = minmax`); the `Random Range Float` override (0.2 / 0.4) sits on the unselected Direct-Set pin and is INERT. *Was read as "the pin override normally wins"; corrected per [P0-D2].* |
| Spawn shape | Sphere Location, `Sphere Radius = 10`, `Surface Only = false`, `Random Seed = 0` |
| Velocity | `Add Velocity` ← `Random Range Vector` **min `(3000, −800, −800)`, max `(5000, 800, 800)`** — a very fast **+X** spray |
| Size | `Sprite Size Mode = Random Non-Uniform`, **min `(25, 70)`, max `(40, 60)`** *(note min.y > max.y; treat as per-axis lo/hi)* |
| Init colour | `RGBA(1, 1, 1, 1)`; module-level **`Color.Scale Alpha = 0.6`** |
| Dyn `Param 1` | 0 |
| Extra module | **`Scale Sprite Size by Speed`** — `Min Scale Factor = (1, 1)`, `Max Scale Factor = (1, 2)`, `Velocity Threshold = 1000`, `Scale Factor Curve = (0, 0)L (1, 1)L` |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (0.2, 0.25)C (1, -9.09372e-09)C`**.

`Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0, 1)C (0.088138, 1)L (0.615756, 1)L (0.98038, 0.391573)C` |
| Green | `(0, 1)C (0.088138, 0.671611)L (0.615756, 0.025)L (0.98038, 0.003677)C` |
| Blue | `(0, 1)C (0.088138, 0.085)L (0.615756, 0.0293413)L (0.98038, 0.022174)C` |
| Alpha | `(0, 1)C (1, 0)C` |

Two `Scale Sprite Size` modules:
- module 6: `Uniform: (0, 0)C (0.1, 1)C (1, 0)C`; `Non-Uniform: X: (0,0)L (1,1)L | Y: (0,0)L (1,1)L`
- module 7 (`001`): `Uniform: (0, 0)L (1, 1)L`; `Non-Uniform: X: (1, 1)L | Y: (0, 1)C (0.3, 0.25)C (1, 0.2)C`

Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Color, 4 Particle State,
5 Dynamic Material Parameters, 6 Scale Sprite Size, 7 Scale Sprite Size 001,
8 Scale Sprite Size by Speed.

### 5.5 `Flare_Stretched_01…04` — 4 sprites, all spawn 0, lifetime 0.5, all Non-Uniform

All four share one `Scale Color` curve, one uniform size curve, and the X of the non-uniform curve:

`Scale Color` (`RGBA Together`): `Red/Green/Blue: (0.218533, 1)L (1, 1)L`;
**`Alpha: (0.225777, 1)L (1, 0)L`**.
`Uniform Curve Sprite Scale`: `(0, 0.5)C (0.1, 1)L (1, 1)L`.
`Non-Uniform Curve Sprite Scale` **X (all four)**: `(0, 1)C (0.9, 2.30968e-08)C` — the streak's
length collapses to zero by t = 0.9.

| | `_01` | `_02` | `_03` | `_04` |
|---|---|---|---|---|
| `Sprite Size` | **`(600, 100)`** | **`(800, 200)`** | **`(1300, 200)`** | **`(700, 60)`** |
| Init colour | `RGBA(1, 0.111932, 0.0343398, 1)` | `RGBA(1, 0.571125, 0.0822827, 1)` | `RGBA(0.323143, 0.00560539, 0.00802319, 1)` | `RGBA(1, 0.854993, 0.508881, 1)` |
| Dyn `Param 1` | 1 | 1 | 1 | 1 |
| Non-Uniform **Y** | `(0, 0.3)C (0.2, 1)C` | `(0, 0.3)C (0.2, 1)C (0.9, 0.2)L` | `(0, 0.3)C (0.2, 1)C` | `(0, 0.3)C (0.2, 1)C (0.9, 0.0999999)L` |
| Material | `Part01` | `Part03_Bright` | `Part03_Bright` | `Star03` |
| `Uniform Sprite Size` (inert) | 550 | 550 | 550 | 550 |

Update order on all four: 1 Particle State, 2 Scale Color, 3 Dynamic Material Parameters,
4 Scale Sprite Size.

### 5.6 `Star_02` / `Star_01`

| | `Star_02` | `Star_01` |
|---|---|---|
| Count / spawn t | 1 / **0.5** | 1 / **0.55** |
| Lifetime | 0.2 | 0.3 |
| `Uniform Sprite Size` | **150** | **80** |
| Init colour | `RGBA(1, 0.184475, 0.386429, 0.4)` | identical |
| Sprite rotation | **`Direct Angle (Degrees)`, angle 0** | angle **0.1** |
| `Sprite Size` (inert) | `(10, 10)` | `(10, 10)` |
| Dyn `Param 1` | 0 | 0 |
| Material | `Star02` | `Star01` |
| `Uniform Curve Sprite Scale` | **`(0, 1)C (1, 0)C`** | **`(0, 1.36552e-08)C (0.1, 1)C (1, 0)C`** |

`Star_02` `Color` (`Color from Curve`) — over-1 RGB at spawn:

| Channel | Keys |
|---|---|
| Red | `(0, 2)C (0.220948, 1)L (0.775128, 1)L (1, 0.391573)C` |
| Green | `(0, 2)C (0.220948, 0.714319)L (0.775128, 0.040915)L (1, 0.003677)C` |
| Blue | `(0, 2)C (0.220948, 0.204)L (0.775128, 0.045186)L (1, 0.022174)C` |
| Alpha | `(0, 1)C` |

`Star_01` `Color`:

| Channel | Keys |
|---|---|
| Red | `(0, 2)C (0.234229, 1)L (0.775128, 1)L (1, 0.391573)C` |
| Green | `(0, 2)C (0.234229, 0.701399)L (0.775128, 0.0509999)L (1, 0.003677)C` |
| Blue | `(0, 2)C (0.234229, 0.168)L (0.775128, 0.0552256)L (1, 0.022174)C` |
| Alpha | `(0, 1)C` |

Update order — `Star_02`: 1 Particle State, 2 Dynamic Material Parameters, 3 Color,
4 Scale Sprite Size. `Star_01`: 1 Particle State, 2 Dynamic Material Parameters,
3 Scale Sprite Size, 4 Color.

### 5.7 The charge-up glows — `Glow_01`, `Glow_02`, `FirstGlow`, `SecondGlow` (all spawn 0, lifetime 0.5)

| | `Glow_01` | `Glow_02` | `FirstGlow` | `SecondGlow` |
|---|---|---|---|---|
| `Uniform Sprite Size` | **1000** | **300** | **1000** | **150** |
| Init colour | `RGBA(0.102242, 0.658375, 1, 0.2)` | same | same | same |
| Module alpha scale | **`Color.Scale Alpha = 0.3`** | **0.5** | **0.3** | **0.5** |
| Dyn `Param 1` | 1 | 1 | 1 | **2.69821** |
| Material | `Part01` | `Part01` | `Part01` | `Part03_Bright` |
| `Uniform Curve Sprite Scale` | `(0, 0.5)C (0.1, 0.9)C (1, 1)C` | same | same | **`(0, 0.5)C (1, 1)C`** |
| `Non-Uniform Curve Sprite Scale` | — | — | `X: (0,1)C (1,0.4)C \| Y: (0,1)C (1,0)C` | same as `FirstGlow` |

`Glow_01` `Color`:
`Red: (0.0881376, 1)L (0.377905, 1)L (0.9345, 0.391573)C |
Green: (0.0881376, 0.693872)L (0.377905, 0.0409152)L (0.9345, 0.00367651)C |
Blue: (0.0881376, 0.147027)L (0.377905, 0.0451862)L (0.9345, 0.0221739)C |
Alpha: (0, 1)C (1, 0)C`

`Glow_02` `Color`:
`Red: (0.214911, 1)L (0.817386, 1)L | Green: (0.214911, 0.693872)L (0.817386, 0.0409152)L |
Blue: (0.214911, 0.147027)L (0.817386, 0.0451862)L | Alpha: (0.406882, 1)C (1, 0)C`

`FirstGlow` `Color` — the effect's charge-up ramp, six keys per channel:
`Red: (0.0301841, 0.00518152)C (0.1147, 0.00913406)L (0.226985, 0.913099)L (0.618171, 1)L (0.792031, 1)L (1, 1)C |
Green: (0.0301841, 0.00518152)C (0.1147, 0.00402472)L (0.226985, 0.0241576)L (0.618171, 0.341915)L (0.792031, 0.752942)L (1, 0.938686)C |
Blue: (0.0301841, 0.00913406)C (0.1147, 0.00699541)L (0.226985, 0.0241576)L (0.618171, 0.109462)L (0.792031, 0.109462)L (1, 0.791298)C |
Alpha: (0, 0)C (0.154543, 1)C`

Read: near-black until t ≈ 0.115, then a fast rise to saturated red by t ≈ 0.227, warming through
orange to near-white by t = 1. **This is the "gathering energy" read.**

`SecondGlow` `Color`:
`Red: (0.618171, 1)L (0.963477, 1)L | Green: (0.618171, 0.341915)L (0.963477, 0.752942)L |
Blue: (0.618171, 0.109462)L (0.963477, 0.109462)L | Alpha: (0, 0)C (0.246302, 1)C`

Update order on all four: 1 Particle State, 2 Dynamic Material Parameters, 3 Scale Sprite Size,
4 Color.

### 5.8 The release flashes — `ShootFlash_01/02`, `FirstFlash`, `SecondFlash_01/02`

All five are camera-facing uniform sprites with no velocity and no size randomness.

| | `ShootFlash_01` | `ShootFlash_02` | `FirstFlash` | `SecondFlash_01` | `SecondFlash_02` |
|---|---|---|---|---|---|
| Count / spawn t | 1 / **0.5** | 1 / **0.5** | **4** / **0.54** | 1 / **0.55** | 1 / **0.5** |
| Lifetime | 0.1 | 0.1 | 0.1 | 0.2 | 0.1 |
| `Uniform Sprite Size` | **1000** | **300** | **400** | **900** | **70** |
| Init colour | `RGBA(0.102242, 0.658375, 1, 0.2)` | same | same | same | same |
| Module alpha scale | **0.3** | **0.7** | **0.6** | **0.3** | **1** |
| Dyn `Param 1` | 1 | 1 | **0.5** | 1 | 1 |
| Material | `Part01` | `Part01` | `Part03_Bright` | `Part01` | `Part01_Bright` |
| `Uniform Curve Sprite Scale` | `(0, 0.5)C (0.1, 0.9)C (1, 1)C` | same | **`(0, 0.5)C (0.1, 1)L (1, 1)L`** | `(0, 0.5)C (0.1, 0.9)C (1, 1)C` | `(0, 0.5)C (0.1, 0.9)C (1, 1)C` |

`ShootFlash_01` `Color`:
`Red: (0.0410504, 1)L (0.417748, 1)L (0.9345, 0.391573)C |
Green: (0.0410504, 0.693872)L (0.417748, 0.0409152)L (0.9345, 0.00367651)C |
Blue: (0.0410504, 0.147027)L (0.417748, 0.0451862)L (0.9345, 0.0221739)C | Alpha: (0, 1)C (1, 0)C`

`ShootFlash_02` `Color`:
`Red: (0.004829, 0.318547)L (0.10987, 1)C (0.214911, 1)L (0.817386, 1)L |
Green: (0.004829, 0.955974)L (0.10987, 1)C (0.214911, 0.693872)L (0.817386, 0.040915)L |
Blue: (0.004829, 1)L (0.10987, 1)C (0.214911, 0.147027)L (0.817386, 0.045186)L |
Alpha: (0.406882, 1)C (1, 0)C`

`FirstFlash` `Color`:
`Red: (0.0374283, 0.715694)C (0.16058, 1)L (0.312708, 1)L (0.659221, 0.913099)L (0.96227, 0.0137021)C |
Green: (0.0374283, 0.89627)C (0.16058, 0.752942)L (0.312708, 0.341915)L (0.659221, 0.0241576)L (0.96227, 0.00182116)C |
Blue: (0.0374283, 1)C (0.16058, 0.109462)L (0.312708, 0.109462)L (0.659221, 0.0241576)L (0.96227, 0.00802319)C |
Alpha: (0, 1)C`

`SecondFlash_01` `Color` — identical to `Glow_01`'s:
`Red: (0.0881376, 1)L (0.377905, 1)L (0.9345, 0.391573)C |
Green: (0.0881376, 0.693872)L (0.377905, 0.0409152)L (0.9345, 0.00367651)C |
Blue: (0.0881376, 0.147027)L (0.377905, 0.0451862)L (0.9345, 0.0221739)C | Alpha: (0, 1)C (1, 0)C`

`SecondFlash_02` `Color`:
`Red: (0, 1)C (0.118322, 1)L (0.295804, 1)L (0.542107, 0.391573)C (0.860851, 0.00913406)L |
Green: (0, 1)C (0.118322, 0.693872)L (0.295804, 0.103)L (0.542107, 0.0258438)C (0.860851, 0.00402472)L |
Blue: (0, 1)C (0.118322, 0.147027)L (0.295804, 0.106994)L (0.542107, 0.043284)C (0.860851, 0.00699541)L |
Alpha: (0, 1)C (0.305463, 1)L (0.993661, 0)L`

Update order on all five: 1 Particle State, 2 Dynamic Material Parameters, 3 Scale Sprite Size,
4 Color.

### 5.9 `Wind_01` — 1 tube mesh, spawn 0.55, lifetime **1.5 s**, LOCAL

| Fact | Value |
|---|---|
| Mesh scale | `Mesh Scale Mode = Uniform`, `Mesh Uniform Scale = 0.3` (renderer additionally applies **`scale: (1, 1, 5)`**) |
| Initial Mesh Orientation | `Orientation Axis = (1,0,0)`, `Orientation Vector = (1,0,0)`, **`Rotation = (0, 0.25, 0)`** (static, not random) |
| Velocity | `Add Velocity`, **`Velocity = (-200, 0, 0)`**, `Scale Added Velocity = (1,1,1)` |
| Init colour | `RGBA(1, 0.184475, 0.386429, 1)`; module-level **`Color.Scale Alpha = 0.7`** |
| `Uniform Sprite Size` | 0 (inert) |
| Dyn `Param 2/3/4` | 0 |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (0.2, 0.15)C (1, 0)C`**.

`Color` (`Color from Curve`) — **near-black RGB throughout**; the emitter reads as a dark
silhouette, not a glow:

| Channel | Keys |
|---|---|
| Red / Green / Blue | `(0, 0.00151763)C` *(single key → constant)* |
| Alpha | **`(0, 0)L (0.240266, 1)C (0.676124, 1)L (1, 0)L`** |

`Scale Mesh Size` → `Scale Factor` (`Scale Float by Curve`):
**`X: (0, 1.5)C (0.2, 2)C | Y: (0, 1.5)C (0.2, 2)C | Z: (0, 0.5)C (0.4, 3)C (0.7, 4.75)C (1, 5)C`**
— the tube widens 1.5→2× while **stretching 0.5→5× along Z**: a lengthening cone of wind.

`Dynamic Material Parameters → Index 0 Param 1` (**`dissolve`**): **`(0, -0.2)C (1, -1)C`**.

`Update Mesh Orientation`: `Rotation Rate = 0.3` (static, not a curve),
`Rotation Vector = (1, 0, 0)` — a slow constant spin about **local X**.

Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Particle State, 4 Color,
5 Scale Mesh Size, 6 Dynamic Material Parameters, 7 Update Mesh Orientation.

### 5.10 `Wind_02` — 5 sprites, spawn 0.55, lifetime **1.5 s**, LOCAL, **SubUV 2×2**

| Fact | Value |
|---|---|
| Size | `Sprite Size Mode = Random Uniform`, min **130**, max **230** |
| Sprite rotation | Random, 0…360 |
| Velocity | `Add Velocity` ← `Random Range Vector 001`, **min `(−100, −20, −20)`, max `(−700, 20, 20)`** — a **−X** drift |
| **Sub UV** | `Sub UVAnimation`: `Start Frame = 0`, `End Frame = 3`, `SubUV Loop Count = 1`; renderer `SubUV: 2x2` |
| Init colour | `RGBA(1, 0.184475, 0.386429, 1)`; `Color.Scale Alpha = 1` |
| Initial Mesh Orientation | `Rotation` ← `Random Range Vector` min `(0, 0.22, 0)` max `(0, 0.28, 0)` — **inert**: the renderer is an `Unaligned`/`FaceCamera` **sprite** and there is no `Align Sprite to Mesh Orientation` module |
| Mesh scale values | `Mesh Scale Min/Max`, `Mesh Uniform Scale 0.3` — all inert on a sprite renderer |
| Dyn `Param 2/3/4` | 0 |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (0.2, 0.25)C (1, 9.74764e-10)C`**.

`Color` (`Color from Curve`) — also near-black:

| Channel | Keys |
|---|---|
| Red | `(0, 0.0103298)C (0.75581, 0.00518152)L` |
| Green | `(0, 0.00273174)C (0.75581, 0.00518152)L` |
| Blue | `(0, 0.00242822)C (0.75581, 0.00913406)L` |
| Alpha | `(0.0205252, 1)L (1, 0)L` |

`Dynamic Material Parameters → Index 0 Param 1`: **`(0, -5.88215e-08)C (1, -1)C`**.
`Uniform Curve Sprite Scale`: `(0, 0.5)C (1, 1)C`.
Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Particle State, 4 Color,
5 Dynamic Material Parameters, 6 Scale Sprite Size.

### 5.11 `LightningStrip` — 1 velocity-aligned sprite, spawn 0.55, lifetime 0.2, LOCAL

**Note the renderer difference from the sibling systems**: here `LightningStrip` is a
`NiagaraSpriteRendererProperties` with `Alignment: VelocityAligned`, whereas in
[NS_FireBall_Hit.md](NS_FireBall_Hit.md) and [NS_Bomb_Explosion.md](NS_Bomb_Explosion.md) the
same-named emitter is a **mesh** renderer on `SM_VFX_Plane01`. Do not copy the translation between
sheets.

| Fact | Value |
|---|---|
| **Position offset** | **`(179.377, 0, 0)`** — the only non-zero position offset in this system; the strip spawns 179 units out along **+X** |
| Lifetime | Direct Set **0.2** (`Min/Max 0.1/0.2` stored, unused) |
| Size | `Sprite Size Mode = Non-Uniform`, **`Sprite Size = (50, 400)`** |
| Velocity | `Add Velocity`, **`Velocity = (10, 0, 0)`** — nominal, to give the velocity-aligned renderer an axis |
| Init colour | `RGBA(0.341915, 0.184475, 1, 1)`; module-level **`Color.Scale Alpha = 0.3`** |
| Mesh scale | `Mesh Scale` ← `Random Range Vector 001` min `(0.5, 0.5, 1)` max `(1.5, 1.5, 2)` — **inert** on a sprite renderer |
| Dyn `Param 1` | 0 |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`**.

`Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0.276487, 1)L` |
| Green | `(0.276487, 0.366253)L` |
| Blue | `(0.276487, 0.184475)L` |
| Alpha | `(0, 0)L (0.274072, 1)C (1, 0)C` |

`Scale Mesh Size` → `Scale Factor` (`Vector from Curve 001`) — **also inert on a sprite renderer**,
transcribed so it is not re-derived:
`X: (0, 0.5)C (0.2, 1)C (1, -1.44926e-08)C | Y: (0, -2.4747e-08)C (1, 1)C |
Z: (0, -5.68323e-08)C (0.2, 0.75)C (1, 1)C`.

Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Color, 4 Particle State,
5 Dynamic Material Parameters, 6 Scale Mesh Size *(inert)*.

### 5.12 `Spike01` — 3 pyramid meshes, spawn 0.55, LOCAL, Facing **Velocity**

| Fact | Value |
|---|---|
| Lifetime | `Random`, **min 0.1, max 0.15** (`InitializeParticle.Lifetime = 0.1` stored, unused) |
| Spawn shape | Sphere Location, `Sphere Radius = 10`, **`Non Uniform Scale = (0.1, 0.1, 0.1)`**, **`Hemisphere X = true`**, `Surface Only = false`, `Random Seed = 0` — effectively a 1-unit half-blob |
| Velocity | `Add Velocity` ← `Random Range Vector 001`, **min `(10, −10, −10)`, max `(50, 10, 10)`** |
| Mesh scale | `Mesh Scale Mode = Random Non-Uniform`, **min `(0.2, 0.2, 0.4)`, max `(0.2, 0.2, 0.7)`** |
| Initial Mesh Orientation | `Orientation Axis = (0, 0, 1)`, `Orientation Vector = (1, 0, 0)`; `Rotation` ← `Random Range Vector` **min `(0, 0, 1)`, max `(0, 0.5, −1)`** |
| Init colour | `RGBA(1, 0.184475, 0.386429, 1)`; `Color.Scale Alpha = 1` |
| Dynamic parameters | **none written** (consistent with `M_VFX_DisAdd_Flat02` reading none, §4.2) |
| `Uniform Sprite Size` | 500 (inert) |

`Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0, 1)C (0.118322, 1)L (0.568669, 1)L (0.703894, 0.391573)C (0.880169, 0.009134)L` |
| Green | `(0, 1)C (0.118322, 0.693872)L (0.568669, 0.040915)L (0.703894, 0.003677)C (0.880169, 0.004025)L` |
| Blue | `(0, 1)C (0.118322, 0.147027)L (0.568669, 0.045186)L (0.703894, 0.022174)C (0.880169, 0.006995)L` |
| Alpha | `(0, 1)C` |

`Scale Mesh Size` → `Scale Factor` (`Scale Float by Curve`):
**`X: (0, 0)C (0.2, 0.5)C (1, 4.17233e-08)C | Y: (0, 0)C (0.2, 0.4)C (1, 5.66244e-08)C |
Z: (0, 0)C (0.2, 1)C`** — X/Y grow then collapse while Z holds: the spikes flatten into slivers.

Update order: 1 Solve Forces and Velocity, 2 Particle State, 3 Color, 4 Scale Mesh Size.
`ScaleFloatByCurve.InitialValue = (1,1,1)`.

### 5.13 `Flames` — 4 sprites, spawn 0.5, WORLD, **SubUV 2×2**

| Fact | Value |
|---|---|
| Lifetime | `[corpus-v3]` `Lifetime Mode = Random` ⇒ **`Lifetime Min/Max = 0.2 / 0.7` DRIVES** (`lifetimeResolved.source = minmax`); the `Random Range Float` override (0.2 / 0.4) sits on the unselected Direct-Set pin and is INERT. *Was read as "the pin override normally wins"; corrected per [P0-D2].* |
| Spawn shape | Sphere Location, `Sphere Radius = 20`, `Surface Only = true` (`Surface Expansion Mode = Outside`, `Band Thickness = 0`), **`Hemisphere Z = true`**, `Radius Position = 1`, `U Position = 0`, `V Position = 0.5`, `Uniform Distribution = 1`, `Uniform Spiral Amount = 1` |
| Velocity | `Add Velocity` ← `Random Range Vector`, **min `(100, −100, −100)`, max `(700, 100, 100)`** — a **+X** cone |
| Size | `Random Uniform`, min **50**, max **100** |
| Sprite rotation | Random 0…360; **`Sprite Rotation Rate` ← `Random Range Float 001`, min −30, max 30** |
| **Sub UV** | `Start Frame = 0`, `End Frame = 3`, `SubUV Loop Count = 1`; renderer `SubUV: 2x2` |
| Init colour | `RGBA(1, 1, 1, 1)`; `Color.Scale Alpha = 1` |
| Dyn params | `Index 0 Param 2 = **5**`, `3/4 = 0`; **`Param3WriteEnabled = true`** |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (1, 0.2)C`**.

`Color` (`Color from Curve`) — the fire ramp, heavily over-1:

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

### 5.14 `Smokes` — 2 sprites, spawn 0.5, WORLD

| Fact | Value |
|---|---|
| Lifetime | `[corpus-v3]` `Lifetime Mode = Random` ⇒ **`Lifetime Min/Max = 0.7 / 1.3` DRIVES** (`lifetimeResolved.source = minmax`); the `Random Range Float` override (0.2 / 0.4) sits on the unselected Direct-Set pin and is INERT. *Was read as "the pin override normally wins"; corrected per [P0-D2].* |
| Spawn shape | Sphere Location, `Sphere Radius = 20`, **`Non Uniform Scale = (1, 1, 0)`** (a flattened XY disc), `Surface Only = true` / `Outside`, **`Hemisphere Z = true`** |
| Velocity | `Add Velocity` ← `Random Range Vector`, **min `(100, −100, −100)`, max `(500, 100, 100)`** |
| Size | `Random Uniform`, min **100**, max **200** |
| Sprite rotation | Random 0…360; `Sprite Rotation Rate` ← `Random Range Float 001`, min −30, max 30 |
| Init colour | `RGBA(1, 1, 1, 1)`; module-level **`Color.Scale Alpha = 0.3`** |
| Dyn `Param 2/3` | 0 / 0 |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (0.2, 0.3)C (1, 0.1)C`**.

`Color` (`Color from Curve`) — note the duplicated t = 0 keys, verbatim:

| Channel | Keys |
|---|---|
| Red | `(0, 0.009134)L (0, 1)C (0.0603682, 1)L (0.292182, 0.391573)C (0.527618, 0)L` |
| Green | `(0, 0.004025)L (0, 1)C (0.0603682, 0.693872)L (0.292182, 0.00367701)C (0.527618, 0)L` |
| Blue | `(0, 0.006995)L (0, 1)C (0.0603682, 0.147027)L (0.292182, 0.0221739)C (0.527618, 0)L` |
| Alpha | `(0.126773, 1)C (0.514337, 0.35)L` |

`Dynamic Material Parameters`:
- `Index 0 Param 1` (**`dissolve`**): `(0, -2.46502e-08)C (1, -1)C`
- `Index 0 Param 4` (**`core_color`**): **`(0, -1)C (0.4, 1)C`**

`Uniform Curve Sprite Scale`: `(0, 0.5)C (0.2, 0.9)C (1, 1)C`.
Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Particle State, 4 Color,
5 Dynamic Material Parameters, 6 Scale Sprite Size, 7 Sprite Rotation Rate.

### 5.15 `Flare01` — 1 sprite, spawn 0.5, lifetime 0.1, size 100

Init colour `RGBA(0.913099, 0.0193824, 0.130136, 0.4)`; module-level **`Color.Scale Alpha = 0.6`**;
`Sprite Rotation Mode = Random`, 0…360; `Dyn Param 2/3/4 = 0`.

`Dynamic Material Parameters → Index 0 Param 1` (**`dissolve`**): **`(0, 1)C (1, -1)C`**.

`Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0, 1)C (0.142469, 1)L (0.47208, 1)L (0.9345, 0.391573)C` |
| Green | `(0, 1)C (0.142469, 0.693872)L (0.47208, 0.040915)L (0.9345, 0.003677)C` |
| Blue | `(0, 1)C (0.142469, 0.147027)L (0.47208, 0.045186)L (0.9345, 0.022174)C` |
| Alpha | `(0, 0)C (0.214911, 1)L (1, 0)C` |

`Uniform Curve Sprite Scale`: `(0, 0.5)C (0.1, 1)L (1, 1)L`.
Update order: 1 Particle State, 2 Dynamic Material Parameters, 3 Scale Sprite Size, 4 Color.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new row in `ck::particles::Get_TemplateSpecs()` is required.**

| Field | Value | Why |
|---|---|---|
| Loop duration | **2.0 s** `[corpus-v3]` | the system's `Once` loop duration ([P0-D3]); the three per-emitter values (1.0 / 5.0 / 0.3) are all leftovers. *Was `[unresolved]`.* |
| Particle lifetime | **1.5 s** by the [P0-D3] formula — **but see the STOP below** | max resolved emitter lifetime (`Wind_01`/`Wind_02`); `Smokes` resolves to 0.7–1.3 (§5.14) |
| Burst count | **50** | the §2 total |

**`[P0-D3 STOP: loop = 2.0 s (system, Once); lifetime = 1.5 s (max resolved, Wind_01/Wind_02);
burst = 50 (§2) — but the longest layer is alive until SpawnDelay + Lifetime = 0.55 + 1.5 =
2.05 s, which exceeds BOTH the formula's 1.5 s template lifetime AND the 2.0 s system loop]`.**
A template particle must outlive `SpawnDelay + LayerLifetime`, so the row needs ≥ 2.05 s, which the
[P0-D3] formula does not produce. The source itself has a layer outliving its own `Once` loop
(`Inactive Response = Complete` lets it finish). Orchestrator ruling required — do not improvise the
lifetime. **Do not set the template lifetime from the longest emitter lifetime alone — add the spawn
delay.** (`NS_FireBall_Hit` shares the two `Wind_*` emitters but has them DISABLED, so it does not
hit this overshoot.)

Layer partition `Seed % 50` with the layer→emitter map from §2. The 0 / 0.5 / 0.54 / 0.55 spawn
beats are reproduced by hiding a layer for `Age < SpawnDelay` and running its curves on
`(Age − SpawnDelay) / Lifetime`, per NS_BasicAttack §5.

### 6.2 VisTag / renderer needs

| Source emitters | Renderer needed | Available today? |
|---|---|---|
| 21 camera-facing sprites across **11 distinct materials** | camera-facing sprite, per-look | **NO — gap 1** |
| `Sparkles_Stretched`, `LightningStrip` | velocity-aligned sprite, 2 materials | **YES** — row-declared `VelocityAlignedSprite` ×2 (the NS_BasicAttack capability) |
| `Spike01` | mesh, Facing **Velocity** | **partial — gap 3**; Default-facing mesh row renderer exists, velocity facing does not |
| `Wind_01` | mesh, Facing Default, **with a renderer-level non-uniform scale `(1, 1, 5)`** | **partial — gap 4**; the row spec carries no renderer scale |
| `Flames`, `Wind_02` | camera-facing sprite **with SubUV 2×2** | **NO — gap 2** |

Eleven distinct sprite materials is the headline number. Consolidation by material:
`Part01` (7 emitters), `Part03_Bright` (4), `Part01_Bright` (2), and one each of `Rainbow`,
`Ring01`, `Part04`, `Star01`, `Star02`, `Star03`, `Wind01`, `Flames01`, `Smoke01`, `Flare01`,
`LightStrip` — **14 sprite looks + 2 mesh looks**.

### 6.3 Mesh needs

Both meshes are procedurally reproducible (§3):

| Generated mesh (proposed) | Reproduces | Build |
|---|---|---|
| `Spike` | `SM_VFX_Spike01` | square pyramid, base ±100 XY at Z = 0, apex `(0,0,200)`, 6 tris; **v = 0 tip → 1 base** |
| `TubeBand` | `SM_VFX_Ring01` | open double-walled tube, radii **99.5 / 100.0**, `Z 0…50`, 64 segments; **v = 0 TOP → 1 BOTTOM**; `u = 0.75 + angle/360` mod 1, seam at ±180° |

**Check the existing `Tube` carrier's UV convention** in `CkParticles_MeshGenerator.cpp` before
adding `TubeBand` — it may already be the same shape with a different UV, in which case the honest
fix is a UV variant, not a second mesh. Same `Spike` mesh as
[NS_Bomb_Explosion.md](NS_Bomb_Explosion.md) §6.3 — **build it once.**

### 6.4 Look / texture needs

Sixteen looks, **all parameterizations of the existing `CkUsf_Look_DissolveAdd`** except the spike's
tiny `M_VFX_FlatAdd` shader (shared with NS_Bomb_Explosion). CkUsf family parameters **not plumbed
today** that this effect needs:

| Parameter | Used by | Notes |
|---|---|---|
| `CamOffset` (50) | `Part03_Bright` (4 emitters) | camera-toward world offset |
| `Core_Intensity` (1) | `Part01_Bright`, `Part03_Bright`, `Wind01`, `Flames01`, `Smoke01` | |
| `Core_Power` (0) | `LightStrip` | disables the core power curve |
| `Glow_Intensity` (2) | `Flames01` | |
| `Gradient_Invert` (0.847619 / 2 / 0) | `Flare01`, `Rainbow`, others | |
| **gradient-map LUT chain** | `Rainbow` | a real 512×2 ramp, not the white pixel |
| separate `GradientShape_Tex` | `Rainbow`, `Flare01` | |
| separate `Main_Tex` vs `Color_Tex` | `Smoke01` | family collapses both into `ShapeTex` |
| `Color_Speed_X` (−0.3) | `Wind02` | a pan on the colour sampler only |

Textures: 19 distinct (§4.3). Reuse `SoftParticle` / `SparkStreak` / `TileNoise`; **measure** the
seven "candidate" mappings listed there before assuming; **new bakes** for at least five, plus a
colour-LUT kind and a 2×2 sub-UV atlas shape.

### 6.5 Capability gap callout

| # | Gap | Severity |
|---|---|---|
| 1 | **No row-declared camera-facing sprite kind.** 21 of 26 emitters are `Unaligned`/`FaceCamera` across **11 materials**; one `User.SpriteMaterial` binding cannot carry them. Additive fix to `ECk_ParticlesRenderer_Kind`; shared with every sheet in this batch. | **BLOCKING** |
| 2 | **No sub-UV / flipbook support anywhere in CkParticles.** `Flames` and `Wind_02` render `SubUV: 2x2` driven by `Sub UVAnimation` (frames 0–3, 1 loop). The DI writes no sub-image index, the builder sets no `SubImageSize`, the generator bakes no atlases. Without it both read as static sprites. | **BLOCKING** for 2 emitters |
| 3 | **Mesh renderer facing mode is not expressible** on a row-declared `Mesh` renderer (`Kind`/`VisTag`/`MeshName`/`LookName` only). `Spike01` needs `Facing: Velocity`. **Workaroundable** — the behavior can write an `Orientation` quat from its own velocity, since it owns the velocity. | Medium |
| 4 | **Renderer-level mesh scale is not expressible.** `Wind_01`'s renderer carries `scale: (1, 1, 5)` on top of the particle's `Mesh Uniform Scale` and its `Scale Mesh Size` curve. **Workaroundable** — fold the 5× into the behavior's `O.Scale.z` or bake it into the generated mesh; but it must not be forgotten, or the wind tube is 1/5 its length. | Low (but easy to miss) |
| 5 | **System-level loop parameters absent from the corpus** (§2). Cadence cannot be finalized. | **Prerequisite** |
| 6 | **Template lifetime must cover `SpawnDelay + LayerLifetime` = 2.05 s** (§6.1), not just the longest emitter lifetime. | Design note |
| 7 | **World space on 21 of 26 emitters**; the template is local-space (NS_BasicAttack §13.2). A cast effect is usually anchored to a caster who may be moving during the 2 s window. Medium risk, unlike the fixed-point explosions. | Medium |
| 8 | **Nine unplumbed CkUsf family parameters** (§6.4), including the gradient-map LUT chain and a separate `GradientShape_Tex`. | Medium |
| 9 | **A colour-LUT texture output kind** and a **2×2 sub-UV atlas shape** are both new *kinds* of bake, not new functions (§4.3). | Medium |
| 10 | ~~Four `[unresolved]` lifetime ranges~~ — **RESOLVED `[corpus-v3]`** (§5.3, §5.4, §5.13, §5.14): `Lifetime Mode = Random` ⇒ the module's `Lifetime Min/Max` drives on all four ([P0-D2]); `Sparkles` 0.3–0.6, `Sparkles_Stretched` 0.1–0.2, `Flames` 0.2–0.7, `Smokes` 0.7–1.3. | Closed |
| 11 | **Inert modules that look load-bearing.** `Wind_02`'s `Initial Mesh Orientation` and `LightningStrip`'s `Mesh Scale` / `Scale Mesh Size` write mesh attributes on **sprite** renderers. Do not implement them. Recorded in §5.10 / §5.11. | None (trap) |
| 12 | **`Opacty_DepthFade`** 20 / 30 / 10 / 0 across instances; CkUsf surface looks do not wire scene depth (known gap, NS_Lightning_Range §13.4). | Low (known) |

**Complexity tier: L.** 26 emitters, 11 sprite materials, 19 textures, two flipbook emitters, two
new mesh carriers, nine unplumbed shader parameters, and a two-act cadence whose loop duration is
not in the corpus. **A clean S/M-tier subset exists**: the 8 charge-up emitters (`Glow_01/02`,
`FirstGlow`, `SecondGlow`, `Flare_Stretched_01…04`) are all plain camera-facing sprites on three
materials with no randomness, no velocity and no sub-UV — that half needs **only gap 1** and is the
right first delivery. The release act needs gaps 2, 3, 4 and most of the texture work.

### 6.6 Behavior id

**Do not allocate an id in this document.** Take the next free id from
`ck::particles::NumBehaviors` at implementation time, bump it, and update the roster paragraph in
`CkParticles/CLAUDE.md`. Five sibling sheets were written in the same pass and none allocates an id
— allocate them in one ordered pass so they cannot collide.

---

## 7+. Reserved for implementation

Sections 7–14 of the recipe schema ([README.md](README.md)) are intentionally absent and are to be
written by the implementation session, from what actually happened.
