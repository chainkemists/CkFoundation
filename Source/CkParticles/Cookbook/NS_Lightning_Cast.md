# Recipe: NS_Lightning_Cast → CkParticles (PLANNED)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: IMPLEMENTED (2026-08-02) — `LightningCast`, BehaviorId 35. `[HUMAN-VERIFY]` open.**

`Behavior_LightningCast.ush` + `ExecuteStage_CPU` case 35, cadence row
`PS_CkParticles_Template_LightningCast` (2.0 s / 1.55 s / burst 30 **+ rate 40/s**), ten row renderers on
VisTags 147–156, **one** new CkUsf look (`LightningDisAdd02`) and **one** new texture bake
(`T_CkParticles_LightningSheet`). Gym pair staged in VfxExamples. §12's human A/B walk has NOT been
performed — it belongs to the campaign's inspection stage.

**This port carries the cookbook's first NON-CONSTANT source spawn rate**, and the way a flat row rate is
turned back into a falling one is the reusable result — see §9.2 and §14.1. §6.5's gaps 1 (camera-facing
kinds), 2 (one template = one cadence) and 3 (sub-UV) are all closed; gap 5 (the Rainbow LUT) stays open
under [P1-D1].

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Lightning_Cast` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior ID | **not allocated** — take the next free id at implementation time from `ck::particles::NumBehaviors` |
| CkUsf looks | none yet |

Corpus evidence (regenerate per [README.md](README.md); `Saved/` is machine-local):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Lightning_Cast.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part01_Bright,Part02,Part03_Bright,Part04,Rainbow,Ring01,Star02,Star03,Lightning02,Arrows}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_Ring04.json` (family reference for the diff)
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Part_03,Part_04,Ring_01,Ring_02,Star_02,Star_03,Lightning_03,Noise_02,LUT_Rainbow_01,WhitePixel}.json`

**The source Niagara asset was never opened in the Niagara editor.** Every fact below is `[corpus]`
unless tagged otherwise.

> ### TWO SYSTEMS SHARE THIS NAME — take the right one
> `[corpus]` The pack ships a second `NS_Lightning_Cast` at
> `Vefects/Anime_Stylized_VFX/VFX/Particles/`. It is a different, **parameterized** system:
> 19 emitters, and a **19-entry `userParameters` list** (`User.Glow Color 01..03`,
> `User.Flare Color 01..04`, `User.Flare Stretched Color 01..04`, `User.Rainbow Color`,
> `User.Ring Color 01`, `User.Star Color 01`, `User.Star Big Color 01`, three colour-curve data
> interfaces, and `User.Scale Overall`). It renders through `MI_VFX_*` instances
> (`MI_VFX_Glow_01`, `MI_VFX_Lightning_02`, `MI_VFX_Lens_Rainbow_01`, …).
>
> **Fastest one-line discriminator:** `userParameters` is **empty** on the system this recipe
> documents and **19 entries long** on the sibling. Second check: `M_VFX_DisAdd_*` materials (this
> one) vs `MI_VFX_*` materials (sibling).
>
> This recipe recreates the **`Anime_VFX/Shared/Skills`** one.

---

## 2. System anatomy `[corpus]`

**21 CPU emitters — 19 enabled, 2 disabled. All `LocalSpace: false` (WORLD space),
`Determinism: false`, `Bounds: Dynamic`.** `userParameters` is **empty**.

Renderers: **18 camera-facing sprites** (`Unaligned` / `FaceCamera` / `Sort: None`),
**1 velocity-aligned sprite** (Sparkles_Stretched), **1 camera sprite with `SubUV: 2x2`**
(Lightning), and 2 velocity-aligned sprites on the DISABLED Arrow emitters.
No mesh renderers, no ribbons, no light renderers, no events, no GPU sims.

**≈ 30 burst particles per loop plus ≈ 13 rate-spawned** (see the arithmetic under the table).

| # | Emitter | Enabled | Spawn | t (s) | Lifetime (s) | Sprite size (effective) | Dyn 1 | Renderer | Material |
|---|---|---|---|---|---|---|---|---|---|
| 1 | Glow_01 | yes | burst 1 | 0 | 1 | Uniform **550** | 1 | camera sprite | `Part01` |
| 2 | Glow_02 | yes | burst 1 | 0 | 1 | Uniform **200** | **0** | camera sprite | `Part02` |
| 3 | Glow_03 | yes | burst 1 | 0 | 1 | Uniform **250** | **2** | camera sprite | `Part01` |
| 4 | Raimbow *(sic)* | yes | burst 1 | 0.1 | 0.3 | Uniform **350** | **0.5** | camera sprite | `Rainbow` |
| 5 | **Arrow** | **DISABLED** | burst 1 | 0 | 1.5 | Non-Uniform (170, 170) | 0 | velocity-aligned | `Arrows` |
| 6 | **BigArrow** | **DISABLED** | burst 1 | 0 | 1.5 | Non-Uniform (150, 240) | 0 | velocity-aligned | `Arrows` |
| 7 | Ring | yes | burst 1 | 0.05 | 0.75 | Uniform **150** | curve | camera sprite | `Ring01` |
| 8 | Sparkles | yes | burst **10** | 0.05 | rand — §5 | Random Uniform **10 … 20** | 1 | camera sprite | `Part01_Bright` |
| 9 | Flare_01 | yes | burst 1 | 0.1 | 0.5 | Uniform **50** | 1 | camera sprite | `Part02` |
| 10 | Flare_02 | yes | burst 1 | 0.1 | 0.5 | Uniform **50** | 1 | camera sprite | `Part02` |
| 11 | Sparkles_Stretched | yes | **rate 20/s**, loop **0.4 s, Once** | — | rand — §5 | Random Non-Uniform (25, 70) … (40, 60) | **0** | **velocity-aligned** | `Part04` |
| 12 | Big_Star | yes | burst 1, loop **0.3 s, Once** | 0 | 0.3 | Uniform **200** | **0** | camera sprite | `Star02` |
| 13 | Flare_Stretched_01 | yes | burst 1 | 0 | 1.2 | Non-Uniform **(700, 100)** | 1 | camera sprite | `Part01` |
| 14 | Flare_Stretched_02 | yes | burst 1 | 0 | 1.2 | Non-Uniform **(700, 100)** | 1 | camera sprite | `Part03_Bright` |
| 15 | Flare_Stretched_03 | yes | burst 1 | 0 | 1.2 | Non-Uniform **(1400, 180)** | 1 | camera sprite | `Part03_Bright` |
| 16 | Flare_Stretched_04 | yes | burst 1 | 0 | 1.2 | Non-Uniform **(400, 70)** | 1 | camera sprite | `Star03` |
| 17 | Star_01 | yes | burst 1 | **0.85** | **0.1** | Uniform **70** | **0** | camera sprite | `Star02` |
| 18 | Star_02 | yes | burst 1 | **0.95** | **0.1** | Uniform **70** | **0** | camera sprite | `Star02` |
| 19 | Lightning | yes | burst **3** @0 **+ rate curve**, loop **0.5 s, Once** | 0 | rand — §5 | Random Uniform **30 … 100** | curve | camera sprite, **SubUV 2×2** | `Lightning02` |
| 20 | Flare_03 | yes | burst **2** | 0.1 | 0.4 | Uniform **250** | 1 | camera sprite | `Part03_Bright` |
| 21 | Flare_04 | yes | burst 1 | 0 | 0.3 | Uniform **80** | **2** | camera sprite | `Part01_Bright` |

Dynamic material parameters 2, 3 and 4 are **0 on every emitter**; only `Write Parameter Index 0` is
true anywhere.

**Per-loop count arithmetic** `[inferred, from the table]`: burst = 1+1+1+1+1+10+1+1+1+1+1+1+1+1+1+3+2+1 = **30**;
rate-spawned = Sparkles_Stretched 20/s × 0.4 s = **8**, Lightning ∫(20 → 0 linear over 0.5 s) = **5**.
Total ≈ **43** particles per system cycle, of which 30 are deterministic burst slots.

**Cadence — this system has THREE distinct loop cadences and two life-cycle modes.** `[corpus]`

| Life Cycle Mode | Emitters | Loop Behavior | Loop Duration |
|---|---|---|---|
| **System** (own loop rows are inert) | 18 of 21 | stored `Infinite` (14) / stored `Once` (4: Star_01, Star_02, and both disabled Arrows are `Infinite`; Star_01/Star_02 store `Once`) | stored 1.0 (most) or 0.3 (Star_01, Star_02) |
| **Self** (own loop rows apply) | **Sparkles_Stretched, Big_Star, Lightning** | **Once** | **0.4 / 0.3 / 0.5** |

The three `Self` emitters are genuine one-shots: they run once for 0.4 / 0.3 / 0.5 s and do not
repeat, inside a system whose other 18 emitters cycle. **This is the fact that decides §6.1** —
see §6.5, gap 2.

**System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
Per [P0-D1] this rules the 18 `Life Cycle Mode = System` emitters; the three `Self` emitters keep
their own rows. *(Was `[unresolved]` with 1.0 s as the working figure.)* The Star_01 burst at
t = 0.85 and Star_02 at t = 0.95 fire comfortably inside a 2.0 s cycle.

> **Star_01 / Star_02 are NOT dead emitters, even though their stored `Loop Duration` (0.3) is
> shorter than their stored `Spawn Time` (0.85 / 0.95).** Both run `Life Cycle Mode = System`, so
> their own loop rows are inert and the system's cycle governs. Reading the rows at face value would
> wrongly delete two visible layers.

---

## 3. Mesh geometry

**N/A — no mesh renderers.** All 21 renderers are `NiagaraSpriteRendererProperties`.

---

## 4. Material family + delta table `[corpus]`

All eleven materials (including the two on the disabled Arrow emitters) are instances of ONE parent,
`/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd` — the family CkUsf already
implements as `CkUsf_Look_DissolveAdd` in `/CkUsf/Looks/DissolveAdd.ush`.

Every one: `MD_Surface`, `BLEND_Translucent`, `MSM_Unlit`, `twoSided: false`, outputs
`EmissiveColor` + `Opacity` only, dynamic-parameter channels **`dissolve`, `distortion`, `offset`,
`core_color`**, expression histogram identical to the family reference.

Deltas versus `M_VFX_DisAdd_Ring04` (reference: `Brightness 30`, `Color_CoreDifferent 1`,
`Core_Intensity 1`, `Core_Power 1`, `Glow_Intensity 1`, `Opacity_Boldness 1`, `Opacty_DepthFade 10`,
`Opacty_StepAdd 0.1`, `Gradient_Invert 0`, `GradientMap_Displacement 0.1`, `Dissolve_Speed_X/Y 0.2`,
`Distortion_Scale_X/Y 0.1`, `Distortion_Intensity 0`, `Dissolve 0`, all `*_Speed`/`*_Offset` 0,
`Color_Core RGBA(1,1,1,0)`, `GradientMap_Tex T_VFX_WhitePixel`, `GradientShape_Tex T_VFX_Noise_02`):

| Material | Main_Tex / Color_Tex | Dissolve_Tex | Distortion_Tex | Brightness | Other deltas |
|---|---|---|---|---|---|
| `Part01` | `T_VFX_Part_01` | `T_VFX_Part_01` | `T_VFX_Noise_02` | **1** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Gradient_Invert 0.5`; `Opacity_Boldness 0.5`; `Opacty_DepthFade 20` |
| `Part01_Bright` | `T_VFX_Part_02` | `T_VFX_Part_02` | `T_VFX_Noise_02` | **10** | `Color_CoreDifferent 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Gradient_Invert 0.5`; `Opacty_DepthFade 20` |
| `Part02` | `T_VFX_Part_02` | `T_VFX_Part_02` | `T_VFX_Noise_02` | **1** | as `Part01_Bright` plus `Core_Intensity 0`, **`Glow_Intensity 0.3`**, `Opacity_Boldness 0.5` |
| `Part03_Bright` | `T_VFX_Part_03` | `T_VFX_Part_03` | `T_VFX_Noise_02` | **10** | `Color_CoreDifferent 0`; **`CamOffset 50`**; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Gradient_Invert 0.5`; `Opacty_DepthFade 20` |
| `Part04` | `T_VFX_Part_04` | `T_VFX_Part_04` | `T_VFX_Noise_02` | **6** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; **`Opacty_DepthFade 30`** |
| `Rainbow` | **`T_VFX_Ring_02`** (Color_Tex `T_VFX_Part_01`) | `T_VFX_Part_01` | `T_VFX_Noise_02` | **1** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; **`GradientMap_Tex T_VFX_LUT_Rainbow_01`**; **`GradientMap_Displacement 0.9`**; **`GradientShape_Tex T_VFX_Part_01`**; **`Gradient_Invert 2`**; **`Opacity_Boldness 1.5`**; **`Opacty_StepAdd 0.3`**; `Opacty_DepthFade 20` |
| `Ring01` | `T_VFX_Ring_01` | `T_VFX_Ring_01` | `T_VFX_Noise_02` | **10** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Opacty_DepthFade 20` |
| `Star02` | `T_VFX_Star_02` | `T_VFX_Star_02` | `T_VFX_Noise_02` | **6** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1` |
| `Star03` | `T_VFX_Star_03` | `T_VFX_Star_03` | `T_VFX_Noise_02` | **10** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1` |
| `Lightning02` | `T_VFX_Lightning_03` | `T_VFX_Lightning_03` | *(reference `T_VFX_Noise_04`)* | **15** | `Color_CoreDifferent 0`; `Core_Intensity 0`; **`Core_Power 0`**; `Dissolve_Speed_X/Y 0`; **`Distortion_Intensity 0.5`**; `Distortion_Scale_X/Y 1`; **`Distortion_Speed_X/Y 0.7`**; `Opacty_DepthFade 20` |
| `Arrows` *(disabled emitters only)* | `T_VFX_Arrow_01` | `T_VFX_Arrow_01` | `T_VFX_Noise_02` | **10** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Gradient_Invert 0.5`; `Opacty_DepthFade 20` |

Two instances carry live branches the rest do not:

1. **`Lightning02` is the only instance with live distortion** (`Distortion_Intensity 0.5`,
   `Distortion_Speed 0.7/0.7`) — a fast-panning UV warp on the lightning bolts. It is also the only
   one with `Core_Power 0`.
2. **`Rainbow` is the only instance with a live gradient-map (LUT) chain.** Everywhere else in the
   family the `GradientMap_Tex` is `T_VFX_WhitePixel` and both shipped recipes dropped the chain as a
   provable no-op. That justification does not hold here. See §6.5, gap 5.

Referenced textures `[corpus]` (all 512×512, `sRGB: false`, `TC_Alpha`, `TEXTUREGROUP_World`
greyscale masks unless noted):

| Texture | Format | Address | Role |
|---|---|---|---|
| `T_VFX_Part_01` | `TSF_G8` | `TA_Clamp`/`TA_Clamp` | Part01; Rainbow dissolve + gradient shape |
| `T_VFX_Part_02` | `TSF_G8` | `TA_Clamp`/`TA_Clamp` | Part01_Bright, Part02 |
| `T_VFX_Part_03` | `TSF_G8` | `TA_Wrap`/`TA_Wrap` | Part03_Bright |
| `T_VFX_Part_04` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Part04 (the velocity-aligned streak) |
| `T_VFX_Ring_01` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Ring01 |
| `T_VFX_Ring_02` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Rainbow main |
| `T_VFX_Star_02` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Star02 |
| `T_VFX_Star_03` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Star03 |
| **`T_VFX_Lightning_03`** | **`TSF_BGRA8`** — a **colour** texture, not a mask | `TA_Wrap`/`TA_Wrap` | Lightning02 main + dissolve; **this is the 2×2 sub-UV flipbook sheet** |
| `T_VFX_Noise_02` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Distortion_Tex on ten of eleven (**dead branch on all but Lightning02**) + GradientShape_Tex on ten |
| `T_VFX_LUT_Rainbow_01` | `TSF_BGRA8`, **512×2**, `sRGB: true`, `TC_Default` | `TA_Wrap`/`TA_Wrap` | Rainbow's gradient LUT |
| `T_VFX_WhitePixel` | `TSF_RGBA16`, 1×1, `sRGB: true`, `TC_Default` | `TA_Wrap` | no-op GradientMap on ten |

---

## 5. Per-layer runtime curves `[corpus]`

`t` = NormalizedAge (0 → 1 over that emitter's own lifetime). `C` = constant key, `L` = linear key —
transcribed verbatim.

**Ten emitters share one Scale Color + one Scale Sprite Size shape.** Where an emitter is listed as
using "the shared fade", it means exactly:

- Scale Color · `Scale RGBA = R (0,1)L (1,1)L | G (0,1)L (1,1)L | B (0,1)L (1,1)L | A (0,1)L (1,0)L`
  (RGB untouched; **alpha ramps linearly 1 → 0 over the whole life**), `Scale Alpha 1`,
  `Scale RGB (1,1,1)`
- Scale Sprite Size · Uniform Curve `(0, 0.5)C (0.1, 1)L (1, 1)L`

Those layers have **no `Color` module at all** — they keep their Initialize Particle colour and fade.

### 1 · Glow_01 — burst 1 @ t=0, life 1 s, size 550

- Initialize color `RGBA(0.0648033, 0.0307135, 1, 1)` — deep blue
- Shared fade · dyn params `[1, 0, 0, 0]`

### 2 · Glow_02 — burst 1 @ t=0, life 1 s, size 200

- Initialize color `RGBA(0.863157, 0.0262412, 1, 1)` — magenta
- Shared fade · dyn params `[**0**, 0, 0, 0]`

### 3 · Glow_03 — burst 1 @ t=0, life 1 s, size 250

- Initialize color `RGBA(1, 0.819765, 0.499, 1)` — warm white
- A `Color` module is present with no override (stored `Scale Alpha 1`, `Scale Color (1,1,1)`);
  the Scale Color module below is what animates it
- Shared fade · dyn params `[**2**, 0, 0, 0]` — the highest static dissolve in the system

### 4 · Raimbow — burst 1 @ t=**0.1**, life 0.3 s, size 350

- Initialize color `RGBA(0.913099, 0.913099, 0.913099, **0.1**)`
- Sprite rotation: `Sprite Rotation Mode = Random`, angle **0 … 360**
- Scale Color: `Scale RGBA = R (0.328403, 0.5)L | G (0.328403, 0.5)L | B (0.328403, 0.5)L |`
  `A (0, 0)L (0.328403, 1)L (1, 0)L` — RGB halved for the whole life (single key), alpha rises to 1
  at t = 0.328403 then falls
- Scale Sprite Size, Uniform Curve: `(0, 0.5)C (0.2, 0.9)C (1, 1)L`
- Dyn params `[**0.5**, 0, 0, 0]`

### 5 · Arrow — **DISABLED** (recorded so its absence is a decision, not an oversight)

- burst 1 @ t=0, life 1.5 s, Sprite Size Mode Non-Uniform **(170, 170)**,
  `Position Offset (0, 0, **−119.316**)`, `Add Velocity (0, 0, **550**)`, velocity-aligned,
  material `Arrows`
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, 0.3)C (1, 0.05)C`
- Color from Curve:
  R `(0, 1)C (0.0784787, 1)L (0.283731, 1)L (0.625415, 0.672443)L (0.947781, 0.223228)C` ·
  G `(0, 0.913099)C (0.0784787, 0.501026)L (0.283731, 0.0773835)L (0.625415, 0.021219)L (0.947781, 0)C` ·
  B `(0, 0.584079)C (0.0784787, 0.0559999)L (0.283731, 0.025)L (0.625415, 0.0168074)L (0.947781, 0.116971)C` ·
  A `(0.0869303, 1)C (1, 0)C`
- Two stacked size curves: Uniform `(0, 0.4)C (0.1, 1)C (1, 0.4)C`; Non-Uniform
  `X (0, 0.2)C (0.3, 0.7)C | Y (0.2, 1)L (1, 1.2)L`

### 6 · BigArrow — **DISABLED**

- burst 1 @ t=0, life 1.5 s, Sprite Size Non-Uniform **(150, 240)**,
  `Position Offset (0, 0, **−52.2087**)`, `Add Velocity (0, 0, **150**)`, velocity-aligned, `Arrows`
- Velocity Scale: `X/Y/Z (0, 1)C (0.3, 0.05)C`
- Color from Curve: same key times as Arrow, values
  R `(0,1)C (0.0784787,1)L (0.283731,1)L (0.625415,0.672443)L (0.947781,0.223228)C` ·
  G `(0,0.913099)C (0.0784787,0.489926)L (0.283731,0.0925239)L (0.625415,0.021219)L (0.947781,0)C` ·
  B `(0,0.584079)C (0.0784787,0.035)L (0.283731,0.0409999)L (0.625415,0.0168074)L (0.947781,0.116971)C` ·
  A `(0.0869303, 1)C (1, 0)C`

### 7 · Ring — burst 1 @ t=**0.05**, life 0.75 s, size 150

- Initialize color `RGBA(0.913099, 0.191202, 1, 0.608)`
- Sprite rotation: `Random`, 0 … 360
- Dyn param 1 (`dissolve`) — Float from Curve: `(0, 0)C (1, -1)C`; params 2/3/4 = 0
- Scale Sprite Size, Uniform Curve: `(0, 0.5)C (0.1, 0.9)C (1, 1)C`
- **No colour animation at all** — Particle Update is
  `Particle State → Dynamic Material Parameters → Scale Sprite Size`. The ring holds its initialize
  colour and alpha 0.608 for its whole life and disappears purely by dissolve.
- Inert pins present in the export: `Lifetime Min 0.3 / Max 0.7`,
  `Uniform Sprite Size Min 150 / Max 160`

### 8 · Sparkles — burst **10** @ t=**0.05**, size Random Uniform 10 … 20

- Lifetime `[corpus-v3]`: **`Lifetime Mode = Random` ⇒ `Lifetime Min 0.5 / Max 1.5` DRIVES**
  (`lifetimeResolved.source = minmax`); the `Random Range Float` override (0.2 … 0.4) sits on the
  unselected Direct-Set pin and is INERT. *Was read as 0.2 … 0.4 following the NS_BasicAttack §2
  precedent; corrected per [P0-D2]. This is a 3.75× lifetime increase and it moves the cadence row
  (§6.1) — `Sparkles` is now the longest layer in the system.*
- Spawn shape: **Sphere Location**, `Sphere Radius **0.2**` (effectively a point),
  `Non Uniform Scale (1,1,1)`, `Sphere Orientation Axis (1,0,0)`, `Surface Only false`
- `Add Velocity from Point`: `Velocity Strength = Random Range Float 001` **350 … 500**,
  `Origin Offset (0,0,0)`, `Velocity Falloff Distance 100` → an omnidirectional 350–500 u/s spray
- Sprite rotation: `Random`, 0 … 360
- Velocity Scale (all three axes identical): `(0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`
- Color from Curve:
  - R `(0.615756, 0.50417)C (0.936915, 0)C`
  - G `(0.615756, 0.104)C (0.936915, 0.0896671)C`
  - B `(0.615756, 1)C (0.936915, 1)C`
  - A `(0.613341, 1)C (1, 0)L`
- Scale Sprite Size, Uniform Curve: `(0, 0)C (0.1, 1)C (1, 0)C`
- `Color.Scale Alpha 1`, dyn params `[1, 0, 0, 0]`

### 9 · Flare_01 — burst 1 @ t=**0.1**, life 0.5 s, size 50

- Initialize color `RGBA(0.102242, 0.658375, 1, 0.2)` — cyan
- Shared fade · dyn params `[1, 0, 0, 0]`

### 10 · Flare_02 — burst 1 @ t=**0.1**, life 0.5 s, size 50

- Initialize color `RGBA(0.102242, 1, 0.838799, 0.258)` — mint
- Shared fade · dyn params `[1, 0, 0, 0]`

### 11 · Sparkles_Stretched — **rate 20/s**, loop **0.4 s Once** (`Life Cycle Mode = Self`)

- Velocity-aligned sprite, material `Part04`
- Lifetime `[corpus-v3]`: **`Lifetime Min 0.3 / Max 0.6` drives**; the 0.2 … 0.4 override is inert
  (same resolution as above, [P0-D2])
- Sprite Size Mode **Random Non-Uniform**: `Sprite Size Min **(25, 70)**`,
  `Sprite Size Max **(40, 60)**` — note Min.y > Max.y, an authored inversion; the exporter reports
  them verbatim `[corpus]`
- Spawn shape: **Sphere Location**, `Sphere Radius **0.1**`, `Non Uniform Scale (1,1,1)`
- `Add Velocity from Point`: strength `Random Range Float 001` **500 … 1500**
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, 0.15)C (1, **−9.09372e-09**)C`
- Color from Curve:
  - R `(0, 1)C (0.079686, 1)L (0.290975, 0.646925)L (1, 0.223228)C`
  - G `(0, 0.913099)C (0.079686, 0.134)L (0.290975, 0.14)L (1, 0)C`
  - B `(0, 0.584079)C (0.079686, 0.731716)L (0.290975, 1)L (1, 0.116971)C`
  - A `(0.080893, 1)L (1, 0)C`
- **Three stacked size modules** (order matters — they multiply):
  1. Scale Sprite Size (Uniform Curve mode): uniform `(0, 0)C (0.1, 1)C (1, 0)C`; the non-uniform
     curve `X (0,0)L (1,1)L | Y (0,0)L (1,1)L` is present but inert under Uniform mode
  2. Scale Sprite Size 001 (Non-Uniform Curve mode): `X (1, 1)L | Y (0, 1)C (0.3, 0.25)C (1, 0.2)C`
     (its uniform curve `(0,0)L (1,1)L` is inert under Non-Uniform mode)
  3. **Scale Sprite Size by Speed**: `Scale Factor Curve (0, 0)L (1, 1)L`,
     `Velocity Threshold 1000`, `Min Scale Factor (1, 1)`, `Max Scale Factor (1, 2)` — stretches the
     streak up to 2× along its length at ≥1000 u/s
- `Color.Scale Alpha **0.8**`, dyn params `[**0**, 0, 0, 0]`

### 12 · Big_Star — burst 1 @ t=0, loop **0.3 s Once** (`Self`), life 0.3 s, size 200

- Initialize color `RGBA(1, 0.184475, 0.386429, 0.4)`
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, 0.25)C (1, −0)C` — inert, the emitter adds no velocity
- Scale Sprite Size, Uniform Curve: `(0, **−3.11599e-08**)C (0.1, 1)C (1, 0)C`
- **No colour animation** — Particle Update is
  `Scale Velocity → Solve Forces → Particle State → Dynamic Material Parameters → Scale Sprite Size`
- Dyn params `[**0**, 0, 0, 0]`

### 13–16 · Flare_Stretched_01 … 04 — burst 1 @ t=0 each, life **1.2 s**, Sprite Size Mode Non-Uniform

All four share one Scale Color and one Uniform size curve, and differ in colour, size and material:

- Scale Color · `Scale RGBA = R (0.218533, 1)L (1, 1)L | G (0.218533, 1)L (1, 1)L |`
  `B (0.218533, 1)L (1, 1)L | A (0.225777, 1)L (1, 0)L` — alpha holds 1 until t = 0.225777, then
  falls linearly to 0
- Scale Sprite Size is in **Non-Uniform Curve** mode; its uniform curve `(0, 0.5)C (0.1, 1)L (1, 1)L`
  is therefore inert, and the non-uniform curve below is what applies
- Dyn params `[1, 0, 0, 0]` on all four

| # | Emitter | Initialize color | Sprite Size | Non-Uniform Curve Sprite Scale | Material |
|---|---|---|---|---|---|
| 13 | Flare_Stretched_01 | `RGBA(0.491021, 0.00182116, 1, 0.5)` | (700, 100) | `X (0, 1)C (0.9, 2.30968e-08)C \| Y (0, 0.3)C (0.2, 1)C` | `Part01` |
| 14 | Flare_Stretched_02 | `RGBA(1, 0.508881, 0.982251, 0.3)` | (700, 100) | `X (0, 1)C (0.9, 2.30968e-08)C \| Y (0, 0.3)C (0.2, 1)C (0.9, 0.2)L` | `Part03_Bright` |
| 15 | Flare_Stretched_03 | `RGBA(0.0185002, 0.00402472, 0.130136, 1)` | **(1400, 180)** | `X (0, 1)C (0.9, 2.30968e-08)C \| Y (0, 0.3)C (0.2, 1)C` | `Part03_Bright` |
| 16 | Flare_Stretched_04 | `RGBA(1, 0.854993, 0.508881, 0.6)` | (400, 70) | `X (0, 1)C (0.9, 2.30968e-08)C \| Y (0, 0.3)C (0.2, 1)C (0.9, 0.0999999)L` | `Star03` |

All four collapse to zero width (`X → ~0` by t = 0.9) while holding or slightly shrinking height —
a horizontal "lens flare" streak that pinches shut.

### 17 · Star_01 — burst 1 @ t=**0.85**, life **0.1 s**, size 70

- Initialize color `RGBA(1, 0.184475, 0.386429, 0.4)`
- `Sprite Rotation Angle **45**` (Sprite Rotation Mode is not Random here — a fixed 45° tilt)
- Scale Sprite Size, Uniform Curve: `(0, 1)C (1, 0)C`
- **No colour animation** · dyn params `[**0**, 0, 0, 0]`
- Inert pins: `Lifetime Min 0.3 / Max 0.6`, `Uniform Sprite Size Min 40 / Max 50`,
  `Sprite Size (10, 10)`

### 18 · Star_02 — burst 1 @ t=**0.95**, life **0.1 s**, size 70

- Identical to Star_01 except `Sprite Rotation Angle **0.1**` and the later spawn time — the two
  form a two-beat sparkle at the very end of the cycle
- Scale Sprite Size, Uniform Curve: `(0, 1)C (1, 0)C` · dyn params `[0, 0, 0, 0]`

### 19 · Lightning — burst **3** @ t=0 **+ rate curve**, loop **0.5 s Once** (`Self`)

- Spawn Rate override: `Float from Curve 001` `(0, **20**)C (1, **0**)C` — spawn rate decays from
  20/s to 0 across the 0.5 s loop, ≈ 5 extra particles
- Lifetime `[corpus-v3]`: **`Lifetime Min 0.3 / Max 0.5` drives**; the 0.2 … 0.4 override is inert
  (same resolution as above, [P0-D2])
- Size: Random Uniform **30 … 100**
- Spawn shape: `Sphere Location`, `Sphere Radius **0**` — every particle spawns at the origin
- `Add Velocity from Point`: strength `Random Range Float 001` **350 … 500** — but from a
  zero-radius sphere the module has no `Position − Origin` to normalize, so it produces NO velocity.
  *(Resolved 2026-08-02 by the NS_Arrow_Cast precedent: its `LightningStrip` layer has the identical
  configuration — a disabled/degenerate location module under Add Velocity from Point — and was ruled to
  leave every particle at the cast point, separated only by its own randomized orientation. Applied
  identically here; §13.2.)*
- Sprite rotation: `Random`, 0 … 360; **Sprite Rotation Rate** = `Float from Curve 002`
  `(0, 1.68162e-07)C (0.1, **90**)C (0.9, 1.43051e-06)C` — spins at 90 °/s between t = 0.1 and 0.9
- **Sub UV Animation**: `SubUV Animation Mode = Linear`, `Start Frame 0`, `End Frame **4**`,
  `SubUV Loop Count 1`, renderer `SubUV: 2x2`
- Velocity Scale: `X/Y/Z (0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`
- Color from Curve — **five keys, with a strobing alpha**:
  - R `(0, 1)L (0.0748566, 1)L (0.598853, 1)C (0.811349, 0.287441)L (1, 0.0512695)C`
  - G `(0, 0.745404)L (0.0748566, 1)L (0.598853, 0.147027)C (0.811349, 0.0409152)L (1, 0.0409152)C`
  - B `(0, 0.304987)L (0.0748566, 1)L (0.598853, 0.982251)C (0.811349, 1)L (1, 1)C`
  - A `(0.162994, 1)L (0.329611, **0**)L (0.504679, 1)C (0.746152, **0**)L (0.959855, 1)L`
    — **the alpha strobes on/off/on/off/on over one life.** This is the flicker that makes the bolts
    read as lightning; a "fade to zero" simplification destroys the effect.
- Dyn param 1 (`dissolve`) — Float from Curve: `(0, 1)C (0.2, **−1**)C (0.3, **0.875**)C (1, −1)C`
  — a second, faster strobe on the dissolve channel; params 2/3/4 = 0
- Scale Sprite Size, Uniform Curve: `(0, 0)C (0.1, 0.8)C (1, 1)C`
- `Color.Scale Alpha 1`

### 20 · Flare_03 — burst **2** @ t=**0.1**, life 0.4 s, size 250

- Initialize color `RGBA(1, 0.4563, 0.111, 0.737104)` — orange
- Shared fade · dyn params `[1, 0, 0, 0]`
- Both particles are identical (no randomness in this emitter) and overlay exactly

### 21 · Flare_04 — burst 1 @ t=0, life 0.3 s, size 80

- Initialize color `RGBA(1, 0.384719, 0.0889999, 0.0371041)` — near-transparent orange
- Shared fade · dyn params `[**2**, 0, 0, 0]`

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new BURST + RATE row `[corpus-v3]`, per [P0-D3] + [P0-D5] + [P2-D5]: loop 2.0 s, particle lifetime
1.55 s, burst 30, spawn rate 40 per second.** Loop = the system's `Once` loop duration (*was 1.0 s, from the
inert emitter rows*); lifetime = max over layers of (spawn delay + resolved lifetime) — **`Sparkles`' 0.05 s
beat plus its resolved 1.5 s Max** (*was 1.2 s under the override-wins assumption, then 1.5 s from the
lifetime alone; corrected 2026-08-02 `[P2-E6]`, the [P2-D5a] class*); burst = the §2 count. Every shorter
layer zeroes colour, size and scale past its own lifetime, and spawn delays (0.05 / 0.1 / 0.85 / 0.95 s)
hide the layer for `age < delay` and run its curves on `(age − delay) / lifetime` — the NS_BasicAttack §5
mechanism.

**Both of this section's "cannot carry" items are SUPERSEDED by C2 + C5 (Phase 2), which landed after this
sheet was written.** The burst-slot approximation it recommended is NOT what shipped:

- **The two rate-spawned emitters are a real rate stack.** The row declares `SpawnRate = 40`, the behavior
  splits burst from rate particles by `SpawnPhase = fmod(EmitterAge − Age, Loop)` ([P2-D5]), and a rate
  particle draws its layer by rate share — 20/40 each, since both emitters carry a nominal 20/s.
- **`Self / Once` becomes a spawn-phase WINDOW.** A rate particle born past its own emitter's window
  (0.4 s for `Sparkles_Stretched`, 0.5 s for `Lightning`) is hidden, which is exactly what "runs once and
  never repeats" means on a template that loops.
- **`Big_Star` needs nothing.** It is `Self / Once` too, but it carries only a burst — and a burst-only
  one-shot fires exactly once per activation, which is what a template burst is (NS_Gunshot_Cast §14.1).
- **`Lightning`'s rate is a CURVE (20 → 0 over its 0.5 s window), and a row rate is a constant.** The row
  declares its PEAK and the behavior THINS the stream: a bolt rate particle at phase `p` survives with
  probability `1 − p/0.5`. Thinning a uniform stream by that factor reproduces the source's arrival density
  exactly and integrates to the same ≈ 5 particles per firing. §9.2.

Loop duration resolved `[corpus-v3]` — see §2. The 2.0 s `Once` cycle leaves Star_02's t = 0.95
burst plenty of room.

### 6.2 VisTag / renderer needs

- **18 camera-facing sprite layers over 9 distinct materials** (Part01 ×3, Part02 ×3,
  Part01_Bright ×2, Part03_Bright ×3, Rainbow, Ring01, Star02 ×3, Star03, Lightning02) →
  **9 row-declared camera-facing sprite renderers**, a renderer kind that **does not exist**
  (§6.5, gap 1).
- **1 velocity-aligned sprite** (Sparkles_Stretched, `Part04`) → the existing
  `VelocityAlignedSprite` row-renderer kind covers this. ✔
- **1 of the camera sprites needs `SubUV: 2x2`** (Lightning) → no sub-UV support anywhere in the
  pipeline (§6.5, gap 3).

VisTag ids: allocate at implementation time above `Get_RosterVisTag_Max()`; never restate a literal.

### 6.3 Mesh / texture / look needs

- **Meshes: none.**
- **CkUsf looks: 10 new** (9 camera-sprite + 1 velocity-aligned), all parameterizations of
  `CkUsf_Look_DissolveAdd` per §4 — except `Rainbow` (needs the LUT chain, gap 5) and `Lightning02`
  (needs sub-UV, gap 3, and is the only instance whose distortion branch is live).
  `Arrows` is NOT needed — both emitters using it are disabled.
- **Textures: 9 procedural stand-ins**, following the NS_BasicAttack §7 method (measure the corpus
  PNGs, bake from the numbers, never copy pixels):
  - `T_VFX_Part_01` → **`T_CkParticles_SoftParticle` already exists**, measured off this exact asset. Reuse.
  - `T_VFX_Part_04` → **`T_CkParticles_SparkStreak` already exists**, measured off this exact asset
    (NS_BasicAttack §7). Reuse — and note its u/v orientation is load-bearing for a velocity-aligned
    quad, exactly as that recipe records.
  - `T_VFX_Noise_02` → existing `T_CkParticles_TileNoise`.
  - `T_VFX_Part_02`, `T_VFX_Part_03`, `T_VFX_Ring_01`, `T_VFX_Ring_02`, `T_VFX_Star_02`,
    `T_VFX_Star_03` — **new bakes**; measure each, do not assume they match the existing library.
  - `T_VFX_Lightning_03` — a **2×2 flipbook sheet in `TSF_BGRA8` colour**, four bolt frames. Neither
    "greyscale mask" nor "single shape" — a new class of procedural bake (four distinct branching
    bolt patterns in one atlas). See gap 3.
  - `T_VFX_LUT_Rainbow_01` — a 512×2 colour LUT; procedurally regenerable as a hue sweep but a new
    asset shape for the generator (`sRGB: true`, `TC_Default`, non-square). See gap 5.

### 6.4 Behavior id

**Do NOT allocate an id in this document.** Take the next free id from `ck::particles::NumBehaviors`
at implementation time and bump it. This batch contains five planned effects; whoever implements
second must re-read the roster, not this sheet.

Layer partition: `Seed % 30` over the 30 deterministic burst slots (plus whatever slots the two
rate emitters are approximated into — 43 total if both are folded in). A modulo over a burst of
exactly N gives the source's partition by construction; never `Rand(Seed) < k` probability bands
(NS_BasicAttack lesson 2).

### 6.5 CAPABILITY GAPS — what the pipeline cannot express today

Conservative list. Each is a real blocker or a real approximation.

1. **No row-level camera-facing-sprite renderer.** `FCk_ParticlesRendererSpec` supports `Mesh` and
   `VelocityAlignedSprite` only; this effect needs nine camera-facing sprite renderers with nine
   looks. **Blocking.** Additive fix, mirrors `VelocityAlignedSprite`. Same gap in all five effects
   in this batch.
2. **One template = one cadence; this source has four.** The 18 System-mode emitters cycle on the
   system clock; Sparkles_Stretched (0.4 s), Big_Star (0.3 s) and Lightning (0.5 s) are
   `Life Cycle Mode = Self` + `Loop Behavior = Once` — they play **once and never repeat**. A
   CkParticles template replays everything every loop. Options: (a) accept the replay and record it
   as a deviation (the three layers become part of the loop — visually this turns a one-shot cast
   flash into a repeating pulse); (b) split into two behaviors/templates and have the caller spawn
   both. `CkParticles/CLAUDE.md` explicitly forbids faking cadence with `frac(Age/Cycle)` inside the
   behavior, so (a) must be an honest deviation, not a hidden hack. **Decide before writing HLSL.**
3. **No sub-UV / flipbook support anywhere in the pipeline.** The Lightning layer is a `SubUV: 2x2`
   sheet driven by `Sub UV Animation` (Linear, frames 0 → 4, loop count 1). The DI's stage output has
   no `SubImageIndex`, `FCk_ParticlesRendererSpec` has no `SubImageSize`, and the texture generator
   has never baked an atlas. Without it the lightning bolts are one static frame instead of four.
   **This is the second-largest gap in this effect** and it recurs in NS_Lightning_Hit and
   NS_Lightning_Muzzle.
4. **`Scale Sprite Size by Speed` needs velocity at shade time** — expressible (the behavior owns
   velocity and can fold the factor into `O.Size`), listed only so it is not mistaken for a gap.
   Not a blocker.
5. **The gradient-map (LUT) chain is not implemented in `CkUsf_Look_DissolveAdd`.** `Rainbow` uses a
   real 512×2 colour LUT with `GradientMap_Displacement 0.9` / `Gradient_Invert 2`. Both shipped
   recipes dropped this chain **because their GradientMap was a white pixel**; that justification does
   not hold here. Either extend the family shader or record the Raimbow layer as a deliberate
   fidelity loss.
6. **Per-particle sprite rotation and rotation RATE on a camera-facing sprite are unconfirmed.**
   Raimbow, Ring, Sparkles and Lightning all randomize rotation 0–360°, Star_01/Star_02 set fixed
   angles (45° / 0.1°), and Lightning additionally spins at 90 °/s. The DI writes `OutRotation`, but
   `CkParticles/CLAUDE.md` documents `Rotation` as applying on **VisTag 2** (smoke) only.
   `[unresolved: whether the shared camera-sprite renderer binds `Particles.SpriteRotation`.]`
   If it does not, the renderer kind from gap 1 must.
7. **World space vs the template's local space.** All 21 emitters are `LocalSpace: false`; the
   CkParticles template is local-space. Same known deviation NS_BasicAttack §13.2 records — visible
   if the caster moves during the 1.2 s life, which for a *cast* animation is plausible.
8. **Material parameters not plumbed through the family look**: `Core_Intensity` (0 on eight of
   eleven), `Core_Power` (**0** on Lightning02), `Glow_Intensity` (**0.3** on Part02),
   `Gradient_Invert` (0.5 on five, **2** on Rainbow), `Opacty_StepAdd` (**0.3** on Rainbow),
   `CamOffset` (**50** on Part03_Bright — three layers use it), `Opacty_DepthFade` (20 on eight,
   **30** on Part04). `DepthFade` is a pre-existing documented CkUsf gap. `CamOffset 50` on three
   large flare layers is the one most likely to change sorting/occlusion visibly.
9. **Two disabled emitters are deliberately not recreated** (Arrow, BigArrow). Recorded here so the
   omission is a decision. If they are ever re-enabled they need the `Arrows` material,
   `T_VFX_Arrow_01`, and two more velocity-aligned sprite renderers.

**Not gaps — confirmed absent from this source, so nothing to build:** no mesh renderers, no ribbon
renderers, no light renderers, no GPU sims, no collision, no event handlers, no user parameters, no
material-binding indirection.

---

## 7. Textures — ONE new bake

Nine of the ten needed paints already existed as identity reuses; only the bolt sheet is new.

| Source texture | Stand-in | Status |
|---|---|---|
| `T_VFX_Part_01` | `SoftParticle` | existing |
| `T_VFX_Part_02` | `SoftParticleBright` | existing |
| `T_VFX_Part_03` | `SoftParticleFine` | existing |
| `T_VFX_Part_04` | `SparkStreak` | existing |
| `T_VFX_Ring_01` | `RingUneven` | existing |
| `T_VFX_Ring_02` | `RingFlare` | existing |
| `T_VFX_Star_02` / `_03` | `StarFourTight` / `StarFourSplit` | existing |
| `T_VFX_Noise_02` / `_04` | `TileNoise` / `TileNoiseCoarse` | existing |
| **`T_VFX_Lightning_03`** | **`LightningSheet`** | **NEW — `ECk_VfxTextureKind::MaskSheet`** |
| `T_VFX_LUT_Rainbow_01` | `LutWhite` | held back by [P1-D1], §13.5 |
| `T_VFX_Arrow_01` | — | not needed; both consumers are DISABLED emitters |

### 7.1 `LightningSheet` — measured, and structurally unlike the other two atlases

The corpus PNG is `TSF_BGRA8` with a FLAT alpha, so the greyscale RGB carries the mask — the same reading
`T_VFX_Impact_02` already took (NS_Gunshot_Hit §7). Measured on the 512² file, per 256² frame:

| Measurement | Frame 0 | Frame 1 | Frame 2 | Frame 3 |
|---|---|---|---|---|
| Peak | 0.9961 | 0.9922 | 0.9843 | 0.9922 |
| Mean | 0.0810 | 0.0478 | 0.0477 | 0.0636 |
| Coverage > 0.05 | 0.156 | 0.110 | 0.105 | 0.145 |
| Coverage > 0.5 | 0.077 | 0.041 | 0.044 | 0.059 |
| Annulus peak radius | ≈ 0.45 | ≈ 0.65 | ≈ 0.65 | ≈ 0.65 |
| Empty inside r | 0.30 | 0.05 | 0.05 | 0.10 |

**The load-bearing measurement is the frame-to-frame correlation: −0.01 … 0.25.** `WindSheet` measures
0.79–0.88 and `ImpactSheet` 0.51–0.78 — those two are ONE shape evolving, and their bakes step a noise
offset per frame. This one is four INDEPENDENT paintings, so the frame index has to RESEED the field
instead of nudging it (stride 53 over a 3-tile base period). Building it like the other two would have
produced four near-identical bolts.

Second measurement that shaped the bake: the radially averaged power spectrum peaks at 1–2 cycles per frame
and is down to ~5 % by 8, so these are BROAD ragged arcs, not hairline filaments — a fine-frequency ridged
noise reads as static rather than as lightning.

Recipe: ridged value noise `1 − |2·Fbm − 1|` at 3 tiles / 4 octaves, thresholded at **0.92** (fitted against
the measured coverage), windowed by a per-frame Gaussian annulus with an inner hole and an outer fade, and
scaled by a per-frame gain fitted so the bake's mean equals the measured mean EXACTLY. Result: coverage
> 0.05 within 0.02–0.05 of measured, coverage > 0.5 within 0.01, peaks 0.95–1.0 against 0.98–1.0, and bake
frame-to-frame correlation 0.02–0.15 against the source's −0.01–0.25.

---

## 8. Mesh

**N/A** — all twenty-one source renderers are sprites (§3). This is the only Cast port in the batch with no
carrier mesh at all.

---

## 9. The behavior — `Behavior_LightningCast.ush` + `ExecuteStage_CPU` case 35

### 9.1 Burst and rate on one row

`SpawnPhase = fmod(EmitterAge − Age, 2.0)`; phase ≈ 0 ⇒ a burst particle taking the exact `Seed % 30`
partition; anything else ⇒ a rate particle taking a rate-weighted draw over the two streaming emitters, then
gated on its own emitter's window. The idiom is HealCast's ([P2-D5]); what is new is §9.2.

Burst partition (§2 emitter order, enabled only): `Glow_01` 0, `Glow_02` 1, `Glow_03` 2, `Raimbow` 3,
`Ring` 4, `Sparkles` 5–14, `Flare_01` 15, `Flare_02` 16, `Big_Star` 17, `Flare_Stretched_01…04` 18–21,
`Star_01` 22, `Star_02` 23, `Lightning` 24–26, `Flare_03` 27–28, `Flare_04` 29.

### 9.2 Thinning a flat rate back into a falling one

The source's `Lightning` emitter overrides its Spawn Rate with `Float from Curve 001` = `(0, 20)C (1, 0)C`
over its own 0.5 s window — a linear decay to zero, integrating to 5 particles. A cadence row carries a
CONSTANT rate, so the row declares the PEAK (20/s, plus `Sparkles_Stretched`'s flat 20/s = 40 total) and the
behavior removes the surplus:

```
survive  ⟺  Rand(Seed, 14) < 1 − Phase / 0.5
```

Thinning a uniform stream of density `R` by a factor `f(p)` yields density `R·f(p)`, so this IS the source's
`20·(1 − p/0.5)` exactly, and its integral over the window is `20 · 0.5 / 2 = 5`. It is stateless, per-Seed
deterministic, and costs one hash. Measured over 4000 draws per band: survival 0.901 / 0.698 / 0.495 /
0.296 / 0.095 against the exact 0.9 / 0.7 / 0.5 / 0.3 / 0.1, mean 0.497.

`Sparkles_Stretched`'s rate is genuinely flat, so it is NOT thinned — and that makes it the test's dead
control (§11).

### 9.3 The strobe

`Lightning`'s alpha is `(0.162994, 1) (0.329611, 0) (0.504679, 1) (0.746152, 0) (0.959855, 1)` — on, off,
on, off, on across ONE life — and its dissolve channel carries a second, faster strobe
`(0, 1) (0.2, −1) (0.3, 0.875) (1, −1)`. Both are transcribed verbatim. A "fade to zero" simplification
would pass every luminance check and destroy the effect.

### 9.4 Sub-UV in LINEAR mode

`Lightning` is the cookbook's only flipbook in Niagara's **Linear** mode rather than Random: every particle
starts on frame 0. Its declared `End Frame` is 4 on a four-frame sheet, so the run takes five steps and the
fifth wraps back onto frame 0 — `fmod(min(floor(t·5), 4), 4)`. §13.1.

### 9.5 The spin

`Sprite Rotation Rate` is `(0, ~0)C (0.1, 90)C (0.9, ~0)C` in degrees per second. The accumulated angle is
that curve's exact integral in normalized-life units times the particle's own life — written out as a
three-line closed form rather than accumulated per frame, because a stepwise sum would tie the spin to frame
cadence and drift against the CPU mirror.

---

## 10. Looks and renderers

Ten row renderers on VisTags **147–156**; nine looks already existed.

| VisTag | Kind | Look | Source emitters |
|---|---|---|---|
| 147 | CameraFacingSprite | `PartDisAdd01` | `Glow_01`, `Glow_03`, `Flare_Stretched_01` |
| 148 | CameraFacingSprite | `PartDisAdd02` | `Glow_02`, `Flare_01`, `Flare_02` |
| 149 | CameraFacingSprite | `RainbowDisAdd` | `Raimbow` |
| 150 | CameraFacingSprite | `RingDisAdd01` | `Ring` |
| 151 | CameraFacingSprite | `PartDisAdd01Bright` | `Sparkles`, `Flare_04` |
| 152 | VelocityAlignedSprite | `PartDisAdd04` | `Sparkles_Stretched` |
| 153 | CameraFacingSprite | `StarDisAdd02` | `Big_Star`, `Star_01`, `Star_02` |
| 154 | CameraFacingSprite | `PartDisAdd03Bright` | `Flare_Stretched_02/03`, `Flare_03` |
| 155 | CameraFacingSprite | `StarDisAdd03` | `Flare_Stretched_04` |
| 156 | CameraFacingSprite, 2×2 | **`LightningDisAdd02`** | `Lightning` |

The nine reuses were each checked value-by-value against §4's delta table before being taken. Note that §4's
reference instance is `M_VFX_DisAdd_Ring04` (not `Part01`), so several rows that LOOK like deltas are
inherited values — e.g. `Gradient_Invert 0` on `Ring01`/`Star02`/`Star03` is Ring04's own, and matches the
existing looks exactly.

**`LightningDisAdd02` is the port's one new look** and the family's oddest instance: the ONLY one whose
distortion branch is both live AND fast-panning (`Distortion_Intensity 0.5` at speed 0.7/0.7 on both axes),
and the only one whose `Core_Power` resolves to 0. Its shape is the new `LightningSheet` atlas, divided by
the row renderer's `SubImageSize` rather than by anything in the look.

`Arrows` is deliberately not authored — both of its emitters are disabled (§13.6).

---

## 11. Tests

`Test_Particles_LightningCastBehavior.cpp` (`CkTests.UnitTests.CkParticles.LightningCastBehavior`):

- **The cadence row**: 2.0 / 1.55 / 30 / rate 40, ten renderers, **zero** meshes, one velocity-aligned, one
  2×2 sheet.
- **The burst partition**: the re-implemented 30-slot map reproduces the source's per-emitter counts
  (1,1,1,1,1,10,1,1,1,1,1,1,1,1,1,3,2,1 and 0 for the rate-only emitter), and each layer's seeds draw
  through its own renderer.
- **The rate draw**: 400 000 seeds split 0.4994 / 0.5006 against the exact half, and both streams reachable.
- **The spawn-phase split, three ways**: a particle at phase 0 takes the burst table, one at the head of its
  window takes the rate table, and one past its window is hidden — 2000 seeds, each of the three counted
  against its own opposite.
- **The thinning, two-sided and with a live control**: the bolt stream's surviving fraction is compared band
  by band against the source's own ramp (bar 0.03, observed max error 0.005), asserted MONOTONE FALLING, and
  asserted to integrate to a half (bar 0.02, observed 0.497) — while `Sparkles_Stretched`, whose source rate
  is flat, must survive at > 0.999 in every band. Without that control the test would pass against a
  behavior that thinned both streams, or neither.
- **The strobe**: the bolt alpha must cross the 0.25 line at least three times in one life. A monotone fade
  scores one.
- **The flipbook**: LINEAR mode starts every particle on frame 0, the index stays in 0..3, and all four
  frames are used.
- **The closing beats**: `Star_01` hidden at 0.5 and alive at 0.86, `Star_02` still hidden at 0.86 and alive
  at 0.96, and `Star_01`'s fixed 45° tilt.
- **Anti-vacuity** over all nineteen layers (burst seeds for eighteen, rate seeds for the streaming one) and
  **death** past 1.55 s on BOTH spawn paths.

---

## 12. Verification — A/B protocol

Gym pair `LIGHTNING CAST` (`Gym.VfxExamples.LightningCast.{Ck,Original}`), offset `(0, 0, 120)`, scale 1.
Loop-Once, so both sides re-arm together and the A/B is a synced replay from t = 0.

`[HUMAN-VERIFY]` — this effect's palette is its identity: deep blue, magenta and warm white, never orange.

- (a) t = 0: three second-long shells — a 550-unit deep blue, a 200-unit magenta and a 250-unit warm white —
  plus a 200-unit star and four horizontal lens streaks (400–1400 units) that pinch shut along X by t ≈ 1.1.
- (b) t = 0.05: the magenta shockwave ring, which holds ONE colour and one alpha for its whole 0.75 s and
  leaves purely by dissolve — if it fades, the no-colour-module reading is wrong.
- (c) t = 0.05: ten blue motes thrown omnidirectionally at 350–500 u/s, living up to 1.5 s. These are the
  longest-lived thing in the system.
- (d) **the bolts**: three at t = 0 plus roughly five more streamed over the next half second, arriving
  DENSELY at first and thinning to nothing — that decay is §9.2's whole claim. Each bolt sits at the cast
  point, spins at 90 °/s between t = 0.1 and 0.9 of its life, plays four frames, and **strobes on/off/on/
  off/on**. If they read as steady sprites, the alpha curve is not running.
- (e) t = 0.1: the lens ring at a TENTH opacity (the faintest Raimbow in the cookbook), two small cyan/mint
  pips and two overlaid orange pips.
- (f) the velocity-aligned streak stream, 20/s over the first 0.4 s only, stretching up to 2× at speed.
- (g) t = 0.85 and 0.95: two 70-unit stars, one tilted 45°, closing the cycle. They are easy to miss and easy
  to delete — their stored 0.3 s loop is INERT (§2).

---

## 13. Confirmed fidelity differences

1. **`End Frame = 4` on a four-frame sheet.** The bolt run takes five steps and the fifth wraps to frame 0.
   `[inferred]` — clamping instead is equally consistent with the corpus. One frame at the end of a
   0.3–0.5 s life.
2. **The bolts have NO velocity.** `Sphere Radius = 0` leaves `Add Velocity from Point` nothing to
   normalize, so every bolt sits at the cast point (§5.19, resolved by the NS_Arrow_Cast precedent). If the
   source's module happens to emit a default direction there, the recreation's bolts will be more clustered
   than the original's.
3. **The rate thinning is exact in DENSITY, not in count per firing.** The source spawns a deterministic
   ~5 bolts; the recreation spawns a Binomial draw with the same mean. Firing to firing, the bolt count
   varies where the source's does not.
4. **World space on all 21 emitters**; the template is local space. Same known deviation as every Cast port.
5. **`RainbowDisAdd` ships against `LutWhite`**, pending [P1-D1]. `Raimbow` is the only consumer here and it
   runs at 0.1 alpha, so this is the least visible instance of that gap in the cookbook.
6. **`Arrow` / `BigArrow` are not recreated** — both DISABLED. Their values are transcribed in §5.5–5.6 so
   the omission stays a decision; re-enabling them needs the `Arrows` look, `T_VFX_Arrow_01` and two more
   velocity-aligned renderers.
7. **Unplumbed family parameters** (§6.5 #8): `Core_Power 0` on `Lightning02`, `Core_Intensity`,
   `Opacty_StepAdd 0.3` on Rainbow, `CamOffset 50` on `Part03_Bright` (three layers), `Opacty_DepthFade`.
   `Core_Power 0` is the one specific to this port — the source disables the bolt paint's core-power curve
   and the recreation cannot.
8. **Sprite rotation on a camera-facing renderer** is assumed to bind `Particles.SpriteRotation`. §6.5 #6
   flagged this as `[unresolved]`; the C1 camera-facing renderer kind writes the attribute, but that it is
   consumed is `[EDITOR-VERIFY]` at the inspection stage — the bolts' 90 °/s spin is the visible test.

---

## 14. Reusable lessons

1. **A time-varying source spawn rate becomes PEAK + thinning, not an averaged constant.** Declaring the
   mean (10/s) would have produced the right particle count with a flat arrival distribution; declaring the
   peak and thinning by the curve's own shape reproduces the density at every instant, costs one hash, and
   stays stateless. Any `Float from Curve` on a Spawn Rate is expressible this way as long as the curve is
   bounded by the declared peak.
2. **A thinning test needs an unthinned control on the SAME row.** `Sparkles_Stretched`'s flat rate is the
   dead control that makes the assertion discriminating: without it, thinning both streams or neither would
   still pass a per-band ratio check. This is §14.7's lesson from NS_BuffLoop in a new shape — the bar is
   not "does the number look right", it is "does an obvious wrong implementation fail".
3. **Frame-to-frame correlation decides how a flipbook atlas is BUILT.** Three sheets in this library, three
   different constructions: one shape nudged (`WindSheet`, corr 0.79–0.88), one shape stepped through
   measured per-frame constants (`ImpactSheet`/`LensSheet`, 0.51–0.93), four independent realizations
   (`LightningSheet`, −0.01–0.25). Measure the correlation before writing the paint function.
4. **`Life Cycle Mode = Self` does not by itself mean "window".** Three Self emitters here: two carry rates
   and need windows, one carries only a burst and needs nothing. The discriminator is the presence of a
   `SpawnRate` module.
5. **A delta table's reference instance matters.** §4 diffs against `M_VFX_DisAdd_Ring04`, not the family's
   usual `Part01`, so values that read as deltas (`Gradient_Invert 0`, `Opacity_Boldness 1`) are inherited.
   Checking a reuse means resolving both tables to absolute values first.
