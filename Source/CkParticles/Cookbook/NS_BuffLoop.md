# Translation sheet: NS_BuffLoop → CkParticles (PRE-IMPLEMENTATION)

Schema and evidence-tag conventions: [README.md](README.md). Exemplars: [NS_BasicAttack.md](NS_BasicAttack.md),
[NS_Lightning_Range.md](NS_Lightning_Range.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior id is allocated, no `.ush` exists, no cadence row was added, no look was authored, no
asset was generated, nothing was built and nothing was rendered.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_BuffLoop` |
| Pack | Vefects — *Anime VFX* |
| Corpus system files | `systems/Vefects/Anime_VFX/Shared/Skills/NS_BuffLoop.{json,txt}` |
| Corpus material files | `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part01_Bright,Part02,Rainbow,Arrows,Star01,Part04}.json` |
| Corpus texture files | `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Part_04,Arrow_01,Star_01,Ring_02,Noise_02,LUT_Rainbow_01,WhitePixel}.json` |
| Meshes | none |

**The source Niagara asset was never opened.** Every fact below is `[corpus]` unless tagged otherwise.

> ### Sibling with a near-identical name
> `[corpus]` `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Buff_Loop` (underscored) is a different,
> parameterized system. Discriminators: `NS_BuffLoop` exports `userParameters: []` and draws through
> `M_VFX_DisAdd_*`; `NS_Buff_Loop` exports **eight** user parameters (`User.Arrow Color 01`,
> `User.Flares Color`, `User.Glow Color 01`, `User.Rainbow Color 01`, `User.Scale Overall`,
> `User.Sparkles Color 01`, `User.Sparkles Stretched Color 01`, `User.Stars Color 01`) and draws
> through `MI_VFX_*`. This sheet documents the **`Anime_VFX/Shared/Skills`** variant only.

---

## 2. System anatomy `[corpus]`

**9 CPU emitters, ALL enabled, ALL world-space (`LocalSpace: false`), `Bounds: Dynamic`,
`Determinism: false`, zero user parameters.**

**Structurally this is NOT a burst system.** Every one of the nine emitters uses **`Spawn Rate`**
(continuous), Loop Behavior **Infinite**, Loop Duration 1.0 s. There is no `Spawn Burst
Instantaneous` module anywhere in the system — the `Loop Duration = 1` only resets `Emitter.Age`,
it does not gate spawning. This is the fundamental difference from its `NS_BuffCast` sibling and it
drives the whole §6 plan.

| # | Emitter | Spawn Rate (/s) | Lifetime | Renderer / alignment | Material | Size |
|---|---|---|---|---|---|---|
| 0 | `Glow_01` | **2** | 2.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part01` | uniform 500 |
| 1 | `Raimbow` *(sic)* | **1** | 0.5 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Rainbow` | uniform 300 |
| 2 | `Arrow` | **3** | 1.5 | Sprite, **VelocityAligned** / FaceCamera | `M_VFX_DisAdd_Arrows` | non-uniform (80, 130) |
| 3 | `Stars` | **2** | rand `[unresolved]` | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Star01` | rand uniform 40–70 |
| 4 | `Sparkles_Stretched` | **10** | rand `[unresolved]` | Sprite, **VelocityAligned** / FaceCamera | `M_VFX_DisAdd_Part04` | rand non-uniform (35,130)–(50,140) |
| 5 | `Glow_02` | **4** | 1.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part01` | uniform 300 |
| 6 | `Sparkles_01` | **10** | rand `[unresolved]` | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part01_Bright` | rand uniform 10–15 |
| 7 | `Flares` | **6** | rand `[unresolved]` | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part02` | rand uniform 50–200 |
| 8 | `Sparkles_Spiral` | **10** | rand `[unresolved]` | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part01_Bright` | rand uniform 10–15 |

**Total spawn rate 48 particles/second.** Steady-state live count ≈ **36** using the shorter
(`Random Range Float`) lifetime reading, ≈ 40 using the longer one — arithmetic, `[inferred]` from the
rate × lifetime products, not a corpus value.

> ### `[unresolved: the lifetime of every randomised emitter]`
> `Stars`, `Sparkles_Stretched`, `Sparkles_01`, `Sparkles_Spiral` and `Flares` all carry BOTH:
> `Lifetime Mode = Random` with `Initialize Particle.Lifetime Min/Max`, **and**
> `[override] Lifetime = dyn:Random Range Float` whose `RandomRangeFloat Min/Max` differ. Niagara's
> Random lifetime mode reads `Lifetime Min/Max`; the `Lifetime` pin an override drives is the
> **Direct Set** input. So the likely reading is that the module's own Min/Max wins and the dynamic
> input is a leftover `[inferred]` — but the corpus cannot settle it.
>
> | Emitter | `Lifetime Min/Max` (Random mode) | `RandomRangeFloat Min/Max` (override) |
> |---|---|---|
> | `Stars` | 0.3 / 0.6 | 0.2 / 0.4 |
> | `Sparkles_Stretched` | 0.3 / 0.6 | 0.2 / 0.4 |
> | `Sparkles_01` | 0.3 / 0.6 | 0.2 / 0.4 |
> | `Sparkles_Spiral` | 0.3 / 0.6 | 0.2 / 0.4 |
> | `Flares` | **1 / 2** | 0.2 / 0.4 |
>
> `Flares` is the one where the two readings are wildly different (a 1–2 s drifting flare vs a
> 0.2–0.4 s blink) — resolve this one before authoring anything.

**Two forces/modules with no CkParticles analogue appear here** — `Vortex Force` (`Sparkles_Spiral`)
and `Scale Sprite Size by Speed` (`Sparkles_Stretched`). Both are expressible as behavior math (§6.5).

The `[values]` blocks again carry Rapid-Iteration entries for **disabled** modules
(`Stars` → `Scale Sprite Size 001` and `Sprite Rotation Rate` are both DISABLED; their curve
overrides are inert). A value in `[values]` is not evidence a module runs.

---

## 3. Mesh geometry

**N/A — NS_BuffLoop has no mesh renderer.** All nine renderers are sprites.

---

## 4. Material family and per-instance deltas `[corpus]`

**All seven materials are instances of `/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd`**
— the family CkUsf already implements. Each is `MD_Surface`, `BLEND_Translucent`, `MSM_Unlit`,
`twoSided: false`, outputs `EmissiveColor` + `Opacity`, dynamic channels
`[dissolve, distortion, offset, core_color]`, expression count 161.

Reference = `M_VFX_DisAdd_Part01` (all family defaults; absolute values transcribed in
[NS_BuffCast.md](NS_BuffCast.md) §4).

| Material | Δ vs `Part01` | Used by |
|---|---|---|
| `M_VFX_DisAdd_Part01` | (reference) | Glow_01, Glow_02 |
| `M_VFX_DisAdd_Part01_Bright` | `Brightness` 1 → **10**; `Core_Intensity` 0 → **1**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Part_02` | Sparkles_01, Sparkles_Spiral |
| `M_VFX_DisAdd_Part02` | `Glow_Intensity` 1 → **0.3**; Main/Color/Dissolve_Tex → `T_VFX_Part_02` | Flares |
| `M_VFX_DisAdd_Rainbow` | `GradientMap_Displacement` 0.1 → **0.9**; `Gradient_Invert` 0.5 → **2**; `Opacity_Boldness` 0.5 → **1.5**; `Opacty_StepAdd` 0.1 → **0.3**; **`GradientMap_Tex` → `T_VFX_LUT_Rainbow_01`**; `GradientShape_Tex` → `T_VFX_Part_01`; `Main_Tex` → `T_VFX_Ring_02` | Raimbow |
| `M_VFX_DisAdd_Arrows` | `Brightness` 1 → **10**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Arrow_01` | Arrow |
| `M_VFX_DisAdd_Star01` | `Brightness` 1 → **6**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **10**; Main/Color/Dissolve_Tex → `T_VFX_Star_01` | Stars |
| `M_VFX_DisAdd_Part04` | `Brightness` 1 → **6**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **30**; Main/Color/Dissolve_Tex → `T_VFX_Part_04` | Sparkles_Stretched |

**`Part01_Bright` and `Part02` share the same three textures (`T_VFX_Part_02`) and differ only in
scalars** — `Part01_Bright` is Brightness 10 / Core_Intensity 1 / Boldness 1, `Part02` is
Glow_Intensity 0.3 with everything else at family default. That pair is the cheapest possible proof
that a look is its parameter defaults, not its texture.

`M_VFX_DisAdd_Part04` here is parameter-identical to the instance NS_BasicAttack already recreated as
the `PartDisAdd04` look — reusable verbatim.

Textures: same set and metadata as [NS_BuffCast.md](NS_BuffCast.md) §4 (all 512² greyscale
`TC_Alpha`, `sRGB=false`), minus `T_VFX_Ring_01` and `T_VFX_Gradient_02`, plus nothing new.
`T_VFX_LUT_Rainbow_01` (512×2, `TSF_BGRA8`, `TC_Default`, **sRGB true**) is again the outlier.

---

## 5. Per-layer runtime curves `[corpus]`

Curves sample **NormalizedAge** unless stated. `C` = constant (step) key, `L` = linear key.
`Dynamic Material Parameters` writes **Index 0 only** on every emitter.

### Layer 0 — `Glow_01` (rate 2/s)
- Initialize Color `RGBA(1, 1, 1, 0.4)`; Lifetime **2.0**; Uniform Sprite Size **500**; `UsePositionOffset = false`, Position Offset (0,0,0).
- Color from Curve:
  - `R: (0, 1)C (0.457591, 1)L (1, 1)C`
  - `G: (0, 0.738388)C (0.457591, 0.166)L (1, 0.735614)C`
  - `B: (0, 0.057)C (0.457591, 0.166)L (1, 0.0469999)C`
  - `A: (0, 0)C (0.258376, 1)L (0.680954, 1)L (1, 0)L`
- **No size curve** (3 update modules) — holds 500 for its whole 2 s life.
- Dynamic params: **`[1, 0, 0, 0]`** constant.

### Layer 1 — `Raimbow` (rate 1/s)
- Initialize Color `RGBA(0.913099, 0.913099, 0.913099, 0.2)`; Lifetime **0.5**; Uniform Sprite Size **300**.
- Sprite Rotation Mode Random, Min 0 / Max 360.
- Scale Color (RGBA Together): `R (0, 0.5)L | G (0, 0.5)L | B (0, 0.5)L | A (0, 1)L (1, 0)L`
  — RGB is a single key at 0.5 (flat multiplier), alpha 1 → 0.
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (0.3, 0.875)C (1, 1)L`.
- Dynamic params: **`[0.5, 0, 0, 0]`** constant.

### Layer 2 — `Arrow` (rate 3/s)
- Lifetime **1.5**; Initialize Color `RGBA(1, 1, 1, 1)`; **Position Offset `(0, 0, -119.316)`**, `UsePositionOffset = true`.
- **`Cylinder Location`**: Radius **120**, Height **150**, Height Midpoint 0.5, Offset **(0, 0, 30)**,
  Random distribution, Spawn Only, `Surface Only = false`, `Override Local Rotation = true`,
  Non Uniform Scale (1,1,1), Apply Owner Scale (0,0,0).
- `Add Velocity` overridden by `Random Range Vector`: **Min `(0, 0, 350)` → Max `(0, 0, 500)`** (Seed 0) — straight up, randomised speed.
- Sprite Size Mode Non-Uniform, `Sprite Size (80, 130)`. (`Sprite Size Min (60,150)` / `Max (100,200)` present but Non-Uniform mode uses the direct value.)
- Sprite Rotation Mode Random, authored angle 0.827273, Min 0 / Max 360.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.3)C (1, 0.05)C`.
- Solve Forces and Velocity: both limiters disabled; Rotational Solver on.
- Color from Curve (the shared "warm→deep-orange" arrow ramp):
  - `R: (0, 1)C (0.0784787, 1)L (0.283731, 1)L (0.625415, 0.672443)L (0.947781, 0.223228)C`
  - `G: (0, 0.913099)C (0.0784787, 0.501026)L (0.283731, 0.0773835)L (0.625415, 0.021219)L (0.947781, 0)C`
  - `B: (0, 0.584079)C (0.0784787, 0.0559999)L (0.283731, 0.025)L (0.625415, 0.0168074)L (0.947781, 0.116971)C`
  - `A: (0.0869303, 1)C (1, 0)C`
- Scale Sprite Size (Uniform Curve): `(0, 0.4)C (0.1, 1)C (1, 0.4)C`.
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 3 — `Stars` (rate 2/s)
- Lifetime `[unresolved]` — `Lifetime Min 0.3 / Max 0.6` vs `RandomRangeFloat 0.2 / 0.4` (see §2).
- `Cylinder Location`: Radius **80**, Height **120**, Midpoint 0.5, Offset **(0, 0, 30)**, Random, Spawn Only.
- `Add Velocity` = `Random Range Vector` **Min `(0, 0, 500)` → Max `(0, 0, 1300)`**.
- Sprite Size Mode Random Uniform: **Min 40, Max 70**. Initialize Color `RGBA(1, 1, 1, 1)`.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.2)C (1, 3.91223e-08)C`.
- Color from Curve:
  - `R: (0, 1)C (0.182618, 1)L (0.383139, 1)L (0.68034, 0.672443)L (0.947781, 0.223228)C`
  - `G: (0, 0.913099)C (0.182618, 0.501026)L (0.383139, 0.077384)L (0.68034, 0.021219)L (0.947781, 0)C`
  - `B: (0, 0.584079)C (0.182618, 0.056)L (0.383139, 0.025)L (0.68034, 0.016807)L (0.947781, 0.116971)C`
  - `A: (0.08693, 1)C (1, 0)C`
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`.
- **`Scale Sprite Size 001` DISABLED** (`X: (0, 0.2)C (0.3, 0.7)C | Y: (0.2, 1)L (1, 1.2)L` — inert).
- **`Sprite Rotation Rate` DISABLED** (`(0, 720)C (1, 0)C` — inert; the BuffCast sibling has the same
  module ENABLED, so this is a real per-system difference, not an export artefact).
- Dynamic params: **`[1, 0, 0, 0]`** constant.

### Layer 4 — `Sparkles_Stretched` (rate 10/s)
- Lifetime `[unresolved]` — `0.3 / 0.6` vs `0.2 / 0.4`.
- `Cylinder Location`: Radius **120**, Height **150**, Midpoint 0.5, Offset **(0, 0, 30)**.
- `Add Velocity` = `Random Range Vector` **Min `(0, 0, 1000)` → Max `(0, 0, 2000)`**.
- Sprite Size Mode **Random Non-Uniform**: `Sprite Size Min (35, 130)` / `Max (50, 140)` — width × length streaks.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.1, 0.15)C (1, -9.09372e-09)C` (final key numerically zero; note the **negative** epsilon).
- Color from Curve (the shared arrow ramp, keys rounded slightly differently by the exporter):
  - `R: (0, 1)C (0.078479, 1)L (0.283731, 1)L (0.625415, 0.672443)L (0.947781, 0.223228)C`
  - `G: (0, 0.913099)C (0.078479, 0.501026)L (0.283731, 0.077384)L (0.625415, 0.021219)L (0.947781, 0)C`
  - `B: (0, 0.584079)C (0.078479, 0.056)L (0.283731, 0.025)L (0.625415, 0.016807)L (0.947781, 0.116971)C`
  - `A: (0.08693, 1)C (1, 0)C`
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`.
- Scale Sprite Size 001 (Non-Uniform Curve): `X: (1, 1)L | Y: (0, 1)C (0.1, 0.2)C (1, 0.15)C` — the
  streak collapses to 20 % of its length by t = 0.1 and holds ~0.15.
- **`Scale Sprite Size by Speed`**: `Scale Factor Curve (0, 0)L (1, 1)L`, `Velocity Threshold` **1000**,
  `Min Scale Factor (1, 1)`, `Max Scale Factor (1, 1.7)` — the streak lengthens up to 1.7× with speed.
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 5 — `Glow_02` (rate 4/s)
- Initialize Color `RGBA(1, 1, 1, 1)`; Lifetime **1.0**; Uniform Sprite Size **300**.
- Color from Curve (near-identical to Glow_01, three values differ):
  - `R: (0, 1)C (0.457591, 1)L (1, 1)C`
  - `G: (0, 0.737001)C (0.457591, 0.155)L (1, 0.734227)C`
  - `B: (0, 0.0519999)C (0.457591, 0.155)L (1, 0.0419999)C`
  - `A: (0, 0)C (0.258376, 1)L (0.680954, 1)L (1, 0)L`
- **No size curve** (3 update modules).
- Dynamic params: **`[1, 0, 0, 0]`** constant.

### Layer 6 — `Sparkles_01` (rate 10/s)
- Lifetime `[unresolved]` — `0.3 / 0.6` vs `0.2 / 0.4`.
- `Cylinder Location`: Radius **120**, Height **150**, Midpoint 0.5, Offset **(0, 0, 30)**.
- `Add Velocity` = `Random Range Vector` **Min `(0, 0, 1000)` → Max `(0, 0, 2000)`**.
- Sprite Size Mode Random Uniform: **Min 10, Max 15**. Initialize Color `RGBA(1, 1, 1, 1)`.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.1, 0.15)C (1, -9.09372e-09)C`.
- Color from Curve: identical to layer 4's.
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`.
- Dynamic params: **`[3, 0, 0, 0]`** constant (dissolve = 3 — well past full erosion; layer reads as a plain glow `[inferred]`).

### Layer 7 — `Flares` (rate 6/s)
- **Color Mode = `Random Hue/Saturation/Value`** — the only emitter in this batch that does not
  Direct-Set its colour. Parameters: `Color RGBA(1, 0, 0.00672436, 1)` (the base),
  `Hue Shift Range (0.5, 0.8)`, `Saturation Range (0.2, 0.2)`, `Value Range (1, 1)`,
  `Alpha Scale Range (0.13, 0.13)`, `Color Minimum RGBA(0,0,0,1)`, `Color Maximum RGBA(1,1,1,1)`.
  (`AdjustHue = true`, `AdjustSaturation/Value/Alpha = false` per the module's flags.)
- Lifetime `[unresolved]` — `Lifetime Min 1 / Max 2` vs `RandomRangeFloat 0.2 / 0.4`. **This is the
  worst of the five conflicts** (see §2).
- `Cylinder Location`: Radius **110**, Height **130**, Midpoint 0.5, Offset **(0, 0, 30)**.
- **No velocity module at all** — Flares do not move; only Cylinder Location places them.
- Sprite Size Mode Random Uniform: **Min 50, Max 200** (the widest size spread in the system).
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0.8)C`.
- Scale Color — **`Scale Mode = RGB and Alpha Separately`** (the only emitter using this mode):
  - `Scale RGBA` curve: `R (0, 1)L (1, 1)L | G (0, 1)L (1, 1)L | B (0, 1)L (1, 1)L | A (0, 0)L (0.236644, 1)L (1, 0)L`
  - `Scale Alpha` (separate Float from Curve): **`(0, 0)L (0.3, 0.125)L (1, 0)L`**
  In "RGB and Alpha Separately" mode the separate `Scale Alpha` curve is the one that drives alpha,
  so the effective alpha envelope peaks at **0.125** at t = 0.3 `[inferred — which of the two alpha
  channels the mode selects is a Niagara module semantic, not a corpus fact]`.
- Dynamic params: **`[8, 0, 0, 0]`** constant — the largest dissolve constant anywhere in this batch.

### Layer 8 — `Sparkles_Spiral` (rate 10/s)
- Lifetime `[unresolved]` — `0.3 / 0.6` vs `0.2 / 0.4`.
- `Cylinder Location`: Radius **80**, Height **120**, Midpoint 0.5, **Offset `(0, 0, 0)`** (the only
  cylinder in the system without the +30 Z lift).
- `Add Velocity` = `Random Range Vector` **Min `(0, 0, 3500)` → Max `(0, 0, 5000)`** — by far the fastest layer.
- Sprite Size Mode Random Uniform: **Min 10, Max 15**.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.1, 0.15)C (1, -9.09372e-09)C`.
- **`Vortex Force`**: `Vortex Axis (0, 0, 1)`, `Vortex Force Amount` **15881.6**,
  `Vortex Origin (0,0,0)`, `Vortex Origin Offset (0,0,0)`, `Vortex Influence Position (0,0,0)`,
  `Influence Falloff Radius` **100**, `Origin Pull Amount` **0**.
  (Its `Debug*` entries are editor-only.) This is what makes the layer spiral.
- Color from Curve: identical to layer 4's.
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`.
- Dynamic params: **`[3, 0, 0, 0]`** constant.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence — the hard problem for this effect

**No existing row fits, and neither does a new burst row without changing what the effect IS.**
The source is nine *continuous* streams at nine different rates with five different lifetimes:

| Emitter | Rate/s | Lifetime |
|---|---|---|
| Glow_01 | 2 | 2.0 |
| Raimbow | 1 | 0.5 |
| Arrow | 3 | 1.5 |
| Stars | 2 | rand |
| Sparkles_Stretched | 10 | rand |
| Glow_02 | 4 | 1.0 |
| Sparkles_01 | 10 | rand |
| Flares | 6 | rand |
| Sparkles_Spiral | 10 | rand |

The existing continuous row (`PS_CkParticles_Template`, `BurstCount 0`) takes its lifetime and loop
**from the emitter factory defaults** — `FCk_ParticlesTemplateSpec`'s `LoopDuration` /
`ParticleLifetime` fields are documented as *unused* for that row. So today a recreation cannot even
state "spawn 48/s with a 2.0 s lifetime", let alone nine different rates.

Two honest routes, both of which are decisions a maintainer must make, not defaults:

- **(A) Extend the cadence table** so a continuous row carries a real `SpawnRate` and
  `ParticleLifetime`, then run ONE stream at 48/s with lifetime 2.0 (the longest layer) and partition
  by `Seed % 48` weighted by rate (Glow_01 gets 2 slots, Raimbow 1, Arrow 3, …). Each layer hides
  itself past its own lifetime, exactly as `Behavior_Slash` does. **Cost:** a new field on
  `FCk_ParticlesTemplateSpec` plus builder support; the emission becomes rate-uniform rather than
  per-emitter-independent, which is visually equivalent at these rates `[inferred]`.
- **(B) Fake it with a burst row** of 48 on a 1.0 s loop. **This is the anti-pattern
  `CkParticles/CLAUDE.md` explicitly names** ("never approximate onto the nearest template"), and it
  is worse here than usual: 48 particles appearing on the same frame every second reads as a pulse,
  and the source has no pulse. Do not do this silently.

Route (A) is the recommendation. It is also reusable — `NS_DebuffLoop` and `NS_HealLoop` in this
batch have exactly the same shape.

### 6.2 VisTag / renderer needs

Seven camera-facing sprite layers with **five distinct materials**, plus two velocity-aligned layers
with two more. As in [NS_BuffCast.md](NS_BuffCast.md) §6.2, the shared 0–4 band cannot express this
(one `User.SpriteMaterial`), and row-declared renderers today offer only `Mesh` and
`VelocityAlignedSprite`.

| Renderer | Kind | Look | Source emitters |
|---|---|---|---|
| a | CameraFacingSprite *(NEW KIND REQUIRED)* | `PartDisAdd01` | Glow_01, Glow_02 |
| b | CameraFacingSprite *(new kind)* | `PartDisAdd01Bright` | Sparkles_01, Sparkles_Spiral |
| c | CameraFacingSprite *(new kind)* | `PartDisAdd02` | Flares |
| d | CameraFacingSprite *(new kind)* | `RainbowDisAdd` | Raimbow |
| e | CameraFacingSprite *(new kind)* | `StarDisAdd01` | Stars |
| f | VelocityAlignedSprite | `ArrowsDisAdd` | Arrow |
| g | VelocityAlignedSprite | `PartDisAdd04` — **already exists** | Sparkles_Stretched |

**7 row renderers, 5 of them needing the new `CameraFacingSprite` kind.** VisTags allocate above the
current roster maximum (behavior 7 owns 5–9); read `Get_RosterVisTag_Max()`, never a literal.

Five of the seven looks (`PartDisAdd01`, `PartDisAdd01Bright`, `PartDisAdd02`, `RainbowDisAdd`,
`StarDisAdd01`, `ArrowsDisAdd`) are shared with `NS_BuffCast` — author them once for the batch.

### 6.3 Look / material needs

Same family (`DissolveAdd`), same unplumbed-parameter list as [NS_BuffCast.md](NS_BuffCast.md) §6.3.
The ones that actually bite here:

| Param | Value here | Plumbed? |
|---|---|---|
| `Glow_Intensity` | 0.3 on `Part02` (Flares) | **no** |
| `Core_Intensity` | 1 on `Part01_Bright` (two layers) | **no** |
| `Gradient_Invert` | 0 / 0.5 / 2 | **no** |
| `GradientMap_Tex` + `GradientMap_Displacement` | LUT + 0.9 on Rainbow | **no** — see §6.5 |
| `Opacty_StepAdd` | 0.3 on Rainbow | **no** |
| `Opacty_DepthFade` | 10 / 20 / 30 | **no** |

`Core_Intensity` matters more here than in BuffCast: two of the nine layers use `Part01_Bright`, whose
only differences from `Part02` are `Brightness`, `Core_Intensity` and `Opacity_Boldness` — dropping
`Core_Intensity` makes those two looks converge on a wrong value.

### 6.4 Texture needs

`T_VFX_Part_01` → existing `SoftParticle`; `T_VFX_Part_04` → existing `SparkStreak`;
`T_VFX_Noise_02` → existing `TileNoise`. New measurement + bake needed for `T_VFX_Part_02`,
`T_VFX_Arrow_01`, `T_VFX_Star_01` (the existing `Flare` bake may fit — **unmeasured**),
`T_VFX_Ring_02`, and the **colour LUT** `T_VFX_LUT_Rainbow_01`. Method: NS_BasicAttack §7.

### 6.5 CAPABILITY GAPS — read before committing a session

1. **CONTINUOUS-CADENCE ROWS ARE NOT PARAMETERIZABLE.** The one continuous template row ignores the
   spec's `LoopDuration`/`ParticleLifetime` and has no spawn-rate field at all. Every "Loop" system in
   this batch needs this. **This is the single highest-leverage pipeline addition for the batch** and
   should be sized before any Loop effect is started.

2. **CAMERA-FACING SPRITE ROW RENDERER — does not exist.** Five layers here need it. Shared need
   across all six sheets.

3. **GRADIENT-MAP LUT.** `M_VFX_DisAdd_Rainbow` drives a real 512×2 sRGB colour ramp through
   `GradientMap_Tex` at `Displacement 0.9` / `Gradient_Invert 2`. The family shader has no gradient
   chain and the procedural texture generator makes only greyscale, `SRGB=false`,
   `TC_VectorDisplacementmap` bakes. Without both additions the Raimbow layer is a white glow.
   (NS_BasicAttack §13.4's "the gradient chain is a no-op on this family" reasoning is instance-specific
   and **does not transfer** — that instance had a 1×1 white gradient map; this one does not.)

4. **`Random Hue/Saturation/Value` colour mode** (`Flares`). Not a pipeline gap — it is per-particle
   math a behavior can do with `CkParticles_Rand(Seed, salt)` and an HSV→RGB conversion. But
   `Common.ush` has no HSV helper today, and the CPU mirror must match bit-for-bit, so budget for a
   shared HSV function written twice.

5. **`Vortex Force`** (`Sparkles_Spiral`, force 15881.6 about +Z, falloff radius 100).
   Also not a pipeline gap, but it is an **acceleration-based** force: `Behavior_Slash`'s
   closed-form-integration discipline (NS_BasicAttack §8, lesson 7) means the spiral path must be
   solved analytically rather than stepped, or GPU and CPU will drift. A vortex about a fixed axis
   with a radial falloff does **not** have a trivial closed form; expect this layer to cost real
   derivation time or to be approximated as a parametric helix and recorded as a deviation.

6. **`Scale Sprite Size by Speed`** (`Sparkles_Stretched`). Expressible — the behavior knows its own
   velocity — but it couples size to the velocity-decay curve, so it must be derived from the same
   closed form, not from a frame delta.

7. **WORLD SPACE vs LOCAL SPACE.** All nine emitters are `LocalSpace: false`; the CkParticles template
   is local-space. Same recorded deviation as NS_BasicAttack §13.2. It matters MORE for a "Loop"
   effect than for a "Cast" one, because a buff loop is typically attached to a *moving* character —
   the source's particles would be left behind in the world, the recreation's would follow the actor.
   **Flag this to the maintainer explicitly; for this family of effects the local-space behaviour may
   actually be the desirable one, but it is a visible difference either way.**

8. **SPRITE ROTATION.** `Raimbow` uses random 0–360° rotation. `[unresolved: whether
   `Particles.SpriteRotation` is bound on the shared camera sprite renderer, and would be on a new
   row-declared one]` — verify in `CkParticles_TemplateBuilder.cpp`.

9. **No ribbon, no sub-UV, no events in this system** — three gaps its `NS_BuffCast` sibling has that
   this one does not. NS_BuffLoop is the *simplest* system in this batch on the renderer axis and the
   *hardest* on the cadence axis.

### 6.6 Behavior id

**Do NOT allocate an id here.** `ck::particles::NumBehaviors` is 18 at the time of writing; the
implementing session allocates from it and bumps it in the same edit.

### 6.7 Complexity assessment

**Tier L** — driven entirely by gap 1 (parameterizable continuous cadence) and gap 5 (closed-form
vortex). With a burst approximation and a dropped vortex it would be **M**, but both of those are
recorded fidelity losses, not free simplifications.

---

## 7+. Reserved for implementation — sections 7–14 per [README.md](README.md) are written by the session that implements this effect.
