# Recipe: NS_PickupCast → CkParticles (PLANNED)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior `.ush`, no CPU mirror, no CkUsf look, no cadence row, no texture bake, no test, no gym
station exists for this effect. Every number below is archaeology read out of the extracted corpus;
nothing here has been compiled, generated, rendered, or looked at. Sections 7+ are reserved for the
implementation session.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_PickupCast` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior ID | **not allocated** — take the next free id at implementation time from `ck::particles::NumBehaviors` |
| CkUsf looks | none yet |

Corpus evidence (regenerate per [README.md](README.md); `Saved/` is machine-local):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_PickupCast.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part01_Bright,Part02,Part03_Bright,Rainbow,Ring01,Star01,Star02}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_Ring04.json` (family reference for the diff)
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Part_03,Ring_01,Ring_02,Star_01,Star_02,Noise_02,LUT_Rainbow_01,WhitePixel}.json`

**The source Niagara asset was never opened in the Niagara editor.** Every fact below is `[corpus]`
unless tagged otherwise.

> ### Sibling in the pack — NOT a name collision here
> `[corpus]` The `Anime_Stylized_VFX` branch ships **`NS_Pickup_Cast`** (with underscores) at
> `Vefects/Anime_Stylized_VFX/VFX/Particles/`, so there is no exact-name ambiguity for this file.
> Discriminator if you land in the wrong one: the sibling has a **non-empty `userParameters`** list
> (10 entries incl. `User.Scale Overall`) and `MI_VFX_*` materials; **this** system has an **empty
> `userParameters`** array and `M_VFX_DisAdd_*` materials.

---

## 2. System anatomy `[corpus]`

**11 CPU emitters, all enabled, all `LocalSpace: false` (WORLD space), `Determinism: false`,
`Bounds: Dynamic`. Every emitter spawns through `Spawn Burst Instantaneous`. Every emitter renders
one camera-facing sprite (`Alignment: Unaligned`, `Facing: FaceCamera`, `Sort: None`).**
No mesh renderers, no ribbons, no sub-UV, no light renderers, no events, no GPU sims.
`userParameters` is **empty**.

**22 particles per loop.**

| # | Emitter | Count | Spawn t (s) | Lifetime (s) | Sprite size (effective) | Dyn param 1 | Material |
|---|---|---|---|---|---|---|---|
| 1 | Bomb_Glow_01 | 1 | 0 | 1 | Uniform **500** | 1 | `M_VFX_DisAdd_Part01` |
| 2 | Bomb_Glow_02 | 1 | 0 | 1 | Uniform **230** | **0** | `M_VFX_DisAdd_Part01` |
| 3 | Bomb_Glow_03 | **3** | **0.05** | 0.5 | Uniform **100** | 1 | `M_VFX_DisAdd_Part02` |
| 4 | Raimbow *(sic)* | 1 | 0 | 0.5 | Uniform **250** | **0.5** | `M_VFX_DisAdd_Rainbow` |
| 5 | Sparkles | **10** | **0.05** | rand **0.5 … 1.0** `[corpus-v3]` | Random Uniform **7 … 10** | 1 | `M_VFX_DisAdd_Part01_Bright` |
| 6 | Ring01 | 1 | 0 | 1 | Uniform **120** | curve (§5) | `M_VFX_DisAdd_Ring01` |
| 7 | Flash_Glow_01 | 1 | **0.05** | **0.2** | Uniform **800** | 1 | `M_VFX_DisAdd_Part01` |
| 8 | Flash_Glow_02 | 1 | **0.05** | **0.1** | Uniform **150** | 1 | `M_VFX_DisAdd_Part03_Bright` |
| 9 | Star01 | 1 | **0.2** | 0.3 | Uniform **40** | 1 | `M_VFX_DisAdd_Star01` |
| 10 | Star02 | 1 | **0.1** | 0.3 | Uniform **100** | curve (§5) | `M_VFX_DisAdd_Star02` |
| 11 | Ring02 | 1 | 0 | 1 | Uniform **140** | curve (§5) | `M_VFX_DisAdd_Ring01` |

Dynamic material parameters 2, 3 and 4 are **0 on every emitter**; only `Write Parameter Index 0` is
true anywhere. Every emitter carries `Kill Particles When Lifetime Has Elapsed = true`,
`Position Mode = Simulation Position`, `Position Offset (0,0,0)`.

**Eight distinct materials over eleven emitters** — Part01 appears three times (emitters 1, 2, 7) and
Ring01 twice (6, 11); the other six are one-offs.

**Cadence caveat — the "Loop Duration = 1" rows are inert.** `[corpus]` All 11 emitters run
`Life Cycle Mode = System`, so the emitter's own `Loop Behavior` / `Loop Duration` are driven by the
system. Every emitter nevertheless stores `Loop Behavior = Infinite`, `Loop Duration Mode = Fixed`,
`Loop Duration = 1`, `Loop Delay = 0`, `UseLoopDelay = false`, `UseLoopCountLimit = false`,
`Loop Count Limit = 1` (inert — the same authored leftover NS_Lightning_Range §4 documents),
`Spawn Probability = 1`, `Use Spawn Probability = false`.

**System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
This is the authority per [P0-D1]; the emitter "Infinite / 1.0 s" rows above are inert leftovers.
*(Was `[unresolved]` — the pre-v3 export wrote emitter stacks only, and 1.0 s was the working guess.)*

---

## 3. Mesh geometry

**N/A — no mesh renderers.** All eleven renderers are `NiagaraSpriteRendererProperties`.

---

## 4. Material family + delta table `[corpus]`

All eight materials are instances of ONE parent,
`/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd` — the family CkUsf already
implements as `CkUsf_Look_DissolveAdd` in `/CkUsf/Looks/DissolveAdd.ush`.

Every one of the eight: `MD_Surface`, `BLEND_Translucent`, `MSM_Unlit`, `twoSided: false`, outputs
`EmissiveColor` + `Opacity` only, dynamic-parameter channels **`dissolve`, `distortion`, `offset`,
`core_color`**, expression histogram identical to the family reference.

Deltas versus `M_VFX_DisAdd_Ring04` (reference: `Brightness 30`, `Color_CoreDifferent 1`,
`Core_Intensity 1`, `Core_Power 1`, `Glow_Intensity 1`, `Opacity_Boldness 1`, `Opacty_DepthFade 10`,
`Opacty_StepAdd 0.1`, `Gradient_Invert 0`, `GradientMap_Displacement 0.1`, `Dissolve_Speed_X/Y 0.2`,
`Distortion_Scale_X/Y 0.1`, `Distortion_Intensity 0`, `Dissolve 0`, `Color_Core RGBA(1,1,1,0)`,
`GradientMap_Tex T_VFX_WhitePixel`, `GradientShape_Tex T_VFX_Noise_02`):

| Material | Main_Tex / Color_Tex | Dissolve_Tex | Distortion_Tex | Brightness | Other deltas |
|---|---|---|---|---|---|
| `Part01` | `T_VFX_Part_01` | `T_VFX_Part_01` | `T_VFX_Noise_02` | **1** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Gradient_Invert 0.5`; `Opacity_Boldness 0.5`; `Opacty_DepthFade 20` |
| `Part01_Bright` | `T_VFX_Part_02` | `T_VFX_Part_02` | `T_VFX_Noise_02` | **10** | `Color_CoreDifferent 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Gradient_Invert 0.5`; `Opacty_DepthFade 20` |
| `Part02` | `T_VFX_Part_02` | `T_VFX_Part_02` | `T_VFX_Noise_02` | **1** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; **`Glow_Intensity 0.3`**; `Gradient_Invert 0.5`; `Opacity_Boldness 0.5`; `Opacty_DepthFade 20` |
| `Part03_Bright` | `T_VFX_Part_03` | `T_VFX_Part_03` | `T_VFX_Noise_02` | **10** | `Color_CoreDifferent 0`; **`CamOffset 50`**; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Gradient_Invert 0.5`; `Opacty_DepthFade 20` |
| `Rainbow` | **`T_VFX_Ring_02`** (Color_Tex `T_VFX_Part_01`) | `T_VFX_Part_01` | `T_VFX_Noise_02` | **1** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; **`GradientMap_Tex T_VFX_LUT_Rainbow_01`**; **`GradientMap_Displacement 0.9`**; **`GradientShape_Tex T_VFX_Part_01`**; **`Gradient_Invert 2`**; **`Opacity_Boldness 1.5`**; **`Opacty_StepAdd 0.3`**; `Opacty_DepthFade 20` |
| `Ring01` | `T_VFX_Ring_01` | `T_VFX_Ring_01` | `T_VFX_Noise_02` | **10** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Opacty_DepthFade 20` |
| `Star01` | `T_VFX_Star_01` | `T_VFX_Star_01` | `T_VFX_Noise_02` | **6** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1` |
| `Star02` | `T_VFX_Star_02` | `T_VFX_Star_02` | `T_VFX_Noise_02` | **6** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1` |

**`Distortion_Intensity` is 0 on all eight — the distortion branch is entirely dead in this system.**

**`M_VFX_DisAdd_Rainbow` is the first instance in the cookbook with a LIVE gradient-map chain.**
Everywhere else in the DissolveAdd family (Ring04, the Slash set, all of NS_PickupLoop) the
`GradientMap_Tex` is `T_VFX_WhitePixel` and the chain is a documented no-op. Here it is a real
512×2 rainbow LUT with `GradientMap_Displacement 0.9`, `GradientShape_Tex T_VFX_Part_01` and
`Gradient_Invert 2`. That is what makes the "Raimbow" layer a rainbow at all, and it is **not
implemented in the CkUsf family shader** — see §6.5, gap 4.

Referenced textures `[corpus]` (all 512×512, `sRGB: false`, `TC_Alpha`, `TEXTUREGROUP_World`
greyscale masks unless noted):

| Texture | Format | Address | Role |
|---|---|---|---|
| `T_VFX_Part_01` | `TSF_G8` | `TA_Clamp`/`TA_Clamp` | Part01 main+dissolve; Rainbow dissolve + gradient shape |
| `T_VFX_Part_02` | `TSF_G8` | `TA_Clamp`/`TA_Clamp` | Part01_Bright, Part02 main+dissolve |
| `T_VFX_Part_03` | `TSF_G8` | `TA_Wrap`/`TA_Wrap` | Part03_Bright main+dissolve |
| `T_VFX_Ring_01` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Ring01 main+dissolve |
| `T_VFX_Ring_02` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Rainbow main |
| `T_VFX_Star_01` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Star01 |
| `T_VFX_Star_02` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Star02 |
| `T_VFX_Noise_02` | `TSF_G16` | `TA_Wrap`/`TA_Wrap` | Distortion_Tex on all eight (**dead branch**) + GradientShape_Tex on seven |
| `T_VFX_LUT_Rainbow_01` | `TSF_BGRA8`, **512×2**, `sRGB: true`, `TC_Default` | `TA_Wrap`/`TA_Wrap` | Rainbow's gradient LUT — **the only colour texture in the set** |
| `T_VFX_WhitePixel` | `TSF_RGBA16`, 1×1, `sRGB: true`, `TC_Default` | `TA_Wrap` | no-op GradientMap on the other seven |

---

## 5. Per-layer runtime curves `[corpus]`

`t` = NormalizedAge (0 → 1 over that emitter's own lifetime). `C` = constant key, `L` = linear key —
transcribed verbatim.

Six of the eleven emitters share one **Scale Color** shape:
`Scale RGBA = R (0,1)L (1,1)L | G (0,1)L (1,1)L | B (0,1)L (1,1)L | A (0,1)L (1,0)L`
— i.e. RGB untouched, **alpha ramps linearly 1 → 0 over the whole life**. Emitters 1, 2, 3, 7, 8 use
exactly this; emitter 4 (Raimbow) uses a variant. Where an emitter has it, it is the ONLY colour
animation — those layers keep their Initialize Particle colour and just fade.

### 1 · Bomb_Glow_01 — 1 @ t=0, life 1 s, size 500

- Initialize color `RGBA(1, 0.266356, 0.184475, 0.35)`
- Scale Color: the shared shape above; `Scale Alpha 1`, `Scale RGB (1,1,1)`
- Scale Sprite Size, Uniform Curve: `(0, 0.5)C (0.1, 1)L (1, 1)L`
- Dyn params `[1, 0, 0, 0]`

### 2 · Bomb_Glow_02 — 1 @ t=0, life 1 s, size 230

- Initialize color `RGBA(0.913099, 0.184475, 0.0193824, 0.7)`
- Scale Color: shared shape · Scale Sprite Size: `(0, 0.5)C (0.1, 1)L (1, 1)L`
- Dyn params `[**0**, 0, 0, 0]` — the only glow layer with dissolve 0

### 3 · Bomb_Glow_03 — **3** @ t=**0.05**, life 0.5 s, size 100

- Initialize color `RGBA(1, 0.947307, 0.520996, 0.3)`
- Scale Color: shared shape
- Scale Sprite Size, Uniform Curve: `(0, 0)C (0.2, 1)L (1, 1)L` — grows from **zero**, unlike the
  0.5-start of the other glows
- Dyn params `[1, 0, 0, 0]`
- All three particles are identical: no randomness anywhere in this emitter, so they overlay exactly

### 4 · Raimbow — 1 @ t=0, life 0.5 s, size 250

- Initialize color `RGBA(0.913099, 0.913099, 0.913099, 0.15)`
- Sprite rotation: `Sprite Rotation Mode = Random`, angle **0 … 360**
- A `Color` module IS present in Particle Update but carries no override — its stored pins are
  `Color.Color = RGBA(1,1,1,1)`, `Scale Alpha 1`, `Scale Color (1,1,1)`
- Scale Color: `Scale RGBA = R (0, 0.5)L | G (0, 0.5)L | B (0, 0.5)L | A (0, 1)L (1, 0)L`
  — **RGB is halved for the whole life** (single key at t=0), alpha fades 1 → 0
- Scale Sprite Size, Uniform Curve: `(0, 0.5)C (0.2, 0.9)C (1, 1)L`
- Dyn params `[**0.5**, 0, 0, 0]`

### 5 · Sparkles — **10** @ t=**0.05**, size Random Uniform 7 … 10

- Lifetime `[corpus-v3]`: **`Lifetime Mode = Random` ⇒ `Lifetime Min 0.5 / Max 1.0` DRIVES.** The
  `Random Range Float` override **0.2 … 0.4** sits on the unselected Direct-Set pin and is INERT
  (`lifetimeResolved.source = minmax`, override listed under `inertOverrides`).
  *Was misread as 0.2 … 0.4 under the override-wins assumption — corrected per [P0-D2].*
- Spawn shape: **Sphere Location**, `Sphere Radius **0.5**` (effectively a point),
  `Non Uniform Scale (1,1,1)`, `Offset (0,0,0)`, `Surface Only false`, `Sphere Distribution Random`
- **`Add Velocity from Point` is ENABLED here** (it is DISABLED in NS_PickupLoop's Sparkles):
  `Velocity Strength = Random Range Float 001` **350 … 500**, `Origin Offset (0,0,0)`,
  `Velocity Falloff Distance 100`. With a 0.5-unit spawn sphere the direction is a near-uniform
  random unit vector — an omnidirectional spray at 350–500 u/s.
- Sprite rotation: `Random`, 0 … 360
- Velocity Scale (Vector from Curve, all three axes identical):
  `(0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C`
- Color from Curve:
  - R `(0.615756, 1)C (0.936915, 0)C`
  - G `(0.615756, 0.508881)C (0.936915, 0.0896671)C`
  - B `(0.615756, 0.191202)C (0.936915, 1)C`
  - A `(0.613341, 1)C (1, 0)L`
- Scale Sprite Size, Uniform Curve: `(0, 0)C (0.1, 1)C (1, 0)C`
- `Solve Forces and Velocity`: `Acceleration Limit 9999`, `Speed Limit 1000` (neither binds)
- `Color.Scale Alpha 1`, dyn params `[1, 0, 0, 0]`

### 6 · Ring01 — 1 @ t=0, life 1 s, size 120, `Color.Scale Alpha = 1`

- Initialize color `RGBA(1, 1, 1, 1)`
- Sprite rotation: `Random`, 0 … 360 (no rotation rate)
- Dyn param 1 (`dissolve`) — Float from Curve: `(0, 1)C (1, -1)C`; params 2/3/4 = 0
- Color from Curve:
  - R `(0, 1)L (0.405675, 1)L (1, 1)C`
  - G `(0, 0.912198)L (0.405675, 0.752942)L (1, 0.450786)C`
  - B `(0, 0.753)L (0.405675, 0.304987)L (1, 0.040915)C`
  - A `(0, 1)C` — **single key, alpha holds at 1 for the whole life; the ring does not fade out,
    it dissolves out** (the dyn-param ramp 1 → −1 is the only disappearance mechanism)
- Scale Sprite Size, Uniform Curve: `(0, 0)C (0.2, 0.7)C (1, 1)C`
- Inert pins present in the export: `Lifetime Min 0.3 / Max 0.7`,
  `Uniform Sprite Size Min 150 / Max 160`

### 7 · Flash_Glow_01 — 1 @ t=**0.05**, life **0.2 s**, size **800**

- Initialize color `RGBA(1, 0.916066, 0.467, 0.1)`
- Scale Color: shared shape · Scale Sprite Size: `(0, 0.5)C (0.1, 1)L (1, 1)L`
- Dyn params `[1, 0, 0, 0]`
- The biggest sprite in the system by 1.6× and the second-shortest life — this is the cast "flash"

### 8 · Flash_Glow_02 — 1 @ t=**0.05**, life **0.1 s**, size 150

- Initialize color **`RGBA(3, 1.91279, 0.458779, 1)`** — **an HDR colour with R = 3**, the only
  above-1 channel anywhere in this batch. `O.Color` must not be saturated before it reaches the look.
- Scale Color: shared shape · Scale Sprite Size: `(0, 0.5)C (0.1, 1)L (1, 1)L`
- Dyn params `[1, 0, 0, 0]`

### 9 · Star01 — 1 @ t=**0.2**, life 0.3 s, size 40, `Color.Scale Alpha = 1`

- Initialize color `RGBA(1, 0.637597, 0.152926, 0.2)`
- Scale Sprite Size, Uniform Curve: `(0, 0)C (0.4, 1)C (1, 0)C`
- Color from Curve:
  - R `(0, 1)C (0.405675, 1)L (1, 1)C`
  - G `(0, 1)C (0.405675, 0.74985)L (1, 0.450786)C`
  - B `(0, 1)C (0.405675, 0.303)L (1, 0.0409152)C`
  - A `(0, 1)C` — single key, no fade
- Dyn params `[1, 0, 0, 0]`

### 10 · Star02 — 1 @ t=**0.1**, life 0.3 s, size 100, `Color.Scale Alpha = 1`

- Initialize color `RGBA(1, 0.637597, 0.152926, 0.2)`
- Dyn param 1 (`dissolve`) — Float from Curve: `(0, **-0.125**)C (1, -1)C`; params 2/3/4 = 0
- Scale Sprite Size, Uniform Curve: `(0, 0)C (0.4, 1)C (1, 0)C`
- Color from Curve — identical keys to Star01:
  R `(0, 1)C (0.405675, 1)L (1, 1)C` · G `(0, 1)C (0.405675, 0.74985)L (1, 0.450786)C` ·
  B `(0, 1)C (0.405675, 0.303)L (1, 0.0409152)C` · A `(0, 1)C`

### 11 · Ring02 — 1 @ t=0, life 1 s, size 140, `Color.Scale Alpha = **0.2**`

- Initialize color `RGBA(1, 1, 1, 1)`; same material as Ring01, larger and much dimmer
- Sprite rotation: `Random`, 0 … 360
- Dyn param 1 — Float from Curve: `(0, 1)C (1, -1)C` (identical to Ring01)
- Color from Curve — identical keys to Ring01:
  R `(0, 1)L (0.405675, 1)L (1, 1)C` · G `(0, 0.912198)L (0.405675, 0.752942)L (1, 0.450786)C` ·
  B `(0, 0.753)L (0.405675, 0.304987)L (1, 0.040915)C` · A `(0, 1)C`
- Scale Sprite Size, Uniform Curve: `(0, 0)C (0.2, 0.7)C (1, 1)C`; a
  `Non-Uniform Curve Sprite Scale X: | Y:` override is present but **empty** and the mode is Uniform,
  so it is inert

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new burst row is required: loop 2.0 s, lifetime 1.0 s, burst 22 `[corpus-v3]`.** Same shape as
`PS_CkParticles_Template_Slash` (1.0 / 0.5 / 19) and routes the same way.
Per [P0-D3]: loop = the v3 `systemState` loop duration (2.0 s, `Once`); lifetime = max resolved
emitter lifetime (1.0 s); burst = §2 counts (22). *Was loop 1.0 s under the pre-v3 guess.*

Particle lifetime for the row must be **1.0 s** — the longest source lifetime (Bomb_Glow_01/02,
Ring01, Ring02, and Sparkles' resolved 1.0 s max). Every shorter layer zeroes its colour, size and scale past its own lifetime, exactly
as `Behavior_Slash` does (NS_BasicAttack §8). Spawn delays (0.05 s on three emitters, 0.1 s on two,
0.2 s on one) are handled the same way NS_BasicAttack §5 handles its 0.06 s spark delay: hide the
layer for `age < delay` and run its curves on `(age − delay) / lifetime`.

Loop duration resolved — see §2's cadence block.

### 6.2 VisTag / renderer needs

**All eleven layers draw camera-facing sprites, through eight distinct materials.** VisTag 0 binds
ONE material via `User.SpriteMaterial`; VisTags 1–4 are the wrong renderer kinds. Row-level renderer
overrides today offer `Mesh` and `VelocityAlignedSprite` only.

⇒ **8 row-declared camera-facing sprite renderers** are needed, which requires a new
`FCk_ParticlesRendererSpec` kind. See §6.5, gap 1. VisTag ids: allocate at implementation time above
`Get_RosterVisTag_Max()`; never restate a literal.

### 6.3 Mesh / texture / look needs

- **Meshes: none.**
- **CkUsf looks: 8 new**, all parameterizations of the existing `CkUsf_Look_DissolveAdd` family
  entry point per §4 — except `Rainbow`, which needs a family extension (§6.5, gap 4).
- **Textures: 7 procedural stand-ins**, following the NS_BasicAttack §7 method (measure the corpus
  PNGs, bake from the numbers, never copy pixels):
  - `T_VFX_Part_01` → **`T_CkParticles_SoftParticle` already exists** and was measured off this exact
    asset (NS_BasicAttack §7: perfectly radially symmetric, fits `pow(1-r, 2.2)`). Reuse.
  - `T_VFX_Part_02`, `T_VFX_Part_03` — new; measure first, do not assume they match Part_01.
  - `T_VFX_Ring_01`, `T_VFX_Ring_02` — ring outlines; new bakes (`T_CkParticles_Ring` exists as an
    SDF ring but was not measured off these).
  - `T_VFX_Star_01`, `T_VFX_Star_02` — new (`T_CkParticles_Flare` exists; measure first).
  - `T_VFX_Noise_02` → existing `T_CkParticles_TileNoise` (NS_BasicAttack §7 already made that call).
  - `T_VFX_LUT_Rainbow_01` — a 512×2 **colour** LUT, unlike every greyscale mask in the library.
    Procedurally regenerable (a hue sweep) rather than measurable in the usual sense, but it is a new
    kind of asset for the generator: `sRGB: true`, `TC_Default`, non-square. See §6.5, gap 4.

### 6.4 Behavior id

**Do NOT allocate an id in this document.** Take the next free id from `ck::particles::NumBehaviors`
at implementation time and bump it. This batch contains five planned effects; whoever implements
second must re-read the roster, not this sheet.

Layer partition: `Seed % 22` over a 22-particle burst gives the source's partition by construction
(NS_BasicAttack lesson 2 — a modulo, never `Rand(Seed) < k` probability bands). Assignment:
0 = Bomb_Glow_01, 1 = Bomb_Glow_02, 2–4 = Bomb_Glow_03, 5 = Raimbow, 6–15 = Sparkles, 16 = Ring01,
17 = Flash_Glow_01, 18 = Flash_Glow_02, 19 = Star01, 20 = Star02, 21 = Ring02.

### 6.5 CAPABILITY GAPS — what the pipeline cannot express today

Conservative list. Each is a real blocker or a real approximation.

1. **No row-level camera-facing-sprite renderer.** `FCk_ParticlesRendererSpec` supports `Mesh` and
   `VelocityAlignedSprite` only; this effect needs eight camera-facing sprite renderers with eight
   looks. **Blocking.** The fix is additive and mirrors `VelocityAlignedSprite` exactly (sprite
   renderer, `Alignment = Unaligned`, `Facing = FaceCamera`, look bound through
   `bOverrideMaterials`), plus a VisTag band. This is the same gap in all five effects in this batch.
2. **Per-particle sprite rotation on a camera-facing sprite is unconfirmed.** Four layers
   (Raimbow, Sparkles, Ring01, Ring02) set `Sprite Rotation Mode = Random` over 0–360°. The DI writes
   `OutRotation`, but `CkParticles/CLAUDE.md` documents `Rotation` as applying on **VisTag 2** (smoke)
   and says nothing about VisTag 0.
   `[unresolved: whether the shared camera-sprite renderer binds `Particles.SpriteRotation`.]`
   If it does not, the new renderer kind from gap 1 must. A missing random rotation on Ring01/Ring02
   is directly visible (two identical concentric rings locked in phase).
3. **HDR particle colour.** Flash_Glow_02 initializes at `RGBA(3, 1.91279, 0.458779, 1)`. Any
   `saturate()` on the behavior's colour output — or a look that clamps `ParticleColor.rgb` — silently
   loses a 3× overbright flash. Verify the DI's colour path and `CkUsf_Look_DissolveAdd` do not clamp;
   this is a correctness question, not a capability one, but it fails silently.
4. **The gradient-map (LUT) chain is not implemented in `CkUsf_Look_DissolveAdd`.**
   `M_VFX_DisAdd_Rainbow` is the first instance in the cookbook where it is live:
   `GradientMap_Tex = T_VFX_LUT_Rainbow_01` (512×2 colour LUT), `GradientMap_Displacement 0.9`,
   `GradientShape_Tex = T_VFX_Part_01`, `Gradient_Invert 2`. Both shipped recipes explicitly dropped
   this chain as a no-op *because their GradientMap was a white pixel* (NS_Lightning_Range §13.3,
   NS_BasicAttack §13.4). That justification does not hold here. Options: extend the family shader
   with the LUT sample (a `GradientTex` + displacement + invert), or record the Raimbow layer as a
   deliberate fidelity loss and ship it as a plain white-ish disc. **Do not silently inherit the
   old "it's a no-op" note.**
5. **World space vs the template's local space.** All eleven emitters are `LocalSpace: false`; the
   CkParticles template is local-space. Same known deviation NS_BasicAttack §13.2 records — visible
   only if the spawning actor moves during the 1.0 s life. For a one-shot pickup *cast* at a fixed
   world point this is low-risk.
6. **Material parameters not plumbed through the family look**: `Core_Intensity` (0 on six of eight),
   `Core_Power`, `Glow_Intensity` (**0.3** on Part02), `Gradient_Invert` (0.5 on four, **2** on
   Rainbow), `Opacty_StepAdd` (**0.3** on Rainbow), `CamOffset` (**50** on Part03_Bright — a
   camera-facing depth offset, which CkUsf has no equivalent for), and `Opacty_DepthFade` (20 on six).
   `DepthFade` is a pre-existing documented CkUsf gap. `Glow_Intensity 0.3` and `CamOffset 50` are
   new and each is plausibly visible.

**Not gaps — confirmed absent from this source, so nothing to build:** no ribbon renderers, no mesh
renderers, no light renderers, no sub-UV flipbooks, no GPU sims, no collision, no event handlers, no
user parameters, no material-binding indirection, no distortion (`Distortion_Intensity` 0 on all
eight materials).

---

## 7+. Reserved for implementation.
