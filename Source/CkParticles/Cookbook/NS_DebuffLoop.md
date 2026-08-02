# Translation sheet: NS_DebuffLoop → CkParticles (PRE-IMPLEMENTATION)

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
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_DebuffLoop` |
| Pack | Vefects — *Anime VFX* |
| Corpus system files | `systems/Vefects/Anime_VFX/Shared/Skills/NS_DebuffLoop.{json,txt}` |
| Corpus material files | `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01_Bright,Ring01,Flames01,Part01,Arrows,Part02}.json` |
| Corpus texture files | `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Ring_01,Arrow_01,Wind_01,Noise_02,Noise_04,WhitePixel}.json` |
| Meshes | none — no mesh renderer |

**The source Niagara asset was never opened.** Every fact below is `[corpus]` unless tagged otherwise.

> ### Sibling with a near-identical name
> `[corpus]` `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Debuff_Loop` (underscored) is a different,
> parameterized system with 9 emitters. Discriminators: `NS_DebuffLoop` exports `userParameters: []`
> and draws through `M_VFX_DisAdd_*`; `NS_Debuff_Loop` exports **ten** user parameters
> (`User.Arrow Color 01/02`, `User.Flames Color 01`, `User.Flare Color 01`, `User.Glow Color 01/02/03`,
> `User.Ring Color 01`, `User.Scale Overall`, `User.Sparkles Color 01`) and draws through
> `MI_VFX_{Arrows_01, Flames_01, Glow_01, Glow_01_Bright, Glow_02, Ring_01}`. This sheet documents the
> **`Anime_VFX/Shared/Skills`** variant only.

---

## 2. System anatomy `[corpus]`

**9 CPU emitters, ALL enabled, ALL world-space (`LocalSpace: false`), `Bounds: Dynamic`,
`Determinism: false`, zero user parameters.**

**Every emitter uses `Spawn Rate` (continuous) — there is no burst module anywhere in the system.**
Three of them run `Loop Behavior = Once` on a 0.3 s loop (a short one-shot burst-of-stream at the
start), the other six run `Loop Behavior = Infinite` on a 1.0 s loop. All nine are
`Life Cycle Mode = System`.

| # | Emitter | Loop | Loop dur | Rate (/s) | Spawn probability | Lifetime | Renderer | Material |
|---|---|---|---|---|---|---|---|---|
| 0 | `Sparkles_Dark` | **Once** | **0.3** | 4 | — | rand `[unresolved]` | Sprite, Unaligned/FaceCamera | `Part01_Bright` |
| 1 | `Ring` | **Once** | **0.3** | 3 | — | rand 1.0–2.0 | Sprite, Unaligned/FaceCamera | `Ring01` |
| 2 | `Flames` | **Once** | **0.3** | 5 | **enabled** (value `[unresolved]`) | rand `[unresolved]` | Sprite, Unaligned/FaceCamera, **SubUV 2×2** | `Flames01` |
| 3 | `Glow_01` | Infinite | 1.0 | 2 | — | 2.0 | Sprite, Unaligned/FaceCamera | `Part01` |
| 4 | `Glow_02` | Infinite | 1.0 | 4 | — | 1.0 | Sprite, Unaligned/FaceCamera | `Part01` |
| 5 | `Glow_03` | Infinite | 1.0 | 4 | — | 1.0 | Sprite, Unaligned/FaceCamera | `Part01` |
| 6 | `Arrow_Green` | Infinite | 1.0 | 4 | **rand 0.5–1.0** | rand 0.6–1.0 | Sprite, **VelocityAligned**/FaceCamera | `Arrows` |
| 7 | `Arrow_Purple` | Infinite | 1.0 | 4 | **rand 0.5–1.0** | rand 0.6–1.0 | Sprite, **VelocityAligned**/FaceCamera | `Arrows` |
| 8 | `Flares` | Infinite | 1.0 | 6 | — | rand `[unresolved]` | Sprite, Unaligned/FaceCamera | `Part02` |

**Steady-state rate from the six infinite emitters: 24 particles/second** (2+4+4+4+4+6), reduced on the
two arrow emitters by their random spawn probability. The three `Once` emitters contribute
≈ 4×0.3 + 3×0.3 + 5×0.3 = **3.6 particles total, one time only** `[inferred]`.

`Flames` sets `Use Spawn Probability = true` but the corpus shows **no `Spawn Probability` override and
no `SpawnRate.SpawnProbability` value** in its store `[unresolved: the effective probability]` —
the two arrow emitters both override it explicitly with `Random Range Float 001` Min 0.5 / Max 1.0.

> ### `[unresolved: lifetime on four emitters]`
> Same `Lifetime Mode = Random` vs `[override] Lifetime = dyn:Random Range Float` conflict as the rest
> of the batch:
>
> | Emitter | `Lifetime Min/Max` (Random mode) | override `RandomRangeFloat` |
> |---|---|---|
> | `Sparkles_Dark` | **1 / 1.5** | 0.2 / 0.4 |
> | `Flames` | **1 / 2** | 0.2 / 0.4 |
> | `Flares` | **1 / 2** | 0.2 / 0.4 |
> | `Ring` | 1 / 2 | *(no override — unambiguous)* |
> | `Arrow_Green` / `Arrow_Purple` | `Lifetime Mode = **Direct Set**` (no Min/Max) | **0.6 / 1.0** — the override IS the source here |
>
> The arrows are the useful counter-example: with `Lifetime Mode = Direct Set` the `Lifetime` pin
> **is** what the module reads, so the `Random Range Float` override unambiguously wins there
> (0.6–1.0). That strengthens the reading that on `Random`-mode emitters the override does **not**
> apply `[inferred]`.

`Ring` also carries `InitializeParticle.Lifetime = 1` alongside its Random Min 1 / Max 2 — inert in
Random mode. `Sparkles_Dark`'s `Curl Noise Force` is present but **DISABLED** (unlike its
`NS_DebuffCast` counterpart, where it is enabled) — its Strength 2500 / Frequency 15 / Seed 11 values
are inert. `Sparkles_Dark`'s `Cylinder Location` carries an **empty `Lathe Profile` curve override**.

---

## 3. Mesh geometry

**N/A — NS_DebuffLoop has no mesh renderer.** All nine renderers are sprites.

---

## 4. Material family and per-instance deltas `[corpus]`

**All six materials are instances of `/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd`.**
`MD_Surface`, `BLEND_Translucent`, `MSM_Unlit`, `twoSided: false`, outputs `EmissiveColor` + `Opacity`,
dynamic channels `[dissolve, distortion, offset, core_color]`, 161 expressions each.

Reference = `M_VFX_DisAdd_Part01`; absolute defaults in [NS_BuffCast.md](NS_BuffCast.md) §4.

| Material | Δ vs `Part01` | Used by |
|---|---|---|
| `M_VFX_DisAdd_Part01` | (reference) | Glow_01, Glow_02, Glow_03 |
| `M_VFX_DisAdd_Part01_Bright` | `Brightness` 1 → **10**; `Core_Intensity` 0 → **1**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Part_02` | Sparkles_Dark |
| `M_VFX_DisAdd_Part02` | `Glow_Intensity` 1 → **0.3**; Main/Color/Dissolve_Tex → `T_VFX_Part_02` | Flares |
| `M_VFX_DisAdd_Ring01` | `Brightness` 1 → **10**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Ring_01` | Ring |
| `M_VFX_DisAdd_Arrows` | `Brightness` 1 → **10**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Arrow_01` | Arrow_Green, Arrow_Purple |
| `M_VFX_DisAdd_Flames01` | `Brightness` 1 → **10**; `Core_Intensity` 0 → **1**; `Dissolve` 0 → **−0.1**; `Dissolve_Scale_X/Y` 1 → **2**; `Distortion_Intensity` 0 → **0.5**; `Distortion_Scale_X/Y` 1 → **2**; `Distortion_Speed_X/Y` 0 → **−0.3**; `Glow_Intensity` 1 → **2**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Main_Tex`/`Color_Tex` → `T_VFX_Wind_01`; `Dissolve_Tex`/`Distortion_Tex` → `T_VFX_Noise_04`; `Color_Core` → **`RGBA(0.015996, 0.014444, 0.014444, 1)`** | Flames |

**Six emitters share three materials**, and `Arrow_Green` / `Arrow_Purple` are the same material
distinguished ONLY by their particle-colour curve (§5). That is the cleanest illustration in the batch
that a "layer" and a "look" are different things — two layers, one look.

**No `Rainbow` instance and no colour LUT here** — this is the only Buff/Debuff/Heal system in the
batch with no gradient-map dependency. That removes the batch's hardest shader gap for this effect.

### Referenced textures `[corpus]`

| Texture | Size | Source format | Compression | sRGB | Address |
|---|---|---|---|---|---|
| `T_VFX_Part_01` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Part_02` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Ring_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Arrow_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Wind_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap — **the 2×2 flipbook atlas** |
| `T_VFX_Noise_02` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Noise_04` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_WhitePixel` | 1×1 | TSF_RGBA16 | TC_Default | true | Wrap/Wrap (inert gradient map) |

---

## 5. Per-layer runtime curves `[corpus]`

Curves sample **NormalizedAge** unless stated. `C` = constant (step) key, `L` = linear key.
`Dynamic Material Parameters` writes **Index 0 only** on every emitter.

### Layer 0 — `Sparkles_Dark` (Once, 0.3 s, rate 4/s)
- Lifetime `[unresolved]` — `1 / 1.5` vs `0.2 / 0.4`.
- **`Cylinder Location`**: Radius **120**, Height **150**, Midpoint 0.5, Offset **(0, 0, 30)**,
  Random, Spawn Only, `Surface Only = false`, `Override Local Rotation = true`,
  `Use Endcaps In Surface Only Mode = true`, Non Uniform Scale (1,1,1), Apply Owner Scale (0,0,0),
  **empty `Lathe Profile` curve override**.
- `Add Velocity` = `Random Range Vector` **Min `(0, 0, -200)` → Max `(0, 0, -700)`** — downward, and
  note **Min is the *smaller magnitude*** while Max is the larger negative; treat the two as per-axis
  lo/hi as exported.
- Sprite Size Mode Random Uniform: **Min 7, Max 15**. Sprite Rotation Random, authored 90, Min 0 / Max 360.
- Initialize Color `RGBA(1, 1, 1, 1)`.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.1)C (1, 3.91223e-08)C`.
- **`Curl Noise Force` is DISABLED** (Strength 2500, Frequency 15, Seed 11, Randomization Vector
  (0.65, 0.125, 0.37), cone 45/45 — all inert).
- Color from Curve — near-black:
  - `R: (0.113492, 0.00102734)C (0.385149, 0.000465223)L`
  - `G: (0.113492, 0.002)C (0.385149, 0.000531684)L`
  - `B: (0.113492, 0.00105217)C (0.385149, 0.002)L`
  - `A: (0, 0)L (0.0857229, 1)C (1, 0)L`
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`.
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 1 — `Ring` (Once, 0.3 s, rate 3/s)
- Lifetime Mode Random, **Min 1.0 / Max 2.0** — unambiguous (no override).
- Sprite Size Mode Random Uniform: **Min 200, Max 300** (`Uniform Sprite Size 200` not selected).
- Sprite Rotation Random, Min 0 / Max 360. Initialize Color `RGBA(1, 1, 1, 1)`.
- Color from Curve — constant near-black with an alpha triangle:
  - `R: (0, 0.00182116)C` *(single key)* | `G: (0, 0.00182116)C` | `B: (0, 0.00212469)C`
  - `A: (0, 0)C (0.499849, 1)L (1, 0)C`
- Dynamic param 1 animated: `Float from Curve` **`(0, 0)C (1, -0.5)C`**. Params 2/3/4 constant 0.
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (0.1, 0.9)C (1, 1)C`.

### Layer 2 — `Flames` (Once, 0.3 s, rate 5/s, spawn probability enabled) — **the sub-UV layer**
- Lifetime `[unresolved]` — `1 / 2` vs `0.2 / 0.4`.
- `Sphere Location`: `Surface Only = true`, `Surface Expansion Mode = Outside`, **Radius 20**,
  `Radius Position 1`, `U Position 0`, `V Position 0.5`, `Uniform Distribution 1`, `Uniform Spiral Amount 1`.
- **`Sub UV Animation`, `Mode = Random`**, `Start Frame 0`, `End Frame 3`, `SubUV Loop Count 1`.
- Sprite Size Mode Random Uniform: **Min 200, Max 300**. Sprite Rotation Random, authored 90.
- **No velocity module** — flames hold their spawn position on the 20-unit shell.
- Color from Curve:
  - `R: (0.113492, 0.175111)C (0.545729, 0.0144)L (0.984002, 0.00560539)L`
  - `G: (0.113492, 0.250158)C (0.545729, 0.0144)L (0.984002, 0.00802319)L`
  - `B: (0.113492, 0.175111)C (0.545729, 0.048)L (0.984002, 0.00560539)L`
  - `A: (1, 1)L` *(single key)*
- Dynamic param 1 animated: `Float from Curve` **`(0, -4.10064e-08)C (1, -1)C`**;
  **`Param 2 = 10` constant** (the distortion channel, matching `Flames01`'s `Distortion_Intensity 0.5`);
  `Param 3`/`Param 4` constant 0.
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (1, 1)C`.
- **`Sprite Rotation Rate` = `Random Range Float 001`, Min −45 / Max 45** °/s.

**Layer 2 is byte-for-byte the same `Flames` emitter as `NS_DebuffCast`'s layer 8**, except for the
spawn module (rate 5/s + probability here, burst 5 there). One behavior layer and one look serve both.

### Layer 3 — `Glow_01` (Infinite, rate 2/s)
- Initialize Color `RGBA(1, 1, 1, 0.4)`; Lifetime **2.0**; Uniform Sprite Size **500**.
- Color from Curve — **a green→violet→green pulse**:
  - `R: (0.150921, 0.093059)C (0.457591, 0.111932)L (0.772714, 0.093059)C`
  - `G: (0.150921, 0.181164)C (0.457591, 0.0409152)L (0.772714, 0.181164)C`
  - `B: (0.150921, 0.0953075)C (0.457591, 0.3564)L (0.772714, 0.0953075)C`
  - `A: (0, 0)C (0.172653, 1)L (0.754603, 1)L (1, 0)L`
  Those two endpoint colours are **exactly `NS_DebuffCast`'s `Sparkles_Bright` Random-Range
  min/max pair** — `RGBA(0.093059, 0.181164, 0.0953075, …)` and `RGBA(0.111932, 0.0409152, 0.3564, …)`.
  The Debuff palette is one green + one violet, reused as a range in one system and as a curve here.
- **No size curve** (3 update modules).
- Dynamic params: **`[1, 0, 0, 0]`** constant.

### Layer 4 — `Glow_02` (Infinite, rate 4/s)
- Initialize Color `RGBA(1, 1, 1, 1)`; Lifetime **1.0**; Uniform Sprite Size **250**.
- Color from Curve (same palette, alpha keys differ slightly from layer 3):
  - `R: (0.150921, 0.093059)C (0.457591, 0.111932)L (0.772714, 0.093059)C`
  - `G: (0.150921, 0.181164)C (0.457591, 0.040915)L (0.772714, 0.181164)C`
  - `B: (0.150921, 0.095307)C (0.457591, 0.3564)L (0.772714, 0.095307)C`
  - `A: (0, 0)C (0.176275, 1)L (0.752188, 1)L (1, 0)L`
- **No size curve.**
- Dynamic params: **`[1, 0, 0, 0]`** constant.

### Layer 5 — `Glow_03` (Infinite, rate 4/s)
- Initialize Color **`RGBA(0.00913406, 0.00334653, 0.0331048, 0.4)`** (a very dark violet);
  Lifetime **1.0**; Uniform Sprite Size **1000** — the largest sprite in the batch.
- **Only 2 update modules: `Particle State` and `Dynamic Material Parameters`.**
  No colour curve, no size curve, no alpha envelope — a static dark bloom that pops on and off with
  its 1 s lifetime.
- Dynamic params: **`[0.7, 0, 0, 0]`** constant.

### Layer 6 — `Arrow_Green` (Infinite, rate 4/s, spawn probability rand 0.5–1.0)
- **`Lifetime Mode = Direct Set`** with `[override] Lifetime = Random Range Float` **Min 0.6 / Max 1.0**
  — unambiguous (see §2).
- **Position Offset `(0, 0, -119.316)`**, `UsePositionOffset = true`.
- `Cylinder Location`: Radius **80**, Height **130**, Midpoint 0.5, **Offset `(0, 0, 150)`** — spawned
  high, unlike the Buff siblings' +30.
- `Add Velocity` = `Random Range Vector` **Min `(0, 0, -150)` → Max `(0, 0, -300)`** — **downward**.
- Sprite Size Mode Non-Uniform, `Sprite Size (90, 150)`. Sprite Rotation Random, Min 0 / Max 360.
- `Mesh Renderer Info` DI override on Initialize Particle (inert — this emitter has a sprite renderer).
- Scale Velocity: `X/Y/Z: (0, 1)C (1, 0.2)C` — a simple two-key decay.
- Color from Curve — **the green arrow**:
  - `R: (0.113492, 0.0165)C (0.46725, 0.00116306)L`
  - `G: (0.113492, 0.03)C (0.46725, 0.00132921)L`
  - `B: (0.113492, 0.0169417)C (0.46725, 0.005)L`
  - `A: (0, 0)L (0.085723, 1)C (1, 0)L`
- Scale Sprite Size (Uniform Curve): `(0, 1)C (1, 0.4)C`.
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 7 — `Arrow_Purple` (Infinite, rate 4/s, spawn probability rand 0.5–1.0)
**Identical to layer 6 in every parameter** — same cylinder (80 / 130 / +150), same velocity range,
same size (90, 150), same lifetime 0.6–1.0, same size curve, same alpha envelope, same material —
**except the colour curve**:
- `R: (0.113492, 0.0137272)C (0.487775, 0.00116306)L`
- `G: (0.113492, 0.009)C (0.487775, 0.00132921)L`
- `B: (0.113492, 0.03)C (0.487775, 0.005)L`
- `A: (0, 0)L (0.085723, 1)C (1, 0)L`
(The mid-key time also differs: 0.487775 here vs 0.46725 on the green arrow.)

### Layer 8 — `Flares` (Infinite, rate 6/s)
- Initialize Color **`RGBA(0.00802319, 0.00402472, 0.0241576, 0.8)`** with
  `Color Mode = **Direct Set**` — note this is the Debuff variant; `NS_BuffLoop`'s `Flares` uses
  `Random Hue/Saturation/Value`. The HSV parameters are still present in the store
  (`Hue Shift Range (0.5, 0.8)`, `Saturation Range (0.2, 0.2)`, `Value Range (1, 1)`,
  `Alpha Scale Range (0.13, 0.13)`, `Color Min/Max` black/white) but are **inert in Direct Set mode**.
- Lifetime `[unresolved]` — `Lifetime Min 1 / Max 2` vs override `RandomRangeFloat 0.2 / 0.4`.
- `Cylinder Location`: Radius **90**, Height **130**, Midpoint 0.5, Offset **(0, 0, 30)**.
- **No velocity module.**
- Sprite Size Mode Random Uniform: **Min 50, Max 200**.
- Scale Sprite Size (Uniform Curve): **`(0, -2.98023e-09)C (0.2, 0.6)C (1, 1)C`** — grows to full over
  its life (the Buff sibling's Flares peaks at 0.1 then decays; this one only grows).
- Scale Color, `Scale Mode = RGB and Alpha Separately`:
  - `Scale RGBA` curve: `R (0, 1)L (1, 1)L | G (0, 1)L (1, 1)L | B (0, 1)L (1, 1)L | A (0, 0)L (0.236644, 1)L (1, 0)L`
  - `Scale Alpha` (Float from Curve): **`(0, 0)L (0.3, 0.125)L (1, 0)L`**
  In "RGB and Alpha Separately" mode the separate `Scale Alpha` drives alpha, so the effective alpha
  envelope peaks at **0.125** `[inferred — module semantic, not a corpus fact]`.
- Dynamic params: **`[8, 0, 0, 0]`** constant.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence

**Same continuous-cadence problem as [NS_BuffLoop.md](NS_BuffLoop.md) §6.1, plus a mixed-loop wrinkle.**
Six emitters stream forever at a combined 24/s (two of them probability-gated); three stream for a
single 0.3 s window and stop. The existing continuous row ignores `LoopDuration`/`ParticleLifetime`
and has no spawn-rate field; no row can express "some layers loop, some fire once".

Recommendation, same as its Buff sibling: **extend `FCk_ParticlesTemplateSpec` with a spawn rate and a
real lifetime for continuous rows**, then run one 24/s stream with lifetime 2.0 and partition by a
rate-weighted `Seed % N` (Glow_01 2, Glow_02 4, Glow_03 4, Arrow_Green 4, Arrow_Purple 4, Flares 6 ⇒
N = 24). The three `Once` layers then have to be either dropped or folded into the first loop only —
neither is expressible today, and the fold is not visually equivalent.

**Spawn probability** (`Arrow_Green` / `Arrow_Purple`, random 0.5–1.0 per spawn) is expressible inside
a behavior: a slot whose `CkParticles_Rand(Seed, salt)` exceeds the drawn probability writes zero
colour/size. It costs particles that render nothing, which is exactly what the source does.

### 6.2 VisTag / renderer needs

| Renderer | Kind | Look | Source emitters |
|---|---|---|---|
| a | CameraFacingSprite *(NEW KIND REQUIRED)* | `PartDisAdd01` | Glow_01, Glow_02, Glow_03 |
| b | CameraFacingSprite *(new kind)* | `PartDisAdd01Bright` | Sparkles_Dark |
| c | CameraFacingSprite *(new kind)* | `RingDisAdd01` | Ring |
| d | CameraFacingSprite *(new kind)* + **SubUV 2×2** | `FlamesDisAdd01` | Flames |
| e | CameraFacingSprite *(new kind)* | `PartDisAdd02` | Flares |
| f | VelocityAlignedSprite | `ArrowsDisAdd` | Arrow_Green, Arrow_Purple |

**Six row renderers, five needing the new `CameraFacingSprite` kind, one of those additionally needing
sub-UV.** Every look here is shared with another sheet in the batch — nothing is unique to this effect.

### 6.3 Look / material needs

Six materials → **five looks** (`Arrows` serves two layers). All are `DissolveAdd` instances.
Unplumbed family parameters that bite:

| Param | Value here | Plumbed? |
|---|---|---|
| `Core_Intensity` | 1 on `Part01_Bright`, `Flames01` | **no** |
| `Glow_Intensity` | 0.3 on `Part02`, 2 on `Flames01` | **no** |
| `Gradient_Invert` | 0 on `Ring01`, `Flames01` | **no** |
| gradient-map LUT | **not used in this system** | n/a — **the one Buff/Debuff/Heal system with no LUT dependency** |

`Flames01`'s distortion/dissolve-scale/`Color_Core` set is fully plumbed already.

### 6.4 Texture needs

`T_VFX_Part_01` → existing `SoftParticle`; `T_VFX_Noise_02` → existing `TileNoise`;
`T_VFX_Noise_04` **unmeasured**. New measurement + bake for `T_VFX_Part_02`, `T_VFX_Ring_01`,
`T_VFX_Arrow_01`, and **`T_VFX_Wind_01` as a 2×2 four-frame ATLAS**. Method: NS_BasicAttack §7.

### 6.5 CAPABILITY GAPS — read before committing a session

1. **CONTINUOUS-CADENCE ROWS ARE NOT PARAMETERIZABLE.** Nine spawn-rate emitters, no rate field.
   Shared with `NS_BuffLoop` and `NS_HealLoop`; fix it once for the three.

2. **MIXED `Once` / `Infinite` LOOP BEHAVIOUR IN ONE SYSTEM — not expressible.** Three emitters fire
   for 0.3 s and stop while six loop forever. A CkParticles template has one cadence.

3. **SUB-UV / FLIPBOOK — does not exist.** `Flames` again (identical emitter to `NS_DebuffCast`'s).
   Needs: renderer `SubImageSize`, a DI `SubImageIndex` output, an atlas-capable texture bake, and a
   sub-UV field on `FCk_ParticlesRendererSpec`. See [NS_DebuffCast.md](NS_DebuffCast.md) §6.5.1.

4. **CAMERA-FACING SPRITE ROW RENDERER — does not exist.** Five layers here.

5. **SPAWN PROBABILITY** — expressible in-behavior (§6.1), no pipeline gap, but it makes the
   *effective* particle count non-deterministic per loop, which any anti-vacuity test must tolerate.

6. **WORLD SPACE vs LOCAL SPACE.** All nine emitters `LocalSpace: false`; template is local-space.
   As with `NS_BuffLoop`, this matters more for a Loop effect than a Cast one because a debuff loop is
   typically parented to a moving character. Flag it to the maintainer.

7. **SPRITE ROTATION + ROTATION RATE.** Six layers use random sprite rotation and `Flames` adds a
   per-particle constant −45..+45 °/s rate. `OutRotation` exists on the DI;
   `[unresolved: whether it is bound on camera-facing sprite renderers]` — verify in
   `CkParticles_TemplateBuilder.cpp`.

8. **No ribbon, no events, no mesh, no LUT, no forces here.** `Sparkles_Dark`'s curl-noise force is
   DISABLED, which is what keeps this effect off the "needs a noise-field solver" list that its
   `NS_DebuffCast` sibling lands on. **This is the simplest system in the batch on the shader and
   physics axes.**

### 6.6 Behavior id

**Do NOT allocate an id here.** `ck::particles::NumBehaviors` is 18 at the time of writing.

### 6.7 Complexity assessment

**Tier L**, but only just, and for two reasons that are both shared with siblings: the
continuous-cadence gap (1) and sub-UV (3). **Drop the `Flames` flipbook and it becomes M**;
solve the continuous-cadence row for the whole Loop trio and this is the cheapest of the three to
finish. Its curves and forces are the simplest in the batch.

---

## 7+. Reserved for implementation — sections 7–14 per [README.md](README.md) are written by the session that implements this effect.
