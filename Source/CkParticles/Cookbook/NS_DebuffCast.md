# Recipe: NS_DebuffCast → CkParticles (IMPLEMENTED)

Schema and evidence-tag conventions: [README.md](README.md). Exemplars: [NS_BasicAttack.md](NS_BasicAttack.md),
[NS_Lightning_Range.md](NS_Lightning_Range.md).

## Completion state — READ FIRST

**Status: IMPLEMENTATION-COMPLETE (2026-08-02) — behavior 32. Human A/B parity NOT yet judged.**

`Behavior_DebuffCast.ush` + `ExecuteStage_CPU` case 32, the `PS_CkParticles_Template_DebuffCast`
cadence row (burst 30 **+ rate 65/s**) with six row renderers on VisTags 113–118, the new procedural
`SM_CkParticles_SlashClaw` carrier, `Test_Particles_DebuffCastBehavior.cpp`, and the **DEBUFF CAST**
pair in the VfxExamples gym. This port adds **no** look and **no** texture — every paint and paint
parameterization it needs already existed. It is the cookbook's **first consumer of C10's curl noise**.
§12's walk is `[HUMAN-VERIFY]` and open.

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
| 5 | `Sparkles_Dark` | yes | **Self / Once** | **0.3** | Burst **7** @ 0 **+ Rate 20/s** | rand **1.0–1.5** `[corpus-v3]` | Sprite, Unaligned/FaceCamera | `Part01_Bright` |
| 6 | `Ring` | yes | **Self / Once** | **0.3** | Burst **3** @ 0 **+ Rate 5/s** | rand 0.3–0.7 | Sprite, Unaligned/FaceCamera | `Ring01` |
| 7 | `Sparkles_Bright` | yes | **Self / Once** | **0.3** | Burst **7** @ 0 **+ Rate 20/s** | rand **1.0–1.5** `[corpus-v3]` | Sprite, Unaligned/FaceCamera | `Part03_Bright` |
| 8 | `Flames` | yes | **Self / Once** | **0.3** | Burst **5** @ 0 | rand **1.0–2.0** `[corpus-v3]` | Sprite, Unaligned/FaceCamera, **SubUV 2×2** | `Flames01` |
| 9 | `Slash` | yes | **Self / Once** | **0.3** | Burst **7** @ 0 **+ Rate 20/s** | rand **1.0–1.5** `[corpus-v3]` | **Mesh** (`SM_VFX_Slash02`), Facing Default | `Slash04` (renderer override) |

**The four disabled emitters are listed deliberately.** They are the *entire* `Bomb_Glow_*` + `Raimbow`
stack that `NS_BuffCast` and `NS_HealCast` run enabled, with identical parameters — the Debuff variant
was authored by disabling the warm/rainbow layers of the Buff variant. Their absence from a recreation
is a recorded decision, not an oversight. Their curves are transcribed in §5 anyway, so a future
session that wants the enabled variant does not re-do the archaeology.

### Cadence: three different shapes in one system

- **System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
  `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
- **`BigArrow`** (and the four disabled `Life Cycle Mode = System` emitters) — governed by the system
  per [P0-D1], so it bursts **once** over that 2.0 s window. Its stored `Infinite / 1.0 s` row is
  inert. *(Was read as "loops forever" on a 1.0 s cycle.)* One burst particle, lifetime 1.5.
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

> ### Lifetime — RESOLVED `[corpus-v3]`
> `Sparkles_Dark`, `Sparkles_Bright`, `Flames`, `Slash` all carry `Lifetime Mode = Random` with
> `Initialize Particle.Lifetime Min/Max` **and** `[override] Lifetime = dyn:Random Range Float`
> (`0.2 / 0.4` on all four). Per [P0-D2] `Random` selects the **Min/Max** pins, so those DRIVE and
> the override on the unselected Direct-Set pin is INERT (`lifetimeResolved.source = minmax` on all
> four, override under `inertOverrides`). The 3–7× gap resolves in favour of the LONG lifetimes; no
> editor check needed.
>
> | Emitter | LIVE (Random mode) | inert override |
> |---|---|---|
> | `Sparkles_Dark` | **1.0 / 1.5** | ~~0.2 / 0.4~~ |
> | `Sparkles_Bright` | **1.0 / 1.5** | ~~0.2 / 0.4~~ |
> | `Flames` | **1.0 / 2.0** | ~~0.2 / 0.4~~ |
> | `Slash` | **1.0 / 1.5** | ~~0.2 / 0.4~~ |
>
> `Ring` is unambiguous and confirmed: Random mode, Min 0.3 / Max 0.7, no override (its stored
> `Lifetime = 1` sits under `inertValues`).

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

The look can be **reused verbatim**, and the `DissolveSpeedY` question is now settled
`[corpus, checked 2026-08-02]`: the material reads `Dissolve_Speed_X = Dissolve_Speed_Y = −0.1`, and
the shipped `SlashDisAdd04` look **already carries `(−0.1, −0.1)`**. This sheet's claim that the look
"currently ships (−0.1, 0)" was stale — it described NS_BasicAttack §4's incomplete delta ROW, not the
look. Nothing to change; the discrepancy was documentary.

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
- Lifetime `[corpus-v3]` — **`Lifetime Min 1.0 / Max 1.5` drives**; the `RandomRangeFloat 0.2 / 0.4` override is inert.
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
- **`Color.Scale Alpha` = 0.45** `[corpus, correction 2026-08-02]` — it lives only in the `[values]`
  block and was missing from this list; it is what keeps the halo faint under a full-height alpha
  triangle. Same class as the four found in [NS_HealCast.md](NS_HealCast.md) §5 and as batch C's [P2-D2].
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
- Lifetime `[corpus-v3]` — **`Lifetime Min 1.0 / Max 1.5` drives**; the `RandomRangeFloat 0.2 / 0.4` override is inert.
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
- Lifetime `[corpus-v3]` — **`Lifetime Min 1.0 / Max 2.0` drives**; the `RandomRangeFloat 0.2 / 0.4` override is inert.
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
- Lifetime `[corpus-v3]` — **`Lifetime Min 1.0 / Max 1.5` drives**; the `RandomRangeFloat 0.2 / 0.4` override is inert.
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

- `BigArrow`: system-governed — one burst over the system's `Once / 2.0 s` loop `[corpus-v3]`,
  1 particle, lifetime 1.5. *(Was read as an infinite 1.0 s loop.)*
- Five one-shot `Self / Once` emitters on a 0.3 s single loop, four of them adding a spawn *rate* on
  top of their burst.

`FCk_ParticlesTemplateSpec` today expresses one `(LoopDuration, ParticleLifetime, BurstCount)` triple
per row. It cannot express "burst N then also stream at R/s for D seconds", nor "one emitter loops
forever while five fire once".

The pragmatic plan:

- **Row `[corpus-v3]`, per [P0-D3]**: loop **2.0 s** (the system's `Once` loop duration — *was
  1.0 s*), particle lifetime **2.0 s** (max resolved — `Flames`' 2.0 Max; *was 1.5 s under the
  override-wins assumption, which capped every randomised layer at 0.4 s*), burst **30** (the exact
  corpus burst total across enabled emitters). Partition `Seed % 30`:
  1 = BigArrow, 7 = Sparkles_Dark, 3 = Ring, 7 = Sparkles_Bright, 5 = Flames, 7 = Slash.
- **The four `Spawn Rate` modules are REPRODUCED, not dropped** `[decision 2026-08-02, batch D]`.
  Dropping them would have cost ≈ 19.5 particles per firing (≈ 40 % of the visible count), and the
  `frac`-derived-burst alternative is exactly the cadence fake `CkParticles/CLAUDE.md` warns against.
  Phase 2's C2 landed a real rate stack and C5 landed the spawn-phase input, so the row declares
  **burst 30 AND rate 65/s** and the behavior splits the two populations by spawn phase. The mapping
  and its price are in [NS_HealCast.md](NS_HealCast.md) §9.2 and §13.2; this port follows it exactly.
- **The `Self / Once` semantics are REPRODUCED as a window**, not lost: a streamed particle whose
  spawn phase falls past 0.3 s — every one of these emitters' own loop duration — is hidden, so each
  streaming layer emits over exactly its own window once per system loop. `Flames` carries no rate at
  all and so streams nothing; `BigArrow` is system-governed and bursts once.

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
   `CkParticles_Rand`; no gap. **`[unresolved]` RESOLVED 2026-08-02 from the corpus:** both emitters
   carry `Color Channel Mode = Link RGB / Link A`, so RGB shares ONE random draw and alpha takes its
   own — a single-t lerp, not three independent ones.

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

## 7. Textures — no new bake

§6.4 asked for measurement and up to five new bakes including a 2×2 flame ATLAS. **None were needed:**
every paint had already been measured off the same corpus PNG by an earlier batch, and the atlas kind
itself (`MaskSheet`) shipped with Phase 1's C4.

| Source paint | Stand-in | Measured in |
|---|---|---|
| `T_VFX_Arrow_01` | `ArrowChevron` | NS_BuffLoop §7.1 |
| `T_VFX_Part_01` | `SoftParticle` | NS_BasicAttack §7 |
| `T_VFX_Part_02` | `SoftParticleBright` | NS_FireBall_Hit §7 |
| `T_VFX_Part_03` | `SoftParticleFine` | NS_FireBall_Hit §7 |
| `T_VFX_Ring_01` | `RingUneven` | NS_FireBall_Hit §7 |
| `T_VFX_Wind_01` | `WindSheet` — the 2×2 four-frame atlas | NS_Fire §7 |
| `T_VFX_Noise_02` | `TileNoise` | NS_BasicAttack §7 |
| `T_VFX_Noise_04` | `TileNoiseCoarse` | NS_FireBall_Hit §7 (§6.4 listed it as "unmeasured"; it was measured there) |

---

## 8. Mesh — `SM_CkParticles_SlashClaw`

**New, and the cookbook's second procedural carrier built from a measured source profile.** §3's
measurement drives it directly, and the implementation went one step further than the Crescent did:
where that mesh stores a coarse 30° knot table, this one stores **all 49 of the source sheet's own
columns**, so the generated surface reproduces `SM_VFX_Slash02`'s flat sheet rather than resampling it.

- **Topology**: 1 band segment × 48 arc segments = 96 triangles, one of the source's two flat sheets.
  The second sheet is dropped because `M_VFX_DisAdd_Slash04` is `twoSided: true`.
- **Rim table, not a radius pair.** §3's "band width" is the EUCLIDEAN distance between the paired
  outer and inner vertices, and their angular positions differ — the cross-section direction ROTATES
  along the sweep (180° over the arc, non-linearly). A radius-pair table like the Crescent's cannot
  express that, so the generator stores `(OuterX, OuterY, InnerX, InnerY)` per column and lerps.
- **Verified against the source**: all 98 rim vertices reproduced to a maximum error of **5 × 10⁻⁵
  units** on a 213-unit mesh — the printed-precision floor of the table itself.
- **UV convention preserved**: `u` runs ALONG the arc, `v` across it, `v = 0` the OUTER edge. That is
  what makes the look's `offset` channel sweep the streak DOWN the claw rather than across it.

Do **not** import `SM_VFX_Slash02`; the table is the source of truth.

---

## 9. The behavior — `Behavior_DebuffCast.ush` + `ExecuteStage_CPU` case 32

### 9.1 The cadence row

| Field | Value | Source |
|---|---|---|
| `LoopDuration` | 2.0 s | the SYSTEM's `Loop Behavior = Once`, `Loop Duration = 2` ([P0-D1]) |
| `ParticleLifetime` | 2.0 s | `Flames`' resolved 2.0 s maximum on a beat of 0 ([P0-D5]) |
| `BurstCount` | 30 | 1+7+3+7+5+7 over the SIX enabled emitters |
| `SpawnRate` | 65 /s | 20+5+20+20 — `Flames` is the one Self/Once emitter with no rate |

Every enabled emitter bursts at loop start, so unlike its two siblings this port has **no spawn beat
at all**.

### 9.2 The partition

Burst particles take `Seed % 30` (0 BigArrow, 1–7 Sparkles_Dark, 8–10 Ring, 11–17 Sparkles_Bright,
18–22 Flames, 23–29 Slash); rate particles take a weighted draw over cumulative shares 20 / 25 / 45 /
65. The spawn-phase split and the window gate work exactly as [NS_HealCast.md](NS_HealCast.md) §9.2
describes.

### 9.3 The curl force — how 2500 and 15 became usable numbers

§6.5 gap 6 ruled the approach (a deterministic Age-parameterized displacement, never step-integrated)
and Phase 2's C10 shipped `CkParticles_CurlPath` for it. Neither of the source's two numbers can be
used raw, and both conversions are **derived and stated** rather than tuned:

- **Frequency.** The module authors its noise field in metres, so the source's `Noise Frequency 15`
  becomes **0.015 per unit**. `[inferred — the position scaling inside the module graph is not in the
  corpus.]`
- **Strength.** The source's `Noise Strength 2500` is an ACCELERATION, and the accumulated curl
  velocity is multiplied every frame by the layer's OWN `Scale Velocity` curve, which crushes it to
  that curve's **0.1** plateau by t = 0.2. Mean displacement over a life `L` is therefore
  `0.5 · 2500 · 0.1 · L²`; the constant-VELOCITY path that covers the same ground in the same time is
  `0.5 · 2500 · 0.1 · L`. `CurlPath` multiplies that velocity by the field, whose mean magnitude is a
  measured property of this plugin's own Fbm — **0.736** over 400 samples at this frequency and spawn
  scale — so the velocity is divided by it.

Measured result: a mean wander of **60 units** at Age 0.6 (min 12, max 112) on the 200-unit shell, and
exactly **0** at Age 0 because the path re-integrates from the spawn point every evaluation.

The **cone mask** (45° / 45°) is not implemented — §13.3.

### 9.4 Per-layer notes worth the reader's time

- **`Sparkles_Dark` IMPLODES.** `Add Velocity from Point` at a constant **−700** from a 200-unit
  shell. Its `Sparkles_Bright` twin explodes outward at 300–1000 from a 5-unit point. The two are
  otherwise near-identical, which makes the sign the easiest thing in the system to get backwards, so
  §11 asserts both directions.
- **`Slash` sits at the origin.** Its `Sphere Location` module is DISABLED, so the only things
  separating seven claws are a per-particle orientation and a non-uniform mesh scale (Y pinned at 1).
- **`Slash`'s `offset` channel sweeps −0.5 → 0.3 → 0.5**, and §3's UV puts `u` along the arc — so that
  sweep IS the streak travelling down the claw, the mechanism NS_BasicAttack §3 identified.
- **`Flames` never move.** No velocity module at all: they hold the 20-unit shell where they spawn,
  turn in place at a per-particle ±45 °/s, cycle a 2×2 flipbook entered at a random frame, and drive
  the distortion channel at a constant **10** — by far the largest distortion in the cookbook.
- **`Flames`' alpha is a single key at 1**, so like the PickupCast rings they never fade; the dissolve
  channel removes them.
- **`BigArrow` starts 50 units ABOVE the origin and falls at 150 u/s.** Its Buff sibling starts below
  and rises. Its authored `Sprite Rotation Angle 3.07129` is inert twice over — the mode is `Unset`
  and the renderer is velocity-aligned.
- **The four DISABLED emitters are not recreated.** §5 keeps their curves so a future "enabled
  variant" needs no new archaeology; that is their only purpose.

---

## 10. Looks and renderers

Six row-declared renderers on VisTags **113–118**, one per enabled source emitter.

| VisTag | Kind | Look | Source material | Serves |
|---|---|---|---|---|
| 113 | VelocityAlignedSprite | `ArrowsDisAdd` | `M_VFX_DisAdd_Arrows` | BigArrow |
| 114 | CameraFacingSprite | `PartDisAdd01Bright` | `M_VFX_DisAdd_Part01_Bright` | Sparkles_Dark |
| 115 | CameraFacingSprite | `RingDisAdd01` | `M_VFX_DisAdd_Ring01` | Ring |
| 116 | CameraFacingSprite | `PartDisAdd03Bright` | `M_VFX_DisAdd_Part03_Bright` | Sparkles_Bright |
| 117 | CameraFacingSprite, SubUV 2×2 | `FlamesDisAdd01` | `M_VFX_DisAdd_Flames01` | Flames |
| 118 | Mesh (`SlashClaw`) | `SlashDisAdd04` | `M_VFX_DisAdd_Slash04` | Slash |

**This port authors no look of its own** — the richest system in the batch needed the fewest new
assets, because every one of its six materials had already been recreated. §4's `SlashDisAdd04`
reconciliation is settled there.

`Get_BehaviorLookName(32)` stays `NAME_None`: every look rides a row renderer that binds it explicitly.

---

## 11. Tests

`Test_Particles_DebuffCastBehavior.cpp` + the `NumBehaviors` 30 → 33 ratchet in
`Test_Particles_RosterSanity.cpp`.

- **The curl force is asserted against a control that scores exactly zero.** Both sparkle clouds spawn
  at `Dir · R` and are driven along `Dir`, so their BALLISTIC path never leaves the ray through the
  origin — any angular deviation from the spawn direction is the curl and nothing else. Measured: mean
  **31.4°** (min 3.7°) on `Sparkles_Dark` and **21.3°** (min 8.7°) on `Sparkles_Bright`, against
  **0.0000°** with the term removed. The test requires every particle to deviate by more than 1° and
  the mean to exceed 5°.
- **The curl contributes exactly nothing at Age 0**, asserted as `Sparkles_Dark` sitting on the
  source's 200-unit shell to within 0.01 units — the stateless-path claim, made falsifiable.
- **Implosion versus explosion**: every dark sparkle closes on the origin (and its velocity points
  back at its own spawn point); every bright one opens away from it.
- **The two `Random Range` colours are keyed on the RECOVERED lerp parameter**, not on a colour
  channel — the lesson [NS_BuffLoop.md](NS_BuffLoop.md) §14.7 records, transferred to this colour
  mode. Live: **113** and **112** distinct half-degree buckets over 120 seeds; **dead control
  (the draw pinned at its midpoint): exactly 1**, so the assertion fails on the defect it names.
- **The rate shares** over 400 000 seeds, worst deviation **0.00064** against a 0.004 bar, plus the
  stronger claim that `BigArrow` and `Flames` take **zero** stream.
- **The spawn-phase split**, asserted both ways over 2000 seeds: inside the window a rate particle
  draws its rate layer, past it every one is hidden.
- **`Flames`**: on the 20-unit shell, **zero** position change over its whole life, a turn rate inside
  ±45 °/s, the distortion channel pinned at 10, and a flipbook frame inside the 2×2 sheet.
- **`Slash`**: no sprite size, mesh scale with Y pinned at 1 and X/Z inside 0.5–1.5, position at the
  cast point, a non-identity per-particle orientation, dissolve pinned at 0.1, and the offset channel
  sweeping forward by more than 0.8 over the life.
- **`BigArrow`** starts at +50, sinks, and draws a taller-than-wide quad.
- **The row's renderer set** is asserted to carry exactly one mesh (named `SlashClaw`) and exactly one
  2×2 sheet.
- Plus the standard per-layer anti-vacuity and death checks, the latter on BOTH spawn paths.

---

## 12. Verification — A/B protocol

`[HUMAN-VERIFY]` — **not yet run.** Open the **VfxExamples** gym, station pair **DEBUFF CAST**.
`NS_DebuffCast` is a `Loop Once` system, so the harness re-arms both sides on completion and the two
pedestals replay in sync from t = 0. Use `Ck_GymVfxExamples_RestartAll` to re-fire them together.

| # | Criterion | Look for |
|---|---|---|
| a | Overall read | a dark, oppressive implosion. Almost every colour here is near-black, so the effect reads by SILHOUETTE and by the flame shell's bloom, not by hue |
| b | What is NOT there | no warm glow and no rainbow ring — the source disables that whole stack. If you see them, four disabled emitters were recreated (§13.6) |
| c | The claws | seven dark comma-shaped blades at the cast point, each differently oriented and scaled, with a bright streak sweeping ALONG the blade from its tail to its tip |
| d | Claw streak direction | the streak must run down the arc, not across it. Across = the mesh's UV is transposed (§8) |
| e | Dark sparkles | near-black motes appearing on a wide (200-unit) shell and falling INWARD, wandering as they go |
| f | The wander | that wander is the curl field. Straight radial lines mean the force is inert (§9.3) |
| g | Bright sparkles | a spray outward from the centre, each particle a solid dark green or dark violet — one colour per particle, not a gradient |
| h | Flames | 200–300 unit puffs pinned to a tight 20-unit shell, turning slowly in place, cycling a four-frame flipbook, heavily distorted |
| i | The arrow | a single large chevron descending from 50 units above the cast point, fading in and out |
| j | Ring | a 220–250 unit near-black halo whose opacity peaks exactly halfway through its life |
| k | Density | ≈ 30 at t = 0 and ≈ 19 more streamed over the next 0.3 s |
| l | World space | move the pedestal mid-effect if you can (§13.7) |

---

## 13. Confirmed fidelity differences

1. **Curl-driven motion is advected along a path from the SPAWN point, not from the particle's actual
   position.** `CurlPath` re-integrates the whole path from spawn on every evaluation with a fixed
   16-step Euler loop, which is what keeps the position a function of (spawn, Age, Seed) alone and
   bit-identical between GPU and CPU. The source integrates the force onto a particle that has already
   travelled. The step count is a fidelity constant, not a tunable.
2. **Both curl constants are converted, not copied** — §9.3 states the derivation. The frequency
   conversion in particular is `[inferred]`: the module's own position scaling is not exported. This
   is the port's biggest open fidelity question and the first thing to judge at the inspection stage.
3. **The `Curl Noise Force`'s cone mask (45° / 45°) is not implemented.** The source masks the force
   into a cone; the port applies the field isotropically.
4. **`Velocity Falloff Distance 100` is treated as inert.** `Sparkles_Dark` spawns at radius 200,
   outside that distance, and the module's falloff curve is not recoverable from the corpus. The
   layer is driven at its stated −700 throughout, which is the reading §5 already describes.
5. **`Initial Mesh Orientation`'s `Random Range Vector (−1,−1,−1)..(1,1,1)` units are unrecoverable**,
   so each claw takes its own random facing — every reading of that range produces differently-facing
   claws, and this is the same call NS_Arrow_Cast §13 makes for its LightningStrip layer.
6. **The four DISABLED emitters are not recreated**, deliberately (§5, §9.4).
7. **World space.** All source emitters are `LocalSpace: false`; the template is local space. Same
   recorded deviation as NS_BasicAttack §13.2.
8. **Unplumbed family parameters**: `Core_Intensity` (1 on three of six looks), `Glow_Intensity` 2 on
   `Flames01` (reproduced, folded into Brightness), `Gradient_Invert`, and **`CamOffset 50`** on
   `Part03_Bright` — the geometric one, which is what stops `Sparkles_Bright` z-fighting the rest.
   `Flames01`'s `Color_Core (0.016, 0.014, 0.014)` is NOT carried by the shared look, and does not need
   to be: the same material sets `Color_CoreDifferent = 0`, which gates the core-colour branch off.
9. **Streamed particles drawn outside their window are allocated and hidden** — see
   [NS_HealCast.md](NS_HealCast.md) §13.2. At 65 /s this row allocates 130 per loop and renders ≈ 19.5
   of them from the stream.

---

## 14. Reusable lessons

1. **When a source force has no expressible units, derive the conversion from something the source
   DOES state.** The curl strength here is not a free constant: the layer's own `Scale Velocity`
   plateau bounds how much of a 2500-unit acceleration survives, and that bound plus the measured mean
   magnitude of our own noise field determines the equivalent path velocity exactly. A tuned number
   would have been indistinguishable at first glance and unauditable later.
2. **Build the control INTO the geometry when you can.** Testing "the curl is running" normally needs
   a second code path to compare against. Here the ballistic path is exactly radial by construction,
   so the angular deviation from the spawn direction is zero without the force and nonzero with it —
   a dead control that needs no dead code.
3. **§14.7's lesson generalizes past HSV.** Any per-particle colour draw can be keyed on the RECOVERED
   draw parameter instead of on a channel: invert the widest-spread channel of the declared range. It
   is brightness- and envelope-independent by construction, and pinning the draw collapses it to one
   bucket — which is what makes the assertion falsifiable.
4. **A measured mesh table can be exact.** The Crescent stores a coarse angular table because its
   measurement was noisy; this one stores all 49 source columns and reproduces the sheet to 5 × 10⁻⁵
   units. When the source's own topology is small and clean, transcribe it rather than fitting it.
5. **Check whether a "missing" material parameter is GATED before recording it as a difference.**
   `Flames01`'s dark `Color_Core` looks like an unported delta until you read `Color_CoreDifferent = 0`
   in the same instance, which switches the whole branch off. The shared look's white core is not a
   loss.
