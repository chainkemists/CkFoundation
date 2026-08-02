# Translation sheet: NS_DebuffLoop → CkParticles (PRE-IMPLEMENTATION)

Schema and evidence-tag conventions: [README.md](README.md). Exemplars: [NS_BasicAttack.md](NS_BasicAttack.md),
[NS_Lightning_Range.md](NS_Lightning_Range.md).

## Completion state — READ FIRST

**Status: IMPLEMENTATION-COMPLETE (2026-08-02) as CkParticles behavior 29, `DebuffLoop`.
NOT visually verified — the §12 human A/B has not been run.**

§1–6 are archaeology against the extracted corpus, re-verified against the v3 sidecar at implementation
time. **Two things were settled in place:**

1. **`Flames`' `[unresolved]` spawn probability is RESOLVED to 1** — the value is in the emitter's
   `[values]` block. The flag is on, the probability is 1, and the layer is ungated. Only the two arrow
   emitters are genuinely probability-gated.
2. **The per-emitter `Color.Scale Alpha` values were missing** (§2). Two are severe — `Flames` at **0.05**
   and `Ring` at **0.35** — so a port that trusted §5 alone would have rendered this effect's two most
   distinctive layers at 20x and 3x their source coverage.

§6.1's `[P0-D3 STOP]` is RESOLVED: campaign decision **[P0-D4]** routed the four rate-only Loop systems
through Phase 2's C2 spawn-rate rows. §6.5 gap 3 (sub-UV) had already been closed by Phase 1's C4, and
gap 4 (the camera-facing row renderer) by Phase 1's C1 — both were spent by earlier batches, which is why
this "Tier L, but only just" sheet ported as the cheapest of the four.

§7 onward is what was actually built.

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
Three of them *store* `Loop Behavior = Once` on a 0.3 s loop, the other six store
`Loop Behavior = Infinite` on a 1.0 s loop. All nine are `Life Cycle Mode = System`.

**System loop `[corpus-v3]`: `Loop Behavior = Infinite`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
Per [P0-D1] this RULES all nine — **every per-emitter Loop row in the table below is inert, including
the three `Once / 0.3 s` rows.** All nine therefore stream continuously on the system's 2.0 s
infinite loop; "three fire once and stop" was an artefact of trusting the inert emitter rows. The
rows are kept in the table as authored leftovers, struck through.

| # | Emitter | Loop *(stored, INERT)* | Loop dur *(inert)* | Rate (/s) | Spawn probability | Lifetime | Renderer | Material |
|---|---|---|---|---|---|---|---|---|
| 0 | `Sparkles_Dark` | ~~Once~~ | ~~0.3~~ | 4 | — | rand **1.0–1.5** `[corpus-v3]` | Sprite, Unaligned/FaceCamera | `Part01_Bright` |
| 1 | `Ring` | ~~Once~~ | ~~0.3~~ | 3 | — | rand 1.0–2.0 `[corpus-v3]` | Sprite, Unaligned/FaceCamera | `Ring01` |
| 2 | `Flames` | ~~Once~~ | ~~0.3~~ | 5 | **enabled** (value `[unresolved]`) | rand **1.0–2.0** `[corpus-v3]` | Sprite, Unaligned/FaceCamera, **SubUV 2×2** | `Flames01` |
| 3 | `Glow_01` | Infinite | 1.0 | 2 | — | 2.0 | Sprite, Unaligned/FaceCamera | `Part01` |
| 4 | `Glow_02` | Infinite | 1.0 | 4 | — | 1.0 | Sprite, Unaligned/FaceCamera | `Part01` |
| 5 | `Glow_03` | Infinite | 1.0 | 4 | — | 1.0 | Sprite, Unaligned/FaceCamera | `Part01` |
| 6 | `Arrow_Green` | Infinite | 1.0 | 4 | **rand 0.5–1.0** | rand 0.6–1.0 | Sprite, **VelocityAligned**/FaceCamera | `Arrows` |
| 7 | `Arrow_Purple` | Infinite | 1.0 | 4 | **rand 0.5–1.0** | rand 0.6–1.0 | Sprite, **VelocityAligned**/FaceCamera | `Arrows` |
| 8 | `Flares` | ~~Infinite~~ | ~~1.0~~ | 6 | — | rand **1.0–2.0** `[corpus-v3]` | Sprite, Unaligned/FaceCamera | `Part02` |

**Steady-state rate `[corpus-v3]`: all nine emitters stream — 4+3+5+2+4+4+4+4+6 = 36 particles/second**,
reduced on the two arrow emitters by their random spawn probability.
*(Was "24/s from six infinite emitters + 3.6 one-time particles from three `Once` emitters" — that
split came from the inert emitter Loop rows.)*

`Flames` sets `Use Spawn Probability = true` with no override — **RESOLVED `[corpus]` (2026-08-02):** its
store DOES carry `Flames.SpawnRate.Spawn Probability = 1`, so the flag is on but the probability is 1 and
the layer is ungated. *(Was `[unresolved: the effective probability]`; the value is in the `[values]`
block, which the earlier read did not reach.)* The two arrow emitters are the only gated layers — both
override it with `Random Range Float 001` Min 0.5 / Max 1.0.

**`Color` module `Scale Alpha` per emitter `[corpus]`** — from the `[values]` blocks, added at
implementation time because §5 does not carry them and two of them are severe: `Sparkles_Dark` 1,
`Ring` **0.35**, `Flames` **0.05**, `Glow_01` **0.8**, `Glow_02` **0.8**, `Arrow_Green` 1,
`Arrow_Purple` 1. `Glow_03` and `Flares` have no `Color` module (Glow_03 has only two update modules at
all; Flares uses `Scale Color`).

> ### Lifetime — RESOLVED `[corpus-v3]` (this system is the [C-D1] discriminating case)
> Per [P0-D2] the `Lifetime Mode` static switch selects the driving pin: `Random` ⇒ Min/Max,
> `Direct Set` ⇒ the `Lifetime` pin (or an override sitting on it). This system contains BOTH shapes,
> which is what made it the discriminator:
>
> | Emitter | Mode | LIVE | inert |
> |---|---|---|---|
> | `Sparkles_Dark` | Random | **1.0 / 1.5** | ~~override 0.2 / 0.4~~ |
> | `Flames` | Random | **1.0 / 2.0** | ~~override 0.2 / 0.4~~ |
> | `Flares` | Random | **1.0 / 2.0** | ~~override 0.2 / 0.4~~ |
> | `Ring` | Random | **1.0 / 2.0** | stored `Lifetime = 1` |
> | `Arrow_Green` / `Arrow_Purple` | **Direct Set** | **override `Random Range Float` 0.6 / 1.0** | — |
>
> The arrows export as `lifetimeResolved.source = "override"`; the four Random-mode emitters export as
> `source = "minmax"` with the override under `inertOverrides`. The sheet's `[inferred]` reading is
> MECHANICALLY CONFIRMED.

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
- Lifetime `[corpus-v3]` — **`1.0 / 1.5` drives**; the `0.2 / 0.4` override is inert.
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
- Lifetime `[corpus-v3]` — **`1.0 / 2.0` drives**; the `0.2 / 0.4` override is inert.
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
- Lifetime `[corpus-v3]` — **`Lifetime Min 1.0 / Max 2.0` drives**; the `RandomRangeFloat 0.2 / 0.4` override is inert.
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

**`[P0-D3 STOP: loop = 2.0 s (system, Infinite/Fixed); lifetime = 2.0 s (max resolved — Glow_01
direct 2.0, Ring/Flames/Flares 2.0 Max); burst = 0 — rate-only system, no burst module anywhere and
§2 carries no burst count]`.** The formula yields a `BurstCount == 0` continuous row that the cadence
table cannot parameterize today. Orchestrator ruling required before a row is written.

**Same continuous-cadence problem as [NS_BuffLoop.md](NS_BuffLoop.md) §6.1.** `[corpus-v3]` The
mixed-loop wrinkle is GONE: all nine emitters are system-governed and the system loops
`Infinite / 2.0 s`, so this is one uniform continuous stream at **36/s** (two layers
probability-gated), not six loops plus three one-shots. The existing continuous row still ignores
`LoopDuration`/`ParticleLifetime` and has no spawn-rate field.

Recommendation, same as its Buff sibling: **extend `FCk_ParticlesTemplateSpec` with a spawn rate and a
real lifetime for continuous rows**, then run one **36/s** stream with lifetime 2.0 and partition by a
rate-weighted `Seed % N` (Sparkles_Dark 4, Ring 3, Flames 5, Glow_01 2, Glow_02 4, Glow_03 4,
Arrow_Green 4, Arrow_Purple 4, Flares 6 ⇒ N = 36). *(Was a 24/s six-layer stream with the three
`Once` layers "dropped or folded" — they are ordinary streaming layers.)*

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

## 7. Textures — no new bake

§6.4 asked for measurement and new bakes for four paints including a 2×2 wind ATLAS. **None were needed:**
every one had been measured off the same corpus PNG by an earlier batch, and the atlas kind itself
(`MaskSheet`) shipped with Phase 1's C4.

| Source paint | Stand-in | Measured in |
|---|---|---|
| `T_VFX_Part_01` | `SoftParticle` | NS_BasicAttack §7 |
| `T_VFX_Part_02` | `SoftParticleBright` | NS_FireBall_Hit §7 |
| `T_VFX_Ring_01` | `RingUneven` | NS_FireBall_Hit §7 (the SDF `Ring` bake was measured and rejected there) |
| `T_VFX_Wind_01` | `WindSheet` — the 2×2 four-frame atlas | NS_Fire §7 |
| `T_VFX_Noise_02` | `TileNoise` | NS_BasicAttack §7 |
| `T_VFX_Noise_04` | `TileNoiseCoarse` | NS_FireBall_Hit §7 (§6.4 listed this one as "unmeasured"; it was measured there) |
| `T_VFX_Arrow_01` | `ArrowChevron` | **NS_BuffLoop §7.1**, this batch — the only new bake in the batch, shared with this port |
| `T_VFX_WhitePixel` | `LutWhite` | Phase 1 C3 |

§4's "this is the only Buff/Debuff/Heal system with no gradient-map dependency" holds, so this port is
also the only one of the four with **no [P1-D1] hold** on any layer.

---

## 8. Mesh

**None.** All nine source renderers are sprites: seven camera-facing (one of them a 2×2 flipbook) and two
velocity-aligned.

---

## 9. The behavior — `Behavior_DebuffLoop.ush` + `ExecuteStage_CPU` case 29

### 9.1 The cadence row

| Field | Value | Source |
|---|---|---|
| `LoopDuration` | 2.0 s | the SYSTEM's `Loop Behavior = Infinite`, `Loop Duration = 2` ([P0-D1]) |
| `ParticleLifetime` | 2.0 s | Glow_01's Direct-Set 2.0 and the Ring / Flames / Flares 2.0 maxima |
| `BurstCount` | 0 | there is no burst module anywhere in the system |
| `SpawnRate` | 36 /s | 4+3+5+2+4+4+4+4+6 |

### 9.2 The partition

A weighted draw against `CkParticles_Rand(Seed, 0)` over cumulative rate shares in the source's emitter
order — see NS_PickupLoop §9.2. Worst per-layer deviation over 400 000 seeds: **0.00080**.

### 9.3 Spawn probability, reproduced rather than dropped

`Arrow_Green` and `Arrow_Purple` each draw a probability from `Random Range Float 001` (0.5 … 1.0) per
spawn and then spawn against it. §6.1 prescribed the shape and the behavior follows it exactly: draw the
probability from salt 11, draw against it with salt 12, and hide the slot that loses. That costs particles
which render nothing — **which is precisely what the source spends**, since a Niagara spawn that fails its
probability check is a spawn that did not happen.

E[1 − P] with P uniform on [0.5, 1] is exactly **0.25**, and the measured loss over 4 000 slots per arrow
is 0.2612 / 0.2542. §6.5 gap 5's warning stands: the *effective* particle count is non-deterministic per
loop, which is why §11's anti-vacuity and partition checks scan several seeds per layer rather than one.

### 9.4 Per-layer notes worth the reader's time

- **`Sparkles_Dark`'s `Curl Noise Force` is DISABLED** — its Strength 2500 / Frequency 15 / Seed 11 values
  are inert, and the DebuffCast sibling has the same module ENABLED. This is the one place in the batch
  where Phase 2's C10 curl helper would have been the right tool and the source says not to use it.
- **The two arrows are ONE look told apart by colour alone** — same cylinder, same velocity range, same
  size, same lifetime, same size curve, same alpha envelope, same material. Only the colour curve and one
  mid-key time differ. One body serves both.
- **`Arrow_*` composes two placement terms**: a cylinder at radius 80 / height 130 lifted **+150** — much
  higher than the Buff siblings' +30 — and Initialize Particle's `Position Offset (0, 0, -119.316)`. They
  spawn high and FALL.
- **`Flames` sits on a 20-unit sphere SHELL with no velocity at all** and turns in place at a per-particle
  ±45 °/s, over a 2×2 flipbook whose start frame is a random draw (the same `Sub UV Animation` idiom the
  four earlier sub-UV ports use).
- **`Glow_03` is the only layer in the cookbook with no curve of any kind** — two update modules, so its
  Initialize colour renders unchanged for its whole second. The largest sprite in the cookbook (1000
  units) pops on and off rather than fading.
- **`Flares` DIRECT-SETS its colour** where its BuffLoop sibling randomizes the hue; the HSV parameters in
  its store are inert in that mode. It is also the only Flares in the batch that only GROWS.

---

## 10. Looks and renderers

Six row-declared renderers on VisTags **91–96** — five camera-facing sprites (one a 2×2 sub-UV sheet) and
one velocity-aligned serving BOTH arrow emitters.

| VisTag | Kind | Look | Source material | Serves |
|---|---|---|---|---|
| 91 | CameraFacingSprite | `PartDisAdd01` | `M_VFX_DisAdd_Part01` | Glow_01, Glow_02, Glow_03 |
| 92 | CameraFacingSprite | `PartDisAdd01Bright` | `M_VFX_DisAdd_Part01_Bright` | Sparkles_Dark |
| 93 | CameraFacingSprite | `RingDisAdd01` | `M_VFX_DisAdd_Ring01` | Ring |
| 94 | CameraFacingSprite, SubUV 2×2 | `FlamesDisAdd01` | `M_VFX_DisAdd_Flames01` | Flames |
| 95 | CameraFacingSprite | `PartDisAdd02` | `M_VFX_DisAdd_Part02` | Flares |
| 96 | VelocityAlignedSprite | `ArrowsDisAdd` | `M_VFX_DisAdd_Arrows` | Arrow_Green, Arrow_Purple |

**This port authors no look of its own** — `ArrowsDisAdd` is NS_BuffLoop's, and the other five predate the
batch. Nine source emitters, six renderers, five of them shared: §4's "the cleanest illustration in the
batch that a layer and a look are different things" survives implementation intact.

`Get_BehaviorLookName(29)` stays `NAME_None`: every look rides a row renderer that binds it explicitly.

---

## 11. Tests

`Test_Particles_DebuffLoopBehavior.cpp` + the `NumBehaviors` 26 → 30 ratchet in
`Test_Particles_RosterSanity.cpp`.

- **The rate-share sweep** over 400 000 seeds, every layer within **0.004** of its source share.
- **Spawn probability is asserted three ways**: the behavior's gate must agree slot-for-slot with a gate
  the test rebuilds from the source's own two draws (over 4 000 slots per arrow, counted rather than
  asserted per sample); the loss share must land within 0.03 of the exact 0.25; and **every other layer
  must lose nothing** — a gate applied to the wrong band shows up there rather than as a density feel.
- **The two arrows are green-dominant and blue-dominant respectively**, through the same renderer.
- **`Glow_03` never fades and never resizes** — the no-curve claim, made falsifiable, and the one a reader
  is most likely to "fix" by adding an alpha envelope.
- **`Flames` advances its flipbook, stays inside the 2×2 sheet, pins distortion at the source's 10, and
  sits exactly on the 20-unit shell.**
- **`Sparkles_Dark` travels DOWNWARD**, unlike every sparkle stream in the Buff and Heal siblings.
- Plus the standard per-layer anti-vacuity and death checks, which scan eight seeds per layer rather than
  one so the probability-gated bands cannot pass or fail on a single draw.

---

## 12. Verification — A/B protocol

`[HUMAN-VERIFY]` — **not yet run.** Open the **VfxExamples** gym, station pair **DEBUFF LOOP**.

> **This pair is a STEADY-STATE comparison, not a synced replay.** `NS_DebuffLoop` is an INFINITE system:
> it never finishes, so the harness's `OnSystemFinished` re-arm never fires and the two sides are never in
> phase. Judge density, palette and motion character over a few seconds; do NOT expect matched frames.
> The two arrow layers are probability-gated, so the two sides' particle counts will not match instant to
> instant even in principle.

| # | Criterion | Look for |
|---|---|---|
| a | Overall read | a dark, oppressive column — almost every colour in this system is near-black, and the effect reads by SILHOUETTE and bloom rather than by hue |
| b | Density | ~36/s nominal, minus about a quarter of the two arrow streams |
| c | Arrows | chevrons FALLING from about 30 units above the spawn point, one green and one violet, stretched along their motion. They should visibly thin out relative to a steady stream |
| d | Flames | 200–300 unit puffs on a tight 20-unit shell, turning slowly in place, cycling a four-frame flipbook, and **very faint** (5 % coverage) |
| e | Ring | 200–300 unit halos at 35 % coverage, growing and dissolving outward |
| f | Glows | three shells at 500 / 250 / **1000** units. The 1000 is a static dark-violet bloom that **pops on and off** rather than fading — if it fades, §13.4 is broken |
| g | Sparkles | near-black motes FALLING out of a wide cylinder |
| h | Flares | a very dark violet haze that only GROWS over its life (the Buff sibling's peaks early and decays) |
| i | Palette | one green and one violet, reused everywhere — §5 notes the two endpoint colours are the same pair `NS_DebuffCast` uses as a random range |
| j | No LUT | this is the one system in the batch with no gradient-map dependency, so nothing here is held back by [P1-D1] |
| k | World space | move the pedestal mid-effect if you can (§13.5) |

---

## 13. Confirmed fidelity differences

1. **The layer partition is a weighted draw, not per-emitter independent spawning** — see NS_PickupLoop
   §13.1.
2. **Spawn probability is reproduced by hiding losing slots**, which costs particles that render nothing.
   That matches what the source spends, but it means the port's *allocated* particle count is constant
   where the source's varies — an internal difference with no visual consequence.
3. **`Sparkles_Dark`'s disabled `Curl Noise Force` is NOT implemented**, deliberately, even though Phase 2
   shipped a curl-noise helper that could express it. The DebuffCast sibling enables the same module, so
   implementing it here would import the wrong system's behaviour.
4. **`Glow_03` has no alpha envelope**, by design — two update modules in the source. It pops on and off.
5. **World space.** All nine source emitters are `LocalSpace: false`; the template is local space. §6.5
   gap 6 asks the maintainer to judge which is preferable for a debuff on a moving character.
6. **`Arrow_*`'s authored sprite rotation is inert** on a velocity-aligned renderer and is not written.
7. **Unplumbed family parameters:** `Core_Intensity` (1 on `Part01_Bright` and `Flames01`),
   `Gradient_Invert` (0 on `Ring01` and `Flames01` — inert against the white ramp). **`Glow_Intensity` IS
   reproduced** on both instances that drive it (0.3 on `Part02`, 2 on `Flames01`), folded into
   Brightness. `Flames01`'s distortion / dissolve-scale / `Color_Core` set is fully plumbed.
8. **`In.EmitterAge` is threaded but unread** — see NS_PickupLoop §13.7. The three emitters that store
   `Loop Behavior = Once` on a 0.3 s loop are `Life Cycle Mode = System`.
9. **Every stand-in texture is a statistical match of the source paint, not a copy** (§7).

---

## 14. Reusable lessons

1. **An `[unresolved]` is often one `[values]` block away.** `Flames`' effective spawn probability was
   marked unresolved because the module listed no override; the store carried the number all along. Read
   the `[values]` block before recording an unknown.
2. **A disabled module is a decision, not an omission — and a shipped capability does not change that.**
   Phase 2 built curl noise; this layer's curl force is off; the port leaves it off. The sibling system
   that enables it is the evidence that the difference is deliberate.
3. **Reproduce a probability gate rather than scaling the rate to match.** Multiplying the arrows' 4/s by
   the mean 0.75 would give the same average density and lose the variance, which is the visible part.
4. **Make "this layer has no curve" a test.** `Glow_03` is two modules; every neighbour has an alpha
   envelope; the pressure to add one is real. `alpha(0.01) == alpha(0.99)` costs one line and holds.
5. **Count, do not assert, inside a four-thousand-sample loop.** The per-slot gate agreement is one
   assertion over a counter, not four thousand formatted ones — the difference between a readable red run
   and an unreadable one.
