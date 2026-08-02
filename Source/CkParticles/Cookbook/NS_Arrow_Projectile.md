# Recipe: NS_Arrow_Projectile → CkParticles behavior 19 (ArrowProjectile) — faithful re-port

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: IMPLEMENTED, NOT YET GATED (2026-08-02).** Behavior id **19** allocated; shader + CPU mirror,
the gym pair and the behavior test are authored. The cadence row and the new `PartDisAdd01` look are
shared with **behavior 18** ([NS_Gunshot_Projectile.md](NS_Gunshot_Projectile.md)) and were authored
there. Nothing has been built, regenerated or rendered in this session — the orchestrator owns every
lane (campaign tripwire 2), and the human A/B (§12) is still open.

| Piece | State |
|---|---|
| `Behavior_ArrowProjectile.ush` + CPU mirror (case 19) | authored; per-layer constants straight off §5, verified against the corpus by a Python mirror (§12) |
| Cadence row `PS_CkParticles_Template_ProjectileTrio` (10 / 10 / 3) | **shared with behavior 18**; reconciled numbers identical (§6.1) |
| Renderer split | VisTag **0** (shared camera sprite, look via `User.SpriteMaterial`) for `Glow_01`; row VisTag **11** (`PartDisAdd04`) for both tails — §6.2 option (a), as recommended |
| CkUsf look `PartDisAdd01` | authored with the Gunshot port; `PartDisAdd04` reused verbatim |
| `Test_Particles_ArrowProjectileBehavior` | authored |
| Gym pair row + station tags | authored |
| Build, look regen, template regen, test lanes | **NOT RUN** — orchestrator gates |
| Visual A/B (§12 `[EDITOR-VERIFY]`) | **OPEN** |

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
`Spawn Burst Instantaneous` **Spawn Count 1 at Spawn Time 0** on every emitter, `UseLoopCountLimit = false`
(the stored `Loop Count Limit = 1` is therefore inert — same authored leftover the Lightning recipe
records).

> ### `[corpus-v3]` CADENCE CORRECTION — the SYSTEM governs, and it fires ONCE
> Every emitter carries `Life Cycle Mode = **System**`, making its own `Emitter State` rows (Loop
> Behavior Infinite, Loop Duration Mode Fixed, **Loop Duration 1.0 s**) **INERT** — the trap campaign
> decision [P0-D1] exists for. The v3 exporter's `systemState` block reads: **Loop Behavior `Once`,
> Loop Duration `10`**, `Inactive Response = Complete (let emitters finish then kill the system)`.
>
> Reconciled cadence: **loop 10.0 s / lifetime 10.0 s / burst 3**, byte-identical to
> `NS_Gunshot_Projectile`, so the shared row survives. The count claim below is withdrawn with it —
> the system fires ONE burst of three particles that live exactly as long as it does, so **3 particles
> are alive, not 30**.

**3 particles per system firing. All three lifetimes are 10 s** (`lifetimeResolved.source = "direct"`,
`Lifetime = 10`; the Projectile emitters' `Lifetime Min/Max 0.2/0.4` are exported as `inertValues`,
confirming this sheet's original reading under [P0-D2]).

| # | Emitter | Count | Spawn t | Lifetime | Renderer | Alignment / Facing | Material | Position offset | Size |
|---|---|---|---|---|---|---|---|---|---|
| 0 | `Glow_01` | 1 | 0 | **10** | Sprite | `Unaligned` / `FaceCamera` | `M_VFX_DisAdd_Part01` | `(-5, 0, 0)` | Uniform **120** |
| 1 | `Projectile_Bright` | 1 | 0 | **10** | Sprite | **`VelocityAligned`** / `FaceCamera` | `M_VFX_DisAdd_Part04` | `(-20.1693, 0, 0)` | Non-Uniform **(20, 50)** |
| 2 | `Projectile_Dark` | 1 | 0 | **10** | Sprite | **`VelocityAligned`** / `FaceCamera` | `M_VFX_DisAdd_Part04` | `(-66.0904, 0, 0)` | Non-Uniform **(35, 150)** |

Sort mode `None` on all three. `Position Mode = Simulation Position` everywhere.

> **`[corpus-v3]` OFFSET CORRECTION.** This sheet recorded `UsePositionOffset = false` on `Glow_01`,
> making its `(-5, 0, 0)` inert. The v3 sidecar reads `UsePositionOffset = "true"` on **all three**
> emitters, so the head sits 5 units back along −X like everything else. Small enough to be invisible
> against a 120-unit sprite, but it is ground truth and the recreation writes it.

**Lifetime 10 s against a Loop-Once 10 s system → exactly one generation, 3 live particles**
`[corpus-v3]`. No emitter has any age-driven curve (§5), so the visible result is three static sprites
that appear together and vanish together when the system completes.

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

### 6.1 Cadence row `[corpus-v3]`

**`PS_CkParticles_Template_ProjectileTrio`, loop 10.0 s, particle lifetime 10.0 s, burst 3** — the
reconciled numbers, derived mechanically from [P0-D3]: loop = the `systemState` Loop Duration, lifetime =
the maximum `lifetimeResolved`, burst = the §2 spawn counts. No existing row is close: `_Single` is
1.0/1.1/1, `_Slash` is 1.0/0.5/19, `_Burst` is 1.2/1.2/96. Layer index = `Seed % 3` (burst UniqueIDs are
sequential, so the burst draws exactly one particle per source emitter).

The pre-reconciliation sheet asked for `1.0 / 10.0 / 3` and noted that a `10.0 / 10.0 / 3` row "would
render identically but gap for one frame at each loop restart, which the source never does". Under the
corrected reading the source **does** stop: it is `Loop Once` and completes. So `10 / 10 / 3` is the
exact row, not the cheap one, and the one-frame boundary is now a §13 known difference rather than a
defect.

`NS_Gunshot_Projectile` reconciles to the identical numbers, so **one row serves both** — named for the
cadence shape rather than for either effect.

### 6.2 Renderers / VisTag

`Glow_01` is `Unaligned` / `FaceCamera` — a plain camera-facing billboard, which the *shared* VisTag 0
renderer already is. But VisTag 0 binds its material through `User.SpriteMaterial`, and one user
parameter cannot also carry the Part04 look for layers 1–2. The sheet offered two options; the port took
the preferred one:

- **(a) TAKEN** — `PartDisAdd01` is bound to VisTag 0 via `Get_BehaviorLookName(19)` and only the
  velocity-aligned Part04 renderer is row-declared (and it is shared with behavior 18, so this port
  declared no renderer of its own at all). Correct facing for every layer.
- (b) rejected — row-declaring both would have cost `Glow_01` its camera facing, since
  `ECk_ParticlesRenderer_Kind` has only `Mesh` and `VelocityAlignedSprite`. Building the camera-facing
  kind (capability C1) is Phase 1's, not this port's.

| Layer | Renderer | VisTag | Look | Bound via |
|---|---|---|---|---|
| 0 `Glow_01` | shared camera sprite | **0** | `PartDisAdd01` | `User.SpriteMaterial` |
| 1–2 Projectiles | row `VelocityAlignedSprite` | **11** | `PartDisAdd04` | explicit on the renderer |

Consequence for callers: the gym pair's `TextureName` MUST stay `NAME_None`. An explicit texture writes
the same `User.SpriteMaterial` and wins over the look binding, which would leave the glow head wearing a
procedural material instance instead of `PartDisAdd01`.

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

**Allocated at implementation time: 19** (`NumBehaviors` 18 → 20 in the same pass that added 18).

### 6.6 Stage outputs the behavior would write

| Output | Value |
|---|---|
| `Position` | layer 0 `(-5, 0, 0)` `[corpus-v3]`; layer 1 `(-20.1693, 0, 0)`; layer 2 `(-66.0904, 0, 0)` |
| `Velocity` | `(0.01, 0, 0)` on layers 1–2, zero on layer 0 — direction is load-bearing, magnitude is not |
| `Color` | the constant RGBA of §5; no curve |
| `Size` | `(120,120)` / `(20,50)` / `(35,150)` |
| `Dynamic` | `[1,0,0,0]` on layer 0, `[0,0,0,0]` on layers 1–2 |
| `Scale`, `Orientation`, `MeshIndex`, `Rotation`, `SpriteAlignment`, `SpriteFacing` | left at default — not a mesh or custom-facing behavior |

### 6.7 CAPABILITY GAPS — what the current pipeline cannot express

Conservative list. Each is a real cost at implementation time.

1. **No camera-facing sprite kind on the row-renderer spec.** `ECk_ParticlesRenderer_Kind` has `Mesh`
   and `VelocityAlignedSprite` only. A row that needs *two* looks where one is camera-facing must
   either route one look through `User.SpriteMaterial` (option (a)) or add a third kind. **Not a gap
   for this effect** — it has exactly one camera-facing layer, so option (a) covers it with no pipeline
   change. It becomes a real gap the moment a system needs TWO distinct camera-facing looks, which is
   capability C1 and most of the pack.
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
6. ~~**Lifetime 10 s costs 30 live particles for 3 visible ones.**~~ **Withdrawn `[corpus-v3]`** — the
   Loop-Once system yields ONE generation, so the cost is 3 particles for 3 visible ones. The template's
   fixed bounds (`±3000`) comfortably contain the static stack.

Nothing in this effect needs ribbons, GPU sim, collision, events, sub-UV flipbooks, light renderers, or
user parameters. **This is the least demanding system in the Arrow/Gunshot batch.**

---

## 7. Procedural textures — measured, never copied

**Nothing new was baked** — identical to [NS_Gunshot_Projectile.md](NS_Gunshot_Projectile.md) §7:
`T_VFX_Part_01` → `T_CkParticles_SoftParticle`, `T_VFX_Part_04` → `T_CkParticles_SparkStreak`, both
measured off the corpus PNGs during the NS_BasicAttack port (that recipe's §7 records the measurements
and the residuals). `T_VFX_Noise_02` and `T_VFX_WhitePixel` are unreachable on these two instances.

---

## 8. CkParticles translation

| | |
|---|---|
| Behavior ID | **19** |
| Template | **`PS_CkParticles_Template_ProjectileTrio`** (loop 10 s, lifetime 10 s, burst 3), shared with behavior 18 |
| Layer partition | `Seed % 3` — burst UniqueIDs are sequential, so the single burst draws exactly one particle per source emitter. Double-modulo, so a negative Seed still lands in range |
| Seeds / salts | **none** — no per-particle randomness exists in this system |
| `VisTag` | **0** (layer 0, shared camera sprite) and **11** (layers 1–2, row-declared) |

Stage outputs written:

| Output | Layer 0 `Glow_01` | Layer 1 `Projectile_Bright` | Layer 2 `Projectile_Dark` |
|---|---|---|---|
| `Position` | `(-5, 0, 0)` `[corpus-v3]` | `(-20.1693, 0, 0)` | `(-66.0904, 0, 0)` |
| `Color` | `(1, 0.775394, 0.257, 0.3)` | `(1, 0.558341, 0.102242, 0.5)` | `(0.06, 0.0470123, 0.0270472, 0.2)` |
| `Size` | `(120, 120)` (Uniform) | `(20, 50)` | `(35, 150)` |
| `Dynamic` | `[1, 0, 0, 0]` | `[0, 0, 0, 0]` | `[0, 0, 0, 0]` |
| `Velocity` | **zero** — no `Add Velocity` module | `(0.01, 0, 0)` | `(0.01, 0, 0)` |
| `VisTag` | 0 | 11 | 11 |
| `Scale`, `Orientation`, `MeshIndex`, `Rotation`, `SpriteAlignment`, `SpriteFacing` | default | default | default |

**The zero velocity on layer 0 is load-bearing, not an omission.** The Gunshot sibling gives its head a
`(0.01, 0, 0)` drift because that head is velocity-aligned and needs an axis. This one is camera-facing
and the source has no `Add Velocity` module on it at all; inventing a drift here would be inventing a
value, and the behavior test asserts the head has none.

**Age decides exactly one thing: death.** No curve exists in this system, so the behavior is a pure
function of the layer index. Past the source's own 10 s the behavior writes zero colour, size and scale —
the mirror of `Particle State`'s *Kill Particles When Lifetime Has Elapsed*. The cadence row's lifetime is
the same 10 s today, so that branch is unreachable in practice; it exists so a future retune of the
shared row cannot leave an arrow trail hanging in the air.

**This is the LEAST demanding system in the batch and the port shows it:** no new capability, no new
renderer kind, no new bake, no new mesh, and — because behavior 18 landed first — no new cadence row and
no new look either. The entire port is one `.ush`, its mirror, a routing case, a look-binding case, a
gym row and a test.

---

## 9. CkUsf translation

Both looks are parameterizations of the existing `DissolveAdd` family and were authored with the Gunshot
port — see [NS_Gunshot_Projectile.md](NS_Gunshot_Projectile.md) §9 for `PartDisAdd01`'s values, the
`OpacityBoldness` plumbing, and the confirmation that the family shader multiplies it in. **No shader
math was written or changed for either port.**

The one thing specific to this recipe: `PartDisAdd01` is reached here through `User.SpriteMaterial`
rather than through a row renderer, so the SAME generated master serves a camera sprite here and a
velocity-aligned sprite in behavior 18. `_UsedWithNiagaraSprites` covers both — the usage flag is about
the renderer class, not the alignment.

---

## 10. Copied asset destinations

**None.** Same as the Gunshot port: procedural textures, recreated materials, no `/Game/Vefects`
reference of any kind. The gym addresses the ORIGINAL by path string only.

---

## 11. Runtime binding path

```
UCk_Utils_Particles_UE::Spawn_BehaviorAtLocation(ctx, 19, Location, Rotation, Scale)
  └─ ck::particles::Get_BehaviorTemplateSystemObjectPath(19) → PS_CkParticles_Template_ProjectileTrio
  └─ SpawnSystemAtLocation → SetIntParameter("User.BehaviorId", 19)
  └─ ck::particles::Get_BehaviorLookName(19) → "PartDisAdd01"
       └─ LoadObject(M_CkUsf_Look_PartDisAdd01) → SetVariableMaterial(User.SpriteMaterial)
            └─ reaches the SHARED camera sprite (VisTag 0) — layer 0 only
  └─ layers 1–2 draw on the row's VisTag 11 renderer, whose PartDisAdd04 master is bound at build time
```

**`TextureName` must stay `NAME_None` at every call site.** An explicit texture writes the same
`User.SpriteMaterial` *after* the look and wins, replacing `PartDisAdd01` on the glow head. The gym pair
row says so in a comment for the same reason.

---

## 12. Exact verification procedure

### Automated

| Test | Lane | Asserts |
|---|---|---|
| `CkTests.UnitTests.CkParticles.ArrowProjectileBehavior` | any (no Niagara, no RHI, no fork) | behavior 19 routes to the shared row and binds `PartDisAdd01`; the 3-slot partition splits across the shared camera sprite (0) and the row streak renderer (11) and is stable across bursts; **`Glow_01` draws on VisTag 0** — the assertion that catches it silently losing camera facing; every §5 colour, size, offset and dynamic channel on all three layers; the head is square and both tails are stretched; **every layer is CONSTANT across its whole life and across seeds**; the head has NO velocity and both tails drift at `(0.01, 0, 0)`; anti-vacuity per layer; alive at the last instant of 10 s and dead past it |
| `CkTests.UnitTests.CkParticles.GunshotProjectileBehavior` | any | owns the shared cadence row's numbers (10 / 10 / 3) and asserts that 19 routes to the same row |
| `CkTests.UnitTests.CkParticles.RosterSanity` | any | picks 19 up from `NumBehaviors`: template routing, renderer-band discipline, finite/renderable outputs across the sweep |
| `CkTests.UnitTests.CkUsf.GeneratesUsableMasters` | `--no-nullrhi` | `PartDisAdd01` compiles |
| `Ck_AutoTest_Particles_SpawnAllBehaviors` | `--no-nullrhi` | id 19 spawns a live component through the shared template |
| `CkAutoTest_VfxExamples_PairStationsSpawn` | `--no-nullrhi` | the ARROW PROJECTILE pair spawns both sides |

**Source-transcription self-check (ran 2026-08-02, scratchpad only).** A Python script parsed the corpus
sidecar, the `.ush` and the CPU mirror independently: **max error 0.0** on every layer's Position /
Color / Size / Dynamic / Velocity and GPU↔CPU lockstep 0.0 on every field. Transcription only — it
cannot see a shader that fails to compile.

Regeneration order: build → `Ck_Usf_GenerateLooks` → **Create Template System** → gates, and
`grep -ac ExecuteStage` on `PS_CkParticles_Template_ProjectileTrio.uasset` must be non-zero.

### `[EDITOR-VERIFY]` — visual fidelity gate (human, ~10 min)

Neither Niagara graph is opened at any point. The original asset stays read-only.

1. Regenerate looks then templates, in that order.
2. PIE the **VfxExamples** gym, go to the **ARROW PROJECTILE** pair, `Ck_GymVfxExamples_RestartAll`.
3. Orbit the camera — criterion (b) is only visible while moving.

   | # | Criterion | Expected |
   |---|---|---|
   | a | Layer count | one round head + two streaks behind it, nothing else |
   | b | **Head facing** | the head stays round and fully visible from EVERY camera angle; the two tails go edge-on-thin when viewed along their axis |
   | c | Stack order along −X | head at the origin, bright tail ~20 units back, dark tail ~66 back |
   | d | Tail length | the dark tail is ~3× the bright one, and both are far shorter than the Gunshot pair's |
   | e | Palette | warm gold head, orange bright tail, dim brown dark tail — cooler and softer than the Gunshot pair |
   | f | Head erosion | the head is visibly eaten by a STATIC noise pattern that does not pan or crawl |
   | g | Head coverage | a soft, half-strength glow rather than a solid disc (`OpacityBoldness` 0.5) |
   | h | Stability over time | nothing moves, brightens, fades or flickers across the whole 10 s |
   | i | Restart | both sides vanish and re-appear together on the 10 s boundary |
   | j | Side by side with GUNSHOT | the arrow reads visibly shorter and cooler; if the two pairs look alike, a per-layer constant is wrong |
   | k | Under rotation / scale | rotate the actor 45°, set scale 2.0 — both respond the same way |

4. Record every mismatch in §13 with the timestamp and criterion letter.

---

## 13. Confirmed fidelity differences or intentional deviations

**The visual gate in §12 has NOT been executed and no lane has been run.**

### Known differences — deliberate

1. **Procedural textures instead of the hand paints** — as NS_Gunshot_Projectile §13.1.
2. **`Opacty_DepthFade` (20 / 30) is not wired.** No scene-depth input on CkUsf surface looks; visible
   only where a sprite intersects geometry, which a pedestal does not.
3. **`Opacty_Step`, `Opacty_StepAdd`, `GradientMap_Displacement`, `Glow_Intensity`, `Core_Power`,
   `Core_Intensity`, `Gradient_Invert`, `CamOffset` are not family parameters.** All resolve to values
   expected to be inert on these two instances `[inferred]`; the omission is real and sibling effects in
   this batch DO drive several of them.
4. **`DistortTex` is `SoftParticle`.** The family helper binds it to the dissolve asset;
   `Distortion_Intensity` is 0 on both instances so the branch is dead.
5. **The recreation loops; the source completes.** `Loop Once` + *Complete* on the source versus a
   forever-looping 10 s template; the harness re-arms the original on `OnSystemFinished` so the pedestals
   stay in phase, but a one-frame gap at the Ck side's loop boundary is possible. This is the trade the
   pre-reconciliation sheet flagged, now understood correctly.
6. **`Glow_01` rides the shared VisTag 0 renderer rather than a row renderer.** Correct facing, but it
   means the head's material travels through `User.SpriteMaterial` — so any caller that passes a
   `TextureName` silently replaces it. Recorded in §11; the gym row comments it.
7. **World space.** Both sides are local-space, so no C12 gap on the pedestal; a moving spawner would
   drag the trail rigidly on both sides, and the gym cannot exercise the difference.

### Unverified

- Every visual criterion in §12. Nothing has been rendered.
- Whether the camera-facing head reads at the same size as the original's once the shared sprite
  renderer's own defaults are applied — the sprite size is asserted, the renderer's handling of it is not.
- Whether `OpacityBoldness` 0.5 composites as intended against the two additive tails.

---

## 14. Reusable lessons for future effects

1. **Two systems from one pack can be one port and a half.** Landing the sibling first (behavior 18)
   turned this port into a `.ush` + mirror + two switch cases + a test: the cadence row, the row
   renderer and the new look were all already there. **Order a batch so the richer system goes first.**
2. **A single camera-facing layer does not need capability C1.** Bind its look through
   `Get_BehaviorLookName` → `User.SpriteMaterial` → shared VisTag 0 and row-declare the rest. The
   capability is only forced when a system needs TWO distinct camera-facing looks.
3. **That route has a caller-visible cost, so record it.** `User.SpriteMaterial` is a single slot and an
   explicit `TextureName` wins over the look binding. Any behavior using this route needs the
   `NAME_None` note at every call site — a silent material swap is exactly the kind of miss an A/B blames
   on the shader.
4. **Re-read every boolean the sheet called inert.** `UsePositionOffset` was recorded false on `Glow_01`
   and is true in the v3 export. It happens to be a 5-unit difference here; the same class of misread on
   a 200-unit offset would be a visible defect blamed on something else.
5. **Sibling systems make the best A/B for each other.** Criterion (j) — "the arrow must read shorter and
   cooler than the gunshot" — catches a swapped constant that neither pair's own comparison would.

