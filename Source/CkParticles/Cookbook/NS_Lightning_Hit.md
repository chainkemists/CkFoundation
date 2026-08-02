# Recipe: NS_Lightning_Hit → CkParticles (PLANNED)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior `.ush`, no CPU mirror, no CkUsf look, no cadence row, no mesh, no texture bake, no test,
no gym station exists for this effect. Every number below is archaeology read out of the extracted
corpus; nothing here has been compiled, generated, rendered, or looked at. Sections 7+ are reserved
for the implementation session.

**This is the largest and least portable effect in the batch** — 22 emitters, 87 burst particles,
ribbons, sub-UV flipbooks, three mesh renderers, and mixed local/world space in one system. Read
§6.5 before estimating it.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Lightning_Hit` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior ID | **not allocated** — take the next free id at implementation time from `ck::particles::NumBehaviors` |
| CkUsf looks | none yet |

Corpus evidence (regenerate per [README.md](README.md); `Saved/` is machine-local):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Lightning_Hit.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part01_Bright,Part04,Flare01,Flames01,Impact01,Lightning02,LightStrip,Flat02,Rainbow,Ring01,Star01,Star04}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_FlatAdd.json` (the SECOND family — see §4)
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_Ring04.json` (family reference for the diff)
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_{Spike01,Plane01,Sphere01}.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Part_04,Ring_01,Ring_02,Star_01,Star_04,Lightning_03,LightStrip_01,Impact_01,Wind_01,Noise_02,LUT_Rainbow_01,WhitePixel}.json`

**The source Niagara asset was never opened in the Niagara editor.** Every fact below is `[corpus]`
unless tagged otherwise.

> ### TWO SYSTEMS SHARE THIS NAME — take the right one
> `[corpus]` The pack ships a second `NS_Lightning_Hit` at
> `Vefects/Anime_Stylized_VFX/VFX/Particles/`. It also has **22 emitters**, so the emitter count is
> NOT a discriminator here. It carries a **17-entry `userParameters` list**
> (`User.Explo Color 01`, `User.Flames Color 01`, `User.Flare Color 01/02`, `User.Glow Color 01/02`,
> `User.Ground Crack Color 01`, `User.Ground Glow Color 01/02`, `User.Lightning Arc Color 01/02`,
> `User.Lightning Color 01`, `User.Ligthning Strip Color 01` *(sic)*, `User.Rainbow Color 01`,
> `User.Ring Color 01`, `User.Spikes Color 01`, `User.Scale Overall`) and renders through `MI_VFX_*`
> instances (`MI_VFX_Flames_01`, `MI_VFX_Flat_01`, `MI_VFX_Impact_01`, …).
>
> **Fastest one-line discriminator:** `userParameters` is **empty** on the system this recipe
> documents and **17 entries long** on the sibling. Second check: `M_VFX_DisAdd_*` materials (this
> one) vs `MI_VFX_*` (sibling).
>
> This recipe recreates the **`Anime_VFX/Shared/Skills`** one.

---

## 2. System anatomy `[corpus]`

**22 CPU emitters, all enabled. `Determinism: false`, `Bounds: Dynamic` on all.**
`userParameters` is **empty**.

**Coordinate space is MIXED: 15 emitters `LocalSpace: true`, 7 `LocalSpace: false`** — see §6.5,
gap 2. This is the only system in the batch that mixes.

Renderers: **13 camera-facing sprites** (of which 2 carry `SubUV: 2x2` and 2 use
`CustomAlignment` + `CustomFacingVector`), **1 velocity-aligned sprite**, **1 more sprite with
`SubUV: 2x2` ×2 for the Flames pair**, **4 mesh renderers**, **2 ribbon renderers**. No light
renderers, no events, no GPU sims.

**87 burst particles per cycle plus ≈ 27 rate-spawned.**

| # | Emitter | Space | Spawn | t (s) | Lifetime (s) | Size / Mesh scale | Dyn 1 | Renderer | Material |
|---|---|---|---|---|---|---|---|---|---|
| 1 | GroundGlow_01 | local | burst 1 | 0 | 0.2 | Uniform **2600** | 1 | sprite, **Custom align/facing** | `Part01` |
| 2 | GroundGlow_02 | local | burst 1 | 0 | 0.2 | Uniform **4000** | **0** | sprite, **Custom align/facing** | `Part01` |
| 3 | Glow_01 | local | burst 1 | 0 | 0.25 | Uniform **160** | **2** | camera sprite | `Part01` |
| 4 | Sparkles | local | burst **20** | 0 | rand — §5 | Random Uniform **5 … 20** | 1 | camera sprite | `Part01_Bright` |
| 5 | Flare_01 | local | burst 1 | **0.1** | **0.1** | Uniform **320** | 1 | camera sprite | `Flare01` |
| 6 | Sparkles_Stretched | local | burst **10**, loop **0.4 s Once** | 0 | rand — §5 | Random Non-Uniform (30, 120) … (50, 130) | **0** | **velocity-aligned** | `Part04` |
| 7 | Lightning_01 | local | burst **3**, loop **0.5 s Once** | 0 | rand — §5 | Random Uniform **130 … 100** *(inverted)* | curve | camera sprite, **SubUV 2×2** | `Lightning02` |
| 8 | Spikes | local | burst **5** | 0 | **0.15** | Mesh Scale rand (0.1, 0.1, 0.6) … (0.1, 0.1, 0.4) | 1 | **mesh**, `Facing: Velocity` | `Flat02` on `SM_VFX_Spike01` |
| 9 | LightningArc_01 | local | burst 1 **+ rate 80 → 0**, loop **0.3 s Once** | 0 | rand 0.2 … 0.3 | Ribbon Width rand **7 … 12** | — | **ribbon** | `Flat02` |
| 10 | LightningArc_02 | local | burst **5** **+ rate 80 → 0**, loop **0.3 s Once** | 0 | rand 0.2 … 0.3 | Ribbon Width rand **2 … 4** | — | **ribbon** | `Flat02` |
| 11 | LightningStrip | local | burst **3** | 0 | 0.3 | Mesh Scale rand **(2, 1, 2) … (3, 1, 4)** | **0** | **mesh**, `Facing: Velocity` | `LightStrip` on `SM_VFX_Plane01` |
| 12 | Lightning_02 | local | **rate curve 10 → 0 only** (no burst), loop **0.5 s Once** | — | rand — §5 | Random Uniform **130 … 100** *(inverted)* | curve | camera sprite, **SubUV 2×2** | `Lightning02` |
| 13 | FlareImpact | local | burst 1 | **0.1** | **0.05** | Uniform **130** | 1 | camera sprite | `Impact01` |
| 14 | Raimbow *(sic)* | **WORLD** | burst 1 | **0.05** | **0.1** | Uniform **500** | **0.5** | camera sprite | `Rainbow` |
| 15 | Ring | **WORLD** | burst 1 | **0.05** | 0.3 | Uniform **230** | curve | camera sprite | `Ring01` |
| 16 | GroundCrack_01 | local | burst 1 | **0.1** | **1** | Uniform **600** | 1 | sprite, **Custom align/facing** | `Star04` |
| 17 | Star02 | **WORLD** | burst **5**, loop **0.3 s Once** | 0 | rand 0.9 … 1.3 | Random Uniform **30 … 50** | 1 | camera sprite | `Star01` |
| 18 | Glow_02 | local | burst 1 | 0 | 0.25 | Uniform **80** | **2** | camera sprite | `Part01` |
| 19 | Bubble_First_Explo | **WORLD** | burst 1 | 0 | **0.15** | Mesh Uniform Scale **0.8** | — | **mesh**, `Facing: Default` | `Flat02` on `SM_VFX_Sphere01` |
| 20 | Spike01 | **WORLD** | burst **5** | 0 | **0.15** | Mesh Scale rand (0.2, 0.2, 0.5) … (0.4, 0.4, 1.5) | — | **mesh**, `Facing: Default` | `Flat02` on `SM_VFX_Spike01` |
| 21 | Flames_01 | **WORLD** | burst **10**, loop **0.3 s Once** | **0.05** | rand — §5 | Random Uniform **200 … 300** | curve | camera sprite, **SubUV 2×2** | `Flames01` |
| 22 | Flames_02 | **WORLD** | burst **10**, loop **0.3 s Once** | **0.05** | rand — §5 | Random Uniform **200 … 300** | curve | camera sprite, **SubUV 2×2** | `Flames01` |

**Burst arithmetic** `[inferred, from the table]`:
1+1+1+20+1+10+3+5+1+5+3+0+1+1+1+1+5+1+1+5+10+10 = **87** burst slots.
Rate-spawned: LightningArc_01 ∫(80 → 0 over 0.3 s) ≈ **12**, LightningArc_02 ≈ **12**,
Lightning_02 ∫(10 → 0 over 0.5 s) ≈ **2.5**. Total ≈ **114** particles per cycle.

Dynamic material parameters 3 and 4 are 0 everywhere. **Parameter 2 (`distortion`) is 10 on
Flames_01 and Flames_02** — the only non-zero `distortion` write in this batch. The ribbon and the
`Flat02`/`Sphere01` mesh emitters write no dynamic parameters at all (their material declares none).

**Aim axis: +Z** `[corpus, from the spawn shapes]`. Sparkles, Sparkles_Stretched, Lightning_01,
Lightning_02, Spikes, Star02, Flames_01 and Flames_02 all set `Hemisphere Z = true`
(and `Hemisphere X = false` where both are written), Spikes adds velocity along **(0,0,100) …
(0,0,500)** and Flames_02 along **(0,0,800) … (0,0,1200)**. This is a ground impact spraying
upward — matching the roster's ImpactBurst convention (surface normal = **+Z**).
The two ribbon emitters are the exception: they keep `Hemisphere X = true` and fire along +X.

**Cadence — two life-cycle modes, three loop durations.** `[corpus]`

| Life Cycle Mode | Emitters | Loop Behavior | Loop Duration |
|---|---|---|---|
| **System** (own loop rows inert) | 14 of 22 | stored `Infinite` | stored 1.0 |
| **Self** (own loop rows apply) | **Sparkles_Stretched, Lightning_01, Lightning_02, LightningArc_01, LightningArc_02, Star02, Flames_01, Flames_02** | **Once** | **0.4 / 0.5 / 0.5 / 0.3 / 0.3 / 0.3 / 0.3 / 0.3** |

Eight `Self` one-shots — more than a third of the system. See §6.5, gap 3.

**System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
Per [P0-D1] this rules the 14 `Life Cycle Mode = System` emitters; the eight `Self` emitters keep
their own rows. *(Was `[unresolved]` with 1.0 s as the working figure — the sheet's suspicion that
1.0 s was too short is confirmed: GroundCrack_01's 0.1 + 1.0 s fits comfortably in 2.0 s and nothing
overlaps.)*

---

## 3. Mesh geometry `[corpus, derived from the .obj / .json]`

Three meshes, two of them trivial.

### `SM_VFX_Spike01` — 16 verts / **6 triangles**

Square-base pyramid with a closed base. Apex `(0, 0, 200)`; base corners `(±100, ±100, 0)`.
Bounds `(-100, -100, 0)` … `(100, 100, 200)`. UV0: apex `(0.5, 0)`, base corners `(0, 1)` / `(1, 1)`
— **u across the base edge, v = 0 at the TIP, v = 1 at the BASE**; the base quad is UV-degenerate
(all four corners at v = 1). Stored material slot `M_VFX_DisAdd_Slash01` is **overridden** by both
emitters that use it (`Flat02`). Identical to the mesh NS_Lightning_Muzzle §3 documents.

### `SM_VFX_Plane01` — 8 verts / **4 triangles**

A hand-doubled flat quad: quad A at `Y ≈ 0`, quad B at `Y ≈ −0.0643`, opposite winding, both
spanning `X ∈ [−100, 100]`, `Z ∈ [0, 200]`. UV0: `u` maps X (`−100 → 0`, `+100 → 1`); `v` maps Z
**inverted** (`Z = 200 → v = 0`). A single quad plus a `_TwoSided` look renders identically at half
the triangles. Identical to the mesh NS_Lightning_Muzzle §3 documents.

### `SM_VFX_Sphere01` — 559 verts / **960 triangles**

A UV sphere of radius **100**: bounds `(-99.9999, -99.9999, -100)` … `(99.99997, 99.99994, 100)`,
`boundsSize (200, 200, 200)`, `numTexCoords: 2`, uv0 spans 0 … 1 fully. Standard latitude/longitude
sphere — a procedural regeneration needs the segment counts, which the JSON does not give directly;
960 triangles is consistent with 32 × 16 quads (`32 × 16 × 2 = 1024`) or 30 × 16 (`960`)
`[inferred: 30 longitudinal × 16 latitudinal segments produces exactly 960 triangles; not confirmed
against the .obj topology]`. Used by ONE emitter (Bubble_First_Explo).

---

## 4. Material families + delta table `[corpus]`

**Two parent graphs**, as in NS_Lightning_Muzzle.

### Family A — `M_VFX_DissolveAdd` (already implemented as `CkUsf_Look_DissolveAdd`)

Twelve of the thirteen materials. All: `MD_Surface`, `BLEND_Translucent`, `MSM_Unlit`,
`twoSided: false`, outputs `EmissiveColor` + `Opacity`, dynamic-parameter channels
**`dissolve`, `distortion`, `offset`, `core_color`**, expression histogram identical to the
family reference.

Deltas versus `M_VFX_DisAdd_Ring04` (reference: `Brightness 30`, `Color_CoreDifferent 1`,
`Core_Intensity 1`, `Core_Power 1`, `Glow_Intensity 1`, `Opacity_Boldness 1`,
`Opacty_DepthFade 10`, `Opacty_StepAdd 0.1`, `Gradient_Invert 0`, `GradientMap_Displacement 0.1`,
`Dissolve_Speed_X/Y 0.2`, `Dissolve_Scale_X/Y 1`, `Distortion_Scale_X/Y 0.1`,
`Distortion_Intensity 0`, `Dissolve 0`, all `*_Speed`/`*_Offset` 0, `Color_Core RGBA(1,1,1,0)`,
`Dissolve_Tex`/`Distortion_Tex` `T_VFX_Noise_04`, `GradientMap_Tex T_VFX_WhitePixel`,
`GradientShape_Tex T_VFX_Noise_02`):

| Material | Main_Tex / Color_Tex | Dissolve_Tex | Brightness | Other deltas |
|---|---|---|---|---|
| `Part01` | `T_VFX_Part_01` | `T_VFX_Part_01` | **1** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Distortion_Tex T_VFX_Noise_02`; `Gradient_Invert 0.5`; `Opacity_Boldness 0.5`; `Opacty_DepthFade 20` |
| `Part01_Bright` | `T_VFX_Part_02` | `T_VFX_Part_02` | **10** | as above minus `Core_Intensity`/`Opacity_Boldness` |
| `Part04` | `T_VFX_Part_04` | `T_VFX_Part_04` | **6** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Distortion_Tex T_VFX_Noise_02`; **`Opacty_DepthFade 30`** |
| `Flare01` | **`T_VFX_Ring_02`** | `T_VFX_Part_01` | **2** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Distortion_Tex T_VFX_Noise_02`; **`GradientShape_Tex T_VFX_Part_01`**; **`Gradient_Invert 0.847619`**; `Opacty_DepthFade 20` |
| `Impact01` | `T_VFX_Impact_01` | `T_VFX_Impact_01` | **12** | `Color_CoreDifferent 0`; `Core_Intensity 0`; **`Core_Power 0`**; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Distortion_Tex T_VFX_Noise_02`; `Gradient_Invert 0.5`; `Opacty_DepthFade 20` |
| `Lightning02` | `T_VFX_Lightning_03` | `T_VFX_Lightning_03` | **15** | `Color_CoreDifferent 0`; `Core_Intensity 0`; **`Core_Power 0`**; `Dissolve_Speed_X/Y 0`; **`Distortion_Intensity 0.5`**; `Distortion_Scale_X/Y 1`; **`Distortion_Speed_X/Y 0.7`**; `Opacty_DepthFade 20` |
| `LightStrip` | `T_VFX_LightStrip_01` | `T_VFX_LightStrip_01` | **7** | `Color_CoreDifferent 0`; `Core_Intensity 0`; **`Core_Power 0`**; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Distortion_Tex T_VFX_Noise_02`; `Gradient_Invert 0.5`; `Opacty_DepthFade 20` |
| `Rainbow` | **`T_VFX_Ring_02`** (Color_Tex `T_VFX_Part_01`) | `T_VFX_Part_01` | **1** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Distortion_Tex T_VFX_Noise_02`; **`GradientMap_Tex T_VFX_LUT_Rainbow_01`**; **`GradientMap_Displacement 0.9`**; **`GradientShape_Tex T_VFX_Part_01`**; **`Gradient_Invert 2`**; **`Opacity_Boldness 1.5`**; **`Opacty_StepAdd 0.3`**; `Opacty_DepthFade 20` |
| `Ring01` | `T_VFX_Ring_01` | `T_VFX_Ring_01` | **10** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Distortion_Tex T_VFX_Noise_02`; `Opacty_DepthFade 20` |
| `Star01` | `T_VFX_Star_01` | `T_VFX_Star_01` | **6** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Distortion_Tex T_VFX_Noise_02` |
| `Star04` | `T_VFX_Star_04` | **`T_VFX_Part_01`** | **1** | `Color_CoreDifferent 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Distortion_Tex T_VFX_Noise_02`; **`Opacty_DepthFade 0`** |
| `Flames01` | **`T_VFX_Wind_01`** | *(reference `T_VFX_Noise_04`)* | **10** | `Color_CoreDifferent 0`; **`Color_Core RGBA(0.015996, 0.014444, 0.014444, 1)`**; **`Dissolve −0.1`**; **`Dissolve_Scale_X/Y 2`**; `Dissolve_Speed_X/Y 0`; **`Distortion_Intensity 0.5`**; **`Distortion_Scale_X/Y 2`**; **`Distortion_Speed_X/Y −0.3`**; **`Glow_Intensity 2`**; `Opacty_DepthFade 20` |

Three instances carry live branches the rest do not:

1. **`Lightning02` and `Flames01` are the only instances with live distortion** (0.5 each, at speeds
   0.7/0.7 and −0.3/−0.3). `Flames01` is also the only one with a non-white `Color_Core`, a
   non-unit `Dissolve_Scale` (2) and `Glow_Intensity 2`.
2. **`Rainbow` is the only instance with a live gradient-map (LUT) chain** — everywhere else
   `GradientMap_Tex` is the white pixel and the chain is a provable no-op. See §6.5, gap 6.
3. **`Star04` is the only instance with `Opacty_DepthFade 0`** — depth fade explicitly off, unlike
   the 20/30 everywhere else. Since CkUsf does not wire scene depth at all, this instance is the one
   the existing gap does NOT affect.

### Family B — `M_VFX_FlatAdd` (**NEW — not implemented in CkUsf**)

`M_VFX_DisAdd_Flat02`, used by the two ribbon emitters and by the Spikes / Spike01 /
Bubble_First_Explo mesh renderers (five emitters in total). Despite the `DisAdd_` name prefix its
parent is `/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_FlatAdd`.

| | |
|---|---|
| Domain / blend / shading | `MD_Surface` / `BLEND_Translucent` / `MSM_Unlit`, `twoSided: false` |
| Connected outputs | `EmissiveColor` + `Opacity` |
| Expression histogram | `Multiply ×2`, `ScalarParameter ×3`, `VectorParameter ×1`, `ParticleColor ×1`, `DepthFade ×1`, `WorldPosition ×1`, `MaterialFunctionCall ×1` — **the whole graph** |
| Texture parameters | **NONE** (`textureParams: []`) |
| Dynamic parameters | **NONE** |
| Scalars on this instance | `Brightness **10**`, `Opacty_DepthFade **0**`, `CamOffset 0` |
| Vectors | `Color_Core RGBA(1, 1, 1, 0)` |

Emissive = `ParticleColor.rgb × Brightness`, opacity = `ParticleColor.a`. No texture, no dissolve,
no UV animation. The simplest look in the cookbook.

### Textures `[corpus]`

All 512×512, `sRGB: false`, `TC_Alpha`, `TEXTUREGROUP_World` unless noted.

| Texture | Format | Address | Role |
|---|---|---|---|
| `T_VFX_Part_01` | `TSF_G8` | `TA_Clamp`/`TA_Clamp` | Part01; Flare01/Rainbow/Star04 dissolve; Flare01/Rainbow gradient shape |
| `T_VFX_Part_02` | `TSF_G8` | `TA_Clamp`/`TA_Clamp` | Part01_Bright |
| `T_VFX_Part_04` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Part04 (velocity-aligned streak) |
| `T_VFX_Ring_01` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Ring01 |
| `T_VFX_Ring_02` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Flare01 + Rainbow main |
| `T_VFX_Star_01` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Star01 |
| `T_VFX_Star_04` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Star04 (the ground crack) |
| `T_VFX_Impact_01` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Impact01 |
| `T_VFX_LightStrip_01` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | LightStrip |
| `T_VFX_Wind_01` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Flames01 main |
| **`T_VFX_Lightning_03`** | **`TSF_BGRA8`** — colour, not a mask | `TA_Wrap`/`TA_Wrap` | Lightning02 main + dissolve; **the 2×2 sub-UV flipbook sheet** |
| `T_VFX_Noise_02` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Distortion_Tex on ten (**dead branch on all ten**) + GradientShape_Tex |
| `T_VFX_Noise_04` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Flames01's inherited Dissolve_Tex + Distortion_Tex (**its distortion branch IS live**) |
| `T_VFX_LUT_Rainbow_01` | `TSF_BGRA8`, **512×2**, `sRGB: true`, `TC_Default` | `TA_Wrap`/`TA_Wrap` | Rainbow's gradient LUT |
| `T_VFX_WhitePixel` | `TSF_RGBA16`, 1×1, `sRGB: true`, `TC_Default` | `TA_Wrap` | no-op GradientMap on eleven of twelve Family-A instances |

**The Flames pair also uses a 2×2 sub-UV sheet** (`SubUV: 2x2` on the renderer, `Sub UV Animation`
mode **Random**, frames 0 → 3) drawn from `T_VFX_Wind_01`.

---

## 5. Per-layer runtime curves `[corpus]`

`t` = NormalizedAge (0 → 1 over that emitter's own lifetime). `C` = constant key, `L` = linear key —
transcribed verbatim.

**Shared shapes referenced below:**

- **"shared fade"** — Scale Color
  `Scale RGBA = R (0,1)L (1,1)L | G (0,1)L (1,1)L | B (0,1)L (1,1)L | A (0,1)L (1,0)L`
  (RGB untouched, alpha 1 → 0), `Scale Alpha 1`, `Scale RGB (1,1,1)`; plus
  Scale Sprite Size Uniform `(0, 0.5)C (0.1, 1)L (1, 1)L`
- **"the lightning ramp"** — the five-key RGB curve shared by emitters 4, 6, 7, 12, 17, 19, 20:
  - R `(0, 1)L (0.074857, 1)L (0.598853, 1)C (0.811349, 0.287441)L (1, 0.051269)C`
  - G `(0, 0.745404)L (0.074857, 1)L (0.598853, 0.147027)C (0.811349, 0.040915)L (1, 0.040915)C`
  - B `(0, 0.304987)L (0.074857, 1)L (0.598853, 0.982251)C (0.811349, 1)L (1, 1)C`
  (Emitters 7 and 12 store the same keys at 6-digit precision: `0.0748566`, `0.0512695`,
  `0.0409152`.) **White → blue-white → deep blue.** Only the alpha differs per emitter.
- **"the ground-glow colour"** — R `(0, 0.752942)C (1, 0.327778)C` · G `(0, 0.162029)C (1, 0.0466651)C` ·
  B `(0, 1)C (1, 1)C` · A `(0, 1)C (1, 0)C` (shared by emitters 1 and 5; emitter 5 stores
  `0.046665` on G)

### 1 · GroundGlow_01 — burst 1 @0, life 0.2 s, size **2600**, `Color.Scale Alpha 1`

- Initialize color `RGBA(1, 1, 1, 1)`
- **`Align Sprite to Mesh Orientation`**: `Mesh Orientation Relative Sprite Alignment Vector (0, 1, 0)`,
  `Facing Vector (0, 0, 1)`, `Orientation Quaternion quat(0, 0, 0, 1)` — i.e. the quad lies FLAT in
  the local XY plane facing +Z, exactly the ground-decal configuration NS_Lightning_Range §6
  documents (renderer `CustomAlignment` + `CustomFacingVector`)
- Color from Curve: the ground-glow colour
- Scale Sprite Size Uniform: `(0, 0.5)C (0.1, 1)L (1, 1)L` · dyn `[1, 0, 0, 0]`

### 2 · GroundGlow_02 — burst 1 @0, life 0.2 s, size **4000** (the largest sprite in the batch)

- Initialize color `RGBA(0.043735, 0.0451862, 1, 0.5)`
- Same `Align Sprite to Mesh Orientation` triple as emitter 1
- shared fade · dyn `[**0**, 0, 0, 0]`

### 3 · Glow_01 — burst 1 @0, life 0.25 s, size 160

Initialize color `RGBA(0.0781874, 0.043735, 1, 0.5)` · shared fade · dyn `[**2**, 0, 0, 0]`

### 4 · Sparkles — burst **20** @0, size Random Uniform 5 … 20

- Lifetime `[corpus-v3]`: **`Lifetime Mode = Random` ⇒ the Min/Max pins DRIVE**
  (`lifetimeResolved.source = minmax`); the `Random Range Float` override (0.2 … 0.4) is INERT
  ([P0-D2]). The live pins are `Lifetime Min **1.0** / Max **0.5**` — **inverted**, recorded
  verbatim; `[unresolved: how Niagara resolves an inverted range]` (see §14 #8). *Was read as
  0.2 … 0.4 under the override-wins assumption, which sidestepped the inversion; the inversion is
  now load-bearing.*
- Spawn shape: `Sphere Location`, radius **100**, `Non Uniform Scale (1,1,1)`,
  **`Hemisphere Z = true`, `Hemisphere X = false`**
- `Add Velocity from Point`: strength `Random Range Float 001` **500 … 2000**
- Sprite rotation: `Random`, 0 … 360
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, **0.05**)C (1, 3.91223e-08)C` — a harder brake than the
  0.15 used elsewhere in the batch
- Color from Curve: the lightning ramp, with a **7-key strobing alpha**:
  A `(0.162994, 1)L (0.29339, **0**)L (0.347721, **0**)L (0.504679, 1)C (0.69182, **0**)L (0.767884, **0**)L (0.959855, 1)L`
  — two flat-zero plateaus, i.e. the sparks blink out twice mid-life
- Scale Sprite Size Uniform: `(0, 0)C (0.1, 1)C (1, 0)C` · `Color.Scale Alpha 1` · dyn `[1, 0, 0, 0]`

### 5 · Flare_01 — burst 1 @ t=**0.1**, life **0.1 s**, size 320, `Color.Scale Alpha **0.4**`

- Initialize color `RGBA(0.102242, 0.658375, 1, 0.2)`
- Color from Curve: the ground-glow colour
- Scale Sprite Size Uniform: `(0, 0.5)C (0.1, 1)L (1, 1)L` · dyn `[1, 0, 0, 0]`

### 6 · Sparkles_Stretched — burst **10** @0, loop **0.4 s Once** (`Self`), velocity-aligned

- Lifetime `[corpus-v3]`: **`Lifetime Min 0.2 / Max 0.4` drives** (`Random` mode, [P0-D2]); the
  `Random Range Float` override happens to carry the same range, so nothing moves here
- Sprite Size Mode **Random Non-Uniform**: `Sprite Size Min **(30, 120)**`, `Max **(50, 130)**`
- Spawn shape: `Sphere Location`, radius **100**, `Non Uniform Scale (1, 0.2, 0.2)`,
  `Hemisphere Z = true`, `Hemisphere X = false`
- `Add Velocity from Point`: strength `Random Range Float 001` **200 … 2000**
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, **0.25**)C (1, −9.09372e-09)C`
- Color from Curve: the lightning ramp, alpha `(0.162994, 1)L` — **single key, alpha is 1 for the
  whole life**
- **Three stacked size modules (they multiply):**
  1. Scale Sprite Size (Uniform Curve mode) `(0, 0)C (0.1, 1)C (1, 0)C`; its non-uniform curve
     `X (0,0)L (1,1)L | Y (0,0)L (1,1)L` is inert under Uniform mode
  2. Scale Sprite Size 001 (Non-Uniform Curve mode) `X (1, 1)L | Y (0, 1)C (0.3, 0.25)C (1, 0.2)C`
  3. **Scale Sprite Size by Speed** `Scale Factor Curve (0, 0)L (1, 1)L`
- `Color.Scale Alpha 1` · dyn `[**0**, 0, 0, 0]`

### 7 · Lightning_01 — burst **3** @0, loop **0.5 s Once** (`Self`), camera sprite, **SubUV 2×2**

- Lifetime `[corpus-v3]`: **`Lifetime Min 0.3 / Max 0.5` drives** (`Random` mode, [P0-D2]); the `Random Range Float` override (0.2 … 0.4) is inert
- Size: Random Uniform `Min **130** / Max **100**` — **inverted**, recorded verbatim
- Spawn shape: `Sphere Location`, radius **100**, `Hemisphere Z = true`
- `Add Velocity from Point`: strength **350 … 500**
- Sprite rotation: `Random`, 0 … 360; **Sprite Rotation Rate** = `Float from Curve 002`
  `(0, 1.68162e-07)C (0.1, **90**)C (0.9, 1.43051e-06)C`
- **Sub UV Animation**: mode `Linear`, `Start Frame 0`, `End Frame **4**`, `SubUV Loop Count 1`
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`
- Color from Curve: the lightning ramp, with the **strobing 5-key alpha**
  `(0.162994, 1)L (0.329611, **0**)L (0.504679, 1)C (0.746152, **0**)L (0.959855, 1)L`
  — byte-identical to NS_Lightning_Cast's and NS_Lightning_Muzzle's Lightning layers
- Dyn param 1 — Float from Curve `(0, 1)C (0.2, **−1**)C (0.3, **0.875**)C (1, −1)C`; 2/3/4 = 0
- Scale Sprite Size Uniform: `(0, 0)C (0.1, 0.8)C (1, 1)C` · `Color.Scale Alpha 1`

### 8 · Spikes — burst **5** @0, life **0.15 s**, **mesh** `SM_VFX_Spike01` / `Flat02`, `Facing: Velocity`

- Initialize color `RGBA(0.871367, 0.06301, 1, 0.5)` — **no `Color` module**, so colour is constant
- `Mesh Scale Mode = Non-Uniform`, `Mesh Scale = Random Range Vector 001`
  `Minimum **(0.1, 0.1, 0.6)**` … `Maximum **(0.1, 0.1, 0.4)**` (Z inverted, verbatim)
- Spawn shape: `Sphere Location`, radius **10**, `Non Uniform Scale (1, 0.2, 0.2)`,
  `Hemisphere Z = true`, `Hemisphere X = false`
- `Add Velocity from Point`: strength `Minimum **200**` … `Maximum **50**` (inverted, verbatim)
- `Add Velocity`: `Random Range Vector` **(0, 0, 100) … (0, 0, 500)** — pure **+Z**
- `Initial Mesh Orientation`: `Rotation Coordinate Space = Mesh`, `Orientation Axis (1, 0, 0)`,
  `Orientation Vector (0, 0, −1)`, `Rotation (0, 0, 0)`
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`
- **Scale Mesh Size** (`Vector from Curve 001`):
  `X (0, 0)C (0.2, 1)C (1, 0)C | Y (0, −2.4747e-08)C (0.2, 1)C (1, 0)C | Z (0, −5.68323e-08)C (0.2, 1)C`
  — Z has no third key, so the spikes flatten rather than shrink
- dyn `[1, 0, 0, 0]` — inert, `Flat02` declares no dynamic parameters

### 9 · LightningArc_01 — **ribbon**, loop **0.3 s Once** (`Self`)

Byte-identical to NS_Lightning_Muzzle §5's LightningArc_01:

- Spawn: burst 1 @0 **+** `Spawn Rate` override `Float from Curve 002` `(0, **80**)C (1, 0)C`
- `Initialize Ribbon`: `Lifetime Mode = Random`, `Lifetime Min **0.2** / Max **0.3**`,
  `Color RGBA(1,1,1,1)`; **`Ribbon Width` = `Random Range Float` 7 … 12**
- Spawn shape: `Sphere Location`, radius **10**, `Non Uniform Scale (1, 0.2, 0.2)`,
  **`Hemisphere X = true`** (the two arcs are the only +X emitters in this system)
- `Add Velocity from Point`: strength **10 … 50**; `Add Velocity`: `Random Range Vector`
  **(700, 0, 0) … (1500, 0, 0)**
- **Curl Noise Force**: `Noise Strength = Float from Curve 001` `(0, **10000**)C (1, 0)C`,
  `Noise Frequency **500**`, `Random Seed **11**`, `Randomization Vector (0.65, 0.125, 0.37)`,
  `Cone Mask Angle 45`, `Cone Mask Falloff Angle 45`, `Pan Noise Field (0,0,0)`
- Velocity Scale: `X/Y/Z (0, 1)C (1, 0)C`
- Color from Curve: R `(0.511923, 1)C (0.813764, 0.584079)C` · G `(0.511923, 1)C (0.813764, 0.06301)C` ·
  B `(0.511923, 1)C (0.813764, 1)C` · A `(0, 0)L (0.220948, 1)L (0.51313, 1)C (1, 0)L`
- **Scale Ribbon Width**: `(0, 4.18339e-08)C (0.2, 1)C (1, 0.4)C`
- `Color.Scale Alpha 1`, material `Flat02`
- `Solve Forces and Velocity`: `Acceleration Limit 9999`, `Speed Limit **1000**` — **this clamp is
  active** (launch 700–1500 u/s plus a 10000-strength curl force)

### 10 · LightningArc_02 — **ribbon**, loop **0.3 s Once** (`Self`)

Identical to Arc_01 except:

| | Arc_01 | Arc_02 |
|---|---|---|
| Burst @0 | 1 | **5** |
| Ribbon Width | 7 … 12 | **2 … 4** |
| `Add Velocity` | (700,0,0) … (1500,0,0) | **(2500,0,0) … (3000,0,0)** |
| Curl `Noise Frequency` | 500 | **1000** |
| Velocity Scale | `(0, 1)C (1, 0)C` | `(0, 1)C (1, **0.2**)C` |
| `Color.Scale Alpha` | 1 | **0.5** |
| Colour R | `(0.511923, 1)C (0.813764, 0.584079)C` | `(0.511923, **0.135751**)C (0.813764, **0.570314**)C` |
| Colour G | `(0.511923, 1)C (0.813764, 0.06301)C` | `(0.511923, **0.036**)C (0.813764, **0.0319999**)C` |
| Colour B / A | as Arc_01 | identical |

### 11 · LightningStrip — burst **3** @0, life 0.3 s, **mesh** `SM_VFX_Plane01` / `LightStrip`, `Facing: Velocity`

- Initialize color `RGBA(0.341915, 0.184475, 1, 1)`, `Color.Scale Alpha **0.3**`
- `Mesh Scale Mode = Non-Uniform`, `Mesh Scale = Random Range Vector 001`
  `Minimum **(2, 1, 2)**` … `Maximum **(3, 1, 4)**`
- `Initial Mesh Orientation`: `Rotation Coordinate Space = Mesh`, `Orientation Axis (1, 0, 0)`,
  `Rotation (0, 0, 0)`, and **`Orientation Vector` = `Random Range Vector`**
  `Minimum **(1, 1, 0)**` … `Maximum **(−1, −1, −1)**` (inverted, verbatim) — a randomly oriented
  strip per particle
- **`Sphere Location` is DISABLED** on this emitter (its `Hemisphere Z = true` override and
  `Sphere Radius 50` are therefore inert) — every strip spawns at the origin
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C` (inert — no velocity added)
- Color from Curve: R `(0.315122, 0.354443)L` · G `(0.315122, 0.2)L` · B `(0.315122, 1)L`
  (single key each → constant RGB) · A `(0, 0)C (0.317537, 1)L (1, 0)C`
- **Scale Mesh Size** (`Vector from Curve 001`):
  `X (0, 0.5)C (0.2, 1)C (1, −1.44926e-08)C | Y (0, −2.4747e-08)C (1, 1)C | Z (0, −5.68323e-08)C (0.2, 0.75)C (1, 1)C`
- dyn `[**0**, 0, 0, 0]`

### 12 · Lightning_02 — **rate only**, loop **0.5 s Once** (`Self`), camera sprite, **SubUV 2×2**

- Spawn: `Spawn Rate` override `Float from Curve 001` `(0, **10**)C (1, 0)C` — **no burst module at
  all**, ≈ 2.5 particles over the 0.5 s loop
- Lifetime `[corpus-v3]`: **`Lifetime Min 0.3 / Max 0.5` drives** (`Random` mode, [P0-D2]); the `Random Range Float` override (0.2 … 0.4) is inert
- Size: Random Uniform `Min **130** / Max **100**` (inverted, verbatim)
- Spawn shape: `Sphere Location`, radius **30**, `Hemisphere Z = true`
- `Add Velocity from Point`: strength **350 … 500**
- Sprite rotation, Sub UV Animation, Velocity Scale, Color curve (strobing 5-key alpha), dyn-param
  curve `(0, 1)C (0.2, −1)C (0.3, 0.875)C (1, −1)C` and size curve `(0, 0)C (0.1, 0.8)C (1, 1)C`:
  **identical to emitter 7**. Lightning_02 is Lightning_01 as a trickle instead of a burst, from a
  30-unit sphere instead of 100.

### 13 · FlareImpact — burst 1 @ t=**0.1**, life **0.05 s** (the shortest in the batch), size 130

- Initialize color `RGBA(0.644888, 0.2, 1, 1)`
- Scale Color: `Scale RGBA = R (0,1)L (1,1)L | G (0,1)L (1,1)L | B (0,1)L (1,1)L |`
  `A (**0.237851**, 1)L (1, 0)L` — alpha holds 1 until t = 0.237851, then falls
- Scale Sprite Size Uniform: `(0, 0.5)C (0.1, 0.9)C (1, 1)C` · dyn `[1, 0, 0, 0]`

### 14 · Raimbow — **WORLD space**, burst 1 @ t=**0.05**, life **0.1 s**, size 500

- Initialize color `RGBA(0.913099, 0.913099, 0.913099, **0.2**)`
- Sprite rotation: `Random`, 0 … 360
- A `Color` module is present with no override (`Scale Alpha 1`, `Scale Color (1,1,1)`)
- Scale Color: `Scale RGBA = R (0, 0.5)L | G (0, 0.5)L | B (0, 0.5)L | A (0, 1)L (1, 0)L`
  — RGB halved for the whole life (single key), alpha 1 → 0
- Scale Sprite Size Uniform: `(0, 0.5)C (0.2, 0.9)C (1, 1)L` · dyn `[**0.5**, 0, 0, 0]`

### 15 · Ring — **WORLD space**, burst 1 @ t=**0.05**, life 0.3 s, size 230

- Initialize color `RGBA(0.913099, 0.191202, 1, 0.608)`
- Sprite rotation: `Random`, 0 … 360
- Dyn param 1 — Float from Curve `(0, 0)C (1, −1)C`; 2/3/4 = 0
- Scale Sprite Size Uniform: `(0, 0.5)C (0.1, 0.9)C (1, 1)C`
- **No colour animation** — Particle Update is
  `Particle State → Dynamic Material Parameters → Scale Sprite Size`. The ring holds its initialize
  colour and alpha 0.608 and disappears purely by dissolve.
- Inert pins: `Lifetime Min 0.3 / Max 0.7`, `Uniform Sprite Size Min 150 / Max 160`

### 16 · GroundCrack_01 — burst 1 @ t=**0.1**, life **1 s** (the longest here), size 600

- Initialize color `RGBA(1, 1, 1, 1)`, `Color.Scale Alpha 1`
- **`Align Sprite to Mesh Orientation`**: alignment `(0, 1, 0)`, facing `(0, 0, 1)`,
  quaternion `quat(0, 0, 0, 1)` — flat on the ground, renderer `CustomAlignment` +
  `CustomFacingVector`
- Sprite rotation: `Random`, 0 … 360
- Color from Curve — **a very fast white → blue → black collapse in the first 13 % of life**:
  - R `(0, 1)C (0.0494285, 0.361307)L (0.12975, 0.00518152)L`
  - G `(0, 0.89627)C (0.0494285, 0.152926)L (0.12975, 0.00518152)L`
  - B `(0, 0.520996)C (0.0494285, **1**)L (0.12975, 0.00913406)L`
  - A `(0, 1)C (1, 0)C`
- Scale Sprite Size Uniform: `(0, 0.5)C (0.1, 1)L (1, 1)L` · dyn `[1, 0, 0, 0]`

### 17 · Star02 — **WORLD space**, burst **5** @0, loop **0.3 s Once** (`Self`), material `Star01`

- Lifetime: `Lifetime Mode = Random`, `Lifetime Min **0.9** / Max **1.3**` — **there is NO
  `[override] Lifetime` on this emitter**, so the Random-mode pins are unambiguously live here
- Size: Random Uniform **30 … 50**
- Spawn shape: `Sphere Location`, radius **70**, `Non Uniform Scale (1, 0.2, 0.2)`,
  `Hemisphere Z = true`
- `Add Velocity from Point`: strength `Random Range Float 001` **200 … 1500**
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, **0.05**)C (1, −0)C`
- Color from Curve: the lightning ramp, alpha `(0.162994, 1)L` — single key, alpha 1 throughout
- Scale Sprite Size Uniform: `(0, **−3.11599e-08**)C (0.1, 1)C (1, 0)C`
- `Color.Scale Alpha 1` · dyn `[1, 0, 0, 0]`
- Note the material/emitter name skew (**do not "fix" it**): the emitter is called `Star02` and
  renders through `M_VFX_DisAdd_**Star01**` — the same class of authored skew NS_BasicAttack §1
  records for `Slash_03` → `..._Slash04`

### 18 · Glow_02 — burst 1 @0, life 0.25 s, size 80

Initialize color `RGBA(1, 0.043735, 0.83077, 0.5)` (magenta) · shared fade · dyn `[**2**, 0, 0, 0]`

### 19 · Bubble_First_Explo — **WORLD**, burst 1 @0, life **0.15 s**, **mesh** `SM_VFX_Sphere01` / `Flat02`, `Facing: Default`

- Initialize color `RGBA(1, 0.184475, 0.386429, 1)`, `Color.Scale Alpha 1`
- `Mesh Scale Mode = **Uniform**`, `Mesh Uniform Scale **0.8**` (a 160-unit-diameter sphere from the
  200-unit source mesh)
- Color from Curve: the lightning ramp, alpha `(0.162994, 1)L` — single key, alpha 1 throughout
- **Scale Mesh Size** (`Scale Float by Curve`): `X/Y/Z (0, 0)C (0.2, **1.5**)C (1, 1)C` — overshoots
  to 1.5× at t = 0.2 then settles back to 1
- Writes no dynamic parameters (`Flat02` declares none)

### 20 · Spike01 — **WORLD**, burst **5** @0, life **0.15 s**, **mesh** `SM_VFX_Spike01` / `Flat02`, `Facing: Default`

- Initialize color `RGBA(1, 0.184475, 0.386429, 1)`, `Color.Scale Alpha 1`
- `Mesh Scale Mode = **Random Non-Uniform**`, `Mesh Scale Min **(0.2, 0.2, 0.5)**` …
  `Max **(0.4, 0.4, 1.5)**` (this one is NOT inverted)
- **`Cone Location` is DISABLED** on this emitter — all five spikes spawn at the origin
- `Initial Mesh Orientation`: `Rotation Coordinate Space = Mesh`, `Orientation Axis (1, 0, 0)`,
  **`Orientation Vector (1, 0, 0)`** (not the `(0,0,−1)` the other spike emitters use), and
  **`Rotation` = `Random Range Vector`** `Minimum **(0, 0, 1)**` … `Maximum **(0, 0.5, −1)**` — a
  random per-particle tilt
- **Scale Mesh Size** (`Scale Float by Curve`):
  `X (0, 0)C (0.2, 1.5)C (1, 4.17233e-08)C | Y (0, 0)C (0.2, 1.5)C (1, 5.66244e-08)C | Z (0, 0)C (0.2, 1.5)C`
  — X/Y collapse to 0, Z has no third key and holds 1.5
- Color from Curve: the lightning ramp, alpha `(0.162994, 1)L` — single key

### 21 · Flames_01 — **WORLD**, burst **10** @ t=**0.05**, loop **0.3 s Once** (`Self`), **SubUV 2×2**

- Lifetime `[corpus-v3]`: **`Lifetime Min 0.2 / Max 0.7` drives** (`Random` mode, [P0-D2]); the `Random Range Float` override (0.2 … 0.4) is inert
- Size: Random Uniform **200 … 300**
- Spawn shape: `Sphere Location`, radius **20**, **`Surface Expansion Mode = Outside`**,
  `Hemisphere Z = true`
- `Add Velocity from Point`: strength `Random Range Float 002` **100 … 700**
- **Sub UV Animation: mode `Random`** (not Linear), `Start Frame 0`, `End Frame **3**`,
  `SubUV Loop Count 1` — each flame picks a random frame of the 2×2 sheet
- Sprite rotation rate: `Random Range Float 001` **−45 … +45** °/s
- Velocity Scale: `X/Y/Z (0, 1)C (1, 0)C`
- Color from Curve — the only non-lightning, non-ground palette in the system:
  - R `(0.269242, 0.387163)L (0.82946, 0.066626)L`
  - G `(0.269242, 0.0708376)L (0.82946, 0.00749903)L`
  - B `(0.269242, 0.708376)L (0.82946, 0.250158)L`
  - A `(0, 0)L (0.455177, 1)L (1, 0)L`
- Dyn params: **1** = Float from Curve `(0, −4.10064e-08)C (1, **−1**)C`; **2 = `10`**
  (the only non-zero `distortion` channel in this batch); 3 = 0; 4 = 0
- Scale Sprite Size Uniform: `(0, 0.5)C (1, 1)C` · `Color.Scale Alpha **0.5**`

### 22 · Flames_02 — **WORLD**, burst **10** @ t=**0.05**, loop **0.3 s Once** (`Self`), **SubUV 2×2**

Identical to Flames_01 except:

- Instead of `Add Velocity from Point`, it uses **`Add Velocity` = `Random Range Vector`**
  `Minimum **(0, 0, 800)**` … `Maximum **(0, 0, 1200)**` — a straight upward jet
- Colour keys shift slightly: R `(**0.245095**, 0.387163)L (0.82946, 0.066626)L` ·
  G `(0.245095, 0.0708376)L (0.82946, **0.007499**)L` · B `(0.245095, 0.708376)L (0.82946, 0.250158)L` ·
  A `(0, 0)L (0.455177, 1)L (1, 0)L`
- Everything else — size 200 … 300, sphere radius 20 Outside/Hemisphere Z, SubUV Random 0 … 3,
  rotation rate ±45, velocity scale, dyn `[curve, 10, 0, 0]`, size curve, `Scale Alpha 0.5` — matches

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new burst row is required `[corpus-v3]`, per [P0-D3]: loop 2.0 s, particle lifetime 1.3 s,
burst 87.** Loop = the system's `Once` loop duration (*was 1.0 s, from the inert emitter rows*);
lifetime = max resolved emitter lifetime — 1.3 s, Star02's `Lifetime Max` (unchanged by [P0-D2]:
Star02 was already read from its Min/Max pins); burst = the §2 count. GroundCrack_01's flat 1.0 s is
the longest deterministic one. Every shorter layer zeroes colour, size and scale past its own lifetime, and the four delayed
emitters (0.05 s on Raimbow/Ring/Flames pair, 0.1 s on Flare_01/FlareImpact/GroundCrack_01) hide for
`age < delay` and run their curves on `(age − delay) / lifetime` (NS_BasicAttack §5).

**87 particles is nearly the shared burst template's entire 96-particle budget** and 4.6× the Slash
row's 19. Verify the GPU cost of an 87-slot `Seed % 87` switch before committing — every particle
evaluates the full behavior branch.

What the row cannot carry:

- The **three rate-spawned emitters** (two ribbon arcs at 80 → 0 /s over 0.3 s, Lightning_02 at
  10 → 0 /s over 0.5 s). The two arcs are moot until ribbons exist (gap 1); Lightning_02's ≈ 2.5
  particles can be folded into the burst as 3 slots at delays
  `t_k = 0.5·(1 − sqrt(1 − k/2.5))` — **record it as a deviation, do not present it as faithful**.
- The **eight `Self` one-shot emitters** run ONCE and never repeat while the other 14 cycle.
  See gap 3.

Loop duration resolved `[corpus-v3]` — see §2.

### 6.2 VisTag / renderer needs

| Source renderer | Emitters | Distinct looks | CkParticles today |
|---|---|---|---|
| Camera-facing sprite | 8 (3, 4, 5, 13, 14, 15, 17, 18) | 6 (`Part01`, `Part01_Bright`, `Flare01`, `Impact01`, `Rainbow`, `Ring01`, `Star01`) | **row-level kind does not exist** (gap 4) |
| Camera sprite **+ SubUV 2×2** | 4 (7, 12, 21, 22) | 2 (`Lightning02`, `Flames01`) | **no sub-UV support at all** (gap 5) |
| Sprite, **`CustomAlignment` + `CustomFacingVector`** | 3 (1, 2, 16) | 2 (`Part01`, `Star04`) | ✔ **VisTag 4** exists (added by NS_Lightning_Range) — but it shares `User.SpriteMaterial` with VisTag 0, so it can carry only ONE of the two looks (gap 4 again) |
| Velocity-aligned sprite | 1 (6) | 1 (`Part04`) | ✔ existing `VelocityAlignedSprite` row kind |
| **Ribbon** | **2** (9, 10) | 1 (`Flat02`) | **NOT SUPPORTED** (gap 1) |
| Mesh, `Facing: Velocity` | 2 (8, 11) | 2 (`Flat02` on Spike01, `LightStrip` on Plane01) | ✔ existing `Mesh` row kind |
| Mesh, `Facing: Default` | 2 (19, 20) | 1 (`Flat02` on Sphere01 and Spike01) | ✔ existing `Mesh` row kind |

VisTag ids: allocate at implementation time above `Get_RosterVisTag_Max()`; never restate a literal.

### 6.3 Mesh / texture / look needs

**Meshes: 3 new procedural generations** (§3):

- `SM_CkParticles_Spike` — square-base pyramid, 6 triangles (shared with NS_Lightning_Muzzle).
- `SM_CkParticles_Plane` — single flat quad `X ∈ [−100,100]`, `Z ∈ [0,200]`, `v` inverted against Z;
  build as ONE quad + a `_TwoSided` look, not the source's hand-doubled 4-triangle version
  (shared with NS_Lightning_Muzzle).
- `SM_CkParticles_Sphere` — radius-100 UV sphere, 960 triangles. **`SM_CkParticles_Shell` already
  exists** in the carrier-mesh set (`Sweep`/`Tube`/`Shell`/`Disc`); check whether it is a usable
  sphere before generating a new one.

**CkUsf looks: 12 new** — 11 Family-A parameterizations of `CkUsf_Look_DissolveAdd` per §4, plus
**`FlatAdd`**, the new family (see NS_Lightning_Muzzle §6.3 — the same one-look addition serves both
effects).

**Textures: 11 procedural stand-ins**, following the NS_BasicAttack §7 method:

- `T_VFX_Part_01` → **`T_CkParticles_SoftParticle` already exists**, measured off this asset. Reuse.
- `T_VFX_Part_04` → **`T_CkParticles_SparkStreak` already exists**, measured off this asset. Reuse.
- `T_VFX_Noise_02` → existing `T_CkParticles_TileNoise` (branch dead on all ten users here).
- `T_VFX_Noise_04` → likely `T_CkParticles_TileNoise` too, but **its branch IS live on Flames01**
  (`Distortion_Intensity 0.5`, `Dissolve_Scale 2`) — measure rather than assume.
- `T_VFX_Part_02`, `T_VFX_Ring_01`, `T_VFX_Ring_02`, `T_VFX_Star_01`, `T_VFX_Star_04`,
  `T_VFX_Impact_01`, `T_VFX_LightStrip_01`, `T_VFX_Wind_01` — **new bakes**; measure each.
  (`T_VFX_Wind_03` already has a measured stand-in, `T_CkParticles_WindBand`, from NS_BasicAttack §7
  — `T_VFX_Wind_01` is a **different** asset and additionally serves as a 2×2 flipbook here.)
- `T_VFX_Lightning_03` — a 2×2 `TSF_BGRA8` colour flipbook. See gap 5.
- `T_VFX_LUT_Rainbow_01` — a 512×2 colour LUT. See gap 6.

### 6.4 Behavior id

**Do NOT allocate an id in this document.** Take the next free id from `ck::particles::NumBehaviors`
at implementation time and bump it. This batch contains five planned effects; whoever implements
second must re-read the roster, not this sheet.

Layer partition: `Seed % 87` over the 87 deterministic burst slots, in the §2 table order. A modulo
over a burst of exactly N gives the source's partition by construction; never `Rand(Seed) < k`
probability bands (NS_BasicAttack lesson 2). **87 is large enough that the switch cost is worth
measuring** — see §6.1.

**The aim axis is +Z**, matching the roster's ImpactBurst convention. The two ribbon arcs are the
exception (+X); if ribbons are dropped (gap 1) the whole recreation is cleanly +Z.

### 6.5 CAPABILITY GAPS — what the pipeline cannot express today

Conservative list. **This effect has more blocking gaps than any other in the batch.**

1. **RIBBON RENDERERS ARE NOT SUPPORTED.** Two emitters (LightningArc_01/02, ≈ 26 particles) render
   through `NiagaraRibbonRendererProperties`. Nothing in CkParticles can express this:
   `FCk_ParticlesRendererSpec` has no ribbon kind, the builder has never emitted one, the DI writes
   no `RibbonWidth`/`RibbonID`/`RibbonLinkOrder`, and **CkUsf deliberately omits the Niagara ribbon
   usage flag** (NS_Lightning_Range §9). Same gap as NS_Lightning_Muzzle — build it once for both.
2. **MIXED COORDINATE SPACE — 15 local, 7 world emitters in ONE system.** No CkParticles template
   can be both: the emitter's `LocalSpace` is a template-level property. The seven world-space
   layers (Raimbow, Ring, Star02, Bubble_First_Explo, Spike01, Flames_01, Flames_02) would either
   follow the spawning actor (if the template stays local) or collapse to the world origin (if it
   goes world, since self-driving behaviors write absolute positions). **This is unique to this
   effect** — every other system in the batch is uniformly one space. Options: split into two
   behaviors/templates, or accept that seven layers follow the actor and record it. Decide before
   writing HLSL.
3. **One template = one cadence; this source has three, across EIGHT one-shot emitters.**
   Fourteen System-mode emitters cycle on the system clock; Sparkles_Stretched (0.4 s),
   Lightning_01/_02 (0.5 s), LightningArc_01/02, Star02, Flames_01/_02 (0.3 s) are
   `Life Cycle Mode = Self` + `Loop Behavior = Once` — they play **once and never repeat**. A
   CkParticles template replays everything every loop, which turns a one-shot ground impact into a
   pulsing loop. `CkParticles/CLAUDE.md` forbids faking cadence with `frac(Age/Cycle)` inside the
   behavior, so this must be an honest deviation or a template split.
4. **No row-level camera-facing-sprite renderer**, and the existing custom-facing VisTag 4 can carry
   only one look. Twelve sprite layers over eight distinct looks (six camera-facing + two
   custom-facing). **Blocking.** Additive fix, mirrors `VelocityAlignedSprite`; the custom-facing
   case additionally needs a row-declarable variant of VisTag 4 rather than the single shared
   `User.SpriteMaterial`. Same gap in all five effects in this batch.
5. **No sub-UV / flipbook support anywhere in the pipeline — and this effect needs TWO modes.**
   Lightning_01/_02 use `Sub UV Animation` mode **Linear** (frames 0 → 4) on `T_VFX_Lightning_03`;
   Flames_01/_02 use mode **Random** (frames 0 → 3) on `T_VFX_Wind_01`. No `SubImageIndex` stage
   output, no `SubImageSize` on the renderer spec, no atlas bake in the texture generator. Four of
   22 emitters (23 burst particles) depend on it.
6. **The gradient-map (LUT) chain is not implemented in `CkUsf_Look_DissolveAdd`.** `Rainbow` uses a
   real 512×2 colour LUT (`GradientMap_Displacement 0.9`, `Gradient_Invert 2`). Both shipped recipes
   dropped the chain **because their GradientMap was a white pixel**; that justification does not
   hold here. Either extend the family shader or record the Raimbow layer as a deliberate loss.
7. **Curl Noise Force has no CkParticles equivalent, and it is not closed-form.** Both ribbon
   emitters drive a 3D curl-noise field (frequency 500 / 1000, strength 10000 → 0, `Random Seed 11`,
   `Randomization Vector (0.65, 0.125, 0.37)`, 45° cone mask) integrated stepwise **under an active
   1000 u/s speed limit**. CkParticles behaviors write closed-form positions to keep GPU and CPU in
   lockstep (NS_BasicAttack lesson 7); a stepwise force integration with a clamp is not closed-form.
   Needs either a deterministic closed-form noise displacement evaluated at `Age`, or an accepted
   approximation. Moot if gap 1 is not closed.
8. **Six inverted `Random Range` ranges** (`min > max`), all recorded verbatim rather than
   normalized: Sparkles' lifetime pins (1 … 0.5), Lightning_01 and Lightning_02's Uniform Sprite Size
   (130 … 100), Spikes' velocity strength (200 … 50) and Mesh Scale Z (0.6 … 0.4), LightningStrip's
   Orientation Vector ((1,1,0) … (−1,−1,−1)).
   `[unresolved: how Niagara resolves an inverted range.]` Silently swapping them changes the effect
   and hides authoring quirks that may be deliberate.
9. **Two spawn-shape modules are DISABLED and their pins are therefore inert**: LightningStrip's
   `Sphere Location` (radius 50, Hemisphere Z) and Spike01's `Cone Location`. Both emitters spawn all
   their particles at the origin. Recorded so the values are not "recovered" by a later reader.
10. **`Facing: Velocity` on a mesh renderer vs `Initial Mesh Orientation`.** Spikes and
    LightningStrip set both. `[inferred: the renderer's velocity facing wins over the module's
    quaternion.]` The CkParticles mesh path drives orientation purely from `O.Orientation`, which can
    encode either — pick the right one and confirm it against the renderer.
11. **Family parameters not plumbed through `CkUsf_Look_DissolveAdd`**: `Core_Intensity` (0 on ten of
    twelve), `Core_Power` (**0** on Impact01, Lightning02, LightStrip), `Gradient_Invert`
    (0.5 on four, **0.847619** on Flare01, **2** on Rainbow), `Opacty_StepAdd` (**0.3** on Rainbow),
    `Glow_Intensity` (**2** on Flames01), a non-white `Color_Core`
    (`RGBA(0.015996, 0.014444, 0.014444, 1)` on Flames01), and `Opacty_DepthFade` (a pre-existing
    documented CkUsf gap; **0** on Star04, 20/30 elsewhere). `Dissolve_Scale` (**2** on Flames01) and
    `DissolveBias` (**−0.1** on Flames01) ARE plumbed.
12. **The `Star02` emitter renders through the `Star01` material** — an authored name skew, the same
    class NS_BasicAttack §1 records for `Slash_03` → `..._Slash04`. **Do not "fix" it.**

**Not gaps — confirmed absent from this source:** no light renderers, no GPU sims, no collision, no
event handlers, no user parameters, no material-binding indirection.

---

## 7+. Reserved for implementation.
