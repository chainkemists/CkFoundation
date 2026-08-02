# Translation sheet: NS_BuffLoop → CkParticles (PRE-IMPLEMENTATION)

Schema and evidence-tag conventions: [README.md](README.md). Exemplars: [NS_BasicAttack.md](NS_BasicAttack.md),
[NS_Lightning_Range.md](NS_Lightning_Range.md).

## Completion state — READ FIRST

**Status: IMPLEMENTATION-COMPLETE (2026-08-02) as CkParticles behavior 28, `BuffLoop`.
NOT visually verified — the §12 human A/B has not been run.**

§1–6 are archaeology against the extracted corpus, re-verified against the v3 sidecar at implementation
time. **Two corrections were made in place:**

1. **§5's Flares parenthetical had the adjust flags backwards** — it claimed only `AdjustHue` was set. The
   corpus sets all four, so the pinned `Saturation Range (0.2, 0.2)` and `Alpha Scale Range (0.13, 0.13)`
   are LIVE, not inert.
2. **The per-emitter `Color.Scale Alpha` values were missing** (§2). Only `Glow_01` is off 1 here, so this
   system escaped lightly; its Heal and Debuff siblings did not.

§6.1's `[P0-D3 STOP]` is RESOLVED: campaign decision **[P0-D4]** routed the four rate-only Loop systems
through Phase 2's C2 spawn-rate rows, and **route (A)** — a real spawn rate on the cadence row plus a
rate-weighted partition — is what shipped. Route (B), the 96-particle burst approximation §6.1 named as
the anti-pattern, was never on the table.

§6.5 gap 5 (the vortex) resolved WITHOUT curl noise: the source force is a plain tangential vortex, so it
has an exact closed form. See §9.4.

§7 onward is what was actually built.

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
(continuous). There is no `Spawn Burst Instantaneous` module anywhere in the system. This is the
fundamental difference from its `NS_BuffCast` sibling and it drives the whole §6 plan.

**System loop `[corpus-v3]`: `Loop Behavior = Infinite`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`** (no `Recalculate Duration Each Loop`
override on this system). All nine emitters are `Life Cycle Mode = System`, so per [P0-D1] the
system rules and the per-emitter `Infinite / 1.0 s` rows are inert leftovers. *(Was read as a 1.0 s
loop.)* The loop duration only resets `Emitter.Age`; it does not gate spawning, so the correction is
low-risk here.

| # | Emitter | Spawn Rate (/s) | Lifetime | Renderer / alignment | Material | Size |
|---|---|---|---|---|---|---|
| 0 | `Glow_01` | **2** | 2.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part01` | uniform 500 |
| 1 | `Raimbow` *(sic)* | **1** | 0.5 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Rainbow` | uniform 300 |
| 2 | `Arrow` | **3** | 1.5 | Sprite, **VelocityAligned** / FaceCamera | `M_VFX_DisAdd_Arrows` | non-uniform (80, 130) |
| 3 | `Stars` | **2** | rand **0.3–0.6** `[corpus-v3]` | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Star01` | rand uniform 40–70 |
| 4 | `Sparkles_Stretched` | **10** | rand **0.3–0.6** `[corpus-v3]` | Sprite, **VelocityAligned** / FaceCamera | `M_VFX_DisAdd_Part04` | rand non-uniform (35,130)–(50,140) |
| 5 | `Glow_02` | **4** | 1.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part01` | uniform 300 |
| 6 | `Sparkles_01` | **10** | rand **0.3–0.6** `[corpus-v3]` | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part01_Bright` | rand uniform 10–15 |
| 7 | `Flares` | **6** | rand **1.0–2.0** `[corpus-v3]` | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part02` | rand uniform 50–200 |
| 8 | `Sparkles_Spiral` | **10** | rand **0.3–0.6** `[corpus-v3]` | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part01_Bright` | rand uniform 10–15 |

**Total spawn rate 48 particles/second.** Steady-state live count ≈ **48** using the `[corpus-v3]`
resolved lifetimes (Σ rate × mean lifetime: 4 + 0.5 + 4.5 + 0.9 + 4.5 + 4 + 4.5 + 9 + 4.5) —
arithmetic, `[inferred]` from the rate × lifetime products, not a corpus value.
*(Was ≈ 36 under the shorter override-wins reading.)*

> ### Lifetime — RESOLVED `[corpus-v3]`
> `Stars`, `Sparkles_Stretched`, `Sparkles_01`, `Sparkles_Spiral` and `Flares` all carry BOTH
> `Lifetime Mode = Random` with `Initialize Particle.Lifetime Min/Max` **and** an
> `[override] Lifetime = dyn:Random Range Float`. Per [P0-D2] the mode selects the driving pin:
> `Random` ⇒ **Min/Max drives**, and the override — which sits on the unselected **Direct Set**
> input — is INERT (`lifetimeResolved.source = minmax`, override under `inertOverrides`) on all five.
> The sheet's `[inferred]` guess is now MECHANICALLY CONFIRMED.
>
> | Emitter | LIVE (Random mode) | inert override |
> |---|---|---|
> | `Stars` | **0.3 / 0.6** | ~~0.2 / 0.4~~ |
> | `Sparkles_Stretched` | **0.3 / 0.6** | ~~0.2 / 0.4~~ |
> | `Sparkles_01` | **0.3 / 0.6** | ~~0.2 / 0.4~~ |
> | `Sparkles_Spiral` | **0.3 / 0.6** | ~~0.2 / 0.4~~ |
> | `Flares` | **1.0 / 2.0** | ~~0.2 / 0.4~~ |
>
> `Flares` — the worst conflict — resolves to the **1–2 s drifting flare**, not the 0.2–0.4 s blink.

**Two forces/modules with no CkParticles analogue appear here** — `Vortex Force` (`Sparkles_Spiral`)
and `Scale Sprite Size by Speed` (`Sparkles_Stretched`). Both are expressible as behavior math (§6.5).

**`Color` module `Scale Alpha` per emitter `[corpus]`** — from the `[values]` blocks, added at
implementation time because §5 does not carry them: `Glow_01` **0.5**; every other emitter that has a
`Color` module resolves **1** (`Raimbow`, `Arrow`, `Stars`, `Sparkles_Stretched`, `Glow_02`,
`Sparkles_01`, `Sparkles_Spiral`). `Flares` has no `Color` module — its alpha comes from the HSV
`Alpha Scale Range` and the `Scale Color` module. This system is the batch's least affected by the
omission; its Heal and Debuff siblings are not.

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
- Lifetime `[corpus-v3]` — **`Lifetime Min 0.3 / Max 0.6` drives**; the `RandomRangeFloat 0.2 / 0.4` override is inert (see §2).
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
- Lifetime `[corpus-v3]` — **`0.3 / 0.6` drives**; the `0.2 / 0.4` override is inert.
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
- Lifetime `[corpus-v3]` — **`0.3 / 0.6` drives**; the `0.2 / 0.4` override is inert.
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
  **CORRECTED `[corpus]` (2026-08-02):** all FOUR adjust flags are `true` —
  `AdjustHue`, `AdjustSaturation`, `AdjustValue` AND `AdjustAlpha`. The earlier parenthetical claimed the
  last three were false, which would have made the pinned `Saturation Range (0.2, 0.2)` and
  `Alpha Scale Range (0.13, 0.13)` inert; they are not. Since the base colour is a pure hue (saturation and
  value both 1), the "range replaces" and "range scales" readings coincide, so the correction is
  unambiguous: saturation resolves to 0.2 and the alpha is scaled by 0.13.
- Lifetime `[corpus-v3]` — **`Lifetime Min 1.0 / Max 2.0` drives**; the `RandomRangeFloat 0.2 / 0.4`
  override is inert. The worst of the five conflicts resolves in favour of the **long** drifting
  flare (see §2).
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
- Lifetime `[corpus-v3]` — **`0.3 / 0.6` drives**; the `0.2 / 0.4` override is inert.
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

**`[P0-D3 STOP: loop = 2.0 s (system, Infinite/Fixed); lifetime = 2.0 s (max resolved — Glow_01
direct 2.0 and Flares' 2.0 Max); burst = 0 — rate-only system, no burst module anywhere and §2
carries no burst count]`.** The formula yields a `BurstCount == 0` continuous row that the cadence
table cannot parameterize today. Orchestrator ruling required before a row is written; routes (A)
and (B) below are the candidate shapes.

**No existing row fits, and neither does a new burst row without changing what the effect IS.**
The source is nine *continuous* streams at nine different rates with five different lifetimes:

| Emitter | Rate/s | Lifetime |
|---|---|---|
| Glow_01 | 2 | 2.0 |
| Raimbow | 1 | 0.5 |
| Arrow | 3 | 1.5 |
| Stars | 2 | rand 0.3–0.6 `[corpus-v3]` |
| Sparkles_Stretched | 10 | rand 0.3–0.6 `[corpus-v3]` |
| Glow_02 | 4 | 1.0 |
| Sparkles_01 | 10 | rand 0.3–0.6 `[corpus-v3]` |
| Flares | 6 | rand 1.0–2.0 `[corpus-v3]` |
| Sparkles_Spiral | 10 | rand 0.3–0.6 `[corpus-v3]` |

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
- **(B) Fake it with a burst row** of 96 on the 2.0 s system loop `[corpus-v3]` (*was 48 on 1.0 s*). **This is the anti-pattern
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

## 7. Textures — one new bake, and it is the library's first polygon

§6.4 asked for five new bakes plus the colour LUT. **One was needed.** Four of the five source paints had
already been measured off the same corpus PNG by an earlier batch:

| Source paint | Stand-in | Measured in |
|---|---|---|
| `T_VFX_Part_01` | `SoftParticle` | NS_BasicAttack §7 |
| `T_VFX_Part_02` | `SoftParticleBright` | NS_FireBall_Hit §7 |
| `T_VFX_Part_04` | `SparkStreak` | NS_BasicAttack §7 |
| `T_VFX_Star_01` | `StarFour` | NS_FireBall_Hit §7 |
| `T_VFX_Ring_02` | `RingFlare` | NS_FireBall_Hit §7 |
| `T_VFX_Noise_02` | `TileNoise` | NS_BasicAttack §7 |
| `T_VFX_LUT_Rainbow_01` | `LutRainbow` baked, **not bound** ([P1-D1], §13.2) | Phase 1 C3 |
| **`T_VFX_Arrow_01`** | **`ArrowChevron` — NEW** | this recipe |

### 7.1 `T_VFX_Arrow_01` → `T_CkParticles_ArrowChevron`

**Reuse was measured first and there was no candidate at all.** Every mask in the library is radial,
streaked, or noise; not one has a straight edge, and this paint has four. That is a structural
disqualification, not a threshold call.

Measured off the 512² corpus PNG (greyscale — R = G = B, alpha flat 1):

| Statistic | Measured |
|---|---|
| Horizontal mirror correlation about u = 0.5 | **0.99999996** — symmetric to six decimals |
| Vertical mirror correlation | **−0.024** — no vertical symmetry at all |
| Lit extent | u 0.205 … 0.793, **v 0.207 … 0.559** — the shape occupies the UPPER HALF only |
| Coverage above 0.02 | 23.5 % of the square |
| Peak | 1.0, reached over a solid interior |

Tracing the **half-intensity contour** gives a quadrilateral, not a curve — the arm's leading edge runs
straight at du/dv ≈ −1.48 from the apex, turns a corner, and closes on a tip:

| Corner | uv |
|---|---|
| outer apex (on the centreline) | (0.5000, 0.2666) |
| outer corner where the leading edge turns | (0.2637, 0.4189) |
| arm tip | (0.3242, 0.5010) |
| inner apex — the notch between the arms | (0.5000, 0.3914) |

Regressing intensity against the **signed distance** to that quad (mirrored into the left half) fits a
two-term exponential over the whole −0.03 … +0.21 range:

```
I(d) = saturate(0.4222 * exp(-49.0 * d) + 0.0650 * exp(-12.5 * d))
```

**RMSE 0.0047** across 22 distance bands. The two terms are physically distinct: the fast one is the
paint's own edge (half-value ~0.010 from the contour) and the slow one is its wide, low-amplitude halo,
which a single exponential cannot carry — a one-term fit is 4x worse on the tail.

Bake kind `Mask` (512² greyscale, `SRGB=false`, `TC_VectorDisplacementmap`), like every other shape paint.

---

## 8. Mesh

**None.** All nine source renderers are sprites: seven camera-facing and two velocity-aligned.

---

## 9. The behavior — `Behavior_BuffLoop.ush` + `ExecuteStage_CPU` case 28

### 9.1 The cadence row

| Field | Value | Source |
|---|---|---|
| `LoopDuration` | 2.0 s | the SYSTEM's `Loop Behavior = Infinite`, `Loop Duration = 2` ([P0-D1]) |
| `ParticleLifetime` | 2.0 s | Glow_01's Direct-Set 2.0 and Flares' 2.0 maximum |
| `BurstCount` | 0 | there is no burst module anywhere in the system |
| `SpawnRate` | 48 /s | 2+1+3+2+10+4+10+6+10 — the densest row in the cookbook |

### 9.2 The partition

A weighted draw against `CkParticles_Rand(Seed, 0)` over cumulative rate shares in the source's emitter
order — see NS_PickupLoop §9.2 for why a modulus cannot serve a rate-only source. Worst per-layer
deviation over 400 000 seeds: **0.00092**.

### 9.3 Per-layer notes worth the reader's time

- **`Arrow` composes TWO placement terms**: a cylinder at radius 120 / height 150 lifted +30, and
  Initialize Particle's own `Position Offset (0, 0, -119.316)`. Dropping either puts the whole stream in
  the wrong place; the test asserts the layer opens below the origin.
- **`Stars` has two DISABLED modules** (`Scale Sprite Size 001` and `Sprite Rotation Rate`) whose curves
  are in the store. The BuffCast sibling has the rotation one ENABLED, so this is a real per-system
  difference rather than an export artefact — implementing it would import the wrong system's behaviour.
  §11 asserts Stars never rotate.
- **`Arrow`'s authored `Sprite Rotation` is inert** on a velocity-aligned renderer and is deliberately not
  written.
- **`Sparkles_Stretched`** derives its speed-driven length from the same closed-form velocity the position
  integral uses (threshold 1000 units/s, up to 1.7x).
- **`Flares`** now takes the corrected four-flag HSV reading: the red base rotates 0.5–0.8 around the hue
  circle — into the cyans and blues — at a pinned 0.2 saturation.

### 9.4 The Vortex Force — a plain vortex, solved in closed form, NOT curl noise

§6.5 gap 5 warned this layer would "cost real derivation time or be approximated as a parametric helix".
The source settles it: `Vortex Force` with `Vortex Axis (0,0,1)`, `Vortex Force Amount 15881.6`,
`Influence Falloff Radius 100` and **`Origin Pull Amount 0`** is a purely TANGENTIAL acceleration about a
fixed axis. Phase 2's C10 curl-noise helper is therefore not used here — it would model a different force.

Because the pull is zero the radius is invariant, so the whole force reduces to an angle:

- angular acceleration at the particle's own spawn radius, `α = 15881.6 · Falloff(r₀) / max(r₀, 10)`
- the layer's `Scale Velocity` curve `S(u)` damps whatever velocity the force builds, so the angular
  velocity is `α · u · S(u)` in normalized life
- the angle is its exact integral, `θ(t) = θ₀ + α · Life² · ∫₀^u τ·S(τ) dτ`

`S` is piecewise linear, so that integral has a closed form (`CkParticles_BuffLoop_SwirlIntegral`, ~15
lines, mirrored exactly). Nothing is stepped, so the path is a pure function of (spawn, Age, Seed) and
GPU and CPU cannot drift — the same discipline `Behavior_Slash`'s velocity integrals follow.

Measured over 24 seeds, the mean swirl over the first 0.28 s is **0.66 rad ≈ 38°** — a visible quarter- to
half-turn, not a blur and not a rounding artefact.

**The falloff law is `[inferred]`**: `Falloff(r) = saturate(1 − r/100)`. Niagara's own falloff curve lives
in a module graph the corpus does not export, and this is the simplest law consistent with the single
exported number. Recorded in §13.3; the lever-arm floor of 10 units is a singularity guard (a tangential
force has no defined direction ON the axis), not a tuning.

---

## 10. Looks and renderers

Seven row-declared renderers on VisTags **84–90** — five camera-facing sprites and two velocity-aligned,
matching the source's own alignment split.

| VisTag | Kind | Look | Source material | New? |
|---|---|---|---|---|
| 84 | CameraFacingSprite | `PartDisAdd01` | `M_VFX_DisAdd_Part01` | reused |
| 85 | CameraFacingSprite | `RainbowDisAdd` | `M_VFX_DisAdd_Rainbow` | reused |
| 86 | VelocityAlignedSprite | `ArrowsDisAdd` | `M_VFX_DisAdd_Arrows` | **NEW** — `CkUsf_LoopLooks_Assets.as` |
| 87 | CameraFacingSprite | `StarDisAdd01` | `M_VFX_DisAdd_Star01` | reused |
| 88 | VelocityAlignedSprite | `PartDisAdd04` | `M_VFX_DisAdd_Part04` | reused |
| 89 | CameraFacingSprite | `PartDisAdd01Bright` | `M_VFX_DisAdd_Part01_Bright` | reused |
| 90 | CameraFacingSprite | `PartDisAdd02` | `M_VFX_DisAdd_Part02` | reused |

`ArrowsDisAdd` is Brightness 10 / Opacity_Boldness 1 over the new `ArrowChevron` paint, everything else
inherited from the `Part01` reference. It is **shared with NS_DebuffLoop**, where two more emitters draw
through it — three source emitters, one look.

`Get_BehaviorLookName(28)` stays `NAME_None`: every look rides a row renderer that binds it explicitly.

---

## 11. Tests

`Test_Particles_BuffLoopBehavior.cpp` + the `NumBehaviors` 26 → 30 ratchet in
`Test_Particles_RosterSanity.cpp`.

- **The rate-share sweep** over 400 000 seeds, every layer within **0.004** of its source share.
- **The vortex is asserted in two halves, and both can fail independently:** the planar radius must be
  invariant between t = 0.01 and t = 0.29 (Origin Pull 0 — a curl-noise or inward-pulling implementation
  fails this), and the polar angle must actually advance on every sampled particle (an inert one fails
  that). The mean swirl is required above 0.1 rad, and **`Sparkles_01` is the control** — same cylinder
  shape, same velocity range, no force, so its total swirl must be zero.
- **`Stars` never rotates** — the disabled-module claim made falsifiable, and it is the one the BuffCast
  sibling would tempt a reader to break.
- **`Arrow` spawns below the origin** (the two composed placement terms) and draws a taller-than-wide quad.
- **Flares**: hue varies per particle — asserted on the **recovered hue**, not on a colour channel
  (§14.7). The brightest channel is exactly 1 and the darkest exactly 0.8, which is the corrected pinned
  0.2 saturation. Alpha bounded both sides against 0.13 × 0.125.
- Plus the standard per-layer anti-vacuity and death checks.

---

## 12. Verification — A/B protocol

`[HUMAN-VERIFY]` — **not yet run.** Open the **VfxExamples** gym, station pair **BUFF LOOP**.

> **This pair is a STEADY-STATE comparison, not a synced replay.** `NS_BuffLoop` is an INFINITE system: it
> never finishes, so the harness's `OnSystemFinished` re-arm never fires and the two sides are never in
> phase. Judge density, palette and motion character over a few seconds; do NOT expect matched frames.

| # | Criterion | Look for |
|---|---|---|
| a | Overall read | a dense warm buff column rising off the ground — the busiest effect in the batch, and still not pulsing |
| b | Density | ~48 live particles; three of the nine layers spawn at 10/s each |
| c | **Spiral** | one of the three sparkle streams visibly WINDS around the vertical axis while the other two go straight up. Roughly a third to a half turn over a particle's life. **If nothing spirals, the vortex is inert; if the spiral also drifts inward or outward, the radius is not being preserved** |
| d | Arrows | chevrons rising and stretching along their motion, warm at spawn and deep orange as they fade |
| e | Sparkles | one round stream and one stretched stream, the stretched one collapsing to a fifth of its length by 10 % of life |
| f | Stars | four-point stars rising fast — and **not rotating** (§13.6) |
| g | Glows | two shells at 500 and 300 units, the 500 at half coverage |
| h | Flares | a faint PALE CYAN-TO-BLUE haze. The source's base colour is red; the hue shift of 0.5–0.8 is what turns it. If it reads red, the hue randomization is not running |
| i | Rainbow ring | ships against a WHITE ramp (§13.2) — the known [P1-D1] hold |
| j | World space | move the pedestal mid-effect if you can (§13.5) |

---

## 13. Confirmed fidelity differences

1. **The layer partition is a weighted draw, not per-emitter independent spawning** — see NS_PickupLoop
   §13.1.
2. **The Rainbow layer ships against a WHITE ramp** — campaign decision **[P1-D1]**. Reverses in one token.
3. **The vortex falloff law is `[inferred]`** (§9.4): `saturate(1 − r/100)` from the single exported
   `Influence Falloff Radius 100`. Niagara's own curve is in an unexported module graph. The swirl's
   CHARACTER (tangential, radius-preserving, decaying with the velocity curve) is exact; its magnitude
   scales with this law, so it is the one dial to reach for if the spiral reads too tight or too slack.
4. **The swirl is positional.** `O.Velocity` carries the source's Add-Velocity climb, as the source's own
   velocity attribute does; the vortex's tangential contribution is folded into position rather than
   written back into velocity. Nothing downstream in this port reads velocity for this layer (its renderer
   is camera-facing), so the difference is not observable here — but it would be on a velocity-aligned one.
5. **World space.** All nine source emitters are `LocalSpace: false`; the template is local space. §6.5
   gap 7 asks the maintainer to judge which is preferable for a buff attached to a moving character.
6. **`Stars`' two disabled modules are NOT implemented**, deliberately — including the rotation rate its
   BuffCast sibling enables.
7. **Unplumbed family parameters:** `Core_Intensity` (1 on `Part01_Bright`, which serves two layers — §6.3
   flags this as the one that matters most here), `Gradient_Invert`, `Opacty_StepAdd`, `Opacty_DepthFade`.
   **`Glow_Intensity 0.3` on `Part02` IS reproduced**, folded into Brightness.
8. **`In.EmitterAge` is threaded but unread** — see NS_PickupLoop §13.7.
9. **Every stand-in texture is a statistical match of the source paint, not a copy** (§7).

---

## 14. Reusable lessons

1. **Read the force's parameters before reaching for the noise solver.** C10's curl noise was built for
   this phase and this layer did not need it: `Origin Pull Amount 0` on a fixed-axis vortex collapses the
   whole force to an angle, and the angle has a closed form. A capability existing is not a reason to
   spend it.
2. **A closed form beats a step count even when a step count is available.** C10's `CurlPath` is stateless
   but still costs 16 iterations of 12 Fbm evaluations; this layer's exact integral is fifteen lines and
   is not an approximation at all.
3. **Assert a force in two halves that can fail independently.** "Radius invariant" and "angle advances"
   together pin down a tangential vortex; either alone is satisfied by something wrong.
4. **A control layer is worth more than a tighter tolerance.** `Sparkles_01` shares the spiral's cylinder
   and velocity range and carries no force, so "only Sparkles_Spiral swirls" is a claim about the
   PARTITION as much as about the force.
5. **A structural statistic can disqualify every reuse candidate at once.** `T_VFX_Arrow_01` has straight
   edges; nothing in the library does. That is one sentence of measurement instead of eight correlations.
6. **Two exponentials, not one, for a paint with a halo.** The chevron's edge and its wide low-amplitude
   glow have decay rates 4x apart; fitting them together is what got the RMSE to 0.0047.
7. **Never assert "the hue varies" on a colour CHANNEL.** This batch's one gate failure was exactly that,
   and the assertion was UNSATISFIABLE rather than merely tight. HSV→RGB hands the pinned Value to a
   different channel in each 60° sector, so a layer whose hue band sits inside one or two sectors holds a
   channel exactly constant while the hue varies fine — this layer's band is [0.4989, 0.7989], which is
   sectors 3 and 4 plus 0.37 % of sector 2, and **blue is the Value across all of it** (measured minimum
   0.99867 over 20 000 seeds, so the bucket key was a single value for every possible seed). The trap has a
   second edge the failure did not show: on a layer whose `Saturation Range` is a real range, a channel key
   varies even when the hue does NOT, so it would have passed against a dead hue shift. **Invert the
   conversion and bucket the recovered hue** — saturation- and value-independent by construction. Measured
   after the fix: 82 / 102 / 93 distinct half-degree hues on the three HSV layers, and exactly **1** with
   the shift removed. The Cast siblings in batch D carry the same colour mode; use the same key.
