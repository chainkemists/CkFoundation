# Recipe: NS_Bomb_Explosion → CkParticles (PRE-IMPLEMENTATION TRANSLATION SHEET)

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
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Bomb_Explosion` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior id | **not allocated** — take the next free id at implementation time |
| Recreation status | not started |

Corpus evidence (all `[corpus]`, exported 2026-08-01):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Bomb_Explosion.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part02,Part03,Part04,Ring01,Impact01,Flare01,LightStrip,Flat02,BubbleNoise_01,BubbleOut_01}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/MI_VFX_FresnelBomb_{FirstExplo01,FirstExplo02,BubbleNoise02}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_FlatAdd.json`
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_{Spike01,Plane01,Sphere01,Sphere02,Ring03}.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/*.json`

**The source Niagara asset was not opened for editing.**

> ### A same-named sibling exists — take the right one
> `[corpus]` `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Bomb_Explosion` is a **different,
> parameterized** system. **Fastest discriminator: it declares 8+ user parameters**
> (`User.Bubble First Explo Color 01`, `User.Bubble Fresnel Color 01`, `User.Bubble Noise Color
> 01/02`, `User.Bubble Out Color 01`, `User.Flare Color 01…03`) and renders through `MI_VFX_*`
> instances (`MI_VFX_Bubble_Noise_01`, `MI_VFX_Fresnel_Bomb_First_Explo_01/02`,
> `MI_VFX_Fresnel_Bomb_Bubble_Noise_02`, `MI_VFX_Glow_01…04`, …). The `Anime_VFX/Shared/Skills`
> variant documented here has an **empty user-parameter list** and renders through `M_VFX_DisAdd_*`
> plus `MI_VFX_FresnelBomb_*`.

---

## 2. System anatomy `[corpus]`

**23 CPU emitters, all enabled, all `Spawn Burst Instantaneous` (no continuous rate anywhere).
162 particles per loop. 14 sprite renderers + 9 mesh renderers. This is the largest effect in the
batch by particle count and by renderer variety.**

| # | Emitter | Space | Count | Spawn t | Lifetime | Renderer | Mesh | Material |
|---|---|---|---|---|---|---|---|---|
| 0 | `Glow_01` | World | 1 | 0 | 0.2 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 1 | `Glow_02` | World | 1 | 0 | 0.2 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 2 | `Glow_03` | World | **2** | 0.05 | 0.3 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 3 | `Ring01` | World | 1 | 0 | **0.1** | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Ring01` |
| 4 | `Ground_Glow_01` | World | 1 | 0 | 0.2 | Sprite, **CustomAlignment / CustomFacingVector** | — | `M_VFX_DisAdd_Part01` |
| 5 | `Spike01` | World | **10** | 0 | 0.15 | **Mesh**, Facing **Default** | `SM_VFX_Spike01` | `M_VFX_DisAdd_Flat02` |
| 6 | `Spike02` | World | **10** | 0 | 0.15 | **Mesh**, Facing **Default** | `SM_VFX_Spike01` | `M_VFX_DisAdd_Flat02` |
| 7 | `Spike03` | World | **10** | 0 | 0.15 | **Mesh**, Facing **Default** | `SM_VFX_Spike01` | `M_VFX_DisAdd_Flat02` |
| 8 | `Glow_04` | World | **2** | 0.05 | 0.3 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part01` |
| 9 | `Glow_05` | World | 1 | 0.05 | **0.1** | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part03` |
| 10 | `Ground_Glow_02` | World | 1 | 0.1 | 0.2 | Sprite, **CustomAlignment / CustomFacingVector** | — | `M_VFX_DisAdd_Part01` |
| 11 | `FlareImpact` | **Local** | 1 | 0 | **0.05** | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Impact01` |
| 12 | `LightningStrip` | **Local** | **5** | 0 | rand 0.1–0.15 | **Mesh**, Facing **Velocity** | `SM_VFX_Plane01` | `M_VFX_DisAdd_LightStrip` |
| 13 | `Sparkles_01` | World | **50** | 0.05 | rand | Sprite, **VelocityAligned**/FaceCamera | — | `M_VFX_DisAdd_Part04` |
| 14 | `Sparkles_02` | World | **50** | 0.05 | rand | Sprite, **VelocityAligned**/FaceCamera | — | `M_VFX_DisAdd_Part04` |
| 15 | `Flare_01` | **Local** | **5** | 0.1 | 0.2 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part02` |
| 16 | `Flare_02` | **Local** | **5** | 0.1 | 0.2 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Part02` |
| 17 | `Flare03` | World | 1 | 0.1 | 0.2 | Sprite, Unaligned/FaceCamera | — | `M_VFX_DisAdd_Flare01` |
| 18 | `Bubble_Noise01` | World | 1 | 0.1 | 0.2 | **Mesh**, Facing **CameraPosition** | `SM_VFX_Sphere02` | `M_VFX_DisAdd_BubbleNoise_01` |
| 19 | `Bubble_First_Explo` | World | 1 | 0 | 0.15 | **Mesh**, Facing **CameraPosition** | `SM_VFX_Sphere01` | `MI_VFX_FresnelBomb_FirstExplo01` |
| 20 | `Bubble_Out` | World | 1 | 0.1 | 0.2 | **Mesh**, Facing **CameraPosition** | `SM_VFX_Ring03` | `M_VFX_DisAdd_BubbleOut_01` |
| 21 | `Bubble_Noise02` | World | 1 | 0.1 | 0.3 | **Mesh**, Facing **CameraPosition** | `SM_VFX_Sphere02` | `MI_VFX_FresnelBomb_FirstExplo02` |
| 22 | `Bubble_Fresnel` | World | 1 | 0.1 | 0.3 | **Mesh**, Facing **CameraPosition** | `SM_VFX_Sphere01` | `MI_VFX_FresnelBomb_BubbleNoise02` |

**Total per loop: 162 particles** (1+1+2+1+1+10+10+10+2+1+1+1+5+50+50+5+5+1+1+1+1+1+1).

Per-emitter `Emitter State` on all 23: `Loop Behavior = Infinite`, `Loop Duration Mode = Fixed`,
`Loop Duration = 1`, `UseLoopDelay = false`, `UseLoopCountLimit = false` (`Loop Count Limit = 1`
inert). `Bounds: Dynamic`, `Determinism: false`. **Every emitter's `Position Offset` is `(0,0,0)`**
— all spatial spread comes from Sphere Location modules and velocity.

> **`Life Cycle Mode = System` on all 23 emitters `[corpus]`.** The stored Loop Behavior / Loop
> Duration are **inert leftovers**; the system drives the cadence.
>
> **System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
> `UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
> Per [P0-D1] this is the authority — one 162-particle burst over a single 2.0 s cycle, all of it
> over by t ≈ 0.5 s. *(Was `[unresolved]`.)* Spawn **times** and burst **counts** are live.

**Three spawn beats**: t = 0 (Glow_01/02, Ring01, Ground_Glow_01, all 30 spikes, FlareImpact,
LightningStrip, Bubble_First_Explo = 45 particles), t = 0.05 (Glow_03/04/05, both 50-sparkle
bursts = 105), t = 0.1 (Ground_Glow_02, Flare_01/02/03, four Bubbles = 16). **The longest layer
lives 0.3 s**; the whole effect is over inside ~0.4 s of the loop start.

---

## 3. Mesh geometry `[corpus]`

Five distinct meshes. All five report section 0's material as `M_VFX_DisAdd_Slash01` in the mesh
asset — that is the mesh's *default* slot material and is **overridden by every emitter's renderer**
(§2). Do not chase it.

### 3.1 `SM_VFX_Spike01` — 16 verts / 6 tris — used by `Spike01/02/03` (30 particles)

Exact geometry (small enough to transcribe in full) `[corpus, from the .obj]`:

- Bounds `X −100…100`, `Y −100…100`, `Z 0…200`. `uv0` covers `0…1` in both axes.
- Verts (4 distinct positions): apex `(0, 0, 200)`; base square
  `(100, 100, 0)`, `(−100, 100, 0)`, `(−100, −100, 0)`, `(100, −100, 0)`.
- **A square pyramid**: 4 side triangles + 2 base triangles = 6 tris. Radius-from-origin
  min 141.421 (base corners), max 200 (apex).
- **UVs**: `corr(v, Z) = −1.000` exactly, `corr(v, radiusXY) = 1.000`; `corr(u, Y) = −1.000`.
  Every base vertex has `v = 1`, the apex has `v = 0`; `u` is 0 on the +Y side and 1 on the −Y side,
  apex `u = 0.5`. So **v runs tip (0) → base (1)** and u runs across the spike. UV list verbatim:
  `(0,1) (0.5,0) (0,1) (0,1) (0.5,0) (1,1) (1,1) (0.5,0) (0,1) (1,1) (0.5,0) (1,1) (0,1) (0,1)
  (1,1) (1,1)`.
- **Trivially procedural.** A `Spike` carrier is 6 triangles of generator code.

### 3.2 `SM_VFX_Plane01` — 8 verts / 4 tris — used by `LightningStrip`

- Bounds `X −100…100`, `Y −0.0643…0`, `Z −0…200`. `uv0` covers `0…1`.
- **Two coincident double-sided quads** in the XZ plane, offset 0.0643 in Y (a paper-thin card).
  Verts: `(±100, 0, 0)`, `(±100, 0, 200)` and the same four at `Y = −0.0643`.
- **UVs**: `corr(u, X) = 1.000`, `corr(v, Z) = −1.000`. u runs −X→+X, **v runs top (0) → bottom (1)**.
- **Trivially procedural.** The 0.0643 Y offset is 0.03 % of the 200-unit span; collapse it to a
  single sheet and make the look `_TwoSided`, exactly as NS_BasicAttack §13.5 did for the crescent.

### 3.3 `SM_VFX_Ring03` — 198 verts / 256 tris — used by `Bubble_Out`

- Bounds `X −100…100`, `Y −100…100`, `Z −0.5…0`. A **flat annulus** (thickness 0.5 on a 200-unit
  span = 0.25 %).
- **Three distinct radii: 53.96, 76.98, 100.00** — two radial bands, 64 segments around
  (angles step 11.25°).
- **UVs**: `corr(v, radiusXY) = 1.000` exactly — `v = 0` at r = 53.96, `v = 0.5` at r = 76.98,
  `v = 1` at r = 100.00. **v runs INNER (0) → OUTER (1)** — the opposite of NS_BasicAttack's
  crescent, and this sign is load-bearing. `u` wraps 0…1 around the ring: `u = 0.25` at −180°,
  `u = 0.75` at 0°, `u = 0.2812` at +168.75° — i.e. **u increases clockwise from the +X axis with
  the seam at ±180°**, `u = 0.75 + angle/360` mod 1.
- **Procedural.** A 2-band, 64-segment flat annulus with that exact UV convention.

### 3.4 `SM_VFX_Sphere01` and `SM_VFX_Sphere02` — 559 verts / 960 tris each

**Both have byte-identical measured geometry** — same vert/tri counts, same bounds, same UV
statistics. `[inferred: they are duplicate assets differing only in name and default material;
confirm with a hash before deduplicating in the recreation.]`

- Radius-from-origin: **constant 100.000** (min = max = mean). A true unit-radius UV sphere ×100.
- `radiusXY` per Z-octile: `0:{0.0..38.3} 1:{55.6..70.7} 2:{83.1..92.4} 3:{98.1..98.1}
  4:{100.0..100.0} 5:{98.1..98.1} 6:{83.1..92.4} 7:{55.6..70.7} 8:{0.0..38.3}` — 8 latitude bands.
- **UVs are a standard spherical projection**: `corr(v, Z) = −0.991` (v = 0 at +Z pole, v = 1 at
  −Z pole, 9 discrete v levels `0.000 / 0.188 / 0.250 / 0.375 / 0.437 / 0.562 / 0.625 / 0.750 /
  0.812 / 1.000`); `corr(u, angleXY) = −0.742` (the imperfect correlation is the 0/1 seam wrap, not
  a mapping irregularity).
- **Procedural.** A 32×16 UV sphere with a Z-pole v-axis reproduces this exactly.

**All five meshes in this effect are procedurally reproducible.** Unlike the Bomb sheets, there is
no hand-authored atlas here — this is the batch's *easy* mesh case.

---

## 4. Material families and per-instance deltas `[corpus]`

**Three parent graphs.** `M_VFX_DissolveAdd` (9 instances), `M_VFX_FlatAdd` (1), and
`M_VFX_FresnelBomb` (3) — the last is **new to the cookbook**.

### 4.1 `M_VFX_DissolveAdd` family

Instances used: `Part01`, `Part02`, `Part03`, `Part04`, `Ring01`, `Impact01`, `Flare01`,
`BubbleNoise_01`, `BubbleOut_01`. Identical base properties on all nine: `MD_Surface`,
**`BLEND_Translucent`**, **`MSM_Unlit`**, `twoSided: false`, outputs **`EmissiveColor` + `Opacity`**,
dynamic-parameter channels **`[dissolve, distortion, offset, core_color]`**, and the family
expression histogram (`ScalarParameter ×41`, `Multiply/Add/AppendVector ×18` each, `Saturate ×12`,
`DynamicParameter ×8`, `Reroute ×8`, `TextureSampleParameter2D ×6`, `Panner ×5`, `Constant ×5`,
`TextureCoordinate ×5`, `LinearInterpolate ×4`, `Clamp ×2`, `OneMinus ×2`, one each of `DepthFade`,
`ParticleColor`, `Power`, `SmoothStep`, `StaticBoolParameter`, `StaticSwitch`, `VectorParameter`,
`MaterialFunctionCall`, `WorldPosition`).

Reference = `M_VFX_DisAdd_Part01` (values quoted in [NS_Bomb_Spawn.md](NS_Bomb_Spawn.md) §4.1 and
[NS_Bomb_Projectile.md](NS_Bomb_Projectile.md) §4.1; repeated in short form): all textures
`T_VFX_Part_01` except `Distortion_Tex`/`GradientShape_Tex` = `T_VFX_Noise_02` and `GradientMap_Tex`
= `T_VFX_WhitePixel`; `Brightness` 1; `Opacity_Boldness` 0.5; `Distortion_Intensity` 0; `Dissolve` 0;
scales 1, speeds/offsets 0; `Gradient_Invert` 0.5; `GradientMap_Displacement` 0.10000000149011612;
`Color_CoreDifferent` 0; `Core_Power` 1; `Core_Intensity` 0; `Glow_Intensity` 1; `Opacty_Step` 0;
`Opacty_StepAdd` 0.10000000149011612; `Opacty_DepthFade` 20; `CamOffset` 0;
`Color_Core` `RGBA(1, 1, 1, 0)`.

| Instance | Delta vs `Part01` |
|---|---|
| `M_VFX_DisAdd_Part01` | *(reference)* — `Glow_01/02/03/04`, `Ground_Glow_01/02` |
| `M_VFX_DisAdd_Part02` | `Glow_Intensity` 1 → **0.3**; Main/Color/Dissolve → **`T_VFX_Part_02`** |
| `M_VFX_DisAdd_Part03` | `Brightness` → **3**; `Core_Intensity` → **1**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Part_03`** |
| `M_VFX_DisAdd_Part04` | `Brightness` → **6**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; `Opacty_DepthFade` 20 → **30**; Main/Color/Dissolve → **`T_VFX_Part_04`** |
| `M_VFX_DisAdd_Ring01` | `Brightness` → **10**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Ring_01`** |
| `M_VFX_DisAdd_Impact01` | `Brightness` → **12**; **`Core_Power` 1 → 0**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Impact_01`** |
| `M_VFX_DisAdd_Flare01` | `Brightness` → **2**; **`Gradient_Invert` 0.5 → 0.847619**; `Opacity_Boldness` → **1**; `Main_Tex`/`Color_Tex` → **`T_VFX_Ring_02`**; `GradientShape_Tex` → **`T_VFX_Part_01`** (`Dissolve_Tex` stays `T_VFX_Part_01`) |
| `M_VFX_DisAdd_BubbleNoise_01` | `Brightness` → **10**; **`Core_Intensity` 0 → 20**; `Dissolve_Speed_Y` 0 → **−0.1**; `Opacity_Boldness` → **1**; Main/Color → **`T_VFX_Gradient_03`**; `Dissolve_Tex` → **`T_VFX_Noise_05`** |
| `M_VFX_DisAdd_BubbleOut_01` | `Brightness` → **4**; **`Color_CoreDifferent` 0 → 1**; **`Core_Intensity` 0 → 5**; `Dissolve_Scale_X/Y` 1 → **0.3 / 0.5**; `Dissolve_Speed_Y` 0 → **1**; `Opacity_Boldness` → **1**; Main/Color → **`T_VFX_Gradient_03`**; `Dissolve_Tex` → **`T_VFX_Noise_06`**; `Color_Core` → **`RGBA(0.226966, 0.520996, 1, 1)`** |

**`Core_Intensity = 20`** on `BubbleNoise_01` is the largest value of any parameter in this batch,
and `Core_Power = 0` on `Impact01` disables the core power curve outright. Both live in a chain
(`Color_Core` / `Color_CoreDifferent` / `Core_Power` / `Core_Intensity`) that the CkUsf DissolveAdd
family currently exposes only as `CoreColor` — see §6.4.

### 4.2 `M_VFX_FlatAdd` family — `M_VFX_DisAdd_Flat02` (the 30 spikes)

| Fact | Value |
|---|---|
| Parent | `Parents/M_VFX_FlatAdd` |
| Blend / shading | `BLEND_Translucent`, `MSM_Unlit`, `twoSided: false`, `MD_Surface` |
| Outputs | `EmissiveColor` + `Opacity` |
| Dynamic parameters | **none** |
| Texture parameters | **none** |
| Expressions | `Multiply ×2`, `ScalarParameter ×3`, `VectorParameter ×1`, `ParticleColor ×1`, `DepthFade ×1`, `WorldPosition ×1`, `MaterialFunctionCall ×1` |
| `Brightness` | **10** (parent default 1) |
| `Opacty_DepthFade` / `CamOffset` | 0 / 0 |
| `Color_Core` | `RGBA(1, 1, 1, 0)` |

Untextured `ParticleColor × 10`, depth-faded. **This is the cheapest look in the whole batch** and
it draws 30 of the 162 particles. Note the spike emitters write **no dynamic parameters at all**
(§5.4) — consistent with a material that reads none.

### 4.3 `M_VFX_FresnelBomb` family — NEW to the cookbook, 3 instances

All three are instances of `Parents/M_VFX_FresnelBomb`. Base properties: `MD_Surface`,
`BLEND_Translucent`, `MSM_Unlit`, `twoSided: false`, outputs `EmissiveColor` + `Opacity`, dynamic
parameters **`[dissolve, distortion, offset, core_color]`** (same channel names as DissolveAdd).

**Expression histogram — a genuinely different graph** (`MI_VFX_FresnelBomb_FirstExplo01`):
`ScalarParameter ×17`, `Saturate ×8`, `Add ×7`, `Multiply ×5`, `Reroute ×4`,
`LinearInterpolate ×3`, **`Fresnel ×2`**, `AppendVector ×2`, `OneMinus ×2`, `Step ×2`,
`VectorParameter ×2`, and one each of `DepthFade`, `DynamicParameter`, `Panner`, `ParticleColor`,
`Power`, `TextureCoordinate`, `TextureSampleParameter2D`.

It samples **one** texture (`Dissolve_Tex`), has **no** `Main_Tex`/`Color_Tex`/`Distortion_Tex`/
`GradientShape_Tex`/`GradientMap_Tex`, and replaces the whole colour chain with a **Fresnel-driven
lerp between two colours** (`Color_Int` inside, `Color_Ext` at grazing angles).

| Parameter | `FirstExplo01` | `FirstExplo02` | `BubbleNoise02` |
|---|---|---|---|
| `Brightness` | **7** | **5** | **7** |
| `Fresnel_01_Expo` | **0.5** | **2** | **3** |
| `Fresnel_01_BaseReflec` | **0.1** | **0** | **0** |
| `Color_Int` | `RGBA(2, 2.20833, 2.5, 0)` | `RGBA(2, 2.20833, 2.5, 0)` | `RGBA(0, 0.05, 1.5, 0)` |
| `Color_Ext` | `RGBA(0, 0.566667, 1, 0)` | `RGBA(0, 0.566667, 1, 0)` | `RGBA(0, 0.166665, 2, 0)` |
| `Color_CoreDifferent` | 0.5 | 0.5 | 0.5 |
| `Core_Intensity` | 0.6 | 0.6 | 0.6 |
| `Core_Power` | 1 | 1 | **1.2** |
| `AddDiss` | **−0.6** | **0** | **0** |
| `Dissolve` / `Dissolve_Invert` | 0 / 0 | 0 / 0 | 0 / 0 |
| `Dissolve_Scale_X` / `_Y` | 1 / **1.4** | 1 / **1.4** | **0.2** / **2** |
| `Dissolve_Speed_X` / `_Y` | 0 / **0.3** | 0 / **0.5** | 0 / **0.75** |
| `Glow_Intensity` | 1 | 1 | 1 |
| `DephFade_Dist` *(sic)* | 0 | 0 | **30** |
| `Dissolve_Tex` | `T_VFX_Noise_05` | `T_VFX_Noise_05` | `T_VFX_Noise_05` |

Note the misspelled parameter name `DephFade_Dist` — transcribed verbatim, do not "fix" it when
matching against the source.

**This family is new shader work**, but it is *small*: one texture, a Fresnel, two colours, a
dissolve. It is a much smaller graph than `M_VFX_DissolveAdd`.

### 4.4 Textures referenced `[corpus]`

| Texture | Size | sRGB | Compression | Address | Format | Role |
|---|---|---|---|---|---|---|
| `T_VFX_Part_01` | 512×512 | false | `TC_Alpha` | `TA_Clamp` | `TSF_G8` | `Part01` all slots; `Flare01` GradientShape/Dissolve |
| `T_VFX_Part_02` | 512×512 | false | `TC_Alpha` | `TA_Clamp` | `TSF_G8` | `Part02` |
| `T_VFX_Part_03` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G8` | `Part03` |
| `T_VFX_Part_04` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `Part04` |
| `T_VFX_Ring_01` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `Ring01` |
| `T_VFX_Ring_02` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `Flare01` Main/Color |
| `T_VFX_Impact_01` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `Impact01` |
| `T_VFX_LightStrip_01` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `LightStrip` |
| `T_VFX_Gradient_03` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | **`TSF_BGRA8`** | `BubbleNoise_01`, `BubbleOut_01` Main/Color |
| `T_VFX_Noise_02` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | Distortion/GradientShape on most |
| `T_VFX_Noise_05` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `BubbleNoise_01` Dissolve; all three FresnelBomb |
| `T_VFX_Noise_06` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `BubbleOut_01` Dissolve |
| `T_VFX_WhitePixel` | 1×1 | true | `TC_Default` | `TA_Wrap` | `TSF_RGBA16` | GradientMap no-op |

`T_VFX_Gradient_03` is the only **colour** (`TSF_BGRA8`) texture in a shape slot in this batch —
every other shape texture is a greyscale mask. The CkUsf DissolveAdd look tints a greyscale mask by
`ParticleColor`; a coloured shape texture is a different composite.

Existing procedural stand-ins usable without new work: `T_VFX_Part_01` → `SoftParticle`,
`T_VFX_Part_04` → `SparkStreak`, `T_VFX_Noise_02` → `TileNoise` (measured in NS_BasicAttack §7).
Candidates worth measuring first: `T_VFX_Ring_01` against the existing SDF `Ring` bake,
`T_VFX_Noise_05/06` against `TileNoise`, `T_VFX_LightStrip_01` against `Streak`.
**New bakes needed:** `T_VFX_Part_02`, `T_VFX_Part_03`, `T_VFX_Ring_02`, `T_VFX_Impact_01`, and a
**colour** bake for `T_VFX_Gradient_03`.

---

## 5. Per-emitter runtime facts and exact curves `[corpus]`

`t` = NormalizedAge over each emitter's own lifetime. `C` = constant key, `L` = linear key.
Where a curve's first key is at `t > 0`, Niagara clamps to that key's value for all earlier `t`.

Shared boilerplate unless contradicted: `Color Mode = Direct Set`, `Position Mode = Simulation
Position`, `Position Offset = (0,0,0)`, `Particle State → Kill Particles When Lifetime Has Elapsed
= true`, `Write Parameter Index 0 = true` (1–3 false), `ScaleColor.Scale Alpha = 1`,
`ScaleColor.Scale RGB = (1,1,1)`, `ScaleSpriteSize.Initial Sprite Size = (0,0)`,
`SolveForcesAndVelocity.Acceleration Limit = 9999`, `Speed Limit = 1000`.

### 5.1 The five Part01 glows — `Glow_01`, `Glow_02`, `Glow_03`, `Glow_04`, `Glow_05`

All are camera-facing `Sprite Size Mode = Uniform` sprites with the **same** `Scale Color` curve
(R/G/B `(0,1)L (1,1)L`; **A `(0,1)L (1,0)L`**) and one of two size curves. Update order on all
five: 1 Particle State, 2 Scale Color, 3 Dynamic Material Parameters, 4 Scale Sprite Size.

| | `Glow_01` | `Glow_02` | `Glow_03` | `Glow_04` | `Glow_05` |
|---|---|---|---|---|---|
| Count / spawn t | 1 / 0 | 1 / 0 | **2** / **0.05** | **2** / **0.05** | 1 / **0.05** |
| Lifetime | 0.2 | 0.2 | 0.3 | 0.3 | **0.1** |
| `Uniform Sprite Size` | **1500** | **800** | **1200** | **700** | **600** |
| Init colour | `RGBA(0.0368895, 0.184475, 1, 0.5)` | `RGBA(0.0193824, 0.0451862, 0.913099, 1)` | `RGBA(0.043735, 0.291771, 1, 0.3)` | `RGBA(0.00972122, 0.130136, 1, 0.8)` | `RGBA(0.665387, 0.768151, 1, 1)` |
| Dyn `Param 1` | 1 | **0** | 2 | 2 | 2 |
| `Uniform Curve Sprite Scale` | `(0, 0.5)C (0.1, 1)L (1, 1)L` | same | **`(0, 0)C (0.2, 1)L (1, 1)L`** | **`(0, 0)C (0.2, 1)L (1, 1)L`** | **`(0, 0)C (0.2, 1)L (1, 1)L`** |
| Material | `Part01` | `Part01` | `Part01` | `Part01` | **`Part03`** |

`Dyn Param 2/3/4 = 0` on all five.

### 5.2 `Ground_Glow_01` / `Ground_Glow_02` — the two ground decals

Identical structure to §5.1 (same Scale Color curve, same `(0, 0.5)C (0.1, 1)L (1, 1)L` size curve,
same update order plus a 5th module), **plus an `Align Sprite to Mesh Orientation` module**:

| | `Ground_Glow_01` | `Ground_Glow_02` |
|---|---|---|
| Count / spawn t | 1 / 0 | 1 / **0.1** |
| Lifetime | 0.2 | 0.2 |
| `Uniform Sprite Size` | **4000** | **2700** |
| Init colour | `RGBA(0.043735, 0.291771, 1, 0.6)` | `RGBA(0.00972122, 0.130136, 1, 1)` |
| Dyn `Param 1` | 1 | 1 |
| `Mesh Orientation Relative Sprite Alignment Vector` | **`(1, 0, 0)`** | **`(1, 0, 0)`** |
| `Mesh Orientation Relative Sprite Facing Vector` | **`(0, 0, 1)`** | **`(0, 0, 1)`** |
| `Orientation Quaternion` | `quat(0, 0, 0, 1)` (identity) | identity |
| Renderer | `CustomAlignment` / `CustomFacingVector` | same |

With identity orientation, facing `(0,0,1)` and alignment `(1,0,0)`, **the quad lies flat in world
XY facing +Z** — a ground decal, exactly the case NS_Lightning_Range §6/§8 added VisTag 4 for.
Sizes 4000 and 2700 make these the largest particles in the effect by far.

Update order: 1 Particle State, 2 Scale Color, 3 Dynamic Material Parameters, 4 Scale Sprite Size,
5 Align Sprite to Mesh Orientation.

### 5.3 `Ring01` — 1 sprite, lifetime **0.1 s**

| Fact | Value |
|---|---|
| Init colour | `RGBA(1, 1, 1, 1)`; module-level **`Color.Scale Alpha = 0.6`** |
| Lifetime | Direct Set **0.1** (`Lifetime Min/Max = 0.3/0.7` stored, unused) |
| Size | Uniform **230** (`Min/Max 150/160` stored, unused) |
| Sprite rotation | `Random`, 0…360 |
| Dyn `Param 2/3/4` | 0 / 0 / 0 |

`Dynamic Material Parameters → Index 0 Param 1` (**`dissolve`**), `Float from Curve`:
**`(0, 1)C (1, -1)C`**.

`Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0, 0.351533)L (1, 0.0159963)C` |
| Green | `(0, 0.947307)L (1, 0.0382044)C` |
| Blue | `(0, 1)L (1, 0.165132)C` |
| Alpha | `(0, 1)C` |

`Uniform Curve Sprite Scale`: `(0, 0)C (0.2, 0.7)C (1, 1)C`.
Update order: 1 Particle State, 2 Dynamic Material Parameters, 3 Color, 4 Scale Sprite Size.

### 5.4 `Spike01` / `Spike02` / `Spike03` — 30 pyramid meshes, lifetime 0.15 s

All three share: burst **10** at t = 0; `Lifetime Mode = Direct Set` **0.15**;
`Mesh Scale Mode = Random Non-Uniform`; `Initial Mesh Orientation` with `Orientation Axis = (1,0,0)`,
`Orientation Vector = (1,0,0)` and `Rotation = Random Range Vector` **min `(0, −0.25, 1)`,
max `(0, 0.25, −1)`**, `Random Seed = 0`; a **DISABLED `Cone Location`** module
(`Cone Angle = 25`, `Cone Axis = (0,0,1)`, `Cone Length = 130`, `Offset = (0,0,0)`,
`Point Distribution = 0`, `Wedge Horizontal/Vertical Angle = 45/45`) — **so all 30 spikes spawn at
the origin**, spread only by orientation; `Uniform Sprite Size = 500` (inert);
`ScaleFloatByCurve.InitialValue = (1,1,1)`; and the **same** `Scale Mesh Size` curve:

`Scale Factor` (`Scale Float by Curve`):
`X: (0, 0)C (0.2, 1.5)C (1, 4.17233e-08)C | Y: (0, 0)C (0.2, 1.5)C (1, 5.66244e-08)C |
Z: (0, 0)C (0.2, 1.5)C` — grow to 1.5× by t = 0.2, then X and Y collapse to zero by death while
**Z holds 1.5**: the spikes flatten into slivers as they die.

| | `Spike01` | `Spike02` | `Spike03` |
|---|---|---|---|
| `Mesh Scale Min` | `(0.02, 0.02, 1)` | `(0.04, 0.04, 1)` | `(0.1, 0.1, 1)` |
| `Mesh Scale Max` | `(0.02, 0.02, 3)` | `(0.04, 0.04, 3)` | `(0.15, 0.15, 3)` |
| Init colour | `RGBA(0.296138, 0.571125, 1, 0.6)` | `RGBA(0, 0.221441, 1, 0.4)` | `RGBA(0.296138, 0.571125, 1, 0.6)` |
| `Color` module | **absent** | **absent** | **present** (below) |
| Update modules | 1 Particle State, 2 Scale Mesh Size | same | 1 Particle State, 2 Scale Mesh Size, 3 Color |

`Spike03`'s `Color` (`Color from Curve`) — the only spike with a colour ramp:

| Channel | Keys |
|---|---|
| Red | `(0, 2)C (0.388772, 0.095)L (1, 0.025)C` |
| Green | `(0, 2)C (0.388772, 0.659996)L (1, 0.157691)C` |
| Blue | `(0, 2)C (0.388772, 1)L (1, 1)C` |
| Alpha | `(0, 1)C` |

`Mesh Uniform Scale = 1`, `Mesh Uniform Scale Min/Max = 1/2` (stored, unused — mode is Random
Non-Uniform). **No emitter writes dynamic parameters** (consistent with `M_VFX_DisAdd_Flat02`
reading none, §4.2).

### 5.5 `FlareImpact` — 1 sprite, lifetime **0.05 s**, LOCAL space

| Fact | Value |
|---|---|
| Init colour | `RGBA(0.644888, 0.2, 1, 1)`; `Color.Scale Alpha = 1` |
| Size | Uniform **350** |
| Dyn `Param 2/3/4` | 0 / 0 / 0 |

`Color` (`Color from Curve`) — over-1 RGB:

| Channel | Keys |
|---|---|
| Red | `(0, 1.132)C (0.447932, 0.147027)L (0.952611, 0.040915)C` |
| Green | `(0, 1.25624)C (0.447932, 0.679543)L (0.952611, 0.171441)C` |
| Blue | `(0, 2)C (0.447932, 1)L (0.952611, 1)C` |
| Alpha | `(0, 1)C (0.996076, 0)L` |

`Dynamic Material Parameters → Index 0 Param 1` (**`dissolve`**): **`(0, 0.5)C (1, -1)C`**.
`Uniform Curve Sprite Scale`: `(0, 0.5)C (0.1, 0.9)C (1, 1)C`.
Update order: 1 Particle State, 2 Color, 3 Dynamic Material Parameters, 4 Scale Sprite Size.

### 5.6 `LightningStrip` — 5 meshes, lifetime rand 0.1–0.15 s, LOCAL space, Facing **Velocity**

| Fact | Value |
|---|---|
| Lifetime | `Lifetime Mode = Random`, **min 0.1, max 0.15** (`InitializeParticle.Lifetime = 0.3` stored, unused) |
| Mesh scale | `Mesh Scale Mode = Non-Uniform`, `Mesh Scale` ← `Random Range Vector 001` **min `(1, 1, 4)`, max `(2, 1, 6)`** |
| Initial Mesh Orientation | `Orientation Axis = (0, 0, 1)`, `Orientation Vector = (1, 0, 0)`; `Rotation` ← `Random Range Vector` **min `(0, 0, 1)`, max `(0, 0.5, −1)`** |
| Velocity | `Add Velocity` ← `Random Range Vector 002` **min `(−1, −1, 0.1)`, max `(1, 1, 1)`** — a *unit-scale* velocity whose only real job is to give the Velocity-facing renderer an axis |
| Init colour | `RGBA(0.341915, 0.184475, 1, 1)`; module-level **`Color.Scale Alpha = 0.5`** |
| Dyn `Param 1/2/3/4` | 0 / 0 / 0 / 0 |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`**.

`Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0.319952, 0.184475)L` |
| Green | `(0.319952, 0.445201)L` |
| Blue | `(0.319952, 1)L` |
| Alpha | `(0.327196, 1)C (1, 0)C` |

`Scale Mesh Size` → `Scale Factor` (`Vector from Curve 001`):
**`X: (0, 0.5)C (0.2, 1)C (1, -1.44926e-08)C | Y: (0, -2.4747e-08)C (1, 1)C |
Z: (0, -5.68323e-08)C (0.2, 0.75)C (1, 1)C`** — X pops then collapses, Y and Z grow from ~0 to 1.

Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Color, 4 Particle State,
5 Dynamic Material Parameters, 6 Scale Mesh Size.

### 5.7 `Sparkles_01` / `Sparkles_02` — 100 velocity-aligned sprites, the effect's bulk

Both burst **50** at t = 0.05, both `Sphere Location` with `Sphere Radius = 80`,
`Hemisphere Z = true` (an **upper hemisphere shell**), `Surface Only = false`,
`Non Uniform Scale = (1,1,1)`, `Sphere Distribution = Random`, `Random Seed = 0`;
both `Add Velocity from Point` with `Origin Offset = (0,0,0)`, `Velocity Falloff Distance = 100`;
both `Sprite Size Mode = Random Non-Uniform`; both `Sprite Rotation Angle = 90` with min/max 0/360;
both carry **two** `Scale Sprite Size` modules.

| | `Sparkles_01` | `Sparkles_02` |
|---|---|---|
| Lifetime | `[corpus-v3]` `Random` ⇒ **`Lifetime Min/Max = 0.3 / 0.5` DRIVES**; the `Random Range Float` override (0.2 / 0.4) is INERT. *Was read as "the pin override normally wins".* | `[corpus-v3]` `Random` ⇒ **`0.2 / 0.3` DRIVES**; same override inert |
| `Velocity Strength` | `Random Range Float 001` **min 2000, max 7000** | **min 4000, max 8000** |
| `Sprite Size Min/Max` | `(20, 120)` / `(35, 140)` | `(20, 60)` / `(30, 80)` |
| Init colour | `RGBA(1, 1, 1, 1)` | **`Random Range Linear Color`, min `RGBA(0, 0.136094, 1, 1)`, max `RGBA(1, 1, 1, 1)`** |
| `Gravity Force` | **absent** | **present, `Gravity = (0, 0, -7000)`** |
| `Color` module | present (below) | **DISABLED** (authored curve identical to `Sparkles_01`'s) |
| `VectorFromCurve.Scale Curve` | `(1, 1, 1)` | **`(0.7, 0.7, 1)`** |
| Dyn `Param 1/2/3/4` | 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 |

`Scale Velocity` → `Velocity Scale` (identical curve on both, before the per-emitter
`Scale Curve` multiplier above): **X/Y/Z all `(0, 1)C (0.2, 0.35)C (1, 0.05)C`**.

`Sparkles_01`'s `Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0, 1)C (0.835497, 0.0409152)C` |
| Green | `(0, 1)C (0.835497, 0.171441)C` |
| Blue | `(0, 1)C (0.835497, 1)C` |
| Alpha | `(0, 1)C` |

Size curves — `Scale Sprite Size` (module 6 / 7 on `_01`, 7 / 8 on `_02`):
- **`_01`** module A: `Uniform: (0, 0)C (0.1, 1)C (1, 0)C`; `Non-Uniform: X: (0,0)L (1,1)L | Y: (0,0)L (1,1)L`
- **`_01`** module B (`Scale Sprite Size 001`): `Uniform: (0, 0)L (1, 1)L`;
  `Non-Uniform: X: (1, 1)L | Y: (0, 1)C (0.9, 0.3)C`
- **`_02`** module A: identical to `_01` module A
- **`_02`** module B: `Uniform: (0, 0)L (1, 1)L`; `Non-Uniform: X: (1, 1)L | Y: (0, 1)C (1, 0.6)C`

Update order — `_01`: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Particle State, 4 Color,
5 Dynamic Material Parameters, 6 Scale Sprite Size, 7 Scale Sprite Size 001.
`_02`: 1 **Gravity Force**, 2 Scale Velocity, 3 Solve Forces and Velocity, 4 Particle State,
5 Color *(disabled)*, 6 Dynamic Material Parameters, 7 Scale Sprite Size, 8 Scale Sprite Size 001.

**`Sparkles_02` is the "debris" pass**: random blue-to-white colour, gravity −7000, no colour ramp,
faster ejection (4000–8000) with a 0.7 XY damping — it arcs and falls. `Sparkles_01` is the bright
radial flash pass.

### 5.8 `Flare_01` / `Flare_02` / `Flare03`

| | `Flare_01` | `Flare_02` | `Flare03` |
|---|---|---|---|
| Space | **Local** | **Local** | World |
| Count / spawn t | **5** / 0.1 | **5** / 0.1 | 1 / 0.1 |
| Lifetime | 0.2 | 0.2 | 0.2 |
| `Uniform Sprite Size` | **900** | **1700** | **320** |
| Init colour | `RGBA(0.102242, 0.577581, 1, 0.4)` | `RGBA(0.0368895, 0.0908417, 1, 0.4)` | `RGBA(0.913099, 0.0193824, 0.130136, 0.4)` |
| Module-level alpha scale | — | — | **`Color.Scale Alpha = 0.8`** |
| Dyn `Param 1` | 0 | 0 | 1 |
| Material | `Part02` | `Part02` | `Flare01` |
| Update modules | 1 Particle State, 2 Scale Color, 3 Dynamic Material Parameters | same | 1 Particle State, 2 Color, 3 Dynamic Material Parameters, 4 Scale Sprite Size |

`Flare_01`/`Flare_02` `Scale Color` (identical), plus an **empty** `Linear Color Curve` override
(`Red: | Green: | Blue: | Alpha:` — no keys; recorded verbatim because an empty curve override is a
trap for a reader who assumes it means something):

| Channel | Keys |
|---|---|
| Red | `(0.193178, 1)L (1, 1)L` |
| Green | `(0.193178, 1)L (1, 1)L` |
| Blue | `(0.193178, 1)L (1, 1)L` |
| Alpha | **`(0, 0)L (0.193178, 1)L (1, 0)L`** |

Neither has a size curve — they hold 900 / 1700 for their whole 0.2 s.

`Flare03`'s `Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0, 1)C (0.352551, 0.147027)L (1, 0.0409152)C` |
| Green | `(0, 1)C (0.352551, 0.679543)L (1, 0.171441)C` |
| Blue | `(0, 1)C (0.352551, 1)L (1, 1)C` |
| Alpha | `(0, 1)C (1, 0)L` |

`Flare03` `Uniform Curve Sprite Scale`: `(0, 0.5)C (0.1, 1)L (1, 1)L`.

### 5.9 The five Bubbles — mesh, Facing **CameraPosition**

All five: burst **1**; `Mesh Scale Mode = Uniform`; `Initial Mesh Orientation` with
`Orientation Axis = (0, 0, 1)`, `Orientation Vector = (1, 0, 0)`, `Rotation = (0, 0, 0)` (**no
randomness**); `ScaleFloatByCurve.InitialValue = (1,1,1)`; `Uniform Sprite Size = 500` (inert).

| | `Bubble_Noise01` | `Bubble_First_Explo` | `Bubble_Out` | `Bubble_Noise02` | `Bubble_Fresnel` |
|---|---|---|---|---|---|
| Spawn t | 0.1 | **0** | 0.1 | 0.1 | 0.1 |
| Lifetime | 0.2 | **0.15** | 0.2 | **0.3** | **0.3** |
| `Mesh Uniform Scale` | **2** | **2** | **3** | **2** | **2** |
| Mesh | `SM_VFX_Sphere02` | `SM_VFX_Sphere01` | `SM_VFX_Ring03` | `SM_VFX_Sphere02` | `SM_VFX_Sphere01` |
| Material | `BubbleNoise_01` | `FresnelBomb_FirstExplo01` | `BubbleOut_01` | `FresnelBomb_FirstExplo02` | `FresnelBomb_BubbleNoise02` |
| Init colour | `RGBA(0.162029, 0.708376, 1, 1)` | `RGBA(0.162029, 0.708376, 1, 1)` | `RGBA(0.162029, 0.708376, 1, 1)` | **`RGBA(1, 1.18333, 2, 1)`** | **`RGBA(1, 1.18333, 2, 1)`** |
| Dyn `Param 1` | **−0.5** *(static)* | curve *(below)* | curve *(below)* | curve *(below)* | curve *(below)* |
| Dyn `Param 2/3/4` | 0 | 0 | 0 | 0 | 0 |

`Scale Mesh Size` → `Scale Factor` (`Scale Float by Curve`), per emitter, all three axes identical
within an emitter:

| Emitter | X / Y / Z keys |
|---|---|
| `Bubble_Noise01` | `(0, 1)C (0.2, 1.5)C (1, 1)C` |
| `Bubble_First_Explo` | **`(0, 0)C (1, 1)C`** |
| `Bubble_Out` | **`(0, 0)C (0.7, 1.5)C`** |
| `Bubble_Noise02` | `(0, 1)C (0.2, 1.5)C (1, 1)C` |
| `Bubble_Fresnel` | `(0, 1)C (0.2, 1.5)C (1, 1)C` |

`Dynamic Material Parameters → Index 0 Param 1` (**`dissolve`**), `Float from Curve`:

| Emitter | Keys |
|---|---|
| `Bubble_Noise01` | *(static −0.5, no curve)* |
| `Bubble_First_Explo` | **`(0, -0.4)C (0.5, 0)C`** |
| `Bubble_Out` | **`(0.4, 0)C (1, -2)C`** |
| `Bubble_Noise02` | **`(0, -0.2)C (1, 1)C`** |
| `Bubble_Fresnel` | **`(0, -0.2)C (1, 1)C`**, with **`Param0WriteEnabled = true`** |

Colour treatment differs per emitter:

`Bubble_Noise01` — `Color` (`Color from Curve`):
`Red: (0, 1)C (0.759433, 0.035)C | Green: (0, 1)C (0.759433, 0.0587079)C |
Blue: (0, 1)C (0.759433, 0.7)C | Alpha: (0.402053, 1)C (1, 0)C`.
Update order: 1 Particle State, 2 Scale Mesh Size, 3 Dynamic Material Parameters, 4 Color.

`Bubble_First_Explo` — `Scale Color` (`RGBA Together`) plus an **empty** `Linear Color Curve`:
`Red/Green/Blue: (0, 1)L (1, 1)L | Alpha: (0, 0)L (1, 1)L` — **alpha ramps UP over life**.
Update order: 1 Particle State, 2 Scale Color, 3 Scale Mesh Size, 4 Dynamic Material Parameters.

`Bubble_Out` — `Color` (`Color from Curve`):
`Red: (0, 1)C (0.872925, 0.035)C | Green: (0, 1)C (0.872925, 0.0587077)C |
Blue: (0, 1)C (0.872925, 0.7)C | Alpha: (0.351343, 1)C (1, 0)C`.
Update order: 1 Particle State, 2 Scale Mesh Size, 3 Dynamic Material Parameters, 4 Color.

`Bubble_Noise02` — `Scale Color` plus an empty `Linear Color Curve`:
`Red/Green/Blue: (0, 1)L (0.748566, 0)L | Alpha: (0.0772714, 0)L (1, 1)L` — RGB fades to black by
t = 0.749 while alpha ramps up. Update order: 1 Particle State, 2 Scale Color, 3 Scale Mesh Size,
4 Dynamic Material Parameters.

`Bubble_Fresnel` — `Color` (`Color from Curve`):
`Red: (0, 1)C (0.614549, 0)L | Green: (0, 1)C (0.614549, 0.0736187)L |
Blue: (0, 1)C (0.614549, 1)L | Alpha: (0, 0)C (1, 1)L`.
Update order: 1 Particle State, 2 Scale Mesh Size, 3 Dynamic Material Parameters, 4 Color.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new row in `ck::particles::Get_TemplateSpecs()` is required.**

| Field | Value | Why |
|---|---|---|
| Loop duration | **2.0 s** `[corpus-v3]` | the system's `Once` loop duration ([P0-D3]). *Was `[unresolved]`.* |
| Particle lifetime | **0.5 s** `[corpus-v3]` | max resolved lifetime — `Sparkles_01`'s resolved `Lifetime Max` 0.5. *Was 0.3 s, which assumed the 0.2/0.4 override drove `Sparkles_01`; [P0-D2] flips that.* |
| Burst count | **162** | the §2 total |

162 is well above the current maximum row (96 on `_Burst`), but it is the *same kind* of number —
no new mechanism, just a bigger burst. Layer partition `Seed % 162` with the layer→emitter map from
§2. Spawn beats (0 / 0.05 / 0.1 s) are reproduced by hiding a layer for `Age < SpawnDelay` and
running its curves on `(Age − SpawnDelay) / Lifetime`, per NS_BasicAttack §5's spark-delay mechanism.

**Verify the burst-count ceiling before committing.** `Add_SpawnEmitterStack` reads `BurstCount`
straight off the spec, but nothing in the cookbook records a template that has been built at 162,
and Niagara's CPU/GPU allocation behaviour at that count on this template shape is unverified
`[unresolved: no evidence either way in the corpus or the module docs]`.

### 6.2 VisTag / renderer needs

| Source emitters | Renderer needed | Available today? |
|---|---|---|
| `Ground_Glow_01/02` | custom-facing sprite (ground decal) | **YES — shared VisTag 4**, which shares `User.SpriteMaterial` with VisTag 0 |
| `Sparkles_01/02` | velocity-aligned sprite, one material (`Part04`) | **YES** — shared VisTag 1, or one row-declared `VelocityAlignedSprite` |
| `Spike01/02/03` | mesh, Facing **Default**, one mesh + one look | **YES** — row-declared `Mesh` (NS_BasicAttack capability) |
| `Glow_01…05`, `Flare_01/02/03`, `Ring01`, `FlareImpact` | camera-facing sprite, **6 distinct materials** | **NO — gap 1** |
| `LightningStrip` | mesh, Facing **Velocity** | **NO — gap 2** |
| `Bubble_*` ×5 | mesh, Facing **CameraPosition**, 4 distinct materials | **NO — gap 2 + gap 3** |

### 6.3 Mesh needs

All five meshes are procedurally reproducible (§3) — **this is the batch's easy mesh case**:

| Generated mesh (proposed) | Reproduces | Build |
|---|---|---|
| `Spike` | `SM_VFX_Spike01` | square pyramid, base ±100 in XY at Z = 0, apex `(0,0,200)`, 6 tris; **v = 0 at the tip, 1 at the base**; u across |
| `Card` | `SM_VFX_Plane01` | single flat quad in XZ, `X ±100`, `Z 0…200`; `u` = −X→+X, `v` = top→bottom; drop the 0.0643 Y offset and make the look `_TwoSided` |
| `FlatAnnulus` | `SM_VFX_Ring03` | 2 radial bands × 64 segments, radii **53.96 / 76.98 / 100.00**, flat in XY; **v = 0 INNER → 1 OUTER**; `u = 0.75 + angle/360` mod 1, seam at ±180° |
| `UvSphere` | `SM_VFX_Sphere01` **and** `SM_VFX_Sphere02` (measured identical, §3.4) | radius 100, 8 latitude bands (~32×16), v = 0 at +Z pole → 1 at −Z pole |

The existing carrier set (`Sweep`/`Tube`/`Shell`/`Disc`) may already cover `Card` and `UvSphere` —
**check `CkParticles_MeshGenerator.cpp` before adding duplicates**, and match the measured UV
conventions above rather than the carriers' existing ones.

### 6.4 Look / texture needs

| Look (proposed) | Family | Notes |
|---|---|---|
| `ExpGlowDisAdd01` | `CkUsf_Look_DissolveAdd` | `Part01` reference values; serves 6 emitters |
| `ExpGlowDisAdd02` | existing | `Part02` (`Glow_Intensity` 0.3) — serves `Flare_01/02` |
| `ExpGlowDisAdd03` | existing | `Part03` |
| `ExpSparkDisAdd04` | existing | `Part04` — serves both 50-sparkle bursts |
| `ExpRingDisAdd01` | existing | `Ring01` — likely a straight reuse of `RingDissolveAdd` with a different `ShapeTex` |
| `ExpImpactDisAdd01` | existing | `Impact01`; needs **`Core_Power` 0** plumbed |
| `ExpFlareDisAdd01` | existing | `Flare01`; needs `Gradient_Invert` 0.847619 plumbed and a **separate `GradientShape_Tex`** |
| `ExpSpikeFlatAdd02` | **new tiny `M_VFX_FlatAdd` shader** | `ParticleColor × 10`, depth fade; draws 30 particles |
| `ExpBubbleNoiseDisAdd` | existing | `BubbleNoise_01`; needs **`Core_Intensity` 20** and a **colour shape texture** |
| `ExpBubbleOutDisAdd` | existing | `BubbleOut_01`; needs `Core_Intensity` 5, `Color_CoreDifferent` 1, `DissolveScale` (0.3, 0.5), `DissolveSpeedY` 1 |
| `ExpFresnelBomb01/02/03` | **new `M_VFX_FresnelBomb` family shader** | one texture + a Fresnel lerp between `Color_Int`/`Color_Ext` + a dissolve (§4.3). Three parameterizations of one small shader |

**CkUsf family parameters not plumbed today that this effect needs:** `Core_Power`,
`Core_Intensity`, `Color_CoreDifferent`, `Gradient_Invert`, a separate `GradientShape_Tex` slot,
and (for the FresnelBomb family) `Fresnel_01_Expo` / `Fresnel_01_BaseReflec` / `Color_Int` /
`Color_Ext` / `AddDiss` / `DephFade_Dist`. The Fresnel family is a **new shader**, not a
parameterization — but a small one.

Textures: reuse `SoftParticle` (`T_VFX_Part_01`), `SparkStreak` (`T_VFX_Part_04`), `TileNoise`
(`T_VFX_Noise_02`). Measure-then-decide on `T_VFX_Ring_01` vs the `Ring` bake, `T_VFX_Noise_05/06`
vs `TileNoise`, `T_VFX_LightStrip_01` vs `Streak`. **New bakes:** `T_VFX_Part_02`, `T_VFX_Part_03`,
`T_VFX_Ring_02`, `T_VFX_Impact_01`, and a **colour** bake for `T_VFX_Gradient_03`.

### 6.5 Capability gap callout

| # | Gap | Severity |
|---|---|---|
| 1 | **No row-declared camera-facing sprite kind.** `FCk_ParticlesRendererSpec` has only `Mesh` and `VelocityAlignedSprite`. Nine emitters here are `Unaligned`/`FaceCamera` across **six** materials; one `User.SpriteMaterial` binding cannot carry them. Additive fix, shared with every sheet in this batch. | **BLOCKING** |
| 2 | **Mesh renderer facing mode is not expressible on a row-declared `Mesh` renderer.** The spec carries `Kind`, `VisTag`, `MeshName`, `LookName` — no facing enum. `LightningStrip` needs `Facing: Velocity` and all five Bubbles need `Facing: CameraPosition`. **Velocity facing can be faked** by writing an `Orientation` quat from the behavior's own velocity; **CameraPosition facing cannot** — the behavior has no camera. | **BLOCKING** for the 5 Bubbles; workaround for `LightningStrip` |
| 3 | **How much CameraPosition facing matters depends on the mesh.** For `SM_VFX_Sphere01/02` a rotation is nearly invisible in silhouette but **not** in the material, because the dissolve pans in UV space and the sphere's v axis is tied to the mesh's Z pole (§3.4) — a fixed orientation gives a fixed dissolve pattern instead of one that tracks the viewer. For `SM_VFX_Ring03` (`Bubble_Out`) it is a **flat annulus**, and a non-billboarded flat annulus edge-on is invisible. Do not assume "it's a sphere, facing doesn't matter". | **BLOCKING** for `Bubble_Out` |
| 4 | **A new material family (`M_VFX_FresnelBomb`)** — a Fresnel-driven two-colour lerp, 3 instances, 5 of the 23 emitters (§4.3). Real work, but a small graph. Also note `Fresnel` needs a view vector, which a `SurfaceUnlit` CkUsf look does have access to `[inferred: CkUsf looks can read camera vector; confirm before scoping]`. | Medium |
| 5 | **System-level loop parameters absent from the corpus** (§2). Cadence cannot be finalized. | **Prerequisite** |
| 6 | **Burst of 162** is 1.7× the largest row built to date; unverified at that count. | Verify |
| 7 | **World space on 19 of 23 emitters.** The template is local-space (NS_BasicAttack §13.2). For an explosion at a fixed point this is low-risk. | Low |
| 8 | **`Opacty_DepthFade`** 20 / 30 / 0 across instances; and `DephFade_Dist` 30 on one FresnelBomb. CkUsf surface looks do not wire scene depth (known gap). **This matters more here than elsewhere** — the two `Ground_Glow_*` decals are 2700–4000 units wide and lie flat on the ground, exactly where a hard intersection line reads badly. | Medium |
| 9 | **A colour shape texture** (`T_VFX_Gradient_03`, `TSF_BGRA8`) in a slot the CkUsf look treats as a greyscale mask tinted by `ParticleColor`. | Medium |
| 10 | ~~Two `[unresolved]` sparkle lifetime ranges~~ — **RESOLVED `[corpus-v3]`** (§5.7): `Lifetime Mode = Random` ⇒ Min/Max drives on both ([P0-D2]); `Sparkles_01` 0.3–0.5, `Sparkles_02` 0.2–0.3. | Closed |
| 11 | **`Sparkles_02`'s `Random Range Linear Color`** spawn colour. Expressible as `CkParticles_Rand` per channel; not a gap, but it must stay bit-identical GPU↔CPU. | Low |

**Complexity tier: L**, driven by gaps 2/3 (mesh facing) and 4 (a new material family). Without
those, **17 of 23 emitters are reachable at M tier** once gap 1 lands: all the glows, flares, the
ring, the impact, the two ground decals (VisTag 4 already exists for exactly this), 100 sparkles on
the existing velocity-aligned sprite, and 30 spikes on a 6-triangle procedural pyramid with the
cheapest look in the cookbook. **That 17-emitter subset is the right first delivery** and it is the
majority of the visible effect; the five Bubbles are the shell/shockwave layer and can follow.

### 6.6 Behavior id

**Do not allocate an id in this document.** Take the next free id from
`ck::particles::NumBehaviors` at implementation time, bump it, and update the roster paragraph in
`CkParticles/CLAUDE.md`. Five sibling sheets were written in the same pass and none allocates an id
— allocate them in one ordered pass so they cannot collide.

---

## 7+. Reserved for implementation

Sections 7–14 of the recipe schema ([README.md](README.md)) are intentionally absent and are to be
written by the implementation session, from what actually happened.
