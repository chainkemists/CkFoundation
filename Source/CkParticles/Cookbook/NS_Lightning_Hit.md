# Recipe: NS_Lightning_Hit → CkParticles (IMPLEMENTED)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: IMPLEMENTED as `BehaviorId 45` (2026-08-02) — the campaign's last port.**

`Behavior_LightningHit.ush` + `ExecuteStage_CPU` case 45, the
`PS_CkParticles_Template_LightningHit` cadence row (2.0 s loop / 1.3 s lifetime / burst 84 + a
ribbon emitter bursting 30), sixteen row renderers on VisTags **225–240** plus the ribbon's **241**,
`Test_Particles_LightningHitBehavior.cpp`, and the VfxExamples gym pair.

**It added ZERO new assets** — no look, no texture bake, no mesh. Every one of its thirteen material
instances and all three of its meshes were already carried by earlier ports, and each was checked
value-by-value against §4/§3 before reuse (§10, §8). That is the integration proof this port exists
to be: the widest system in the pack, drawn entirely out of the library the previous twenty-eight
ports built.

**It is still the least portable effect in the pack** — 22 emitters, ribbons, sub-UV flipbooks in
BOTH of Niagara's modes, three mesh carriers over four renderers, three custom-facing ground quads,
and mixed local/world space. §6.5 is answered gap by gap; the surviving differences are in §13.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Lightning_Hit` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior ID | **45** (`LightningHit`) |
| CkUsf looks | **none new** — fourteen existing looks, listed in §10 |

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

**Row assignment `[P5-H1]`:** six of those 87 belong to the two RIBBON emitters, so the cadence row's
main burst is **84** (81 sprite/mesh + Lightning_02's three solved points) and its ribbon emitter
bursts **30** (6 burst + 24 solved). See §6.1.

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
- Spawn shape: `Sphere Location`, radius **100**, `Hemisphere Z = true`, `Hemisphere X = false`.
  Its stored `Non Uniform Scale (1, 0.2, 0.2)` is **INERT** — `UseNonUniformScale = false` `[corpus]`,
  so the cloud is a full hemisphere and not the thin cigar the squash would make ([P5-H3])
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
- Spawn shape: `Sphere Location`, radius **100**, `Hemisphere Z = true`, **`Surface Only = true`**
  `[corpus]` — the ONE emitter in this system that spawns on the sphere's SURFACE rather than through
  its volume, so its three bolts sit at a fixed 100 units rather than filling the ball ([P5-H5])
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
- Spawn shape: `Sphere Location`, radius **70**, `Hemisphere Z = true`. Its stored
  `Non Uniform Scale (1, 0.2, 0.2)` is **INERT** — `UseNonUniformScale = false` `[corpus]` ([P5-H4])
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
- Spawn shape: `Sphere Location`, radius **20**, `Hemisphere Z = true`. Its
  `Surface Expansion Mode = Outside` is **INERT** — `Surface Only = false` `[corpus]`, so the puffs
  fill the 20-unit volume rather than sitting on its shell ([P5-H6])
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

## 6. Translation plan (CkParticles / CkUsf) — AS IMPLEMENTED

### 6.1 Cadence row

**`PS_CkParticles_Template_LightningHit`: loop 2.0 s, particle lifetime 1.3 s, burst 84, plus a
ribbon emitter bursting 30.**

- **Loop** = the system's own `Loop Once / 2.0 s` `[corpus-v3]`, which rules the fourteen
  `Life Cycle Mode = System` emitters ([P0-D1]).
- **Lifetime** = max over layers of (spawn delay + resolved lifetime) ([P0-D5]) = **1.3 s**, Star02's
  `Lifetime Max`. The runners-up are GroundCrack_01's 0.1 + 1.0 = 1.1 and Lightning_02's last solved
  release at 0.5 plus its own 0.5.
- **Burst 84**, not the 87 this sheet's §2 counted. **[P5-H1] correction, the [P3-G6] class, second
  sighting:** 87 includes LightningArc_01's one and LightningArc_02's five burst particles, and those
  are RIBBON particles on the row's second emitter, not sprites on the first. The twenty sprite/mesh
  emitters carry 81 between them; Lightning_02 adds the three solved points below; the arcs' six ride
  the ribbon emitter with their twenty-four solved ones. Itemization right, row assignment wrong —
  exactly what NS_Lightning_Muzzle's §6.1 got wrong for the same two emitters.
- **The row declares NO spawn rate.** Both of the source's rate-driven populations are integrable, so
  [P3-G8] inverts their CDFs rather than reading the emitter clock, and `RosterSanity`'s derived rule
  therefore REQUIRES behavior 45 to be emitter-clock independent (asserted).

**Lightning_02 — a rate with no burst module at all.** Its `Spawn Rate` override falls 10 → 0 across
its own 0.5 s `Self / Once` window, integrating to 2.5 particles. Under [P3-G8]'s midpoint convention
(the same one NS_Lightning_Muzzle's arcs use) point *i* of three is released at
`0.5 · (1 − sqrt(1 − (i + 0.5)/2.5))` — **0.052786 / 0.183772 / 0.500000**. **[P5-H2]**: this sheet's
plan already stated that formula, but wrote its numerator as `k` without saying where `k` starts;
`k = i + 0.5` is the reading that matches [P3-G8], and it is what shipped. The third point lands
exactly at the window edge, because the source's own integral over the window is 2.5 rather than 3 —
so this layer over-emits by half a particle at the tail, recorded in §13.

### 6.2 VisTag / renderer needs — RESOLVED

Sixteen row renderers, **225–240**, plus the ribbon emitter's **241**; the ceiling stays derived
(`Get_RosterVisTag_Max()` → 241).

| VisTag | Kind | Look | Source emitters |
|---|---|---|---|
| 225 | CustomFacingSprite | `PartDisAdd01` | GroundGlow_01, GroundGlow_02 |
| 226 | CameraFacingSprite | `PartDisAdd01` | Glow_01, Glow_02 |
| 227 | CameraFacingSprite | `PartDisAdd01Bright` | Sparkles |
| 228 | CameraFacingSprite | `FlareDisAdd01` | Flare_01 |
| 229 | VelocityAlignedSprite | `PartDisAdd04` | Sparkles_Stretched |
| 230 | CameraFacingSprite, SubUV 2×2 | `LightningDisAdd02` | Lightning_01, Lightning_02 |
| 231 | Mesh `Spike` | `FlatAdd02` | Spikes |
| 232 | Mesh `Card` | `LightStripDisAdd` | LightningStrip |
| 233 | CameraFacingSprite | `ImpactDisAdd01` | FlareImpact |
| 234 | CameraFacingSprite | `RainbowDisAdd` | Raimbow |
| 235 | CameraFacingSprite | `RingDisAdd01` | Ring |
| 236 | CustomFacingSprite | `ExpGroundMarkDisAdd` | GroundCrack_01 |
| 237 | CameraFacingSprite | `StarDisAdd01` | Star02 |
| 238 | Mesh `UvSphere`, MeshScale 0.8 | `FlatAdd02` | Bubble_First_Explo |
| 239 | Mesh `Spike` | `FlatAdd02` | Spike01 |
| 240 | CameraFacingSprite, SubUV 2×2 | `FlamesDisAdd01` | Flames_01, Flames_02 |
| 241 | Ribbon (2nd emitter) | `FlatAdd02Ribbon` | LightningArc_01, LightningArc_02 |

**[P5-H7] correction:** §6.2's original table said the camera-facing row covered "6 distinct looks"
beside a list of SEVEN (`Part01`, `Part01_Bright`, `Flare01`, `Impact01`, `Rainbow`, `Ring01`,
`Star01`). Itemization right, count wrong.

Two structural notes the table carries:

- **The pyramid carrier is declared TWICE** (231 and 239) because the source draws it once on an
  emitter whose renderer faces VELOCITY and once on one that does not, and a renderer is the only
  place that distinction can live.
- **`PartDisAdd01` is declared twice too** (225 and 226) for the same reason on the sprite side: the
  two ground decals are custom-facing quads and the two glows are billboarded, and the same
  material instance draws both. That is the NS_ExplosionGround shape, second sighting.

### 6.3 Mesh / texture / look needs — ALL ALREADY CARRIED

- **Meshes: 0 new.** `SM_VFX_Spike01` is `SM_CkParticles_Spike`, `SM_VFX_Plane01` is
  `SM_CkParticles_Card` and `SM_VFX_Sphere01` is `SM_CkParticles_UvSphere` (baked in batch H off
  this exact asset — 559 verts / 960 tris / radius 100). §3's "3 new procedural generations" and its
  "check whether Shell is a usable sphere" note are both SUPERSEDED: the answer to the sphere was a
  name, and batch H already recorded that Shell is radius 50 with a different seam.
- **Textures: 0 new.** All thirteen paints resolve to existing measured bakes (§7).
- **CkUsf looks: 0 new.** §6.3's "12 new" is SUPERSEDED — every Family-A instance and both Family-B
  usages were already generated for earlier ports (§10).

### 6.4 Behavior id

**45**, the next free id at implementation time; `NumBehaviors` 45 → 46 in one bump. Layer partition
is `Seed % 84` over the burst slots in the §2 emitter order, minus the two arc emitters and plus
Lightning_02's three solved slots in its own table position.

**The aim axis is +Z** (the roster's ImpactBurst convention). The two arcs are the exception and keep
the source's +X, exactly as they do in NS_Lightning_Muzzle.

### 6.5 The original capability gaps, answered

| # | Gap as written | Answer |
|---|---|---|
| 1 | Ribbon renderers unsupported | **CLOSED by [P3-D1]** — second emitter + seed bank + `RibbonIdBinding`. This row's arc pair is the fifth consumer. |
| 2 | Mixed local/world space | **The C12 non-goal.** All 22 layers are implemented in LOCAL space and the seven world ones are recorded in §13.1. |
| 3 | One template, three cadences, eight `Self / Once` one-shots | **CLOSED.** [P2-E7]: a burst-only `Self / Once` emitter needs no window at all, and six of the eight are burst-only. The two that are not (the arcs, and Lightning_02) are handled by [P3-G8]'s CDF inversion. No `frac(Age/Cycle)` anywhere. |
| 4 | No row-level camera-facing sprite; VisTag 4 carries one look | **CLOSED by C1** — `CameraFacingSprite` and `CustomFacingSprite` are row kinds and each binds its own look master. |
| 5 | No sub-UV support, and this effect needs TWO modes | **CLOSED by C3/C4.** `SubImageSize` on the renderer spec, `SubImageIndex` on the stage output; Lightning runs LINEAR and Flames RANDOM off one row. |
| 6 | The gradient-map LUT chain, for `Rainbow` | **The standing [P1-D1] deferral** every Rainbow consumer takes — the chain exists and is held at the white default. §13.4. |
| 7 | Curl noise not closed-form, under an active speed limit | **CLOSED by C10, and the clamp was never active.** [P3-G4] established for these same two emitters that `Clamp Velocity = false` and `Mask Curl Noise = false`, so the path is a plain `CkParticles_CurlPath`. |
| 8 | Six inverted `Random Range` ranges | **RESOLVED by [P0-D7]'s reading**: Niagara's `RandomRangeFloat` lerps Min → Max regardless of order, so an inverted pair is a range traversed backwards and nothing else. All six are transcribed AS AUTHORED (§9.5). |
| 9 | Two disabled spawn shapes | Implemented as disabled: LightningStrip and Spike01 both spawn at the origin. |
| 10 | `Facing: Velocity` vs `Initial Mesh Orientation` | **RESOLVED per emitter** — see §9.4. |
| 11 | Family parameters not plumbed through the look | Unchanged; §13.5 lists them per instance. |
| 12 | `Star02` renders through the `Star01` material | Implemented as authored. Not fixed. |

### 6.6 Corrections applied to this sheet at implementation

All the ratified transcription / arithmetic / toggle class; each one is itemization-right and a
derived-or-omitted statement wrong.

- **[P5-H1]** §6.1's burst of **87** included the arcs' six burst particles. Main burst is **84**
  (81 + Lightning_02's three solved points) and the ribbon emitter carries thirty of its own. The
  [P3-G6] class, second sighting.
- **[P5-H2]** §6.1's `t_k = 0.5·(1 − sqrt(1 − k/2.5))` did not say where `k` starts. `k = i + 0.5`
  is [P3-G8]'s midpoint convention and is what shipped.
- **[P5-H3]** §5.6 Sparkles_Stretched's `Non Uniform Scale (1, 0.2, 0.2)` is INERT
  (`UseNonUniformScale = false`). Taking it at face value would have collapsed the streak cloud into
  a thin cigar. The [P2-E5] toggle class.
- **[P5-H4]** §5.17 Star02's `(1, 0.2, 0.2)` is INERT for the same reason.
- **[P5-H5]** §5.7 Lightning_01's `Sphere Location` is `Surface Only = true` — the only one in the
  system — so its bolts sit on a 100-unit shell rather than filling the ball. §5 omitted the flag.
- **[P5-H6]** §5.21/§5.22's `Surface Expansion Mode = Outside` is INERT (`Surface Only = false`), so
  the flame puffs fill the 20-unit volume.
- **[P5-H7]** §6.2's camera-facing row said six distinct looks beside a list of seven.
- **[P5-H8]** Documentary: Sparkles' `Speed Limit 1000` is inert (`Clamp Velocity = false`), the
  [P3-G4] class again. The sheet never claimed otherwise; recorded so a later reader does not
  "recover" it.

---

## 7. Textures — ZERO new bakes

Thirteen source paints, thirteen existing measured stand-ins. Each was checked against the §4 table's
`Main_Tex` / `Dissolve_Tex` assignment for the instance that uses it, not merely by name.

| Source paint | Existing bake | First measured for |
|---|---|---|
| `T_VFX_Part_01` | `SoftParticle` | NS_BasicAttack §7 |
| `T_VFX_Part_02` | `SoftParticleBright` | NS_FireBall_Hit §7 |
| `T_VFX_Part_04` | `SparkStreak` | NS_BasicAttack §7 |
| `T_VFX_Ring_01` | `RingUneven` | NS_FireBall_Hit §7 |
| `T_VFX_Ring_02` | `RingFlare` | NS_FireBall_Hit §7 |
| `T_VFX_Star_01` | `StarFour` | NS_FireBall_Hit §7 |
| `T_VFX_Star_04` | `ExpGroundScorch` | NS_ExplosionGround §7 — pixelwise correlation **0.9677**, the highest in the library, and the bake that proved `T_VFX_Star_04` is a lumpy blob rather than a star |
| `T_VFX_Impact_01` | `ImpactStar` | NS_FireBall_Hit §7 |
| `T_VFX_LightStrip_01` | `LightStrip` | NS_FireBall_Hit §7 |
| `T_VFX_Wind_01` | `WindSheet` (2×2 atlas) | NS_Fire §7 |
| `T_VFX_Lightning_03` | `LightningSheet` (2×2 atlas, independent frames) | NS_Lightning_Cast §7 |
| `T_VFX_Noise_02` | `TileNoise` | NS_BasicAttack §7 |
| `T_VFX_Noise_04` | `TileNoiseCoarse` | NS_FireBall_Hit §7 |
| `T_VFX_LUT_Rainbow_01` | `LutWhite` (held at the family default) | the [P1-D1] deferral |

The §6.3 note "`T_VFX_Noise_04` … measure rather than assume, its branch IS live on Flames01" is
answered by the shipped `FlamesDisAdd01`: that look already points its `DistortTex` at
`TileNoiseCoarse` (the Noise_04 stand-in) and its `DissolveTex` at the same, with
`DistortIntensity 0.5` and `DistortScale 2.0` live — i.e. the measurement the sheet asked for was
already done for NS_Fire and NS_FireBall_Hit, on the same material instance.

## 8. Meshes — reused, not rebuilt

| Source | Carrier | Provenance |
|---|---|---|
| `SM_VFX_Spike01` | `SM_CkParticles_Spike` | NS_FireBall_Hit §8; 16 verts / 6 tris, apex (0,0,200), base ±100 |
| `SM_VFX_Plane01` | `SM_CkParticles_Card` | NS_Arrow_Cast §8; ONE quad + a two-sided look rather than the source's hand-doubled pair, with the source's inverted `v` |
| `SM_VFX_Sphere01` | `SM_CkParticles_UvSphere` | NS_Bomb_Explosion §8; radius 100, seam at ±180°, v pole-to-pole. 32 × 16 quads against the source's 960 triangles — the difference is two rings of pole slivers |

§3's `[inferred]` "30 longitudinal × 16 latitudinal produces exactly 960" is superseded by batch H's
direct measurement of the same asset: the carrier is built at 32 × 16 with a 0.05 rad pole gap, and
the triangle-count gap is the pole closure.

## 9. The behavior — `Behavior_LightningHit.ush` + `ExecuteStage_CPU` case 45

### 9.1 The burst partition

`Seed % 84`, in §2's emitter order minus the arcs and plus Lightning_02's solved trio:

| Slots | Count | Emitter | VisTag |
|---|---|---|---|
| 0–1 | 2 | GroundGlow_01, GroundGlow_02 | 225 |
| 2 | 1 | Glow_01 | 226 |
| 3–22 | 20 | Sparkles | 227 |
| 23 | 1 | Flare_01 | 228 |
| 24–33 | 10 | Sparkles_Stretched | 229 |
| 34–36 | 3 | Lightning_01 | 230 |
| 37–41 | 5 | Spikes | 231 |
| 42–44 | 3 | LightningStrip | 232 |
| 45–47 | 3 | Lightning_02 (solved release times) | 230 |
| 48 | 1 | FlareImpact | 233 |
| 49 | 1 | Raimbow | 234 |
| 50 | 1 | Ring | 235 |
| 51 | 1 | GroundCrack_01 | 236 |
| 52–56 | 5 | Star02 | 237 |
| 57 | 1 | Glow_02 | 226 |
| 58 | 1 | Bubble_First_Explo | 238 |
| 59–63 | 5 | Spike01 | 239 |
| 64–73 | 10 | Flames_01 | 240 |
| 74–83 | 10 | Flames_02 | 240 |

The four beat-carrying groups (0.05 s on Raimbow / Ring / both Flames, 0.1 s on Flare_01 /
FlareImpact / GroundCrack_01) hide for `age < delay` and run their curves on `(age − delay) / life`,
the NS_BasicAttack §5 shape.

### 9.2 The two Lightning emitters are ONE branch

Lightning_01 and Lightning_02 share every curve, the spin integral, the sub-UV walk, the dissolve
ramp and the inverted 130 → 100 size range; they differ only in their spawn sphere (100 surface vs
30 volume) and in when they are released. The behavior expresses that as one helper taking
`(PointAge, SpawnRadius)`, which is also why they share VisTag 230.

### 9.3 The arcs — a direct transcription, not a re-derivation

`diff` of the two `LightningArc` emitter blocks against NS_Lightning_Muzzle's, with nothing
normalized, is **ZERO lines**. The arc branch is therefore Behavior_LightningMuzzle.ush's, verbatim:
the same 13 / 17 ribbon split, the same `0.3·(1 − sqrt(1 − (i + 0.5)/12))` release times, the same
curl frequencies (0.5 / 1.0) and the same measured field means (0.7400 / 0.7426) behind the
acceleration-to-velocity conversion. Re-deriving them would have been an opportunity to disagree
with a gated port for no reason.

### 9.4 `Facing: Velocity` vs `Initial Mesh Orientation` — resolved per emitter (§6.5 gap 10)

The source's mesh renderers face VELOCITY on Spikes and LightningStrip and DEFAULT on Spike01 and
Bubble_First_Explo. Niagara's `Velocity` facing mode aligns the mesh's local **+X** to the velocity,
and every CkParticles carrier is built along **+Z**, so mirroring the mode onto the renderer would
point the pyramids broadside to their own motion. Both are therefore reproduced through the
particle's own orientation with the renderer left at `Default`:

- **Spikes** writes `QuatFromZTo(velocity)` — point-first, the NS_Lightning_Muzzle precedent for the
  identical source emitter.
- **LightningStrip has no velocity at all** (its `Add Velocity` stack is empty and its `Velocity
  Scale` curve is therefore inert), so Niagara's velocity facing has nothing to align to and the
  emitter's own `Initial Mesh Orientation` is the whole answer. Its `Orientation Vector` is a
  `Random Range Vector` from (1, 1, 0) to (−1, −1, −1), and the behavior carries the carrier's +X
  onto that direction. **This also answers NS_Bomb_Explosion §13.6's open question** for the
  zero-velocity case: with no velocity, the authored orientation is the only thing that can win.
- **Spike01** carries mesh +X onto (1, 0, 0) — an identity — plus a `Random Range Vector` rotation of
  at most 1° in yaw and half a degree in pitch. Implemented as authored rather than dropped, because
  a near-identity is still a per-particle difference and dropping it would have been a silent choice.

### 9.5 The six inverted ranges, transcribed as authored

`[P0-D7]`'s reading, applied uniformly: `lerp(Min, Max, rand)` regardless of which is larger.

| Emitter | Pin | Authored | Implemented |
|---|---|---|---|
| Sparkles | Lifetime | Min 1.0 / Max 0.5 | `lerp(1.0, 0.5, r)` → [0.5, 1.0] |
| Lightning_01 / _02 | Uniform Sprite Size | Min 130 / Max 100 | `lerp(130, 100, r)` |
| Spikes | Velocity Strength | Min 200 / Max 50 | `lerp(200, 50, r)` |
| Spikes | Mesh Scale Z | 0.6 … 0.4 | `lerp(0.6, 0.4, r)` |
| LightningStrip | Orientation Vector | (1,1,0) … (−1,−1,−1) | component-wise `lerp` |
| Spike01 | Rotation | (0,0,1) … (0,0.5,−1) | component-wise `lerp` |

**The inversion is unobservable in the OUTPUT** — `lerp(a, b, r)` and `lerp(b, a, 1 − r)` have the
same distribution — so what the reading actually fixes is which SEED gets which draw. It is recorded
because §6.5 gap 8 asked, and because a reader who "normalized" the pins would produce a different
per-particle assignment for no stated reason.

### 9.6 Sub-UV in both of Niagara's modes, on one row

- **Lightning (LINEAR, Start 0, End 4, loop 1):** `fmod(min(floor(t·5), 4), 4)` — every particle
  starts on frame 0, and the declared End Frame of 4 on a four-frame sheet wraps its fifth step back
  onto 0. Byte-identical to NS_Lightning_Muzzle's.
- **Flames (RANDOM, Start 0, End 3, loop 1):** a random start frame stepping forward,
  `fmod(start + min(floor(t·4), 3), 4)` — the NS_Fire idiom.

The test asserts them against EACH OTHER: the bolts' start-frame set must be exactly {0} and the
flames' must have three or more members. A single shared implementation fails one of the two.

## 10. Looks and renderers — sixteen reuses, zero new

Every material instance was checked value-by-value against §4's delta table before reuse. Because a
`M_VFX_DisAdd_*` instance is ONE asset in the pack, a look already generated for another port carries
identical parameters by construction — but the check still matters, because the LOOK NAME does not
have to correspond to the instance a reader assumes.

| §4 instance | Look | Verified |
|---|---|---|
| `Part01` | `PartDisAdd01` | B 1, boldness 0.5, Gradient_Invert 0.5 |
| `Part01_Bright` | `PartDisAdd01Bright` | B 10, boldness 1 |
| `Part04` | `PartDisAdd04` | B 6 |
| `Flare01` | `FlareDisAdd01` | B 2, Gradient_Invert 0.847619, shape `RingFlare` / dissolve `SoftParticle` |
| `Impact01` | `ImpactDisAdd01` | B 12 |
| `Lightning02` | `LightningDisAdd02` | B 15, DistortIntensity 0.5, DistortSpeed 0.7/0.7, DistortScale 1, DistortTex `TileNoiseCoarse` |
| `LightStrip` | `LightStripDisAdd` | B 7, MESH usage |
| `Rainbow` | `RainbowDisAdd` | B 1, boldness 1.5, GradientMapDisplacement 0.9, Gradient_Invert 2 |
| `Ring01` | `RingDisAdd01` | B 10 |
| `Star01` | `StarDisAdd01` | B 6 |
| `Star04` | `ExpGroundMarkDisAdd` | B 1, dissolve `SoftParticle` (the source's `T_VFX_Part_01`), DistortTex `TileNoise` — the instance matches exactly, and the look's NAME says "ExpGroundMark" because NS_ExplosionGround got to it first |
| `Flames01` | `FlamesDisAdd01` | B 20 (10 × Glow_Intensity 2), DissolveBias −0.1, DissolveScale 2/2, DistortIntensity 0.5, DistortSpeed −0.3, DistortScale 2 |
| `Flat02` (mesh) | `FlatAdd02` | B 10, mesh-particle usage |
| `Flat02` (ribbon) | `FlatAdd02Ribbon` | B 10, ribbon usage |

Two verified-inert differences, both pre-existing and neither touched:

- `PartDisAdd01` and `PartDisAdd04` ship `DistortScale 0.1` where §4's family reference reads 1.0.
  Both instances have `Distortion_Intensity 0`, so the whole distortion branch is dead — the adjacent
  finding batch B already logged.
- `PartDisAdd04` ships `Gradient_Invert 0.5` where §4's Ring04 reference implies 0. Its
  `GradientMap_Tex` is the white default, under which the gradient chain is provably a no-op (the
  Phase-1 exact-0.0 inertness proof).

## 11. Tests

`Test_Particles_LightningHitBehavior.cpp` (`CkTests.UnitTests.CkParticles.LightningHitBehavior`)
drives `Execute_Stage_CPU` directly — no Niagara system, no RHI, no forked engine. It carries every
standing standard plus this port's own claims:

- **the cadence row and the ribbon spec**, including the sixteen-renderer count, the two 2×2 sheets,
  the two custom-facing quads, the twice-declared pyramid carrier and the bubble's 0.8 renderer
  scale;
- **the layer partition, bucketed WHILE ALIVE** across a 1.4 s / 400-step sweep whose step is finer
  than the shortest window in the system (FlareImpact's 0.05 s). Never-drawn 0, inconsistent 0;
- **the VisTag band on drawing samples only**, with hidden samples asserted fully inert, and the
  derived roster ceiling asserted to be this port's ribbon renderer;
- **Lightning_02's three solved release times**, each to 6e-4 on a 2.5e-4 sweep;
- **the inverted lifetime range**, pinned at BOTH ends over 40 seeds (0.5125 … 0.9795 measured
  against the [0.5, 1.0] span) — a one-sided assertion could not tell the reading from its inverse;
- **the sparkle strobe**: both flat-zero plateaus at zero, the 0.504679 return to one, and a turning-
  point count that separates a strobe from a fade;
- **both sub-UV modes against each other** (see §9.6);
- **every beat**, hidden before and drawing after, on all six delayed layers;
- **corpus-derived colour values at BOTH ramp ends** on the ground-glow curve, the crack's
  white → blue → black collapse (including its 0.87025 alpha at the collapse key), Ring's held 0.608,
  FlareImpact's held-then-falling alpha, and the shared lightning ramp's three opening keys;
- **the ribbon-bank partition both ways** — 13 / 17 by ribbon id, one tag on the ribbon bank, and no
  main-bank particle reaching the arc renderer over 5040 seeds;
- **the arc spawn inversion** — nine of twelve in the first half of the window, both ends pinned;
- **emitter-clock independence** over 400 seeds × 3 clocks.

**§14.7's recovered-parameter colour key does not apply here.** Every colour mode in this system is
`Direct Set` (`Color Mode = NewEnumerator1` on all 22 emitters) — there is no `Random Hue/Saturation/
Value` layer and no `Random Range` colour, so there is no per-particle colour variation to bucket.
The discipline transferred instead as the two-sided lifetime assertion above, which is the same idea
applied to the one parameter this system DOES randomize in a way a naive assertion could not catch.

`Test_Particles_RosterSanity.cpp` bumps to `NumBehaviors == 46`; every other assertion in it is
roster-driven and picks the new row up without an edit.

## 12. Verification — A/B protocol `[HUMAN-VERIFY]`

VfxExamples gym, station pair **LIGHTNING HIT** (`Gym.VfxExamples.LightningHit.Ck` /
`.Original`), spawn offset (0, 0, 20). `Ck_GymVfxExamples_RestartAll` re-fires both sides in sync.
Judge in this order:

a. **The ground read.** Two flat decals (2600 and 4000 units) plus the 600-unit crack should lie flat
   and read as one bright wash under the impact, not as billboards turning to face you. Orbit the
   station: a custom-facing quad stays flat, a billboarded one does not.
b. **The crack's colour snap.** White → blue → black inside the first 13 % of a full second, then a
   long slow alpha fade. If it reads as a steady blue for a second, the three-key curve collapsed.
c. **The sparkle strobe.** Twenty motes blinking OUT twice mid-flight. A steady fade is the failure.
d. **The bolts.** Six flipbook cards total — three at once, then a trickle of three more across the
   first half-second. If all six land together, the CDF inversion is not running.
e. **The arcs.** Two lightning ribbons firing along +X, one thick (7–12) and one thin (2–4), both
   visibly bent rather than straight. Compare their swirl against the original's; NS_Lightning_Muzzle
   §13 already records that 16 Euler steps under-resolve it, and this is the same field.
f. **The upward spray.** Spikes point-first along their own motion, Star02's five stars carrying the
   longest tail in the effect (up to 1.3 s), and the two flame layers — one spreading, one a straight
   vertical jet.
g. **The one-frame flashes.** FlareImpact at 0.1 s for 0.05 s, and Raimbow at 0.05 for 0.1. Both are
   nearly subliminal; check they exist rather than judging their shape.
h. **What NOT to judge:** the seven world-space layers do not separate at a stationary pedestal
   (§13.1), and the Raimbow layer's rainbow LUT is held at white (§13.4) — so that layer will read as
   a plain grey ring on both sides only if the original's LUT is also inert, which it is NOT. Expect
   a colour difference there and do not chase it.

## 13. Confirmed fidelity differences or intentional deviations

1. **Mixed coordinate space — the C12 non-goal.** Seven of the 22 source emitters are world-space
   (Raimbow, Ring, Star02, Bubble_First_Explo, Spike01, Flames_01, Flames_02) and fifteen are local.
   A CkParticles template's `LocalSpace` is an emitter property, so one template cannot be both. All
   twenty-two layers are implemented in LOCAL space. **At a stationary pedestal the two are
   identical** — a world-space emitter only separates from a local one when the system MOVES — which
   is why the campaign's C12 non-goal covers it. This is the only system in the pack that mixes, and
   it is recorded here rather than worked around ([P3-F5]-style honesty).
2. **Lightning_02 over-emits by half a particle at the tail.** Its source rate integrates to 2.5
   particles over the window; three burst slots is the nearest integer capacity, and the third lands
   at the window edge where the source's rate has already fallen to zero. The alternative — two slots
   — would under-emit by the same half. Chosen for the [P3-G8] midpoint convention's consistency.
3. **The [P4-D2] light clause does not apply.** `NS_Lightning_Hit` has NO light renderer at all
   `[corpus]` — the sheet's §6.5 "not gaps" list already says so — so nothing was dropped for it.
   Recorded because the four explosion ports of batch H all carry the clause and a reader comparing
   the family would otherwise look for it here.
4. **The Raimbow layer's gradient LUT is held at white** — the standing [P1-D1] deferral. This is the
   only instance in the system with a live gradient-map chain (`GradientMap_Tex
   T_VFX_LUT_Rainbow_01`, `GradientMap_Displacement 0.9`, `Gradient_Invert 2`), so the layer draws as
   a monochrome ring rather than a spectral one. It is a REAL visible difference, not an inert one,
   and it resolves when the maintainer answers [P1-D1].
5. **Family parameters the DissolveAdd look does not plumb** (§6.5 gap 11): `Core_Intensity`
   (0 on ten of twelve instances), `Core_Power` (0 on Impact01, Lightning02, LightStrip),
   `Opacty_StepAdd` (0.3 on Rainbow), a non-white `Color_Core` (Flames01's
   RGBA(0.015996, 0.014444, 0.014444, 1)), and `Opacty_DepthFade` (a pre-existing CkUsf gap — 0 on
   Star04, 20/30 elsewhere; CkUsf wires no scene depth). `Glow_Intensity` IS reproduced, folded into
   Brightness.
6. **The arc swirl is under-resolved at 16 Euler steps** — inherited verbatim from
   NS_Lightning_Muzzle §13, since the emitters are byte-identical. The recreation's arcs read as a
   coherent random walk of the right magnitude where the source's is a tighter swirl. Raising the
   step count is a fidelity constant that moves every curl-driven effect and is not this port's to
   change.
7. **The card carrier is one quad plus a two-sided look** where the source hand-doubles it into four
   triangles, and **the sphere carrier closes its poles with slivers** (1024 triangles against the
   source's 960). Both inherited from the carriers' own recipes.
8. **Spike01's per-particle rotation is at most one degree.** Implemented as authored; it is very
   close to no rotation at all, and a reader should not expect to see it.
9. **`Sparkles_Stretched`'s length is capped by its speed factor at 2×.** The source's
   `Scale Sprite Size by Speed` uses `Velocity Threshold 1000` with `Max Scale Factor (1, 2)`, which
   is implemented exactly — but the layer's launch speeds run to 2000 u/s, so most streaks sit at the
   cap for the first fifth of their life.

## 14. Reusable lessons

1. **A burst count that includes a ribbon emitter's particles is the campaign's most repeated
   arithmetic error.** [P3-G6] caught it on NS_Lightning_Muzzle; [P5-H1] caught the identical error
   here, on the identical two emitters. Whenever a sheet's §2 burst total is summed over a table that
   contains ribbon emitters, subtract them before writing the row.
2. **A stored spawn-shape pin is worth nothing until its toggle is read.** Three of this port's six
   corrections are toggles: `UseNonUniformScale` (twice), `Surface Only` (twice, in opposite
   directions). The pin is always exported; the toggle decides whether it means anything. Same class
   as [P2-E5], [P3-G4] and [P3-G5], and it has now appeared in five consecutive batches.
3. **A renderer facing mode is not portable across mesh axes.** Niagara's `Velocity` facing aligns
   local +X; every carrier in this library is built along +Z. Mirroring the source's renderer flag
   would have been a faithful-looking change that pointed every pyramid sideways. Reproduce velocity
   facing through the particle's own orientation instead, and record it (§9.4).
4. **When two ports share an emitter, diff it and transcribe rather than re-derive.** The arc pair
   diffs to zero lines against NS_Lightning_Muzzle's. Re-deriving its curl conversion would have been
   an invitation to land on a different constant than a gated port, for no gain.
5. **An inverted range is invisible in the output distribution and visible in the seed assignment.**
   State that explicitly when recording one, or a later reader will "fix" it and quietly change which
   particle is which ([P0-D7], §9.5).
6. **A port that adds no assets still owes the value-by-value check.** Twelve of this port's fourteen
   look reuses came from ports with unrelated names (`ExpGroundMarkDisAdd` is this system's
   `Star04`). Matching by instance, not by name, is what makes zero-new-assets a finding rather than
   an assumption.
7. **The last port in a campaign is the integration test.** Everything this effect needed already
   existed: C1's sprite kinds, C3/C4's atlases and `SubImageIndex`, C8's renderer mesh scale,
   C10's curl field, [P3-D1]'s second emitter and seed bank, [P3-G8]'s CDF inversion, [P2-E7]'s
   burst-only-window rule, [P0-D2]/[P0-D7]'s lifetime rules and [P0-D5]'s cadence formula. Nothing
   new was added, and the phase charter's "a new capability requirement is a STOP" was never reached.
