# Recipe: NS_FireBall_Projectile → CkParticles (PRE-IMPLEMENTATION TRANSLATION SHEET)

Schema and evidence-tag conventions: [README.md](README.md).
Exemplars this sheet copies: [NS_BasicAttack.md](NS_BasicAttack.md) §1–6, [NS_Lightning_Range.md](NS_Lightning_Range.md) §1–5.

## Completion state — READ FIRST

**Status: IMPLEMENTATION-COMPLETE (2026-08-02, Phase 3 batch F). Behavior id 36. Not yet A/B'd.**

`Behavior_FireBallProjectile.ush` + `ExecuteStage_CPU` case 36, the
`PS_CkParticles_Template_FireBallProjectile` cadence row (10 s / 10 s / burst 15 / rate 408 per second
**plus a ribbon emitter at 100 points per second** — the cookbook's first), one new CkUsf look
(`TrailFlatAdd`, the first to opt into `_UsedWithNiagaraRibbons`), zero new textures, zero new meshes,
`Test_Particles_FireBallProjectileBehavior.cpp`, and a VfxExamples gym pair. **Nothing has been rendered
or visually compared** — §12 is open.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_FireBall_Projectile` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior id | **36** (`FireBallProjectile`) |
| Recreation status | implementation-complete; `[HUMAN-VERIFY]` open |

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

Burst particles at loop start: **15** (1 + 5 + 5 + 1 + 1 + 1 + 1; `Smokes`'s 5 land at t = 0.04,
`FirstGlow`'s 1 at t = 0.05). *(The sheet stated 13 beside its own itemization and then caught itself
mid-sentence — corrected to 15 at implementation, [P3-F1].)* Sustained rate afterwards:
**408 sprites/s** (5 + 200 + 200 + 3) plus **100 ribbon points/s** (50 + 50). Bounds `Dynamic`
everywhere, `Determinism: false`.

> **`Life Cycle Mode = Self` on all 9 emitters `[corpus]` — this system is the batch's exception.**
> The per-emitter Loop Behavior / Loop Duration values below are therefore **live**, not leftovers.
> [P0-D1]'s "the system rows rule" does NOT apply here — it is scoped to system-governed emitters.
> For the record, the system's own rows `[corpus-v3]` are **`Loop Behavior = Infinite`,
> `Loop Duration = 10.0 s`, `Loop Delay = 0`, `Inactive Response = Complete`**, which AGREES with the
> per-emitter 10 s; the two readings coincide, so nothing moves.
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

`Scale Sprite Size` carries both curves, but `Scale Sprite Size Mode = **Uniform Curve**`, so only the
first is live and the non-uniform pair is INERT `[corpus]` *(the mode was not stated — [P3-F2], the [P2-E5]
class)*:
- `Uniform Curve Sprite Scale` — **LIVE**: `(0, 0.5)C (1, 1)C`
- `Non-Uniform Curve Sprite Scale` — *inert*: `X: (0, 1)C (1, 0.4)C | Y: (0, 1)C (1, 0)C`

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
| Lifetime | `[corpus-v3]` `Lifetime Mode = Random` ⇒ **`Lifetime Min/Max = 0.2 / 0.3` DRIVES** (`lifetimeResolved.source = minmax`); the `Random Range Float` override (0.2 / 0.4) sits on the unselected Direct-Set pin and is INERT. *Was read as "the pin override normally wins, giving 0.2–0.4"; corrected per [P0-D2].* |
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
| Lifetime | `[corpus-v3]` `Random` ⇒ **`Lifetime Min/Max = 0.3 / 0.4` DRIVES**; the 0.2 / 0.4 override is INERT ([P0-D2], same resolution as §5.2). |
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

`Scale Sprite Size` carries both curves, but `Scale Sprite Size Mode = **Non-Uniform Curve**` — the
opposite of `SecondGlow` — so the uniform curve is INERT `[corpus]` *(the mode was not stated — [P3-F3])*:
- `Uniform Curve Sprite Scale` — *inert*: `(0, 0.5)C (0.1, 0.9)C (1, 1)C`
- `Non-Uniform Curve Sprite Scale` — **LIVE**: `X: (0, 1)C (1, 0.4)C | Y: (0, 1)C (1, 0)C`
  (the quad squeezes to a 200-unit-wide horizontal sliver by the end of life)

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
| Loop duration | **10 s** (live — `Life Cycle Mode = Self`, §2; the system's own `Infinite / 10.0 s` agrees `[corpus-v3]`) |
| Longest particle lifetime | **10 s** (`Projectile_01/02/03`) |
| Burst at loop start | 13–15 particles across 4 emitters, at t = 0 / 0.04 / 0.05 |
| Sustained sprite rate | **408 /s** (5 + 200 + 200 + 3) |
| Sustained ribbon rate | **100 /s** (50 + 50) |

A 10-second loop at 408 sprites/s is **~4080 live sprites at steady state**, ignoring the ribbons.
That is a different order of magnitude from every existing row (largest burst today: 96).

**SUPERSEDED at implementation (Phase 3, batch F).** The reading above predates C2 (a row may declare a
burst AND a continuous rate on one emitter), C5 (the spawn-phase split that tells the two populations
apart) and C6a (a ribbon-bearing row's SECOND emitter). All nine emitters land on ONE row and ONE
behavior: `PS_CkParticles_Template_FireBallProjectile` at 10 s / 10 s / burst 15 / rate 408 per second,
with a ribbon emitter at 100 points per second. The `~4080 live sprites` figure above is the
RECREATION's allocation, not the source's — the source's own steady state is ~126 live sprites, because
every emitter there carries its own short lifetime where the template gives all of them the row's 10 s.
That cost is recorded in §13.

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
| 10 | ~~Two `[unresolved]` lifetime ranges~~ — **RESOLVED `[corpus-v3]`** (§5.2, §5.3): `Lifetime Mode = Random` ⇒ the module's `Lifetime Min/Max` drives on both ([P0-D2]); `Flames01` 0.2–0.3, `Smokes` 0.3–0.4. | Closed |

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

## 7. Textures — ZERO new bakes

Every paint this port needs was measured and baked by an earlier batch, and every one of them is reached
through a look that already binds it (§10):

| Source texture | Existing bake | Reached through |
|---|---|---|
| `T_VFX_Part_01` | `SoftParticle` | `PartDisAdd01` |
| `T_VFX_Part_03` | `SoftParticleFine` | `PartDisAdd03Bright` |
| `T_VFX_Part_04` | `SparkStreak` | `PartDisAdd04` |
| `T_VFX_Wind_01` | `WindSheet` (2×2) | `FlamesDisAdd01` |
| `T_VFX_Cloud_05` | `Cloud05` | `SmokeDisAdd01` |
| `T_VFX_Noise_02` | `TileNoise` | `PartDisAdd01` / `PartDisAdd04` distortion |
| `T_VFX_Noise_04` | `TileNoiseCoarse` | `FlamesDisAdd01` dissolve |
| `T_VFX_Noise_07` | `TileNoiseBanded` | `SmokeDisAdd01` dissolve |
| `T_VFX_WhitePixel` | `LutWhite` | the family's inert gradient default |

**§4.3's "new bakes needed" list is discharged, not skipped.** It named `T_VFX_Part_03`, `T_VFX_Cloud_04`,
`T_VFX_Cloud_05` and `T_VFX_Noise_07`; batches A and E measured and baked all four (as
`SoftParticleFine`, `Cloud04`, `Cloud05` and `TileNoiseBanded`) for the FireBall_Hit / FireBall_Cast ports,
which draw the **same material instances** this system does. Reuse here is therefore not an approximation —
it is the same paint through the same look. The two candidates §4.3 flagged for measurement,
`T_VFX_Wind_01` and `T_VFX_Noise_04`, were likewise resolved by batch A rather than assumed.

The trail's own material carries **no textures at all** (§4.2), so the ribbon adds none either.

---

## 8. Mesh

**None.** This is the only port in the cookbook with no mesh dependency of any kind: two sprite quads
(camera-facing and velocity-aligned) and one ribbon.

---

## 9. The behavior — `Behavior_FireBallProjectile.ush` + `ExecuteStage_CPU` case 36

### 9.1 One row, two emitters, three populations

The row is `PS_CkParticles_Template_FireBallProjectile`: **loop 10 s / lifetime 10 s / burst 15 / rate
408 per second**, plus a **ribbon emitter at 100 points per second**. Three populations reach the
behavior and each is selected by data it already receives:

| Population | Selector | Source |
|---|---|---|
| Ribbon trail | `CkParticles_IsRibbonSeed(Seed)` | the ribbon emitter's graph adds `RibbonSeedBase` to its `UniqueID` |
| Burst layer | `SpawnPhase ≈ 0`, then `Seed % 15` | the four burst emitters' own counts |
| Rate layer | `SpawnPhase ≠ 0`, then a weighted draw on `Rand(Seed, 0)` | the four Spawn Rates, 5 / 200 / 200 / 3 |

The burst partition is exact by construction (burst `UniqueID`s are sequential, so one modulus period
carries 1 SecondGlow, 5 Flames01, 5 Smokes, 1 FirstGlow and one of each `Projectile_*`). The rate draw is
weighted rather than a modulus, because a modulus cannot express a 3-per-second layer against a
200-per-second one.

**No layer here needs a WINDOW.** Every source emitter's `Life Cycle Mode = Self` loop duration is 10 s —
the row's own — so nothing streams for a slice of the loop the way the Cast ports' emitters do. The only
timing structure is the two burst **beats**: `Smokes` at 0.04 s and `FirstGlow` at 0.05 s. Those belong to
the `Spawn Burst Instantaneous` module, so they apply to the burst population **only** and a streamed
particle of the same layer takes no delay.

### 9.2 The trail: what the ribbon particles actually do

The source's `Trail_01` / `Trail_02` are not event-spawned (`eventHandlers: []` on all nine emitters
`[corpus]`), so C6c's leader-path collapse is not needed here at all. They are ordinary rate-spawned
particles with:

- `Add Velocity = (-1000, 0, 0)` — enabled, world space,
- a `Curl Noise Force` of strength **+5000** / **−5000** at frequency 3, seed 11,
- `Scale Velocity` **DISABLED**, so the backward run is exactly linear,
- lifetime 0.25 s, ribbon width 8 tapering to 0, alpha scaled by 0.3.

That means the streamer is **self-driven** and is reproduced exactly at a stationary spawn point: the
ribbon links its particles in spawn order (Niagara's default `bLinkOrderUseUniqueID`), the youngest sits
at the head and the oldest 250 units back, and the two mirrored curls twist them apart. This is why the
FireBall trail renders at the A/B pedestal and the Bomb trail does not.

**The two ribbons are ONE renderer and two ribbon IDs.** They share `M_VFX_FlatAdd` and every curve; only
the curl sign differs. `RibbonIdBinding` reads `Particles.MeshIndex`, so the behavior writes
`LocalSeed % 2` there and Niagara links two independent streamers out of one particle soup — which is
[P3-D1] option (c) doing the job it was ruled in for.

### 9.3 Curl conversion — derived, not tuned

Same two-step conversion the NS_DebuffCast port established (§9.3 there), with one simplification:

- **Frequency** — the module authors its field in metres, so the source's `3` becomes `0.003` per unit
  `[inferred]`.
- **Strength** — the source figure is an ACCELERATION where `CkParticles_CurlPath` advects with a
  VELOCITY. DebuffCast's layers crushed that acceleration through their own `Scale Velocity` plateau;
  **this layer's Scale Velocity is DISABLED**, so there is no plateau and the conversion is the bare one:
  mean displacement over a life `L` under constant acceleration `A` is `0.5·A·L²`, so the constant-velocity
  path covering the same ground is `0.5·A·L`. That is divided by the **measured** mean magnitude of this
  plugin's own curl field over the region the trail visits — **0.8139**, over 4000 samples within ±300
  units at frequency 0.003 and seed 11.

Net: `Strength = ±0.5 × 5000 × 0.25 / 0.8139 = ±767.9` units/s, giving a curl displacement of ~27 units
on one streamer and ~94 on the other by end of life, against the 250-unit linear run. The asymmetry is
real and expected — the two paths advect into different parts of the field, so the mirror is near-exact
early (cosine −0.92 at 0.05 s) and only approximate later (−0.67 at 0.25 s).

### 9.4 The dead tail is frozen, not streaming

The row gives every particle a 10 s lifetime because the three `Projectile_*` cores need it. A trail point
whose 0.25 s is up therefore survives another 9.75 s. It is hidden (width 0, colour 0) **and** its position
is evaluated at a clamped age, so the ribbon's dead tail stops where it died instead of streaming 10 000
units down −X and dragging the ribbon's bounds with it. Same for the sprite layers: `Hidden()` zeroes
colour, size and scale.

### 9.5 What the sheet's inert curves cost

Three of §5's curves are authored and never run, and each would change the read if transcribed anyway:

- `SecondGlow`'s non-uniform curve (mode is **Uniform Curve** — [P3-F2]) would squash the glow to a
  sliver; the live uniform curve only grows it 0.5 → 1.
- `FirstGlow`'s uniform curve (mode is **Non-Uniform Curve** — [P3-F3]) would hold a 500-unit square where
  the live pair squeezes it to 200 × 0.
- All six `Scale Sprite Size` modules on the three `Projectile_*` emitters are DISABLED, so not one curve
  runs on the fireball's core. A "pop then fade" reading of them would animate the one thing in this
  system that is deliberately static.

---

## 10. Looks and renderers

**Six row renderers on the main emitter, one on the ribbon emitter. ZERO new looks on the main emitter —
one new look for the trail.**

| VisTag | Kind | Look | Source emitter |
|---|---|---|---|
| 157 | VelocityAlignedSprite | `PartDisAdd04` | `Projectile_01` |
| 158 | VelocityAlignedSprite | `PartDisAdd01` | `Projectile_02`, `Projectile_03` |
| 159 | CameraFacingSprite | `PartDisAdd03Bright` | `SecondGlow` |
| 160 | CameraFacingSprite | `PartDisAdd01` | `FirstGlow` |
| 161 | CameraFacingSprite, 2×2 | `FlamesDisAdd01` | `Flames01` |
| 162 | CameraFacingSprite | `SmokeDisAdd01` | `Smokes` |
| 163 | **Ribbon** | `TrailFlatAdd` *(new)* | `Trail_01` + `Trail_02` |

Every reused look was checked value-by-value against §4.1's delta table before reuse and is an **exact**
match — not a near miss — because this system draws the *same material instances* (`M_VFX_DisAdd_Part01`,
`Part03_Bright`, `Part04`, `Flames01`, `Smoke01`) the FireBall_Hit / FireBall_Cast ports already carry.
`PartDisAdd01` appears twice because the source draws it on two different renderer classes; per the
batch-E rule that is two ROW RENDERERS, but one look — the usage flags are the same (`Sprites`) for both,
unlike the `LightStripDisAddSprite` case where one was a mesh.

**`TrailFlatAdd` is the cookbook's first ribbon-drawn look.** It is `M_VFX_FlatAdd` used *directly* — the
parent graph, not an instance — so `Brightness = 1`, against `FlatAdd02`'s 10. It opts into
`_UsedWithNiagaraRibbons`, the third independent usage flag; a master without it draws as the engine
default under a ribbon renderer, which is why the template builder refuses to emit the renderer in that
case.

---

## 11. Tests

`Test_Particles_FireBallProjectileBehavior.cpp` — `CkTests.UnitTests.CkParticles.FireBallProjectileBehavior`.
It drives the CPU mirror only (no Niagara, no RHI, no forked engine) and asserts:

- the cadence row's four numbers AND the itemization they are derived from (per-emitter bursts sum to 15,
  per-emitter rates sum to 408), plus the ribbon emitter's rate, its single `Ribbon`-kind renderer, its
  look name and its VisTag;
- the burst partition reproduces every source emitter's own count exactly;
- each layer reaches its own renderer;
- the rate stream's per-layer share against the source's rate share (bar 0.004; measured worst deviation
  **0.00076**), and that the three `Projectile_*` layers never appear in it;
- both burst **beats**, each against its own opposite, plus the dead control that a *streamed* particle of
  the same layer takes no delay;
- colour ramps at BOTH ends and a point in between, at corpus-derived values with a 1e-4 tolerance —
  `SecondGlow` R 2.0 → 3.0 (2.5265734 at t = 0.8), `Flames01` R 5.0 → 3.0 (4.1661088 at t = 0.2),
  `Smokes` alpha 0.6 → 0.21, `FirstGlow` alpha 0 → 0.1 and its blue 0.7912982 → 0.1094617;
- `FirstGlow`'s live non-uniform size curve (200 × 0 at end of life), which a uniform-curve reading fails;
- the three cores' exact size, colour, dissolve and VisTag at three ages, and their death at 10 s;
- every `Flames01` particle plays all four sub-UV frames (**300/300**);
- **the seed bank, both ways**: no main-bank id reaches VisTag 163 over 5000 seeds, every ribbon-bank id
  reaches it and alternates `MeshIndex` 0/1, and a main-bank id's population is unaffected;
- the trail's width taper 8 → 0, its alpha 0.3 → 0, its hidden-and-FROZEN state past 0.25 s;
- **the mirrored pair**: the two trails' curl offsets point to opposite sides at three ages (cosine < 0)
  and are near-exactly opposite early (cosine < −0.9). A single-signed curl, or a dropped second ribbon,
  fails this outright.

`Test_Particles_RosterSanity.cpp` covers the rest generically: `NumBehaviors` 38, the template route, the
VisTag ceiling (now walking ribbon renderers), and the emitter-clock rule — behavior 36 rides a burst+rate
row, so it is REQUIRED to move with the clock.

---

## 12. Verification — A/B protocol

`[HUMAN-VERIFY]` Open the **VfxExamples** gym, cycle to **FIREBALL PROJECTILE**, and run
`Ck_GymVfxExamples_RestartAll`. In order:

a. **The core reads as a single hard object**, not three sprites. Three velocity-aligned quads
   (50×50 warm, 250×500 orange, 100×400 dark red) stacked at the same point, static for the whole 10 s.
b. **Two blue glows behind it** — a small one pulsing to a 3× cyan-white peak late in its half-second, and
   a large faint halo that squeezes to a horizontal sliver as it fades.
c. **A continuous fire wash streaming backwards**, brightest right at the core, with visible 4-frame
   flipbook motion inside individual puffs.
d. **Smoke behind the fire**, larger, greyer, thrown further and braking harder.
e. **TWO thin bright streamers** trailing to −X and twisting to opposite sides of the axis. This is the
   port's headline; if there is only one, or if they overlap without separating, the ribbon id is not
   reaching `RibbonIdBinding`.
f. Overall brightness: the whole thing should read hotter than the original ONLY if `Opacty_DepthFade` is
   the culprit (§13) — otherwise the two should sit at the same exposure.

## 13. Confirmed fidelity differences or intentional deviations

**The visual gate in §12 has NOT been executed.** Everything below is a *known* difference derived from
the source data, or *unverified*.

### Known differences — deliberate

1. **Population cost: the recreation allocates ~32× the source's live sprites.** The template gives every
   particle the ROW's lifetime, and this row's is 10 s because the three cores need it. At 408/s that is
   **4080 live sprites** where the source's own per-emitter lifetimes (0.2–1.0 s) give it **~126**; the
   ribbon emitter likewise allocates 1000 points where the source holds ~25. Every surplus particle is
   hidden (colour, size, scale zero; ribbon width zero) and frozen in place, so the *image* is right — the
   cost is GPU allocation and ribbon tessellation, not pixels. This is the same class as HealCast's
   "~14 of 100 streamed per loop", scaled up by the 10 s row. A per-emitter lifetime on the ribbon spec
   would fix half of it and is not a capability the cookbook has.
2. **World space on 4 of 9 emitters** (`Flames01`, `Smokes`, `Trail_01/02`) — the C12 non-goal. At a
   stationary pedestal the local and world frames coincide, so the difference is unobservable here;
   attached to a MOVING spawner the source's fire, smoke and trails would be left behind in world space
   while ours follow rigidly.
3. **`CamOffset = 50` on `Part03_Bright` is not plumbed.** The family's camera-toward world-position push;
   `SecondGlow` is the only layer in this system that drives it. Recorded by the `PartDisAdd03Bright` look
   itself since batch A.
4. **`Glow_Intensity` (2 on `Flames01`) is folded into Brightness** — `FlamesDisAdd01` carries 20, i.e.
   10 × 2. `Core_Intensity` (1 on `Flames01` and `Smoke01`) is NOT plumbed, nor is `Color_CoreDifferent`.
5. **`Smoke01`'s separate `Main_Tex` and `Color_Tex`** (`T_VFX_Cloud_05` vs `T_VFX_Cloud_04`) collapse into
   the family's one `ShapeTex` slot; the shape carries `Main_Tex`. Recorded by the look since batch A.
6. **`Opacty_DepthFade`** (20 / 30 across instances, 0 on the trail's `M_VFX_FlatAdd`) is not wired —
   CkUsf surface looks have no scene-depth input. Visible only where a sprite intersects geometry.
7. **The curl field is sampled along the curl-only path**, not along the true path that includes the
   −1000 units/s backward run. `CkParticles_CurlPath` advects from the spawn position through the field
   alone, which is the shared stateless helper's contract and the same approximation NS_DebuffCast
   records. Here the linear run (250 units) is an order of magnitude larger than the curl displacement, so
   the two streamers sample a *narrower* slice of the field than the source's particles would.
8. **The mirror is only near-exact early.** Equal-and-opposite strength does not give equal-and-opposite
   displacement once the two paths advect into different field regions — measured cosine −0.92 at 0.05 s,
   −0.67 at 0.25 s, with magnitudes 27 and 94 units. The source has exactly the same property (its two
   emitters are also nonlinear advections of one field), so this is a *shape* difference, not a structural
   one, but the two are not guaranteed to diverge identically.
9. **`Trail_01/02`'s dynamic-parameter writes are dropped.** `M_VFX_FlatAdd` declares no dynamic
   parameters (§5.7), so nothing reads them.
10. **The recreation loops; the source completes.** Same as every projectile port — the row loops its 10 s
    forever while the source's `Inactive Response = Complete` lets it die. The A/B harness re-arms the
    original on `OnSystemFinished`, so the two pedestals stay in phase.
11. **The source's colour curve on `Smokes` carries two keys at t = 0**; the second wins for every
    reachable `t`, and the cookbook's `KeyN` family cannot express a zero-width segment, so the first is
    dropped. The only value that differs is at exactly t = 0.

### Unverified

- Every visual criterion in §12. Nothing has been rendered.
- Whether 4080 allocated particles per instance is acceptable in a real scene. It has not been profiled.
- Whether the curl conversion's field-mean divisor lands the streamers at the source's amplitude. The
  arithmetic is confirmed; the perceptual result is not.

---

## 14. Reusable lessons

1. **A sheet that says "not one behavior" may just predate the capability.** §6.1's three-way split was
   correct against the Phase-0 pipeline and wrong against the Phase-3 one. Re-read a plan section against
   the capability matrix before executing it, and rewrite it in place when it is superseded.
2. **`Scale Sprite Size Mode` decides which of the two authored curves is live, and the two emitters here
   choose OPPOSITE modes.** Transcribing both, or guessing one, changes the silhouette. Any sheet §5 that
   lists two size curves without naming the mode is under-specified — the [P2-E5] class, now seen three
   times.
3. **`eventHandlers` being empty is itself evidence.** C6c's leader-path collapse exists for event-spawned
   trails; a rate-spawned trail with its own velocity needs none of it, and reaching for the machinery
   anyway would have replaced a faithful self-driven streamer with a synthetic one.
4. **Two ribbons with one material are one renderer and two ribbon ids.** `RibbonIdBinding` is the cheap
   separator; a second renderer would have cost a second VisTag and a second master for no gain.
5. **A row lifetime chosen for the longest layer is paid by every layer.** When a row's lifetime is set by
   a 10 s core, a 0.25 s trail point survives 40× longer than it should. Hiding it is necessary but not
   sufficient — its POSITION must be frozen too, or the invisible tail drags the ribbon's geometry across
   the map.
