# Translation sheet: NS_DebuffCast → CkParticles (PRE-IMPLEMENTATION)

Schema and evidence-tag conventions: [README.md](README.md). Exemplars: [NS_BasicAttack.md](NS_BasicAttack.md),
[NS_Lightning_Range.md](NS_Lightning_Range.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior id is allocated, no `.ush` exists, no cadence row was added, no look was authored, no
mesh or texture was generated, nothing was built and nothing was rendered.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_DebuffCast` |
| Pack | Vefects — *Anime VFX* |
| Corpus system files | `systems/Vefects/Anime_VFX/Shared/Skills/NS_DebuffCast.{json,txt}` |
| Corpus material files | `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Arrows,Part01_Bright,Ring01,Part03_Bright,Flames01,Slash04,Part01,Part02,Rainbow}.json` |
| Corpus mesh files | `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_Slash02.{json,obj}` |
| Corpus texture files | `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Arrow_01,Part_01,Part_02,Part_03,Ring_01,Wind_01,Noise_02,Noise_04,LUT_Rainbow_01,WhitePixel}.json` |

**The source Niagara asset was never opened.** Every fact below is `[corpus]` unless tagged otherwise.

> ### Sibling with a near-identical name
> `[corpus]` `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Debuff_Cast` (underscored) is a different,
> parameterized system with **7** emitters. Discriminators: `NS_DebuffCast` exports
> `userParameters: []` and draws through `M_VFX_DisAdd_*`; `NS_Debuff_Cast` exports **ten**
> (`User.Arrow Color 01`, `User.Flames Color 01`, `User.Glow Color 01`, `User.Ring Color 01`,
> `User.Scale Overall`, `User.Slash Color 01`, `User.Slash Color 02`, `User.Sparkles Color 01`,
> `User.Sparkles Color 02`, `User.Sparkles Dark Color 01`) and draws through `MI_VFX_*`. This sheet
> documents the **`Anime_VFX/Shared/Skills`** variant only.

---

## 2. System anatomy `[corpus]`

**10 CPU emitters, of which FOUR ARE DISABLED, all world-space (`LocalSpace: false`),
`Bounds: Dynamic`, `Determinism: false`, zero user parameters.**

**This is the richest and most structurally awkward system in the batch.** It mixes three different
emitter cadences and is the only one with a mesh renderer, a sub-UV flipbook, and disabled emitters.

| # | Emitter | Enabled | Life Cycle / Loop | Loop dur | Spawn | Lifetime | Renderer | Material |
|---|---|---|---|---|---|---|---|---|
| 0 | `Bomb_Glow_01` | **DISABLED** | System / Infinite | 1.0 | Burst 1 @ 0 | 1.0 | Sprite, Unaligned/FaceCamera | `Part01` |
| 1 | `Bomb_Glow_02` | **DISABLED** | System / Infinite | 1.0 | Burst 1 @ 0 | 1.0 | Sprite, Unaligned/FaceCamera | `Part01` |
| 2 | `Bomb_Glow_03` | **DISABLED** | System / Infinite | 1.0 | Burst 1 @ 0 | 1.0 | Sprite, Unaligned/FaceCamera | `Part02` |
| 3 | `Raimbow` *(sic)* | **DISABLED** | System / Infinite | 1.0 | Burst 1 @ 0 | 1.0 | Sprite, Unaligned/FaceCamera | `Rainbow` |
| 4 | `BigArrow` | yes | **System / Infinite** | **1.0** | Burst **1** @ 0 | **1.5** | Sprite, **VelocityAligned**/FaceCamera | `Arrows` |
| 5 | `Sparkles_Dark` | yes | **Self / Once** | **0.3** | Burst **7** @ 0 **+ Rate 20/s** | rand `[unresolved]` | Sprite, Unaligned/FaceCamera | `Part01_Bright` |
| 6 | `Ring` | yes | **Self / Once** | **0.3** | Burst **3** @ 0 **+ Rate 5/s** | rand 0.3–0.7 | Sprite, Unaligned/FaceCamera | `Ring01` |
| 7 | `Sparkles_Bright` | yes | **Self / Once** | **0.3** | Burst **7** @ 0 **+ Rate 20/s** | rand `[unresolved]` | Sprite, Unaligned/FaceCamera | `Part03_Bright` |
| 8 | `Flames` | yes | **Self / Once** | **0.3** | Burst **5** @ 0 | rand `[unresolved]` | Sprite, Unaligned/FaceCamera, **SubUV 2×2** | `Flames01` |
| 9 | `Slash` | yes | **Self / Once** | **0.3** | Burst **7** @ 0 **+ Rate 20/s** | rand `[unresolved]` | **Mesh** (`SM_VFX_Slash02`), Facing Default | `Slash04` (renderer override) |

**The four disabled emitters are listed deliberately.** They are the *entire* `Bomb_Glow_*` + `Raimbow`
stack that `NS_BuffCast` and `NS_HealCast` run enabled, with identical parameters — the Debuff variant
was authored by disabling the warm/rainbow layers of the Buff variant. Their absence from a recreation
is a recorded decision, not an oversight. Their curves are transcribed in §5 anyway, so a future
session that wants the enabled variant does not re-do the archaeology.

### Cadence: three different shapes in one system

- **`BigArrow`** — `Life Cycle Mode = System`, `Loop Behavior = Infinite`, Loop Duration 1.0, one burst
  particle per loop, lifetime 1.5. Loops forever.
- **Five one-shot emitters** (`Sparkles_Dark`, `Ring`, `Sparkles_Bright`, `Flames`, `Slash`) —
  `Life Cycle Mode = **Self**`, `Loop Behavior = **Once**`, Loop Duration **0.3**. They fire once and
  stop.
- **Four of those five ALSO carry a `Spawn Rate` module** on top of the burst
  (`Sparkles_Dark` 20/s, `Ring` 5/s, `Sparkles_Bright` 20/s, `Slash` 20/s). Over a 0.3 s single loop
  that is 6, 1.5, 6 and 6 additional particles respectively `[inferred — rate × duration arithmetic]`.

**Particles per firing (enabled emitters only):**
burst 1 + 7 + 3 + 7 + 5 + 7 = **30**, plus ≈ 6 + 1.5 + 6 + 6 ≈ **19.5** from the four rate modules ⇒
**≈ 50 per firing** `[inferred]`. The burst-only count of **30** is the exact `[corpus]` number.

`Loop Count Limit = 1` with `UseLoopCountLimit = false` again everywhere — inert, same trap as the
siblings.

> ### `[unresolved: lifetime on five emitters]`
> `Sparkles_Dark`, `Sparkles_Bright`, `Flames`, `Slash` all carry `Lifetime Mode = Random` with
> `Initialize Particle.Lifetime Min/Max` **and** `[override] Lifetime = dyn:Random Range Float` whose
> `RandomRangeFloat Min/Max` is `0.2 / 0.4` on all four. The module's own ranges are:
>
> | Emitter | `Lifetime Min/Max` | override `RandomRangeFloat` |
> |---|---|---|
> | `Sparkles_Dark` | **1 / 1.5** | 0.2 / 0.4 |
> | `Sparkles_Bright` | **1 / 1.5** | 0.2 / 0.4 |
> | `Flames` | **1 / 2** | 0.2 / 0.4 |
> | `Slash` | **1 / 1.5** | 0.2 / 0.4 |
>
> Random mode reads `Lifetime Min/Max`, so **1–1.5 / 1–2 is the likely truth** and the dynamic input
> is a leftover `[inferred]`. The gap is 3–7×, so this must be settled in the editor before any
> behavior is authored. `Ring` is unambiguous: Random mode, Min 0.3 / Max 0.7, no override.

---

## 3. Mesh geometry — `SM_VFX_Slash02` `[corpus, measured from the .obj]`

`meshes/.../SM_VFX_Slash02.json`: **196 vertices, 192 triangles, 2 UV channels.**
Bounds min `(-4.146598815917969, -213.81114196777344, -4.824250936508179e-06)`,
max `(203.29752349853516, 0.0041351318359375, 0.100008275359869)`,
size `(207.44412231445312, 213.81527709960938, 0.10001309961080551)`. `uv0` covers `(0,0)`–`(1,1)` fully.
Section 0 (192 tris) names `M_VFX_DisAdd_Slash01` in slot `WorldGridMaterial` — **but the Niagara mesh
renderer overrides it to `M_VFX_DisAdd_Slash04`**, so the mesh's own material is not what draws.

**This is NOT the same carrier as `SM_VFX_Slash01`** (NS_BasicAttack's 805-vert / 960-tri full-360°
annulus). It is a much simpler **tapered quarter-arc ribbon**, and a Python parse of the OBJ
(`v`/`vt`/`f`, vertices paired to UVs through the face triplets) gives its exact profile:

- **Two flat sheets** at `z = 0` and `z = 0.1` (98 paired vertices each; thickness 0.1 — 0.05 % of the
  222-unit radius). 48 quad segments × 2 tris × 2 sheets = 192 triangles.
- **`u` runs ALONG the arc**, 49 discrete columns evenly spaced at Δu ≈ 0.02083 (= 1/48).
  **`v` runs ACROSS the band: `v = 0` is the OUTER edge, `v = 1` the INNER edge**
  (mean radius 182.39 at v ≈ 0, 161.05 at v ≈ 1).
- The arc sweeps **angle −90° → 0°** in the XY plane (counter-clockwise from −Y to +X), starting at the
  origin and spiralling outward.

Measured per-column profile (mid-line radius and band width, both sheets identical):

| u | mid angle (°) | mid radius | band width | outer r | inner r |
|---|---|---|---|---|---|
| 0.0000 | (origin — angle undefined) | 2.70 | **5.40** | — | — |
| 0.0625 | −87.62 | 44.37 | 11.47 | | |
| 0.1250 | −84.29 | 82.92 | 17.55 | | |
| 0.1875 | −80.97 | 116.27 | 23.62 | | |
| 0.2500 | −77.51 | 144.68 | 29.70 | | |
| 0.3125 | −73.87 | 168.34 | 35.77 | | |
| 0.3750 | −70.03 | 187.45 | 41.85 | | |
| 0.4375 | −65.97 | 202.17 | 47.92 | | |
| **0.5000** | **−61.82** | **213.29** | **51.02 (max)** | | |
| 0.5625 | −57.22 | 219.29 | 47.92 | | |
| 0.6250 | −52.20 | 222.44 | 41.85 | | |
| 0.6875 | −46.49 | 222.34 | 35.77 | | |
| 0.7500 | −39.93 | 219.36 | 29.70 | | |
| 0.8125 | −32.29 | 214.17 | 23.62 | | |
| 0.8330 | −29.45 | 212.13 | 21.60 | 222.73 | 201.53 |
| 0.8750 | −23.27 | 207.88 | 17.55 | 216.52 | 199.24 |
| 0.9165 | −16.34 | 203.88 | 13.50 | 210.56 | 197.20 |
| 0.9580 | −8.58 | 200.83 | 9.45 | 205.54 | 196.12 |
| **1.0000** | **0.00** | **199.60** | **5.40** | **202.30** | **196.90** |

**The width taper is exactly symmetric about u = 0.5** (5.40 … 51.02 … 5.40) and is piecewise-linear
except for a slight rounding across the four columns nearest the peak (47.92, 49.60, 50.66, 51.02,
50.66, 49.60, 47.92). The mid-line radius grows steeply to ≈ 222 by u ≈ 0.65 and then eases back to
199.6 — the shape is a **comma / claw**, not a constant-radius annulus arc.

This profile is enough to regenerate the mesh procedurally the way NS_BasicAttack §8.2 regenerated the
crescent, with the same UV convention (`u` along, `v` outer→inner) and the same "build one flat sheet
and rely on `_TwoSided`" simplification.

---

## 4. Material family and per-instance deltas `[corpus]`

**All nine materials are instances of `/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd`.**
`MD_Surface`, `BLEND_Translucent`, `MSM_Unlit`, outputs `EmissiveColor` + `Opacity`, dynamic channels
`[dissolve, distortion, offset, core_color]`, 161 expressions each. `twoSided: false` on all except
**`Slash04` (`twoSided: true`)**.

Reference = `M_VFX_DisAdd_Part01`; its absolute defaults are transcribed in [NS_BuffCast.md](NS_BuffCast.md) §4.

| Material | Δ vs `Part01` | Used by |
|---|---|---|
| `M_VFX_DisAdd_Arrows` | `Brightness` 1 → **10**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Arrow_01` | BigArrow |
| `M_VFX_DisAdd_Part01_Bright` | `Brightness` 1 → **10**; `Core_Intensity` 0 → **1**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Part_02` | Sparkles_Dark |
| `M_VFX_DisAdd_Ring01` | `Brightness` 1 → **10**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Ring_01` | Ring |
| `M_VFX_DisAdd_Part03_Bright` | `Brightness` 1 → **10**; **`CamOffset` 0 → 50**; `Core_Intensity` 0 → **1**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Part_03` | Sparkles_Bright |
| `M_VFX_DisAdd_Flames01` | `Brightness` 1 → **10**; `Core_Intensity` 0 → **1**; `Dissolve` 0 → **−0.1**; `Dissolve_Scale_X/Y` 1 → **2**; `Distortion_Intensity` 0 → **0.5**; `Distortion_Scale_X/Y` 1 → **2**; `Distortion_Speed_X/Y` 0 → **−0.3**; `Glow_Intensity` 1 → **2**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Main_Tex`/`Color_Tex` → `T_VFX_Wind_01`; `Dissolve_Tex`/`Distortion_Tex` → `T_VFX_Noise_04`; **`Color_Core` `RGBA(1,1,1,0)` → `RGBA(0.015996, 0.014444, 0.014444, 1)`** | Flames |
| `M_VFX_DisAdd_Slash04` | `Brightness` 1 → **5**; `Dissolve` 0 → **−0.1**; `Dissolve_Speed_X` 0 → **−0.1**; `Dissolve_Speed_Y` 0 → **−0.1**; `Gradient_Invert` 0.5 → **0**; `MainTex_Scale_Y` 1 → **1.2**; `Opacity_Boldness` 0.5 → **1**; **`twoSided: true`** | Slash (mesh) |
| `M_VFX_DisAdd_Part01` | (reference) | Bomb_Glow_01/02 — **disabled** |
| `M_VFX_DisAdd_Part02` | `Glow_Intensity` 1 → **0.3**; textures → `T_VFX_Part_02` | Bomb_Glow_03 — **disabled** |
| `M_VFX_DisAdd_Rainbow` | see [NS_BuffCast.md](NS_BuffCast.md) §4 (the LUT instance) | Raimbow — **disabled** |

**`M_VFX_DisAdd_Flames01` is by far the most parameterized instance in this batch** — it is the only
one that drives `Distortion_Intensity`, `Distortion_Speed`, non-unit `Dissolve_Scale`, and a
**non-white `Color_Core`**. Every one of those except `Color_Core` IS already plumbed in
`CkUsf_Look_DissolveAdd`; `CoreColor` is plumbed too, so `Flames01` is the *best*-covered material here.

**`M_VFX_DisAdd_Slash04` is the SAME instance NS_BasicAttack already recreated** as the `SlashDisAdd04`
look (its §9 defaults table: ShapeTex SoftParticle, DissolveTex SoftParticle, Brightness 5,
DissolveSpeed (−0.1, 0), Bias −0.1, MainTexScale (1, 1.2)). **Two corpus discrepancies to reconcile at
implementation time:**
- NS_BasicAttack §4 records `Dissolve_Speed_X −0.1` only; this export reads **both** `Dissolve_Speed_X`
  **and** `Dissolve_Speed_Y` as −0.1. The `SlashDisAdd04` look currently ships `(−0.1, 0)`.
- NS_BasicAttack §4 records `Main_Tex = T_VFX_Part_01`; this export agrees
  (`Main/Color/Dissolve_Tex` are all family-default `T_VFX_Part_01` — no texture delta row appears).

The look can be **reused verbatim**, but the `DissolveSpeedY` value should be re-read off the corpus
before trusting either recipe.

### Referenced textures `[corpus]`

| Texture | Size | Source format | Compression | sRGB | Address |
|---|---|---|---|---|---|
| `T_VFX_Arrow_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Part_01` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Part_02` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Part_03` | 512×512 | TSF_G8 | TC_Alpha | false | **Wrap/Wrap** |
| `T_VFX_Ring_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Wind_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Noise_02` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Noise_04` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_LUT_Rainbow_01` | 512×2 | TSF_BGRA8 | TC_Default | **true** | Wrap/Wrap (disabled emitter only) |
| `T_VFX_WhitePixel` | 1×1 | TSF_RGBA16 | TC_Default | true | Wrap/Wrap |

**`T_VFX_Wind_01` is the Flames flipbook sheet** — the renderer declares `SubUV: 2x2`, so this 512²
texture is a 2×2 atlas of four 256² frames. Its metadata does not say so; only the renderer does.

---

## 5. Per-layer runtime curves `[corpus]`

Curves sample **NormalizedAge** unless stated. `C` = constant (step) key, `L` = linear key.
`Dynamic Material Parameters` writes **Index 0 only** on every emitter.

### Layer 4 — `BigArrow` (enabled; burst 1 @ t = 0, lifetime 1.5, infinite loop)
- Initialize Color `RGBA(1, 1, 1, 1)`; **Position Offset `(0, 0, 50)`** with `UsePositionOffset = true`
  (the Buff sibling's arrow starts *below* the origin at −52.2087; this one starts **above** it).
- `Add Velocity`: **`(0, 0, -150)`** — **downward**, where `NS_BuffCast`'s `BigArrow` goes **up** at
  `(0, 0, 150)`. That sign flip is the whole Buff↔Debuff read.
- Sprite Size Mode Non-Uniform, `Sprite Size (170, 240)`; `Sprite UV Scale (1, 1)`; `Uniform Sprite Size 300` not selected.
- Sprite Rotation Mode Random, authored angle **3.07129**, Min 0 / Max 360.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.3, 0.05)C`.
- Solve Forces and Velocity: both limiters disabled, Rotational Solver on.
- Color from Curve — **near-black, a "dark arrow"**:
  - `R: (0, 0.00233049)C (0.460006, 0.00116306)L (1, 0.00182116)C`
  - `G: (0, 0.00139829)C (0.460006, 0.00132921)L (1, 0.00182116)C`
  - `B: (0, 0.005)C (0.460006, 0.005)L (1, 0.00212469)C`
  - `A: (0, 0)C (0.15575, 1)L (0.557803, 1)L (1, 0)C`
- **No size curve** (5 update modules).
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 5 — `Sparkles_Dark` (burst 7 @ 0 + rate 20/s, Self/Once 0.3 s)
- Lifetime `[unresolved]` — `Lifetime Min 1 / Max 1.5` vs override `RandomRangeFloat 0.2 / 0.4`.
- **`Sphere Location`, `Surface Only = true`, `Surface Expansion Mode = Outside`,
  `Sphere Radius` 200**, `Radius Position 1`, `U Position 0`, `V Position 0.5`,
  `Uniform Distribution 1`, `Uniform Spiral Amount 1`, Random distribution, Spawn Only,
  Offset (0,0,0), Band Thickness 0. Particles start on a **200-unit sphere shell**.
- **`Add Velocity from Point`: `Velocity Strength` = −700** (a constant, not a random range),
  Origin Offset (0,0,0), Falloff Distance 100. Negative strength ⇒ particles **implode toward the
  centre** from the shell. This is the Debuff signature.
- Sprite Size Mode Random Uniform: **Min 7, Max 20**. Sprite Rotation Random, authored 90, Min 0 / Max 360.
- Initialize Color `RGBA(1, 1, 1, 1)`.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.1)C (1, 3.91223e-08)C`.
- **`Curl Noise Force`**: `Noise Strength` **2500**, `Noise Frequency` **15**, `Random Seed` **11**,
  `Randomization Vector (0.65, 0.125, 0.37)`, `Pan Noise Field (0,0,0)`,
  `Cone Mask Angle` 45 / `Falloff Angle` 45.
- Color from Curve — **near-black again**:
  - `R: (0.113492, 0.00102734)C (0.385149, 0.000465223)L`
  - `G: (0.113492, 0.002)C (0.385149, 0.000531684)L`
  - `B: (0.113492, 0.00105217)C (0.385149, 0.002)L`
  - `A: (0, 0)L (0.0857229, 1)C (1, 0)L`
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`.
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 6 — `Ring` (burst 3 @ 0 + rate 5/s, Self/Once 0.3 s)
- Lifetime Mode Random, **Min 0.3 / Max 0.7** — unambiguous (no override).
  (`InitializeParticle.Lifetime = 1` is present but inert in Random mode.)
- Sprite Size Mode Random Uniform: **Min 220, Max 250** (`Uniform Sprite Size 200` not selected).
- Sprite Rotation Random, Min 0 / Max 360. Initialize Color `RGBA(1, 1, 1, 1)`.
- Color from Curve — **a constant near-black with an alpha triangle**:
  - `R: (0, 0.00182116)C` *(single key)*
  - `G: (0, 0.00182116)C` *(single key)*
  - `B: (0, 0.00212469)C` *(single key)*
  - `A: (0, 0)C (0.499849, 1)L (1, 0)C`
- Dynamic param 1 animated: `Float from Curve` **`(0, 0)C (1, -0.5)C`** — dissolve slides 0 → −0.5
  (the Buff sibling's Ring runs −0.325 → −0.5). Params 2/3/4 constant 0.
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (0.1, 0.9)C (1, 1)C`.

### Layer 7 — `Sparkles_Bright` (burst 7 @ 0 + rate 20/s, Self/Once 0.3 s)
- **`Color Mode = Random Range`** — colour is a per-particle lerp between
  `Color Minimum RGBA(0.093059, 0.181164, 0.0953075, 1)` (a dark green) and
  `Color Maximum RGBA(0.111932, 0.0409152, 0.3564, 1)` (a dark violet).
  (`InitializeParticle.Color RGBA(1,1,1,1)` is present but not used in Random Range mode.)
- Lifetime `[unresolved]` — `Lifetime Min 1 / Max 1.5` vs override `RandomRangeFloat 0.2 / 0.4`.
- `Sphere Location`: **`Surface Only = false`**, `Sphere Radius` **5** (a near-point, unlike layer 5's 200),
  `Surface Expansion Mode = Outside`, Random, Spawn Only.
- `Add Velocity from Point` = `Random Range Float 001` **Min 300 → Max 1000** (outward).
- Sprite Size Mode Random Uniform: **Min 20, Max 70**. Sprite Rotation Random, authored 90.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.1)C (1, 3.91223e-08)C`.
- `Curl Noise Force`: identical settings to layer 5 (Strength 2500, Frequency 15, Seed 11,
  Randomization Vector (0.65, 0.125, 0.37), cone 45/45).
- **No `Color` update module** — the spawn-time Random Range colour holds for the whole life.
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0.5)C`.
- Scale Color, `Scale Mode = RGB and Alpha Separately`, `Scale Alpha` (Float from Curve):
  **`(0, -7.86015e-08)L (0.2, 1)L (1, 0)L`** (first key is numerically zero).
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 8 — `Flames` (burst 5 @ 0, Self/Once 0.3 s) — **the sub-UV layer**
- Lifetime `[unresolved]` — `Lifetime Min 1 / Max 2` vs override `RandomRangeFloat 0.2 / 0.4`.
- `Sphere Location`: `Surface Only = true`, `Surface Expansion Mode = Outside`,
  **`Sphere Radius` 20**, `Radius Position 1`, `V Position 0.5`, `Uniform Distribution 1`, `Uniform Spiral Amount 1`.
- **`Sub UV Animation`, `SubUV Animation Mode = Random`**, `Start Frame 0`, `End Frame 3`,
  `SubUV Loop Count 1` — one pass through a 4-frame (2×2) flipbook, **entered at a random frame**.
- Sprite Size Mode Random Uniform: **Min 200, Max 300** — the largest sprites in the system.
- Sprite Rotation Random, authored 90, Min 0 / Max 360. Initialize Color `RGBA(1, 1, 1, 1)`.
- **No velocity module at all** — flames stay on the 20-unit shell where they spawn.
- Color from Curve:
  - `R: (0.113492, 0.175111)C (0.545729, 0.0144)L (0.984002, 0.00560539)L`
  - `G: (0.113492, 0.250158)C (0.545729, 0.0144)L (0.984002, 0.00802319)L`
  - `B: (0.113492, 0.175111)C (0.545729, 0.048)L (0.984002, 0.00560539)L`
  - `A: (1, 1)L` *(single key — alpha flat 1)*
  A sickly desaturated green (`G` above `R`/`B` at t = 0.113) fading to near-black.
- Dynamic param 1 animated: `Float from Curve` **`(0, -4.10064e-08)C (1, -1)C`** — dissolve 0 → −1.
  **`Index 0 Param 2 = 10` constant** — the distortion channel is driven at **10**, by far the
  largest distortion in the batch, and `Flames01` is the one material with `Distortion_Intensity` 0.5.
  `Param 3`/`Param 4` constant 0.
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (1, 1)C`.
- **`Sprite Rotation Rate` = `Random Range Float 001`, Min −45 / Max 45** (°/s, per-particle constant).

### Layer 9 — `Slash` (burst 7 @ 0 + rate 20/s, Self/Once 0.3 s) — **the mesh layer**
- **`Color Mode = Random Range`**: `Color Minimum RGBA(0.0154102, 0.03, 0.0157825, 0.45)` (dark green),
  `Color Maximum RGBA(0.00942192, 0.00344404, 0.03, 0.45)` (dark violet). Note **alpha 0.45 on both ends**.
- Lifetime `[unresolved]` — `Lifetime Min 1 / Max 1.5` vs override `RandomRangeFloat 0.2 / 0.4`.
- **`Mesh Scale Mode = Random Non-Uniform`**: `Mesh Scale Min (0.5, 1, 0.5)` → `Max (1.5, 1, 1.5)`
  (Y is pinned at 1). `Mesh Uniform Scale Min 1 / Max 2` present but not selected.
- **`Initial Mesh Orientation`, `Rotation Coordinate Space = Mesh`,
  `[override] Rotation = Random Range Vector` Min `(-1, -1, -1)` → Max `(1, 1, 1)`**,
  `Orientation Axis (1, 0, 0)`, `Orientation Vector (1, 0, 0)`.
  `[unresolved: the units of the Rotation vector]` — a ±1 range is not degrees and is unlikely to be
  a full ±180°; it is most plausibly a normalized rotator scale the module maps internally. Unlike
  NS_BasicAttack's `Slash_*` emitters (a CONSTANT mesh orientation), **this layer's orientation is
  randomised per particle**, which is the visually load-bearing fact regardless of the units.
- **`Sphere Location` is DISABLED** — its `Sphere Radius 200` / `Surface Only true` values are inert.
  So every Slash mesh spawns at the system origin with `Position Offset (0,0,0)`.
- `Mesh Renderer Info` is a `NiagaraDataInterfaceMeshRendererInfo` override on Initialize Particle.
- Solve Forces and Velocity present (no velocity source, so inert).
- **No `Color` update module** — spawn-time Random Range colour holds.
- Dynamic params: `Param 1 = 0.1` constant, `Param 2 = 0` constant, `Param 4 = 0` constant, and
  **`Param 3` (the `offset` / UV-pan channel) animated**:
  `Float from Curve` **`(0, -0.5)C (0.3, 0.3)C (1, 0.5)C`** — the streak sweeps along `u` from −0.5
  through 0.3 at t = 0.3 to 0.5. Given §3's `u`-along-the-arc UV, **this is what makes the claw
  streak sweep**, exactly the mechanism NS_BasicAttack §3 identified.
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C` — **inert on a mesh renderer**
  (`Particles.SpriteSize` is not read by `NiagaraMeshRendererProperties`) `[inferred]`.
- Scale Color, `Scale Mode = RGB and Alpha Separately`; its `Linear Color Curve` override is **empty on
  all four channels** and `Scale Alpha` = `Float from Curve 001` **`(0, -0)L (0.2, 1)L (1, 0)L`**.

### Layers 0–3 — the DISABLED `Bomb_Glow_*` + `Raimbow` stack
Parameters identical to `NS_BuffCast`'s layers 0–3 (§5 there): colours
`RGBA(1, 0.184475, 0.386429, 0.4)` / `RGBA(0.913099, 0.0193824, 0.130136, 0.4)` /
`RGBA(0.313989, 0, 0.00227652, 0.483)` / `RGBA(0.913099, 0.913099, 0.913099, 0.1)`; uniform sizes
500 / 400 / 200 / 450; lifetime 1; Scale RGBA `RGB flat 1, A (0, 1)L (1, 0)L` (Raimbow: `RGB (0, 0.5)L`);
size curves `(0, 0.5)C (0.1, 1)L (1, 1)L` (Raimbow: `(0, 0.5)C (0.2, 0.9)C (1, 1)L`);
dynamic param 1 = 1 / 0 / 2 / 0.5. **All four are disabled — do not recreate them.**

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new row is required, and the source's cadence does not fit the table's shape.** The enabled
emitters split into two incompatible cadences:

- `BigArrow`: infinite 1.0 s loop, 1 particle, lifetime 1.5.
- Five one-shot `Self / Once` emitters on a 0.3 s single loop, four of them adding a spawn *rate* on
  top of their burst.

`FCk_ParticlesTemplateSpec` today expresses one `(LoopDuration, ParticleLifetime, BurstCount)` triple
per row. It cannot express "burst N then also stream at R/s for D seconds", nor "one emitter loops
forever while five fire once".

The pragmatic plan:

- **Row**: loop **1.0 s**, particle lifetime **1.5 s** (the longest layer), burst **30** (the exact
  corpus burst total across enabled emitters). Partition `Seed % 30`:
  1 = BigArrow, 7 = Sparkles_Dark, 3 = Ring, 7 = Sparkles_Bright, 5 = Flames, 7 = Slash.
- **The four `Spawn Rate` modules are DROPPED** in that plan, costing ≈ 19.5 particles per firing
  (≈ 40 % of the visible count). Alternative: widen the burst to ≈ 50 and give each rate-fed layer
  extra slots with a `frac`-derived spawn delay across the 0.3 s window — this is *closer* but is an
  approximation either way, and the burst-vs-stream difference is exactly the kind of cadence fake
  `CkParticles/CLAUDE.md` warns against. **Make it a recorded decision.**
- **The `Self / Once` semantics are LOST**: the CkParticles template loops. The recreation will re-fire
  every 1.0 s where the source fires once. For a "Cast" effect that is spawned per-cast and destroyed,
  this may not matter — but it must be checked against how the caller spawns it.

### 6.2 VisTag / renderer needs

| Source emitter | Renderer needed | Available today? |
|---|---|---|
| BigArrow | `VelocityAlignedSprite` + look | **yes** |
| Sparkles_Dark, Ring, Sparkles_Bright | camera-facing sprite, each with a distinct look | **NO — new kind** |
| Flames | camera-facing sprite + look + **2×2 sub-UV flipbook** | **NO — new kind AND sub-UV** |
| Slash | `Mesh` + one generated mesh + look | **yes** |

| Renderer | Kind | Mesh | Look | Source |
|---|---|---|---|---|
| a | VelocityAlignedSprite | — | `ArrowsDisAdd` | BigArrow |
| b | CameraFacingSprite *(NEW KIND)* | — | `PartDisAdd01Bright` | Sparkles_Dark |
| c | CameraFacingSprite *(new kind)* | — | `RingDisAdd01` | Ring |
| d | CameraFacingSprite *(new kind)* | — | `PartDisAdd03Bright` | Sparkles_Bright |
| e | CameraFacingSprite *(new kind)* + **SubUV 2×2** | — | `FlamesDisAdd01` | Flames |
| f | Mesh | `SlashClaw` (new, §3) | `SlashDisAdd04` — **already exists** | Slash |

Six row renderers, four needing the new kind, one additionally needing sub-UV. VisTags allocate above
`Get_RosterVisTag_Max()`.

### 6.3 Look / material needs

Six looks; **one (`SlashDisAdd04`) already exists verbatim** from NS_BasicAttack (subject to the
`DissolveSpeedY` reconciliation in §4). `ArrowsDisAdd`, `PartDisAdd01Bright` are shared with the other
sheets in this batch.

Unplumbed family parameters that bite here:

| Param | Value here | Plumbed? |
|---|---|---|
| `Core_Intensity` | 1 on `Part01_Bright`, `Part03_Bright`, `Flames01` (three of six looks) | **no** |
| `Glow_Intensity` | 2 on `Flames01` | **no** |
| `Gradient_Invert` | 0 on `Ring01`, `Flames01`, `Slash04` | **no** |
| **`CamOffset`** | **50 on `Part03_Bright`** | **no** |
| `Opacty_DepthFade` | 20 default everywhere here | **no** (inert-ish — all at default) |

`CamOffset 50` is new to this batch and is a *geometric* parameter (a camera-ward world-position
offset in the material), not a shading one. It is what stops `Sparkles_Bright` z-fighting with the
other layers. Omitting it is a real, visible deviation, not a shading nuance.

`Flames01`'s `Distortion_Intensity 0.5` + `Distortion_Speed (−0.3, −0.3)` + `Dissolve_Scale (2, 2)` +
`Color_Core (0.015996, 0.014444, 0.014444)` are **all already plumbed** — this look should port cleanly.

### 6.4 Mesh and texture needs

- **New procedural mesh `SM_CkParticles_SlashClaw`** from §3's measured table: 48 arc segments, one
  flat sheet (rely on `_TwoSided` from `Slash04`), `u` along the arc, `v` outer→inner, symmetric width
  taper 5.40 → 51.02 → 5.40, mid-line radius per the table. Do **not** import `SM_VFX_Slash02`.
- Textures: `T_VFX_Part_01` → existing `SoftParticle`; `T_VFX_Noise_02` → existing `TileNoise`;
  `T_VFX_Noise_04` → **unmeasured**, may map to `TileNoise` or need its own bake.
  New measurement + bake needed for `T_VFX_Part_02`, `T_VFX_Part_03`, `T_VFX_Arrow_01`,
  `T_VFX_Ring_01`, and **`T_VFX_Wind_01` as a 2×2 FLIPBOOK ATLAS** (four distinct flame frames in one
  512² image — the procedural generator has never produced a multi-frame atlas).

### 6.5 CAPABILITY GAPS — read before committing a session

1. **SUB-UV / FLIPBOOK — does not exist anywhere in the pipeline.** `Flames` declares `SubUV: 2x2` on
   its renderer and drives it with a `Sub UV Animation` module (`Mode = Random`, frames 0–3, one loop).
   CkParticles has: no `SubImageSize` on any renderer, no `Particles.SubImageIndex` output on the DI,
   no atlas support in the procedural texture generator, and no sub-UV field on
   `FCk_ParticlesRendererSpec`. **Recreating this layer needs four additions.** Approximating it with a
   static frame is possible and must be recorded as a deviation — but "flames that don't animate" is
   a visible loss on the layer carrying the effect's name.

2. **CAMERA-FACING SPRITE ROW RENDERER — does not exist.** Four layers here. Shared batch need.

3. **`Self / Once` EMITTER LIFECYCLE — not expressible.** Five of the six enabled emitters fire once
   and stop; every CkParticles template loops. See §6.1.

4. **`Spawn Burst` + `Spawn Rate` ON THE SAME EMITTER — not expressible.** Four emitters do this.
   `FCk_ParticlesTemplateSpec` has one `BurstCount` and no rate.

5. **PER-PARTICLE RANDOM MESH ORIENTATION.** The DI *does* output `OutOrientation` (a quat), so this is
   expressible — but NS_BasicAttack's `Behavior_Slash` writes a **constant** quat, so there is no
   precedent for a randomised one, and the `Random Range Vector (-1,-1,-1)..(1,1,1)` rotation's units
   are `[unresolved]` (§5, layer 9). Budget archaeology, not just code.

6. **`Curl Noise Force`** (`Sparkles_Dark`, `Sparkles_Bright`; Strength 2500, Frequency 15, Seed 11).
   Not a pipeline gap — but it is a **stateful, accumulating** force with no closed form. The module's
   GPU/CPU-lockstep rule plus NS_BasicAttack's closed-form-integration lesson mean this must either be
   (a) reimplemented as a deterministic Age-parameterized 3D noise displacement added to position
   (feasible: `Common.ush` would gain a curl-noise helper, mirrored in C++), or (b) dropped. Do not
   step-integrate it.

7. **`Color Mode = Random Range`** (`Sparkles_Bright`, `Slash`). Trivially expressible with
   `CkParticles_Rand`; no gap. Note it is a **per-channel independent** lerp or a single-t lerp
   `[unresolved: which]` — Niagara's Random Range colour mode detail is not in the corpus.

8. **NEGATIVE `Add Velocity from Point`** (`Sparkles_Dark`, strength −700 from a 200-unit shell).
   Expressible; just note the implosion direction is the Debuff signature and easy to get backwards.

9. **WORLD SPACE vs LOCAL SPACE.** All emitters `LocalSpace: false`; template is local. Same recorded
   deviation as NS_BasicAttack §13.2.

10. **DISABLED EMITTERS.** Four are disabled and must NOT be recreated. Their curves are in §5 so a
    future "enabled variant" needs no new archaeology — that is the only reason they are there.

### 6.6 Behavior id

**Do NOT allocate an id here.** `ck::particles::NumBehaviors` is 18 at the time of writing.

### 6.7 Complexity assessment

**Tier L** — sub-UV flipbook (gap 1) alone puts it there, and it is compounded by the
`Self/Once` + `burst-plus-rate` cadence (gaps 3–4) and the curl-noise force (gap 6).
**This is the highest-effort effect in the batch.** It is also the one with the most reusable
by-product: the `SlashClaw` mesh and a sub-UV capability would both serve future recreations.

---

## 7+. Reserved for implementation — sections 7–14 per [README.md](README.md) are written by the session that implements this effect.
