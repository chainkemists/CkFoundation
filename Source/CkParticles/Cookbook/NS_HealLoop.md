# Translation sheet: NS_HealLoop → CkParticles (PRE-IMPLEMENTATION)

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
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_HealLoop` |
| Pack | Vefects — *Anime VFX* |
| Corpus system files | `systems/Vefects/Anime_VFX/Shared/Skills/NS_HealLoop.{json,txt}` |
| Corpus material files | `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Rainbow,Part01_Bright,Part04,Star01,Star02,Part01,Part02}.json` |
| Corpus texture files | `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Part_04,Ring_02,Star_01,Star_02,Noise_02,LUT_Rainbow_01,WhitePixel}.json` |
| Meshes | none — no mesh renderer |

**The source Niagara asset was never opened.** Every fact below is `[corpus]` unless tagged otherwise.

> ### Sibling with a near-identical name
> `[corpus]` `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Heal_Loop` (underscored) is a different,
> parameterized system, also with 9 emitters. Discriminators: `NS_HealLoop` exports
> `userParameters: []` and draws through `M_VFX_DisAdd_*`; `NS_Heal_Loop` exports **nine**
> (`User.Flares Color 01`, `User.Glow Color 01/02/03`, `User.Rainbow Color 01`, `User.Scale Overall`,
> `User.Sparkles Color 01/02`, `User.Star Color 01`) and draws through
> `MI_VFX_{Glow_01, Glow_01_Bright, Glow_02, Glow_04, Lens_Rainbow_01, Star_01, Star_02}`.
> This sheet documents the **`Anime_VFX/Shared/Skills`** variant only.

---

## 2. System anatomy `[corpus]`

**9 CPU emitters, ALL enabled, ALL world-space (`LocalSpace: false`), `Bounds: Dynamic`,
`Determinism: false`, zero user parameters.**

**Every emitter uses `Spawn Rate` (continuous) — no burst module anywhere.** All nine are
`Life Cycle Mode = System`. Four of them run `Loop Behavior = Once` on a 0.3 s loop; five run
`Infinite` on a 1.0 s loop. Structurally this is `NS_DebuffLoop`'s shape with different content.

| # | Emitter | Loop | Loop dur | Rate (/s) | Lifetime | Renderer | Material | Size |
|---|---|---|---|---|---|---|---|---|
| 0 | `Raimbow` *(sic)* | Infinite | 1.0 | **0.5** | 1.0 | Sprite, Unaligned/FaceCamera | `Rainbow` | uniform 450 |
| 1 | `Sparkles_01` | **Once** | **0.3** | 10 | rand `[unresolved]` | Sprite, Unaligned/FaceCamera | `Part01_Bright` | rand uniform 6–10 |
| 2 | `Sparkles_Stretched` | **Once** | **0.3** | 10 | rand `[unresolved]` | Sprite, **VelocityAligned**/FaceCamera | `Part04` | rand non-uniform (25,70)–(40,60) |
| 3 | `Star01` | **Once** | **0.3** | 1 | rand 0.3–0.6 | Sprite, Unaligned/FaceCamera | **`Star02`** | rand uniform 40–50 |
| 4 | `Star02` | **Once** | **0.3** | 1 | rand 0.3–0.6 | Sprite, Unaligned/FaceCamera | **`Star01`** | rand uniform 30–40 |
| 5 | `Glow_01` | Infinite | 1.0 | 2 | 2.0 | Sprite, Unaligned/FaceCamera | `Part01` | uniform 550 |
| 6 | `Glow_02` | Infinite | 1.0 | 2 | 2.0 | Sprite, Unaligned/FaceCamera | `Part01` | uniform 250 |
| 7 | `Flares` | Infinite | 1.0 | 6 | rand `[unresolved]` | Sprite, Unaligned/FaceCamera | `Part02` | rand uniform 50–200 |
| 8 | `Glow_03` | Infinite | 1.0 | 2 | 2.0 | Sprite, Unaligned/FaceCamera | `Part02` | uniform 220 |

**Note the same naming inversion as `NS_HealCast` `[corpus]`, do not "fix" it:** emitter `Star01`
draws with `M_VFX_DisAdd_Star02` and emitter `Star02` draws with `M_VFX_DisAdd_Star01`.

**Steady-state rate from the five infinite emitters: 12.5 particles/second** (0.5+2+2+6+2).
The four `Once` emitters contribute 10×0.3 + 10×0.3 + 1×0.3 + 1×0.3 = **6.6 particles, one time only**
`[inferred]`. **This is by far the sparsest system in the batch** — `Raimbow` spawns one particle every
two seconds.

Steady-state live count from the infinite emitters ≈ 0.5×1 + 2×2 + 2×2 + 6×lifetime(Flares) + 2×2
≈ **12.5 + 6×L** `[inferred]`, which is 21 if `Flares` lives ~1.4 s.

> ### `[unresolved: lifetime on three emitters]`
> Same `Lifetime Mode = Random` vs `[override] Lifetime = dyn:Random Range Float` conflict as the rest
> of the batch. Niagara's Random mode reads `Lifetime Min/Max`; an override drives the **Direct Set**
> pin. The likely reading is that Min/Max wins `[inferred]`:
>
> | Emitter | `Lifetime Min/Max` (Random mode) | override `RandomRangeFloat` |
> |---|---|---|
> | `Sparkles_01` | **0.3 / 0.6** | 0.2 / 0.4 |
> | `Sparkles_Stretched` | **0.3 / 0.6** | 0.2 / 0.4 |
> | `Flares` | **1 / 2** | 0.2 / 0.4 |
>
> `Star01` / `Star02` are `Random` with Min 0.3 / Max 0.6 and **no override** — unambiguous.
> `Raimbow`, `Glow_01/02/03` are `Direct Set` — unambiguous (1.0, 2.0, 2.0, 2.0).
> `Flares` is again the worst conflict (1–2 s vs 0.2–0.4 s); resolve it first.

`Star01`, `Star02` carry an **empty `Lathe Profile` curve override** on their `Cylinder Location`.
`Star01`/`Star02` also carry `InitializeParticle.Lifetime = 1` alongside their Random ranges — inert.

---

## 3. Mesh geometry

**N/A — NS_HealLoop has no mesh renderer.** All nine renderers are sprites.

---

## 4. Material family and per-instance deltas `[corpus]`

**All seven materials are instances of `/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd`.**
`MD_Surface`, `BLEND_Translucent`, `MSM_Unlit`, `twoSided: false`, outputs `EmissiveColor` + `Opacity`,
dynamic channels `[dissolve, distortion, offset, core_color]`, 161 expressions each.

Reference = `M_VFX_DisAdd_Part01`; absolute defaults in [NS_BuffCast.md](NS_BuffCast.md) §4.

| Material | Δ vs `Part01` | Used by |
|---|---|---|
| `M_VFX_DisAdd_Part01` | (reference) | Glow_01, Glow_02 |
| `M_VFX_DisAdd_Part02` | `Glow_Intensity` 1 → **0.3**; Main/Color/Dissolve_Tex → `T_VFX_Part_02` | Flares, Glow_03 |
| `M_VFX_DisAdd_Part01_Bright` | `Brightness` 1 → **10**; `Core_Intensity` 0 → **1**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Part_02` | Sparkles_01 |
| `M_VFX_DisAdd_Part04` | `Brightness` 1 → **6**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **30**; Main/Color/Dissolve_Tex → `T_VFX_Part_04` | Sparkles_Stretched |
| `M_VFX_DisAdd_Star01` | `Brightness` 1 → **6**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **10**; Main/Color/Dissolve_Tex → `T_VFX_Star_01` | **Star02** |
| `M_VFX_DisAdd_Star02` | as `Star01` but Main/Color/Dissolve_Tex → `T_VFX_Star_02` | **Star01** |
| `M_VFX_DisAdd_Rainbow` | `GradientMap_Displacement` 0.1 → **0.9**; `Gradient_Invert` 0.5 → **2**; `Opacity_Boldness` 0.5 → **1.5**; `Opacty_StepAdd` 0.1 → **0.3**; **`GradientMap_Tex` `T_VFX_WhitePixel` → `T_VFX_LUT_Rainbow_01`**; `GradientShape_Tex` `T_VFX_Noise_02` → `T_VFX_Part_01`; `Main_Tex` → `T_VFX_Ring_02` | Raimbow |

**Every material in this system also appears in another sheet of this batch.** Nothing here is unique;
`M_VFX_DisAdd_Part04` is parameter-identical to NS_BasicAttack's existing `PartDisAdd04` look.

**No sub-UV, no mesh, no `CamOffset`, no `Flames01`** — on the material axis this is the *cheapest*
system in the batch, its only hard dependency being the Rainbow LUT.

### Referenced textures `[corpus]`

| Texture | Size | Source format | Compression | sRGB | Address |
|---|---|---|---|---|---|
| `T_VFX_Part_01` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Part_02` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Part_04` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
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

### Layer 0 — `Raimbow` (Infinite, rate **0.5**/s, lifetime 1.0)
- Initialize Color `RGBA(0.913099, 0.913099, 0.913099, 0.1)`; Uniform Sprite Size **450**.
- Sprite Rotation Mode Random, Min 0 / Max 360.
- Scale Color (RGBA Together):
  `R (0, 0.5)L | G (0, 0.5)L | B (0, 0.5)L | **A (0, 0)L (0.269242, 1)L (1, 0)L**`
  — RGB flat 0.5; alpha is a **triangle** peaking at t = 0.269 (the Cast variants ramp 1 → 0 instead).
- Scale Sprite Size (Uniform Curve): **`(0, 0.8)C (1, 1)L`** — a gentle 0.8 → 1.0 grow, unlike the
  Cast variants' `(0, 0.5)C (0.2, 0.9)C (1, 1)L`.
- Dynamic params: **`[0.5, 0, 0, 0]`** constant.

### Layer 1 — `Sparkles_01` (Once, 0.3 s, rate 10/s)
- Lifetime `[unresolved]` — `0.3 / 0.6` vs override `0.2 / 0.4`.
- **`Cylinder Location`**: Radius **80**, Height **130**, Midpoint 0.5, **Offset `(0, 0, 0)`**,
  Random, Spawn Only, `Surface Only = false`, `Override Local Rotation = true`,
  `Use Endcaps In Surface Only Mode = true`, Non Uniform Scale (1,1,1), Apply Owner Scale (0,0,0).
- `Add Velocity` = `Random Range Vector` **Min `(0, 0, 1000)` → Max `(0, 0, 2000)`** — upward.
- Sprite Size Mode Random Uniform: **Min 6, Max 10**. Initialize Color `RGBA(1, 1, 1, 1)`;
  authored `Sprite Rotation Angle 90` with no `Sprite Rotation Mode` line (a fixed 90° `[inferred]`).
- Scale Velocity: `X/Y/Z: (0, 1)C (0.1, 0.15)C (1, -9.09372e-09)C`.
- Color from Curve — the shared "heal" ramp:
  - `R: (0, 0.715694)C (0.079686, 1)L (0.290975, 0.109462)L (0.713553, 0.024158)C`
  - `G: (0, 0.89627)C (0.079686, 0.854993)L (0.290975, 1)L (0.713553, 0.913099)C`
  - `B: (0, 1)C (0.079686, 0.376262)L (0.290975, 0.617207)L (0.713553, 0.141263)C`
  - `A: (0.080893, 1)L (1, 0)C`
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`; the non-uniform companion
  `X/Y: (0, 0)L (1, 1)L` is not the active channel.
- Dynamic params: **`[3, 0, 0, 0]`** constant.

**Byte-identical to `NS_HealCast`'s layer 4** except for the spawn module (rate-only here; burst 3 @
0.05 + rate 20 there). One behavior layer and one look serve both.

### Layer 2 — `Sparkles_Stretched` (Once, 0.3 s, rate 10/s)
- Lifetime `[unresolved]` — `0.3 / 0.6` vs override `0.2 / 0.4`.
- `Cylinder Location`: Radius **80**, Height **120**, Midpoint 0.5, Offset **(0, 0, 0)**.
- `Add Velocity` = `Random Range Vector` **Min `(0, 0, 1000)` → Max `(0, 0, 1600)`**
  (`NS_HealCast`'s is 1000 → 1700).
- Sprite Size Mode **Random Non-Uniform**: `Sprite Size Min (25, 70)` → `Max (40, 60)`.
  **Note the Y bound inversion `[corpus]`** — min Y (70) exceeds max Y (60). Same as `NS_HealCast`.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.15)C (1, -9.09372e-09)C`.
- Color from Curve: identical keys to layer 1's.
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`.
- Scale Sprite Size 001 (Non-Uniform Curve): `X: (1, 1)L | Y: (0, 1)C (0.3, 0.25)C (1, 0.2)C`.
- **`Scale Sprite Size by Speed`**: `Scale Factor Curve (0, 0)L (1, 1)L`, `Velocity Threshold` **1000**,
  `Min Scale Factor (1, 1)`, `Max Scale Factor (1, 2)`.
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layers 3 & 4 — `Star01` / `Star02` (Once, 0.3 s, rate **1**/s)
**Identical in every parameter except sprite size and material.**
- Lifetime Mode Random, **Min 0.3 / Max 0.6** — unambiguous.
- `Cylinder Location`: Radius **30**, Height **60**, Midpoint 0.5, **Offset `(0, 0, -30)`**
  (spawned below the origin), empty `Lathe Profile` override.
- `Add Velocity`: **`(0, 0, 500)`** constant — straight up.
- Sprite Size Mode Random Uniform: `Star01` **Min 40 / Max 50**, `Star02` **Min 30 / Max 40**.
  (`Sprite Size (10, 10)` and `Uniform Sprite Size 450` present but not selected.)
- Initialize Color `RGBA(1, 0.184475, 0.386429, 0.4)` (overridden by the update curve).
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.25)C (1, -0)C`.
- Color from Curve — the heal ramp with a green-dominant tail:
  - `R: (0, 0.715694)C (0.0796861, 1)L (0.234229, 0.921582)L (0.80652, 0.109462)C`
  - `G: (0, 0.89627)C (0.0796861, 0.854993)L (0.234229, 0.723055)L (0.80652, 1)C`
  - `B: (0, 1)C (0.0796861, 0.376262)L (0.234229, 0.462077)L (0.80652, 0.617207)C`
  - `A: (0.0808934, 1)L (1, 0)C`
- Scale Sprite Size (Uniform Curve): `(0, -3.11599e-08)C (0.1, 1)C (1, 0)C`.
- Dynamic params: **`[1, 0, 0, 0]`** constant.
- Materials swapped relative to emitter names (§2/§4).

**Byte-identical to `NS_HealCast`'s layers 6 & 7** except for the spawn module.

### Layer 5 — `Glow_01` (Infinite, rate 2/s, lifetime 2.0)
- Initialize Color `RGBA(1, 1, 1, 0.4)`; Uniform Sprite Size **550** — the largest in this system.
- Color from Curve — **a constant green with a blue dip**:
  - `R: (0.169031, 0.296138)C (0.470872, 0.296138)L (0.817386, 0.296138)C` *(flat)*
  - `G: (0.169031, 1)C (0.470872, 1)L (0.817386, 1)C` *(flat 1)*
  - `B: (0.169031, 0.737911)C (0.470872, 0.47932)L (0.817386, 0.737911)C` *(dips at mid-life)*
  - `A: (0, 0)C (0.237851, 1)L (0.712345, 1)L (1, 0)L`
- **No size curve** (3 update modules).
- Dynamic params: **`[3, 0, 0, 0]`** constant.

### Layer 6 — `Glow_02` (Infinite, rate 2/s, lifetime 2.0)
- Initialize Color `RGBA(1, 1, 1, 1)`; Uniform Sprite Size **250**.
- Color from Curve — **the most keyed curve in this system: a two-cycle R/B flicker**:
  - `R: (0.169031, 0.0609999)C (0.318744, 1)L (0.470872, 0.077)L (0.62783, 1)L (0.817386, 0.062)C`
  - `G: (0.169031, 1)C (0.318744, 0.78728)L (0.470872, 1)L (0.62783, 0.790767)L (0.817386, 1)C`
  - `B: (0.169031, 0.650356)C (0.318744, 0.085)L (0.470872, 0.317213)L (0.62783, 0.1)L (0.817386, 0.650728)C`
  - `A: (0, 0)C (0.237851, 1)L (0.712345, 1)L (1, 0)L`
  Green stays near 1 while R and B swing between ~0.06 and ~1 twice — a saturated-green ↔ white pulse.
- **No size curve.**
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 7 — `Flares` (Infinite, rate 6/s)
- **`Color Mode = Random Hue/Saturation/Value`** — base `Color RGBA(0.0749313, 1, 0, 1)` (pure green),
  `Hue Shift Range` **(0.2, −0.2)** *(note the descending range, as exported)*,
  `Saturation Range` **(0.35, 0.5)**, `Value Range (1, 1)`, `Alpha Scale Range` **(0.07, 0.1)**,
  `Color Minimum RGBA(0,0,0,1)`, `Color Maximum RGBA(1,1,1,1)`.
  (The `NS_BuffLoop` sibling uses the same mode with a red base and `Saturation Range (0.2, 0.2)` /
  `Alpha Scale Range (0.13, 0.13)`.)
- Lifetime `[unresolved]` — `Lifetime Min 1 / Max 2` vs override `RandomRangeFloat 0.2 / 0.4`.
- `Cylinder Location`: Radius **80**, Height **100**, Midpoint 0.5, **Offset `(0, 0, -10)`**.
- **No velocity module** — flares hold their spawn position.
- Sprite Size Mode Random Uniform: **Min 50, Max 200**.
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0.8)C`.
- Scale Color, `Scale Mode = RGB and Alpha Separately`:
  - `Scale RGBA` curve: `R (0, 1)L (1, 1)L | G (0, 1)L (1, 1)L | B (0, 1)L (1, 1)L | A (0, 0)L (0.236644, 1)L (1, 0)L`
  - `Scale Alpha` (Float from Curve): **`(0, 0)L (0.3, 0.125)L (1, 0)L`**
  In "RGB and Alpha Separately" mode the separate `Scale Alpha` drives alpha, so the effective envelope
  peaks at **0.125** `[inferred — module semantic, not a corpus fact]`.
- Dynamic params: **`[8, 0, 0, 0]`** constant.

### Layer 8 — `Glow_03` (Infinite, rate 2/s, lifetime 2.0)
- Initialize Color `RGBA(1, 1, 1, 1)`; Uniform Sprite Size **220**; material `Part02` (not `Part01`).
- Color from Curve — **identical keys to layer 5's** (flat green, blue dip at mid-life).
- **No size curve.**
- Dynamic params: **`[0, 0, 0, 0]`** constant.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence

**Same continuous-cadence problem as [NS_BuffLoop.md](NS_BuffLoop.md) §6.1 and
[NS_DebuffLoop.md](NS_DebuffLoop.md) §6.1** — nine `Spawn Rate` emitters, no burst; the existing
continuous template row ignores `LoopDuration`/`ParticleLifetime` and has no rate field at all.
Plus the same mixed `Once` / `Infinite` split (four vs five).

This system makes the gap most acute because **its rates are fractional and very low**: `Raimbow` at
0.5/s means one particle every two seconds. Approximating it as a per-loop burst would turn a slow,
sparse drift into a metronome.

Recommendation, identical to the two sibling Loop sheets: **extend `FCk_ParticlesTemplateSpec` with a
real spawn rate and lifetime for continuous rows**, then run one 12.5/s stream (the five infinite
emitters) with lifetime 2.0 and a rate-weighted partition. Because 12.5 is fractional, the weights are
not integers — `Raimbow` 0.5, `Glow_01` 2, `Glow_02` 2, `Flares` 6, `Glow_03` 2. Either scale the
partition by 2 (25 slots: Raimbow 1, Glows 4/4/4, Flares 12) or drive `Raimbow` on an every-other-loop
gate. **The four `Once` layers are not expressible today.**

### 6.2 VisTag / renderer needs

| Renderer | Kind | Look | Source emitters |
|---|---|---|---|
| a | CameraFacingSprite *(NEW KIND REQUIRED)* | `RainbowDisAdd` | Raimbow |
| b | CameraFacingSprite *(new kind)* | `PartDisAdd01Bright` | Sparkles_01 |
| c | CameraFacingSprite *(new kind)* | `StarDisAdd02` | Star01 |
| d | CameraFacingSprite *(new kind)* | `StarDisAdd01` | Star02 |
| e | CameraFacingSprite *(new kind)* | `PartDisAdd01` | Glow_01, Glow_02 |
| f | CameraFacingSprite *(new kind)* | `PartDisAdd02` | Flares, Glow_03 |
| g | VelocityAlignedSprite | `PartDisAdd04` — **already exists** | Sparkles_Stretched |

**Seven row renderers, six needing the new `CameraFacingSprite` kind, one reusing an existing look
verbatim.** Every look here is shared with another sheet in the batch — **NS_HealLoop introduces no
new look and no new texture beyond what its siblings already require.**

### 6.3 Look / material needs

Seven materials → **seven looks**, of which `PartDisAdd04` already exists and the other six are all
authored for `NS_HealCast` / `NS_BuffLoop` / `NS_DebuffLoop` anyway.

Unplumbed family parameters that bite:

| Param | Value here | Plumbed? |
|---|---|---|
| `Glow_Intensity` | 0.3 on `Part02` (two layers) | **no** |
| `Core_Intensity` | 1 on `Part01_Bright` | **no** |
| `Gradient_Invert` | 0 on `Part04`, `Star01/02`; **2 on `Rainbow`** | **no** |
| `GradientMap_Tex` + `GradientMap_Displacement` | LUT + 0.9 on `Rainbow` | **no** — see §6.5 |
| `Opacty_StepAdd` | 0.3 on `Rainbow` | **no** |
| `Opacty_DepthFade` | 10 / 20 / 30 | **no** |

**No `CamOffset`, no `Distortion_*`, no `Color_Core` deltas** — the shading surface this system needs
is the narrowest in the batch.

### 6.4 Texture needs

`T_VFX_Part_01` → existing `SoftParticle`; `T_VFX_Part_04` → existing `SparkStreak`;
`T_VFX_Noise_02` → existing `TileNoise`. New measurement + bake for `T_VFX_Part_02`, `T_VFX_Ring_02`,
`T_VFX_Star_01`, `T_VFX_Star_02` (the existing `Flare` bake may fit the stars — **unmeasured**),
and the **colour LUT** `T_VFX_LUT_Rainbow_01`. **No flipbook atlas needed.**
Method: NS_BasicAttack §7.

### 6.5 CAPABILITY GAPS — read before committing a session

1. **CONTINUOUS-CADENCE ROWS ARE NOT PARAMETERIZABLE.** Nine spawn-rate emitters, no rate field on
   `FCk_ParticlesTemplateSpec`, and the one continuous row's `LoopDuration`/`ParticleLifetime` are
   documented as unused. Shared with `NS_BuffLoop` and `NS_DebuffLoop`. **Fix once, serves three.**

2. **FRACTIONAL SPAWN RATES.** `Raimbow` at **0.5/s** cannot be expressed as an integer per-loop slot
   count on a 1.0 s loop. Whatever cadence mechanism is added must handle sub-1/s rates, or this layer
   doubles in frequency.

3. **MIXED `Once` / `Infinite` LOOP BEHAVIOUR IN ONE SYSTEM — not expressible.** Four emitters stream
   for 0.3 s and stop while five stream forever.

4. **CAMERA-FACING SPRITE ROW RENDERER — does not exist.** Six layers here. Shared batch need; this is
   the single highest-leverage addition for the batch as a whole.

5. **GRADIENT-MAP LUT.** `M_VFX_DisAdd_Rainbow` drives a 512×2 sRGB colour ramp through
   `GradientMap_Tex` at `Displacement 0.9` / `Gradient_Invert 2`. The family shader has no gradient
   chain; the procedural generator makes only greyscale, `SRGB=false`, `TC_VectorDisplacementmap`
   bakes. Without both additions the Raimbow layer is a white glow.
   **This is NS_HealLoop's only hard shader gap.**

6. **`Random Hue/Saturation/Value` colour mode** (`Flares`). Expressible with `CkParticles_Rand` plus
   an HSV→RGB helper, but `Common.ush` has none today and the CPU mirror must match bit-for-bit —
   budget one shared helper written twice. Note the **descending `Hue Shift Range (0.2, −0.2)`** and
   the `Alpha Scale Range (0.07, 0.1)`, both of which a naive `lerp(min, max, rand)` gets subtly wrong
   if it assumes min < max.

7. **`Scale Sprite Size by Speed`** (`Sparkles_Stretched`, up to 2× length above a 1000-unit
   threshold). Expressible, but must be derived from the same closed-form velocity integral as
   position, never from a frame delta (NS_BasicAttack §8, lesson 7).

8. **INVERTED SIZE BOUNDS** on `Sparkles_Stretched` (`Min (25, 70)` / `Max (40, 60)`). `[corpus]`, not
   a transcription error. Any per-component `min <= v <= max` assertion must sort the bounds first.

9. **WORLD SPACE vs LOCAL SPACE.** All nine emitters `LocalSpace: false`; the template is local-space.
   As with the other Loop effects this matters more than for a Cast effect — a heal loop is typically
   attached to a moving character, and the source's particles would be left behind in the world while
   the recreation's follow. **Flag to the maintainer; for this family the local-space behaviour may be
   the more desirable one, but it is a visible difference either way.**

10. **SPRITE ROTATION.** `Raimbow` uses random 0–360°; `Sparkles_01` and `Sparkles_Stretched` carry a
    fixed authored 90°. `[unresolved: whether `Particles.SpriteRotation` is bound on camera-facing
    sprite renderers]` — verify in `CkParticles_TemplateBuilder.cpp`.

11. **No ribbon, no mesh, no sub-UV, no events, no forces, no `CamOffset`.** **This is the cleanest
    system in the batch** — its entire difficulty is cadence (gaps 1–3) plus the rainbow LUT (gap 5).

### 6.6 Behavior id

**Do NOT allocate an id here.** `ck::particles::NumBehaviors` is 18 at the time of writing; the
implementing session allocates from it and bumps it in the same edit.

### 6.7 Complexity assessment

**Tier M**, conditional: **M** if the continuous-cadence row and the camera-facing sprite renderer are
already built by a sibling effect in this batch (they are prerequisites for three of the six), and the
rainbow gradient is dropped as a recorded deviation. **L** if this effect is attempted first, because
it would have to fund both of those capabilities plus the LUT path on its own.

**Recommendation: schedule NS_HealLoop LAST in the batch.** It reuses every look and texture the others
need, introduces nothing new, and is the cheapest possible validation that the shared capabilities work.

---

## 7+. Reserved for implementation — sections 7–14 per [README.md](README.md) are written by the session that implements this effect.
