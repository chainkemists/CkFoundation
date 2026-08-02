# Recipe: NS_PickupCast → CkParticles (IMPLEMENTED)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: IMPLEMENTATION-COMPLETE (2026-08-02) — behavior 30. Human A/B parity NOT yet judged.**

`Behavior_PickupCast.ush` + `ExecuteStage_CPU` case 30, the `PS_CkParticles_Template_PickupCast`
cadence row with eight row renderers on VisTags 97–104, `Test_Particles_PickupCastBehavior.cpp`, and
the **PICKUP CAST** pair in the VfxExamples gym all exist. This port adds **no** look and **no**
texture: all eight of its DissolveAdd instances were already carried by an earlier batch. §12's walk
is `[HUMAN-VERIFY]` and open.

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

**A new burst row is required: loop 2.0 s, lifetime 1.05 s, burst 22 `[corpus-v3]`.** Same shape as
`PS_CkParticles_Template_Slash` (1.0 / 0.5 / 19) and routes the same way.
Per [P0-D3]: loop = the v3 `systemState` loop duration (2.0 s, `Once`); lifetime = max over layers of
(spawn delay + resolved lifetime) per [P0-D5]; burst = §2 counts (22). *Was loop 1.0 s under the
pre-v3 guess.*

> **Correction applied at implementation (2026-08-02):** this section originally derived the row's
> particle lifetime as **1.0 s**, the longest resolved emitter lifetime taken on its own. [P0-D5]
> resolves lifetime as `max(spawn delay + resolved lifetime)`, and `Sparkles` bursts at **0.05 s**
> with a resolved maximum of **1.0 s**, so the row is **1.05 s**. Same arithmetic-only shape as the
> corrections batches B and C ratified: the itemization was right and the derived number was not.
> At 1.0 s the last 50 ms of every sparkle would have been culled by the template rather than by the
> layer.

Every shorter layer zeroes its colour, size and scale past its own lifetime, exactly
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

## 7. Textures — no new bake

§6.3 asked for measurement and up to seven new bakes. **None were needed:** every paint this system
uses had already been measured off the same corpus PNG by an earlier batch, and the whole eight-material
set resolves through looks that already exist.

| Source paint | Stand-in | Measured in |
|---|---|---|
| `T_VFX_Part_01` | `SoftParticle` | NS_BasicAttack §7 |
| `T_VFX_Part_02` | `SoftParticleBright` | NS_FireBall_Hit §7 |
| `T_VFX_Part_03` | `SoftParticleFine` | NS_FireBall_Hit §7 |
| `T_VFX_Ring_01` | `RingUneven` | NS_FireBall_Hit §7 (the SDF `Ring` bake was measured and rejected there) |
| `T_VFX_Ring_02` | `RingFlare` | NS_FireBall_Hit §7 |
| `T_VFX_Star_01` | `StarFour` | NS_FireBall_Hit §7 |
| `T_VFX_Star_02` | `StarFourTight` | NS_Arrow_Cast §7 |
| `T_VFX_Noise_02` | `TileNoise` | NS_BasicAttack §7 |
| `T_VFX_LUT_Rainbow_01` | `LutWhite` | Phase 1 C3 — held back by [P1-D1], see §13 |
| `T_VFX_WhitePixel` | `LutWhite` | Phase 1 C3 |

---

## 8. Mesh

**None.** All eleven source renderers are camera-facing sprites — the only port in the Cast batch with
no mesh and no flipbook.

---

## 9. The behavior — `Behavior_PickupCast.ush` + `ExecuteStage_CPU` case 30

### 9.1 The cadence row

| Field | Value | Source |
|---|---|---|
| `LoopDuration` | 2.0 s | the SYSTEM's `Loop Behavior = Once`, `Loop Duration = 2` ([P0-D1]) |
| `ParticleLifetime` | 1.05 s | Sparkles' 0.05 s beat + its resolved 1.0 s maximum ([P0-D5]) |
| `BurstCount` | 22 | 1+1+3+1+10+1+1+1+1+1+1, the exact per-emitter counts |
| `SpawnRate` | 0 | there is no `Spawn Rate` module anywhere in the system |

**This is the batch's only burst-ONLY port**, which is what makes its partition exact rather than
statistical: `Seed % 22` reproduces the source's per-emitter counts by construction, where the two
siblings' streamed layers can only match in expectation.

### 9.2 The partition

`Seed % 22`, in the source's own emitter order: 0 Bomb_Glow_01, 1 Bomb_Glow_02, 2–4 Bomb_Glow_03,
5 Raimbow, 6–15 Sparkles, 16 Ring01, 17 Flash_Glow_01, 18 Flash_Glow_02, 19 Star01, 20 Star02,
21 Ring02.

### 9.3 Per-layer notes worth the reader's time

- **Four spawn beats, not one.** Bomb_Glow_01/02, Raimbow and both rings fire at loop start;
  Bomb_Glow_03, Sparkles and both flashes at **0.05 s**; Star02 at **0.1 s**; Star01 at **0.2 s**.
  Each layer hides for `Age < delay` and runs its curves on `(Age − delay) / life` — the
  `Behavior_Slash` idiom (NS_BasicAttack §5).
- **`Flash_Glow_02` is the cookbook's only HDR layer.** Its Initialize colour is
  `RGBA(3, 1.91279, 0.458779, 1)`. Nothing on the output path saturates it, and §11 asserts the 3
  survives — a `saturate()` added anywhere would cost a 3× overbright flash and nothing else would
  look wrong.
- **`Raimbow`'s Initialize colour never shows.** A live `Color` module writes `RGBA(1,1,1,1)` over
  the 0.913 grey and the Scale Color module behind it halves the RGB, so the layer is a flat 0.5
  grey. The same reading NS_Arrow_Cast §5 records for its own Raimbow, and the corpus confirms the
  module is present and enabled with its default white pin.
- **The two rings never FADE.** Both carry a single alpha key, so the dissolve channel sliding
  `1 → −1` is their only disappearance mechanism. `Ring02` differs from `Ring01` in exactly two
  things: 140 units instead of 120, and a `Color.Scale Alpha` of **0.2** against `Ring01`'s 1.
- **`Bomb_Glow_03`'s three particles are identical.** The source emitter carries no randomness at
  all, so all three overlay exactly; the behavior gives them no per-Seed draw either, and §11 asserts
  the overlay.
- **`Sparkles` hold WHITE for three fifths of their life.** Their colour curve's first key sits at
  `t = 0.6158`, and a clamped-key lerp holds the first key's value before it — so the crossover to
  blue happens late rather than immediately.

---

## 10. Looks and renderers

Eight row-declared renderers on VisTags **97–104**, every one a `CameraFacingSprite` — the source has
no mesh renderer, no ribbon, no sub-UV and no velocity-aligned quad.

| VisTag | Look | Source material | Serves |
|---|---|---|---|
| 97 | `PartDisAdd01` | `M_VFX_DisAdd_Part01` | Bomb_Glow_01, Bomb_Glow_02, Flash_Glow_01 |
| 98 | `PartDisAdd02` | `M_VFX_DisAdd_Part02` | Bomb_Glow_03 |
| 99 | `RainbowDisAdd` | `M_VFX_DisAdd_Rainbow` | Raimbow |
| 100 | `PartDisAdd01Bright` | `M_VFX_DisAdd_Part01_Bright` | Sparkles |
| 101 | `RingDisAdd01` | `M_VFX_DisAdd_Ring01` | Ring01, Ring02 |
| 102 | `PartDisAdd03Bright` | `M_VFX_DisAdd_Part03_Bright` | Flash_Glow_02 |
| 103 | `StarDisAdd01` | `M_VFX_DisAdd_Star01` | Star01 |
| 104 | `StarDisAdd02` | `M_VFX_DisAdd_Star02` | Star02 |

**This port authors no look of its own.** §4's delta table was checked value-by-value against each
existing look's defaults before reuse, and all eight matched — including the two that quote a
different family REFERENCE (§4 measures deltas against `M_VFX_DisAdd_Ring04`, the hit/cast batches
against `M_VFX_DisAdd_Part01`), because the ABSOLUTE values agree.

`Get_BehaviorLookName(30)` stays `NAME_None`: every look rides a row renderer that binds it explicitly.

---

## 11. Tests

`Test_Particles_PickupCastBehavior.cpp` + the `NumBehaviors` 30 → 33 ratchet in
`Test_Particles_RosterSanity.cpp`.

- **The partition is asserted as an exact modulo**, slot by slot: every one of the 22 residue classes
  belongs to exactly one source emitter and resolves to that emitter's renderer. A probability-band
  partition would pass a share test and fail this one.
- **Every delayed layer is hidden a millisecond before its own beat and visible after it** — the
  cheapest way to get this port wrong is to start all eleven layers on frame 0.
- **`Flash_Glow_02`'s peak red is asserted to be exactly 3** — the HDR claim, made falsifiable.
- **Both rings hold their alpha across the whole life while their dissolve runs 1 → −1**, and Ring02
  is asserted to be exactly a fifth of Ring01's opacity.
- **`Bomb_Glow_03`'s three slots are asserted identical** in size and position.
- **Sparkles are asserted WHITE inside their curve's clamped head** (R = 1, B < 0.25).
- Plus the standard per-layer anti-vacuity and death checks.

---

## 12. Verification — A/B protocol

`[HUMAN-VERIFY]` — **not yet run.** Open the **VfxExamples** gym, station pair **PICKUP CAST**.
`NS_PickupCast` is a `Loop Once` system, so the harness re-arms both sides on completion and the two
pedestals replay in sync from t = 0. Use `Ck_GymVfxExamples_RestartAll` to re-fire them together.

| # | Criterion | Look for |
|---|---|---|
| a | Overall read | a warm orange-red pop with a white core, two expanding rings and a brief spray of motes |
| b | Beat structure | the body glows and rings open first; 50 ms later the flash, the pips and the sparkles; the two stars arrive last (0.1 s and 0.2 s) |
| c | The flash | an **800-unit** pale-yellow sheet — the biggest sprite in the system — gone inside 0.2 s |
| d | The HDR pop | a small very bright pip at 0.05–0.15 s. If it reads the same brightness as the others, the 3× red has been clamped (§9.3) |
| e | Rings | two concentric rings, one at 120 and one at 140 units, the outer one much dimmer. They should ERODE away rather than fade |
| f | Ring phase | both rings take a random rotation, so they must NOT sit in identical phase |
| g | Sparkles | ten motes thrown omnidirectionally at 350–500 u/s, white at first and turning blue late in their flight |
| h | Stars | a small four-point star at 0.2 s and a larger, differently-painted one at 0.1 s |
| i | Rainbow layer | a flat mid-grey lens ring, NOT a rainbow — held back by [P1-D1] (§13.1) |
| j | World space | move the pedestal mid-effect if you can (§13.4) |

---

## 13. Confirmed fidelity differences

1. **The `Raimbow` layer is grey, not a rainbow.** `M_VFX_DisAdd_Rainbow` is the family's one live
   gradient-map chain, and `RainbowDisAdd` ships against the flat white ramp pending [P1-D1] — the
   family's `Gradient_Invert` remap is not recoverable from the corpus. §6.5 gap 4's warning stands
   and is DISCHARGED as a recorded loss rather than silently inherited: the chain is a provable
   multiply-by-one against a white ramp, so the layer renders exactly as it would with no chain at
   all. This is the same hold NS_FireBall_Hit §13 records.
2. **Unplumbed family parameters.** `Core_Intensity` (0 on six of eight), `Core_Power`,
   `Opacty_StepAdd` (0.3 on Rainbow), `Opacty_DepthFade` (10–20), and **`CamOffset 50`** on
   `Part03_Bright` — a camera-ward world-position push that CkUsf has no equivalent for. Omitting it
   changes only that one layer's depth sorting against the rest. `Glow_Intensity` IS reproduced,
   folded into Brightness (`PartDisAdd02` ships 0.3).
3. **`Distortion_Intensity` is 0 on all eight materials**, so the whole distortion branch is dead in
   this system — the looks carry the parameter but nothing drives it.
4. **World space.** All eleven source emitters are `LocalSpace: false`; the template is local space.
   Same recorded deviation as NS_BasicAttack §13.2 — visible only if the spawner moves during the
   1.05 s life, which a one-shot pickup cast at a fixed point does not.
5. **Salt reuse between `CkParticles_RandDir` and the lifetime/speed draws.** `RandDir` consumes
   salts 1 and 2, which are also the layer's lifetime and speed draws, so a sparkle's direction is
   correlated with how long and how fast it flies. Every shipped port in the cookbook has this
   property; it is recorded once here rather than treated as new.

---

## 14. Reusable lessons

1. **A burst-only source is the cheapest kind to port, and the strictest to test.** With no rate
   stack there is no draw and no share — the modulo IS the source's per-emitter counts, so the test
   can assert slot-by-slot exactness instead of a tolerance. Reach for that assertion whenever a row
   declares `SpawnRate 0`.
2. **Check a candidate look against ABSOLUTE values, not against the delta table's arithmetic.**
   This sheet's §4 measures deltas from `M_VFX_DisAdd_Ring04`; the hit and cast batches measured
   theirs from `M_VFX_DisAdd_Part01`. Two different references cannot be compared row by row — only
   the resolved absolutes can, and all eight matched once resolved.
3. **"A Color module is present but carries no override" means the module RUNS with its default
   pin.** For `Raimbow` that pin is white, so it overwrites the Initialize colour entirely. Reading
   it as inert would have shipped a 0.913 grey where the source draws 0.5.
4. **[P0-D5] is not optional arithmetic.** A row whose lifetime ignores a layer's spawn delay culls
   the tail of that layer with the template rather than with the behavior, and nothing in the code
   says so — the particle simply stops existing. Recompute `max(delay + life)` every time.
