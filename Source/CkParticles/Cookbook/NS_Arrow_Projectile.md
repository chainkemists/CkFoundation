# Recipe: NS_Arrow_Projectile → CkParticles (PLANNING SHEET)

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior id is allocated, no `.ush` exists, no look exists, no cadence row exists, no mesh or texture
has been baked, and nothing has ever been rendered. Every section below is archaeology read out of the
extracted corpus plus a translation *plan*. Sections 7+ are reserved for the implementation session.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Arrow_Projectile` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Role in the pack | the in-flight body of the arrow ability (paired with `NS_Arrow_Cast` and `NS_Arrow_Hit`) |

Corpus evidence (`Saved/` is machine-local; regenerate per [README.md](README.md)):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Arrow_Projectile.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Part01,Part04}.json`
- `materials/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd` (parent, via `parentChain`)
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Part_01,Part_04,Noise_02,WhitePixel}.json`

**The source Niagara asset was never opened.** Every fact tagged `[corpus]` is read from the files above.

> ### Two systems share this name — take the right one
> `[corpus]` The pack ships a second `NS_Arrow_Projectile` at
> `Vefects/Anime_Stylized_VFX/VFX/Particles/`. It has the same emitter count (3) but is the
> **parameterized** variant.
>
> **Fastest discriminator: the user-parameter list.** This recipe's system has `userParameters: []`
> — *empty*. The Stylized sibling exposes `User.Color 01`, `User.Color 02`, `User.Color 03`,
> `User.Scale Overall`. Second discriminator: the Stylized sibling renders through `MI_VFX_Glow_01` /
> `MI_VFX_Glow_04`, this one through `M_VFX_DisAdd_Part01` / `M_VFX_DisAdd_Part04`.
>
> This recipe documents the **`Anime_VFX/Shared/Skills`** variant only.

---

## 2. System anatomy `[corpus]`

**3 CPU emitters, LOCAL space, `Determinism: false`, `Bounds: Dynamic`, no user parameters.**
Every emitter: `Emitter State` Loop Behavior **Infinite**, Loop Duration Mode **Fixed**,
**Loop Duration 1.0 s**; `Spawn Burst Instantaneous` **Spawn Count 1 at Spawn Time 0**,
`UseLoopCountLimit = false` (the stored `Loop Count Limit = 1` is therefore inert — same authored
leftover the Lightning recipe records).

**3 particles per loop. All three lifetimes are 10 s.**

| # | Emitter | Count | Spawn t | Lifetime | Renderer | Alignment / Facing | Material | Position offset | Size |
|---|---|---|---|---|---|---|---|---|---|
| 0 | `Glow_01` | 1 | 0 | **10** | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | `(-5, 0, 0)` | Uniform **120** |
| 1 | `Projectile_Bright` | 1 | 0 | **10** | Sprite | **`VelocityAligned`** / `FaceCamera` | `M_VFX_DisAdd_Part04` | `(-20.1693, 0, 0)` | Non-Uniform **(20, 50)** |
| 2 | `Projectile_Dark` | 1 | 0 | **10** | Sprite | **`VelocityAligned`** / `FaceCamera` | `M_VFX_DisAdd_Part04` | `(-66.0904, 0, 0)` | Non-Uniform **(35, 150)** |

Sort mode `None` on all three. `Position Mode = Simulation Position` everywhere.
`UsePositionOffset` is **false on `Glow_01`** and **true on the two Projectile emitters** — so
`Glow_01`'s stored `(-5, 0, 0)` offset is **inert** and it sits at the origin; only the two
Projectile offsets apply.

**Lifetime 10 s against a 1.0 s loop is the defining cadence fact.** Each emitter bursts one particle
per second and each lives ten seconds, so at steady state **10 generations per emitter are alive
simultaneously — 30 live particles**. Because no emitter has any age-driven curve (§5), all ten
generations of a layer are numerically identical and coincident: the *visible* result is exactly three
static sprites. The recreation must still reproduce the count if it wants the same additive
over-brightening from the stacked copies.

### Modules present, and which of them are inert

| Emitter | Particle Spawn | Particle Update |
|---|---|---|
| `Glow_01` | `Initialize Particle` | `Particle State`, `Dynamic Material Parameters` |
| `Projectile_Bright` | `Initialize Particle`, `Add Velocity` | `Solve Forces and Velocity`, `Particle State`, `Dynamic Material Parameters`, `Scale Sprite Size` **(DISABLED)**, `Scale Sprite Size 001` **(DISABLED)** |
| `Projectile_Dark` | same as Bright | same as Bright |

Inert authored values, recorded so a reader does not implement them `[corpus]`:

- `Lifetime Mode = Direct Set` on all three → `Lifetime Min/Max = 0.2 / 0.4` on the Projectile
  emitters **never apply**; the effective lifetime is the Direct Set `10`.
- `Sprite Rotation Mode = Unset` on all three → `Sprite Rotation Angle = 90`,
  `Sprite Rotation Angle Min/Max = 0 / 360` **never apply**.
- `Sprite Size Mode = Uniform` on `Glow_01` → its `Sprite Size` pair is unused; `Sprite Size Mode =
  Non-Uniform` on the Projectiles → their `Uniform Sprite Size = 50` and `Sprite Size Min/Max`
  `(35,80)`/`(50,90)` are unused.
- Both `Scale Sprite Size` modules on both Projectile emitters are **DISABLED**, so the curves they
  carry — uniform `(0,0)C (0.1,1)C (1,0)C` and non-uniform `Y: (0,1)C (1,0.6)C` — do not run. They are
  transcribed in §5 only to record that they were checked and rejected.
- `Solve Forces and Velocity`: `Clamp Velocity = false`, `Limit Acceleration = false`, so
  `Acceleration Limit = 9999` / `Speed Limit = 1000` do not bind.

---

## 3. Mesh geometry

**N/A — sprite renderers only.** No `NiagaraMeshRendererProperties` in this system `[corpus]`.

---

## 4. Material family and per-instance deltas `[corpus]`

Both materials are instances of the **same parent**, `Parents/M_VFX_DissolveAdd` — the family the
CkUsf `DissolveAdd.ush` look already implements (`RingDissolveAdd`, `SlashDisAdd0*`, `PartDisAdd04`).

Family base properties, identical on both instances: `MD_Surface`, **`BLEND_Translucent`**,
**`MSM_Unlit`**, `twoSided: false`, connected outputs **`EmissiveColor` + `Opacity`** only,
dynamic-parameter channel names **`dissolve`, `distortion`, `offset`, `core_color`**.

Reference row is `M_VFX_DisAdd_Part01` (it resolves the parent's defaults on every scalar except the
ones listed). Full reference values, quoted so this table survives the corpus:

`Brightness 1`, `Opacity_Boldness 0.5`, `Glow_Intensity 1`, `Core_Power 1`, `Core_Intensity 0`,
`Gradient_Invert 0.5`, `GradientMap_Displacement 0.1`, `Opacty_Step 0`, `Opacty_StepAdd 0.1`,
`Opacty_DepthFade 20`, `CamOffset 0`, `Dissolve 0`, `Dissolve_Invert 0`, `Dissolve_Scale_X/Y 1`,
`Dissolve_Speed_X/Y 0`, `Dissolve_Offset_X/Y 0`, `Distortion_Intensity 0`, `Distortion_Scale_X/Y 1`,
`Distortion_Speed_X/Y 0`, `MainTex_Scale_X/Y 1`, `MainTex_Speed_X/Y 0`, `MainTex_Offset_X/Y 0`,
`Color_Scale_X/Y 1`, `Color_Speed_X/Y 0`, `Color_Offset_X/Y 0`, `GradientShape_Scale_X/Y 1`,
`GradientShape_Speed_X/Y 0`, `Color_Core = RGBA(1, 1, 1, 0)`.
Textures: `Main_Tex`/`Color_Tex`/`Dissolve_Tex = T_VFX_Part_01`, `Distortion_Tex`/`GradientShape_Tex =
T_VFX_Noise_02`, `GradientMap_Tex = T_VFX_WhitePixel`.

| Material | Used by | Deltas vs the `Part01` reference |
|---|---|---|
| `M_VFX_DisAdd_Part01` (ref) | `Glow_01` | — |
| `M_VFX_DisAdd_Part04` | `Projectile_Bright`, `Projectile_Dark` | `Brightness` **6**; `Opacity_Boldness` **1**; `Gradient_Invert` **0**; `Opacty_DepthFade` **30**; `Main_Tex`/`Color_Tex`/`Dissolve_Tex` → **`T_VFX_Part_04`** |

Expression histogram of the family `[corpus, M_VFX_DisAdd_Part01.json]`: `ScalarParameter ×41`,
`Add ×18`, `AppendVector ×18`, `Multiply ×18`, `Saturate ×12`, `DynamicParameter ×8`, `Reroute ×8`,
`TextureSampleParameter2D ×6`, `Panner ×5`, `TextureCoordinate ×5`, `Constant ×5`,
`LinearInterpolate ×4`, `Clamp ×2`, `OneMinus ×2`, `DepthFade ×1`, `SmoothStep ×1`, `Power ×1`,
`ParticleColor ×1`, `WorldPosition ×1`, `VectorParameter ×1`, `StaticSwitch ×1`,
`StaticBoolParameter ×1`, `MaterialFunctionCall ×1`.

### Textures referenced `[corpus]`

| Texture | Size / format | Address | Verdict |
|---|---|---|---|
| `T_VFX_Part_01` | 512×512 `TSF_G8` `TC_Alpha`, sRGB false | `TA_Clamp` / `TA_Clamp` | **required** — the round soft glow. The generator already ships a measured stand-in: `T_CkParticles_SoftParticle` (`pow(saturate(1-r), 2.2)`, NS_BasicAttack §7) |
| `T_VFX_Part_04` | 512×512 `TSF_G16` `TC_Alpha`, sRGB false | `TA_Wrap` / `TA_Wrap` | **required** — the streak. Stand-in exists: `T_CkParticles_SparkStreak` (NS_BasicAttack §7) |
| `T_VFX_Noise_02` | 512×512 `TSF_G16` `TC_Alpha` | `TA_Wrap` | not needed — `Distortion_Intensity = 0` on both instances, so the distortion branch is dead; `GradientShape` is scale 1 / speed 0 into a white-pixel gradient map |
| `T_VFX_WhitePixel` | 1×1 `TSF_RGBA16` `TC_Default`, **sRGB true** | `TA_Wrap` | not needed — a white pixel makes the gradient-map chain a no-op |

**No new procedural bake is required for this effect.** Both required textures already have measured
stand-ins in `CkParticles_TextureGenerator.cpp`.

---

## 5. Per-layer runtime values `[corpus]`

**There is not one age-driven curve in this system.** No `Color` module, no `Scale Color`, no enabled
`Scale Sprite Size`, no `Scale Velocity`. Every layer is a constant for its whole 10 s life. That is
the single most useful fact in this sheet: the behavior is a pure function of the layer index.

| Layer | `InitializeParticle.Color` (RGBA) | Sprite size | Dynamic params `[dissolve, distortion, offset, core_color]` | Velocity |
|---|---|---|---|---|
| 0 `Glow_01` | `(1, 0.775394, 0.257, 0.3)` | Uniform `120` → `(120, 120)` | **`[1, 0, 0, 0]`** | none (no `Add Velocity` module) |
| 1 `Projectile_Bright` | `(1, 0.558341, 0.102242, 0.5)` | `(20, 50)` | `[0, 0, 0, 0]` | `Add Velocity = (0.01, 0, 0)`, `Scale Added Velocity = (1,1,1)` |
| 2 `Projectile_Dark` | `(0.06, 0.0470123, 0.0270472, 0.2)` | `(35, 150)` | `[0, 0, 0, 0]` | `Add Velocity = (0.01, 0, 0)`, `Scale Added Velocity = (1,1,1)` |

`Color Mode = Direct Set` and `Color Channel Mode = Link RGB / Link A` on all three, so the RGBA above
IS the particle colour for the whole life. No `Color.Scale Alpha` / `Scale Color` values are authored
on any of the three emitters.

**`Add Velocity = (0.01, 0, 0)` is 0.01 units/second — visually zero, but NOT zero.** It exists solely
to give `VelocityAligned` a defined axis. With it, both streak sprites align along local **+X** and stay
put; drop it and Niagara's velocity alignment degenerates. **Reproduce the direction, not the
magnitude.**

**`dissolve = 1` on `Glow_01` (and 0 elsewhere).** On the DissolveAdd family a *positive* dissolve
threshold erodes; the Lightning and Slash instances drive it negative to stay intact. `Glow_01` is
therefore the one layer this system actively eats away, with `Dissolve_Speed_X/Y = 0` on `Part01` so
the noise does not pan — a static erosion pattern, not an animated one `[corpus; visual consequence
inferred]`.

### Disabled-module curves, transcribed for the record only

Both `Scale Sprite Size` modules on `Projectile_Bright` and `Projectile_Dark` are **DISABLED** and do
not run:

- `Scale Sprite Size` (Uniform Curve mode): `None: (0, 0)C (0.1, 1)C (1, 0)C`;
  non-uniform companion `X: (0, 0)L (1, 1)L | Y: (0, 0)L (1, 1)L`
- `Scale Sprite Size 001` (Non-Uniform Curve mode): uniform companion `None: (0, 0)L (1, 1)L`;
  `X: (1, 1)L | Y: (0, 1)C (1, 0.6)C`

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.1 Cadence row

**New row required: `PS_CkParticles_Template_ArrowProjectile`, loop 1.0 s, particle lifetime 10.0 s,
burst 3.** No existing row is close: `_Single` is 1.0/1.1/1, `_Slash` is 1.0/0.5/19, `_Burst` is
1.2/1.2/96. Layer index = `Seed % 3` (burst UniqueIDs are sequential, so each loop draws exactly one
particle per source emitter).

At steady state that row holds **30** live particles, matching the source's ten overlapping
generations. A cheaper `10.0 / 10.0 / 3` row would render identically *because no layer has a
time-varying curve* — but it would gap for one frame at each loop restart, which the source never does.
**Recommend the exact row**; record the equivalence so a future perf pass knows the trade is available.

This row is a candidate for sharing with `NS_Gunshot_Projectile`, whose cadence is **identical**
(1.0 / 10.0 / 3) — see that recipe's §6. If both are implemented, one row named for the shape of the
cadence rather than for one effect (e.g. `PS_CkParticles_Template_ProjectileTrio`) serves both.

### 6.2 Renderers / VisTag

Two different looks on two different renderer kinds, so `Get_BehaviorLookName` stays `NAME_None` and
the row declares its own renderers (the NS_BasicAttack mechanism, `FCk_ParticlesRendererSpec`):

| Row renderer | Kind | Look | Draws |
|---|---|---|---|
| A | `VelocityAlignedSprite` | new `PartDisAdd01` | layer 0 (`Glow_01`) |
| B | `VelocityAlignedSprite` | existing `PartDisAdd04` | layers 1–2 |

**Renderer A is a compromise and must be recorded as one.** `Glow_01` is `Unaligned` / `FaceCamera` —
a plain camera-facing billboard — which the *shared* VisTag 0 renderer already is. But VisTag 0 binds
its material through `User.SpriteMaterial`, and one user parameter cannot also carry the Part04 look
for layers 1–2. Two clean options at implementation time, decide then and record which:

- **(a)** bind `PartDisAdd01` to VisTag 0 via `Get_BehaviorLookName` and row-declare only the
  velocity-aligned Part04 renderer. Correct facing for every layer, one row renderer. *Preferred.*
- **(b)** row-declare both. Costs `Glow_01` its camera facing unless the row spec gains a
  camera-facing sprite kind (`ECk_ParticlesRenderer_Kind` currently has only `Mesh` and
  `VelocityAlignedSprite`).

VisTag ids allocate above `SharedRendererVisTag_Max` and after whatever the Slash row already took —
**do not restate a literal**; read `ck::particles::Get_RosterVisTag_Max()`.

### 6.3 CkUsf looks

| Look | State | Parameters (against the family's current 15-param signature) |
|---|---|---|
| `PartDisAdd04` | **already exists** (NS_BasicAttack): `ShapeTex`/`DissolveTex` = `SparkStreak`, `Brightness` 6 | reusable as-is |
| `PartDisAdd01` | **NEW** | `ShapeTex`/`DissolveTex`/`DistortTex` = `SoftParticle` (and `TileNoise` for distort, which is dead); `Brightness` **1**; `DissolveSpeed`/`DissolveSpeedY` 0; `DissolveBias` 0; `DissolveScale` (1,1); `DistortIntensity` 0; `DistortSpeed` (0,0); `MainTexScale` (1,1); `CoreColor` (1,1,1); `OpacityBoldness` **0.5** |

`OpacityBoldness 0.5` is the first instance in the cookbook that is not 1.0 — the existing five looks
all resolve 1. Worth an explicit check that the family shader actually multiplies it in rather than
treating it as a no-op.

### 6.4 Mesh / texture needs

**None new.** No meshes. Both textures map onto existing procedural bakes (`SoftParticle`,
`SparkStreak`).

### 6.5 Behavior id

**Do not allocate now.** At the time of writing `ck::particles::NumBehaviors = 18` (ids 0–17 in use),
so the next free id is whatever `NumBehaviors` reads at implementation time. Bump `NumBehaviors`, never
restate a maximum.

### 6.6 Stage outputs the behavior would write

| Output | Value |
|---|---|
| `Position` | layer 0 `(0,0,0)` (its `UsePositionOffset` is false); layer 1 `(-20.1693, 0, 0)`; layer 2 `(-66.0904, 0, 0)` |
| `Velocity` | `(0.01, 0, 0)` on layers 1–2, zero on layer 0 — direction is load-bearing, magnitude is not |
| `Color` | the constant RGBA of §5; no curve |
| `Size` | `(120,120)` / `(20,50)` / `(35,150)` |
| `Dynamic` | `[1,0,0,0]` on layer 0, `[0,0,0,0]` on layers 1–2 |
| `Scale`, `Orientation`, `MeshIndex`, `Rotation`, `SpriteAlignment`, `SpriteFacing` | left at default — not a mesh or custom-facing behavior |

### 6.7 CAPABILITY GAPS — what the current pipeline cannot express

Conservative list. Each is a real cost at implementation time.

1. **No camera-facing sprite kind on the row-renderer spec.** `ECk_ParticlesRenderer_Kind` has `Mesh`
   and `VelocityAlignedSprite` only. A row that needs *two* looks where one is camera-facing must
   either route one look through `User.SpriteMaterial` (option (a) above) or add a third kind. **Small,
   additive** — but it is a pipeline change, not a data edit.
2. **`Opacty_DepthFade` (20 / 30) is not wired.** CkUsf surface looks have no scene-depth input. Same
   known gap the Lightning and Slash recipes record. Visible only where a sprite intersects geometry.
3. **`Opacty_StepAdd` (0.1) and `GradientMap_Displacement` (0.1) are not wired.** Both resolve to the
   family defaults here and the gradient map is a white pixel, so the omission is expected to be
   inert `[inferred]`.
4. **`Glow_Intensity` (1 on both instances) is not a family parameter.** Inert at 1; named because
   sibling recipes in this batch DO drive it (`M_VFX_DisAdd_Part02`, `Glow_Intensity 0.3`).
5. **World space.** The source's three emitters are `LocalSpace: true`, which matches the CkParticles
   template — **no gap here**, unlike the Slash recipe. Recorded because it is the first effect in the
   cookbook where the spaces agree.
6. **Lifetime 10 s costs 30 live particles for 3 visible ones.** Not a capability gap, a budget one.
   The template's fixed bounds (`±3000`) comfortably contain a static 3-sprite stack.

Nothing in this effect needs ribbons, GPU sim, collision, events, sub-UV flipbooks, light renderers, or
user parameters. **This is the least demanding system in the Arrow/Gunshot batch.**

---

## 7+. Reserved for implementation
