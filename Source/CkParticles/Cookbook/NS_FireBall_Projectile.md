# Recipe: NS_FireBall_Projectile → CkParticles (PRE-IMPLEMENTATION TRANSLATION SHEET)

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
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_FireBall_Projectile` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior id | **not allocated** — take the next free id at implementation time |
| Recreation status | not started |

Corpus evidence (all `[corpus]`, exported 2026-08-01):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_FireBall_Projectile.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part03_Bright,Part04,Flames01,Smoke01}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_FlatAdd.json`
- `textures/Vefects/Anime_VFX/Shared/Textures/*.json`

**No meshes.** Sprite and ribbon renderers only.

**The source Niagara asset was not opened for editing.**

> ### A same-named sibling exists — and the casing differs
> `[corpus]` The pack ships `Vefects/Anime_Stylized_VFX/VFX/Particles/**NS_Fireball_Projectile**`
> — note the lowercase **b**. That alone is a discriminator, but do not rely on a case-insensitive
> filesystem to preserve it. **Fastest robust discriminator: the sibling declares 8 user parameters**
> (`User.Flames Color 01`, `User.Glow Color 01…05`, `User.Scale Overall`, `User.Smoke Color 01`) and
> renders through `MI_VFX_*` instances (`MI_VFX_Glow_01`, `MI_VFX_Glow_03_Bright`, `MI_VFX_Glow_04`,
> `MI_VFX_Flames_01`, `MI_VFX_Smoke_01`, `MI_VFX_Flat_01`). The `Anime_VFX/Shared/Skills` variant
> documented here has an **empty user-parameter list** and renders through `M_VFX_DisAdd_*`.

---

## 2. System anatomy `[corpus]`

**9 CPU emitters. This is the only system in its batch that is a genuine LOOPING effect rather than
a one-shot** — six emitters carry a `Spawn Rate` module in addition to (or instead of) a burst.

| # | Emitter | Space | Spawn | Burst count | Burst t | Rate (/s) | Lifetime | Renderer | Material |
|---|---|---|---|---|---|---|---|---|---|
| 0 | `SecondGlow` | **Local** | Burst + Rate | 1 | 0 | **5** | 0.5 | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Part03_Bright` |
| 1 | `Flames01` | World | Burst + Rate | 5 | 0 | **200** | rand 0.2–0.3 | Sprite, Unaligned/FaceCamera, **SubUV 2×2** | `M_VFX_DisAdd_Flames01` |
| 2 | `Smokes` | World | Burst + Rate | 5 | **0.04** | **200** | rand 0.3–0.4 | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Smoke01` |
| 3 | `FirstGlow` | **Local** | Burst + Rate | 1 | **0.05** | **3** | 1 | Sprite, Unaligned/FaceCamera | `M_VFX_DisAdd_Part01` |
| 4 | `Projectile_01` | **Local** | Burst | 1 | 0 | — | **10** | Sprite, **VelocityAligned**/FaceCamera | `M_VFX_DisAdd_Part04` |
| 5 | `Projectile_02` | **Local** | Burst | 1 | 0 | — | **10** | Sprite, **VelocityAligned**/FaceCamera | `M_VFX_DisAdd_Part01` |
| 6 | `Projectile_03` | **Local** | Burst | 1 | 0 | — | **10** | Sprite, **VelocityAligned**/FaceCamera | `M_VFX_DisAdd_Part01` |
| 7 | `Trail_01` | World | Rate only | — | — | **50** | 0.25 | **Ribbon** | `Parents/M_VFX_FlatAdd` |
| 8 | `Trail_02` | World | Rate only | — | — | **50** | 0.25 | **Ribbon** | `Parents/M_VFX_FlatAdd` |

Burst particles at loop start: **13** (1 + 5 + 5 + 1 + 1 + 1 + 1 − wait: 1+5+5+1+1+1+1 = 15;
`Smokes`'s 5 land at t = 0.04, `FirstGlow`'s 1 at t = 0.05). Sustained rate afterwards:
**408 sprites/s** (5 + 200 + 200 + 3) plus **100 ribbon points/s** (50 + 50). Bounds `Dynamic`
everywhere, `Determinism: false`.

> **`Life Cycle Mode = Self` on all 9 emitters `[corpus]` — this system is the batch's exception.**
> The per-emitter Loop Behavior / Loop Duration values below are therefore **live**, not leftovers:
>
> | Emitter | Loop Behavior | Loop Duration Mode | Loop Duration | Inactive Response |
> |---|---|---|---|---|
> | `SecondGlow` | Infinite | Fixed | **10** | Kill |
> | `Flames01` | Infinite | **Infinite** | 10 *(inert — duration mode is Infinite)* | Complete |
> | `Smokes` | Infinite | Fixed | **10** (also `EmitterState.Loop Count = 1`) | Complete |
> | `FirstGlow` | Infinite | Fixed | **10** | Kill |
> | `Projectile_01/02/03` | Infinite | Fixed | **10** | Kill |
> | `Trail_01/02` | Infinite | Fixed | **10** | Complete |
>
> A 10-second fixed loop with `Projectile_*` particles that live exactly 10 s means the three
> velocity-aligned core sprites are effectively **permanent for the projectile's flight** — they are
> re-burst once per loop and the loop is longer than any real projectile lifetime.

**Every emitter carries a `Position Offset`, and they differ** — this is the effect's shape:

| Emitter | `InitializeParticle.Position Offset` |
|---|---|
| `SecondGlow` | `(-16.6948, 0, 0)` |
| `Flames01` | `(-27.6096, 0, 0)` |
| `Smokes` | `(-24.8649, 0, 0)` |
| `FirstGlow` | `(-17.223, 0, 0)` |
| `Projectile_01/02/03` | `(-20.1693, 0, 0)` |
| `Trail_01/02` | `(0, 0, 0)` |

All offsets are **−X**, i.e. behind the projectile: the fireball travels **+X** and its flames,
smoke and glows sit 16–28 units behind the core. `Projectile_01/02/03` and `Trail_01/02` add
velocity along **+X** / **−X** respectively (§5).

---

## 3. Mesh geometry

**N/A — this system uses no mesh renderers.** Two sprite kinds (camera-facing and velocity-aligned)
and two ribbons.

---

## 4. Material families and per-instance deltas `[corpus]`

### 4.1 `M_VFX_DissolveAdd` family — 5 of the 6 materials

`M_VFX_DisAdd_{Part01, Part03_Bright, Part04, Flames01, Smoke01}` are instances of
`Parents/M_VFX_DissolveAdd`. Identical base properties on all five: `MD_Surface`,
**`BLEND_Translucent`**, **`MSM_Unlit`**, `twoSided: false`, outputs **`EmissiveColor` + `Opacity`**,
dynamic-parameter channels **`[dissolve, distortion, offset, core_color]`**, and the family
expression histogram (`ScalarParameter ×41`, `Multiply/Add/AppendVector ×18` each, `Saturate ×12`,
`DynamicParameter ×8`, `Reroute ×8`, `TextureSampleParameter2D ×6`, `Panner ×5`, `Constant ×5`,
`TextureCoordinate ×5`, `LinearInterpolate ×4`, `Clamp ×2`, `OneMinus ×2`, and one each of
`DepthFade`, `ParticleColor`, `Power`, `SmoothStep`, `StaticBoolParameter`, `StaticSwitch`,
`VectorParameter`, `MaterialFunctionCall`, `WorldPosition`).

Reference instance = `M_VFX_DisAdd_Part01`: `Main_Tex`/`Color_Tex`/`Dissolve_Tex` = `T_VFX_Part_01`;
`Distortion_Tex`/`GradientShape_Tex` = `T_VFX_Noise_02`; `GradientMap_Tex` = `T_VFX_WhitePixel`;
`Brightness` 1; `Opacity_Boldness` 0.5; `Distortion_Intensity` 0; `Dissolve` 0; all `*_Scale_X/Y` 1;
all `*_Speed_X/Y` and `*_Offset_X/Y` 0; `Gradient_Invert` 0.5; `GradientMap_Displacement`
0.10000000149011612; `Color_CoreDifferent` 0; `Core_Power` 1; `Core_Intensity` 0; `Glow_Intensity` 1;
`Opacty_Step` 0; `Opacty_StepAdd` 0.10000000149011612; `Opacty_DepthFade` 20; `CamOffset` 0;
`Color_Core` `RGBA(1, 1, 1, 0)`.

| Instance | Delta vs `Part01` |
|---|---|
| `M_VFX_DisAdd_Part01` | *(reference)* — `Projectile_02`, `Projectile_03`, `FirstGlow` |
| `M_VFX_DisAdd_Part03_Bright` | `Brightness` → **10**; **`CamOffset` 0 → 50**; `Core_Intensity` → **1**; `Opacity_Boldness` → **1**; Main/Color/Dissolve → **`T_VFX_Part_03`** |
| `M_VFX_DisAdd_Part04` | `Brightness` → **6**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` → **1**; `Opacty_DepthFade` 20 → **30**; Main/Color/Dissolve → **`T_VFX_Part_04`** |
| `M_VFX_DisAdd_Flames01` | `Brightness` → **10**; `Core_Intensity` → **1**; **`Dissolve` 0 → −0.1**; `Dissolve_Scale_X/Y` 1 → **2/2**; **`Distortion_Intensity` 0 → 0.5**; `Distortion_Scale_X/Y` 1 → **2/2**; `Distortion_Speed_X/Y` 0 → **−0.3/−0.3**; `Glow_Intensity` 1 → **2**; `Gradient_Invert` → **0**; `Opacity_Boldness` → **1**; Main/Color → **`T_VFX_Wind_01`**; Dissolve/Distortion → **`T_VFX_Noise_04`**; `Color_Core` → **`RGBA(0.015996, 0.014444, 0.014444, 1)`** |
| `M_VFX_DisAdd_Smoke01` | `Brightness` → **10**; `Core_Intensity` → **1**; **`Dissolve` 0 → −0.1**; **`Distortion_Intensity` 0 → 0.4**; `Distortion_Speed_X/Y` 0 → **0.1/0.1**; `GradientMap_Displacement` 0.1 → **0.75**; `Gradient_Invert` → **0**; **`Opacity_Boldness` 0.5 → 3**; `Color_Tex` → **`T_VFX_Cloud_04`**; `Main_Tex` → **`T_VFX_Cloud_05`**; `Dissolve_Tex` → **`T_VFX_Noise_07`**; `Distortion_Tex` → **`T_VFX_Noise_04`**; `Color_Core` → **`RGBA(0.001, 0.001, 0.001, 1)`** |

**`Flames01` and `Smoke01` are the first instances in this batch that actually turn distortion on**
(0.5 and 0.4). NS_Lightning_Range §13.1 dropped the distortion branch because its instance resolved
`Distortion_Intensity = 0`; that reasoning does **not** transfer — the CkUsf family already exposes
`DistortIntensity` (NS_BasicAttack §9, param 13), so this is a parameterization, not new shader work.

**`Smoke01` is also the first with `Main_Tex ≠ Color_Tex`** (`T_VFX_Cloud_05` vs `T_VFX_Cloud_04`).
The CkUsf family's `ShapeTex` maps `Main_Tex`/`Color_Tex` as one slot (NS_BasicAttack §9, params
1–3) — a genuine, if small, fidelity gap.

### 4.2 `M_VFX_FlatAdd` (parent, used directly by both ribbons)

| Fact | Value |
|---|---|
| Parent chain | **none — this is a base material**, used directly by `Trail_01`/`Trail_02` |
| Blend / shading | `BLEND_Translucent`, `MSM_Unlit`, `twoSided: false`, `MD_Surface` |
| Connected outputs | `EmissiveColor` + `Opacity` |
| Dynamic parameters | **none** |
| Texture parameters | **none** |
| Expressions | `Multiply ×2`, `ScalarParameter ×3`, `VectorParameter ×1`, `ParticleColor ×1`, `DepthFade ×1`, `WorldPosition ×1`, `MaterialFunctionCall ×1` |
| `Brightness` | **1** |
| `Opacty_DepthFade` | 0 |
| `CamOffset` | 0 |
| `Color_Core` | `RGBA(1, 1, 1, 0)` |

**This is the simplest material in the entire batch**: `ParticleColor × Brightness`, depth-faded,
with no texture and no dynamic parameters. Trivial to recreate as a CkUsf look — the difficulty is
entirely the *ribbon renderer* it draws through (§6.5).

Note the emitters *do* write dynamic parameters (`Trail_01/02` set `Index 0 Param 1` from a curve,
§5.7) which `M_VFX_FlatAdd` **does not read**. Recorded so a future session does not chase it.

### 4.3 Textures referenced `[corpus]`

| Texture | Size | sRGB | Compression | Address | Format | Role |
|---|---|---|---|---|---|---|
| `T_VFX_Part_01` | 512×512 | false | `TC_Alpha` | `TA_Clamp` | `TSF_G8` | `Part01` Main/Color/Dissolve |
| `T_VFX_Part_03` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G8` | `Part03_Bright` |
| `T_VFX_Part_04` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `Part04` |
| `T_VFX_Wind_01` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `Flames01` Main/Color |
| `T_VFX_Cloud_04` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `Smoke01` Color_Tex |
| `T_VFX_Cloud_05` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `Smoke01` Main_Tex |
| `T_VFX_Noise_02` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | Distortion/GradientShape on `Part01`, `Part03_Bright`, `Part04` |
| `T_VFX_Noise_04` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `Flames01`/`Smoke01` Dissolve/Distortion |
| `T_VFX_Noise_07` | 512×512 | false | `TC_Alpha` | `TA_Wrap` | `TSF_G16` | `Smoke01` Dissolve |
| `T_VFX_WhitePixel` | 1×1 | true | `TC_Default` | `TA_Wrap` | `TSF_RGBA16` | GradientMap no-op |

Existing procedural stand-ins usable without new work: `T_VFX_Part_01` → `SoftParticle`,
`T_VFX_Part_04` → `SparkStreak`, `T_VFX_Noise_02` → `TileNoise` (all three measured in
NS_BasicAttack §7). **Candidates worth measuring before assuming:** `T_VFX_Wind_01` against the
existing `WindBand` bake (which was parameterized off `T_VFX_Wind_03`, a *different* asset), and
`T_VFX_Noise_04` against `TileNoise`. **New bakes needed:** `T_VFX_Part_03`, `T_VFX_Cloud_04`,
`T_VFX_Cloud_05`, `T_VFX_Noise_07`. The existing `Smoke` bake (FBM + erosion in A) is the natural
starting parameterization for the two Cloud textures — **measure, then bake from the numbers.**

---

## 5. Per-emitter runtime facts and exact curves `[corpus]`

`t` = NormalizedAge over each emitter's own lifetime. `C` = constant key, `L` = linear key.
Where a curve's first key is at `t > 0`, Niagara clamps to that key's value for all earlier `t`.

Shared boilerplate unless contradicted: `Color Mode = Direct Set`, `Position Mode = Simulation
Position`, `Particle State → Kill Particles When Lifetime Has Elapsed = true`,
`Write Parameter Index 0 = true` (indices 1–3 false), `SolveForcesAndVelocity.Acceleration Limit =
9999`, `Speed Limit = 1000`, `VectorFromCurve.Scale Curve = (1,1,1)`, `FloatFromCurve.Scale Curve = 1`.

### 5.1 `SecondGlow` — 1 burst + 5/s, lifetime 0.5 s, LOCAL space

| Fact | Value |
|---|---|
| Position offset | `(-16.6948, 0, 0)` |
| Init colour | `RGBA(0.102242, 0.658375, 1, 0.2)`; module-level **`Color.Scale Alpha = 0.5`**, `Scale Color = (1,1,1)` |
| Size | `Sprite Size Mode = Uniform`, `Uniform Sprite Size = 130` |
| Dyn params | `Index 0 Param 1 = 3.5`, `2/3/4 = 0` |

`Scale Sprite Size` carries **both** curves:
- `Uniform Curve Sprite Scale`: `(0, 0.5)C (1, 1)C`
- `Non-Uniform Curve Sprite Scale`: `X: (0, 1)C (1, 0.4)C | Y: (0, 1)C (1, 0)C`

`Color` (`Color from Curve`) — note the **over-1 RGB**, an HDR ramp:

| Channel | Keys |
|---|---|
| Red | `(0.618171, 2)L (0.963477, 3)L` |
| Green | `(0.618171, 0.683829)L (0.963477, 2.25883)L` |
| Blue | `(0.618171, 0.218923)L (0.963477, 0.328385)L` |
| Alpha | `(0, 0)C (0.246302, 1)C (0.618171, 1)L (1, 0)L` |

Update order: 1 Particle State, 2 Dynamic Material Parameters, 3 Scale Sprite Size, 4 Color.

### 5.2 `Flames01` — 5 burst + 200/s, lifetime rand, WORLD space, **SubUV 2×2**

| Fact | Value |
|---|---|
| Position offset | `(-27.6096, 0, 0)` |
| Lifetime | `Lifetime Mode = Random`; `InitializeParticle.Lifetime Min/Max = **0.2 / 0.3**`; the `Lifetime` pin is overridden by `Random Range Float` with **min 0.2 / max 0.4** `[unresolved: which of the two ranges is live — the pin override normally wins, giving 0.2–0.4]` |
| Spawn shape | **Sphere Location**, `Sphere Radius = 10`, `Sphere Orientation Axis = (1,0,0)`, `Non Uniform Scale = (1,1,1)`, `Radius Position = 1`, `U Position = 0`, `V Position = 0.5`, `Uniform Distribution = 1`, `Uniform Spiral Amount = 1`, `Uniform Endpoint Offset = 0`, `Surface Only = true` (`Surface Expansion Mode = Outside`, `Band Thickness = 0`), **`Hemisphere Z = false`**, `Sphere Distribution = Random`, `Random Seed = 0` |
| Velocity | `Add Velocity` ← `Random Range Vector 001`, **min `(-20, -10, -10)`, max `(-100, 10, 10)`**, `Scale Added Velocity = (1,1,1)` — note min.x > max.x; treat as per-axis lo/hi |
| Size | `Sprite Size Mode = Random Uniform`, min **30**, max **50** |
| Sprite rotation | `Random`, angle min 0, max 360; **`Sprite Rotation Rate` ← `Random Range Float 001`, min −30, max 30** |
| **Sub UV** | `Sub UVAnimation`: `Start Frame = 0`, `End Frame = 3`, `SubUV Loop Count = 1`; renderer `SubUV: 2x2` |
| Init colour | `RGBA(1, 1, 1, 1)`; `Color.Scale Alpha = 1` |
| Dyn params | `Index 0 Param 2 = **5**`, `Param 3 = 0`, `Param 4 = 0`; **`Param3WriteEnabled = true`** |

`Scale Velocity` → `Velocity Scale` (`Vector from Curve`): **X/Y/Z all `(0, 1)C (1, 0.2)C`**.

`Color` (`Color from Curve`) — heavily over-1 (a fire ramp):

| Channel | Keys |
|---|---|
| Red | `(0.0796861, 5)L (0.368246, 3)L (0.738907, 0.250158)L` |
| Green | `(0.0796861, 3.43343)L (0.368246, 0.67227)L (0.738907, 0.00749903)L` |
| Blue | `(0.0796861, 0.115767)L (0.368246, 0.0841786)L (0.738907, 0.00749903)L` |
| Alpha | `(0, 0)L (0.303049, 1)L (0.992454, 0)L` |

`Dynamic Material Parameters → Index 0 Param 1` (**`dissolve`**), `Float from Curve`:
**`(0, -2.46502e-08)C (1, -1)C`** — i.e. 0 → −1 linear-in-effect (the first key is float noise
for zero).

`Uniform Curve Sprite Scale`: `(0, 0.2)C (0.2, 0.75)C (1, 1)C`.

Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Particle State, 4 Color,
5 Dynamic Material Parameters, 6 Scale Sprite Size, 7 Sprite Rotation Rate.

### 5.3 `Smokes` — 5 burst @ t = 0.04 + 200/s, lifetime rand, WORLD space

| Fact | Value |
|---|---|
| Position offset | `(-24.8649, 0, 0)` |
| Lifetime | `Random`; `InitializeParticle.Lifetime Min/Max = **0.3 / 0.4**`; pin override `Random Range Float` min 0.2 / max 0.4 `[unresolved: same ambiguity as §5.2]` |
| Spawn shape | Sphere Location, `Sphere Radius = 20`, **`Non Uniform Scale = (1, 1, 0)`** — a flattened disc in XY, `Surface Only = true` / `Outside`, `Hemisphere Z = false`, `Radius Position = 1`, `V Position = 0.5` |
| Velocity | `Add Velocity` ← `Random Range Vector 001`, **min `(-50, -10, -10)`, max `(-150, 10, 10)`** |
| Size | `Random Uniform`, min **150**, max **200** |
| Sprite rotation | Random 0…360; `Sprite Rotation Rate` ← `Random Range Float 001`, min −30, max 30 |
| Init colour | `RGBA(1, 1, 1, 1)`; **`Color.Scale Alpha = 0.6`** |
| Dyn `Param 2/3` | 0 / 0 |

`Scale Velocity` → `Velocity Scale`: **X/Y/Z all `(0, 1)C (0.2, 0.2)C (1, 0.1)C`**.

`Color` (`Color from Curve`) — note the **duplicated t = 0 keys**, transcribed verbatim:

| Channel | Keys |
|---|---|
| Red | `(0, 0.009134)L (0, 1)C (0.0603682, 1)L (0.22457, 1)L (0.363417, 0.391573)C (0.527618, 0)L` |
| Green | `(0, 0.004025)L (0, 1)C (0.0603682, 0.693872)L (0.22457, 0.040915)L (0.363417, 0.003677)C (0.527618, 0)L` |
| Blue | `(0, 0.006995)L (0, 1)C (0.0603682, 0.147027)L (0.22457, 0.045186)L (0.363417, 0.022174)C (0.527618, 0)L` |
| Alpha | `(0.126773, 1)C (0.514337, 0.35)L` |

`Dynamic Material Parameters`:
- `Index 0 Param 1` (**`dissolve`**): `(0, -2.46502e-08)C (1, -1)C`
- `Index 0 Param 4` (**`core_color`**): **`(0, -1)C (0.25, 1)C`**

`Uniform Curve Sprite Scale`: `(0, 0.2)C (0.2, 0.4)C (1, 1)C`.

Update order: 1 Scale Velocity, 2 Solve Forces and Velocity, 3 Particle State, 4 Color,
5 Dynamic Material Parameters, 6 Scale Sprite Size, 7 Sprite Rotation Rate.

### 5.4 `FirstGlow` — 1 burst @ t = 0.05 + 3/s, lifetime 1 s, LOCAL space

| Fact | Value |
|---|---|
| Position offset | `(-17.223, 0, 0)` |
| Init colour | `RGBA(0.102242, 0.658375, 1, 0.2)`; **`Color.Scale Alpha = 0.1`** |
| Size | Uniform **500** |
| Dyn params | `Index 0 Param 1 = 1`, `2/3/4 = 0` |

`Scale Sprite Size` (both curves):
- `Uniform Curve Sprite Scale`: `(0, 0.5)C (0.1, 0.9)C (1, 1)C`
- `Non-Uniform Curve Sprite Scale`: `X: (0, 1)C (1, 0.4)C | Y: (0, 1)C (1, 0)C`

`Color` (`Color from Curve`):

| Channel | Keys |
|---|---|
| Red | `(0, 1)C (0.13281, 1)L (0.322366, 1)L (0.96227, 0.913099)L` |
| Green | `(0, 0.938686)C (0.13281, 0.752942)L (0.322366, 0.341915)L (0.96227, 0.0241576)L` |
| Blue | `(0, 0.791298)C (0.13281, 0.109462)L (0.322366, 0.109462)L (0.96227, 0.0241576)L` |
| Alpha | `(0, 0)C (0.276487, 1)L (0.67371, 1)L (1, 0)C` |

Update order: 1 Particle State, 2 Dynamic Material Parameters, 3 Scale Sprite Size, 4 Color.

### 5.5 `Projectile_01 / _02 / _03` — the core, 1 sprite each, lifetime **10 s**, LOCAL, VelocityAligned

All three are structurally identical: burst 1 at t = 0, position offset `(-20.1693, 0, 0)`,
`Add Velocity` `Velocity = (0.01, 0, 0)` with `Scale Added Velocity = (1,1,1)` — a **nominal +X
velocity whose only job is to give the velocity-aligned renderer an axis**, `Sprite Size Mode =
Non-Uniform`, `Lifetime Mode = Direct Set` **10**.

| | `Projectile_01` | `Projectile_02` | `Projectile_03` |
|---|---|---|---|
| `Sprite Size` | **`(50, 50)`** | **`(250, 500)`** | **`(100, 400)`** |
| Init colour | `RGBA(1, 0.672443, 0.376262, 1)` | `RGBA(1, 0.205079, 0.0168074, 0.3)` | `RGBA(0.130136, 0.00477695, 0.00477695, 0.5)` |
| Dyn `Param 1` | **0** | **3** | **2** |
| Material | `Part04` | `Part01` | `Part01` |

**No colour curve, no size curve, no alpha curve executes on any of the three.** Both
`Scale Sprite Size` modules on each emitter are **DISABLED**; their authored curves are recorded
here only so a future session does not re-derive them:

- `Scale Sprite Size` *(disabled)*: `Uniform: (0, 0)C (0.1, 1)C (1, 0)C`;
  `Non-Uniform: X: (0, 0)L (1, 1)L | Y: (0, 0)L (1, 1)L`
- `Scale Sprite Size 001` *(disabled)*: `Uniform: (0, 0)L (1, 1)L`;
  `Non-Uniform: X: (1, 1)L | Y: (0, 1)C (1, 0.6)C`

Update order on each: 1 Solve Forces and Velocity, 2 Particle State, 3 Dynamic Material Parameters,
4 Scale Sprite Size *(disabled)*, 5 Scale Sprite Size 001 *(disabled)*.

These three sprites are **static, permanent, velocity-aligned quads** at fixed size and colour —
the fireball's hard core. All the motion in the effect is in the flames, smoke and trails around them.

### 5.6 `Trail_01` / `Trail_02` — ribbons, 50/s, lifetime 0.25 s, WORLD space

Identical except for the sign of the curl-noise strength:

| Fact | Value |
|---|---|
| Spawn | `Spawn Rate = 50` (no burst) |
| Initialize Ribbon | `Color Channel Mode = Link RGBA`, `Color Mode = Direct Set`, colour `RGBA(1, 1, 1, 1)`; `Lifetime Mode = Direct Set` **0.25**; `Ribbon Width Mode = Direct Set`, **`Ribbon Width = 8`**; `Position Offset = (0,0,0)`; module-level **`Color.Scale Alpha = 0.3`** |
| `Add Velocity` | **`Velocity = (-1000, 0, 0)`**, `Scale Added Velocity = (1,1,1)` — the trail streams **−X**, i.e. backwards |
| `Curl Noise Force` | `Noise Frequency = 3`, **`Noise Strength = 5000`** (`Trail_01`) / **`−5000`** (`Trail_02`), `Random Seed = 11`, `Randomization Vector = (0.65, 0.125, 0.37)`, `Pan Noise Field = (0,0,0)`, `Cone Mask Angle = 45`, `Cone Mask Falloff Angle = 45` |
| `Scale Velocity` | **DISABLED** — authored `X: (0,1)L (1,1)L \| Y: (0,1)L (1,1)L \| Z: (0,1)C (0.25,-1)C (0.5,1)C (0.75,-1)C (1,1)C` |

`Color` (`Color from Curve`) — identical on both, over-1 RGB:

| Channel | Keys |
|---|---|
| Red | `(0, 2)C (0.161787, 2)L (0.359795, 1)L (0.96227, 0.913099)L` |
| Green | `(0, 1.87737)C (0.161787, 1.50588)L (0.359795, 0.341915)L (0.96227, 0.024158)L` |
| Blue | `(0, 1.5826)C (0.161787, 0.218924)L (0.359795, 0.109462)L (0.96227, 0.024158)L` |
| Alpha | `(0, 1)C (1, 0)C` |

`Scale Ribbon Width` → `Ribbon Width Scale` (`Float from Curve`): **`(0, 1)C (1, 0)C`** — the
ribbon tapers linearly from width 8 to 0 over its 0.25 s.

Update order on both: 1 Scale Velocity *(disabled)*, 2 Curl Noise Force (`001` on `Trail_02`),
3 Solve Forces and Velocity, 4 Particle State, 5 Color, 6 Scale Ribbon Width.

**The two ribbons are a mirrored pair** — equal-and-opposite curl noise on the same seed, producing
two counter-twisting streamers. That symmetry is the read; dropping one changes the effect.

### 5.7 A dynamic-parameter write with no reader

`Trail_01/02` set `DynamicMaterialParameters.Index 0 Param 2/3/4 = 0` and
`FloatFromCurve.Scale Curve = 1`, but **`M_VFX_FlatAdd` declares no dynamic parameters** (§4.2).
The writes are inert. Do not plumb them.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence — this effect does not fit the template model

The cadence table (`ck::particles::Get_TemplateSpecs()`) expresses **one** of: a continuous
spawn-rate stack (`BurstCount = 0`), or an instantaneous burst of N per loop. This system needs
**both at once, at four different rates, on a 10-second loop**:

| Requirement | Value |
|---|---|
| Loop duration | **10 s** (live — `Life Cycle Mode = Self`, §2) |
| Longest particle lifetime | **10 s** (`Projectile_01/02/03`) |
| Burst at loop start | 13–15 particles across 4 emitters, at t = 0 / 0.04 / 0.05 |
| Sustained sprite rate | **408 /s** (5 + 200 + 200 + 3) |
| Sustained ribbon rate | **100 /s** (50 + 50) |

A 10-second loop at 408 sprites/s is **~4080 live sprites at steady state**, ignoring the ribbons.
That is a different order of magnitude from every existing row (largest burst today: 96).

**Honest reading: this is not a "recreate as one behavior" job.** The three `Projectile_*` core
sprites plus one glow are a small, cheap, high-value recreation; the 200/s flames and 200/s smoke
are a continuous-rate emitter pair that needs its own row; the ribbons need a renderer class that
does not exist. Scope it as: **core + glow now, flames/smoke as a second row, trails behind a
ribbon capability.**

### 6.2 VisTag / renderer needs

| Source emitter | Renderer | Shared set covers it? |
|---|---|---|
| `Projectile_01/02/03` | velocity-aligned sprite, three different materials | **VisTag 1** is the shared velocity-aligned sprite but binds one material; row-declared `VelocityAlignedSprite` (the NS_BasicAttack capability) covers it — **3 row renderers** |
| `SecondGlow`, `FirstGlow` | camera-facing sprite, 2 materials | needs the **new `CameraFacingSprite` row kind** (gap 1) |
| `Smokes` | camera-facing sprite | same |
| `Flames01` | camera-facing sprite **with SubUV 2×2 flipbook** | **NO — gap 2** |
| `Trail_01/02` | **ribbon** | **NO — gap 3** |

### 6.3 Mesh needs

**None.** This is the only sheet in the batch with no mesh dependency at all.

### 6.4 Look / texture needs

| Look (proposed) | Family entry point | Notes |
|---|---|---|
| `FbProjCoreDisAdd04` | `CkUsf_Look_DissolveAdd` (existing) | `Part04` parameterization |
| `FbProjCoreDisAdd01` | existing | `Part01` reference values; serves `Projectile_02`, `Projectile_03`, `FirstGlow` |
| `FbProjGlowDisAdd03B` | existing | `Part03_Bright`; needs **`CamOffset` 50** plumbed |
| `FbFlamesDisAdd01` | existing | first real use of `DistortIntensity` 0.5 + `DissolveBias` −0.1 + `DissolveScale` (2,2) + `DistortScale` (2,2) + `DistortSpeed` (−0.3,−0.3) + `Glow_Intensity` 2 |
| `FbSmokeDisAdd01` | existing | `DistortIntensity` 0.4, `DistortSpeed` (0.1,0.1), `OpacityBoldness` **3**, `GradientMap_Displacement` 0.75, and **two different shape textures** (§4.1 note) |
| `FbTrailFlatAdd` | **new** — a tiny `M_VFX_FlatAdd` family shader | `ParticleColor × Brightness`, depth-faded; 5 minutes of HLSL, blocked only by the ribbon renderer |

**Family parameters not plumbed today that this effect needs:** `CamOffset` (50 on
`Part03_Bright`), `Glow_Intensity` (2 on `Flames01`), `Core_Intensity` (1 on `Flames01`/`Smoke01`),
and a **separate `Main_Tex` vs `Color_Tex`** slot for `Smoke01`.

Textures: `SoftParticle` / `SparkStreak` / `TileNoise` already cover `T_VFX_Part_01` /
`T_VFX_Part_04` / `T_VFX_Noise_02`. New bakes: `T_VFX_Part_03`, `T_VFX_Cloud_04`, `T_VFX_Cloud_05`,
`T_VFX_Noise_07`; measure `T_VFX_Wind_01` and `T_VFX_Noise_04` against the existing `WindBand` and
`TileNoise` bakes before assuming reuse. **A 2×2 sub-UV flipbook atlas is a new bake *shape***
(four sub-images in one texture), not just a new function — see gap 2.

### 6.5 Capability gap callout

| # | Gap | Severity |
|---|---|---|
| 1 | **No row-declared camera-facing sprite kind.** `FCk_ParticlesRendererSpec` has only `Mesh` and `VelocityAlignedSprite`. Four emitters here are `Unaligned`/`FaceCamera` with four different materials. Additive fix; shared with every other sheet in this batch. | **BLOCKING** |
| 2 | **No sub-UV / flipbook support anywhere in CkParticles.** `Flames01` renders `SubUV: 2x2` and drives it with a `Sub UVAnimation` module (`Start Frame 0`, `End Frame 3`, `SubUV Loop Count 1`). The DI writes no sub-image index, the renderer builder sets no `SubImageSize`, and the texture generator bakes no atlases. Without this the flames are a **static** sprite instead of a 4-frame animation. | **BLOCKING** for `Flames01` |
| 3 | **Ribbon renderer does not exist.** Same as [NS_Bomb_Projectile.md](NS_Bomb_Projectile.md) gap 1: no ribbon in the DI, no ribbon kind in the renderer spec, and CkUsf deliberately ships **no** Niagara ribbon usage flag (NS_Lightning_Range §9). `Trail_01/02` are a mirrored pair and are a large part of what makes this read as a fireball. | **BLOCKING** for the trails |
| 4 | **Mixed burst + continuous-rate cadence in one system, at four different rates, on a 10 s loop with ~4080 live sprites at steady state** (§6.1). The cadence table expresses one rate OR one burst per row. Needs either multiple behaviors composed at one transform (the pattern `CkParticles/CLAUDE.md` already recommends for spells/trails) or a genuinely new cadence shape. | **BLOCKING** as one behavior; **solvable** by composition |
| 5 | **Curl Noise Force** (`Trail_01/02`, strength ±5000, frequency 3, seed 11). Not a pipeline gap — it is behavior math a `.ush` can implement — but it must stay bit-identical between the GPU `.ush` and the C++ mirror, and curl noise is a non-trivial function to keep in lockstep. Flag it as a *cost*, not a blocker. | Medium |
| 6 | **World space on 4 of 9 emitters** (`Flames01`, `Smokes`, `Trail_01/02`) while 5 are local. The template is local-space. **For a projectile this is the worst case**: the whole point of a world-space trail is that it stays behind a moving emitter. NS_BasicAttack §13.2 records this deviation as "visible only if the spawning actor MOVES" — here it *always* moves. | **BLOCKING** for fidelity of the trailing half |
| 7 | **`Smoke01` uses different `Main_Tex` and `Color_Tex`**; the CkUsf family collapses them into one `ShapeTex`. Small, real. | Low |
| 8 | **`CamOffset`, `Glow_Intensity`, `Core_Intensity` unplumbed** in the CkUsf DissolveAdd family. | Medium |
| 9 | **`Opacty_DepthFade`** 20 / 30 across instances; CkUsf surface looks do not wire scene depth (known gap, NS_Lightning_Range §13.4). | Low (known) |
| 10 | **Two `[unresolved]` lifetime ranges** (§5.2, §5.3): `Lifetime Mode = Random` with `Lifetime Min/Max` on the module *and* a `Random Range Float` override on the `Lifetime` pin, carrying different numbers. Resolve before implementing. | Prerequisite |

**Complexity tier: L.** Three independent capabilities are missing (camera-facing row sprite,
sub-UV flipbooks, ribbons), the cadence needs composition rather than a row, and world-space
trailing is exactly the case the local-space template cannot fake. A **useful S-tier subset exists**
— the three `Projectile_*` core sprites plus `SecondGlow`/`FirstGlow`, which is 6 of 9 emitters and
needs only gap 1 — and that subset is the right first delivery.

### 6.6 Behavior id

**Do not allocate an id in this document.** Take the next free id from
`ck::particles::NumBehaviors` at implementation time, bump it, and update the roster paragraph in
`CkParticles/CLAUDE.md`. Five sibling sheets were written in the same pass and none allocates an id
— allocate them in one ordered pass so they cannot collide. If this effect ships as two behaviors
(core + flames/smoke, §6.1), it consumes two ids.

---

## 7+. Reserved for implementation

Sections 7–14 of the recipe schema ([README.md](README.md)) are intentionally absent and are to be
written by the implementation session, from what actually happened.
