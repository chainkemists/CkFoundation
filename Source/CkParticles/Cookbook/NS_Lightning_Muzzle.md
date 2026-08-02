# Recipe: NS_Lightning_Muzzle → CkParticles (PLANNED)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior `.ush`, no CPU mirror, no CkUsf look, no cadence row, no mesh, no texture bake, no test,
no gym station exists for this effect. Every number below is archaeology read out of the extracted
corpus; nothing here has been compiled, generated, rendered, or looked at. Sections 7+ are reserved
for the implementation session.

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
slots. Ribbon rate-spawns: two emitters × ∫(80 → 0 linear over 0.3 s) = 2 × 12 = **≈ 24**.

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
  `Noise Frequency **500**`, `Random Seed **11**`,
  `Randomization Vector (0.65, 0.125, 0.37)`, `Curl Noise Cone Mask Angle 45`,
  `Cone Mask Falloff Angle 45`, `Pan Noise Field (0,0,0)`
- Velocity Scale: `X/Y/Z (0, 1)C (1, 0)C`
- Color from Curve:
  - R `(0.511923, 1)C (0.813764, 0.584079)C`
  - G `(0.511923, 1)C (0.813764, 0.06301)C`
  - B `(0.511923, 1)C (0.813764, 1)C`
  - A `(0, 0)L (0.220948, 1)L (0.51313, 1)C (1, 0)L`
- **Scale Ribbon Width**: `Float from Curve` `(0, 4.18339e-08)C (0.2, 1)C (1, 0.4)C`
- `Color.Scale Alpha 1`, material `Flat02`
- `Solve Forces and Velocity`: `Acceleration Limit 9999`, `Speed Limit 1000` — **this one binds**:
  the arcs are launched at 700–1500 u/s along X and the curl force adds up to 10000, so the 1000 u/s
  speed limit is an active clamp, unlike everywhere else in this batch

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
`Spawn Rate` curve `(0, 80)C (1, 0)C`, material `Flat02` — matches Arc_01 exactly. Arc_01 is the
wide bright core; Arc_02 is five thin, faster, dimmer blue-violet filaments.

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

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new burst row is required `[corpus-v3]`, per [P0-D3]: loop 2.0 s, particle lifetime 0.6 s,
burst 30.** Loop = the system's `Once` loop duration (*was 1.0 s, from the inert emitter rows*);
lifetime = max resolved lifetime among the **enabled** emitters — `Sparkles`' resolved 0.6 s Max
(*was 0.5 s, Flare_01/02, under the override-wins assumption that capped Sparkles at 0.4 s; the
disabled `Arrow`/`BigArrow` pair at 1.5 s is excluded because it is not recreated*); burst = the §2
count. Every shorter layer zeroes colour, size and scale past its own
lifetime (the NS_BasicAttack §8 mechanism). **Every burst in this system is at t = 0**, so no spawn
delay machinery is needed for the burst layers.

What the row cannot carry:

- The **two ribbon emitters' rate curves** (80 → 0 /s over 0.3 s each, ≈ 12 particles apiece). Moot
  until ribbons exist at all (gap 1).
- The **four `Self` one-shot emitters** (Sparkles_Stretched 0.4 s, Lightning 0.5 s,
  LightningArc_01/02 0.3 s) run ONCE and never repeat while the other 11 cycle. See gap 3.

Loop duration resolved `[corpus-v3]` — see §2.

### 6.2 VisTag / renderer needs

| Source renderer | Count | Distinct looks | CkParticles today |
|---|---|---|---|
| Camera-facing sprite | 9 | 5 (`Part01`, `Part02`, `Part01_Bright`, `Part03_Bright`) | **row-level kind does not exist** (gap 2) |
| Camera-facing sprite **+ SubUV 2×2** | 1 | 1 (`Lightning02`) | **no sub-UV support at all** (gap 4) |
| Velocity-aligned sprite | 1 | 1 (`Part04`) | ✔ existing `VelocityAlignedSprite` row kind |
| **Ribbon** | **2** | 1 (`Flat02`) | **NOT SUPPORTED — no ribbon renderer, no CkUsf ribbon usage flag** (gap 1) |
| Mesh, `Facing: Velocity` | 1 | 1 (`Flat02` on `SM_VFX_Spike01`) | ✔ existing `Mesh` row kind; velocity facing is expressible by writing an orientation quat from velocity |
| Mesh, `Facing: Default` | 2 | 2 (`LightStrip`, `Lightning01`, both on `SM_VFX_Plane01`) | ✔ existing `Mesh` row kind |

VisTag ids: allocate at implementation time above `Get_RosterVisTag_Max()`; never restate a literal.

### 6.3 Mesh / texture / look needs

**Meshes: 2 new procedural generations, both trivial** (§3):

- `SM_CkParticles_Spike` — square-base pyramid, base 200 × 200 at Z = 0, apex at (0, 0, 200), closed
  base; UV `u` across the base edge, `v = 0` at the tip, `v = 1` at the base. 6 triangles.
- `SM_CkParticles_Plane` — a single flat quad spanning `X ∈ [−100, 100]`, `Z ∈ [0, 200]` at `Y = 0`;
  UV `u` maps X, `v` maps Z **inverted** (`v = 0` at `Z = 200`). Build it as ONE quad + a
  `_TwoSided` look rather than reproducing the source's 0.064-unit hand-doubling (2 triangles
  instead of 4) — the same call NS_BasicAttack §13.5 made.

**CkUsf looks: 9 new**, of which **8 are Family-A parameterizations** of the existing
`CkUsf_Look_DissolveAdd` per §4, and **1 is a NEW family**:

- **`FlatAdd`** — `M_VFX_FlatAdd` is a second parent graph with **no textures and no dynamic
  parameters**: emissive = `ParticleColor.rgb × Brightness`, opacity = `ParticleColor.a`, plus an
  (here-disabled) DepthFade and CamOffset. It is the simplest look in the cookbook — a small new
  `.ush` with `Brightness`, `CoreColor`, `DepthFade` and `CamOffset` parameters, or a three-parameter
  degenerate case. `Flat02` (Brightness 10) is the only instance in this batch.
- `Lightning01` needs two family parameters that are **not currently plumbed**: `MainTex_Speed_Y` and
  `Color_Speed_Y` (both −0.5) — a vertical UV pan on the main and colour samples. `DissolveBias` (0.4)
  and `DistortIntensity`/`DistortSpeed` (0.3 / 0.7) ARE plumbed.

**Textures: 8 procedural stand-ins**, following the NS_BasicAttack §7 method (measure the corpus
PNGs, bake from the numbers, never copy pixels):

- `T_VFX_Part_01` → **`T_CkParticles_SoftParticle` already exists**, measured off this exact asset. Reuse.
- `T_VFX_Part_04` → **`T_CkParticles_SparkStreak` already exists**, measured off this exact asset. Reuse.
- `T_VFX_Noise_02` → existing `T_CkParticles_TileNoise` (and its branch is dead on every instance here).
- `T_VFX_Part_02`, `T_VFX_Part_03`, `T_VFX_Lightning_01`, `T_VFX_Lightning_02`,
  `T_VFX_LightStrip_01` — **new bakes**; measure each.
- `T_VFX_Lightning_03` — a **2×2 flipbook sheet in `TSF_BGRA8` colour**, four bolt frames. A new
  class of bake. See gap 4.
- `T_VFX_Arrow_01` is **NOT needed** — both emitters using it are disabled.

### 6.4 Behavior id

**Do NOT allocate an id in this document.** Take the next free id from `ck::particles::NumBehaviors`
at implementation time and bump it. This batch contains five planned effects; whoever implements
second must re-read the roster, not this sheet.

Layer partition: `Seed % 30` over the 30 deterministic burst slots. Assignment (in table order):
0 = Glow_01, 1 = Glow_02, 2 = Glow_03, 3–6 = Sparkles, 7 = Flare_01, 8 = Flare_02,
9–11 = Sparkles_Stretched, 12–14 = Lightning, 15–16 = Flare_03, 17 = Flare_04, 18–21 = Spikes,
22 = LightningArc_01 burst, 23–27 = LightningArc_02 burst, 28 = LightningSpot, 29 = LightningBeam.
A modulo over a burst of exactly N gives the source's partition by construction; never
`Rand(Seed) < k` probability bands (NS_BasicAttack lesson 2).

**The aim axis is already correct**: the source fires along **+X**, matching the roster's
MuzzleFlash/Tracer convention. No re-aiming, and this is the one place in this batch where the
existing convention lines up for free.

### 6.5 CAPABILITY GAPS — what the pipeline cannot express today

Conservative list. Each is a real blocker or a real approximation.

1. **RIBBON RENDERERS ARE NOT SUPPORTED — this is the largest gap in this effect.**
   Two emitters (LightningArc_01/02, ≈ 26 particles) render through
   `NiagaraRibbonRendererProperties`. Nothing in CkParticles can express this:
   `FCk_ParticlesRendererSpec` has no ribbon kind, the template builder has never emitted a ribbon
   renderer, the DI writes no `RibbonWidth`/`RibbonID`/`RibbonLinkOrder`, and **CkUsf deliberately
   omits the Niagara ribbon usage flag** (NS_Lightning_Range §9: "Ribbon and mesh-particle usages
   were deliberately **not** added" — the mesh one has since been added, ribbon has not). Building
   this means: a ribbon usage flag on `LookDefinition`, a ribbon renderer kind on the row spec, and
   at minimum a per-particle ribbon width output on the DI. **Scope this before committing to a
   faithful port.** Without it, the two arc layers are simply absent — and they are the visual core
   of a lightning muzzle flash.
2. **No row-level camera-facing-sprite renderer.** Nine camera-sprite layers over four distinct
   looks; VisTag 0 binds ONE material via `User.SpriteMaterial`, and the row spec offers only `Mesh`
   and `VelocityAlignedSprite`. **Blocking.** Additive fix, mirrors `VelocityAlignedSprite`. Same gap
   in all five effects in this batch.
3. **One template = one cadence; this source has three.** Eleven System-mode emitters cycle on the
   system clock; Sparkles_Stretched (0.4 s), Lightning (0.5 s) and both LightningArcs (0.3 s) are
   `Life Cycle Mode = Self` + `Loop Behavior = Once` — they play **once and never repeat**. A
   CkParticles template replays everything every loop. Either accept the replay as a recorded
   deviation or split into two behaviors. `CkParticles/CLAUDE.md` forbids faking cadence with
   `frac(Age/Cycle)` inside the behavior, so it must be an honest deviation.
4. **No sub-UV / flipbook support anywhere in the pipeline.** The Lightning layer is a `SubUV: 2x2`
   sheet driven by `Sub UV Animation` (Linear, frames 0 → 4, loop count 1). No `SubImageIndex` stage
   output, no `SubImageSize` on the renderer spec, no atlas bake in the texture generator. Without
   it the bolts are one static frame.
5. **Curl Noise Force has no CkParticles equivalent, and it is not closed-form.** Both ribbon
   emitters drive a 3D curl-noise field (`Noise Frequency` 500 / 1000, strength 10000 → 0 over life,
   `Random Seed 11`, `Randomization Vector (0.65, 0.125, 0.37)`, 45° cone mask) integrated stepwise
   by `Solve Forces and Velocity` **under an active 1000 u/s speed limit**. CkParticles behaviors
   write closed-form positions precisely to keep GPU and CPU in lockstep (NS_BasicAttack lesson 7);
   a stepwise force integration with a clamp is not closed-form and would drift between the two
   implementations. A faithful arc needs either a deterministic closed-form noise displacement
   (e.g. summed sine octaves evaluated at `Age`, not integrated) or an accepted approximation.
   **Name this in the plan; do not discover it while writing HLSL.**
6. **Three inverted `Random Range` ranges** (`min > max`): Sparkles' velocity strength (500 … 200),
   Spikes' velocity strength (200 … 50), Spikes' Mesh Scale Z (0.6 … 0.4).
   `[unresolved: how Niagara resolves an inverted range.]` Recorded verbatim rather than normalized —
   silently swapping them changes the effect and hides an authoring quirk that may be deliberate.
7. **`Facing: Velocity` on a mesh renderer vs `Initial Mesh Orientation`.** The Spikes emitter sets
   both. `[inferred: the renderer's velocity facing wins over the module's quaternion.]` The
   CkParticles mesh path drives orientation purely from the behavior's `O.Orientation` quat, which
   can encode either — but pick the right one, and confirm which the source actually uses before
   claiming fidelity.
8. **Two family-A parameters not plumbed through `CkUsf_Look_DissolveAdd`**: `MainTex_Speed_Y` and
   `Color_Speed_Y` (both **−0.5** on `Lightning01`, the beam mesh). Without them the beam's texture
   does not scroll along its length — a visible loss on a lightning beam specifically. Also not
   plumbed, as elsewhere: `Core_Intensity`, `Core_Power` (**0** on Lightning02 and LightStrip),
   `Glow_Intensity` (0.3 on Part02), `Gradient_Invert`, `CamOffset` (50 on Part03_Bright), and
   `Opacty_DepthFade` (a pre-existing documented CkUsf gap).
9. **Two disabled emitters are deliberately not recreated** (Arrow, BigArrow — the only world-space
   emitters in the system). Recorded so the omission is a decision.

**Not gaps — confirmed absent from this source:** no light renderers, no GPU sims, no collision, no
event handlers, no user parameters, no material-binding indirection, and — unlike NS_PickupCast and
NS_Lightning_Cast — **no live gradient-map/LUT chain** (every Family-A instance keeps the white-pixel
`GradientMap_Tex`, so the existing "it's a no-op" justification holds here).

**Local space matches for once.** All 15 active emitters are `LocalSpace: true`, which is what the
CkParticles template already is — this is the only effect in the batch without the world-space
deviation NS_BasicAttack §13.2 records.

---

## 7+. Reserved for implementation.
