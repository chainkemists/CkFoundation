# Translation sheet: NS_Fire (Vefects Anime VFX)

Schema and evidence-tag conventions: [README.md](README.md). Exemplars: [NS_BasicAttack.md](NS_BasicAttack.md),
[NS_Lightning_Range.md](NS_Lightning_Range.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior, no `.ush`, no look, no mesh, no texture, no cadence row, no test, no gym station exists
for this effect. No behavior id is allocated. Nothing has been rendered or looked at. Sections 1–6 are
archaeology and a plan; everything in them comes from the extracted corpus and is tagged `[corpus]`.

**This is the SIMPLEST system in the batch**: four sprite emitters, no meshes, three materials, all
from the already-implemented `DissolveAdd` family. It has exactly **one** hard capability gap (sub-UV
flipbook) and one soft one (a randomized per-loop burst count). If the batch is scheduled, start here.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Fire` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| User parameters | **none** — `userParameters: []` `[corpus]` |
| Behavior id | **not allocated** — take the next free id at implementation time from `ck::particles::NumBehaviors` |

Corpus evidence (regenerate per [README.md](README.md); `Saved/` is machine-local and gitignored):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Fire.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part04,Flames01}.json`
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_04,Wind_01,Noise_02,Noise_04}.json`

**The source Niagara asset was never opened, in the Niagara editor or otherwise.** Every fact below is
`[corpus]` unless tagged otherwise.

> ### ⚠ TWO SYSTEMS SHARE THIS EXACT NAME — take the right one
> `[corpus]` The pack ships a second `NS_Fire` at
> `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Fire`. **This is a true name collision, the same trap
> `NS_Lightning_Range.md` §1 documents** — a search by asset name returns both.
>
> **Fastest one-line discriminator `[corpus]`: the stylized sibling exposes a `User.*` parameter block
> — `User.Flames Color 01` (a `NiagaraDataInterfaceColorCurve`), `User.Glow Color 01` =
> RGBA(1, 0.0908417, 0.043735, 0.5), `User.Glow Color 02` = RGBA(1, 0.496933, 0.043735, 0.5),
> `User.Scale Overall`, `User.Sparkles Color 01` — and renders through `MI_VFX_Flames_01`,
> `MI_VFX_Glow_01`, `MI_VFX_Glow_04`. This target has an EMPTY user-parameter list and renders through
> `M_VFX_DisAdd_Part01`, `M_VFX_DisAdd_Part04`, `M_VFX_DisAdd_Flames01`.**
>
> Note that the stylized sibling's `User.Glow Color 01/02` values are **byte-identical** to this
> system's `Bomb_Glow_01`/`Bomb_Glow_02` initialize colours — it is a parameterized fork of the same
> effect, not a different one. That similarity is precisely why the name check is not enough on its
> own; check the user-parameter list.

> ### Emitter naming skew — do NOT "fix" it
> `[corpus]` Two emitters are named `Bomb_Glow_01` / `Bomb_Glow_02` in a system called `NS_Fire`, and
> the material they draw with is `M_VFX_DisAdd_Part01`, not a "glow" material. Leftover naming from a
> bomb effect; harmless, but do not rename in the recreation's layer table or the mapping back to the
> corpus stops working.

---

## 2. System anatomy `[corpus]`

**4 CPU emitters, all enabled, all WORLD space (`localSpace: false`), all bounds Dynamic,
`determinism: false`. 8–10 particles per burst** (the count is randomized — §2.3).

All four are sprite emitters; **there is no mesh renderer, no ribbon, and no light renderer in this
system.**

**System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
All four emitters are `Life Cycle Mode = System`, so per [P0-D1] this RULES and **every per-emitter
Loop row in the table below is inert** — including `Flames`' `Once / 0.3`. The whole system fires one
burst over a single 2.0 s cycle. *(Was read as a 1.0 s infinite loop with one divergent emitter.)*

| # | Emitter | Count | Spawn t | Loop behav. / dur. | Lifetime | Renderer | Material |
|---|---|---|---|---|---|---|---|
| 1 | `Bomb_Glow_01` | 1 | 0.05 | ~~Infinite / 1.0~~ *(inert)* | 0.25 | Sprite, Unaligned + FaceCamera | `M_VFX_DisAdd_Part01` |
| 2 | `Bomb_Glow_02` | 1 | 0.05 | ~~Infinite / 1.0~~ *(inert)* | 0.2 | Sprite, Unaligned + FaceCamera | `M_VFX_DisAdd_Part01` |
| 3 | `Flames` | 3 | 0.05 | ~~Once / 0.3~~ *(inert)* | rand **0.2–0.7** `[corpus-v3]` | Sprite, Unaligned + FaceCamera, **SubUV 2×2** | `M_VFX_DisAdd_Flames01` |
| 4 | `Sparkles` | **rand 3–5** | 0 | ~~Infinite / 1.0~~ *(inert)* | rand **0.3–1.0** `[corpus-v3]` | Sprite, **VelocityAligned** + FaceCamera | `M_VFX_DisAdd_Part04` |

Every `Spawn Burst Instantaneous` carries `UseLoopCountLimit = false`, so the stored
`Loop Count Limit = 1` is an **inert authored leftover** — same trap `NS_Lightning_Range.md` §4
records. `[corpus-v3]` Each emitter bursts **once**, over the system's single 2.0 s `Once` loop —
*not* "once per loop, forever".

### 2.1 Spawn shapes and forces `[corpus]`

| Emitter | Location | Velocity | Forces |
|---|---|---|---|
| `Bomb_Glow_01` | Simulation Position, offset (0,0,0), `UsePositionOffset = false` | — | — |
| `Bomb_Glow_02` | Simulation Position, offset (0,0,0), `UsePositionOffset = false` | — | — |
| `Flames` | **Sphere Location**, radius **20**, `Surface Only = true`, `Surface Expansion Mode = Outside`, `UseNonUniformScale = true` with scale (1,1,1), **`Hemisphere Z = true`**, `Radius Position 1`, `U Position 0`, `V Position 0.5`, `Uniform Distribution 1`, `Uniform Spiral Amount 1`, offset (0,0,0) | **Add Velocity from Point**, strength rand **100–250**, `Velocity Falloff Distance 100`, origin offset (0,0,0) | — |
| `Sparkles` | **Sphere Location**, radius **2** (essentially a point), `Surface Only = false`, `UseNonUniformScale = false`, **`Hemisphere Z = true`**, offset (0,0,0) | **Add Velocity from Point**, strength rand **300–1000**, `Velocity Falloff Distance 100` | — |

Both velocity emitters run `Solve Forces and Velocity` with `Clamp Velocity = false`,
`Limit Acceleration = false`, `Rotational Solver Is Enabled = true`, `Acceleration Limit 9999`,
`Speed Limit 1000` — the limits are authored but **not engaged**, so treat them as inert.

**No gravity, no drag, no curl noise, no collision anywhere in this system.** Position is a clean
closed-form integral of the velocity-scale curves (§5).

### 2.2 No events, no ribbons, no lights `[corpus]`

`stacks` is length 3 on every emitter (Emitter Update / Particle Spawn / Particle Update) and no
`Generate Location Event` module appears anywhere. `renderers` is length 1 on every emitter and every
entry is `NiagaraSpriteRendererProperties`. This system has none of the explosion family's
event/ribbon/light machinery.

### 2.3 ⚠ The burst count is RANDOM `[corpus]`

`Sparkles` overrides its spawn count: `[override] Spawn Count = dyn:Random Range Int` with
`Sparkles.RandomRangeInt.Minimum = 3`, `Maximum = 5`. So the system emits **8, 9 or 10 particles per
firing**, re-rolled each time the system runs.

Every other burst count in the batch is a literal. This one is not, and the cadence table's
`BurstCount` is an `int32` literal (§6.1).

### 2.4 Randomized lifetimes — RESOLVED `[corpus-v3]`

`Flames` and `Sparkles` both set `Lifetime Mode = Random` **and** carry
`[override] Lifetime = dyn:Random Range Float`. Per [P0-D2] the mode's static-switch selects the
driving pin: `Random` ⇒ **`Lifetime Min / Max` DRIVES**, and the override — which sits on the
unselected Direct-Set pin — is INERT (`lifetimeResolved.source = minmax` on both, override under
`inertOverrides`).

| Emitter | LIVE `InitializeParticle.Lifetime Min / Max` | inert `RandomRangeFloat` |
|---|---|---|
| `Flames` | **0.2 / 0.7** | ~~0.2 / 0.4~~ |
| `Sparkles` | **0.3 / 1.0** | ~~0.2 / 0.4~~ |

**This sheet previously took the OPPOSITE reading** — "the `[override]` wins, so both are rand
0.2–0.4 and the `Lifetime Min / Max` pins are dead", following the `NS_BasicAttack.md` §2 precedent.
That reading is WRONG under [P0-D2]. The consequence is real: the longest layer is **1.0 s, not
0.4 s**, which changes the cadence row (§6.1).

---

## 3. Mesh geometry

**N/A — no mesh renderers.** All four emitters draw sprites.

---

## 4. Material family and per-instance deltas `[corpus]`

**One family, three instances.** All three are instances of
`/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd` — the family the CkUsf
`DissolveAdd` look **already implements** (`/CkUsf/Looks/DissolveAdd.ush`, entry
`CkUsf_Look_DissolveAdd`; see `NS_BasicAttack.md` §9).

All three: `MD_Surface` / `BLEND_Translucent` / `MSM_Unlit`, `twoSided: false`, connected outputs
**`EmissiveColor` + `Opacity`** only, dynamic parameters **`[dissolve, distortion, offset,
core_color]`**. Expression histogram identical to the family (`ScalarParameter ×41`,
`DynamicParameter ×8`, `TextureSampleParameter2D ×6`, `Saturate ×12`, `Panner ×5`, `ParticleColor ×1`,
`DepthFade ×1`, `SmoothStep ×1`, `StaticSwitch ×1`).

Deltas stated **against `M_VFX_DisAdd_Slash01`**, the family reference `NS_BasicAttack.md` §4 already
documents (Brightness 5, `Dissolve_Speed` (0.3, −0.1), `Distortion_Intensity` 1, `Opacty_DepthFade` 20,
`GradientMap_Displacement` 0.1, `Opacty_StepAdd` 0.1, `Opacity_Boldness` 1, `Gradient_Invert` 0,
`Core_Intensity` 0, unit scales/speeds, `Color_Core` RGBA(1,1,1,0), `GradientMap_Tex` =
`T_VFX_WhitePixel`, `Dissolve_Tex`/`Distortion_Tex`/`GradientShape_Tex` = `T_VFX_Noise_02`). Anything
not listed is unchanged.

| Material | Main / Color tex | Dissolve tex | Distort tex | Brightness | Other deltas vs Slash01 |
|---|---|---|---|---|---|
| `Part01` (both glows) | `T_VFX_Part_01` | `T_VFX_Part_01` | *(Noise_02)* | **1** | `Dissolve_Speed` (0, 0); `Distortion_Intensity` **0**; `Gradient_Invert` **0.5**; `Opacity_Boldness` **0.5** |
| `Part04` (sparkles) | `T_VFX_Part_04` | `T_VFX_Part_04` | *(Noise_02)* | **6** | `Dissolve_Speed` (0, 0); `Distortion_Intensity` **0**; `Opacty_DepthFade` **30** |
| `Flames01` | `T_VFX_Wind_01` | `T_VFX_Noise_04` | `T_VFX_Noise_04` | **10** | `Dissolve` **−0.1**; `Dissolve_Scale` (**2, 2**); `Dissolve_Speed` (0, 0); `Distortion_Intensity` **0.5**; `Distortion_Scale` (**2, 2**); `Distortion_Speed` (**−0.3, −0.3**); `Core_Intensity` **1**; `Glow_Intensity` **2**; `Color_Core` **RGBA(0.015996, 0.014444, 0.014444, 1)** |

**`M_VFX_DisAdd_Part04` is ALREADY RECREATED** — `NS_BasicAttack.md` §9 ships it as the CkUsf look
`PartDisAdd04` (SparkStreak / SparkStreak / Brightness 6). This system can bind that look unchanged;
only `Part01` and `Flames01` are new parameterizations.

**Family parameters this system exercises that the CkUsf `DissolveAdd` look does NOT plumb today:**

- `Opacty_DepthFade` (20 / 30) — CkUsf surface looks do not wire scene depth. Known gap
  (`NS_BasicAttack.md` §13.3).
- `Core_Intensity` (1 on `Flames01`) and its `Color_Core` — a dark core tint. The look exposes
  `CoreColor` but drives it only through the `core_color` dynamic channel.
- `Glow_Intensity` (2 on `Flames01`) and `Gradient_Invert` (0.5 on `Part01`).

### 4.1 Texture dependency audit `[corpus]`

All greyscale masks (`TC_Alpha`, `sRGB = false`, `TEXTUREGROUP_World`), 512×512, so Particle Color
does all the tinting.

| Texture | Src fmt | Address | Used as | Procedural stand-in available today? |
|---|---|---|---|---|
| `T_VFX_Part_01` | TSF_G8 | Clamp/Clamp | `Part01` main + color + dissolve | **YES** — `T_CkParticles_SoftParticle`, measured off this exact texture (`NS_BasicAttack.md` §7: radially symmetric, fits `pow(1−r, 2.2)`) |
| `T_VFX_Part_04` | TSF_G16 | Wrap/Wrap | `Part04` main + color + dissolve | **YES** — `T_CkParticles_SparkStreak`, measured off this exact texture |
| `T_VFX_Noise_02` | TSF_G16 | Wrap/Wrap | family-default distortion + gradient shape | **YES** — `T_CkParticles_TileNoise` |
| `T_VFX_Wind_01` | TSF_G16 | Wrap/Wrap | `Flames01` main + color | **PARTIAL** — `T_CkParticles_WindBand` was measured off `T_VFX_Wind_03`, a *different* texture. Needs its own measurement. |
| `T_VFX_Noise_04` | TSF_G16 | Wrap/Wrap | `Flames01` dissolve + distortion | **needs measurement** — probably lands in `TileNoise`'s band, but that is an assumption, not a measurement |
| `T_VFX_WhitePixel` | TSF_RGBA16, sRGB | Wrap/Wrap | gradient map on all three | **no-op** — not copied, not needed |

**Only two new bakes are required** for the whole system. That is the lowest texture cost in the batch.

---

## 5. Per-emitter runtime curves — EXACT keyframes `[corpus]`

All curves sample **NormalizedAge** (0→1 over that emitter's own particle lifetime). `C` = constant
key, `L` = linear key. Verbatim, including the source's own float noise (`2.46502e-08` is an authored
zero — reproduce it as 0).

### 1. `Bomb_Glow_01` — sprite, 1 particle, spawn t 0.05, lifetime 0.25
- **Scale Color** (RGBA Together, Vector4 from Curve): R, G, B all (0, 1)L (1, 1)L | **A (0, 1)L (1, 0)L**
- **Scale Sprite Size** (Uniform Curve): (0, 0.5)C (**0.1**, 1)L (1, 1)L
- Initialize: `Color = RGBA(1, 0.0908417, 0.043735, **0.5**)` (Direct Set — there is **no** `Color from Curve` module on this emitter, so the initialize colour is what draws, scaled by the alpha ramp above), `Uniform Sprite Size` **500**, position offset (0,0,0)
- Dynamic params **[1, 0, 0, 0]** constant — `dissolve` pinned at 1
- `ScaleColor.Scale Alpha 1`, `ScaleColor.Scale RGB (1, 1, 1)`

### 2. `Bomb_Glow_02` — sprite, 1 particle, spawn t 0.05, lifetime 0.2
Structurally identical to `Bomb_Glow_01`; three values differ:
- Initialize `Color = **RGBA(1, 0.496933, 0.043735, 0.5)**` (a brighter, more orange core)
- `Uniform Sprite Size` **400**
- Dynamic params **[0, 0, 0, 0]** — `dissolve` pinned at **0**, not 1
- Same curves: Vector4 R, G, B (0, 1)L (1, 1)L | A (0, 1)L (1, 0)L; size (0, 0.5)C (0.1, 1)L (1, 1)L

### 3. `Flames` — sprite, SubUV 2×2, 3 particles, spawn t 0.05, lifetime rand **0.2–0.7** `[corpus-v3]` (§2.4)
- **Scale Velocity** (Vector from Curve): X, Y, Z all (0, 1)C (1, 0.2)C
- **Color** (Color from Curve, HDR, and the curves do NOT start at t = 0):
  R (**0.0796861**, **5**)L (**0.368246**, **3**)L (**0.738907**, 0.250158)L |
  G (0.0796861, **3.43343**)L (0.368246, 0.67227)L (0.738907, 0.00749903)L |
  B (0.0796861, 0.115767)L (0.368246, 0.0841786)L (0.738907, 0.00749903)L |
  A (0, 0)L (**0.303049**, 1)L (**0.992454**, 0)L
- **Dyn param 1 (`dissolve`)** (Float from Curve): (0, −2.46502e-08)C (1, −1)C
- **Dyn param 2 (`distortion`) = 5 constant**; param 3 = 0; param 4 = 0; **`Param3WriteEnabled = true`**
- **Scale Sprite Size** (Uniform Curve): (0, 0.5)C (0.2, 0.9)C (1, 1)C
- **Sprite Rotation Rate**: `Random Range Float 001`, **−30..30** deg/s
- Sizes: Random Uniform, `Uniform Sprite Size Min 50 / Max 200`. Initial sprite rotation Random,
  angle **0..360°** (`Sprite Rotation Angle` 90 is an inert leftover — the mode is Random)
- `Sub UVAnimation`: mode **Random**, start frame 0, end frame 3, `SubUV Loop Count 1`
- Initialize `Color = RGBA(1, 1, 1, 1)` (overridden by the curve); `Color.Scale Alpha 1`;
  `Color.Scale Color (1, 1, 1)`
- Inert leftovers exported on this emitter: `Sprite Size (200, 180)`, `Sprite Size Min (30, 70)`,
  `Sprite Size Max (50, 90)`, `Uniform Sprite Size 50` — the size mode is Random **Uniform**, so only
  the `Uniform Sprite Size Min/Max` pair drives

### 4. `Sparkles` — velocity-aligned sprite, rand 3–5 particles, spawn t 0, lifetime rand **0.3–1.0** `[corpus-v3]` (§2.4)
- **Scale Velocity**: X, Y, Z all (0, 1)C (0.2, 0.35)C (1, 0.05)C
- **Color**: R (0, 1)C (**0.327196**, 1)L (**0.779958**, 1)C | G (0, 0.703584)C (0.327196, 0.288367)L (0.779958, 0.0419999)C | B (0, 0.031)C (0.327196, 0.0369999)L (0.779958, 0.0642406)C | A (0, 1)C
  — a warm yellow-white that reddens over life; R never leaves 1
- **Scale Sprite Size** (Uniform Curve): (0, 0)C (0.1, 1)C (1, 0)C — pops in, fades out.
  Its non-uniform curve X (0, 0)L (1, 1)L | Y (0, 0)L (1, 1)L is authored but the mode is Uniform, so
  it is inert
- **Scale Sprite Size 001** (Non-Uniform Curve, ENABLED): X (1, 1)L | **Y (0.1, 1)C (1, 0.4)C** — the
  streak shortens to 40% over life. Its uniform curve (0, 0)L (1, 1)L is inert (mode is Non-Uniform)
- Sizes: Random Non-Uniform, min **(15, 40)** max **(30, 50)** — width × length, so a velocity-aligned
  quad narrow in u and long in v (the same orientation fact `NS_BasicAttack.md` §7 records as
  load-bearing)
- Dynamic params **[0, 0, 0, 0]** constant
- Initialize `Color = RGBA(1, 1, 1, 1)` (overridden by the curve); `Color.Scale Alpha 1`
- Inert leftovers: `Uniform Sprite Size Min 5 / Max 10`, `Sprite Size (200, 180)`,
  `Uniform Sprite Size 50` — the mode is Random **Non-Uniform**

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.0 Capability-gap callout — READ BEFORE SCHEDULING

**This system has ONE hard gap and one soft one.** It has no ribbon, no light renderer, no events, no
collision, no GPU sim, no user parameters and no meshes — the three heaviest gaps in the explosion
family are all absent here.

| # | Gap | Why the pipeline can't express it | Cheapest honest options |
|---|---|---|---|
| **G3** | **Sub-UV flipbook** — `Flames` renders `SubUV: 2x2` with a `Sub UVAnimation` module in mode **Random**, frames 0–3, loop count 1 | No `SubImageIndex` output on `FCkParticles_StageOutput`; the shared sprite renderers declare no `SubImageSize`; no CkUsf look samples a flipbook atlas | (a) Bake a **single** flame texture and drop the flipbook — visible as *less per-particle variety*, not as breakage, and with only 3 flame particles per loop the variety loss is small; (b) add `SubImageIndex` to the DI contract (GPU + CPU mirror), a `SubImageSize` field on the renderer spec, and atlas UV maths in the look. Reusable — `NS_Dash`'s `Wind_Smokes` needs the same thing |
| **G3b** | **Randomized per-loop burst count** — `Sparkles` spawns `Random Range Int` 3–5 (§2.3) | `FCk_ParticlesTemplateSpec::BurstCount` is an `int32` literal; the template emits a fixed burst | **Solvable in the behavior, not the table**: declare the burst at the maximum (5 sparkle slots) and hide the surplus per loop. Burst `UniqueID`s are sequential, so `LoopIndex = Seed / BurstCount` is derivable inside the behavior and `CkParticles_Rand(LoopIndex, salt)` gives a stable per-loop roll. A slot whose index exceeds the roll writes zero colour/size. This is **work, not a gap** — but it is a technique nothing in the roster uses yet, so budget for it |

Two further items that are **work, not gaps**:

- ~~**Per-emitter cadence divergence (mild).**~~ **GONE `[corpus-v3]`** — all four emitters are
  `Life Cycle Mode = System`, so `Flames`' `Once / 0.3` and the others' `Infinite / 1.0` are all
  inert; the system's `Once / 2.0 s` governs uniformly. There is no per-emitter cadence divergence in
  this system.
- **Position integration.** Both moving emitters drive velocity through a piecewise-linear
  `Vector from Curve` scale with no drag and no acceleration, so position integrates **exactly** in
  closed form — the `NS_BasicAttack.md` §8 spark pattern applies unchanged, and GPU/CPU lockstep is
  straightforward here.

**Loop authority — RESOLVED `[corpus-v3]`.** Every emitter has `Life Cycle Mode = System`, so per
[P0-D1] the system's `Loop Once / 2.0 s` drives them and the emitter-local `Infinite / 1.0` and
`Once / 0.3` rows are inert. *(Was `[unresolved]`; the working figure was 1.0 s.)*

### 6.1 Cadence row

**A new row is required** — no existing row matches (`_Single` is 1.0 / 1.1 / 1; `_Slash` is
1.0 / 0.5 / 19; `_Burst` is 1.2 / 1.2 / 96).

```
{ TEXT("PS_CkParticles_Template_Fire"), 2.0f, 1.0f, 10, Get_FireRendererSpecs() }
```

Per [P0-D3]: loop = the system loop duration, lifetime = max resolved lifetime, burst = §2 counts.

- `LoopDuration` **2.0** `[corpus-v3]` — the system's `Once` loop duration. *Was 1.0, taken from the
  three Infinite emitters' inert Loop rows.*
- `ParticleLifetime` **1.0** `[corpus-v3]` — `Sparkles`' resolved `Lifetime Max`. *Was `[unresolved]`
  with 0.4 s as this sheet's working reading; §2.4's override-wins assumption is corrected, so the
  longest layer is 2.5× what the sheet assumed.* Every shorter layer must write zero colour, zero
  size and zero scale past its own lifetime, or dead layers hang in the air (`NS_BasicAttack.md` §8).
- `BurstCount` **10** — the MAXIMUM per-loop count (1 + 1 + 3 + 5). Layer index = `Seed % 10`
  (double-modulo so a negative Seed still lands in range): 0 `Bomb_Glow_01`, 1 `Bomb_Glow_02`,
  2–4 `Flames`, 5–9 `Sparkles`. Slots 5–9 are gated by the per-loop roll of §6.0 G3b, so 0–2 of them
  hide each loop.
- **Spawn-time offsets are layer state, not cadence.** `Bomb_Glow_01`, `Bomb_Glow_02` and `Flames` all
  spawn at t = 0.05: those layers hide (colour/size 0) for `age < 0.05` and run their curves on
  `(age − 0.05) / lifetime` — the `NS_BasicAttack.md` §5 spark treatment.

### 6.2 Renderer / VisTag needs

Three distinct materials across four emitters. The shared VisTag set almost covers it, but **VisTag 0
and VisTag 1 each bind exactly one material through `User.SpriteMaterial`**, and this system needs two
different camera-facing materials — so row-declared renderers are required
(`NS_BasicAttack.md` §8.1; "one user parameter cannot carry several materials").

| Row renderer | Kind | Look | Source emitters |
|---|---|---|---|
| 1 | camera-facing sprite | `PartDisAdd01` (new) | `Bomb_Glow_01`, `Bomb_Glow_02` — same material, ONE renderer serves both |
| 2 | camera-facing sprite | `FlamesDisAdd01` (new) | `Flames` |
| 3 | **`VelocityAlignedSprite`** | **`PartDisAdd04`** — **already exists** (`NS_BasicAttack.md` §9) | `Sparkles` |

Renderer 3 is a `Get_SlashRendererSpecs()`-style entry that needs **no new look at all**.

**One new renderer kind is required**: `ECk_ParticlesRenderer_Kind` today has only `Mesh` and
`VelocityAlignedSprite`. A **`CameraFacingSprite`** kind with an explicitly bound look is needed for
renderers 1 and 2 — small, mechanical, and reusable by every other sheet in this batch.

VisTags: three, allocated above `Get_RosterVisTag_Max()` (9 as of 2026-08-01, since behavior 7 owns
5–9). Read the ceiling from `Get_RosterVisTag_Max()`; never restate a literal
(`NS_BasicAttack.md` §14.4).

### 6.3 Mesh needs

**None.** No mesh renderers in this system.

### 6.4 Look needs

Two new `DissolveAdd` parameterizations; the third look already exists:

| Look | Status | ShapeTex | DissolveTex | DistortTex | Brightness | Notable |
|---|---|---|---|---|---|---|
| `PartDisAdd01` | **new** | SoftParticle | SoftParticle | TileNoise | **1** | `DistortIntensity` 0, `OpacityBoldness` **0.5**, `Gradient_Invert` 0.5 (unplumbed) |
| `FlamesDisAdd01` | **new** | (new Wind01 bake) | (new Noise04 bake) | (same) | **10** | `DissolveBias` **−0.1**, `DissolveScale` (2, 2), `DistortIntensity` **0.5**, `DistortScale` (2, 2), `DistortSpeed` (−0.3, −0.3), `Core_Intensity` 1 + `Color_Core` (0.015996, 0.014444, 0.014444) and `Glow_Intensity` 2 — the last three are **unplumbed** today |
| `PartDisAdd04` | **EXISTS** | SparkStreak | SparkStreak | — | 6 | ships with behavior 7 |

The family entry point needs two new parameters to serve `FlamesDisAdd01` faithfully:
`CoreIntensity` (with its `Color_Core` tint) and `GlowIntensity`. Both are inert on every existing
look (their instances resolve `Core_Intensity 0` / `Glow_Intensity 1`), so the extension follows the
`NS_BasicAttack.md` §9 precedent: extend the shared shader, then prove every existing look regenerates
unchanged.

### 6.5 Texture needs

**Only two new bakes** (§4.1): a `T_VFX_Wind_01` stand-in and a `T_VFX_Noise_04` stand-in, both
measurement-driven in `CkParticles_TextureGenerator.cpp` the way `NS_BasicAttack.md` §7 did — derive
numbers (profile shape, streak count, falloff exponent, anisotropy) from the corpus PNGs, never copy
pixels. `T_VFX_Part_01`, `T_VFX_Part_04` and `T_VFX_Noise_02` are already served by
`SoftParticle`, `SparkStreak` and `TileNoise`.

### 6.6 Behavior id

**Do NOT allocate an id in this document.** At implementation time take the next free id from
`ck::particles::NumBehaviors` (18 as of 2026-08-01, ids 0..17) and bump it.

### 6.7 Known deviations already implied

- **World space → local space.** All four emitters are world-space; CkParticles templates are
  local-space because self-driving behaviors write absolute positions. Same deviation
  `NS_BasicAttack.md` §13.2 records. Visible only if the spawning actor moves during the effect —
  which, for a fire attached to something, it might.
- **`Opacty_DepthFade`** (20 on the glows and flames, 30 on the sparkles) is dropped — CkUsf surface
  looks do not wire scene depth.
- **HDR colour** on `Flames` (R up to 5, G up to 3.43343) must survive the behavior's colour path
  unclamped. `RosterSanity` deliberately does not assert an alpha upper bound; confirm it does not
  assert a colour one either before writing these values.
- **The two glows have no colour curve at all** — their RGB comes straight from
  `InitializeParticle.Color` under a Vector4 `Scale Color` whose RGB channels are flat 1. Do not
  invent a curve for them; write the constant and the alpha ramp.

---

## 7+. Reserved for implementation.
