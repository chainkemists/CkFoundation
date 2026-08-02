# Recipe: NS_BuffCast → CkParticles (IMPLEMENTATION-COMPLETE)

Schema and evidence-tag conventions: [README.md](README.md). Exemplars: [NS_BasicAttack.md](NS_BasicAttack.md),
[NS_Lightning_Range.md](NS_Lightning_Range.md).

## Completion state — READ FIRST

**Status: IMPLEMENTATION-COMPLETE (2026-08-02, Phase 3 batch G). Behavior id 38. Not yet A/B'd.**

`Behavior_BuffCast.ush` + `ExecuteStage_CPU` case 38, the `PS_CkParticles_Template_BuffCast` cadence
row (2.0 s / 1.5 s / burst 23, plus a ribbon emitter bursting 301 EVENT samples), one new CkUsf look
(`TrailDisAdd03`, ribbon-drawn), one new texture (`LinearRamp`), zero new meshes,
`Test_Particles_BuffCastBehavior.cpp`, and a VfxExamples gym pair. Nothing has been rendered or
visually compared — §12 is open.

**This is the cookbook's FIRST consumer of C6c, the event collapse.** §9.2 is the section to read
before touching the trail: the source's ribbon is spawned by per-frame location events, and the
recreation turns those events into closed-form samples of the leader's own path.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_BuffCast` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Corpus system files | `systems/Vefects/Anime_VFX/Shared/Skills/NS_BuffCast.{json,txt}` |
| Corpus material files | `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part02,Rainbow,Arrows,Star01,Part04,Ring01,Trail03}.json` |
| Corpus texture files | `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_02,Part_04,Arrow_01,Star_01,Ring_01,Ring_02,Gradient_02,Noise_02,LUT_Rainbow_01,WhitePixel}.json` |
| Meshes | none — this system has no mesh renderer |

**The source Niagara asset was never opened.** Every fact below is `[corpus]` unless tagged otherwise.

> ### A sibling with a near-identical name exists — take the right one
> `[corpus]` The pack ships `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Buff_Cast` (underscored
> name). It is a **different, parameterized** system. Fastest one-line discriminators, either of
> which settles it on its own:
> - **User parameters.** `NS_BuffCast` exports `userParameters: []`. `NS_Buff_Cast` exports **ten**:
>   `User.Arrow Big Color 01`, `User.Arrow Color 01`, `User.Glow Color 01/02/03`,
>   `User.Rainbow Color 01`, `User.Ring Color 01`, `User.Scale Overall`, `User.Sparkles Color 01`,
>   `User.Sparkles Trail Color 01`.
> - **Renderer materials.** `NS_BuffCast` draws through `M_VFX_DisAdd_*`; `NS_Buff_Cast` draws through
>   `MI_VFX_*` (`MI_VFX_Glow_01/02/04`, `MI_VFX_Arrows_01`, `MI_VFX_Lens_Rainbow_01`,
>   `MI_VFX_Ring_01`, `MI_VFX_Star_01`, `MI_VFX_Trail_03`).
>
> This sheet documents the **`Anime_VFX/Shared/Skills`** variant only — the fixed-colour, no-user-param one.

---

## 2. System anatomy `[corpus]`

**10 CPU emitters, ALL enabled, ALL world-space (`LocalSpace: false`), `Bounds: Dynamic`,
`Determinism: false`, zero user parameters.** Nine are sprite emitters that burst at loop start; the
tenth is a **ribbon**.

**System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.** All ten emitters are
`Life Cycle Mode = System`, so per [P0-D1] this RULES and **every per-emitter Loop row below is
inert** — including the ribbon's `Once / 0.4 s`. *(Was read as a 1.0 s loop from the inert emitter
rows.)*

**23 sprite particles per loop** (1+1+1+1+1+1+7+7+3), plus the ribbon (event-spawned, unbounded —
see below).

| # | Emitter | Spawn | Spawn t | Lifetime | Loop | Renderer / alignment | Material | Size |
|---|---|---|---|---|---|---|---|---|
| 0 | `Bomb_Glow_01` | Burst 1 | 0 | 1.0 | 1.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part01` | uniform 500 |
| 1 | `Bomb_Glow_02` | Burst 1 | 0 | 1.0 | 1.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part01` | uniform 400 |
| 2 | `Bomb_Glow_03` | Burst 1 | 0 | 1.0 | 1.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Part02` | uniform 200 |
| 3 | `Raimbow` *(sic)* | Burst 1 | 0 | 1.0 | 1.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Rainbow` | uniform 450 |
| 4 | `Arrow` | Burst 1 | 0 | **1.5** | 1.0 | Sprite, **VelocityAligned** / FaceCamera | `M_VFX_DisAdd_Arrows` | non-uniform (170, 170) |
| 5 | `BigArrow` | Burst 1 | 0 | **1.5** | 1.0 | Sprite, **VelocityAligned** / FaceCamera | `M_VFX_DisAdd_Arrows` | non-uniform (150, 240) |
| 6 | `Sparkles_02` | Burst **7** | **0.05** | rand **0.4–0.7** `[corpus-v3]` | ~~1.0~~ | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Star01` | rand uniform 40–70 |
| 7 | `Sparkles_01` | Burst **7** | 0 | rand 0.2–0.4 | 1.0 | Sprite, **VelocityAligned** / FaceCamera | `M_VFX_DisAdd_Part04` | rand non-uniform (35,80)–(50,90) |
| 8 | `Ring` | Burst **3** | 0 | rand 0.3–0.7 | 1.0 | Sprite, Unaligned / FaceCamera | `M_VFX_DisAdd_Ring01` | rand uniform 150–160 |
| 9 | `Sparkles_02_Trail` | **event-spawned, 1/event** `[corpus-v3]` | — | 0.2 *(`Initialize Ribbon`)* | ~~0.4, Once~~ *(inert)* | **Ribbon** | `M_VFX_DisAdd_Trail03` | ribbon width 15 |

`Spawn Burst Instantaneous.Loop Count Limit = 1` on all nine sprite emitters, but
`UseLoopCountLimit = false` on all nine — the value is an authored leftover and **never fires**
`[corpus]`. Same trap NS_Lightning_Range records. `[corpus-v3]` Every sprite emitter bursts **once**
over the system's single 2.0 s `Once` loop — *not* "once per 1.0 s loop, forever", which was the
inert-emitter-row reading.

> ### The ribbon's spawn chain — RESOLVED `[corpus-v3]`
> `Sparkles_02_Trail` has **no spawn module in `Emitter Update`** because it is spawned from an
> **event handler**, which the v3 exporter now dumps:
>
> | Field | Value |
> |---|---|
> | `sourceEmitter` | `Sparkles_02` |
> | `eventName` | `LocationEvent` |
> | `executionMode` | `SpawnedParticles` |
> | `spawnCount` | **1 per event** (`randomSpawnCount = false`) |
> | `maxEventsPerFrame` | **0** (unbounded) |
> | handler module | `Receive Location Event` — `Position`, `Acceleration`, `Ribbon ID`, `Ribbon UV Distance`, `Coordinate Space Transform` = **Apply**; **`Velocity`**, `Color`, `Normalized Age`, `Random Normalized Float` = **Output**; `Interpolate Spawned Positions = true` **[P3-G2]** |
>
> The sender is `Sparkles_02`'s `Generate Location Event`. Its `Event Type` is **`Every Frame`**, and
> its own toggles read `Use Event Probability = false` and `UseEventDelay = false` — so the stored
> Send Rate 30, Event Probability 0.5, Delay Before Sending Events 0.5, Movement Tolerance 0.5 and
> Unit Spacing 20 are ALL INERT (corpus caveat 8 in its toggle form). **[P3-G1]** Each of the 7
> `Sparkles_02` particles therefore emits ONE location event PER FRAME from t = 0.05 until it dies,
> and each event spawns exactly one ribbon particle that inherits the emitting particle's POSITION and
> **Ribbon ID** — one ribbon strand per sparkle. It does NOT inherit velocity ([P3-G2]), and its own
> `Add Velocity from Point` is disabled, so a trail point holds where it was placed.
>
> *(Was read as a probabilistic 30/s stream after a delay; the toggles say otherwise.)*
>
> Ribbon particle lifetime is **0.2 s**, from `Initialize Ribbon.Lifetime` — note the emitter's
> `lifetimeResolved` reads `NO_MODULE` because it only inspects `Initialize Particle`.
>
> The ribbon's `Loop Behavior = Once` / duration 0.4 s is **inert**: the emitter is
> `Life Cycle Mode = System`, so the system's `Once / 2.0 s` governs (see the system loop block above).
> *(Was `[unresolved]` — the pre-v3 exporter emitted only three stages per emitter and no event
> handler stack at all.)*

`Sparkles_02`'s `[values]` block additionally carries a full set of `GenerateLocationEvent.*` entries
**and** `Sparkles_02_Trail` carries `RandomRangeFloat001.*` entries for its disabled module — the
exporter dumps the emitter's whole Rapid-Iteration store, including parameters belonging to disabled
or removed modules. **A value in `[values]` is not evidence that a module is running**; cross-check
the module list. (`Scale Sprite Size 001` on `Sparkles_02` is explicitly marked DISABLED and its two
curve overrides are therefore inert.)

---

## 3. Mesh geometry

**N/A — NS_BuffCast has no mesh renderer.** All ten renderers are sprite or ribbon.

---

## 4. Material family and per-instance deltas `[corpus]`

**All eight materials are instances of ONE parent, `/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd`**
— the same family CkUsf already implements as the `DissolveAdd` look entry point. Every one of them:
`MD_Surface`, `BLEND_Translucent`, `MSM_Unlit` (`unlit: true`), `twoSided: false`, connected outputs
`EmissiveColor` + `Opacity`, dynamic-parameter channels **`[dissolve, distortion, offset, core_color]`**,
expression count 161 (identical histograms — one graph, N parameterizations).

Reference row = `M_VFX_DisAdd_Part01` (all-defaults instance). Its absolute values, needed because the
delta table below is relative to it:

`Brightness 1`, `Glow_Intensity 1`, `Opacity_Boldness 0.5`, `Opacty_DepthFade 20`,
`Opacty_Step 0`, `Opacty_StepAdd 0.1`, `Gradient_Invert 0.5`, `GradientMap_Displacement 0.1`,
`Core_Intensity 0`, `Core_Power 1`, `Color_CoreDifferent 0`, `CamOffset 0`, `Dissolve 0`,
`Dissolve_Invert 0`, all `*_Scale_{X,Y} 1`, all `*_Speed_{X,Y} 0`, all `*_Offset_{X,Y} 0`,
`Color_Core RGBA(1, 1, 1, 0)`; textures `Main_Tex`/`Color_Tex`/`Dissolve_Tex` = `T_VFX_Part_01`,
`Distortion_Tex`/`GradientShape_Tex` = `T_VFX_Noise_02`, `GradientMap_Tex` = `T_VFX_WhitePixel` (1×1).

| Material | Δ vs `Part01` | Used by |
|---|---|---|
| `M_VFX_DisAdd_Part01` | (reference — no deltas) | Bomb_Glow_01, Bomb_Glow_02 |
| `M_VFX_DisAdd_Part02` | `Glow_Intensity` 1 → **0.3**; Main/Color/Dissolve_Tex → `T_VFX_Part_02` | Bomb_Glow_03 |
| `M_VFX_DisAdd_Rainbow` | `GradientMap_Displacement` 0.1 → **0.9**; `Gradient_Invert` 0.5 → **2**; `Opacity_Boldness` 0.5 → **1.5**; `Opacty_StepAdd` 0.1 → **0.3**; **`GradientMap_Tex` `T_VFX_WhitePixel` → `T_VFX_LUT_Rainbow_01`**; `GradientShape_Tex` `T_VFX_Noise_02` → `T_VFX_Part_01`; `Main_Tex` → `T_VFX_Ring_02` | Raimbow |
| `M_VFX_DisAdd_Arrows` | `Brightness` 1 → **10**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Arrow_01` | Arrow, BigArrow |
| `M_VFX_DisAdd_Star01` | `Brightness` 1 → **6**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **10**; Main/Color/Dissolve_Tex → `T_VFX_Star_01` | Sparkles_02 |
| `M_VFX_DisAdd_Part04` | `Brightness` 1 → **6**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **30**; Main/Color/Dissolve_Tex → `T_VFX_Part_04` | Sparkles_01 |
| `M_VFX_DisAdd_Ring01` | `Brightness` 1 → **10**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; Main/Color/Dissolve_Tex → `T_VFX_Ring_01` | Ring |
| `M_VFX_DisAdd_Trail03` | `Brightness` 1 → **4**; `Gradient_Invert` 0.5 → **0**; `Opacity_Boldness` 0.5 → **1**; `Opacty_DepthFade` 20 → **30**; Main/Color/Dissolve_Tex → `T_VFX_Gradient_02` | Sparkles_02_Trail (ribbon) |

**Nothing in this system animates `Dissolve_Speed`, `Distortion_Intensity` or `MainTex_Scale`** — the
three deltas `NS_BasicAttack` leaned on are all at family defaults here. The differentiation is
carried by `Brightness`, `Glow_Intensity`, `Opacity_Boldness`, the gradient-map chain, and the shape
texture.

`M_VFX_DisAdd_Part04` here is **byte-identical in parameters** to the `Part04` instance NS_BasicAttack
already recreated as the `PartDisAdd04` look — see §6.

### Referenced textures `[corpus]`

| Texture | Size | Source format | Compression | sRGB | Address |
|---|---|---|---|---|---|
| `T_VFX_Part_01` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Part_02` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Part_04` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Arrow_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Star_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Ring_01` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Ring_02` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| `T_VFX_Gradient_02` | 512×512 | TSF_G8 | TC_Alpha | false | Clamp/Clamp |
| `T_VFX_Noise_02` | 512×512 | TSF_G16 | TC_Alpha | false | Wrap/Wrap |
| **`T_VFX_LUT_Rainbow_01`** | **512×2** | **TSF_BGRA8** | **TC_Default** | **true** | Wrap/Wrap |
| `T_VFX_WhitePixel` | 1×1 | TSF_RGBA16 | TC_Default | true | Wrap/Wrap |

Everything except the LUT is a greyscale mask that the existing procedural-texture generator can plausibly
stand in for. **`T_VFX_LUT_Rainbow_01` is not a mask — it is a 512×2 sRGB colour ramp**, and it is the
Rainbow layer's entire visual identity. See §6.

---

## 5. Per-layer runtime curves `[corpus]`

Every curve samples **NormalizedAge** (age / that emitter's own lifetime) unless stated. `C` = constant
(step) key, `L` = linear key. Keys are transcribed at the corpus's own precision.

`Dynamic Material Parameters` writes **Index 0 only** on every emitter (`Write Parameter Index 0 = true`,
1/2/3 false), so a single float4 `[dissolve, distortion, offset, core_color]` per particle.

### Layer 0 — `Bomb_Glow_01`
- Initialize Color: `RGBA(1, 0.184475, 0.386429, 0.4)`; Uniform Sprite Size **500**; Lifetime 1; Position Offset (0,0,0); `UsePositionOffset = false`.
- Scale Color (RGBA Together): `R (0, 1)L (1, 1)L | G (0, 1)L (1, 1)L | B (0, 1)L (1, 1)L | A (0, 1)L (1, 0)L`.
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (0.1, 1)L (1, 1)L`.
- Dynamic params: **`[1, 0, 0, 0]`** constant (dissolve = 1).

### Layer 1 — `Bomb_Glow_02`
- Initialize Color: `RGBA(0.913099, 0.0193824, 0.130136, 0.4)`; Uniform Sprite Size **400**; Lifetime 1.
- Scale Color: identical to layer 0 (`RGB` flat 1, `A (0, 1)L (1, 0)L`).
- Scale Sprite Size: identical to layer 0 — `(0, 0.5)C (0.1, 1)L (1, 1)L`.
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 2 — `Bomb_Glow_03`
- Initialize Color: `RGBA(0.313989, 0, 0.00227652, 0.483)`; Uniform Sprite Size **200**; Lifetime 1.
- Scale Color: identical to layer 0.
- Scale Sprite Size: identical to layer 0.
- Dynamic params: **`[2, 0, 0, 0]`** constant (dissolve = 2 — past full erosion at the family's threshold; the
  layer reads as a soft glow rather than a dissolving shape `[inferred]`).

### Layer 3 — `Raimbow`
- Initialize Color: `RGBA(0.913099, 0.913099, 0.913099, 0.1)`; Uniform Sprite Size **450**; Lifetime 1.
- **Sprite Rotation Mode = Random**, `Sprite Rotation Angle Min 0` / `Max 360`.
- Scale Color (RGBA Together): `R (0, 0.5)L | G (0, 0.5)L | B (0, 0.5)L | A (0, 1)L (1, 0)L`
  — RGB is a **single key at 0.5**, i.e. a flat 0.5 multiplier; alpha ramps 1 → 0 over life.
- Scale Sprite Size: `(0, 0.5)C (0.2, 0.9)C (1, 1)L`.
- Dynamic params: **`[0.5, 0, 0, 0]`** constant.

### Layer 4 — `Arrow`
- Lifetime **1.5** (exceeds the 1.0 s loop by 0.5 s → generations overlap by half a loop).
- Initialize Color `RGBA(1, 1, 1, 1)`; **Position Offset `(0, 0, -119.316)`** with `UsePositionOffset = true`.
- `Add Velocity`: **`(0, 0, 550)`**, `Scale Added Velocity (1,1,1)` — straight up.
- Sprite Size Mode Non-Uniform, `Sprite Size (170, 170)`. (`Sprite Size Min (5,5)` / `Max (10,10)` and
  `Uniform Sprite Size 150` / `Min 5` / `Max 10` are also in the store but the mode selects the direct
  `Sprite Size` value.)
- Sprite Rotation Mode Random, authored `Sprite Rotation Angle 0.827273`, Min 0 / Max 360.
- Scale Velocity (Vector from Curve), all three axes identical: `X/Y/Z: (0, 1)C (0.2, 0.3)C (1, 0.05)C`.
- Solve Forces and Velocity: `Clamp Velocity = false`, `Limit Acceleration = false`, Rotational Solver on.
  (`Acceleration Limit 9999` / `Speed Limit 1000` are present but both limiters are disabled — inert.)
- Color from Curve:
  - `R: (0, 1)C (0.0784787, 1)L (0.283731, 1)L (0.625415, 0.672443)L (0.947781, 0.223228)C`
  - `G: (0, 0.913099)C (0.0784787, 0.501026)L (0.283731, 0.0773835)L (0.625415, 0.021219)L (0.947781, 0)C`
  - `B: (0, 0.584079)C (0.0784787, 0.0559999)L (0.283731, 0.025)L (0.625415, 0.0168074)L (0.947781, 0.116971)C`
  - `A: (0.0869303, 1)C (1, 0)C`
- Scale Sprite Size (Uniform Curve): `(0, 0.4)C (0.1, 1)C (1, 0.4)C`.
- Scale Sprite Size 001 (Non-Uniform Curve): `X: (0, 0.2)C (0.3, 0.7)C | Y: (0.2, 1)L (1, 1.2)L`
  (its `Uniform Curve Sprite Scale = (0, 0)L (1, 1)L` is not the active channel in Non-Uniform mode).
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 5 — `BigArrow`
- Lifetime **1.5**; Initialize Color `RGBA(1, 1, 1, 1)`; **Position Offset `(0, 0, -52.2087)`**, `UsePositionOffset = true`.
- `Add Velocity`: **`(0, 0, 150)`**.
- Sprite Size Mode Non-Uniform, `Sprite Size (150, 240)`; Uniform Sprite Size 300 present but not selected.
- Sprite Rotation Mode Random, authored angle 0.827273, Min 0 / Max 360.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.3, 0.05)C` — **two keys**, a harder and later cut than Arrow's three.
- Color from Curve (note G and B differ from Arrow's):
  - `R: (0, 1)C (0.0784787, 1)L (0.283731, 1)L (0.625415, 0.672443)L (0.947781, 0.223228)C`
  - `G: (0, 0.913099)C (0.0784787, 0.489926)L (0.283731, 0.0925239)L (0.625415, 0.021219)L (0.947781, 0)C`
  - `B: (0, 0.584079)C (0.0784787, 0.035)L (0.283731, 0.0409999)L (0.625415, 0.0168074)L (0.947781, 0.116971)C`
  - `A: (0.0869303, 1)C (1, 0)C`
- **No Scale Sprite Size module at all** (5 update modules) — BigArrow holds its authored size for its whole life.
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 6 — `Sparkles_02` (7 particles, spawn t = 0.05)
- **Lifetime `[corpus-v3]`: `Lifetime Min 0.4` / `Lifetime Max 0.7` DRIVES.** `Lifetime Mode = Random`,
  so per [P0-D2] the Min/Max pins are the driving pins and the `[override] Lifetime = dyn:Random Range
  Float` (0.2 / 0.4) sits on the unselected Direct-Set input and is INERT
  (`lifetimeResolved.source = minmax`, override under `inertOverrides`). The sheet's `[inferred]`
  guess that 0.4–0.7 wins is now MECHANICALLY CONFIRMED; no editor check needed.
  *(The same shape recurs on every `Sparkles_*` emitter across this batch — all resolve the same way.)*
- `Sphere Location`: `Sphere Radius` **0.5**, Random distribution, Spawn Only, `Surface Only = false`,
  Offset (0,0,0), Non Uniform Scale (1,1,1). A 0.5-unit sphere — effectively a point.
- `Add Velocity from Point`: strength from `Random Range Float 001` **Min 1000, Max 1000** (a constant
  1000), Origin Offset (0,0,0), Velocity Falloff Distance 100.
- Sprite Size Mode Random Uniform: **Min 40, Max 70**. Sprite Rotation Mode Random, authored angle 90, Min 0 / Max 360.
- Initialize Color `RGBA(1, 1, 1, 1)`.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.2)C (1, 3.91223e-08)C` (final key is numerically zero).
- Color from Curve:
  - `R: (0, 1)C (0.416541, 1)L (1, 1)C`
  - `G: (0, 0.708376)C (0.416541, 0.341915)L (1, 0.109462)C`
  - `B: (0, 0.0466651)C (0.416541, 0.109462)L (1, 0.130136)C`
  - `A: (0, 1)C`  *(single key — alpha is flat 1 for the whole life)*
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`.
- **Scale Sprite Size 001 is DISABLED** — its `X: (0, 0.2)C (0.3, 0.7)C | Y: (0.2, 1)L (1, 1.2)L` is inert.
- `Sprite Rotation Rate` (Float from Curve): **`(0, 720)C (1, 0)C`** — 720 °/s spin decaying to 0.
- `Generate Location Event`, `Event Type = Every Frame`, `Use Event Probability = false`,
  `UseEventDelay = false` — one event per particle per FRAME, unconditionally. The stored Send Rate 30 /
  Event Probability 0.5 / Delay 0.5 / Movement Tolerance 0.5 / Unit Spacing 20 belong to the other event
  types and are inert **[P3-G1]**. **This IS the ribbon's driver, confirmed `[corpus-v3]` — see §2's
  event-chain box.**
- Dynamic params: **`[1, 0, 0, 0]`** constant.

### Layer 7 — `Sparkles_01` (7 particles, spawn t = 0)
- Lifetime Random: `Lifetime Min 0.2` / `Max 0.4`, and its `[override] Lifetime = dyn:Random Range Float`
  resolves `RandomRangeFloat Min 0.2 / Max 0.4` — **the two agree here, so 0.2–0.4 is unambiguous**
  (unlike layer 6).
- `Sphere Location`: `Sphere Radius` **0.1**, Random, Spawn Only, Surface Only false.
- `Add Velocity from Point`: strength `Random Range Float 001` **Min 1300, Max 2000** — a real random
  range, unlike layer 6's constant 1000.
- Sprite Size Mode **Random Non-Uniform**: `Sprite Size Min (35, 80)`, `Max (50, 90)` — width × length,
  velocity-aligned streaks. Initialize Color `RGBA(1, 1, 1, 1)`.
- Scale Velocity: `X/Y/Z: (0, 1)C (0.2, 0.35)C (1, 0.05)C`.
- Color from Curve:
  - `R: (0, 1)C (0.416541, 1)L (1, 1)C`
  - `G: (0, 0.69869)C (0.416541, 0.30758)L (1, 0.109462)C`
  - `B: (0, 0.0149999)C (0.416541, 0.063)L (1, 0.130136)C`
  - `A: (0, 1)C`
- Scale Sprite Size (Uniform Curve): `(0, 0)C (0.1, 1)C (1, 0)C`; its non-uniform companion
  `X: (0, 0)L (1, 1)L | Y: (0, 0)L (1, 1)L` is not the active channel in Uniform mode.
- Scale Sprite Size 001 (Non-Uniform Curve): `X: (1, 1)L | Y: (0, 1)C (1, 0.6)C` — the streak shortens
  to 0.6 of its length over life while its width holds.
- Dynamic params: **`[0, 0, 0, 0]`** constant.

### Layer 8 — `Ring` (3 particles, spawn t = 0)
- Lifetime Mode **Random**: Min **0.3**, Max **0.7**. (`InitializeParticle.Lifetime = 1` is also in the
  store; Random mode means Min/Max win and the 1 is inert `[corpus]`.)
- **`Color.Scale Alpha` = 0.25** — the only emitter in this system below 1, so every alpha value below is
  quartered. *(Was MISSING from this section — the [P2-D2] class, corrected as **[P3-G3]**.)*
- Sprite Size Mode Random Uniform: **Min 150, Max 160** (`Uniform Sprite Size 200` present but not selected).
- Sprite Rotation Mode Random, Min 0 / Max 360. Initialize Color `RGBA(1, 1, 1, 1)`.
- Color from Curve:
  - `R: (0.0446725, 1)C (0.216118, 1)L (0.418956, 1)L (0.814971, 0.672443)L (0.988832, 0.223228)C`
  - `G: (0.0446725, 0.913099)C (0.216118, 0.502887)L (0.418956, 0.501026)L (0.814971, 0.021219)L (0.988832, 0)C`
  - `B: (0.0446725, 0.584079)C (0.216118, 0.0561285)L (0.418956, 0.056)L (0.814971, 0.016807)L (0.988832, 0.116971)C`
  - `A: (0, 0)L (0.076064, 1)C (1, 0)C`
- **Dynamic param 1 is the only ANIMATED dynamic channel in this whole system**:
  `Index 0 Param 1 = Float from Curve (0, -0.325)C (1, -0.5)C` — the dissolve threshold slides
  −0.325 → −0.5 over life. Params 2/3/4 constant 0.
- Scale Sprite Size (Uniform Curve): `(0, 0.5)C (0.1, 0.9)C (1, 1)C`.

### Layer 9 — `Sparkles_02_Trail` (ribbon)
- Emitter State: **Loop Behavior = Once**, Loop Duration **0.4** (every other emitter is Infinite / 1.0).
- `Initialize Ribbon`: Color `RGBA(1, 1, 1, 1)`, Lifetime **0.2**, **Position Offset `(100, 0, 0)`**
  (`UsePositionOffset = false`, so the offset is inert `[corpus]`), **Ribbon Width 15**, Ribbon Width Mode Direct Set.
- Color from Curve — **and its `CurveIndex` is `linked:Emitter.Age`, not NormalizedAge**:
  - `R: (0, 1)C (0.416541, 1)L (1, 1)C`
  - `G: (0, 0.708376)C (0.416541, 0.341915)L (1, 0.109462)C`
  - `B: (0, 0.0466651)C (0.416541, 0.109462)L (1, 0.130136)C`
  - `A: (0, 1)C (0.119529, 1)L (0.997283, 0)L`
- `Scale Ribbon Width` (Float from Curve): `(0, 1)C (1, 0)C`.
- Dynamic params: **`[1, 0, 0, 0]`** constant.
- `Add Velocity from Point` is present but **DISABLED**; its `RandomRangeFloat001 Min/Max 1000` is inert.

---

## 6. Translation plan (CkParticles / CkUsf) — AS IMPLEMENTED

*This section is REWRITTEN in place. Its original text was authored before Phase 1's and Phase 3's
capability work and listed five blockers (camera-facing row renderer, ribbon renderer, event spawning,
gradient LUT, sprite rotation) that no longer exist. Each is resolved below; the batch-D precedent
(supersede a stale plan rather than annotate it) applies.*

### 6.1 Cadence row

**A new row, `PS_CkParticles_Template_BuffCast`:**

| Field | Value | Why |
|---|---|---|
| Loop duration | **2.0 s** `[corpus-v3]` | the system's `Once` loop duration ([P0-D1]/[P0-D3]); every per-emitter `Loop Duration = 1` is inert |
| Particle lifetime | **1.5 s** | [P0-D5] max over layers of (spawn delay + resolved lifetime) — `Arrow`/`BigArrow`, both 1.5 s off a zero beat |
| Burst count | **23** | 1+1+1+1+1+1+7+7+3, the nine sprite emitters' own counts (§2) |
| Spawn rate | **0** | every sprite emitter is a `Spawn Burst Instantaneous`; nothing streams |
| Ribbon emitter | **burst 301**, one `Ribbon` renderer | 43 event samples for each of the seven sparkles — see §9.2 |

Layer partition is `Seed % 23`, in table order: 0 `Bomb_Glow_01`, 1 `Bomb_Glow_02`, 2 `Bomb_Glow_03`,
3 `Raimbow`, 4 `Arrow`, 5 `BigArrow`, 6–12 `Sparkles_02`, 13–19 `Sparkles_01`, 20–22 `Ring`.

### 6.2 VisTag / renderer needs — RESOLVED

`ECk_ParticlesRenderer_Kind::CameraFacingSprite` landed in Phase 1 and `::Ribbon` in Phase 3, so both
original blockers are closed. Seven row renderers cover the nine sprite emitters, plus the ribbon:

| VisTag | Kind | Look | Source emitters |
|---|---|---|---|
| 167 | CameraFacingSprite | `PartDisAdd01` | Bomb_Glow_01, Bomb_Glow_02 |
| 168 | CameraFacingSprite | `PartDisAdd02` | Bomb_Glow_03 |
| 169 | CameraFacingSprite | `RainbowDisAdd` | Raimbow |
| 170 | VelocityAlignedSprite | `ArrowsDisAdd` | Arrow, BigArrow |
| 171 | CameraFacingSprite | `StarDisAdd01` | Sparkles_02 |
| 172 | VelocityAlignedSprite | `PartDisAdd04` | Sparkles_01 |
| 173 | CameraFacingSprite | `RingDisAdd01` | Ring |
| 174 | **Ribbon** (ribbon emitter) | `TrailDisAdd03` | Sparkles_02_Trail |

`Get_BehaviorLookName(38)` stays `NAME_None` — every look binds explicitly on its own renderer.

### 6.3 Look / material needs — SEVEN REUSED, ONE NEW

All eight source materials are `M_VFX_DissolveAdd` instances. Seven were already carried by earlier
ports and were checked value-by-value against §4 before reuse (§10). Only `M_VFX_DisAdd_Trail03` is
new, and only because it is the first ribbon-drawn instance of the family.

### 6.4 Texture needs — ONE NEW BAKE

See §7. Eight of the nine masks are paints an earlier batch already measured and baked; the ninth,
`T_VFX_Gradient_02`, is not a painting at all.

### 6.5 The original capability gaps, closed

1. **Ribbon renderer** — landed as C6a/[P3-D1] (second emitter + seed bank + `RibbonIdBinding`). The
   trail is recreated in full, not approximated and not dropped.
2. **Event-driven spawning** — expressed by C6c's collapse (§9.2), not by an event mechanism.
3. **Camera-facing row renderer** — landed in Phase 1 (C1).
4. **Gradient-map LUT** — the chain is plumbed (C3) and `RainbowDisAdd` already carries the
   displacement and invert values; the ramp itself stays `LutWhite` pending [P1-D1], exactly as every
   other Rainbow consumer does. Recorded in §13.
5. **World vs local space** — the C12 known difference, §13.
6. **Sprite rotation** — `Particles.SpriteRotation` is bound on every row-declared sprite renderer;
   the four rotating layers are reproduced, including `Sparkles_02`'s decaying 720°/s spin.
7. **Non-uniform sprite size** — `OutSize` is a float2 and the velocity-aligned kind stretches on
   `Size.y`, which is what the two chevrons and `Sparkles_01` need.
8. **No sub-UV in this system** — still true; nothing here declares a `SubImageSize`.

### 6.6 Corrections applied to this sheet at implementation

- **[P3-G1]** §2's event-chain box read `Generate Location Event`'s stored values as live: "Send Rate
  30, Event Probability 0.5, Delay Before Sending Events 0.5". The module's own toggles say otherwise
  — `Event Type = Every Frame`, `Use Event Probability = false`, `UseEventDelay = false` — so the
  probability, the delay AND the send rate are all INERT and the emitter sends one event per particle
  per FRAME. Corpus caveat 8 (`[values]` presence ≠ evidence), in its toggle form.
- **[P3-G2]** the same box listed `Velocity` among the handler's **Apply** fields. The corpus reads
  `Velocity (Vector 2) = Output`, so a ribbon point does NOT inherit the sparkle's velocity — which,
  with its own `Add Velocity from Point` disabled, is why a trail point holds its position. This is the
  fact the whole collapse rests on; the sheet's reading would have had the trail flying apart.
- **[P3-G3]** §5's layer 8 (`Ring`) omitted `Color.Scale Alpha`, which the corpus gives as **0.25**.
  The [P2-D2] class, fourth sighting: the layer's alpha peaks at a quarter, not at one.

---

## 7. Textures — ONE new bake

Eight of the nine masks this system needs already exist, each measured off this very paint by an
earlier batch: `T_VFX_Part_01` → `SoftParticle`, `T_VFX_Part_02` → `SoftParticleBright`,
`T_VFX_Part_04` → `SparkStreak`, `T_VFX_Arrow_01` → `ArrowChevron`, `T_VFX_Star_01` → `StarFour`,
`T_VFX_Ring_01` → `RingUneven`, `T_VFX_Ring_02` → `RingFlare`, `T_VFX_Noise_02` → `TileNoise`.
`T_VFX_LUT_Rainbow_01` → `LutRainbow` exists too and is held back only by [P1-D1].

### 7.1 `LinearRamp` — a TRANSCRIPTION, not a stand-in

`T_VFX_Gradient_02` was measured before anything was written, per the measure-before-reuse rule. The
measurement ended the question immediately:

- the image is **exactly `1 - u`**: the largest deviation from that closed form anywhere in the 512²
  is **0.0024**, which is under one 8-bit quantum and is precisely the difference between the source's
  `u = X/511` sampling and this baker's `(X + 0.5)/512`;
- it is **constant in v** to **1.4e-14** (row-wise standard deviation);
- its best correlation against every other paint in the corpus is **0.084** (`T_VFX_Noise_07`), so
  there is nothing to reuse.

A ramp is functional CONFIG rather than art — the same call `Px_LutWhite` already made — so the bake
is the closed form itself and carries no fitted constants. `Px_LinearRamp` is two lines.

---

## 8. Meshes — none

`NS_BuffCast` has no mesh renderer. Nothing was generated.

---

## 9. The behavior — `Behavior_BuffCast.ush` + `ExecuteStage_CPU` case 38

### 9.1 The sprite layers

Nine layers, partitioned by `Seed % 23`, each hiding itself past its own lifetime (the NS_BasicAttack
§8 mechanism) since the row's 1.5 s is the longest of them. Everything is a direct transcription of
§5; the three shapes worth naming:

- **The two chevrons compose TWO size modules.** `Arrow` runs a uniform curve AND a non-uniform one,
  so its quad opens at a fifth of its width and stretches to 1.2 of its length; `BigArrow` carries no
  size module at all and holds its authored `(150, 240)` for its whole life.
- **`Raimbow`'s Scale Color has a single RGB key**, so the tint is a flat 0.5 multiplier and NOT a
  ramp — the layer's colour never moves, only its alpha does.
- **`Ring` is the only animated dynamic channel in the system**: its dissolve threshold slides
  −0.325 → −0.5 over life, so the shockwave erodes rather than fading.

### 9.2 The event collapse — C6c's first real consumer

The source's trail is spawned by events, and the corpus states the whole chain: `Sparkles_02` runs a
`Generate Location Event` whose **`Event Type` is `Every Frame`**, with `Use Event Probability` and
`UseEventDelay` both **false** ([P3-G1]). Each event spawns exactly one ribbon particle through a
`Receive Location Event` that **Applies** Position, Acceleration, Ribbon ID, Ribbon UV Distance and the
coordinate-space transform, and merely **Outputs** Velocity, Colour, Normalized Age and the random
float ([P3-G2]). The ribbon emitter's own `Add Velocity from Point` is DISABLED.

Put together: a ribbon point is a **sample of the leader's position, taken at the instant the event
fired, and held**. That is exactly what a stateless closed form can express, and it is what C6c
predicted:

```
trail point (Strand, Step)  ->  CkParticles_BuffCast_SparklePos(6 + Strand, Step / 60)
```

- **`Strand` is the ribbon id** (`LocalSeed / 43`), matching the source's per-particle Ribbon ID: seven
  sparkles, seven strands, one renderer, separated by `RibbonIdBinding` ← `Particles.MeshIndex`.
- **`Step / 60` is the event time.** The source emits one event per FRAME, so the sample spacing is a
  frame; 60 Hz is the reference rate it is quoted against, stated rather than tuned, and 43 steps is
  what covers the 0.7 s maximum a sparkle can live. A point whose step is past its own leader's death
  is hidden — that event was never sent.
- **The leader is the SAME function the sparkle sprite draws itself with.** Not a re-derivation: one
  `CkParticles_BuffCast_SparklePos`, called from both branches, so the trail sits on the sparkle by
  construction and the test asserts it as an identity rather than within a tolerance.
- **The colour rides `Emitter.Age`.** The source's `Color from Curve` on the ribbon has
  `CurveIndex = linked:Emitter.Age`, so every live trail point carries the same colour at any instant
  and the whole trail fades together at 0.997 s. A ribbon burst particle's own age IS the loop clock,
  so the recreation reads `In.Age` for the colour and the point's own 0.2 s window separately.
- **The width is the ribbon's own**: 15 units under a `Scale Ribbon Width` curve running 1 → 0 across
  the point's 0.2 s, written to `Size.x` (which is the one float `RibbonWidthBinding` reads).

### 9.3 Why the ribbon emitter BURSTS rather than streams

The source's cadence is per-frame, i.e. a rate. A rate on the ribbon emitter would force the behavior
to read the emitter clock to know when each point spawned, and the point's own position depends on
that time. A BURST does not: burst particles land at loop phase zero, so `In.Age` is the loop clock
directly and the point index carries its own sample time. The row therefore states the population as
a capacity (301 = 7 × 43) and the behavior solves the times — the same shape NS_Bomb_Projectile used
for arc length, applied to time instead of distance.

### 9.4 The leader identity, and what it costs

The two emitters have separate `Particles.UniqueID` counters, so there is no particle identity shared
between them that does not assume UniqueID arithmetic (that ids start at zero and never skip) which
the CPU mirror cannot verify. The recreation therefore names the leader by its **burst slot**: the
sparkle layer draws its lifetime, direction, size and rotation from `Seed % 23` rather than from
`Seed`, and the trail's strand index names the same slot.

The trade is stated rather than hidden: the trail sits exactly on a drawn sparkle (the thing the
effect is *about*), and in exchange the seven sparkles take the same seven paths on every firing where
the source re-randomizes. Recorded in §13.2.

---

## 10. Looks and renderers

**Seven looks reused, one new.** Every reuse was checked value-by-value against §4's delta table
before it was taken:

| Source material | Look | Check |
|---|---|---|
| `M_VFX_DisAdd_Part01` | `PartDisAdd01` | Brightness 1, Opacity_Boldness 0.5, `SoftParticle` — the family reference, matches |
| `M_VFX_DisAdd_Part02` | `PartDisAdd02` | Brightness 1 × Glow_Intensity 0.3 = 0.3, Opacity_Boldness 0.5, `SoftParticleBright` — matches |
| `M_VFX_DisAdd_Rainbow` | `RainbowDisAdd` | Displacement 0.9, Gradient_Invert 2, Boldness 1.5, `RingFlare`/`SoftParticle` — matches |
| `M_VFX_DisAdd_Arrows` | `ArrowsDisAdd` | Brightness 10, Boldness 1, `ArrowChevron` — matches |
| `M_VFX_DisAdd_Star01` | `StarDisAdd01` | Brightness 6, Gradient_Invert 0, Boldness 1, `StarFour` — matches |
| `M_VFX_DisAdd_Part04` | `PartDisAdd04` | Brightness 6, `SparkStreak` — matches |
| `M_VFX_DisAdd_Ring01` | `RingDisAdd01` | Brightness 10, Gradient_Invert 0, Boldness 1, `RingUneven` — matches |
| `M_VFX_DisAdd_Trail03` | **`TrailDisAdd03`** (new) | Brightness 4, Gradient_Invert 0, Boldness 1, `LinearRamp` on shape AND dissolve |

`TrailDisAdd03` opts into `_UsedWithNiagaraRibbons`. It is the third look to do so and the first from a
system whose ribbon is event-spawned.

---

## 11. Tests

`Test_Particles_BuffCastBehavior.cpp` (`CkTests.UnitTests.CkParticles.BuffCastBehavior`), on the CPU
mirror — no Niagara, no template asset, no RHI, no forked engine. What it pins:

- the cadence row and its ribbon emitter, by value;
- the burst partition per VisTag (2/1/1/2/7/7/3) and its stability across 500 moduli;
- **the event collapse**, as an identity: a trail point at (strand, step) sits exactly where the
  sparkle layer draws sparkle `6 + strand` at age `step / 60`, over 35 samples;
- **the emitter-clock colour**: two trail points with different spawn times, sampled at one instant,
  carry the same colour — an implementation that indexed the point's own age fails only this;
- the trail's ramp at both ends (green 0.664387 at emitter age 0.05, 0.341915 at 0.416541) and its
  width taper 15 → 7.5 → gone;
- the strand partition (7 × 43) and a seed bank that is disjoint in BOTH directions;
- **`Ring`'s alpha peaks at a quarter** — the [P3-G3] correction, bounded two-sided so both a missing
  scale (peaks at 1) and a dead curve (peaks at 0) fail;
- Arrow's and BigArrow's colour at both ramp ends AND on the ramp (0.250205 vs 0.254641 at t = 0.2 —
  two curves that a copy-paste implementation would collapse into one);
- `Raimbow`'s flat tint;
- emitter-clock independence over 400 seeds × 3 clocks.

---

## 12. Verification — A/B protocol `[HUMAN-VERIFY]`

Gym: **VfxExamples**, pair **"BUFF CAST"** (behavior 38 against
`/Game/Vefects/Anime_VFX/Shared/Skills/NS_BuffCast`). `Ck_GymVfxExamples_RestartAll` re-fires both
sides in sync. Judge in this order:

a. **The rise.** Both chevrons start about a metre below the cast point and climb — Arrow fast and
   shrinking, BigArrow slow and fixed. If either sits still, the velocity integral is wrong.
b. **The sparkle trails.** Seven short streamers, each one BEHIND a visible sparkle and following it.
   A trail that floats free of any sparkle is the §9.4 identity breaking.
c. **The trail fades TOGETHER.** All seven strands go out at the same moment, about a second in, not
   one point at a time. That is the `Emitter.Age` colour index.
d. **The rings.** Three shockwaves at a quarter alpha, eroding rather than fading.
e. **The rainbow lens.** A flat grey-white lens ring, not a colour sweep — the LUT is held white by
   [P1-D1], so a colour difference here is EXPECTED and is not a port defect.
f. **The palette.** Deep pink-red glows, warm sparkles. If the whole thing reads white, the looks are
   falling back to the default material.

---

## 13. Confirmed fidelity differences or intentional deviations

1. **Trail density is quoted at 60 Hz.** The source emits one location event per FRAME per sparkle, so
   its trail density is genuinely frame-rate dependent; the recreation states 60 as the reference rate
   and spaces its samples at 1/60 s. At another frame rate the source's trail is denser or sparser and
   the recreation's is not. The visual difference is point spacing along an already-continuous ribbon.
2. **The seven sparkles repeat across firings.** Their per-particle randomness is drawn from the burst
   SLOT rather than the UniqueID, which is what lets the trail attach to a drawn sparkle (§9.4). The
   source re-randomizes each firing. Visible only by watching several firings in a row.
3. **World space.** All ten source emitters are `LocalSpace: false`; the CkParticles template is
   local-space. The C12 non-goal — visible only if the spawning actor moves during the 1.5 s life, and
   the A/B pedestals do not move.
4. **The Rainbow LUT is white.** `RainbowDisAdd` carries the source's displacement (0.9) and invert (2)
   but binds `LutWhite` rather than `LutRainbow`, pending [P1-D1]'s open `Gradient_Invert` remap. Same
   deferral every other Rainbow consumer takes.
5. **Family parameters the look does not plumb**: `Opacty_DepthFade` (20/10/30 across these instances),
   `Opacty_StepAdd`, `Core_Power`, `Color_CoreDifferent`. The pre-existing CkUsf gap, unchanged here.
6. **The ribbon's `Position Offset (100, 0, 0)` is inert** in the source (`UsePositionOffset = false`)
   and is therefore not reproduced. Documentary — a source quirk, not a difference.

---

## 14. Reusable lessons

1. **`[values]` blocks lie about toggles, not just about disabled modules.** `Generate Location Event`
   stores an Event Probability of 0.5 and a 0.5 s delay, and BOTH are gated off by their own `Use…`
   booleans. The pre-implementation sheet read the values and built a story around them ("up to 30/s,
   50 % probability, after a 0.5 s delay") that the corpus contradicts on three counts. Read the toggle
   before the value, every time.
2. **An event chain collapses cleanly only if the leader is a FUNCTION, not a particle.** The moment
   the trail and the leader share one closed form, the collapse is exact and the test can assert an
   identity. Two transcriptions of the same curves could only have been asserted within a tolerance,
   and would have drifted at the first edit.
3. **A per-frame source rate becomes a BURST, not a row rate.** A rate forces the behavior to read the
   emitter clock; a burst carries its own sample index and leaves the clock unread. Where the sample
   TIMES are solvable — from a frame cadence here, from a falling rate in NS_Lightning_Muzzle, from arc
   length in NS_Bomb_Projectile — the burst is the better shape, and it keeps `RosterSanity`'s
   emitter-clock independence assertion meaningful for the row.
4. **Measure the paint before writing a generator for it.** `T_VFX_Gradient_02` looks like a texture and
   is a two-line closed form. Ten minutes of measurement replaced what would have been a fitted bake.
