# Recipe: NS_Gunshot_Projectile → CkParticles (PLANNING SHEET)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior id is allocated, no `.ush` exists, no look exists, no cadence row exists, no mesh or texture
has been baked, and nothing has ever been rendered. Sections 7+ are reserved for the implementation
session.

**Read [NS_Arrow_Projectile.md](NS_Arrow_Projectile.md) alongside this.** The two systems are the same
three-emitter construction with different numbers; the sections below call out every place they differ
so the pair can share one cadence row and one look set.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Gunshot_Projectile` |
| Pack | Vefects — *Anime VFX* |
| Role in the pack | the in-flight bullet body (paired with `NS_Gunshot_Cast` and `NS_Gunshot_Hit`) |

Corpus evidence:

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Gunshot_Projectile.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part04}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd` (parent)
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_04,Noise_02,WhitePixel}.json`

**The source Niagara asset was never opened.**

> ### Two systems share this name — take the right one
> `[corpus]` A second `NS_Gunshot_Projectile` lives at `Vefects/Anime_Stylized_VFX/VFX/Particles/`,
> also with 3 emitters.
>
> **Fastest discriminator: the user-parameter list.** This system's is **empty**; the Stylized
> sibling's is `User.Glow Color 01`, `User.Glow Color 02`, `User.Glow Color 03`, `User.Scale Overall`.
> Second discriminator: the sibling renders through `MI_VFX_Glow_01` / `MI_VFX_Glow_04`, this one
> through `M_VFX_DisAdd_Part01` / `M_VFX_DisAdd_Part04`.

---

## 2. System anatomy `[corpus]`

**3 CPU emitters, LOCAL space, `Determinism: false`, `Bounds: Dynamic`, no user parameters.**
Every emitter: Loop Behavior **Infinite**, Loop Duration Mode **Fixed**, **Loop Duration 1.0 s**;
`Spawn Burst Instantaneous` **Spawn Count 1 at Spawn Time 0**, `UseLoopCountLimit = false` (stored
`Loop Count Limit = 1` inert).

**3 particles per loop. All three lifetimes are 10 s.**

| # | Emitter | Count | Spawn t | Lifetime | Renderer | Alignment / Facing | Material | Position offset | Size |
|---|---|---|---|---|---|---|---|---|---|
| 0 | `Glow_01` | 1 | 0 | **10** | Sprite | **`VelocityAligned`** / `FaceCamera` | `M_VFX_DisAdd_Part01` | `(-52.048, 0, 0)` | Non-Uniform **(80, 300)** |
| 1 | `Projectile_Bright` | 1 | 0 | **10** | Sprite | **`VelocityAligned`** / `FaceCamera` | `M_VFX_DisAdd_Part04` | `(-77.6262, 0, 0)` | Non-Uniform **(20, 200)** |
| 2 | `Projectile_Dark` | 1 | 0 | **10** | Sprite | **`VelocityAligned`** / `FaceCamera` | `M_VFX_DisAdd_Part04` | `(-194.751, 0, 0)` | Non-Uniform **(35, 500)** |

`Sort: None`, `Position Mode = Simulation Position`, `UsePositionOffset = true` on **all three**
(unlike the Arrow variant, where `Glow_01`'s offset is inert).

**Differences from `NS_Arrow_Projectile` `[corpus]`:**

| | Arrow | Gunshot |
|---|---|---|
| `Glow_01` alignment | `Unaligned` (billboard) | **`VelocityAligned`** |
| `Glow_01` size mode | Uniform 120 | **Non-Uniform (80, 300)** |
| `Glow_01` `UsePositionOffset` | **false** (offset inert) | **true** |
| `Glow_01` `Add Velocity` module | absent | **present**, `(0.01, 0, 0)` |
| Offsets | −5 / −20.1693 / −66.0904 | **−52.048 / −77.6262 / −194.751** |
| Sizes | 120 / (20,50) / (35,150) | **(80,300) / (20,200) / (35,500)** |

The Gunshot trail is **much longer and entirely streaked** — every layer is velocity-aligned and the
dark tail reaches 500 units along the axis, versus the Arrow's 150. That, plus the −194.75 offset, is
the whole visual difference between an arrow and a bullet in this pack.

### Modules present, and which are inert

| Emitter | Particle Spawn | Particle Update |
|---|---|---|
| `Glow_01` | `Initialize Particle`, `Add Velocity` | `Solve Forces and Velocity`, `Particle State`, `Dynamic Material Parameters` |
| `Projectile_Bright` | `Initialize Particle`, `Add Velocity` | `Solve Forces and Velocity`, `Particle State`, `Dynamic Material Parameters`, `Scale Sprite Size` **(DISABLED)**, `Scale Sprite Size 001` **(DISABLED)** |
| `Projectile_Dark` | same as Bright | same as Bright |

Inert authored values `[corpus]`:

- `Lifetime Mode = Direct Set` everywhere → `Lifetime Min/Max = 0.2 / 0.4` on the Projectile emitters
  never apply. Effective lifetime is **10**.
- `Sprite Rotation Mode = Unset` everywhere → `Sprite Rotation Angle 90` / `Min 0` / `Max 360` never
  apply.
- `Sprite Size Mode = Non-Uniform` everywhere → `Uniform Sprite Size` (120 on `Glow_01`, 50 on the
  Projectiles) and `Sprite Size Min/Max` `(35,80)`/`(50,90)` never apply.
- Both `Scale Sprite Size` modules on both Projectile emitters are **DISABLED** (curves in §5).
- `Clamp Velocity = false`, `Limit Acceleration = false` → `Speed Limit 1000` / `Acceleration Limit
  9999` do not bind.

**Lifetime 10 s on a 1.0 s loop → 10 overlapping generations per emitter, 30 live particles.** As in
the Arrow variant, no layer has an age-driven curve, so the generations are numerically identical and
the visible result is three static streaks.

---

## 3. Mesh geometry

**N/A — sprite renderers only** `[corpus]`.

---

## 4. Material family and per-instance deltas `[corpus]`

**Identical to `NS_Arrow_Projectile` §4** — the same two instances of the same
`Parents/M_VFX_DissolveAdd` parent. Repeated here so this sheet stands alone.

Family base: `MD_Surface`, **`BLEND_Translucent`**, **`MSM_Unlit`**, `twoSided: false`, outputs
**`EmissiveColor` + `Opacity`**, dynamic channels **`dissolve`, `distortion`, `offset`, `core_color`**.

Reference row `M_VFX_DisAdd_Part01` (parent defaults except where noted below): `Brightness 1`,
`Opacity_Boldness 0.5`, `Glow_Intensity 1`, `Core_Power 1`, `Core_Intensity 0`, `Gradient_Invert 0.5`,
`GradientMap_Displacement 0.1`, `Opacty_Step 0`, `Opacty_StepAdd 0.1`, `Opacty_DepthFade 20`,
`CamOffset 0`, `Dissolve 0`, `Dissolve_Invert 0`, `Dissolve_Scale_X/Y 1`, `Dissolve_Speed_X/Y 0`,
`Distortion_Intensity 0`, `Distortion_Scale_X/Y 1`, `Distortion_Speed_X/Y 0`, `MainTex_Scale_X/Y 1`,
`MainTex_Speed_X/Y 0`, `Color_Scale_X/Y 1`, `Color_Speed_X/Y 0`, `GradientShape_Scale_X/Y 1`,
`GradientShape_Speed_X/Y 0`, `Color_Core = RGBA(1,1,1,0)`; `Main_Tex`/`Color_Tex`/`Dissolve_Tex =
T_VFX_Part_01`, `Distortion_Tex`/`GradientShape_Tex = T_VFX_Noise_02`, `GradientMap_Tex =
T_VFX_WhitePixel`.

| Material | Used by | Deltas vs `Part01` |
|---|---|---|
| `M_VFX_DisAdd_Part01` (ref) | `Glow_01` | — |
| `M_VFX_DisAdd_Part04` | `Projectile_Bright`, `Projectile_Dark` | `Brightness` **6**; `Opacity_Boldness` **1**; `Gradient_Invert` **0**; `Opacty_DepthFade` **30**; `Main_Tex`/`Color_Tex`/`Dissolve_Tex` → **`T_VFX_Part_04`** |

### Textures `[corpus]`

| Texture | Size / format | Address | Verdict |
|---|---|---|---|
| `T_VFX_Part_01` | 512×512 `TSF_G8` `TC_Alpha`, sRGB false | `TA_Clamp` | **required**; existing stand-in `T_CkParticles_SoftParticle` |
| `T_VFX_Part_04` | 512×512 `TSF_G16` `TC_Alpha`, sRGB false | `TA_Wrap` | **required**; existing stand-in `T_CkParticles_SparkStreak` |
| `T_VFX_Noise_02` | 512×512 `TSF_G16` `TC_Alpha` | `TA_Wrap` | not needed — `Distortion_Intensity = 0` |
| `T_VFX_WhitePixel` | 1×1 `TSF_RGBA16`, sRGB true | `TA_Wrap` | not needed — no-op gradient map |

**No new procedural bake is required.**

> **Watch the streak texture's orientation.** `T_VFX_Part_04`'s measured profile (NS_BasicAttack §7)
> is a vertical stripe of half-width 0.156 running along **v**, anisotropy |dV|/|dU| = 0.31 — narrow
> in u, long in v. That matches `(width, length)` sprite sizes like `(35, 500)` exactly. The existing
> `SparkStreak` bake reproduces it and must not be "corrected".

---

## 5. Per-layer runtime values `[corpus]`

**No age-driven curve exists in this system.** No `Color`, `Scale Color`, enabled `Scale Sprite Size`,
or `Scale Velocity` module anywhere. Every layer is constant for its 10 s life.

| Layer | `InitializeParticle.Color` (RGBA) | Sprite size | Dynamic params `[dissolve, distortion, offset, core_color]` | Velocity |
|---|---|---|---|---|
| 0 `Glow_01` | `(1, 0.194618, 0.021219, 0.5)` | `(80, 300)` | **`[1, 0, 0, 0]`** | `Add Velocity = (0.01, 0, 0)` |
| 1 `Projectile_Bright` | `(1, 0.552046, 0.147, 1)` | `(20, 200)` | `[0, 0, 0, 0]` | `Add Velocity = (0.01, 0, 0)` |
| 2 `Projectile_Dark` | `(0.1, 0.0171441, 0.00865005, 0.2)` | `(35, 500)` | `[0, 0, 0, 0]` | `Add Velocity = (0.01, 0, 0)` |

`Color Mode = Direct Set`, `Color Channel Mode = Link RGB / Link A` on all three. `Scale Added
Velocity = (1,1,1)` on all three. No `Color.Scale Alpha` / `Scale Color` authored.

Colour comparison against the Arrow variant `[corpus]` — the Gunshot palette is **redder and hotter**:

| Layer | Arrow | Gunshot |
|---|---|---|
| 0 | `(1, 0.775394, 0.257, 0.3)` | `(1, 0.194618, 0.021219, 0.5)` |
| 1 | `(1, 0.558341, 0.102242, 0.5)` | `(1, 0.552046, 0.147, 1)` |
| 2 | `(0.06, 0.0470123, 0.0270472, 0.2)` | `(0.1, 0.0171441, 0.00865005, 0.2)` |

**`Add Velocity = (0.01, 0, 0)` is 0.01 units/second** — visually zero but non-zero, present only to
give `VelocityAligned` a defined axis. All three layers therefore streak along local **+X**, which is
also the pipeline's existing aim-axis convention for `MuzzleFlash` / `Tracer` / `Beam`. Reproduce the
direction; the magnitude is irrelevant.

**`dissolve = 1` on `Glow_01`** (0 on the other two): the only layer this system erodes.
`Dissolve_Speed_X/Y = 0` on `Part01`, so the erosion pattern is static rather than panning.

### Disabled-module curves, transcribed for the record only

`Scale Sprite Size` (Uniform Curve): `None: (0, 0)C (0.1, 1)C (1, 0)C`; companion
`X: (0, 0)L (1, 1)L | Y: (0, 0)L (1, 1)L`.
`Scale Sprite Size 001` (Non-Uniform Curve): companion `None: (0, 0)L (1, 1)L`;
`X: (1, 1)L | Y: (0, 1)C (1, 0.6)C`.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**loop 1.0 s / particle lifetime 10.0 s / burst 3** — **identical to `NS_Arrow_Projectile`**. No
existing row matches (`_Single` 1.0/1.1/1, `_Slash` 1.0/0.5/19, `_Burst` 1.2/1.2/96).

**Share one row across both projectile recreations.** They differ only in per-layer constants, which
live in the behaviors, not the row. Name the row for the cadence shape rather than one effect —
e.g. `PS_CkParticles_Template_ProjectileTrio` — and let both behaviors route to it. That keeps the
cadence table honest (a row is a cadence, not an effect) and avoids two byte-identical templates.

Layer index = `Seed % 3`.

### 6.2 Renderers / VisTag

**Simpler than the Arrow variant: every layer is `VelocityAligned`.** Two looks, one renderer kind:

| Row renderer | Kind | Look | Draws |
|---|---|---|---|
| A | `VelocityAlignedSprite` | new `PartDisAdd01` | layer 0 |
| B | `VelocityAlignedSprite` | existing `PartDisAdd04` | layers 1–2 |

Both kinds already exist in `ECk_ParticlesRenderer_Kind`, so **this effect needs no new renderer
capability at all** — unlike the Arrow variant, whose `Glow_01` is camera-facing.

`Get_BehaviorLookName` stays `NAME_None`; the row renderers bind their look masters explicitly.
VisTags allocate above `SharedRendererVisTag_Max`, read through `Get_RosterVisTag_Max()` — never a
literal.

If the shared cadence row of §6.1 is adopted, the row carries **all four** renderers (two per
projectile effect) and each behavior writes only its own two VisTags. A particle whose VisTag no
renderer claims simply does not draw, so the cross-talk is benign — but the roster test's
"every VisTag a behavior writes is inside the renderer set" assertion still passes only if both
behaviors' tags are declared on the shared row.

### 6.3 CkUsf looks

| Look | State | Values |
|---|---|---|
| `PartDisAdd04` | **already exists** (NS_BasicAttack) — `SparkStreak`, `Brightness` 6 | reusable as-is |
| `PartDisAdd01` | **NEW** — shared with `NS_Arrow_Projectile` | `ShapeTex`/`DissolveTex` = `SoftParticle`; `Brightness` **1**; `OpacityBoldness` **0.5**; every other family parameter at its inert default (`DissolveSpeed` 0, `DissolveBias` 0, `DissolveScale` (1,1), `DistortIntensity` 0, `DistortSpeed` (0,0), `MainTexScale` (1,1), `CoreColor` (1,1,1)) |

### 6.4 Mesh / texture needs

**None new.** No meshes; both textures already have measured procedural stand-ins.

### 6.5 Behavior id

**Do not allocate now.** `ck::particles::NumBehaviors` was 18 at the time of writing; the next free id
is whatever it reads at implementation time. Bump it, never restate a maximum.

### 6.6 Stage outputs the behavior would write

| Output | Value |
|---|---|
| `Position` | `(-52.048, 0, 0)` / `(-77.6262, 0, 0)` / `(-194.751, 0, 0)` — all three offsets apply |
| `Velocity` | `(0.01, 0, 0)` on every layer; direction load-bearing, magnitude not |
| `Color` | the constant RGBA of §5 |
| `Size` | `(80,300)` / `(20,200)` / `(35,500)` |
| `Dynamic` | `[1,0,0,0]` on layer 0, `[0,0,0,0]` on layers 1–2 |
| `Scale`, `Orientation`, `MeshIndex`, `Rotation`, `SpriteAlignment`, `SpriteFacing` | default |

### 6.7 CAPABILITY GAPS — what the current pipeline cannot express

1. **`Opacty_DepthFade` (20 on `Part01`, 30 on `Part04`) is not wired.** CkUsf surface looks have no
   scene-depth input; same gap the Lightning and Slash recipes record.
2. **`Opacty_StepAdd` (0.1), `GradientMap_Displacement` (0.1), `Glow_Intensity` (1), `Core_Power` (1),
   `Core_Intensity` (0), `Gradient_Invert` (0.5 / 0) are not family parameters.** All resolve to
   values that are expected to be inert on these two instances `[inferred]`, but the omission is real
   and sibling effects in this batch DO drive several of them.
3. **`OpacityBoldness = 0.5`** is the first non-1.0 value the cookbook has met. Confirm the family
   shader actually multiplies it rather than treating it as a no-op before trusting the port.

**No gap in cadence, space, renderer kind, mesh, or texture.** Local space matches; both renderer
kinds exist; nothing needs ribbons, GPU sim, collision, events, sub-UV flipbooks, light renderers, or
user parameters.

---

## 7+. Reserved for implementation
