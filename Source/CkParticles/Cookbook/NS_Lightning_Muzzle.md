# Recipe: NS_Lightning_Muzzle → CkParticles (IMPLEMENTATION-COMPLETE)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: IMPLEMENTATION-COMPLETE (2026-08-02, Phase 3 batch G). Behavior id 39. Not yet A/B'd.**

`Behavior_LightningMuzzle.ush` + `ExecuteStage_CPU` case 39, the
`PS_CkParticles_Template_LightningMuzzle` cadence row (2.0 s / 0.6 s / burst 24, plus a ribbon emitter
bursting the arc pair's 30 points), two new CkUsf looks (`LightningDisAdd01`, `FlatAdd02Ribbon`), two
new textures (`LightningBolt`, `LightningBand`), ZERO new meshes,
`Test_Particles_LightningMuzzleBehavior.cpp`, and a VfxExamples gym pair. Nothing has been rendered or
visually compared — §12 is open.

**Read §6.6 before trusting §5 on the arcs:** the speed clamp this sheet called "active" is disabled in
the source, which is what made the arcs closed-form-expressible after all.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Lightning_Muzzle` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior ID | **not allocated** — take the next free id at implementation time from `ck::particles::NumBehaviors` |
| CkUsf looks | none yet |

Corpus evidence (regenerate per [README.md](README.md); `Saved/` is machine-local):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Lightning_Muzzle.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part01_Bright,Part02,Part03_Bright,Part04,Lightning01,Lightning02,LightStrip,Flat02,Arrows}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_FlatAdd.json` (the SECOND family — see §4)
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_Ring04.json` (family reference for the diff)
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_{Spike01,Plane01}.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Part_03,Part_04,Lightning_01,Lightning_02,Lightning_03,LightStrip_01,Noise_02,WhitePixel}.json`

**The source Niagara asset was never opened in the Niagara editor.** Every fact below is `[corpus]`
unless tagged otherwise.

> ### TWO SYSTEMS SHARE THIS NAME — take the right one
> `[corpus]` The pack ships a second `NS_Lightning_Muzzle` at
> `Vefects/Anime_Stylized_VFX/VFX/Particles/`: 15 emitters and a **16-entry `userParameters` list**
> (`User.Glow Color 01..03`, `User.Flare Color 01..04`, `User.Lightning Arc Color 01/02`,
> `User.Lightning Beam Color 01`, `User.Lightning Spot Color 01`, `User.Spikes Color 01`,
> `User.Sparkles Color 01`, `User.Sparkles Stretched Color 01`, `User.Scale Overall`), rendering
> through `MI_VFX_*` instances.
>
> **Fastest one-line discriminator:** `userParameters` is **empty** on the system this recipe
> documents and **16 entries long** on the sibling. Second check: `M_VFX_DisAdd_*` materials (this
> one) vs `MI_VFX_*` (sibling).
>
> This recipe recreates the **`Anime_VFX/Shared/Skills`** one.

---

## 2. System anatomy `[corpus]`

**17 CPU emitters — 15 enabled, 2 disabled.** All 15 active emitters are `LocalSpace: **true**`
(LOCAL space, unlike the Cast/Pickup systems); the two disabled Arrow emitters are world-space.
`Determinism: false`, `Bounds: Dynamic` on all. `userParameters` is **empty**.

Renderers: **10 camera-facing sprites** (`Unaligned` / `FaceCamera` / `Sort: None`, one of them with
`SubUV: 2x2`), **1 velocity-aligned sprite**, **3 mesh renderers**, **2 ribbon renderers**, plus 2
velocity-aligned sprites on the disabled Arrow emitters. No light renderers, no events, no GPU sims.

**≈ 30 burst particles per cycle plus ≈ 24 ribbon rate-spawns.**

| # | Emitter | Enabled | Spawn | Lifetime (s) | Size / Mesh scale | Dyn 1 | Renderer | Material |
|---|---|---|---|---|---|---|---|---|
| 1 | Glow_01 | yes | burst 1 @0 | 0.4 | Uniform **550** | 1 | camera sprite | `Part01` |
| 2 | Glow_02 | yes | burst 1 @0 | 0.4 | Uniform **200** | **0** | camera sprite | `Part02` |
| 3 | Glow_03 | yes | burst 1 @0 | **0.2** | Uniform **250** | **2** | camera sprite | `Part01` |
| 4 | **Arrow** | **DISABLED** | burst 1 @0 | 1.5 | Non-Uniform (170, 170) | 0 | velocity-aligned | `Arrows` |
| 5 | **BigArrow** | **DISABLED** | burst 1 @0 | 1.5 | Non-Uniform (150, 240) | 0 | velocity-aligned | `Arrows` |
| 6 | Sparkles | yes | burst **4** @0 | rand — §5 | Random Uniform **10 … 20** | 1 | camera sprite | `Part01_Bright` |
| 7 | Flare_01 | yes | burst 1 @0 | 0.5 | Uniform **50** | 1 | camera sprite | `Part02` |
| 8 | Flare_02 | yes | burst 1 @0 | 0.5 | Uniform **50** | 1 | camera sprite | `Part02` |
| 9 | Sparkles_Stretched | yes | burst **3** @0, loop **0.4 s Once** | rand — §5 | Random Non-Uniform (25, 80) … (40, 90) | **0** | **velocity-aligned** | `Part04` |
| 10 | Lightning | yes | burst **3** @0, loop **0.5 s Once** | rand — §5 | Random Uniform **30 … 100** | curve | camera sprite, **SubUV 2×2** | `Lightning02` |
| 11 | Flare_03 | yes | burst **2** @0 | **0.2** | Uniform **250** | 1 | camera sprite | `Part03_Bright` |
| 12 | Flare_04 | yes | burst 1 @0 | **0.2** | Uniform **80** | **2** | camera sprite | `Part01_Bright` |
| 13 | Spikes | yes | burst **4** @0 | **0.15** | Mesh Scale rand (0.1, 0.1, 0.6) … (0.1, 0.1, 0.4) | 1 | **mesh**, `Facing: Velocity` | `Flat02` on `SM_VFX_Spike01` |
| 14 | LightningArc_01 | yes | burst 1 @0 **+ rate curve 80 → 0**, loop **0.3 s Once** | rand 0.2 … 0.3 | Ribbon Width rand **7 … 12** | — | **ribbon** | `Flat02` |
| 15 | LightningArc_02 | yes | burst **5** @0 **+ rate curve 80 → 0**, loop **0.3 s Once** | rand 0.2 … 0.3 | Ribbon Width rand **2 … 4** | — | **ribbon** | `Flat02` |
| 16 | LightningSpot | yes | burst 1 @0 | 0.3 | Mesh Scale **(1.2, 1.4, 5)** | **0** | **mesh**, `Facing: Default` | `LightStrip` on `SM_VFX_Plane01` |
| 17 | LightningBeam | yes | burst 1 @0 | 0.3 | Mesh Scale **(2.5, 2.5, 4)** | curve | **mesh**, `Facing: Default` | `Lightning01` on `SM_VFX_Plane01` |

Dynamic material parameters 2, 3 and 4 are **0 on every emitter that writes them**; only
`Write Parameter Index 0` is true anywhere. The two ribbon emitters have no Dynamic Material
Parameters module at all (their material, `Flat02`, declares no dynamic parameters — §4).

**Burst arithmetic** `[inferred, from the table]`: 1+1+1+4+1+1+3+3+2+1+4+1+5+1+1 = **30** burst
slots. Ribbon rate-spawns: two emitters × ∫(80 → 0 linear over 0.3 s) = 2 × 12 = **24**.

**How those 30 split across the two EMITTERS the recreation builds `[P3-G6]`:** six of them
(LightningArc_01's 1 and LightningArc_02's 5) are RIBBON particles and live on the row's second
emitter, so the main burst is **24** and the ribbon emitter carries **30** of its own — its six burst
points plus the twenty-four solved rate points. The itemization above is right; the row assignment is
the correction.

**Aim axis: +X** `[corpus, from the spawn shapes]`. Sparkles, Sparkles_Stretched, Spikes,
LightningArc_01 and LightningArc_02 all use `Sphere Location` with `Hemisphere X = true` and a
`Non Uniform Scale` that squashes Y/Z (`(1, 0.1, 0.1)` on Sparkles, `(1, 0.2, 0.2)` on the other
four), and all four velocity emitters add `Random Range Vector` velocity along **+X only**
(`(500,0,0)…(2000,0,0)`, `(1000,0,0)…(3000,0,0)`, `(100,0,0)…(500,0,0)`, `(700,0,0)…(1500,0,0)`,
`(2500,0,0)…(3000,0,0)`). This matches the CkParticles roster convention for MuzzleFlash/Tracer
(forward = **+X**) exactly — no re-aiming needed.

**Cadence — two life-cycle modes, four loop durations.** `[corpus]`

| Life Cycle Mode | Emitters | Loop Behavior | Loop Duration |
|---|---|---|---|
| **System** (own loop rows inert) | 13 of 17 | stored `Infinite` | stored 1.0 |
| **Self** (own loop rows apply) | **Sparkles_Stretched, Lightning, LightningArc_01, LightningArc_02** | **Once** | **0.4 / 0.5 / 0.3 / 0.3** |

The four `Self` emitters are genuine one-shots. See §6.5, gap 2.

**System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
Per [P0-D1] this rules the 13 `Life Cycle Mode = System` emitters; the four `Self` emitters keep
their own rows. *(Was `[unresolved]` with 1.0 s as the working figure; no corroborating late burst
was available, and the true value is 2×.)*

---

## 3. Mesh geometry `[corpus, derived from the .obj files]`

Both meshes are trivial — this is good news: they are cheap procedural regenerations, not
measurement problems like NS_BasicAttack's 805-vert crescent.

### `SM_VFX_Spike01` — 16 verts / **6 triangles**

A **square-base pyramid with a closed base**:

- Apex at `(0, 0, 200)`; base corners at `(±100, ±100, 0)` — base 200 × 200, height 200
- Bounds `(-100, -100, 0)` … `(100, 100, 200)`, size `(200, 200, 200)`
- 4 side triangles + 2 base triangles = 6; 16 verts because every corner is split per-face
- **UV0 (`numTexCoords: 2`, uv0 spans 0 … 1 fully):** apex = `(0.5, 0)`; base corners = `(0, 1)` or
  `(1, 1)`. So **u runs ACROSS the base edge and v runs ALONG the spike, v = 0 at the TIP,
  v = 1 at the BASE.** The base quad carries the same `(0/1, 1)` corners, i.e. it is UV-degenerate
  (all four corners at v = 1) — the base face samples a single texture row.
- Its stored material slot is `M_VFX_DisAdd_Slash01`, which the emitter **overrides** with
  `M_VFX_DisAdd_Flat02` (`overrideMaterials[0]`). Do not copy the stored slot.

### `SM_VFX_Plane01` — 8 verts / **4 triangles**

A **hand-doubled flat quad** (two coincident quads with opposite winding, offset 0.064 units in Y):

- Quad A at `Y ≈ 0`, quad B at `Y ≈ −0.0643`; both span `X ∈ [−100, 100]`, `Z ∈ [0, 200]`
- Bounds `(-100, -0.0642912, 0)` … `(100, 5.96e-06, 200)`, size `(200, 0.0643, 200)`
- **UV0: `u` maps X (`X = −100 → u = 0`, `X = +100 → u = 1`); `v` maps Z INVERTED
  (`Z = 200 → v = 0`, `Z = 0 → v = 1`).** So v = 0 is the TOP of the plane.
- The 0.064-unit Y offset and opposite winding make it manually two-sided. **A single flat quad plus
  a `_TwoSided` look renders identically at half the triangle count** — the same call
  NS_BasicAttack §13.5 made for the crescent's 0.1-unit thickness.
- Stored material slot is `WorldGridMaterial` / `M_VFX_DisAdd_Slash01`; both emitters override it.

Both mesh emitters use `Initial Mesh Orientation` with
`Rotation Coordinate Space = **Mesh**`, `Orientation Axis (1, 0, 0)`,
`Orientation Vector (0, 0, **−1**)`, `Rotation (0, 0, 0)` — i.e. mesh local +X is carried onto
world −Z. Spikes additionally uses `Facing: Velocity` on the renderer, which overrides that with the
per-particle velocity direction `[inferred: Niagara's mesh `Facing: Velocity` takes precedence over
the Initial Mesh Orientation quaternion; confirm against the renderer before relying on it]`.

---

## 4. Material families + delta table `[corpus]`

**This system uses TWO parent graphs, not one.**

### Family A — `M_VFX_DissolveAdd` (already implemented as `CkUsf_Look_DissolveAdd`)

Eight of the nine active materials. All: `MD_Surface`, `BLEND_Translucent`, `MSM_Unlit`,
`twoSided: false`, outputs `EmissiveColor` + `Opacity`, dynamic-parameter channels
**`dissolve`, `distortion`, `offset`, `core_color`**, expression histogram identical to the family
reference.

Deltas versus `M_VFX_DisAdd_Ring04` (reference: `Brightness 30`, `Color_CoreDifferent 1`,
`Core_Intensity 1`, `Core_Power 1`, `Glow_Intensity 1`, `Opacity_Boldness 1`,
`Opacty_DepthFade 10`, `Opacty_StepAdd 0.1`, `Gradient_Invert 0`, `Dissolve_Speed_X/Y 0.2`,
`Distortion_Scale_X/Y 0.1`, `Distortion_Intensity 0`, `Dissolve 0`, all `*_Speed`/`*_Offset` 0,
`Color_Core RGBA(1,1,1,0)`, `GradientMap_Tex T_VFX_WhitePixel`):

| Material | Main_Tex / Color_Tex | Dissolve_Tex | Brightness | Other deltas |
|---|---|---|---|---|
| `Part01` | `T_VFX_Part_01` | `T_VFX_Part_01` | **1** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Distortion_Tex T_VFX_Noise_02`; `Gradient_Invert 0.5`; `Opacity_Boldness 0.5`; `Opacty_DepthFade 20` |
| `Part01_Bright` | `T_VFX_Part_02` | `T_VFX_Part_02` | **10** | as above minus `Core_Intensity`/`Opacity_Boldness` |
| `Part02` | `T_VFX_Part_02` | `T_VFX_Part_02` | **1** | as `Part01_Bright` plus `Core_Intensity 0`, **`Glow_Intensity 0.3`**, `Opacity_Boldness 0.5` |
| `Part03_Bright` | `T_VFX_Part_03` | `T_VFX_Part_03` | **10** | `Color_CoreDifferent 0`; **`CamOffset 50`**; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Distortion_Tex T_VFX_Noise_02`; `Gradient_Invert 0.5`; `Opacty_DepthFade 20` |
| `Part04` | `T_VFX_Part_04` | `T_VFX_Part_04` | **6** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Distortion_Tex T_VFX_Noise_02`; **`Opacty_DepthFade 30`** |
| `Lightning01` | `T_VFX_Lightning_01` | **`T_VFX_Lightning_02`** | **10** | `Color_CoreDifferent 0`; **`Color_Speed_Y −0.5`**; **`MainTex_Speed_Y −0.5`**; **`Dissolve 0.4`** (static bias); `Dissolve_Speed_X/Y 0`; **`Distortion_Intensity 0.3`**; `Distortion_Scale_X/Y 1`; **`Distortion_Speed_X/Y 0.7`**; `Opacty_DepthFade 20` |
| `Lightning02` | `T_VFX_Lightning_03` | `T_VFX_Lightning_03` | **15** | `Color_CoreDifferent 0`; `Core_Intensity 0`; **`Core_Power 0`**; `Dissolve_Speed_X/Y 0`; **`Distortion_Intensity 0.5`**; `Distortion_Scale_X/Y 1`; **`Distortion_Speed_X/Y 0.7`**; `Opacty_DepthFade 20` |
| `LightStrip` | `T_VFX_LightStrip_01` | `T_VFX_LightStrip_01` | **7** | `Color_CoreDifferent 0`; `Core_Intensity 0`; **`Core_Power 0`**; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Distortion_Tex T_VFX_Noise_02`; `Gradient_Invert 0.5`; `Opacty_DepthFade 20` |
| `Arrows` *(disabled emitters only)* | `T_VFX_Arrow_01` | `T_VFX_Arrow_01` | **10** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Gradient_Invert 0.5`; `Opacty_DepthFade 20` |

**`Lightning01` is the most parameter-heavy instance in this batch**: it is the only one that pans
BOTH the main texture and the colour texture (`MainTex_Speed_Y` and `Color_Speed_Y`, both −0.5), the
only one with a non-zero static `Dissolve` bias (0.4), and one of three with live distortion.
`Lightning01` and `Lightning02` are the only two instances whose distortion branch is live.

### Family B — `M_VFX_FlatAdd` (**NEW — not implemented in CkUsf**)

`M_VFX_DisAdd_Flat02`, used by the two ribbon emitters and (via renderer override) by the Spikes
mesh. Despite the `DisAdd_` name prefix its parent is
`/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_FlatAdd` — **a completely different, much
smaller graph.**

| | |
|---|---|
| Domain / blend / shading | `MD_Surface` / `BLEND_Translucent` / `MSM_Unlit`, `twoSided: false` |
| Connected outputs | `EmissiveColor` + `Opacity` |
| Expression histogram | `Multiply ×2`, `ScalarParameter ×3`, `VectorParameter ×1`, `ParticleColor ×1`, `DepthFade ×1`, `WorldPosition ×1`, `MaterialFunctionCall ×1` — **that's the whole graph** |
| Texture parameters | **NONE** — `textureParams: []` |
| Dynamic parameters | **NONE** |
| Scalar params on this instance | `Brightness **10**`, `Opacty_DepthFade **0**`, `CamOffset 0` |
| Vector params | `Color_Core RGBA(1, 1, 1, 0)` |

In plain terms: **emissive = ParticleColor.rgb × Brightness, opacity = ParticleColor.a**, with a
DepthFade term that is disabled here (`Opacty_DepthFade 0`) and a camera-offset term at 0. No
texture, no dissolve, no UV animation. It is the simplest look in the whole cookbook and is a
one-evening CkUsf addition — see §6.3.

### Textures `[corpus]`

All 512×512, `sRGB: false`, `TC_Alpha`, `TEXTUREGROUP_World` unless noted.

| Texture | Format | Address | Role |
|---|---|---|---|
| `T_VFX_Part_01` | `TSF_G8` | `TA_Clamp`/`TA_Clamp` | Part01 |
| `T_VFX_Part_02` | `TSF_G8` | `TA_Clamp`/`TA_Clamp` | Part01_Bright, Part02 |
| `T_VFX_Part_03` | `TSF_G8` | `TA_Wrap`/`TA_Wrap` | Part03_Bright |
| `T_VFX_Part_04` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Part04 (velocity-aligned streak) |
| `T_VFX_Lightning_01` | `TSF_G8` | `TA_Wrap`/`TA_Wrap` | Lightning01 main |
| `T_VFX_Lightning_02` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Lightning01 dissolve |
| **`T_VFX_Lightning_03`** | **`TSF_BGRA8`** — colour, not a mask | `TA_Wrap`/`TA_Wrap` | Lightning02 main + dissolve; **the 2×2 sub-UV flipbook sheet** |
| `T_VFX_LightStrip_01` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | LightStrip |
| `T_VFX_Noise_02` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Distortion_Tex on six (**dead branch on all six** — those instances have `Distortion_Intensity 0`) + GradientShape_Tex |
| `T_VFX_WhitePixel` | `TSF_RGBA16`, 1×1, `sRGB: true`, `TC_Default` | `TA_Wrap` | no-op GradientMap on all Family-A instances |

**The gradient-map chain is a provable no-op in this system** (every Family-A instance keeps
`GradientMap_Tex = T_VFX_WhitePixel`), so the NS_Lightning_Range §13.3 / NS_BasicAttack §13.4
justification for dropping it holds here — unlike in NS_PickupCast and NS_Lightning_Cast.

---

## 5. Per-layer runtime curves `[corpus]`

`t` = NormalizedAge (0 → 1 over that emitter's own lifetime). `C` = constant key, `L` = linear key —
transcribed verbatim.

**Seven emitters share one Scale Color + one Scale Sprite Size shape** ("the shared fade"):

- Scale Color · `Scale RGBA = R (0,1)L (1,1)L | G (0,1)L (1,1)L | B (0,1)L (1,1)L | A (0,1)L (1,0)L`
  (RGB untouched; **alpha ramps linearly 1 → 0**), `Scale Alpha 1`, `Scale RGB (1,1,1)`
- Scale Sprite Size · Uniform Curve `(0, 0.5)C (0.1, 1)L (1, 1)L`

Those layers have **no `Color` module override** — they keep their Initialize Particle colour and fade.

### 1 · Glow_01 — burst 1 @0, life 0.4 s, size 550

Initialize color `RGBA(0.0648033, 0.0307135, 1, 1)` (deep blue) · shared fade · dyn `[1, 0, 0, 0]`

### 2 · Glow_02 — burst 1 @0, life 0.4 s, size 200

Initialize color `RGBA(0.863157, 0.0262412, 1, 1)` (magenta) · shared fade · dyn `[**0**, 0, 0, 0]`

### 3 · Glow_03 — burst 1 @0, life **0.2 s**, size 250

Initialize color `RGBA(1, 0.819765, 0.499, 1)` (warm white). A `Color` module is present with no
override (stored `Color.Color RGBA(1,1,1,1)`, `Scale Alpha 1`, `Scale Color (1,1,1)`) · shared fade ·
dyn `[**2**, 0, 0, 0]`

### 6 · Sparkles — burst **4** @0, size Random Uniform 10 … 20

- Lifetime `[corpus-v3]`: **`Lifetime Mode = Random` ⇒ `Lifetime Min 0.3 / Max 0.6` DRIVES**
  (`lifetimeResolved.source = minmax`); the `Random Range Float` override (0.2 … 0.4) sits on the
  unselected Direct-Set pin and is INERT. *Was read as 0.2 … 0.4 following the NS_BasicAttack §2
  precedent; corrected per [P0-D2]. `Sparkles` is now the longest ENABLED layer and it moves the
  cadence row (§6.1).*
- Spawn shape: `Sphere Location`, `Sphere Radius **10**`, `Non Uniform Scale **(1, 0.1, 0.1)**`,
  `Sphere Orientation Axis (1,0,0)`, **`Hemisphere X = true`**, `Surface Only false` — a thin
  half-cigar pointing +X
- `Add Velocity from Point`: strength `Random Range Float 001` with `Minimum **500**`,
  `Maximum **200**` — **min > max, an authored inversion; recorded verbatim**
  `[unresolved: how Niagara's Random Range Float resolves an inverted range — the result is either
  a constant or an empty range; do not silently normalize it]`
- `Add Velocity`: `Random Range Vector` `Minimum (500, 0, 0)` … `Maximum (2000, 0, 0)` — pure **+X**
- Sprite rotation: `Random`, 0 … 360
- Velocity Scale (all three axes): `(0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`
- Color from Curve:
  - R `(0.511923, 0.296138)C (0.876547, 0)C`
  - G `(0.511923, 0.571125)C (0.876547, 0.0896671)C`
  - B `(0.511923, 1)C (0.876547, 1)C`
  - A `(0.51313, 1)C (1, 0)L`
- Scale Sprite Size, Uniform Curve: `(0, 0)C (0.1, 1)C (1, 0)C`
- `Color.Scale Alpha 1`, dyn `[1, 0, 0, 0]`

### 7 · Flare_01 — burst 1 @0, life 0.5 s, size 50

Initialize color `RGBA(0.102242, 0.658375, 1, 0.2)` (cyan) · shared fade · dyn `[1, 0, 0, 0]`

### 8 · Flare_02 — burst 1 @0, life 0.5 s, size 50

Initialize color `RGBA(0.102242, 1, 0.838799, 0.258)` (mint) · shared fade · dyn `[1, 0, 0, 0]`

### 9 · Sparkles_Stretched — burst **3** @0, loop **0.4 s Once** (`Self`), velocity-aligned

- Lifetime `[corpus-v3]`: **`Lifetime Min 0.3 / Max 0.4` drives**; the 0.2 … 0.4 override is inert
  ([P0-D2], same resolution as above)
- Sprite Size Mode **Random Non-Uniform**: `Sprite Size Min **(25, 80)**`, `Max **(40, 90)**`
- Spawn shape: `Sphere Location`, radius **10**, `Non Uniform Scale (1, 0.2, 0.2)`,
  **`Hemisphere X = true`**
- `Add Velocity from Point`: strength `Random Range Float 001` **200 … 800**
- `Add Velocity`: `Random Range Vector` **(1000, 0, 0) … (3000, 0, 0)** — pure +X
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, 0.15)C (1, **−9.09372e-09**)C`
- Color from Curve:
  - R `(0, 1)C (0.079686, 1)L (0.290975, 0.646925)L (1, 0.223228)C`
  - G `(0, 0.913099)C (0.079686, 0.134)L (0.290975, 0.14)L (1, 0)C`
  - B `(0, 0.584079)C (0.079686, 0.731716)L (0.290975, 1)L (1, 0.116971)C`
  - A `(0.080893, 1)L (1, 0)C`
- **Three stacked size modules (they multiply):**
  1. Scale Sprite Size (Uniform Curve mode): `(0, 0)C (0.1, 1)C (1, 0)C`; its non-uniform curve
     `X (0,0)L (1,1)L | Y (0,0)L (1,1)L` is inert under Uniform mode
  2. Scale Sprite Size 001 (Non-Uniform Curve mode): `X (1, 1)L | Y (0, 1)C (0.3, 0.25)C (1, 0.2)C`
  3. **Scale Sprite Size by Speed**: `Scale Factor Curve (0, 0)L (1, 1)L`,
     `Velocity Threshold **1000**`, `Min Scale Factor (1, 1)`, `Max Scale Factor **(1, 2)**`
- `Color.Scale Alpha **0.8**`, dyn `[**0**, 0, 0, 0]`

### 10 · Lightning — burst **3** @0, loop **0.5 s Once** (`Self`), camera sprite, **SubUV 2×2**

- Lifetime `[corpus-v3]`: **`Lifetime Min 0.3 / Max 0.5` drives**; the 0.2 … 0.4 override is inert
  ([P0-D2], same resolution as above)
- Size: Random Uniform **30 … 100**
- Spawn shape: `Sphere Location`, `Sphere Radius **0**` — every particle at the origin
- `Add Velocity from Point`: strength `Random Range Float 001` **350 … 500**
  `[unresolved: the emitted direction from a radius-0 point source is not derivable from the corpus]`
- Sprite rotation: `Random`, 0 … 360; **Sprite Rotation Rate** = `Float from Curve 002`
  `(0, 1.68162e-07)C (0.1, **90**)C (0.9, 1.43051e-06)C` — 90 °/s between t = 0.1 and 0.9
- **Sub UV Animation**: mode `Linear`, `Start Frame 0`, `End Frame **4**`, `SubUV Loop Count 1`
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`
- Color from Curve — **five keys with a strobing alpha**:
  - R `(0, 1)L (0.0748566, 1)L (0.598853, 1)C (0.811349, 0.287441)L (1, 0.0512695)C`
  - G `(0, 0.745404)L (0.0748566, 1)L (0.598853, 0.147027)C (0.811349, 0.0409152)L (1, 0.0409152)C`
  - B `(0, 0.304987)L (0.0748566, 1)L (0.598853, 0.982251)C (0.811349, 1)L (1, 1)C`
  - A `(0.162994, 1)L (0.329611, **0**)L (0.504679, 1)C (0.746152, **0**)L (0.959855, 1)L`
    — **the alpha strobes on/off/on/off/on over one life.** Simplifying it to a fade destroys the
    lightning read. (Byte-identical to NS_Lightning_Cast's and NS_Lightning_Hit's Lightning layers.)
- Dyn param 1 (`dissolve`) — Float from Curve: `(0, 1)C (0.2, **−1**)C (0.3, **0.875**)C (1, −1)C`;
  params 2/3/4 = 0
- Scale Sprite Size, Uniform Curve: `(0, 0)C (0.1, 0.8)C (1, 1)C` · `Color.Scale Alpha 1`

### 11 · Flare_03 — burst **2** @0, life **0.2 s**, size 250

Initialize color `RGBA(1, 0.4563, 0.111, 0.737104)` (orange) · shared fade · dyn `[1, 0, 0, 0]`.
Both particles are identical (no randomness) and overlay exactly.

### 12 · Flare_04 — burst 1 @0, life **0.2 s**, size 80

Initialize color `RGBA(1, 0.384719, 0.0889999, 0.0371041)` · shared fade · dyn `[**2**, 0, 0, 0]`

### 13 · Spikes — burst **4** @0, life **0.15 s**, **mesh** `SM_VFX_Spike01` / `Flat02`, `Facing: Velocity`

- Initialize color `RGBA(0.871367, 0.06301, 1, 0.5)` — **no `Color` module at all**, so the colour is
  constant for the whole life; only the mesh scale animates
- `Mesh Scale Mode = Non-Uniform`, `Mesh Scale = Random Range Vector 001`
  `Minimum **(0.1, 0.1, 0.6)**` … `Maximum **(0.1, 0.1, 0.4)**` — X and Y are pinned at 0.1; Z is an
  inverted range (min > max), recorded verbatim `[unresolved — same inversion question as Sparkles]`
- Spawn shape: `Sphere Location`, radius **10**, `Non Uniform Scale (1, 0.2, 0.2)`,
  **`Hemisphere X = true`**
- `Add Velocity from Point`: strength `Random Range Float 001` `Minimum **200**` … `Maximum **50**`
  (inverted again)
- `Add Velocity`: `Random Range Vector` **(100, 0, 0) … (500, 0, 0)** — +X
- `Initial Mesh Orientation`: `Rotation Coordinate Space = Mesh`, `Orientation Axis (1, 0, 0)`,
  `Orientation Vector (0, 0, −1)`, `Rotation (0, 0, 0)`
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`
- **Scale Mesh Size** (`Vector from Curve 001`):
  `X (0, 0)C (0.2, 1)C (1, 0)C | Y (0, −2.4747e-08)C (0.2, 1)C (1, 0)C | Z (0, −5.68323e-08)C (0.2, 1)C`
  — note Z has **no third key**, so it holds 1 from t = 0.2 to death while X and Y collapse: the
  spikes flatten to a line rather than shrinking away
- dyn `[1, 0, 0, 0]` — but `Flat02` declares **no dynamic parameters**, so this write is inert

### 14 · LightningArc_01 — **ribbon**, loop **0.3 s Once** (`Self`)

- Spawn: burst 1 @ t=0 **plus** `Spawn Rate` override `Float from Curve 002` `(0, **80**)C (1, 0)C`
- `Initialize Ribbon`: `Lifetime Mode = Random`, `Lifetime Min **0.2** / Max **0.3**`
  (the Direct-Set pin stores 1, inert), `Color RGBA(1,1,1,1)`, `Position Offset (0,0,0)`;
  **`Ribbon Width` = `Random Range Float` 7 … 12**
- Spawn shape: `Sphere Location`, radius **10**, `Non Uniform Scale (1, 0.2, 0.2)`,
  **`Hemisphere X = true`**
- `Add Velocity from Point`: strength `Random Range Float 001` **10 … 50**
- `Add Velocity`: `Random Range Vector` **(700, 0, 0) … (1500, 0, 0)** — +X
- **Curl Noise Force**: `Noise Strength = Float from Curve 001` `(0, **10000**)C (1, 0)C`,
  `Noise Frequency **500**`, `Random Seed **11**`, `Randomization Vector (0.65, 0.125, 0.37)`,
  `Pan Noise Field (0,0,0)`, `Noise Quality / Cost = Baked (Medium)`. Its `Curl Noise Cone Mask Angle 45`
  / `Cone Mask Falloff Angle 45` are **INERT** — the module's `Mask Curl Noise = false` **[P3-G5]**.
- Velocity Scale: `X/Y/Z (0, 1)C (1, 0)C`
- Color from Curve:
  - R `(0.511923, 1)C (0.813764, 0.584079)C`
  - G `(0.511923, 1)C (0.813764, 0.06301)C`
  - B `(0.511923, 1)C (0.813764, 1)C`
  - A `(0, 0)L (0.220948, 1)L (0.51313, 1)C (1, 0)L`
- **Scale Ribbon Width**: `Float from Curve` `(0, 4.18339e-08)C (0.2, 1)C (1, 0.4)C`
- `Color.Scale Alpha 1`, material `Flat02`
- `Solve Forces and Velocity`: `Acceleration Limit 9999`, `Speed Limit 1000` — **both INERT**
  `[corpus]`: the module's own `Clamp Velocity = false` and `Limit Acceleration = false`, exactly as on
  every other emitter in this batch. *(Was read as "this one binds"; corrected as **[P3-G4]** — it was
  the sheet's headline reason for calling the arcs non-closed-form.)*

### 15 · LightningArc_02 — **ribbon**, loop **0.3 s Once** (`Self`)

Identical structure to Arc_01 with these differences:

| | Arc_01 | Arc_02 |
|---|---|---|
| Burst count @ t=0 | 1 | **5** |
| Ribbon Width random | 7 … 12 | **2 … 4** |
| `Add Velocity` range | (700,0,0) … (1500,0,0) | **(2500,0,0) … (3000,0,0)** |
| Curl `Noise Frequency` | 500 | **1000** |
| Velocity Scale | `(0, 1)C (1, 0)C` | `(0, 1)C (1, **0.2**)C` |
| `Color.Scale Alpha` | 1 | **0.5** |
| Colour R | `(0.511923, 1)C (0.813764, 0.584079)C` | `(0.511923, **0.135751**)C (0.813764, **0.570314**)C` |
| Colour G | `(0.511923, 1)C (0.813764, 0.06301)C` | `(0.511923, **0.036**)C (0.813764, **0.0319999**)C` |
| Colour B | `(0.511923, 1)C (0.813764, 1)C` | `(0.511923, 1)C (0.813764, 1)C` |
| Colour A | `(0, 0)L (0.220948, 1)L (0.51313, 1)C (1, 0)L` | identical |

Everything else — lifetime 0.2 … 0.3, spawn sphere, curl strength curve `(0, 10000)C (1, 0)C`,
seed 11, randomization vector, `Scale Ribbon Width (0, 4.18339e-08)C (0.2, 1)C (1, 0.4)C`, the
`Spawn Rate` curve `(0, 80)C (1, 0)C`, material `Flat02`, and the two inert limiters — matches Arc_01
exactly. Arc_01 is the wide bright core; Arc_02 is the thinner, faster, dimmer blue-violet one.
**Neither emitter carries a Ribbon ID module**, so each is ONE ribbon strand linking its own particles
in spawn order — Arc_02 is not five filaments **[P3-G7]**.

### 16 · LightningSpot — burst 1 @0, life 0.3 s, **mesh** `SM_VFX_Plane01` / `LightStrip`, `Facing: Default`

- Initialize color `RGBA(0.341915, 0.184475, 1, 0.3)`, `Color.Scale Alpha **0.1**`
- `Mesh Scale Mode = Non-Uniform`, `Mesh Scale **(1.2, 1.4, 5)**`, `Mesh Uniform Scale 1`
- `Initial Mesh Orientation`: coordinate space `Mesh`, `Orientation Axis (1,0,0)`,
  `Orientation Vector (0, 0, −1)`, `Rotation (0,0,0)`
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C` (inert — no velocity is added)
- Color from Curve: R `(0.315122, 0.354443)L` · G `(0.315122, 0.2)L` · B `(0.315122, 1)L`
  (single key each — constant RGB) · A `(0, 0)C (0.317537, 1)L (1, 0)C`
- **Scale Mesh Size** (`Vector from Curve 001`):
  `X (0, 0.5)C (0.2, 1)C (1, −1.44926e-08)C | Y (0, −2.4747e-08)C (1, 1)C | Z (0, −5.68323e-08)C (0.2, 0.75)C (1, 1)C`
- dyn `[**0**, 0, 0, 0]`

### 17 · LightningBeam — burst 1 @0, life 0.3 s, **mesh** `SM_VFX_Plane01` / `Lightning01`, `Facing: Default`

- Initialize color `RGBA(0.341915, 0.184475, 1, 1)`, `Color.Scale Alpha 1`
- **`Position Offset (−30, 0, 0)`** — the only non-zero position offset in this system
- `Mesh Scale Mode = Non-Uniform`, `Mesh Scale **(2.5, 2.5, 4)**`, `Mesh Uniform Scale 1`
- `Initial Mesh Orientation`: same as LightningSpot
- Dyn param 1 (`dissolve`) — Float from Curve: `(0, 1)C (1, −1)C`; params 2/3/4 = 0
- Color from Curve — the same five-key lightning ramp as emitter 10, with a **one-key alpha**:
  - R `(0, 1)L (0.074857, 1)L (0.598853, 1)C (0.811349, 0.287441)L (1, 0.051269)C`
  - G `(0, 0.745404)L (0.074857, 1)L (0.598853, 0.147027)C (0.811349, 0.040915)L (1, 0.040915)C`
  - B `(0, 0.304987)L (0.074857, 1)L (0.598853, 0.982251)C (0.811349, 1)L (1, 1)C`
  - A `(0.162994, 1)L` — **single key: alpha is 1 for the whole life.** The beam disappears purely
    by the `dissolve` ramp 1 → −1, not by fading.
- **Scale Mesh Size** (`Vector from Curve 001`):
  `X (0, 0.5)C (0.2, 1)C | Y (0, −2.4747e-08)C (1, 1)C | Z (0, −5.68323e-08)C (0.2, 0.75)C (1, 1)C`
  — X has no third key, so it holds 1 after t = 0.2

---

## 6. Translation plan (CkParticles / CkUsf) — AS IMPLEMENTED

*This section is REWRITTEN in place. Its original text predates Phase 1's and Phase 3's capability work
and listed nine gaps, five of them blockers that no longer exist. Each is answered below; the batch-D
precedent (supersede a stale plan rather than annotate it) applies.*

### 6.1 Cadence row

**A new row, `PS_CkParticles_Template_LightningMuzzle`:**

| Field | Value | Why |
|---|---|---|
| Loop duration | **2.0 s** `[corpus-v3]` | the system's `Once` loop duration ([P0-D1]/[P0-D3]) |
| Particle lifetime | **0.6 s** | max resolved lifetime among the ENABLED emitters — `Sparkles`' 0.6 s Max; every burst is at t = 0 so no delay is added |
| Burst count | **24** | the thirteen enabled sprite/mesh emitters' own counts — **not 30**; see §6.6 |
| Spawn rate | **0** | the only source rate belongs to the two arcs, and it is inverted into their burst (§9.3) |
| Ribbon emitter | **burst 30**, one `Ribbon` renderer | 1 + 12 for Arc_01, 5 + 12 for Arc_02 |

Layer partition is `Seed % 24`, in table order: 0 `Glow_01`, 1 `Glow_02`, 2 `Glow_03`, 3–6 `Sparkles`,
7 `Flare_01`, 8 `Flare_02`, 9–11 `Sparkles_Stretched`, 12–14 `Lightning`, 15–16 `Flare_03`,
17 `Flare_04`, 18–21 `Spikes`, 22 `LightningSpot`, 23 `LightningBeam`.

The four `Self / Once` emitters need no spawn window: `Sparkles_Stretched`, `Lightning` and both arcs
carry a BURST, and a burst-only one-shot fires exactly once per activation, which is what a template
burst is ([P2-E7]). The arcs' additional Spawn Rate is the one thing that is not a burst in the source,
and §9.3 turns it into one.

### 6.2 VisTag / renderer needs — RESOLVED

Nine row renderers for the thirteen enabled sprite/mesh emitters, plus the ribbon:

| VisTag | Kind | Look | Source emitters |
|---|---|---|---|
| 175 | CameraFacingSprite | `PartDisAdd01` | Glow_01, Glow_03 |
| 176 | CameraFacingSprite | `PartDisAdd02` | Glow_02, Flare_01, Flare_02 |
| 177 | CameraFacingSprite | `PartDisAdd01Bright` | Sparkles, Flare_04 |
| 178 | VelocityAlignedSprite | `PartDisAdd04` | Sparkles_Stretched |
| 179 | CameraFacingSprite, **SubImageSize 2×2** | `LightningDisAdd02` | Lightning |
| 180 | CameraFacingSprite | `PartDisAdd03Bright` | Flare_03 |
| 181 | Mesh `Spike` | `FlatAdd02` | Spikes |
| 182 | Mesh `Card` | `LightStripDisAdd` | LightningSpot |
| 183 | Mesh `Card` | `LightningDisAdd01` | LightningBeam |
| 184 | **Ribbon** (ribbon emitter) | `FlatAdd02Ribbon` | LightningArc_01, LightningArc_02 |

Three meshes on one row is the heaviest mesh load in the cookbook.

### 6.3 Mesh / texture / look needs

**Meshes: ZERO new.** The two the sheet planned already exist and are the same geometry:

- `SM_VFX_Spike01` (square-base pyramid, base 200 × 200 at Z = 0, apex at (0, 0, 200), closed base) is
  `SM_CkParticles_Spike`, generated for NS_FireBall_Hit;
- `SM_VFX_Plane01` (quad, X ∈ [−100, 100], Z ∈ [0, 200], `v` inverted so v = 0 is the TOP) is
  `SM_CkParticles_Card`, whose surface function is `Lerp(-100, 100, s)` on X, `Lerp(200, 0, t)` on Z with
  `uv = (s, t)` — the source's UV convention exactly, including the inversion. The hand-doubled 0.064-unit
  second quad the source carries is replaced by a `_TwoSided` look, the same call NS_BasicAttack §13.5 made.

**Looks: EIGHT reused, TWO new** (§10). **Textures: TWO new bakes** (§7); everything else this system
needs, including `T_VFX_Lightning_03` → `LightningSheet`, an earlier batch already measured.

### 6.4 Behavior id

**39.** Allocated at implementation time from `ck::particles::NumBehaviors`, which moved 38 → 40 in one
bump with NS_BuffCast (38).

### 6.5 The original capability gaps, answered

1. **Ribbon renderers** — landed as C6a/[P3-D1]. Both arcs are recreated, as TWO ribbon ids on ONE
   renderer (§9.3).
2. **Row-level camera-facing sprite** — landed in Phase 1 (C1).
3. **One template, three cadences** — dissolved: the four `Self / Once` emitters all burst, and a
   burst-only one-shot IS a template burst ([P2-E7]). No deviation is needed.
4. **Sub-UV** — landed in Phase 1 (C4). The bolt layer declares a 2×2 grid and steps it in Linear mode.
5. **Curl noise** — landed in Phase 2 (C10) as `CkParticles_CurlPath`. The sheet's fear that the source's
   integration carried an ACTIVE speed clamp is WRONG and is corrected in §6.6: both limiters are
   disabled, so the advection is unclamped and closed-form-expressible. §13 records what remains.
6. **Three inverted `Random Range` ranges** — transcribed as authored (`lerp(Min, Max, r)` with Min > Max
   simply descends), never silently normalized. Sparkles' 500…200, Spikes' 200…50 and Spikes' Mesh Scale
   Z 0.6…0.4 all read as authored.
7. **`Facing: Velocity` on the Spikes mesh** — resolved by the ratified precedent: a row-declared mesh
   renderer draws with Facing Default, and a source emitter whose renderer faces VELOCITY is reproduced
   by writing `CkParticles_QuatFromZTo(velocity)` (Common.ush's own contract, established by
   NS_FireBall_Hit). The `Initial Mesh Orientation` the emitter also carries is the thing Facing Velocity
   overrides, so it is not applied — §13.
8. **`MainTex_Speed_Y` / `Color_Speed_Y`** — still not plumbed; the beam's texture does not scroll along
   its length. §13.
9. **The two disabled Arrow emitters** — deliberately not recreated, and the burst count excludes them.

### 6.6 Corrections applied to this sheet at implementation

- **[P3-G4]** §5.14/§5.15 said of the arcs' `Solve Forces and Velocity`: "**this one binds** — the
  1000 u/s speed limit is an active clamp". The corpus reads `Clamp Velocity = false` and
  `Limit Acceleration = false` on BOTH arcs, so the Speed Limit 1000 and Acceleration Limit 9999 are
  INERT, exactly as they are on every other emitter in the batch. This was the sheet's headline reason
  for calling the arcs "not closed-form" (§6.5 gap 5); with the clamp gone the advection is a plain
  `CkParticles_CurlPath`.
- **[P3-G5]** the same sections list a `Curl Noise Cone Mask Angle 45` / `Falloff 45` pair as if live.
  The module's own `Mask Curl Noise = false`, so the cone mask is inert and is not implemented — and
  unlike NS_DebuffCast §13.3, that is now a proven inertness rather than an omission.
- **[P3-G6]** §6.1's burst count of **30** included the arcs' six burst particles. Those are RIBBON
  particles and live on the row's second emitter, so the main burst is **24** and the ribbon emitter
  carries 30 of its own (6 burst + 24 solved). The itemization was right; the row assignment was not.
- **[P3-G7]** §5.15 describes Arc_02 as "five thin filaments". Neither arc emitter carries a Ribbon ID
  module, so Niagara's default groups each emitter's particles into ONE ribbon: Arc_02 is a single
  strand linking its five burst points and its twelve solved ones, not five strands. The phrase was a
  visual gloss, not a corpus reading.

---

## 7. Textures — TWO new bakes

Reused, each measured off this very paint by an earlier batch: `T_VFX_Part_01` → `SoftParticle`,
`T_VFX_Part_02` → `SoftParticleBright`, `T_VFX_Part_03` → `SoftParticleFine`, `T_VFX_Part_04` →
`SparkStreak`, `T_VFX_LightStrip_01` → `LightStrip`, `T_VFX_Lightning_03` → `LightningSheet`,
`T_VFX_Noise_02` → `TileNoise`, `T_VFX_Noise_04` → `TileNoiseCoarse`. `T_VFX_Arrow_01` is not needed —
both emitters using it are disabled.

The two `Lightning01` paints are new, and both were measured against every other corpus paint first.
`T_VFX_Lightning_01`'s best correlation to anything already baked is **0.43** (`T_VFX_Part_03`, and only
after a 104-row roll); `T_VFX_Lightning_02`'s is **0.72** (`T_VFX_Wind_02`, i.e. `WindBandMid`). The
0.72 is the closest near-miss the cookbook has rejected so far — for scale, the one reuse this campaign
has ACCEPTED on a correlation (`T_VFX_Wind_02` ≡ `T_VFX_Wind_03` rolled) measured **1.00000**, and
`LensSheet` was rejected at 0.56. A band-shaped paint correlating 0.72 with another band-shaped paint is
family resemblance, not identity.

### 7.1 `LightningBolt` — the library's first MEASURED-PROFILE paint

`T_VFX_Lightning_01` is a single vertical filament: 87.9 % of it is exactly black, and every lit pixel
sits in a 66-pixel corridor. Measured:

- the centre line **wanders** between u 0.513 and 0.568 (mean 0.5403, std 0.0150), and its spectrum along
  v is dominated by the first two harmonics — a slow snake, not jitter;
- the peak brightness varies 0.067 … 0.712 along v with two bright stretches;
- the width above 5 % varies 6.7 … 37.5 px, correlated with the peak;
- the cross-section is a sharp core with a long tail that no single closed form fits (a Gaussian is too
  narrow at 16 px, an exponential too wide).

None of that is fittable, so the bake carries the measurement itself: **16 centre anchors, 16 peak
anchors, 16 width anchors and 11 cross-section anchors**, through `Sample_Profile` — the idiom
`Px_LightStrip` established and `Surface_SlashClaw`'s 49-column rim table scaled up. One fitted constant,
the 0.70 on the width (the measured widths are thresholded on rows whose peak varies, so they overstate
the profile's own scale).

Bake vs source: mean **0.00694 / 0.00704**, coverage above 0.05 **0.0354 / 0.0357**, black fraction
**0.891 / 0.879**, p99 0.194 / 0.153, maximum 0.699 / 0.777 — and a **PIXELWISE correlation of 0.930**,
the highest any bake in this library has reached.

### 7.2 `LightningBand` — the dissolve behind the bolt

`T_VFX_Lightning_02` is a horizontal band, symmetric about v = 0.5 (top-vs-flipped-bottom correlation
**0.958**), half-maximum between v 0.318 and 0.680, with a long soft shoulder outside it. Inside the band
the texture is a SMOOTH low-frequency modulation, not grain: neighbouring columns correlate at 0.99 and
columns 64 px apart still at 0.77.

The band is the measured 32-anchor v-profile verbatim (normalized against its measured in-band plateau of
0.5424); the modulation is the library's own Fbm at 6 × 4 tiles, 2 octaves, depth 0.7 — the coarsest
tiling that reproduces the measured spread. Bake vs source: in-band mean **0.584 / 0.540**, in-band std
**0.140 / 0.134**, maximum **0.814 / 0.831**, global mean **0.226 / 0.223**. The residual is correlation
LENGTH: the bake's columns stay correlated further than the source's (0.85 vs 0.77 at 64 px), so the bake
is slightly smoother than the paint. Recorded in §13.

---

## 8. Meshes — reused, not rebuilt

Zero new meshes. `SM_CkParticles_Spike` and `SM_CkParticles_Card` are the geometry §3 measured, already
generated for earlier ports; §6.3 records the match, including the Card's inverted `v`.

---

## 9. The behavior — `Behavior_LightningMuzzle.ush` + `ExecuteStage_CPU` case 39

### 9.1 The burst layers

Thirteen enabled emitters over `Seed % 24`, every burst at t = 0, each layer hiding itself past its own
lifetime. Direct transcriptions of §5; what is worth naming:

- **Seven layers share one fade and one grow curve** — the same pair NS_Lightning_Cast carries, and the
  three Glow colours, both Flare colours and the whole Lightning ramp are BYTE-IDENTICAL between the two
  systems. The recreation states them again rather than sharing a helper across behaviors: a behavior is
  a transcription of its own source, and the two sources are free to diverge.
- **`Spikes` carry no Color module at all**, so the violet is constant for the whole 0.15 s; only the mesh
  scale animates, and its Z curve has no third key, so the pyramids FLATTEN to a line rather than
  shrinking away.
- **`LightningBeam`'s alpha is a single key**, so it never fades — it leaves entirely through its dissolve
  ramp 1 → −1.
- **`LightningSpot`'s `Color.Scale Alpha` is 0.1**, so its plane is a tenth as bright as its authored
  colour suggests.
- **The two plane meshes' orientation** is the source's `Initial Mesh Orientation` (`Orientation Axis
  (1,0,0)` onto `Orientation Vector (0,0,-1)`), i.e. mesh-local +X carried onto world −Z, which on a
  carrier whose width is X and length is Z is a quarter turn about +Y and nothing else.

### 9.2 Aim

The source fires along **+X** — five emitters spawn through an X-hemisphere sphere squashed on Y and Z,
and every velocity-adding module adds along +X only. That is already the roster's MuzzleFlash/Tracer
convention, so nothing is re-aimed. This is the one port in the batch where the existing convention lines
up for free.

### 9.3 The arcs — a falling rate inverted into a burst

Each arc emitter runs a burst AND a Spawn Rate overridden with a curve falling **80 → 0** across its own
0.3 s window. A row rate is a constant, and the [P2-E8] peak-and-thin idiom that NS_Lightning_Cast used
needs the emitter clock — which a ribbon point cannot afford, because its POSITION depends on when it
spawned. So the rate is inverted instead:

```
N(s) = 80s - (400/3)s^2        (the source's own cumulative count; N(0.3) = 12)
tau_i = 0.3 * (1 - sqrt(1 - (i + 0.5)/12))
```

Twelve points per arc at those times, plus the burst points at zero: 1 + 12 for Arc_01, 5 + 12 for
Arc_02, thirty in all. The inversion is exact, not an approximation — and it is checkable: the source's
integral says **nine of the twelve** land in the first half of the window, which is what the test asserts
(a uniform spread or an averaged rate lands six).

Spawn order is link order is ribbon order, because the point index ascends with its solved time within
each arc. The two arcs are separated by `RibbonIdBinding` ← `Particles.MeshIndex` — one renderer, two
ribbons, the [P3-D1] option-(c) shape the fireball's mirrored pair established.

Each point's own life is a per-particle 0.2 … 0.3 s ([P0-D6]'s hand-applied `Initialize Ribbon` reading,
confirmed against the corpus: `Lifetime Mode = Random`, the Direct-Set 1 inert), and it is visible only
inside `[tau, tau + life]`. The row's 0.6 s lifetime is exactly `0.3 + 0.3`, so the last point of the
longest life dies at the row's boundary.

### 9.4 The curl conversion

Following the NS_FireBall_Projectile precedent, stated rather than tuned:

- **Frequency**: the source's `Noise Frequency` is authored in metres, so 500 becomes 0.5 per unit and
  1000 becomes 1.0 `[inferred — the position scaling inside the module graph is not in the corpus]`.
- **Strength**: the source figure is an ACCELERATION where `CurlPath` advects with a VELOCITY. The
  strength curve falls 10000 → 0 over the life, so its time average is 5000, and the constant-velocity
  path covering the same ground over a life L is `0.5 * 5000 * L`. That is divided by the MEASURED mean
  magnitude of this plugin's own curl field over the region the arcs visit: **0.7400** at frequency 0.5
  and **0.7426** at 1.0, each over 4000 samples within ±400 units at the source's own seed 11. (The two
  agree because the field's magnitude is a derivative in lattice space and does not depend on the
  sampling rate; what changes with frequency is which part of the field a path visits.)
- The 45° cone mask is inert ([P3-G5]) and the 1000 u/s speed limit is inert ([P3-G4]), so nothing
  clamps the result.

§13 records what this does NOT reproduce.

---

## 10. Looks and renderers

**Eight looks reused, two new.** Every reuse was checked value-by-value against §4's delta table:

| Source material | Look | Check |
|---|---|---|
| `M_VFX_DisAdd_Part01` | `PartDisAdd01` | Brightness 1, Boldness 0.5, `SoftParticle` — matches |
| `M_VFX_DisAdd_Part01_Bright` | `PartDisAdd01Bright` | Brightness 10, Boldness 1, `SoftParticleBright` — matches |
| `M_VFX_DisAdd_Part02` | `PartDisAdd02` | Brightness 1 × Glow_Intensity 0.3, Boldness 0.5 — matches |
| `M_VFX_DisAdd_Part03_Bright` | `PartDisAdd03Bright` | Brightness 10, `SoftParticleFine` — matches |
| `M_VFX_DisAdd_Part04` | `PartDisAdd04` | Brightness 6, `SparkStreak` — matches |
| `M_VFX_DisAdd_Lightning02` | `LightningDisAdd02` | Brightness 15, Distortion 0.5 at speed 0.7, `LightningSheet` — matches |
| `M_VFX_DisAdd_LightStrip` | `LightStripDisAdd` | Brightness 7, `LightStrip`, MESH usage — matches, and the mesh variant is the one this port needs |
| `M_VFX_DisAdd_Flat02` (mesh) | `FlatAdd02` | Brightness 10, sprite + mesh usage — matches |
| `M_VFX_DisAdd_Lightning01` | **`LightningDisAdd01`** (new) | Brightness 10, Dissolve bias 0.4, Distortion 0.3 at speed 0.7, `LightningBolt` / `LightningBand`, MESH usage |
| `M_VFX_DisAdd_Flat02` (ribbon) | **`FlatAdd02Ribbon`** (new) | the SAME instance, a third master — see below |

**`FlatAdd02Ribbon` is a real finding, not a duplicate.** `_UsedWithNiagaraSprites`,
`_UsedWithNiagaraMeshParticles` and `_UsedWithNiagaraRibbons` are three independent usage flags, and a
master that declares only the first two renders as the engine default under a ribbon renderer. This
source draws ONE material instance on a mesh renderer (the spikes) and on two ribbon renderers, so it
needs two masters — the same rule that produced `LightStripDisAddSprite` in batch E, now in its third
form. The builder refuses to emit a ribbon renderer over a master without the flag, so the two are
checked against each other rather than trusted.

---

## 11. Tests

`Test_Particles_LightningMuzzleBehavior.cpp`
(`CkTests.UnitTests.CkParticles.LightningMuzzleBehavior`), on the CPU mirror. What it pins:

- the cadence row and its ribbon emitter, by value;
- the burst partition per VisTag (2/3/5/3/3/2/4/1/1) and its stability across 500 moduli;
- **the arc spawn inversion**: the solved release times are strictly increasing, NINE of the twelve land
  in the first half of the window (a uniform or averaged rate lands six), and both ends match the
  inversion (0.00632 and 0.23876);
- the arc pair as TWO ribbon ids split 13 / 17, a seed bank disjoint in BOTH directions, and the curl
  force actually BENDING the arcs off the barrel axis;
- **the bolt strobe**: at least two peaks and two troughs in the alpha over one life, plus the colour at
  both ramp ends and all four sub-UV frames starting at 0 (Linear, not Random);
- **the spikes FLATTEN**: X and Y reach zero at death while Z holds, and the colour never moves because
  the emitter carries no Color module;
- the beam's alpha at BOTH ends (1 and still 1), its dissolve 1 → −1, its −30 offset and its held X scale;
- `LightningSpot`'s alpha peaking at a TENTH, bounded two-sided;
- `Glow_01` at both ends of its fade;
- emitter-clock independence over 400 seeds × 3 clocks.

---

## 12. Verification — A/B protocol `[HUMAN-VERIFY]`

Gym: **VfxExamples**, pair **"LIGHTNING MUZZLE"** (behavior 39 against
`/Game/Vefects/Anime_VFX/Shared/Skills/NS_Lightning_Muzzle`). Judge in this order:

a. **The direction.** Everything that moves goes down +X. If anything sprays backwards or sideways, the
   hemisphere shape or the added velocity is wrong.
b. **The arcs.** Two jagged streamers down the barrel — one wide and bright, one thin, faster and
   blue-violet. They should WANDER, not fly straight. A straight arc means the curl is not being applied;
   a hairball means its strength is.
c. **The bolt strobe.** The 2×2 sheet bolts blink on-off-on-off-on across their life. A smooth fade is
   the single most destructive simplification available here.
d. **The spikes.** Four violet pyramids pointing along their own flight, flattening to slivers rather
   than shrinking.
e. **The beam plane.** A bright bolt plane down the barrel that dissolves away rather than fading, and a
   dim wide ground glow under it.
f. **The palette.** Deep blue, magenta and warm white shells opening the flash — the same three colours
   NS_Lightning_Cast opens with.

---

## 13. Confirmed fidelity differences or intentional deviations

1. **The curl field is not the source's field, and 16 steps under-resolve it.** The source integrates a
   BAKED vector field (`Noise Quality / Cost = Baked (Medium)`) frame by frame; the recreation advects
   through this plugin's own Fbm curl in 16 fixed steps (the C10 fidelity constant). At the converted
   frequencies (0.5 and 1.0 per unit) the lattice cell is 2 and 1 units respectively, while a step covers
   tens of units — so the sampled field decorrelates between steps and the path reads as a coherent
   RANDOM WALK of about the right magnitude rather than as the source's exact swirl. The arc's overall
   reach and bend are converted quantities; its precise shape is not. This is the port's open fidelity
   question, the NS_DebuffCast §13.2 class.
2. **`Facing: Velocity` vs `Initial Mesh Orientation` on the Spikes.** The source sets both; the
   recreation applies the velocity facing (the ratified reading, and what the renderer does) and drops
   the module's quaternion.
3. **`MainTex_Speed_Y` and `Color_Speed_Y` (both −0.5) are not plumbed**, so the beam's texture does not
   scroll along its length. Also not plumbed, as elsewhere: `Core_Intensity` (1 on Lightning01),
   `Core_Power` (0 on Lightning02 and LightStrip), `Glow_Intensity` (folded into Brightness),
   `Gradient_Invert`, `CamOffset` (50 on Part03_Bright) and `Opacty_DepthFade`.
4. **`LightningBand` is smoother than its source.** Its columns stay correlated at 0.85 where the paint's
   are at 0.77 (§7.2). On a dissolve mask this reads as slightly larger erosion patches.
5. **The two disabled Arrow emitters are not recreated** — the only world-space emitters in the system,
   and the burst count excludes them. Recorded so the omission is a decision.
6. **The plane carrier is single-sided geometry plus a `_TwoSided` look**, not the source's hand-doubled
   quad. Half the triangles, same render.
7. **Local space matches for once.** All fifteen active source emitters are `LocalSpace: true`, which is
   what the CkParticles template already is — this is the only port in the batch without the C12
   world-space difference.

---

## 14. Reusable lessons

1. **Read the limiter's toggle before believing the limit.** The sheet called the arcs' 1000 u/s speed
   limit "an active clamp" and built its hardest capability gap on it. `Clamp Velocity = false` two lines
   away made the whole problem disappear. Same class as [P3-G1] on the event probabilities, in the same
   batch — a `Use…`/`Clamp…`/`Limit…` boolean is the first thing to read on any module whose stored value
   would change the plan.
2. **A non-constant spawn rate is a CDF you can invert.** [P2-E8] thinned a uniform stream to match a
   falling rate, which needs the emitter clock. Inverting the cumulative count instead gives the exact
   spawn TIMES, needs no clock, and turns the population into a burst — strictly better wherever the rate
   curve is integrable. The falling linear ramp here inverts to one square root.
3. **A ribbon renderer is a THIRD usage flag.** `FlatAdd02Ribbon` is the third master of one source
   instance in this cookbook. When a source draws one `M_VFX_*` on renderer classes that differ, count
   the classes, not the materials.
4. **Check the mesh library before planning a generator.** Both meshes this port "needed" already existed
   from earlier batches, including the Card's inverted `v` convention. The sheet planned two new
   generations; the answer was two names.
5. **Byte-identical curves across sibling systems are worth naming but not sharing.** Three Glow colours,
   two Flare colours and the entire Lightning ramp are identical to NS_Lightning_Cast's. Stating them
   again in this behavior keeps each transcription answerable to its own source; a shared helper would
   couple two ports that the pack is free to diverge.
