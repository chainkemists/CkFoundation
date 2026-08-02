# Translation sheet: NS_HealCast → CkParticles (PRE-IMPLEMENTATION)

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
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_HealCast` |
| Pack | Vefects — *Anime VFX* |
| Corpus system files | `systems/Vefects/Anime_VFX/Shared/Skills/NS_HealCast.{json,txt}` |
| Corpus material files | `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Rainbow,Ring01,Part01_Bright,Part04,Star01,Star02,Part07}.json` |
| Corpus texture files | `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Part_04,Part_08,Ring_01,Ring_02,Star_01,Star_02,Noise_02,LUT_Rainbow_01,WhitePixel}.json` |
| Meshes | none — no mesh renderer |

**The source Niagara asset was never opened.** Every fact below is `[corpus]` unless tagged otherwise.

> ### Sibling with a near-identical name
> `[corpus]` `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Heal_Cast` (underscored) is a different,
> parameterized system, also with 9 emitters. Discriminators: `NS_HealCast` exports
> `userParameters: []` and draws through `M_VFX_DisAdd_*`; `NS_Heal_Cast` exports **eight**
> (`User.Glow Color 01/02`, `User.Lens Color 01`, `User.Rainbow Color 01`, `User.Ring Color 01`,
> `User.Scale Overall`, `User.Sparkles Color 01`, `User.Star Color 01`) and draws through
> `MI_VFX_{Glow_01, Glow_01_Bright, Glow_04, Glow_07, Lens_Rainbow_01, Ring_01, Star_01, Star_02}`.
> This sheet documents the **`Anime_VFX/Shared/Skills`** variant only.

---

## 2. System anatomy `[corpus]`

**9 CPU emitters, ALL enabled, ALL world-space (`LocalSpace: false`), `Bounds: Dynamic`,
`Determinism: false`, zero user parameters.**

Two cadence groups, exactly as in `NS_DebuffCast`:

- **Four `Life Cycle Mode = System`, `Loop Behavior = Infinite`, Loop Duration 1.0** emitters, each
  bursting once per loop.
- **Five `Life Cycle Mode = Self`, `Loop Behavior = Once`** emitters on a **0.3 s** loop (0.2 s for
  `Lens`), each carrying **both** a `Spawn Burst Instantaneous` **and** a `Spawn Rate` module.

| # | Emitter | Life Cycle / Loop | Loop dur | Burst | Burst t | Rate (/s) | Lifetime | Renderer | Material |
|---|---|---|---|---|---|---|---|---|---|
| 0 | `Bomb_Glow_01` | System / Infinite | 1.0 | 1 | 0 | — | 1.0 | Sprite, Unaligned/FaceCamera | `Part01` |
| 1 | `Bomb_Glow_02` | System / Infinite | 1.0 | 1 | 0 | — | 1.0 | Sprite, Unaligned/FaceCamera | `Part01` |
| 2 | `Raimbow` *(sic)* | System / Infinite | 1.0 | 1 | 0 | — | 1.0 | Sprite, Unaligned/FaceCamera | `Rainbow` |
| 3 | `Ring` | System / Infinite | 1.0 | 1 | **0.05** | — | **0.5** | Sprite, Unaligned/FaceCamera | `Ring01` |
| 4 | `Sparkles_01` | **Self / Once** | **0.3** | **3** | **0.05** | **20** | rand `[unresolved]` | Sprite, Unaligned/FaceCamera | `Part01_Bright` |
| 5 | `Sparkles_Stretched` | **Self / Once** | **0.3** | **5** | **0.05** | **10** | rand `[unresolved]` | Sprite, **VelocityAligned**/FaceCamera | `Part04` |
| 6 | `Star01` | **Self / Once** | **0.3** | **1** | **0.05** | **5** | rand 0.3–0.6 | Sprite, Unaligned/FaceCamera | `Star02` |
| 7 | `Star02` | **Self / Once** | **0.3** | **1** | **0.05** | **5** | rand 0.3–0.6 | Sprite, Unaligned/FaceCamera | `Star01` |
| 8 | `Lens` | **Self / Once** | **0.2** | **3** | **0.05** | **10** | rand 1.0–1.5 | Sprite, **VelocityAligned**/FaceCamera, **SubUV 2×2** | `Part07` |

**Note the naming inversion `[corpus]`, do not "fix" it:** emitter `Star01` draws with
`M_VFX_DisAdd_Star02`, and emitter `Star02` draws with `M_VFX_DisAdd_Star01`. Same class of authored
skew as NS_BasicAttack's `Slash_03` → `M_VFX_DisAdd_Slash04`.

**Particles per firing (burst only, the exact `[corpus]` number): 1+1+1+1+3+5+1+1+3 = 17.**
The five `Spawn Rate` modules add ≈ 20×0.3 + 10×0.3 + 5×0.3 + 5×0.3 + 10×0.2 = **≈ 17 more**
`[inferred — rate × loop-duration arithmetic]`, so ≈ **34 total per firing**. The four
infinite-loop emitters re-burst every 1.0 s; the five `Once` emitters fire only on the first loop.

`Loop Count Limit = 1` with `UseLoopCountLimit = false` throughout — inert, same trap as the siblings.

> ### `[unresolved: lifetime on two emitters]`
> Same `Random`-mode-vs-`[override] Lifetime` conflict the whole batch has:
>
> | Emitter | `Lifetime Min/Max` (Random mode) | override `RandomRangeFloat` |
> |---|---|---|
> | `Sparkles_01` | **0.3 / 0.6** | 0.2 / 0.4 |
> | `Sparkles_Stretched` | **0.3 / 0.6** | 0.2 / 0.4 |
>
> `Star01`, `Star02` (0.3 / 0.6) and `Lens` (1.0 / 1.5) have **no `Lifetime` override** and are
> unambiguous. `Ring` is `Lifetime Mode = Direct Set` with `Lifetime = 0.5` — its `Lifetime Min 0.3 /
> Max 0.7` entries are inert leftovers.

Three emitters (`Sparkles_01`, `Star01`, `Star02`, `Lens`) carry an **empty `Lathe Profile` curve
override** on their `Cylinder Location`. Several carry `InitializeParticle.Lifetime = 1` alongside
Random ranges — inert.

---

## 3. Mesh geometry

**N/A — NS_HealCast has no mesh renderer.** All nine renderers are sprites.

---

## 4. Material family and per-instance deltas `[corpus]`

**All eight materials are instances of `/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd`.**
`MD_Surface`, `BLEND_Translucent`, `MSM_Unlit`, `twoSided: false`, outputs `EmissiveColor` + `Opacity`,
dynamic channels `[dissolve, distortion, offset, core_color]`, 161 expressions each.

Reference = `M_VFX_DisAdd_Part01`; absolute defaults in [NS_BuffCast.md](NS_BuffCast.md) §4.

| Material | Δ vs `Part01` | Used by |
|---|---|---|
| `M_VFX_DisAdd_Part01` | (reference) | Bomb_Glow_01, Bomb_Glow_02 |
| `M_VFX_DisAdd_Rainbow` | `GradientMap_Displacement` 0.1 → **0.9**; `Gradient_Invert` 0.5 → **2**; `Opacity_Boldness` 0.5 → **1.5**; `Opacty_StepAdd` 0.1 → **0.3**; **`GradientMap_Tex` `T_VFX_WhitePixel` → `T_VFX_LUT_Rainbow_01`**; `GradientShape_Tex` `T_VFX_Noise_02` → `T_VFX_Part_01`; `Main_Tex` → `T_VFX_Ring_02` | Raimbow |
| `M_VFX_DisAdd_Ring01` | `Brightness` 1 → **10**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Ring_01` | Ring |
| `M_VFX_DisAdd_Part01_Bright` | `Brightness` 1 → **10**; `Core_Intensity` 0 → **1**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Part_02` | Sparkles_01 |
| `M_VFX_DisAdd_Part04` | `Brightness` 1 → **6**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **30**; Main/Color/Dissolve_Tex → `T_VFX_Part_04` | Sparkles_Stretched |
| `M_VFX_DisAdd_Star02` | `Brightness` 1 → **6**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **10**; Main/Color/Dissolve_Tex → `T_VFX_Star_02` | Star01 *(the naming inversion)* |
| `M_VFX_DisAdd_Star01` | `Brightness` 1 → **6**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **10**; Main/Color/Dissolve_Tex → `T_VFX_Star_01` | Star02 *(the naming inversion)* |
| `M_VFX_DisAdd_Part07` | `Brightness` 1 → **2**; **`CamOffset` 0 → 30**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **30**; Main/Color/Dissolve_Tex → `T_VFX_Part_08` | Lens |

**`M_VFX_DisAdd_Star01` and `M_VFX_DisAdd_Star02` differ ONLY in their three texture references** —
every scalar and vector parameter is identical. One look definition with a swapped `ShapeTex` covers
both; this is the cheapest look pair in the batch.

**`M_VFX_DisAdd_Part04` here is parameter-identical to the instance NS_BasicAttack already recreated as
the `PartDisAdd04` look** — reusable verbatim.

**`M_VFX_DisAdd_Part07` is the flipbook material** (`Lens`, `SubUV 2x2`) and is the second instance in
this batch to use **`CamOffset` (30)**, after `NS_DebuffCast`'s `Part03_Bright` (50).

### Referenced textures `[corpus]`

| Texture | Size | Source format | Compression | sRGB | Address |
|---|---|---|---|---|---|
| `T_VFX_Part_01` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Part_02` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Part_04` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| **`T_VFX_Part_08`** | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap — **the 2×2 lens-flare atlas** |
| `T_VFX_Ring_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Ring_02` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Star_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Star_02` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Noise_02` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| **`T_VFX_LUT_Rainbow_01`** | **512×2** | **TSF_BGRA8** | **TC_Default** | **true** | Wrap/Wrap |
| `T_VFX_WhitePixel` | 1×1 | TSF_RGBA16 | TC_Default | true | Wrap/Wrap |

---

## 5. Per-layer runtime curves `[corpus]`

Curves sample **NormalizedAge** unless stated. `C` = constant (step) key, `L` = linear key.
`Dynamic Material Parameters` writes **Index 0 only** on every emitter.

**A single "heal" colour ramp is shared by six of the nine layers** — cyan-white → green → deep green:
`R 0.715694 → 1 → 0.109462 → …`, `G 0.89627 → 0.854993 → 1 → …`, `B 1 → 0.376262 → 0.617207 → …`.
The per-layer tables below transcribe each variant's exact keys because the tail values and key times
differ even where the head is identical.

### Layer 0 — `Bomb_Glow_01` (burst 1 @ 0, lifetime 1.0, infinite)
- Initialize Color `RGBA(1, 0.184475, 0.386429, 0.4)` (overridden by the update curve);
  Uniform Sprite Size **500**; `UsePositionOffset = false`.
- Color from Curve:
  - `R: (0, 0.715694)C (0.0796861, 1)L (0.234229, 0.109462)L (1, 0.017642)C`
  - `G: (0, 0.89627)C (0.0796861, 0.854993)L (0.234229, 1)L (1, 0.323143)C`
  - `B: (0, 1)C (0.0796861, 0.376262)L (0.234229, 0.814847)L (1, 0.0648033)C`
  - `A: (0, 0)C (0.0808934, 1)L (1, 0)C`
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (0.1, 0.9)C (1, 1)C`.
- Dynamic params: **`[1, 0, 0, 0]`** constant.

### Layer 1 — `Bomb_Glow_02` (burst 1 @ 0, lifetime 1.0, infinite)
- Initialize Color `RGBA(0.913099, 0.0193824, 0.130136, 0.4)`; Uniform Sprite Size **260**.
- Color from Curve (the shared ramp with a **different, brighter tail**):
  - `R: (0, 0.715694)C (0.079686, 1)L (0.290975, 0.109462)L (0.713553, 0.0241576)C`
  - `G: (0, 0.89627)C (0.079686, 0.854993)L (0.290975, 1)L (0.713553, 0.913099)C`
  - `B: (0, 1)C (0.079686, 0.376262)L (0.290975, 0.617207)L (0.713553, 0.141263)C`
  - `A: (0, 0)C (0.080893, 1)L (1, 0)C`
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (0.1, 1)L (1, 1)L`.
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 2 — `Raimbow` (burst 1 @ 0, lifetime 1.0, infinite)
- Initialize Color `RGBA(0.913099, 0.913099, 0.913099, **0.15**)`; Uniform Sprite Size **350**.
  (The Buff variant uses alpha 0.1 and size 450 — this is the delta between the two systems' rainbows.)
- Sprite Rotation Mode Random, Min 0 / Max 360.
- Scale Color (RGBA Together): `R (0, 0.5)L | G (0, 0.5)L | B (0, 0.5)L | A (0, 1)L (1, 0)L`
  — RGB is a single flat-0.5 key.
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (0.2, 0.9)C (1, 1)L`.
- Dynamic params: **`[0.5, 0, 0, 0]`** constant.

### Layer 3 — `Ring` (burst 1 @ **0.05**, lifetime **0.5**, infinite)
- `Lifetime Mode = Direct Set`, `Lifetime = 0.5` (the `Lifetime Min 0.3 / Max 0.7` entries are inert).
- Sprite Size Mode Uniform, **Uniform Sprite Size 140** (`Min 150 / Max 160` inert in Uniform mode).
- Sprite Rotation Mode Random, Min 0 / Max 360. Initialize Color `RGBA(1, 1, 1, 1)`.
- Color from Curve — identical keys to layer 1's.
- **Dynamic param 1 animated**: `Float from Curve` **`(0, -0.325)C (1, -0.5)C`**
  (the same −0.325 → −0.5 dissolve slide as `NS_BuffCast`'s Ring). Params 2/3/4 constant 0.
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (0.1, 0.9)C (1, 1)C`.
- **Module order note**: `Dynamic Material Parameters` runs *before* `Color` in this emitter's update
  stack (module 2 vs 3), the reverse of every other emitter in the batch. No functional consequence
  `[inferred]` — the two write disjoint attributes.

### Layer 4 — `Sparkles_01` (Self/Once 0.3 s; burst 3 @ 0.05 + rate 20/s)
- Lifetime `[unresolved]` — `0.3 / 0.6` vs override `0.2 / 0.4`.
- **`Cylinder Location`**: Radius **80**, Height **130**, Midpoint 0.5, **Offset `(0, 0, 0)`**,
  Random, Spawn Only, `Surface Only = false`, `Override Local Rotation = true`.
- `Add Velocity` = `Random Range Vector` **Min `(0, 0, 1000)` → Max `(0, 0, 2000)`** — upward.
- Sprite Size Mode Random Uniform: **Min 6, Max 10** — the smallest sprites in the batch.
- Initialize Color `RGBA(1, 1, 1, 1)`; Sprite Rotation authored 90 (mode not Random here — no
  `Sprite Rotation Mode` line, so the authored angle is a fixed 90° `[inferred]`).
- Scale Velocity: `X/Y/Z: (0, 1)C (0.1, 0.15)C (1, -9.09372e-09)C`.
- Color from Curve — the shared ramp, **alpha without a fade-in**:
  - `R: (0, 0.715694)C (0.079686, 1)L (0.290975, 0.109462)L (0.713553, 0.024158)C`
  - `G: (0, 0.89627)C (0.079686, 0.854993)L (0.290975, 1)L (0.713553, 0.913099)C`
  - `B: (0, 1)C (0.079686, 0.376262)L (0.290975, 0.617207)L (0.713553, 0.141263)C`
  - `A: (0.080893, 1)L (1, 0)C`  *(first key at t = 0.0809, not 0)*
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`; its non-uniform companion
  `X/Y: (0, 0)L (1, 1)L` is not the active channel.
- Dynamic params: **`[3, 0, 0, 0]`** constant.

### Layer 5 — `Sparkles_Stretched` (Self/Once 0.3 s; burst 5 @ 0.05 + rate 10/s)
- Lifetime `[unresolved]` — `0.3 / 0.6` vs override `0.2 / 0.4`.
- `Cylinder Location`: Radius **80**, Height **120**, Midpoint 0.5, Offset **(0, 0, 0)**.
- `Add Velocity` = `Random Range Vector` **Min `(0, 0, 1000)` → Max `(0, 0, 1700)`**.
- Sprite Size Mode **Random Non-Uniform**: `Sprite Size Min (25, 70)` → `Max (40, 60)`.
  **Note the Y bound inversion `[corpus]`**: min Y (70) is GREATER than max Y (60). Treat as exported;
  Niagara lerps between them per component, so Y draws in `[60, 70]` descending `[inferred]`.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.15)C (1, -9.09372e-09)C`.
- Color from Curve: identical keys to layer 4's (including the `A: (0.080893, 1)L (1, 0)C` shape).
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`.
- Scale Sprite Size 001 (Non-Uniform Curve): `X: (1, 1)L | Y: (0, 1)C (0.3, 0.25)C (1, 0.2)C`.
- **`Scale Sprite Size by Speed`**: `Scale Factor Curve (0, 0)L (1, 1)L`, `Velocity Threshold` **1000**,
  `Min Scale Factor (1, 1)`, `Max Scale Factor (1, 2)` — up to **2×** length at speed
  (the Buff sibling's is 1.7×).
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layers 6 & 7 — `Star01` / `Star02` (Self/Once 0.3 s; burst 1 @ 0.05 + rate 5/s)
**Identical in every parameter except sprite size and material.**
- Lifetime Mode Random, **Min 0.3 / Max 0.6** — unambiguous (no override).
  (`InitializeParticle.Lifetime = 1` present but inert.)
- `Cylinder Location`: Radius **30**, Height **60**, Midpoint 0.5, **Offset `(0, 0, -30)`**
  (spawned *below* the origin), Random, Spawn Only, empty `Lathe Profile` override.
- `Add Velocity`: **`(0, 0, 500)`** constant — straight up.
- Sprite Size Mode Random Uniform: `Star01` **Min 40 / Max 50**, `Star02` **Min 30 / Max 40**.
  (`Sprite Size (10, 10)` and `Uniform Sprite Size 450` present but not selected.)
- Initialize Color `RGBA(1, 0.184475, 0.386429, 0.4)` (overridden by the update curve).
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.25)C (1, -0)C`.
- Color from Curve — the shared ramp with a **green-dominant tail**:
  - `R: (0, 0.715694)C (0.0796861, 1)L (0.234229, 0.921582)L (0.80652, 0.109462)C`
  - `G: (0, 0.89627)C (0.0796861, 0.854993)L (0.234229, 0.723055)L (0.80652, 1)C`
  - `B: (0, 1)C (0.0796861, 0.376262)L (0.234229, 0.462077)L (0.80652, 0.617207)C`
  - `A: (0.0808934, 1)L (1, 0)C`
- Scale Sprite Size (Uniform Curve): `(0, -3.11599e-08)C (0.1, 1)C (1, 0)C` (first key numerically zero).
- Dynamic params: **`[1, 0, 0, 0]`** constant.
- Materials are **swapped relative to the emitter names** (§2): `Star01` → `M_VFX_DisAdd_Star02`,
  `Star02` → `M_VFX_DisAdd_Star01`.

### Layer 8 — `Lens` (Self/Once **0.2 s**; burst 3 @ 0.05 + rate 10/s) — **the sub-UV layer**
- Lifetime Mode Random, **Min 1.0 / Max 1.5** — unambiguous (no override). Note the lifetime is
  **5–7.5× the emitter's own 0.2 s loop**.
- `Cylinder Location`: Radius **40**, Height **70**, Midpoint 0.5, **Offset `(0, 0, -70)`**, empty
  `Lathe Profile` override.
- `Add Velocity` = `Random Range Vector` **Min `(0, 0, 150)` → Max `(0, 0, 300)`**.
- **`Sub UV Animation`, `Mode = Random`**, `Start Frame 0`, `End Frame 3`, `SubUV Loop Count 1`.
- Sprite Size Mode **Random Non-Uniform**: `Sprite Size Min (80, 350)` → `Max (100, 400)` — long
  velocity-aligned lens streaks, the largest non-uniform sprites in the batch.
- Initialize Color `RGBA(1, 0.184475, 0.386429, 0.4)` (overridden by the update curve).
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.25)C (1, -0)C`.
- Color from Curve — **no key at t = 0 on any channel**:
  - `R: (0.0350136, 1)L (0.200423, 0.109462)L (0.776336, 0.0241576)C`
  - `G: (0.0350136, 0.854993)L (0.200423, 1)L (0.776336, 0.913099)C`
  - `B: (0.0350136, 0.376262)L (0.200423, 0.617207)L (0.776336, 0.141263)C`
  - `A: (0.0808934, 1)L`  *(single key — alpha holds 1 from t = 0.0809 to the end)*
- **Dynamic param 1 animated**: `Float from Curve` **`(0, -1)C (0.2, 3.16647e-08)C (1, -1)C`** —
  dissolve dips from fully-intact to 0 at t = 0.2 and back, i.e. the lens flare **erodes at its
  midpoint and re-forms**. Params 2/3/4 constant 0.
- **No size curve at all** (5 update modules) — the spawn-time random size holds for the whole life.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new row is required, and — as with `NS_DebuffCast` — the source's cadence does not fit the table's
shape.** Four emitters loop forever bursting 1 each; five fire once on a 0.2–0.3 s loop with a burst
*and* a rate.

Plan:

- **Row**: loop **1.0 s**, particle lifetime **1.5 s** (the longest layer, `Lens`), burst **17** (the
  exact corpus burst total). Partition `Seed % 17`:
  0 = Bomb_Glow_01, 1 = Bomb_Glow_02, 2 = Raimbow, 3 = Ring, 4–6 = Sparkles_01,
  7–11 = Sparkles_Stretched, 12 = Star01, 13 = Star02, 14–16 = Lens.
- **Spawn delays are per-layer and must be honoured**: `Ring`, `Sparkles_01`, `Sparkles_Stretched`,
  `Star01`, `Star02`, `Lens` all spawn at **t = 0.05**, not 0. `Behavior_Slash` already has the exact
  idiom for this (NS_BasicAttack §5: hide for `age < delay`, run curves on `(age − delay) / lifetime`).
- **The five `Spawn Rate` modules are DROPPED** in that plan, costing ≈ 17 particles — **half the
  visible count**. Widening the burst to ≈ 34 with `frac`-derived delays across each layer's 0.2–0.3 s
  window is closer but still an approximation. **Record whichever is chosen as a maintainer decision.**
- **The `Self / Once` semantics are LOST**: five layers will re-fire every 1.0 s where the source fires
  once. Check how the caller spawns "Cast" effects before deciding whether that matters.

### 6.2 VisTag / renderer needs

| Renderer | Kind | Look | Source emitters |
|---|---|---|---|
| a | CameraFacingSprite *(NEW KIND REQUIRED)* | `PartDisAdd01` | Bomb_Glow_01, Bomb_Glow_02 |
| b | CameraFacingSprite *(new kind)* | `RainbowDisAdd` | Raimbow |
| c | CameraFacingSprite *(new kind)* | `RingDisAdd01` | Ring |
| d | CameraFacingSprite *(new kind)* | `PartDisAdd01Bright` | Sparkles_01 |
| e | CameraFacingSprite *(new kind)* | `StarDisAdd02` | Star01 |
| f | CameraFacingSprite *(new kind)* | `StarDisAdd01` | Star02 |
| g | VelocityAlignedSprite | `PartDisAdd04` — **already exists** | Sparkles_Stretched |
| h | VelocityAlignedSprite + **SubUV 2×2** | `PartDisAdd07` | Lens |

**Eight row renderers, six needing the new `CameraFacingSprite` kind, one needing sub-UV on a
velocity-aligned sprite.** `PartDisAdd01`, `PartDisAdd01Bright`, `RainbowDisAdd`, `RingDisAdd01` are
shared with other sheets in the batch; `StarDisAdd01`/`02` are shared with `NS_HealLoop` and (for
`StarDisAdd01`) with `NS_BuffCast`/`NS_BuffLoop`.

### 6.3 Look / material needs

Eight looks, one (`PartDisAdd04`) already existing. `StarDisAdd01`/`StarDisAdd02` differ **only** in
`ShapeTex`/`DissolveTex` — the cheapest possible look pair.

Unplumbed family parameters that bite:

| Param | Value here | Plumbed? |
|---|---|---|
| `Core_Intensity` | 1 on `Part01_Bright` | **no** |
| `Gradient_Invert` | 0 on `Ring01`, `Part04`, `Star01/02`, `Part07`; **2 on `Rainbow`** | **no** |
| `GradientMap_Tex` + `GradientMap_Displacement` | LUT + 0.9 on `Rainbow` | **no** — see §6.5 |
| `Opacty_StepAdd` | 0.3 on `Rainbow` | **no** |
| **`CamOffset`** | **30 on `Part07`** | **no** |
| `Opacty_DepthFade` | 10 / 20 / 30 | **no** |

### 6.4 Texture needs

`T_VFX_Part_01` → existing `SoftParticle`; `T_VFX_Part_04` → existing `SparkStreak`;
`T_VFX_Noise_02` → existing `TileNoise`. New measurement + bake for `T_VFX_Part_02`,
`T_VFX_Ring_01`, `T_VFX_Ring_02`, `T_VFX_Star_01`, `T_VFX_Star_02` (the existing `Flare` bake may fit
one or both — **unmeasured**), **`T_VFX_Part_08` as a 2×2 four-frame ATLAS**, and the **colour LUT**
`T_VFX_LUT_Rainbow_01`. Method: NS_BasicAttack §7.

### 6.5 CAPABILITY GAPS — read before committing a session

1. **SUB-UV / FLIPBOOK ON A VELOCITY-ALIGNED SPRITE — does not exist.** `Lens` declares `SubUV: 2x2`
   on a **`VelocityAligned`** renderer, so this is strictly harder than `NS_DebuffCast`'s camera-facing
   flipbook: it needs sub-UV support on the one row-renderer kind that already exists. Pipeline needs:
   `SubImageSize` on the renderer spec, a `Particles.SubImageIndex` DI output, an atlas-capable
   texture bake, and (for `Mode = Random`) a per-particle random start frame. **Four additions.**

2. **CAMERA-FACING SPRITE ROW RENDERER — does not exist.** Six layers here — the most of any effect in
   the batch. Shared need across all six sheets.

3. **GRADIENT-MAP LUT.** `M_VFX_DisAdd_Rainbow` again. The family shader has no gradient chain and the
   procedural generator makes only greyscale non-sRGB bakes. Without both additions the Raimbow layer
   is a plain white glow. (NS_BasicAttack §13.4's "gradient chain is a no-op" reasoning is
   instance-specific and does **not** transfer.)

4. **`Self / Once` EMITTER LIFECYCLE — not expressible.** Five of the nine emitters. See §6.1.

5. **`Spawn Burst` + `Spawn Rate` ON THE SAME EMITTER — not expressible.** Five emitters, ≈ half the
   particle count. See §6.1.

6. **`CamOffset 30`** on the `Lens` material. A geometric camera-ward offset in the source material,
   not plumbed in `DissolveAdd.ush`. Omitting it changes the layer's depth sorting against the rest of
   the effect, which is exactly what it is there to control.

7. **`Scale Sprite Size by Speed`** (`Sparkles_Stretched`, up to 2× length above a 1000-unit
   threshold). Expressible — the behavior knows its own velocity — but it must come from the same
   closed-form velocity integral as position, never from a frame delta (NS_BasicAttack §8, lesson 7).

8. **PER-LAYER SPAWN DELAY (0.05 s)** on six of nine layers. **Not a gap** — `Behavior_Slash` already
   does this — but it is easy to forget, and forgetting it makes the whole effect start on frame 0
   instead of staggered.

9. **INVERTED SIZE BOUNDS** on `Sparkles_Stretched` (`Sprite Size Min (25, 70)` / `Max (40, 60)`).
   `[corpus]`, not a transcription error. Any test asserting `min <= value <= max` per component will
   fail on Y unless it sorts the bounds first.

10. **WORLD SPACE vs LOCAL SPACE.** All nine emitters `LocalSpace: false`; template is local-space.
    Same recorded deviation as NS_BasicAttack §13.2.

11. **SPRITE ROTATION.** `Raimbow` and `Ring` use random 0–360°; `Sparkles_01` and
    `Sparkles_Stretched` carry a fixed authored 90°. `[unresolved: whether `Particles.SpriteRotation`
    is bound on camera-facing sprite renderers]` — verify in `CkParticles_TemplateBuilder.cpp`.

12. **No ribbon, no mesh, no events, no forces in this system.** On the physics axis this is the
    simplest "Cast" effect in the batch.

### 6.6 Behavior id

**Do NOT allocate an id here.** `ck::particles::NumBehaviors` is 18 at the time of writing.

### 6.7 Complexity assessment

**Tier L** — sub-UV on a velocity-aligned sprite (gap 1) plus the rainbow LUT (gap 3) plus the
burst+rate/`Self-Once` cadence (gaps 4–5). Drop the `Lens` flipbook animation and the rainbow gradient
and it falls to **M**, at two recorded fidelity costs. Its curves and kinematics are otherwise
straightforward — every layer is "spawn in a cylinder, move up, decay".

---

## 7+. Reserved for implementation — sections 7–14 per [README.md](README.md) are written by the session that implements this effect.
