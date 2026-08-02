# Translation sheet: NS_BuffCast → CkParticles (PRE-IMPLEMENTATION)

Schema and evidence-tag conventions: [README.md](README.md). Exemplars: [NS_BasicAttack.md](NS_BasicAttack.md),
[NS_Lightning_Range.md](NS_Lightning_Range.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior id is allocated, no `.ush` exists, no cadence row was added, no look was authored, no
asset was generated, nothing was built and nothing was rendered. Sections 1–6 are archaeology and a
plan; §6's capability-gap callout is the part an implementer must read before committing a session.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_BuffCast` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Corpus system files | `systems/Vefects/Anime_VFX/Shared/Skills/NS_BuffCast.{json,txt}` |
| Corpus material files | `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part02,Rainbow,Arrows,Star01,Part04,Ring01,Trail03}.json` |
| Corpus texture files | `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Part_04,Arrow_01,Star_01,Ring_01,Ring_02,Gradient_02,Noise_02,LUT_Rainbow_01,WhitePixel}.json` |
| Meshes | none — this system has no mesh renderer |

**The source Niagara asset was never opened.** Every fact below is `[corpus]` unless tagged otherwise.

> ### A sibling with a near-identical name exists — take the right one
> `[corpus]` The pack ships `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Buff_Cast` (underscored
> name). It is a **different, parameterized** system. Fastest one-line discriminators, either of
> which settles it on its own:
> - **User parameters.** `NS_BuffCast` exports `userParameters: []`. `NS_Buff_Cast` exports **ten**:
>   `User.Arrow Big Color 01`, `User.Arrow Color 01`, `User.Glow Color 01/02/03`,
>   `User.Rainbow Color 01`, `User.Ring Color 01`, `User.Scale Overall`, `User.Sparkles Color 01`,
>   `User.Sparkles Trail Color 01`.
> - **Renderer materials.** `NS_BuffCast` draws through `M_VFX_DisAdd_*`; `NS_Buff_Cast` draws through
>   `MI_VFX_*` (`MI_VFX_Glow_01/02/04`, `MI_VFX_Arrows_01`, `MI_VFX_Lens_Rainbow_01`,
>   `MI_VFX_Ring_01`, `MI_VFX_Star_01`, `MI_VFX_Trail_03`).
>
> This sheet documents the **`Anime_VFX/Shared/Skills`** variant only — the fixed-colour, no-user-param one.

---

## 2. System anatomy `[corpus]`

**10 CPU emitters, ALL enabled, ALL world-space (`LocalSpace: false`), `Bounds: Dynamic`,
`Determinism: false`, zero user parameters.** Nine are sprite emitters that burst at loop start on a
**1.0 s** loop; the tenth is a **ribbon**.

**23 sprite particles per loop** (1+1+1+1+1+1+7+7+3), plus the ribbon (count `[unresolved]`, see below).

| # | Emitter | Spawn | Spawn t | Lifetime | Loop | Renderer / alignment | Material | Size |
|---|---|---|---|---|---|---|---|---|
| 0 | `Bomb_Glow_01` | Burst 1 | 0 | 1.0 | 1.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part01` | uniform 500 |
| 1 | `Bomb_Glow_02` | Burst 1 | 0 | 1.0 | 1.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part01` | uniform 400 |
| 2 | `Bomb_Glow_03` | Burst 1 | 0 | 1.0 | 1.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part02` | uniform 200 |
| 3 | `Raimbow` *(sic)* | Burst 1 | 0 | 1.0 | 1.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Rainbow` | uniform 450 |
| 4 | `Arrow` | Burst 1 | 0 | **1.5** | 1.0 | Sprite, **VelocityAligned** / FaceCamera | `M_VFX_DisAdd_Arrows` | non-uniform (170, 170) |
| 5 | `BigArrow` | Burst 1 | 0 | **1.5** | 1.0 | Sprite, **VelocityAligned** / FaceCamera | `M_VFX_DisAdd_Arrows` | non-uniform (150, 240) |
| 6 | `Sparkles_02` | Burst **7** | **0.05** | rand — **`[unresolved]`**, see §5 | 1.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Star01` | rand uniform 40–70 |
| 7 | `Sparkles_01` | Burst **7** | 0 | rand 0.2–0.4 | 1.0 | Sprite, **VelocityAligned** / FaceCamera | `M_VFX_DisAdd_Part04` | rand non-uniform (35,80)–(50,90) |
| 8 | `Ring` | Burst **3** | 0 | rand 0.3–0.7 | 1.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Ring01` | rand uniform 150–160 |
| 9 | `Sparkles_02_Trail` | `[unresolved]` | — | 0.2 | **0.4, Loop Behavior = Once** | **Ribbon** | `M_VFX_DisAdd_Trail03` | ribbon width 15 |

`Spawn Burst Instantaneous.Loop Count Limit = 1` on all nine sprite emitters, but
`UseLoopCountLimit = false` on all nine — the value is an authored leftover and **never fires**
`[corpus]`. Same trap NS_Lightning_Range records. Every sprite emitter really bursts once per 1.0 s
loop, forever.

> ### `[unresolved: how the ribbon emitter spawns]`
> `Sparkles_02_Trail`'s `Emitter Update` stack contains **one** module — `Emitter State` — with **no
> spawn module at all**, and its `Particle Spawn` stack's only enabled module is `Initialize Ribbon`
> (`Add Velocity from Point` is present but **disabled**). Meanwhile `Sparkles_02` runs a
> **`Generate Location Event`** module (Event Send Rate 30, Event Probability 0.5, Delay Before
> Sending Events 0.5, Movement Tolerance 0.5, Unit Spacing 20, evaluation "Every Frame").
>
> **The corpus exporter emits only three stages per emitter** — `Emitter Update`, `Particle Spawn`,
> `Particle Update` (verified against `NS_BuffCast.json`: every emitter reports exactly those three).
> An **Event Handler stack is therefore not exported at all.** The overwhelmingly likely reading is
> that `Sparkles_02_Trail` is event-spawned from `Sparkles_02`'s location events `[inferred]`, but
> the linkage, the receiving module, and the spawn count per event are **not in the corpus** and must
> not be guessed. Confirming it requires either an exporter change or a human opening the asset.
>
> Also unresolved: whether the ribbon's `Loop Behavior = Once` (duration 0.4 s) means the trail plays
> only on the system's first loop while the nine sprite emitters loop forever.

`Sparkles_02`'s `[values]` block additionally carries a full set of `GenerateLocationEvent.*` entries
**and** `Sparkles_02_Trail` carries `RandomRangeFloat001.*` entries for its disabled module — the
exporter dumps the emitter's whole Rapid-Iteration store, including parameters belonging to disabled
or removed modules. **A value in `[values]` is not evidence that a module is running**; cross-check
the module list. (`Scale Sprite Size 001` on `Sparkles_02` is explicitly marked DISABLED and its two
curve overrides are therefore inert.)

---

## 3. Mesh geometry

**N/A — NS_BuffCast has no mesh renderer.** All ten renderers are sprite or ribbon.

---

## 4. Material family and per-instance deltas `[corpus]`

**All eight materials are instances of ONE parent, `/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd`**
— the same family CkUsf already implements as the `DissolveAdd` look entry point. Every one of them:
`MD_Surface`, `BLEND_Translucent`, `MSM_Unlit` (`unlit: true`), `twoSided: false`, connected outputs
`EmissiveColor` + `Opacity`, dynamic-parameter channels **`[dissolve, distortion, offset, core_color]`**,
expression count 161 (identical histograms — one graph, N parameterizations).

Reference row = `M_VFX_DisAdd_Part01` (all-defaults instance). Its absolute values, needed because the
delta table below is relative to it:

`Brightness 1`, `Glow_Intensity 1`, `Opacity_Boldness 0.5`, `Opacty_DepthFade 20`,
`Opacty_Step 0`, `Opacty_StepAdd 0.1`, `Gradient_Invert 0.5`, `GradientMap_Displacement 0.1`,
`Core_Intensity 0`, `Core_Power 1`, `Color_CoreDifferent 0`, `CamOffset 0`, `Dissolve 0`,
`Dissolve_Invert 0`, all `*_Scale_{X,Y} 1`, all `*_Speed_{X,Y} 0`, all `*_Offset_{X,Y} 0`,
`Color_Core RGBA(1, 1, 1, 0)`; textures `Main_Tex`/`Color_Tex`/`Dissolve_Tex` = `T_VFX_Part_01`,
`Distortion_Tex`/`GradientShape_Tex` = `T_VFX_Noise_02`, `GradientMap_Tex` = `T_VFX_WhitePixel` (1×1).

| Material | Δ vs `Part01` | Used by |
|---|---|---|
| `M_VFX_DisAdd_Part01` | (reference — no deltas) | Bomb_Glow_01, Bomb_Glow_02 |
| `M_VFX_DisAdd_Part02` | `Glow_Intensity` 1 → **0.3**; Main/Color/Dissolve_Tex → `T_VFX_Part_02` | Bomb_Glow_03 |
| `M_VFX_DisAdd_Rainbow` | `GradientMap_Displacement` 0.1 → **0.9**; `Gradient_Invert` 0.5 → **2**; `Opacity_Boldness` 0.5 → **1.5**; `Opacty_StepAdd` 0.1 → **0.3**; **`GradientMap_Tex` `T_VFX_WhitePixel` → `T_VFX_LUT_Rainbow_01`**; `GradientShape_Tex` `T_VFX_Noise_02` → `T_VFX_Part_01`; `Main_Tex` → `T_VFX_Ring_02` | Raimbow |
| `M_VFX_DisAdd_Arrows` | `Brightness` 1 → **10**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Arrow_01` | Arrow, BigArrow |
| `M_VFX_DisAdd_Star01` | `Brightness` 1 → **6**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **10**; Main/Color/Dissolve_Tex → `T_VFX_Star_01` | Sparkles_02 |
| `M_VFX_DisAdd_Part04` | `Brightness` 1 → **6**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **30**; Main/Color/Dissolve_Tex → `T_VFX_Part_04` | Sparkles_01 |
| `M_VFX_DisAdd_Ring01` | `Brightness` 1 → **10**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Ring_01` | Ring |
| `M_VFX_DisAdd_Trail03` | `Brightness` 1 → **4**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **30**; Main/Color/Dissolve_Tex → `T_VFX_Gradient_02` | Sparkles_02_Trail (ribbon) |

**Nothing in this system animates `Dissolve_Speed`, `Distortion_Intensity` or `MainTex_Scale`** — the
three deltas `NS_BasicAttack` leaned on are all at family defaults here. The differentiation is
carried by `Brightness`, `Glow_Intensity`, `Opacity_Boldness`, the gradient-map chain, and the shape
texture.

`M_VFX_DisAdd_Part04` here is **byte-identical in parameters** to the `Part04` instance NS_BasicAttack
already recreated as the `PartDisAdd04` look — see §6.

### Referenced textures `[corpus]`

| Texture | Size | Source format | Compression | sRGB | Address |
|---|---|---|---|---|---|
| `T_VFX_Part_01` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Part_02` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Part_04` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Arrow_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Star_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Ring_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Ring_02` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Gradient_02` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Noise_02` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| **`T_VFX_LUT_Rainbow_01`** | **512×2** | **TSF_BGRA8** | **TC_Default** | **true** | Wrap/Wrap |
| `T_VFX_WhitePixel` | 1×1 | TSF_RGBA16 | TC_Default | true | Wrap/Wrap |

Everything except the LUT is a greyscale mask that the existing procedural-texture generator can plausibly
stand in for. **`T_VFX_LUT_Rainbow_01` is not a mask — it is a 512×2 sRGB colour ramp**, and it is the
Rainbow layer's entire visual identity. See §6.

---

## 5. Per-layer runtime curves `[corpus]`

Every curve samples **NormalizedAge** (age / that emitter's own lifetime) unless stated. `C` = constant
(step) key, `L` = linear key. Keys are transcribed at the corpus's own precision.

`Dynamic Material Parameters` writes **Index 0 only** on every emitter (`Write Parameter Index 0 = true`,
1/2/3 false), so a single float4 `[dissolve, distortion, offset, core_color]` per particle.

### Layer 0 — `Bomb_Glow_01`
- Initialize Color: `RGBA(1, 0.184475, 0.386429, 0.4)`; Uniform Sprite Size **500**; Lifetime 1; Position Offset (0,0,0); `UsePositionOffset = false`.
- Scale Color (RGBA Together): `R (0, 1)L (1, 1)L | G (0, 1)L (1, 1)L | B (0, 1)L (1, 1)L | A (0, 1)L (1, 0)L`.
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (0.1, 1)L (1, 1)L`.
- Dynamic params: **`[1, 0, 0, 0]`** constant (dissolve = 1).

### Layer 1 — `Bomb_Glow_02`
- Initialize Color: `RGBA(0.913099, 0.0193824, 0.130136, 0.4)`; Uniform Sprite Size **400**; Lifetime 1.
- Scale Color: identical to layer 0 (`RGB` flat 1, `A (0, 1)L (1, 0)L`).
- Scale Sprite Size: identical to layer 0 — `(0, 0.5)C (0.1, 1)L (1, 1)L`.
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 2 — `Bomb_Glow_03`
- Initialize Color: `RGBA(0.313989, 0, 0.00227652, 0.483)`; Uniform Sprite Size **200**; Lifetime 1.
- Scale Color: identical to layer 0.
- Scale Sprite Size: identical to layer 0.
- Dynamic params: **`[2, 0, 0, 0]`** constant (dissolve = 2 — past full erosion at the family's threshold; the
  layer reads as a soft glow rather than a dissolving shape `[inferred]`).

### Layer 3 — `Raimbow`
- Initialize Color: `RGBA(0.913099, 0.913099, 0.913099, 0.1)`; Uniform Sprite Size **450**; Lifetime 1.
- **Sprite Rotation Mode = Random**, `Sprite Rotation Angle Min 0` / `Max 360`.
- Scale Color (RGBA Together): `R (0, 0.5)L | G (0, 0.5)L | B (0, 0.5)L | A (0, 1)L (1, 0)L`
  — RGB is a **single key at 0.5**, i.e. a flat 0.5 multiplier; alpha ramps 1 → 0 over life.
- Scale Sprite Size: `(0, 0.5)C (0.2, 0.9)C (1, 1)L`.
- Dynamic params: **`[0.5, 0, 0, 0]`** constant.

### Layer 4 — `Arrow`
- Lifetime **1.5** (exceeds the 1.0 s loop by 0.5 s → generations overlap by half a loop).
- Initialize Color `RGBA(1, 1, 1, 1)`; **Position Offset `(0, 0, -119.316)`** with `UsePositionOffset = true`.
- `Add Velocity`: **`(0, 0, 550)`**, `Scale Added Velocity (1,1,1)` — straight up.
- Sprite Size Mode Non-Uniform, `Sprite Size (170, 170)`. (`Sprite Size Min (5,5)` / `Max (10,10)` and
  `Uniform Sprite Size 150` / `Min 5` / `Max 10` are also in the store but the mode selects the direct
  `Sprite Size` value.)
- Sprite Rotation Mode Random, authored `Sprite Rotation Angle 0.827273`, Min 0 / Max 360.
- Scale Velocity (Vector from Curve), all three axes identical: `X/Y/Z: (0, 1)C (0.2, 0.3)C (1, 0.05)C`.
- Solve Forces and Velocity: `Clamp Velocity = false`, `Limit Acceleration = false`, Rotational Solver on.
  (`Acceleration Limit 9999` / `Speed Limit 1000` are present but both limiters are disabled — inert.)
- Color from Curve:
  - `R: (0, 1)C (0.0784787, 1)L (0.283731, 1)L (0.625415, 0.672443)L (0.947781, 0.223228)C`
  - `G: (0, 0.913099)C (0.0784787, 0.501026)L (0.283731, 0.0773835)L (0.625415, 0.021219)L (0.947781, 0)C`
  - `B: (0, 0.584079)C (0.0784787, 0.0559999)L (0.283731, 0.025)L (0.625415, 0.0168074)L (0.947781, 0.116971)C`
  - `A: (0.0869303, 1)C (1, 0)C`
- Scale Sprite Size (Uniform Curve): `(0, 0.4)C (0.1, 1)C (1, 0.4)C`.
- Scale Sprite Size 001 (Non-Uniform Curve): `X: (0, 0.2)C (0.3, 0.7)C | Y: (0.2, 1)L (1, 1.2)L`
  (its `Uniform Curve Sprite Scale = (0, 0)L (1, 1)L` is not the active channel in Non-Uniform mode).
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 5 — `BigArrow`
- Lifetime **1.5**; Initialize Color `RGBA(1, 1, 1, 1)`; **Position Offset `(0, 0, -52.2087)`**, `UsePositionOffset = true`.
- `Add Velocity`: **`(0, 0, 150)`**.
- Sprite Size Mode Non-Uniform, `Sprite Size (150, 240)`; Uniform Sprite Size 300 present but not selected.
- Sprite Rotation Mode Random, authored angle 0.827273, Min 0 / Max 360.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.3, 0.05)C` — **two keys**, a harder and later cut than Arrow's three.
- Color from Curve (note G and B differ from Arrow's):
  - `R: (0, 1)C (0.0784787, 1)L (0.283731, 1)L (0.625415, 0.672443)L (0.947781, 0.223228)C`
  - `G: (0, 0.913099)C (0.0784787, 0.489926)L (0.283731, 0.0925239)L (0.625415, 0.021219)L (0.947781, 0)C`
  - `B: (0, 0.584079)C (0.0784787, 0.035)L (0.283731, 0.0409999)L (0.625415, 0.0168074)L (0.947781, 0.116971)C`
  - `A: (0.0869303, 1)C (1, 0)C`
- **No Scale Sprite Size module at all** (5 update modules) — BigArrow holds its authored size for its whole life.
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 6 — `Sparkles_02` (7 particles, spawn t = 0.05)
- **`[unresolved: this emitter's particle lifetime]`** — the corpus states two different ranges and
  which one the engine evaluates is not decidable from the export:
  - `Initialize Particle` has `Lifetime Mode = Random` with `Lifetime Min 0.4` / `Lifetime Max 0.7`
    (Random mode reads these two inputs);
  - the same module carries `[override] Lifetime = dyn:Random Range Float`, and
    `RandomRangeFloat.Minimum = 0.2` / `Maximum = 0.4` (Random Seed 0) — but the `Lifetime` pin an
    override drives is the **Direct Set** input, which Random mode does not read.
  The likely reading is that **0.4–0.7 wins** and the `Random Range Float` dynamic input is a leftover
  from an earlier Direct-Set configuration `[inferred]`. Do not encode either range without confirming
  in the editor; the ranges do not overlap except at a single point, so guessing wrong is visible.
  *(The same shape recurs on every `Sparkles_*` emitter across this batch — see the sibling sheets.)*
- `Sphere Location`: `Sphere Radius` **0.5**, Random distribution, Spawn Only, `Surface Only = false`,
  Offset (0,0,0), Non Uniform Scale (1,1,1). A 0.5-unit sphere — effectively a point.
- `Add Velocity from Point`: strength from `Random Range Float 001` **Min 1000, Max 1000** (a constant
  1000), Origin Offset (0,0,0), Velocity Falloff Distance 100.
- Sprite Size Mode Random Uniform: **Min 40, Max 70**. Sprite Rotation Mode Random, authored angle 90, Min 0 / Max 360.
- Initialize Color `RGBA(1, 1, 1, 1)`.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.2)C (1, 3.91223e-08)C` (final key is numerically zero).
- Color from Curve:
  - `R: (0, 1)C (0.416541, 1)L (1, 1)C`
  - `G: (0, 0.708376)C (0.416541, 0.341915)L (1, 0.109462)C`
  - `B: (0, 0.0466651)C (0.416541, 0.109462)L (1, 0.130136)C`
  - `A: (0, 1)C`  *(single key — alpha is flat 1 for the whole life)*
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`.
- **Scale Sprite Size 001 is DISABLED** — its `X: (0, 0.2)C (0.3, 0.7)C | Y: (0.2, 1)L (1, 1.2)L` is inert.
- `Sprite Rotation Rate` (Float from Curve): **`(0, 720)C (1, 0)C`** — 720 °/s spin decaying to 0.
- `Generate Location Event` (Every Frame): Send Rate 30, Event Probability 0.5, Delay Before Sending 0.5,
  Movement Tolerance 0.5, Unit Spacing 20. **This is the ribbon's presumed driver — see §2's unresolved box.**
- Dynamic params: **`[1, 0, 0, 0]`** constant.

### Layer 7 — `Sparkles_01` (7 particles, spawn t = 0)
- Lifetime Random: `Lifetime Min 0.2` / `Max 0.4`, and its `[override] Lifetime = dyn:Random Range Float`
  resolves `RandomRangeFloat Min 0.2 / Max 0.4` — **the two agree here, so 0.2–0.4 is unambiguous**
  (unlike layer 6).
- `Sphere Location`: `Sphere Radius` **0.1**, Random, Spawn Only, Surface Only false.
- `Add Velocity from Point`: strength `Random Range Float 001` **Min 1300, Max 2000** — a real random
  range, unlike layer 6's constant 1000.
- Sprite Size Mode **Random Non-Uniform**: `Sprite Size Min (35, 80)`, `Max (50, 90)` — width × length,
  velocity-aligned streaks. Initialize Color `RGBA(1, 1, 1, 1)`.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.35)C (1, 0.05)C`.
- Color from Curve:
  - `R: (0, 1)C (0.416541, 1)L (1, 1)C`
  - `G: (0, 0.69869)C (0.416541, 0.30758)L (1, 0.109462)C`
  - `B: (0, 0.0149999)C (0.416541, 0.063)L (1, 0.130136)C`
  - `A: (0, 1)C`
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`; its non-uniform companion
  `X: (0, 0)L (1, 1)L | Y: (0, 0)L (1, 1)L` is not the active channel in Uniform mode.
- Scale Sprite Size 001 (Non-Uniform Curve): `X: (1, 1)L | Y: (0, 1)C (1, 0.6)C` — the streak shortens
  to 0.6 of its length over life while its width holds.
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 8 — `Ring` (3 particles, spawn t = 0)
- Lifetime Mode **Random**: Min **0.3**, Max **0.7**. (`InitializeParticle.Lifetime = 1` is also in the
  store; Random mode means Min/Max win and the 1 is inert `[corpus]`.)
- Sprite Size Mode Random Uniform: **Min 150, Max 160** (`Uniform Sprite Size 200` present but not selected).
- Sprite Rotation Mode Random, Min 0 / Max 360. Initialize Color `RGBA(1, 1, 1, 1)`.
- Color from Curve:
  - `R: (0.0446725, 1)C (0.216118, 1)L (0.418956, 1)L (0.814971, 0.672443)L (0.988832, 0.223228)C`
  - `G: (0.0446725, 0.913099)C (0.216118, 0.502887)L (0.418956, 0.501026)L (0.814971, 0.021219)L (0.988832, 0)C`
  - `B: (0.0446725, 0.584079)C (0.216118, 0.0561285)L (0.418956, 0.056)L (0.814971, 0.016807)L (0.988832, 0.116971)C`
  - `A: (0, 0)L (0.076064, 1)C (1, 0)C`
- **Dynamic param 1 is the only ANIMATED dynamic channel in this whole system**:
  `Index 0 Param 1 = Float from Curve (0, -0.325)C (1, -0.5)C` — the dissolve threshold slides
  −0.325 → −0.5 over life. Params 2/3/4 constant 0.
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (0.1, 0.9)C (1, 1)C`.

### Layer 9 — `Sparkles_02_Trail` (ribbon)
- Emitter State: **Loop Behavior = Once**, Loop Duration **0.4** (every other emitter is Infinite / 1.0).
- `Initialize Ribbon`: Color `RGBA(1, 1, 1, 1)`, Lifetime **0.2**, **Position Offset `(100, 0, 0)`**
  (`UsePositionOffset = false`, so the offset is inert `[corpus]`), **Ribbon Width 15**, Ribbon Width Mode Direct Set.
- Color from Curve — **and its `CurveIndex` is `linked:Emitter.Age`, not NormalizedAge**:
  - `R: (0, 1)C (0.416541, 1)L (1, 1)C`
  - `G: (0, 0.708376)C (0.416541, 0.341915)L (1, 0.109462)C`
  - `B: (0, 0.0466651)C (0.416541, 0.109462)L (1, 0.130136)C`
  - `A: (0, 1)C (0.119529, 1)L (0.997283, 0)L`
- `Scale Ribbon Width` (Float from Curve): `(0, 1)C (1, 0)C`.
- Dynamic params: **`[1, 0, 0, 0]`** constant.
- `Add Velocity from Point` is present but **DISABLED**; its `RandomRangeFloat001 Min/Max 1000` is inert.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new row is required.** No existing row in `ck::particles::Get_TemplateSpecs()` matches:

| Row | Loop | Lifetime | Burst |
|---|---|---|---|
| `PS_CkParticles_Template` | continuous | — | — |
| `PS_CkParticles_Template_Burst` | 1.2 | 1.2 | 96 |
| `PS_CkParticles_Template_Single` | 1.0 | 1.1 | 1 |
| `PS_CkParticles_Template_Slash` | 1.0 | 0.5 | 19 |
| **needed here** | **1.0** | **1.5** | **23** |

Loop 1.0 s (every sprite emitter), template particle lifetime **1.5 s** (the longest source layer —
`Arrow`/`BigArrow`; every shorter layer must self-hide past its own lifetime the way `Behavior_Slash`
does), burst **23** (1+1+1+1+1+1+7+7+3). The ribbon is **not** in that count — it cannot be expressed
at all (§6.5).

Layer partition: `Seed % 23`, matching `Behavior_Slash`'s `Seed % 19` idiom — burst UniqueIDs are
sequential, so one template particle lands on exactly one source particle per loop. Suggested banding:
0 = Bomb_Glow_01, 1 = Bomb_Glow_02, 2 = Bomb_Glow_03, 3 = Raimbow, 4 = Arrow, 5 = BigArrow,
6–12 = Sparkles_02, 13–19 = Sparkles_01, 20–22 = Ring.

### 6.2 VisTag / renderer needs

The shared set (0–4) cannot carry this effect: **seven of the nine sprite layers are camera-facing
sprites that each need a DIFFERENT material**, and VisTag 0 / VisTag 4 both bind through the single
`User.SpriteMaterial` user parameter. Row-declared renderers are the right mechanism — but the
existing `ECk_ParticlesRenderer_Kind` has only **`Mesh`** and **`VelocityAlignedSprite`**.

| Source emitter | Renderer needed | Available today? |
|---|---|---|
| Bomb_Glow_01/02/03, Raimbow, Sparkles_02, Ring | camera-facing sprite with a per-row explicit look | **NO — new renderer kind required** |
| Arrow, BigArrow, Sparkles_01 | `VelocityAlignedSprite` with a per-row look | yes |
| Sparkles_02_Trail | ribbon | **NO — see §6.5** |

**Minimum pipeline addition: a `CameraFacingSprite` kind on `FCk_ParticlesRendererSpec`.** It is the
same shape as `VelocityAlignedSprite` (one look bound explicitly via `bOverrideMaterials`), differing
only in the renderer's `Alignment`/`FacingMode`, so it is additive and small — but it does not exist
and must be written. Every effect in this batch needs it.

Distinct looks needed for this system: **8** if each material instance gets its own
(Part01, Part02, Rainbow, Arrows, Star01, Part04, Ring01, Trail03) — but `Bomb_Glow_01` and
`Bomb_Glow_02` share `Part01`, and `Arrow`/`BigArrow` share `Arrows`, so **6 distinct renderers** would
suffice if two layers may share one VisTag. They can: VisTag selects a renderer, not a layer.
So **6 row renderers** (5 camera-facing + … see the split below), which with the existing shared band
means VisTags allocated **above 9** (behavior 7 owns 5–9).

| Renderer | Kind | Look | Source emitters |
|---|---|---|---|
| a | CameraFacingSprite *(new kind)* | `PartDisAdd01` | Bomb_Glow_01, Bomb_Glow_02 |
| b | CameraFacingSprite *(new kind)* | `PartDisAdd02` | Bomb_Glow_03 |
| c | CameraFacingSprite *(new kind)* | `RainbowDisAdd` | Raimbow |
| d | CameraFacingSprite *(new kind)* | `StarDisAdd01` | Sparkles_02 |
| e | CameraFacingSprite *(new kind)* | `RingDisAdd01` | Ring |
| f | VelocityAlignedSprite | `ArrowsDisAdd` | Arrow, BigArrow |
| g | VelocityAlignedSprite | `PartDisAdd04` — **already exists** (NS_BasicAttack) | Sparkles_01 |

That is **7 renderers**, of which 5 need the new kind and 1 reuses an existing look verbatim.

### 6.3 Look / material needs

All eight materials are `M_VFX_DissolveAdd` instances, so the existing `CkUsf_Look_DissolveAdd` entry
point is the right family shader — but **five of its needed parameters are not plumbed today**. Against
the 15-parameter signature (`ShapeTex`, `DissolveTex`, `DistortTex`, `CoreColor`, `Brightness`,
`DissolveSpeed`, `DissolveEdge`, `DistortScale`, `OpacityBoldness`, `DissolveSpeedY`, `DissolveBias`,
`DissolveScale`, `DistortIntensity`, `DistortSpeed`, `MainTexScale`):

| Source param this batch uses | Plumbed? | Consequence if omitted |
|---|---|---|
| `Brightness` | yes | — |
| `Opacity_Boldness` | yes (`OpacityBoldness`) | — |
| `Glow_Intensity` (0.3 on Part02) | **no** | Bomb_Glow_03 reads ~3× too bright |
| `Gradient_Invert` (0 vs 0.5 vs 2) | **no** | gradient chain differences lost |
| `GradientMap_Tex` + `GradientMap_Displacement` | **no** | **fatal for the Rainbow layer** — see §6.5 |
| `Opacty_StepAdd` (0.3 on Rainbow) | **no** | opacity bias lost |
| `Opacty_DepthFade` (10/20/30) | **no** | same gap NS_BasicAttack §13.3 records |
| `Core_Intensity` / `Core_Power` / `Color_CoreDifferent` | **no** | (all 0/1 defaults in this system — inert here) |
| `CamOffset` | **no** | (0 in this system — inert here) |

Only `Glow_Intensity`, `Gradient_Invert` and the gradient-map pair are new *and* non-default here.
`Glow_Intensity` and `Gradient_Invert` are cheap scalar additions to the family shader.

### 6.4 Texture needs

Nine greyscale 512² masks. Mapping onto the existing procedural bake set
(`Glow`, `Flare`, `Smoke`, `Electric`, `Streak`, `Ring`, `SweepStreak`, `TileNoise`, `SlashArc01`,
`SlashArc02`, `WindBand`, `SoftParticle`, `SparkStreak`):

| Source | Plausible existing stand-in | New bake needed? |
|---|---|---|
| `T_VFX_Part_01` | `SoftParticle` (NS_BasicAttack measured it as `pow(1-r, 2.2)` radially symmetric) | no |
| `T_VFX_Part_04` | `SparkStreak` (already measured and baked) | no |
| `T_VFX_Noise_02` | `TileNoise` (NS_BasicAttack §7 established this mapping) | no |
| `T_VFX_Ring_01` | `Ring` (SDF ring) — **unmeasured**, likely needs its own parameterization | probably |
| `T_VFX_Part_02` | — | **yes** — measure it |
| `T_VFX_Arrow_01` | — | **yes** — a directional chevron/arrow shape; no existing bake resembles one |
| `T_VFX_Star_01` | `Flare` (star) — **unmeasured** | probably |
| `T_VFX_Ring_02` | — | **yes** (Rainbow's `Main_Tex`) |
| `T_VFX_Gradient_02` | — | **yes** (ribbon only — moot if the ribbon is dropped) |
| **`T_VFX_LUT_Rainbow_01`** | — | **yes, and it is NOT a mask** — 512×2 sRGB BGRA colour ramp |

None of these has been measured off its PNG yet. NS_BasicAttack §7's method (32-bin per-axis profiles,
structure tensor, zero-crossing counts, radial ring means) applies unchanged and must be run per texture
at implementation time.

### 6.5 CAPABILITY GAPS — read before committing a session

1. **RIBBON RENDERER — does not exist, at any level.** `Sparkles_02_Trail` is a
   `NiagaraRibbonRendererProperties` emitter. `ECk_ParticlesRenderer_Kind` has no ribbon kind, the
   template builder emits none, the DI writes no ribbon attributes (`RibbonWidth`, `RibbonID`,
   `RibbonLinkOrder`), and `CkParticles/CLAUDE.md` records that CkUsf's ribbon usage flag is
   "deliberately absent". **Recreating layer 9 requires new pipeline capability in three places
   (renderer kind, DI outputs, CkUsf usage flag).** The honest options are (a) build ribbon support,
   (b) approximate the trail as a chain of velocity-aligned sprites and record it as a §13 deviation,
   or (c) drop the layer and record it. **Do not assume (b) is free** — a sprite chain does not
   reproduce a continuous ribbon's width taper or its `Emitter.Age`-indexed colour.

2. **EVENT-DRIVEN SPAWNING — not expressible, and not even fully readable from the corpus.**
   `Generate Location Event` on `Sparkles_02` is a Niagara event, and CkParticles has no event
   mechanism at all: the DI's `ExecuteStage` is a **pure, stateless per-particle function** with no
   inter-particle or inter-emitter channel. The event handler stack is additionally **not exported**
   (§2), so the spawn contract is unknown. Both facts point the same way: layer 9 cannot be faithfully
   ported without new capability AND new archaeology.

3. **CAMERA-FACING SPRITE ROW RENDERER — does not exist.** Five of this system's nine sprite layers
   need one each with a distinct look. Additive and small, but it is a code change, not data. **All six
   effects in this batch need it**; it is worth doing once, first, before any of them.

4. **GRADIENT-MAP LUT — the Rainbow layer's entire identity, and unrepresentable today.**
   `M_VFX_DisAdd_Rainbow` swaps `GradientMap_Tex` from the family's 1×1 `T_VFX_WhitePixel` to a
   **512×2 sRGB colour ramp** and raises `GradientMap_Displacement` to 0.9 with `Gradient_Invert` 2.
   NS_BasicAttack §13.4 dismissed the gradient chain as "a no-op on this family (a white-pixel gradient
   map)" — **that reasoning does not transfer here**, because this instance's gradient map is real
   content. Reproducing the rainbow needs (a) a gradient-map sample + displacement in
   `DissolveAdd.ush`, and (b) a colour-ramp texture the procedural generator does not currently make
   (every existing bake is greyscale, `SRGB=false`, `TC_VectorDisplacementmap`). Without both, the
   Raimbow layer renders as a plain white-ish glow.

5. **WORLD SPACE vs LOCAL SPACE.** All ten source emitters are `LocalSpace: false`. The CkParticles
   template is local-space (self-driving behaviors write absolute positions). Same deviation
   NS_BasicAttack §13.2 records: visible only if the spawning actor moves during the 1.5 s life.
   Record it; do not "fix" it per-effect.

6. **SPRITE ROTATION.** Four layers use `Sprite Rotation Mode = Random` (0–360°) and `Sparkles_02`
   additionally spins at 720 °/s decaying to 0. The DI does write `OutRotation` (sprite degrees), but
   `CkParticles/CLAUDE.md` documents rotation as applying on **VisTag 2 (smoke sprite)** and is silent
   for the other sprite tags. `[unresolved: whether Particles.SpriteRotation is bound on the shared
   camera sprite renderer and would be bound on a new row-declared camera-facing sprite]` — verify
   against `CkParticles_TemplateBuilder.cpp` before planning the layer, because a silently-ignored
   rotation makes four layers wrong in a way no headless test catches.

7. **NON-UNIFORM sprite size on a camera-facing sprite.** `Arrow` (170×170) and `BigArrow` (150×240)
   are non-uniform, and `Sparkles_01` is non-uniform random. `OutSize` is a float2, so this is
   expressible — but the existing `VelocityAlignedSprite` documentation says "stretch is
   `Particles.SpriteSize.y` along motion", so the axis convention must be checked, not assumed.

8. **No sub-UV in this system** — unlike its Debuff/Heal siblings, NS_BuffCast has no flipbook. One
   fewer gap here.

### 6.6 Behavior id

**Do NOT allocate an id in this document.** `ck::particles::NumBehaviors` is 18 today (ids 0–17), so
the next free id is 18 *at the time of writing*, but the sibling sheets in this batch and any
concurrent work also want ids. The implementing session allocates from `NumBehaviors` at that moment
and bumps it in the same edit.

### 6.7 Complexity assessment

**Tier L.** Not because the curves are hard — they are ordinary — but because a faithful port needs
**three pipeline capabilities that do not exist**: a camera-facing-sprite row renderer (small), a
gradient-map/colour-LUT path through the DissolveAdd family plus non-greyscale procedural textures
(medium), and a ribbon renderer with its DI attributes (large). Dropping the ribbon and the rainbow
gradient would bring it to **M**, at a fidelity cost that must be a recorded maintainer decision
rather than a silent one.

---

## 7+. Reserved for implementation — sections 7–14 per [README.md](README.md) are written by the session that implements this effect.
