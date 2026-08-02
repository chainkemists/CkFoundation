# Recipe: NS_HealCast → CkParticles (IMPLEMENTED)

Schema and evidence-tag conventions: [README.md](README.md). Exemplars: [NS_BasicAttack.md](NS_BasicAttack.md),
[NS_Lightning_Range.md](NS_Lightning_Range.md).

## Completion state — READ FIRST

**Status: IMPLEMENTATION-COMPLETE (2026-08-02) — behavior 31. Human A/B parity NOT yet judged.**

`Behavior_HealCast.ush` + `ExecuteStage_CPU` case 31, the `PS_CkParticles_Template_HealCast` cadence
row (the cookbook's **first** to declare a burst AND a spawn rate) with eight row renderers on
VisTags 105–112, one new look (`PartDisAdd07`), one new texture bake (`LensSheet`),
`Test_Particles_HealCastBehavior.cpp`, and the **HEAL CAST** pair in the VfxExamples gym. §12's walk
is `[HUMAN-VERIFY]` and open.

**Four `[values]`-block corrections were applied to §5 at implementation time** — see the boxed notes
there and §14.2. They are the same class the batch-C sweep ratified as [P2-D2]: a `Color.Scale Alpha`
that lives only in the corpus store and never made it into the transcribed curve list.

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
| 4 | `Sparkles_01` | **Self / Once** | **0.3** | **3** | **0.05** | **20** | rand **0.3–0.6** `[corpus-v3]` | Sprite, Unaligned/FaceCamera | `Part01_Bright` |
| 5 | `Sparkles_Stretched` | **Self / Once** | **0.3** | **5** | **0.05** | **10** | rand **0.3–0.6** `[corpus-v3]` | Sprite, **VelocityAligned**/FaceCamera | `Part04` |
| 6 | `Star01` | **Self / Once** | **0.3** | **1** | **0.05** | **5** | rand 0.3–0.6 | Sprite, Unaligned/FaceCamera | `Star02` |
| 7 | `Star02` | **Self / Once** | **0.3** | **1** | **0.05** | **5** | rand 0.3–0.6 | Sprite, Unaligned/FaceCamera | `Star01` |
| 8 | `Lens` | **Self / Once** | **0.2** | **3** | **0.05** | **10** | rand 1.0–1.5 | Sprite, **VelocityAligned**/FaceCamera, **SubUV 2×2** | `Part07` |

**Note the naming inversion `[corpus]`, do not "fix" it:** emitter `Star01` draws with
`M_VFX_DisAdd_Star02`, and emitter `Star02` draws with `M_VFX_DisAdd_Star01`. Same class of authored
skew as NS_BasicAttack's `Slash_03` → `M_VFX_DisAdd_Slash04`.

**Particles per firing (burst only, the exact `[corpus]` number): 1+1+1+1+3+5+1+1+3 = 17.**
The five `Spawn Rate` modules add ≈ 20×0.3 + 10×0.3 + 5×0.3 + 5×0.3 + 10×0.2 = **≈ 17 more**
`[inferred — rate × loop-duration arithmetic]`, so ≈ **34 total per firing**. `[corpus-v3]` The
system is `Loop Once / 2.0 s`, so the four `Life Cycle Mode = System` emitters burst **once** over
that 2.0 s window, not once per second; the five `Self / Once` emitters likewise fire only once.
*(Was stated as "re-burst every 1.0 s" from the inert emitter rows.)*

`Loop Count Limit = 1` with `UseLoopCountLimit = false` throughout — inert, same trap as the siblings.

> ### Lifetime — RESOLVED `[corpus-v3]`
> Both ambiguous emitters are `Lifetime Mode = Random`, so per [P0-D2] the **Min/Max pins drive** and
> the Direct-Set `RandomRangeFloat` override is INERT (`lifetimeResolved.source = minmax`):
>
> | Emitter | LIVE (Random mode) | inert override |
> |---|---|---|
> | `Sparkles_01` | **0.3 / 0.6** | ~~0.2 / 0.4~~ |
> | `Sparkles_Stretched` | **0.3 / 0.6** | ~~0.2 / 0.4~~ |
>
> `Star01`, `Star02` (0.3 / 0.6) and `Lens` (1.0 / 1.5) have no `Lifetime` override and were already
> unambiguous — v3 confirms all three (their stored `Lifetime = 1` sits under `inertValues`).
> `Ring` is `Lifetime Mode = Direct Set` with `Lifetime = 0.5` — its `Lifetime Min 0.3 / Max 0.7`
> entries are inert leftovers, also confirmed.
>
> **System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
> `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.** Per [P0-D1] this rules
> the four `Life Cycle Mode = System` emitters — their stored `Infinite / 1.0 s` rows are inert. The
> five `Life Cycle Mode = Self` emitters keep their OWN live `Once / 0.3 s` (0.2 s for `Lens`) rows.

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
- **`Color.Scale Alpha` = 0.5** `[corpus, correction 2026-08-02]` — it lives only in the `[values]`
  block and was missing from this list; it halves the layer's whole alpha envelope.
- Color from Curve:
  - `R: (0, 0.715694)C (0.0796861, 1)L (0.234229, 0.109462)L (1, 0.017642)C`
  - `G: (0, 0.89627)C (0.0796861, 0.854993)L (0.234229, 1)L (1, 0.323143)C`
  - `B: (0, 1)C (0.0796861, 0.376262)L (0.234229, 0.814847)L (1, 0.0648033)C`
  - `A: (0, 0)C (0.0808934, 1)L (1, 0)C`
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (0.1, 0.9)C (1, 1)C`.
- Dynamic params: **`[1, 0, 0, 0]`** constant.

### Layer 1 — `Bomb_Glow_02` (burst 1 @ 0, lifetime 1.0, infinite)
- Initialize Color `RGBA(0.913099, 0.0193824, 0.130136, 0.4)`; Uniform Sprite Size **260**.
- **`Color.Scale Alpha` = 0.5** `[corpus, correction 2026-08-02]` — as layer 0.
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
- **`Color.Scale Alpha` = 0.15** `[corpus, correction 2026-08-02]` — the dimmest scale in the system,
  and missing from this list until implementation.
- Color from Curve — identical keys to layer 1's.
- **Dynamic param 1 animated**: `Float from Curve` **`(0, -0.325)C (1, -0.5)C`**
  (the same −0.325 → −0.5 dissolve slide as `NS_BuffCast`'s Ring). Params 2/3/4 constant 0.
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (0.1, 0.9)C (1, 1)C`.
- **Module order note**: `Dynamic Material Parameters` runs *before* `Color` in this emitter's update
  stack (module 2 vs 3), the reverse of every other emitter in the batch. No functional consequence
  `[inferred]` — the two write disjoint attributes.

### Layer 4 — `Sparkles_01` (Self/Once 0.3 s; burst 3 @ 0.05 + rate 20/s)
- Lifetime `[corpus-v3]` — **`0.3 / 0.6` drives** (Random mode); the `0.2 / 0.4` override is inert.
- **`Cylinder Location`**: Radius **80**, Height **130**, Midpoint 0.5, **Offset `(0, 0, 0)`**,
  Random, Spawn Only, `Surface Only = false`, `Override Local Rotation = true`.
- `Add Velocity` = `Random Range Vector` **Min `(0, 0, 1000)` → Max `(0, 0, 2000)`** — upward.
- Sprite Size Mode Random Uniform: **Min 6, Max 10** — the smallest sprites in the batch.
- Initialize Color `RGBA(1, 1, 1, 1)`; **`Color.Scale Alpha` = 0.15** `[corpus, correction 2026-08-02]`.
- Sprite Rotation: the authored 90 is **INERT** `[corpus, correction 2026-08-02]` — `Sprite Rotation Mode`
  reads `Unset`, so Initialize Particle never writes `Particles.SpriteRotation` at all and the sprite
  draws unrotated. *(Was `[inferred]` as a fixed 90°.)*
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
- Lifetime `[corpus-v3]` — **`0.3 / 0.6` drives** (Random mode); the `0.2 / 0.4` override is inert.
- `Cylinder Location`: Radius **80**, Height **120**, Midpoint 0.5, Offset **(0, 0, 0)**.
- **`Color.Scale Alpha` = 0.15** `[corpus, correction 2026-08-02]`.
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
**Identical in every parameter except sprite size, material and alpha scale.**
`[corpus, correction 2026-08-02]` `Star01` carries `Color.Scale Alpha` **1** and `Star02` **0.7** — a
third difference this line originally denied.
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
- **`Color.Scale Alpha` = 0.7** `[corpus, correction 2026-08-02]`, so the single-key alpha resolves to a
  flat 0.7 rather than 1.
- **No size curve at all** (5 update modules) — the spawn-time random size holds for the whole life.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**A new row is required, and — as with `NS_DebuffCast` — the source's cadence does not fit the table's
shape.** Four emitters loop forever bursting 1 each; five fire once on a 0.2–0.3 s loop with a burst
*and* a rate.

Plan:

- **Row `[corpus-v3]`, per [P0-D3]**: loop **2.0 s** (the system's `Once` loop duration — *was 1.0 s
  under the pre-v3 emitter-row reading*), particle lifetime **1.55 s**, burst **17** (the exact corpus
  burst total). Partition `Seed % 17`:
  0 = Bomb_Glow_01, 1 = Bomb_Glow_02, 2 = Raimbow, 3 = Ring, 4–6 = Sparkles_01,
  7–11 = Sparkles_Stretched, 12 = Star01, 13 = Star02, 14–16 = Lens.

  > **Correction applied at implementation (2026-08-02):** the lifetime was derived here as **1.5 s**,
  > `Lens`' resolved maximum on its own. [P0-D5] resolves it as `max(spawn delay + resolved lifetime)`
  > and `Lens` bursts at **0.05 s**, so the row is **1.55 s**. Arithmetic only — the itemization was
  > right.

- **Spawn delays are per-layer and must be honoured**: `Ring`, `Sparkles_01`, `Sparkles_Stretched`,
  `Star01`, `Star02`, `Lens` all spawn at **t = 0.05**, not 0. `Behavior_Slash` already has the exact
  idiom for this (NS_BasicAttack §5: hide for `age < delay`, run curves on `(age − delay) / lifetime`).
  **This applies to BURST particles only** — a streamed particle's own spawn time IS its phase, so
  adding the beat on top of it would delay it twice.
- **The five `Spawn Rate` modules are REPRODUCED, not dropped** `[decision 2026-08-02, batch D]`.
  §6.1 originally offered dropping them (costing ≈ 17 particles — half the visible count) or faking
  them with `frac`-derived burst slots. Phase 2's C2 landed a real rate stack and C5 landed the
  spawn-phase input, so the row now declares **burst 17 AND rate 50/s** and the behavior splits the
  two populations by spawn phase. §9.2 states the mapping and §13.2 states what it costs.
- **The `Self / Once` semantics are REPRODUCED as a window**, not lost: a streamed particle whose
  spawn phase falls past its own emitter's loop duration (0.3 s, 0.2 s for `Lens`) is hidden, so each
  streaming layer emits over exactly its own window once per system loop. §13.2 records the price.

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

## 7. Textures — one new bake

§6.4 asked for measurement and up to seven new bakes. **One was needed.** Everything except the lens
atlas had already been measured off the same corpus PNG by an earlier batch.

| Source paint | Stand-in | Measured in |
|---|---|---|
| `T_VFX_Part_01` | `SoftParticle` | NS_BasicAttack §7 |
| `T_VFX_Part_02` | `SoftParticleBright` | NS_FireBall_Hit §7 |
| `T_VFX_Part_04` | `SparkStreak` | NS_BasicAttack §7 |
| **`T_VFX_Part_08`** | **`LensSheet` — NEW, §7.1** | this recipe |
| `T_VFX_Ring_01` | `RingUneven` | NS_FireBall_Hit §7 |
| `T_VFX_Ring_02` | `RingFlare` | NS_FireBall_Hit §7 |
| `T_VFX_Star_01` | `StarFour` | NS_FireBall_Hit §7 |
| `T_VFX_Star_02` | `StarFourTight` | NS_Arrow_Cast §7 |
| `T_VFX_Noise_02` | `TileNoise` | NS_BasicAttack §7 |
| `T_VFX_LUT_Rainbow_01` | `LutWhite` | Phase 1 C3 — held back by [P1-D1], see §13 |

### 7.1 `LensSheet` — the measured 2×2 lens-flare atlas

Both existing `MaskSheet` bakes were measured against `T_VFX_Part_08` and **rejected**: Pearson
**0.56** against the wind paint (`WindSheet`'s source) and **0.26** against the impact one. Neither is
close, and the reason is structural — those two are directional bursts and this is a plain puff.

Measured, frame by frame, off the corpus PNG (512², four 256² frames, row-major):

| Quantity | Measurement |
|---|---|
| Per-frame peak | **0.6275, 0.4627, 0.4078, 0.3608** — monotonically DECAYING in frame order |
| Frame-to-frame correlation | 0.77 – 0.93, i.e. one flare dimming rather than four paints |
| Centroid (frame-local) | (0.505, **0.402**) — a tenth of a frame ABOVE centre |
| Support radius | 0.69 of the half-frame (2 % of peak) |
| Radial law | `pow(1 − r/R, k)`, per-frame fits k = 1.64 / 1.77 / 1.50 / 1.49 → **1.60** |
| Angular content | harmonics **2 and 3** at ≈ 0.25 of the mean; a lobed puff, not a disc |
| Coverage above 0.02 | 0.237 – 0.264, near-constant across frames |

The bake reproduces those: whole-sheet Pearson **0.849** against the source, per-frame 0.83 – 0.88,
coverage 0.255 – 0.288 against the measured 0.237 – 0.264. Its peak runs ≈ 10 % hot because the lobe
crest can coincide with the radial maximum — recorded in §13.5 rather than tuned away.

---

## 8. Mesh

**None.** All nine source renderers are sprites: seven camera-facing and two velocity-aligned, one of
those two a 2×2 flipbook.

---

## 9. The behavior — `Behavior_HealCast.ush` + `ExecuteStage_CPU` case 31

### 9.1 The cadence row

| Field | Value | Source |
|---|---|---|
| `LoopDuration` | 2.0 s | the SYSTEM's `Loop Behavior = Once`, `Loop Duration = 2` ([P0-D1]) |
| `ParticleLifetime` | 1.55 s | `Lens`' 0.05 s beat + its resolved 1.5 s maximum ([P0-D5]) |
| `BurstCount` | 17 | 1+1+1+1+3+5+1+1+3, the exact per-emitter burst counts |
| `SpawnRate` | 50 /s | 20+10+5+5+10, the five `Self / Once` emitters' own Spawn Rates |

### 9.2 Burst + rate on one row — how the two populations are told apart

This is the cookbook's first row to declare BOTH stacks, and the behavior needs no flag to separate
them: **spawn phase does it.** `SpawnPhase = fmod(EmitterAge − Age, LoopDuration)`; the burst module
fires at Spawn Time 0 so its particles land at phase 0, and the rate stack spreads its particles
across the whole loop.

- **Phase ≈ 0 → burst particle.** Takes `Seed % 17` over the source's own per-emitter counts, and
  carries its emitter's 0.05 s beat.
- **Otherwise → rate particle.** Takes a weighted draw against `CkParticles_Rand(Seed, 0)` over
  cumulative rate shares (20 / 30 / 35 / 40 / 50), which is the same idiom the four Loop ports use. A
  modulus cannot express these shares.
- **A rate particle born past its own emitter's window is hidden.** The window is that emitter's own
  `Loop Duration` — 0.3 s, or 0.2 s for `Lens` — which is what reproduces `Life Cycle Mode = Self /
  Loop Once` on a template that loops forever.

**Why the rate is the SUM and not something smaller.** With row rate `R` and share `s_L = rate_L / R`,
layer `L` spawns at exactly `R · s_L = rate_L` particles per second while its window is open, which is
the source's own density. The particles drawn for `L` outside its window are the price; §13.2 counts
them.

The epsilon that classifies phase 0 is a float-precision floor, not a tolerance: `EmitterAge` grows
without bound while `Age` stays small, so a burst particle's phase drifts off zero by roughly
`EmitterAge · 2⁻²⁴`. The 1 ms floor holds the classification correct past two hours of continuous
play, and a rate particle unlucky enough to spawn inside that millisecond is misread as a burst
particle at a rate of a few hundredths of a particle per loop.

### 9.3 Per-layer notes worth the reader's time

- **`Lens` is the cookbook's only velocity-aligned FLIPBOOK.** Its 2×2 sheet is divided by the row
  renderer's `SubImageSize` while its quad's long axis tracks the climb. Its `Sub UV Animation` is in
  `Random` mode, so the start frame is a per-particle draw and the animation makes exactly one pass.
- **`Lens`' dissolve ERODES AND RE-FORMS**: `(0, −1) → (0.2, 0) → (1, −1)`. Every other dissolve in
  the cookbook slides one way. It is also the only layer with no size curve at all.
- **`Sparkles_01`'s authored 90° rotation is inert** — `Sprite Rotation Mode` is `Unset`, so the
  module never writes `Particles.SpriteRotation`. §5's `[inferred]` fixed-90 reading was wrong and is
  corrected in place.
- **`Sparkles_Stretched`'s Y size bounds are INVERTED in the source** — `Min (25, 70)` / `Max (40, 60)`.
  Niagara lerps each component between its own pair, so Y draws in [60, 70] descending. Transcribed as
  authored; §11 sorts the bounds rather than asserting `min ≤ v ≤ max`.
- **Its `Scale Sprite Size by Speed` doubles the LENGTH only**, 1× below the 1000-unit threshold and 2×
  at or past it — and its velocity range STARTS at 1000, so every particle is at the ceiling while it
  is young. Computed from the same closed-form velocity the position integral uses, never from a frame
  delta (NS_BasicAttack §8, lesson 7).
- **The star emitters and their paints are SWAPPED** (`Star01` → `M_VFX_DisAdd_Star02`). Preserved,
  and §11 asserts it, so a reader "fixing" the skew fails a named test.
- **Four layers share one heal ramp verbatim** (Glow_02, Ring, Sparkles_01, Sparkles_Stretched); the
  glow head, the stars and the lens each carry their own.

---

## 10. Looks and renderers

Eight row-declared renderers on VisTags **105–112** — six camera-facing sprites and two
velocity-aligned, one of those a 2×2 sub-UV sheet.

| VisTag | Kind | Look | Source material | Serves |
|---|---|---|---|---|
| 105 | CameraFacingSprite | `PartDisAdd01` | `M_VFX_DisAdd_Part01` | Bomb_Glow_01, Bomb_Glow_02 |
| 106 | CameraFacingSprite | `RainbowDisAdd` | `M_VFX_DisAdd_Rainbow` | Raimbow |
| 107 | CameraFacingSprite | `RingDisAdd01` | `M_VFX_DisAdd_Ring01` | Ring |
| 108 | CameraFacingSprite | `PartDisAdd01Bright` | `M_VFX_DisAdd_Part01_Bright` | Sparkles_01 |
| 109 | VelocityAlignedSprite | `PartDisAdd04` | `M_VFX_DisAdd_Part04` | Sparkles_Stretched |
| 110 | CameraFacingSprite | `StarDisAdd02` | `M_VFX_DisAdd_Star02` | Star01 *(the naming inversion)* |
| 111 | CameraFacingSprite | `StarDisAdd01` | `M_VFX_DisAdd_Star01` | Star02 *(the naming inversion)* |
| 112 | VelocityAlignedSprite, SubUV 2×2 | **`PartDisAdd07` — NEW** | `M_VFX_DisAdd_Part07` | Lens |

**One new look.** `PartDisAdd07` (`CkUsf_CastLooks_Assets.as`): Brightness 2, Opacity_Boldness 1,
Gradient_Invert 0, everything else the family reference's, over the new `LensSheet` bake. The other
seven were checked value-by-value against §4's delta table and reused unchanged — `PartDisAdd04` in
particular is parameter-identical to the instance NS_BasicAttack already recreated, exactly as §4
predicted.

`Get_BehaviorLookName(31)` stays `NAME_None`: every look rides a row renderer that binds it explicitly.

---

## 11. Tests

`Test_Particles_HealCastBehavior.cpp` + the `NumBehaviors` 30 → 33 ratchet in
`Test_Particles_RosterSanity.cpp`.

- **The spawn-phase split is asserted against its own opposite.** One seed is evaluated at three
  phases: 0 (burst — must read the burst table), mid-window (rate — must read the rate table), and
  past the window (must be hidden). 2000 seeds, all three exact. Collapsing any of the three into
  another is the specific failure a burst+rate row can have.
- **The rate shares** over 400 000 seeds, every layer within **0.004** of its source share, plus the
  stronger claim that the four system-governed emitters take **zero** stream — they have no `Spawn
  Rate` module and nothing may leak into them.
- **The burst counts** are asserted slot-by-slot against the source's per-emitter numbers.
- **The 50 ms beat is asserted on the burst path only**, and a streamed `Sparkles_01` is asserted to
  be alive immediately — that pair is what proves the delay is not applied twice.
- **`Lens`: the flipbook stays inside the 2×2 sheet, uses all four frames, advances for every sampled
  particle, and its dissolve starts fully eroded and re-forms by its midpoint.** The renderer spec is
  asserted to be VELOCITY-ALIGNED with a (2, 2) grid — the capability §6.5 gap 1 called the hardest
  thing in the sheet.
- **`Sparkles_Stretched` draws a taller-than-wide quad and its speed stretch is on the length axis
  only** (measured ratio band 2.84 – 5.12 at spawn).
- **The star naming inversion is asserted**, so "fixing" it fails a named test.
- Plus the standard per-layer anti-vacuity and death checks, the latter on BOTH spawn paths.

---

## 12. Verification — A/B protocol

`[HUMAN-VERIFY]` — **not yet run.** Open the **VfxExamples** gym, station pair **HEAL CAST**.
`NS_HealCast` is a `Loop Once` system, so the harness re-arms both sides on completion and the two
pedestals replay in sync from t = 0. Use `Ck_GymVfxExamples_RestartAll` to re-fire them together.

| # | Criterion | Look for |
|---|---|---|
| a | Overall read | a green-white bloom with a column of fine sparkles rising out of it |
| b | Palette | cyan-white → white → saturated GREEN → dark. If it reads magenta or red the shared ramp's middle key is wrong |
| c | Beat | the three body layers open at t = 0; everything else on a 50 ms beat |
| d | Density | roughly 17 particles at the beat and a further ≈ 14 streamed over the next 0.3 s. The stream should stop while the glows are still alive — that is the `Self / Once` window |
| e | Lens flares | three long velocity-aligned streaks (80–100 × 350–400) rising slowly from BELOW the spawn point, each cycling a four-frame flipbook once |
| f | Lens dissolve | each flare should visibly ERODE, re-form around its midpoint, and erode again — unique in the cookbook |
| g | Sparkles | fine motes (6–10 units) thrown up hard out of a wide cylinder, and streaks (25–40 wide) alongside them stretching up to 2× while fast |
| h | Stars | two four-point flares rising from 30 units BELOW the origin; the larger is the brighter |
| i | Ring | a 140-unit halo at 15 % opacity, starting partly eroded and sliding only to −0.5 |
| j | Rainbow layer | a flat mid-grey lens ring, NOT a rainbow — held back by [P1-D1] (§13.1) |
| k | World space | move the pedestal mid-effect if you can (§13.6) |

---

## 13. Confirmed fidelity differences

1. **The `Raimbow` layer is grey, not a rainbow** — [P1-D1], as NS_FireBall_Hit §13 records. §6.5
   gap 3 is DISCHARGED as a recorded loss rather than silently inherited.
2. **Streamed particles drawn outside their own window are allocated and hidden.** The row spawns
   50 /s across the whole 2.0 s loop (100 particles) of which ≈ 14 fall inside their layer's window
   and render; the rest cost a particle slot and draw nothing. That is the price of expressing a
   `Self / Once` sub-emitter on a looping template, and it buys the source's exact per-layer stream
   DENSITY, which the alternative (drop the rate) does not.
3. **`CamOffset 30` on `Part07` is not plumbed.** A camera-ward world-position push in the source
   material; omitting it changes the lens layer's depth sorting against the rest of the effect, which
   is what it is there to control.
4. **Other unplumbed family parameters**: `Core_Intensity` (1 on `Part01_Bright`), `Gradient_Invert`,
   `Opacty_StepAdd` (0.3 on Rainbow), `Opacty_DepthFade` (10 / 20 / 30).
5. **`LensSheet`'s peak runs ≈ 10 % above the measured per-frame maxima**, because its lobe crest can
   land on the radial maximum. Whole-sheet correlation against the source is 0.849; the alternative
   (normalising the lobe) undershoots the measured peak instead. On an additive unlit flare this is a
   brightness nudge, not a shape difference.
6. **World space.** All nine source emitters are `LocalSpace: false`; the template is local space.
   Same recorded deviation as NS_BasicAttack §13.2.
7. **Salt reuse between `CkParticles_RandDir` and the lifetime/speed draws** — see NS_PickupCast §13.5.

---

## 14. Reusable lessons

1. **Spawn phase is the whole answer to burst-plus-rate.** No flag, no extra attribute, no second
   emitter: `fmod(EmitterAge − Age, Loop)` is 0 for a burst particle and spread for a rate particle,
   and the epsilon that separates them is a float-precision floor rather than a tuning. Set the row's
   rate to the SUM of the source's per-emitter rates and give each layer its own share, and the
   in-window density comes out exactly right.
2. **`[values]` blocks are where `Color.Scale Alpha` hides.** Four of this sheet's nine layers carried
   one that never reached §5 — 0.5, 0.5, 0.15, 0.15 and 0.7 — because the transcription pass reads the
   Color MODULE and the scalar lives in the emitter's value store. Batch C ratified the same class as
   [P2-D2]; batch D found five more. Grep the store for `Scale Alpha` on every layer before writing
   the behavior.
3. **`Sprite Rotation Mode = Unset` beats an authored angle.** An emitter can carry a perfectly
   plausible `Sprite Rotation Angle = 90` that the module never writes. Read the MODE, not the value —
   the same trap `Lifetime Mode` set in [P0-D2].
4. **A burst delay belongs to burst particles only.** A streamed particle's spawn time IS its phase;
   adding the emitter's burst beat on top of it delays the layer twice, and nothing about the result
   looks obviously wrong — it just starts late. The test that catches it evaluates the same layer on
   both paths.
5. **Measure a candidate sheet before assuming a flipbook is a flipbook.** `WindSheet` and
   `ImpactSheet` were both already 2×2 `MaskSheet` bakes of the right dimensions, and both correlate
   badly (0.56 / 0.26) with this one because the STRUCTURE differs — directional bursts versus a
   decaying puff. Dimensional compatibility is not evidence.
