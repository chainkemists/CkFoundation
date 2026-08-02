# Recipe: NS_PickupLoop → CkParticles (PLANNED)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: IMPLEMENTATION-COMPLETE (2026-08-02) as CkParticles behavior 26, `PickupLoop`.
NOT visually verified — the §12 human A/B has not been run.**

§1–6 are archaeology against the extracted corpus, re-verified against the v3 sidecar at implementation
time. **One reading was found WRONG and corrected in place:** §5's Flares layer claimed the Scale Color
module multiplies its two alpha terms and warned that missing that makes the flares ~8x too bright. The
corpus says `Scale Mode = RGB and Alpha Separately` on all four Loop systems' Flares, which means the
SEPARATE Scale Alpha curve is the only alpha channel and the RGBA curve's own alpha is inert — the peak is
0.125, not the ~0.1 product, and the warning pointed the wrong way. Its Colour Mode line was also
incomplete: the corpus sets all FOUR adjust flags, not just `AdjustHue`.

§6.1's `[P0-D3 STOP]` is RESOLVED: campaign decision **[P0-D4]** routed the four rate-only Loop systems
through Phase 2's C2 spawn-rate rows, and route **(a)** — one continuous stream with in-behavior
sub-lifetimes — is what shipped, with the rate carried as row data exactly as §6.1 recommended.

§7 onward is what was actually built.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_PickupLoop` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Behavior ID | **not allocated** — take the next free id at implementation time from `ck::particles::NumBehaviors` |
| CkUsf looks | none yet |

Corpus evidence (regenerate per [README.md](README.md); `Saved/` is machine-local):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_PickupLoop.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part01_Bright,Part02,Ring03,Star01,Star02}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_Ring04.json` (the family reference this
  sheet diffs against — it is the instance `RingDissolveAdd` already implements)
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Ring_01,Star_01,Star_02,Noise_02}.json`

**The source Niagara asset was never opened in the Niagara editor.** Every fact below is `[corpus]`
unless tagged otherwise.

> ### Sibling in the pack — NOT a name collision here
> `[corpus]` The pack's `Anime_Stylized_VFX` branch ships a similar effect, but it is named
> **`NS_Pickup_Loop`** (with underscores) at `Vefects/Anime_Stylized_VFX/VFX/Particles/`, so there is
> no exact-name ambiguity for this file — unlike the three Lightning systems in this batch.
> The fastest discriminator if you ever land in the wrong one: the sibling has a **non-empty user
> parameter list** (10 entries including `User.Scale Overall`) and renders through
> `MI_VFX_*` instances; **this** system has an **empty `userParameters` array** and renders through
> `M_VFX_DisAdd_*` instances.

---

## 2. System anatomy `[corpus]`

**9 CPU emitters, all enabled, all `LocalSpace: false` (WORLD space), all `Determinism: false`,
all `Bounds: Dynamic`. Every emitter spawns through `Spawn Rate` — this system has NO burst at all.
Every emitter renders one camera-facing sprite (`Alignment: Unaligned`, `Facing: FaceCamera`,
`Sort: None`).** No mesh renderers, no ribbons, no sub-UV, no light renderers, no events, no GPU sims.
`userParameters` is **empty**.

| Emitter | Spawn Rate (/s) | Lifetime (s) | Sprite size (effective) | Scale Alpha | Dyn param 1 | Material |
|---|---|---|---|---|---|---|
| Bomb_Glow_01 | 2 | 2 | Uniform **220** | 0.5 | 1 | `M_VFX_DisAdd_Part01` |
| Bomb_Glow_02 | 2 | 1 | Uniform **220** | 0.5 | 1 | `M_VFX_DisAdd_Part01` |
| Bomb_Glow_03 | 4 | 1 | Uniform **350** | 1 | 1 | `M_VFX_DisAdd_Part01` |
| Bomb_Glow_04 | 4 | 1 | Uniform **100** | 0.3 | 1 | `M_VFX_DisAdd_Part02` |
| Sparkles | 5 | rand **0.6 … 1.0** `[corpus-v3]` | Random Uniform **7 … 10** | 1 | 1 | `M_VFX_DisAdd_Part01_Bright` |
| Ring01 | 0.5 | **4** | Uniform **160** | 0.25 | curve (§5) | `M_VFX_DisAdd_Ring03` |
| Star01 | 2 | 1 | Uniform **40** | 1 | 1 | `M_VFX_DisAdd_Star01` |
| Star02 | 2 | 0.8 | Uniform **80** | 1 | **0.745454** | `M_VFX_DisAdd_Star02` |
| Flares | 6 | rand **1.0 … 2.0** `[corpus-v3]` | Random Uniform **50 … 200** | via curve | **8** | `M_VFX_DisAdd_Part02` |

Dynamic material parameters 2, 3 and 4 are **0 on every emitter**; only `Write Parameter Index 0` is
true anywhere.

**Steady-state particle count** `[inferred, arithmetic on the table]`: Σ(rate × mean lifetime) ≈
4 + 2 + 4 + 4 + 4 + 2 + 2 + 1.6 + 9 ≈ **33 live particles** at any instant, using the `[corpus-v3]`
resolved Min/Max lifetimes for Sparkles (0.6…1.0) and Flares (1.0…2.0).
*Was ≈ 23 under the override-wins assumption (0.2…0.4 on both) — corrected per [P0-D2].*

**Cadence caveat — the "Loop Duration = 1" rows are inert here.** `[corpus]` All 9 emitters run
`Life Cycle Mode = System`, which means the emitter's own `Loop Behavior` / `Loop Duration` are
driven by the system, not by the emitter. Every emitter nevertheless stores
`Loop Behavior = Infinite`, `Loop Duration Mode = Fixed`, `Loop Duration = 1`, `Loop Delay = 0`,
`UseLoopDelay = false`.

**System loop `[corpus-v3]`: `Loop Behavior = Infinite`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
Authority per [P0-D1]; the emitter "Infinite / 1.0 s" rows are inert leftovers.
*(Was `[unresolved]` with 1.0 s as the working figure.)* For a pure spawn-rate system the loop
duration does not gate spawning at all — it only wraps emitter Age — so the correction is low-risk
**for this effect specifically**.

---

## 3. Mesh geometry

**N/A — no mesh renderers.** All nine renderers are `NiagaraSpriteRendererProperties`.

---

## 4. Material family + delta table `[corpus]`

All six materials are instances of ONE parent, `/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd`
— the same family `M_VFX_DisAdd_Ring04` (NS_Lightning_Range) and the `M_VFX_DisAdd_Slash*` set
(NS_BasicAttack) came from, and the family the CkUsf `RingDissolveAdd` / `SlashDisAdd*` looks already
implement via `CkUsf_Look_DissolveAdd` in `/CkUsf/Looks/DissolveAdd.ush`.

Every one of the six: `MD_Surface`, `BLEND_Translucent`, `MSM_Unlit`, `twoSided: false`, connected
outputs `EmissiveColor` + `Opacity` only, dynamic-parameter channel names **`dissolve`, `distortion`,
`offset`, `core_color`**, and an expression histogram identical to the family reference
(`ScalarParameter ×41`, `DynamicParameter ×8`, `Multiply/Add/AppendVector ×18` each, `Saturate ×12`,
`Panner ×5`, `TextureSampleParameter2D ×6`, `ParticleColor ×1`, `DepthFade ×1`, `SmoothStep ×1`,
`StaticSwitch ×1`).

Deltas versus the `M_VFX_DisAdd_Ring04` reference (reference values: `Brightness 30`,
`Color_CoreDifferent 1`, `Core_Intensity 1`, `Core_Power 1`, `Glow_Intensity 1`,
`Opacity_Boldness 1`, `Opacty_DepthFade 10`, `Opacty_StepAdd 0.1`, `Gradient_Invert 0`,
`GradientMap_Displacement 0.1`, `Dissolve_Speed_X/Y 0.2`, `Distortion_Scale_X/Y 0.1`,
`Distortion_Intensity 0`, `Dissolve 0`, `MainTex_Scale_X/Y 1`, all `*_Speed`/`*_Offset` 0,
`Color_Core RGBA(1,1,1,0)`, `GradientMap_Tex T_VFX_WhitePixel`, `GradientShape_Tex T_VFX_Noise_02`):

| Material | Main_Tex / Color_Tex | Dissolve_Tex | Distortion_Tex | Brightness | Other deltas |
|---|---|---|---|---|---|
| `Part01` | `T_VFX_Part_01` | `T_VFX_Part_01` | `T_VFX_Noise_02` | **1** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Gradient_Invert 0.5`; `Opacity_Boldness 0.5`; `Opacty_DepthFade 20` |
| `Part01_Bright` | `T_VFX_Part_02` | `T_VFX_Part_02` | `T_VFX_Noise_02` | **10** | `Color_CoreDifferent 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; `Gradient_Invert 0.5`; `Opacty_DepthFade 20` |
| `Part02` | `T_VFX_Part_02` | `T_VFX_Part_02` | `T_VFX_Noise_02` | **1** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1`; **`Glow_Intensity 0.3`**; `Gradient_Invert 0.5`; `Opacity_Boldness 0.5`; `Opacty_DepthFade 20` |
| `Ring03` | `T_VFX_Ring_01` | *(reference `T_VFX_Noise_04` — NOT overridden)* | `T_VFX_Noise_02` | **10** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; **`Distortion_Intensity 0.5`**; `Distortion_Scale_X/Y 1`; **`Distortion_Speed_X/Y 0.1`**; `Opacty_DepthFade 20` |
| `Star01` | `T_VFX_Star_01` | `T_VFX_Star_01` | `T_VFX_Noise_02` | **6** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1` |
| `Star02` | `T_VFX_Star_02` | `T_VFX_Star_02` | `T_VFX_Noise_02` | **6** | `Color_CoreDifferent 0`; `Core_Intensity 0`; `Dissolve_Speed_X/Y 0`; `Distortion_Scale_X/Y 1` |

Two facts that shape the shader work:

1. **`Ring03` is the only instance in this system with a live distortion branch**
   (`Distortion_Intensity 0.5`, `Distortion_Speed 0.1/0.1`). Every other instance leaves
   `Distortion_Intensity` at the family default **0**, i.e. distortion is dead on 5 of 6.
2. **`Ring03` is also the only instance that does NOT override `Dissolve_Tex`**, so it inherits the
   parent's `T_VFX_Noise_04`. On all five others `Dissolve_Tex == Main_Tex` — the shape erodes
   against itself.

Referenced textures `[corpus]` (all `Texture2D`, `sRGB: false`, `TC_Alpha`, `TEXTUREGROUP_World`,
512×512 — greyscale masks, so Particle Color does all the tinting):

| Texture | Source format | Address | Used by |
|---|---|---|---|
| `T_VFX_Part_01` | `TSF_G8` | `TA_Clamp` / `TA_Clamp` | Part01 (main+dissolve) |
| `T_VFX_Part_02` | `TSF_G8` | `TA_Clamp` / `TA_Clamp` | Part01_Bright, Part02 (main+dissolve) |
| `T_VFX_Ring_01` | `TSF_G16` | `TA_Wrap` / `TA_Wrap` | Ring03 (main) |
| `T_VFX_Star_01` | `TSF_G16` | `TA_Wrap` / `TA_Wrap` | Star01 |
| `T_VFX_Star_02` | `TSF_G16` | `TA_Wrap` / `TA_Wrap` | Star02 |
| `T_VFX_Noise_02` | `TSF_G16` | `TA_Wrap` / `TA_Wrap` | Distortion_Tex on 5 of 6 (dead branch on all but Ring03) + `GradientShape_Tex` on all 6 |
| `T_VFX_Noise_04` | `TSF_G16` | `TA_Wrap` / `TA_Wrap` | Ring03 dissolve (inherited default) |
| `T_VFX_WhitePixel` | `TSF_RGBA16`, 1×1, `sRGB: true` | `TA_Wrap` | `GradientMap_Tex` on all 6 — a **no-op gradient map**, same as NS_Lightning_Range §7 |

---

## 5. Per-layer runtime curves `[corpus]`

`t` = NormalizedAge (0 → 1 over that emitter's own lifetime). `C` = constant key, `L` = linear key —
transcribed verbatim from the corpus `[override]` lines.

> **RESOLVED `[corpus-v3]` — Sparkles and Flares read `Lifetime Min`/`Lifetime Max`.** Both emitters
> are `Lifetime Mode = Random`, so per [P0-D2] the Min/Max pins DRIVE and the
> `[override] Lifetime = dyn:Random Range Float` sitting on the unselected Direct-Set pin is INERT
> (`lifetimeResolved.source = minmax`, override under `inertOverrides`).
> *Was read as the override (0.2 … 0.4 on both) under the override-wins assumption.*

### Bomb_Glow_01 — rate 2/s, lifetime 2 s, size 220, `Color.Scale Alpha = 0.5`

- Initialize color `RGBA(1, 0.266356, 0.184475, 0.35)` (overridden by the update curve)
- Color from Curve:
  - R `(0.237851, 1)C (0.492605, 1)L (0.763055, 1)C`
  - G `(0.237851, 0.708376)C (0.492605, 0.527115)L (0.763055, 0.708376)C`
  - B `(0.237851, 0.0466651)C (0.492605, 0.109462)L (0.763055, 0.0466651)C`
  - A `(0, 0)L (0.231814, 1)C (0.767884, 1)L (1, 0)C`
- Dyn params `[dissolve, distortion, offset, core_color] = [1, 0, 0, 0]` (all static)

### Bomb_Glow_02 — rate 2/s, lifetime 1 s, size 220, `Color.Scale Alpha = 0.5`

- Initialize color `RGBA(1, 0.266356, 0.184475, 0.35)`
- Color from Curve (five keys per channel):
  - R `(0.108663, 1)L (0.237851, 1)C (0.492605, 1)L (0.763055, 1)C (0.924842, 1)L`
  - G `(0.108663, 0.838799)L (0.237851, 0.854993)C (0.492605, 0.637597)L (0.763055, 0.854993)C (0.924842, 0.838799)L`
  - B `(0.108663, 0.296138)L (0.237851, 0.376262)C (0.492605, 0.296138)L (0.763055, 0.376262)C (0.924842, 0.296138)L`
  - A `(0, 0)L (0.171446, 1)C (0.835497, 1)L (1, 0)C`
- Dyn params `[1, 0, 0, 0]`

### Bomb_Glow_03 — rate 4/s, lifetime 1 s, size 350, `Color.Scale Alpha = 1`

- Initialize color `RGBA(1, 0.947307, 0.520996, 0.3)`
- Color from Curve — RGB is **constant**:
  - R `(0, 1)C` · G `(0, 0.266356)C` · B `(0, 0.184475)C`
  - A `(0, 0)L (0.304256, 1)C (0.67371, 1)L (1, 0)C`
- Dyn params `[1, 0, 0, 0]`

### Bomb_Glow_04 — rate 4/s, lifetime 1 s, size 100, `Color.Scale Alpha = 0.3`

- Initialize color `RGBA(1, 0.947307, 0.520996, 0.3)`
- Color from Curve — RGB constant:
  - R `(0, 1)C` · G `(0, 0.947307)C` · B `(0, 0.520996)C`
  - A `(0, 0)L (0.304256, 1)C (0.67371, 1)L (1, 0)C` (identical alpha envelope to Bomb_Glow_03)
- Dyn params `[1, 0, 0, 0]`

### Sparkles — rate 5/s, size Random Uniform 7 … 10, `Color.Scale Alpha = 1`

- Lifetime `[corpus-v3]`: **`Lifetime Min 0.6 / Max 1.0` drives**; the `Random Range Float`
  override 0.2 … 0.4 is inert (see the resolved note above)
- Spawn shape: **Sphere Location**, `Sphere Radius 70`, `Non Uniform Scale (1,1,1)`,
  `Sphere Orientation Axis (1,0,0)`, `Offset (0,0,0)`, `Surface Only false`,
  `Sphere Distribution Random`
- `Add Velocity from Point` is **DISABLED** — its `Random Range Float 001` (350 … 500) is therefore
  inert. **Sparkles have NO initial velocity in this system** (the module that would give it is off).
- Sprite rotation: `Sprite Rotation Mode = Random`, angle **0 … 360**
- Velocity Scale (Vector from Curve, all three axes identical):
  `(0, 1)C (0.2, 0.15)C (1, 3.91223e-08)C` — with zero initial velocity this is a no-op
- Color from Curve:
  - R `(0.615756, 1)C (0.936915, 0)C`
  - G `(0.615756, 0.508881)C (0.936915, 0.0896671)C`
  - B `(0.615756, 0.191202)C (0.936915, 1)C`
  - A `(0.613341, 1)C (1, 0)L`
- Scale Sprite Size, Uniform Curve: `(0, 0)C (0.1, 1)C (0.2, 0.6)C (1, 0)C`
- `Solve Forces and Velocity`: `Acceleration Limit 9999`, `Speed Limit 1000` (neither binds)
- Dyn params `[1, 0, 0, 0]`

### Ring01 — rate 0.5/s, lifetime **4 s**, size 160, `Color.Scale Alpha = 0.25`

- Initialize color `RGBA(1, 1, 1, 1)`
- Sprite rotation: `Random`, angle 0 … 360; **Sprite Rotation Rate** = `Random Range Float`
  **−20 … +20** (deg/s)
- Dyn param 1 (`dissolve`) — Float from Curve: `(0, -1)C (0.5, 0.5)C (1, -1)C`; params 2/3/4 = 0
- Color from Curve:
  - R `(0, 1)L (0.281316, 1)L (0.889828, 1)L`
  - G `(0, 0.947307)L (0.281316, 0.693872)L (0.889828, 0.0612461)L`
  - B `(0, 0.665387)L (0.281316, 0.147027)L (0.889828, 0.00856812)L`
  - A `(0, 0)C (0.261998, 1)L (0.723212, 1)L (1, 0)L`
- Inert pins present in the export (mode is Uniform / Direct Set, so these do NOT apply):
  `Lifetime Min 0.3 / Max 0.7`, `Uniform Sprite Size Min 150 / Max 160`

### Star01 — rate 2/s, lifetime 1 s, size 40, `Color.Scale Alpha = 1`

- Initialize color `RGBA(1, 0.637597, 0.152926, 0.2)`
- Color from Curve:
  - R `(0.141262, 1)C (0.429822, 1)L (0.84757, 1)C`
  - G `(0.141262, 0.947307)C (0.429822, 0.693872)L (0.84757, 0.450786)C`
  - B `(0.141262, 0.665387)C (0.429822, 0.147027)L (0.84757, 0.0409152)C`
  - A `(0, 0)C (0.161787, 1)L (0.839119, 1)L (1, 0)L`
- Dyn params `[1, 0, 0, 0]`
- No size curve — Star01's Particle Update is `Particle State → Dynamic Material Parameters → Color`

### Star02 — rate 2/s, lifetime 0.8 s, size 80, `Color.Scale Alpha = 1`

- Initialize color `RGBA(1, 0.637597, 0.152926, 0.2)`
- Scale Sprite Size, Uniform Curve: `(0, 0.4)C (0.5, 1)C (1, 0.4)C`
- Color from Curve — same shape as Star01, one digit different on the last blue key:
  - R `(0.141262, 1)C (0.429822, 1)L (0.84757, 1)C`
  - G `(0.141262, 0.947307)C (0.429822, 0.693872)L (0.84757, 0.450786)C`
  - B `(0.141262, 0.665387)C (0.429822, 0.147027)L (0.84757, 0.040915)C`
  - A `(0, 0)C (0.161787, 1)L (0.839119, 1)L (1, 0)L`
- Dyn params `[**0.745454**, 0, 0, 0]` — the only non-integer static dissolve in the system

### Flares — rate 6/s, size Random Uniform 50 … 200

- Lifetime `[corpus-v3]`: **`Lifetime Min 1.0 / Max 2.0` drives**; the `Random Range Float`
  override 0.2 … 0.4 is inert (see the resolved note above)
- Spawn shape: **Sphere Location**, `Sphere Radius 100`, `Non Uniform Scale (1,1,1)`, `Offset (0,0,0)`,
  `Surface Only false`
- **`Color Mode = Random Hue/Saturation/Value`** `[corpus]` over the base `RGBA(1, 0.329981, 0, 1)`, with
  **all four adjust flags set** (`AdjustHue`, `AdjustSaturation`, `AdjustValue`, `AdjustAlpha` — the
  earlier reading listed only the first): `Hue Shift Range (0.1, -0.1)`, `Saturation Range (0.35, 0.5)`,
  `Value Range (1, 1)`, `Alpha Scale Range (0.1, 0.2)`, `Color Minimum RGBA(0,0,0,1)`,
  `Color Maximum RGBA(1,1,1,1)`.
  The base is a pure hue (max 1, min 0), so its saturation and value are both 1 — which makes the
  "range replaces" and "range scales" readings of `Saturation Range` / `Value Range` produce identical
  numbers here. The ambiguity is inert for this instance.
- Sprite rotation: `Sprite Rotation Mode = Unset` (no rotation)
- Scale Sprite Size (Uniform Curve mode): uniform `(0, 0)C (0.1, 1)C (1, 0.8)C`;
  the non-uniform curve `X (0,0)L (1,1)L | Y (0,0)L (1,1)L` is present but **inert** under Uniform mode
- **Scale Color** (this emitter has no `Color` module):
  - Scale RGBA (Vector4 from Curve): R `(0, 1)L (1, 1)L` · G `(0, 1)L (1, 1)L` ·
    B `(0, 1)L (1, 1)L` · A `(0, 0)L (0.236644, 1)L (1, 0)L`
  - Scale Alpha (Float from Curve): `(0, 0)L (0.3, 0.125)L (1, 0)L`
  - `Scale RGB (1, 1, 1)`
  - **CORRECTED `[corpus]`** — `Scale Mode = RGB and Alpha Separately`, so the SEPARATE `Scale Alpha`
    curve is the only alpha channel and the Scale-RGBA curve's own alpha is INERT. The envelope peaks at
    **0.125** at t = 0.3, not at the ~0.1 product the earlier reading computed. Multiplying the two, as
    that reading prescribed, darkens the layer by ~20 %. Same mode on all four Loop systems' Flares.
- Dyn params `[**8**, 0, 0, 0]` — by far the largest static dissolve value in the system

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new row is required and it is NOT a burst row.** Every emitter here spawns through
`Spawn Rate`, at nine *different* rates (0.5, 2, 2, 2, 2, 4, 4, 5, 6 per second) and with per-emitter
lifetimes from 0.8 s to 4 s. The cadence table's row shape today is
`(asset name, loop duration, particle lifetime, burst count)` with `count 0` meaning "the continuous
spawn-rate stack" — the continuous stack's **rate is not a row field**, and one row carries exactly
one particle lifetime.

**`[P0-D3 STOP: loop = 2.0 s (system, Infinite/Fixed); lifetime = 4.0 s (max resolved, Ring01);
burst = 0 — rate-only system, every emitter spawns through Spawn Rate and §2 carries no burst
count]`.** The [P0-D3] formula produces a `BurstCount == 0` continuous row, which the cadence table
cannot parameterize today (no per-row spawn rate). Orchestrator ruling required before a row is
written; the two candidate shapes below are unchanged, now with the resolved numbers.

Two viable shapes, both of which need a decision before any HLSL is written:

- **(a) One continuous row + in-behavior sub-lifetimes.** Add a row with count 0 and a particle
  lifetime of **4.0 s** (the longest source lifetime, Ring01) and have each layer zero its
  colour/size past its own lifetime — exactly the mechanism `Behavior_Slash` uses for its
  0.3/0.5/0.06+rand layers. Costs: the effective per-layer rate becomes (template rate ÷ number of
  layers), which is a knob the table does not currently expose, and 4 s of dead template particles
  for the 0.8 s layers.
- **(b) One burst row at the 2.0 s system cycle `[corpus-v3]`** with a burst count matching the
  per-cycle totals (2× the per-second totals, ≈ 55) and per-layer lifetimes handled as in (a).
  *Was stated as a 1.0 s cycle / ≈ 27–28.* This trades the source's *continuous,
  uncorrelated* spawn stream for a synchronized once-per-second pulse. **That is a visible change** —
  this effect's whole identity is a calm, non-pulsing idle loop — so (b) should be rejected unless
  (a) is blocked.

Recommendation: **(a)**, plus adding a `SpawnRate` field to `FCk_ParticlesTemplateSpec` for
`BurstCount == 0` rows. That field does not exist today (see §6.5).

### 6.2 VisTag / renderer needs

**All nine layers draw camera-facing sprites through nine materials in six distinct
parameterizations.** VisTag 0 (the shared camera sprite) binds ONE material via
`User.SpriteMaterial`, so it can carry at most one of the six looks. VisTags 1–4 are the wrong
renderer kinds (velocity-aligned, smoke, mesh, custom-facing). Row-level renderer overrides today
offer exactly two kinds — `Mesh` and `VelocityAlignedSprite` — **neither of which is a camera-facing
sprite**.

⇒ This effect needs **6 row-declared camera-facing sprite renderers** (one per look), which requires
a new `FCk_ParticlesRendererSpec` kind. See §6.5, gap 1. VisTag ids: allocate at implementation time
above `Get_RosterVisTag_Max()`; do not restate a literal.

### 6.3 Mesh / texture / look needs

- **Meshes: none.**
- **CkUsf looks: 6 new**, all parameterizations of the existing `CkUsf_Look_DissolveAdd` family
  entry point — `Part01`, `Part01_Bright`, `Part02`, `Ring03`, `Star01`, `Star02` per §4's table.
  The family already plumbs `ShapeTex`, `DissolveTex`, `DistortTex`, `CoreColor`, `Brightness`,
  `DissolveSpeed`, `DissolveEdge`, `DistortScale`, `OpacityBoldness`, `DissolveSpeedY`,
  `DissolveBias`, `DissolveScale`, `DistortIntensity`, `DistortSpeed`, `MainTexScale`. Every §4
  delta lands in that set **except** `Core_Intensity`, `Core_Power`, `Glow_Intensity`,
  `Gradient_Invert` and `Opacty_DepthFade`, which are not plumbed — see §6.5, gap 6.
- **Textures: 5 procedural stand-ins needed**, following the NS_BasicAttack §7 method (measure the
  corpus PNGs, bake from the numbers, never copy pixels):
  - `T_VFX_Part_01`, `T_VFX_Part_02` — soft round particles. **`T_CkParticles_SoftParticle` already
    exists** and was baked as the stand-in for `T_VFX_Part_01` (NS_BasicAttack §7 measured it as
    perfectly radially symmetric, fitting `pow(1-r, 2.2)`). `T_VFX_Part_02` is a *different* asset and
    needs its own measurement before reusing that bake.
  - `T_VFX_Ring_01` — ring outline; **new bake** (`T_CkParticles_Ring` exists in the library but was
    authored as an SDF ring, not measured off this asset — verify before reusing).
  - `T_VFX_Star_01`, `T_VFX_Star_02` — star/flare shapes; **new bakes** (`T_CkParticles_Flare` exists;
    measure first).
  - `T_VFX_Noise_02` → the existing `T_CkParticles_TileNoise`, per NS_BasicAttack §7.
  - `T_VFX_Noise_04` (Ring03's inherited dissolve) → likely also `T_CkParticles_TileNoise`; measure.

### 6.4 Behavior id

**Do NOT allocate an id in this document.** At implementation time take the next free id from
`ck::particles::NumBehaviors` and bump it; the roster is a single-definition list and two planning
sheets both writing "the next id" is how collisions happen. This batch contains five planned effects
— whoever implements second must re-read the roster, not this sheet.

Layer partition: with a continuous row there is no per-loop burst, so `Seed % 9` (or a weighted
residue table honouring the 0.5 … 6/s rate spread) replaces NS_BasicAttack's `Seed % 19`.
A flat `Seed % 9` would give every layer an equal share and **would not reproduce the source's rate
mix** — a weighted partition over a 55-slot residue (rates × 10: 20/20/40/40/50/5/20/20/60 → 275,
or the reduced 5.5-slot equivalent) is the faithful shape. Decide this before writing HLSL.

### 6.5 CAPABILITY GAPS — what the pipeline cannot express today

Conservative list. Each is a real blocker or a real approximation, not a nice-to-have.

1. **No row-level camera-facing-sprite renderer.** `FCk_ParticlesRendererSpec` supports `Mesh` and
   `VelocityAlignedSprite` only. This effect needs six camera-facing sprite renderers with six
   different looks. **This is the single blocking gap** and it is the same gap in all five effects in
   this batch. The fix is additive and mirrors the existing `VelocityAlignedSprite` kind exactly (a
   sprite renderer with `Alignment = Unaligned`, `Facing = FaceCamera`, look bound through
   `bOverrideMaterials`), plus one more VisTag band. Estimate it once, spend it once.
2. **Spawn rate is not a cadence-row field.** `FCk_ParticlesTemplateSpec` carries loop duration,
   particle lifetime and burst count; `BurstCount == 0` selects "the continuous spawn-rate stack"
   whose rate lives in the builder, not the row. A faithful port of a 9-rate spawn-rate system needs
   the rate as data.
3. **Per-particle sprite rotation on a camera-facing sprite is unconfirmed.** Ring01 needs a random
   initial rotation (0–360°) plus a rotation rate (±20 °/s). The DI writes `OutRotation` (sprite
   degrees) and `CkParticles/CLAUDE.md` documents `Rotation` as applying on **VisTag 2** (the smoke
   sprite) — it does not say the VisTag 0 camera sprite binds `Particles.SpriteRotation`.
   `[unresolved: whether the shared camera-sprite renderer binds SpriteRotation.]` If it does not,
   the new renderer kind from gap 1 must bind it. Check the template builder before promising it.
4. **World space vs the template's local space.** All nine emitters are `LocalSpace: false`; the
   CkParticles template is local-space because self-driving behaviors write absolute positions.
   Same known deviation NS_BasicAttack §13.2 records: visible only if the spawning actor moves during
   a particle's life. For a *pickup idle loop* attached to a moving pickup this is **more likely to
   be visible here than it was for the slash** — worth an explicit decision rather than an inherited
   default.
5. **Two multiplied alpha curves on Flares.** Not a pipeline gap — `O.Color.a` can carry the product —
   but it is the one place in this effect where a "transcribe one curve" reflex produces an ~8×
   brightness error. Recorded here so it is not rediscovered at the visual gate.
6. **Five material parameters are not plumbed through `CkUsf_Look_DissolveAdd`**:
   `Core_Intensity` (0 on 5 of 6 instances), `Core_Power`, `Glow_Intensity` (0.3 on Part02),
   `Gradient_Invert` (0.5 on four instances), `Opacty_DepthFade` (20 on four). `DepthFade` is a
   pre-existing, documented CkUsf gap (NS_Lightning_Range §13.4 — CkUsf surface looks do not wire
   scene depth). The other four are new; each is either a family-shader extension or a recorded
   fidelity difference. **`Glow_Intensity 0.3` on Part02 is the one most likely to be visible** —
   it is a 70 % brightness cut on two of the nine layers.

**Not gaps — confirmed absent from this source, so nothing to build:** no ribbon renderers, no mesh
renderers, no light renderers, no sub-UV flipbooks, no GPU sims, no collision, no event handlers, no
user parameters, no material-binding indirection.

---

## 7. Textures — every candidate MEASURED, and not one new bake

§6.3 asked for five new bakes. By implementation time **all five source paints were already covered** by
bakes measured off the SAME corpus PNGs in earlier batches, so this port adds nothing to the library:

| Source paint | Stand-in | Why it is the same paint, not a lookalike |
|---|---|---|
| `T_VFX_Part_01` | `SoftParticle` | measured off this exact PNG (NS_BasicAttack §7) |
| `T_VFX_Part_02` | `SoftParticleBright` | measured off this exact PNG (NS_FireBall_Hit §7) |
| `T_VFX_Ring_01` | `RingUneven` | measured off this exact PNG (NS_FireBall_Hit §7); §6.3's "verify before reusing `T_CkParticles_Ring`" was answered there — the SDF ring was measured and REJECTED, the source's interior being exactly empty where the SDF's is not |
| `T_VFX_Star_01` | `StarFour` | measured off this exact PNG (NS_FireBall_Hit §7); the generic `Flare` bake was measured and rejected there |
| `T_VFX_Star_02` | `StarFourTight` | measured off this exact PNG (NS_Arrow_Cast §7) |
| `T_VFX_Noise_02` | `TileNoise` | NS_BasicAttack §7 |
| `T_VFX_Noise_04` | `TileNoiseCoarse` | NS_FireBall_Hit §7 |
| `T_VFX_WhitePixel` | `LutWhite` | the family's inert white ramp (C3) |

**Identity, not resemblance, is the reuse argument here.** Every row above names the same source asset an
earlier recipe measured; nothing was reused on a correlation.

---

## 8. Mesh

**None.** All nine source renderers are sprites, and all six of the row's renderers are camera-facing
sprite quads.

---

## 9. The behavior — `Behavior_PickupLoop.ush` + `ExecuteStage_CPU` case 26

### 9.1 The cadence row, and why it is the cookbook's first CONTINUOUS one

| Field | Value | Source |
|---|---|---|
| `LoopDuration` | 2.0 s | the SYSTEM's `Loop Behavior = Infinite`, `Loop Duration = 2` ([P0-D1]) |
| `ParticleLifetime` | 4.0 s | Ring01's resolved Direct-Set lifetime, the longest layer |
| `BurstCount` | 0 | there is no burst module anywhere in the system |
| `SpawnRate` | 27.5 /s | 2+2+4+4+5+0.5+2+2+6, the nine emitters' `SpawnRate` values |

On a rate-only source the loop duration does not gate spawning at all — it only wraps `Emitter.Age` — so
the [P0-D1] correction from the inert emitter rows to the system's 2.0 s is low-risk for this effect
specifically, exactly as §2 predicted.

### 9.2 The partition is a WEIGHTED DRAW, not a modulus

Every prior port slices a per-loop burst with `Seed % N`. That is unavailable here: there is no burst, and
`Ring01` spawns **0.5 particles per second** — 1.82 % of the stream, a share no integer slot count under 55
can express. The behavior instead draws `CkParticles_Rand(Seed, 0)` once, scales it by the total rate, and
walks a cumulative-share cascade in the source's own emitter order. Reproducing the source's rate mix is
then exact by construction rather than by rounding, and adding a layer never re-maps the others.

Measured over 400 000 seeds, the worst per-layer deviation from the source share across all nine layers is
**0.00092** (§11).

### 9.3 Per-layer notes worth the reader's time

- **The four bomb glows** differ only in life, size, palette and the `Color` module's `Scale Alpha`
  (0.5 / 0.5 / 1 / 0.3). Those Scale Alpha values are load-bearing and appear only in §2's table and the
  corpus `[values]` blocks, never in §5; the behavior applies each one on top of its layer's alpha curve.
  Three of the four share one body.
- **Sparkles do not move.** Their `Add Velocity from Point` module is DISABLED, so the 350–500 range and
  the whole Velocity-Scale curve are inert. Implementing the disabled module would turn a calm idle
  shimmer into a spray — the single most tempting wrong thing in this system.
- **Ring01 is the only ANIMATED dissolve in the Loop batch**: −1 → 0.5 → −1 over four seconds, so the ring
  assembles out of nothing, holds, and erodes away. It also carries a per-particle rotation RATE that can
  turn either way (±20 °/s).
- **Star02 carries the system's only non-integer static dissolve** (0.745454).
- **Flares** take the corrected alpha reading of §5 and the full four-flag HSV randomization.

---

## 10. Looks and renderers

Six row-declared **camera-facing sprite** renderers on VisTags **71–76**, one per distinct source material:

| VisTag | Look | Source material | New? |
|---|---|---|---|
| 71 | `PartDisAdd01` | `M_VFX_DisAdd_Part01` | reused (NS_BasicAttack) |
| 72 | `PartDisAdd01Bright` | `M_VFX_DisAdd_Part01_Bright` | reused (NS_FireBall_Hit) |
| 73 | `PartDisAdd02` | `M_VFX_DisAdd_Part02` | reused (NS_FireBall_Hit) |
| 74 | `RingDisAdd03` | `M_VFX_DisAdd_Ring03` | **NEW** — `Script/CkUsf/CkUsf_LoopLooks_Assets.as` |
| 75 | `StarDisAdd01` | `M_VFX_DisAdd_Star01` | reused (NS_FireBall_Hit) |
| 76 | `StarDisAdd02` | `M_VFX_DisAdd_Star02` | reused (NS_Arrow_Cast) |

`RingDisAdd03` is the system's only instance with a LIVE distortion branch (`Distortion_Intensity 0.5`,
`Distortion_Speed 0.1/0.1`, `Distortion_Scale 1`) and the only one that does NOT override `Dissolve_Tex`,
so it erodes against the parent's coarse noise rather than against its own shape. Both facts are in the
look's parameters, and the second is why its `DissolveTex` is `TileNoiseCoarse` while its `ShapeTex` is
`RingUneven`.

`Get_BehaviorLookName(26)` stays `NAME_None`: every look rides a row renderer that binds it explicitly.

---

## 11. Tests

`Test_Particles_PickupLoopBehavior.cpp` + the `NumBehaviors` 26 → 30 ratchet in
`Test_Particles_RosterSanity.cpp`.

The partition is the file's centre of gravity, because a rate-only port fails QUIETLY: a drifted threshold
does not crash, it just gives one source emitter the wrong share of the stream.

- **The rate-share sweep** counts 400 000 seeds through a cascade the test rebuilds from the source rate
  table (not from the behavior's constants) and requires every layer within **0.004** of its source share.
  Observed worst case 0.00092 — a 4.3x margin.
- **Determinism**: the layer a seed draws must not depend on the age it is evaluated at.
- **`CkParticles_Rand` is re-implemented in the test**, deliberately: the 24-bit avalanche IS what the
  partition claims to use.
- **Ring01's animated dissolve** is asserted at all three keys, and it is the only layer still alive at
  3.9 s.
- **Sparkles are asserted STATIONARY** — the disabled-module claim, made falsifiable.
- **Flares' alpha is bounded on BOTH sides**: never above 0.2 × 0.125, and above 90 % of it. The floor is
  what catches the §5 error this recipe corrected — the two-alpha reading tops out near 0.0198 and fails.
- Plus the standard per-layer anti-vacuity and death checks.

---

## 12. Verification — A/B protocol

`[HUMAN-VERIFY]` — **not yet run.** Open the **VfxExamples** gym, station pair **PICKUP LOOP**.

> **This pair is a STEADY-STATE comparison, not a synced replay.** `NS_PickupLoop` is an INFINITE system:
> it never finishes, so the harness's `OnSystemFinished` re-arm never fires and the two sides are never in
> phase. Judge density, palette and motion character over a few seconds; do NOT expect matched frames.
> `Ck_GymVfxExamples_RestartAll` restarts both, but they drift apart again immediately.

| # | Criterion | Look for |
|---|---|---|
| a | Overall read | a calm, non-pulsing idle glow — a hovering pickup. **If either side reads as a metronome, the cadence is wrong**, and that is this effect's whole identity |
| b | Density | roughly 33 live particles at any instant. The ring is the sparsest thing on screen |
| c | Ring | ONE 160-unit halo every two seconds, rotating slowly either way, that assembles out of nothing and erodes away rather than fading |
| d | Bomb glows | four warm shells at 220 / 220 / 350 / 100 units, the two 220s at half coverage and the 100 at a third |
| e | Sparkles | fine motes scattered through a 70-unit ball that **do not move at all** — they twinkle in place and die |
| f | Stars | two four-point stars, the smaller held steady, the larger breathing between 40 % and full size |
| g | Flares | a wide, very faint warm haze, hue-varied particle to particle, peaking around 12 % opacity |
| h | Palette | warm oranges and reds throughout, with the sparkles turning BLUE as they die |
| i | World space | move the pedestal mid-effect if you can. The source is WORLD space and the port is LOCAL (§13.5). For a *pickup* this is the deviation most likely to matter in real use |

---

## 13. Confirmed fidelity differences

1. **The layer partition is a weighted draw, not per-emitter independent spawning.** Nine independent
   Niagara emitters each own their own spawn accumulator; the port has ONE emitter whose particles are
   assigned a layer on draw. Over any window the proportions match exactly, but the arrival times are
   correlated where the source's are independent. `[inferred]` visually equivalent at these rates.
2. **`Ring03`'s live distortion is reproduced; `Opacty_DepthFade 20` is not.** DepthFade is a pre-existing,
   documented CkUsf gap (NS_Lightning_Range §13.4) — CkUsf surface looks do not wire scene depth.
3. **Other unplumbed family parameters:** `Core_Intensity` (0 on five of six instances — inert there),
   `Core_Power`, `Color_CoreDifferent`, and `Gradient_Invert 0.5` on four instances (inert against the
   white ramp). **`Glow_Intensity 0.3` on `Part02` IS reproduced**, folded into Brightness — on an unlit
   additive composite the two are the same emissive scale.
4. **Sprite rotation on camera-facing row renderers** — §6.5 gap 3 asked whether `Particles.SpriteRotation`
   is bound there. It is: the `CameraFacingSprite` kind is documented as spinning the quad in screen space,
   and Ring01 / Sparkles / Flares write `O.Rotation` accordingly.
5. **World space.** All nine source emitters are `LocalSpace: false`; the CkParticles template is local
   space. §6.5 gap 4 flagged this as *more* likely to be visible here than for a one-shot, because a pickup
   loop is attached to something that may move. Unchanged and recorded, not silent.
6. **Every stand-in texture is a statistical match of the source paint, not a copy** (§7).
7. **`In.EmitterAge` is threaded but unread.** Phase 2's C5 input exists for the windowed sub-loops of the
   Cast family; this system has none — every emitter is `Life Cycle Mode = System`, so its stored `Loop`
   rows are inert and there is nothing to gate. The roster-wide emitter-clock independence sweep in
   `Test_Particles_RosterSanity` therefore still holds with this behavior on the roster.

---

## 14. Reusable lessons

1. **A rate-only source cannot be partitioned by a modulus.** `Seed % N` forces every layer onto a multiple
   of 1/N; a 0.5/s layer inside a 27.5/s stream has no such slot. Draw once and walk cumulative shares —
   the mix is then exact, fractional rates cost nothing, and adding a layer does not re-map the others.
2. **A planning sheet's warnings can point the wrong way, and the corpus is the arbiter.** §5's "both alpha
   terms multiply — getting this wrong makes the flares ~8x too bright" was backwards: the module's own
   `Scale Mode` makes one of the two inert. The three sibling sheets read the same module correctly, which
   is what made the outlier visible.
3. **`[values]` blocks carry load-bearing numbers the prose sections omit.** Every `Color.Scale Alpha` in
   this batch lives only there; four of this system's nine layers would have rendered at the wrong
   coverage if the port had trusted §5 alone.
4. **Reuse by IDENTITY beats reuse by correlation.** All five "new bakes needed" turned out to be paints an
   earlier batch had already measured off the same PNG. Check the source asset name against the existing
   recipes' §7 tables before measuring anything.
5. **Make a disabled module's absence falsifiable.** "Sparkles have no velocity" is a claim a test can hold
   (`position(t) == position(0)`); left as a comment it is the kind of thing a later session helpfully
   "fixes".
