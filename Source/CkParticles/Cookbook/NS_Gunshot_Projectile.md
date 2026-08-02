# Recipe: NS_Gunshot_Projectile → CkParticles behavior 18 (GunshotProjectile) — faithful re-port

Schema and evidence-tag conventions: [README.md](README.md).

## Completion state — READ FIRST

**Status: IMPLEMENTED, NOT YET GATED (2026-08-02).** Behavior id **18** allocated; shader + CPU mirror,
the shared cadence row, the new `PartDisAdd01` look, the gym pair and the behavior test are all authored.
Nothing has been built, regenerated or rendered in this session — the orchestrator owns every lane
(campaign tripwire 2), and the human A/B (§12) is still open.

| Piece | State |
|---|---|
| `Behavior_GunshotProjectile.ush` + CPU mirror (case 18) | authored; per-layer constants straight off §5, verified against the corpus by a Python mirror (§12) |
| Cadence row `PS_CkParticles_Template_ProjectileTrio` (10 / 10 / 3) | authored in `Get_TemplateSpecs()`, **shared with behavior 19** |
| Row renderers VisTag 10 (`PartDisAdd01`) + 11 (`PartDisAdd04`) | authored on that row |
| CkUsf look `PartDisAdd01` | authored (new); `PartDisAdd04` reused verbatim |
| `Test_Particles_GunshotProjectileBehavior` + roster size literal | authored |
| Gym pair row + station tags | authored |
| Build, look regen, template regen, test lanes | **NOT RUN** — orchestrator gates |
| Visual A/B (§12 `[EDITOR-VERIFY]`) | **OPEN** |

**Read [NS_Arrow_Projectile.md](NS_Arrow_Projectile.md) alongside this.** The two systems are the same
three-emitter construction with different numbers; the sections below call out every place they differ,
and the pair does share one cadence row and one look set.

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
`Spawn Burst Instantaneous` **Spawn Count 1 at Spawn Time 0** on every emitter, `UseLoopCountLimit = false`
(stored `Loop Count Limit = 1` inert).

> ### `[corpus-v3]` CADENCE CORRECTION — the SYSTEM governs, and it fires ONCE
> Every emitter carries `Life Cycle Mode = **System**`, which makes its own `Emitter State` rows
> (Loop Behavior Infinite, Loop Duration Mode Fixed, Loop Duration 1.0 s) **INERT** — exactly the trap
> campaign decision [P0-D1] exists for. The v3 exporter's `systemState` block reads:
> **Loop Behavior `Once`, Loop Duration `10`**, `Inactive Response = Complete (let emitters finish then
> kill the system)`.
>
> The reconciled cadence is therefore **loop 10.0 s / lifetime 10.0 s / burst 3** — not the 1.0 s loop
> this sheet recorded on 2026-08-01. The consequence is not cosmetic: the system fires **one** burst of
> three particles that live exactly as long as the system does, so **3 particles are alive, not 30**, and
> the "10 overlapping generations" reading below is withdrawn. `NS_Arrow_Projectile` resolves to the
> identical numbers, so the shared row survives the correction.

**3 particles per system firing. All three lifetimes are 10 s** (`lifetimeResolved.source = "direct"`,
`Lifetime = 10`; the Projectile emitters' `Lifetime Min/Max 0.2/0.4` are exported as `inertValues`,
confirming this sheet's original reading under [P0-D2]).

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

**Lifetime 10 s on a Loop-Once 10 s system → exactly one generation, 3 live particles** `[corpus-v3]`.
No layer has an age-driven curve, so the visible result is three static streaks that appear together and
vanish together when the system completes.

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

### 6.1 Cadence row `[corpus-v3]`

**loop 10.0 s / particle lifetime 10.0 s / burst 3** — **identical to `NS_Arrow_Projectile`**, so the
shared row survives the §2 correction. No existing row matches (`_Single` 1.0/1.1/1, `_Slash` 1.0/0.5/19,
`_Burst` 1.2/1.2/96).

Derived mechanically from [P0-D3]: loop = the `systemState` Loop Duration (10 s), particle lifetime =
the maximum `lifetimeResolved` among the emitters (10 s), burst = the §2 spawn counts (1 + 1 + 1).
The system is `Loop Once`, so its 10 s duration doubles as the gym's re-fire cadence — the A/B harness
re-arms a finishing original on `OnSystemFinished`, which is exactly what a 10 s looping template does.

**One row across both projectile recreations.** They differ only in per-layer constants, which live in
the behaviors, not the row. It is named for the cadence shape rather than for one effect —
`PS_CkParticles_Template_ProjectileTrio` — and both behaviors route to it. That keeps the cadence table
honest (a row is a cadence, not an effect) and avoids two byte-identical templates.

Layer index = `Seed % 3`.

### 6.2 Renderers / VisTag

**Simpler than the Arrow variant: every layer is `VelocityAligned`.** Two looks, one renderer kind:

| Row renderer | Kind | VisTag | Look | Draws |
|---|---|---|---|---|
| A | `VelocityAlignedSprite` | **10** | new `PartDisAdd01` | layer 0 |
| B | `VelocityAlignedSprite` | **11** | existing `PartDisAdd04` | layers 1–2 |

Both kinds already exist in `ECk_ParticlesRenderer_Kind`, so **this effect needs no new renderer
capability at all** — unlike the Arrow variant, whose `Glow_01` is camera-facing.

`Get_BehaviorLookName` stays `NAME_None`; the row renderers bind their look masters explicitly.
VisTags allocate above `SharedRendererVisTag_Max`, read through `Get_RosterVisTag_Max()` — never a
literal.

The shared row ended up carrying **two** renderers, not four: `NS_Arrow_Projectile` routes its
camera-facing head through the shared VisTag 0 sprite (its §6.2 option (a)) and shares this row's
Part04 renderer for its two tails, so nothing further had to be declared.

### 6.3 CkUsf looks

| Look | State | Values |
|---|---|---|
| `PartDisAdd04` | **already exists** (NS_BasicAttack) — `SparkStreak`, `Brightness` 6 | reusable as-is |
| `PartDisAdd01` | **NEW** — shared with `NS_Arrow_Projectile` | `ShapeTex`/`DissolveTex` = `SoftParticle`; `Brightness` **1**; `OpacityBoldness` **0.5**; every other family parameter at its inert default (`DissolveSpeed` 0, `DissolveBias` 0, `DissolveScale` (1,1), `DistortIntensity` 0, `DistortSpeed` (0,0), `MainTexScale` (1,1), `CoreColor` (1,1,1)) |

### 6.4 Mesh / texture needs

**None new.** No meshes; both textures already have measured procedural stand-ins.

### 6.5 Behavior id

**Allocated at implementation time: 18** (`NumBehaviors` 18 → 20 in the same pass that added 19).

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

## 7. Procedural textures — measured, never copied

**Nothing new was baked.** Both required paints already have measured procedural stand-ins from the
NS_BasicAttack port (that recipe's §7 records the measurements and the residuals):

| Source paint | Stand-in | Used by |
|---|---|---|
| `T_VFX_Part_01` (512² `TSF_G8`, `TA_Clamp`) | `T_CkParticles_SoftParticle` — `pow(saturate(1-r), 2.2)` | `PartDisAdd01` shape + dissolve |
| `T_VFX_Part_04` (512² `TSF_G16`, `TA_Wrap`) | `T_CkParticles_SparkStreak` | `PartDisAdd04` shape + dissolve |

`T_VFX_Noise_02` and `T_VFX_WhitePixel` are not needed: `Distortion_Intensity` resolves to 0 on both
instances and the gradient map is a white pixel.

The streak stand-in's measured profile is narrow in u and long in v (half-width 0.156 along **v**,
anisotropy |dV|/|dU| = 0.31), which is what makes `(35, 500)` read as a streak rather than a smear. It
must not be "corrected" to run the other way.

---

## 8. CkParticles translation

| | |
|---|---|
| Behavior ID | **18** |
| Template | **`PS_CkParticles_Template_ProjectileTrio`** (new cadence row: loop 10 s, lifetime 10 s, burst 3), shared with behavior 19 |
| Layer partition | `Seed % 3` — burst UniqueIDs are sequential, so the single burst draws exactly one particle per source emitter. Double-modulo, so a negative Seed still lands in range |
| Seeds / salts | **none** — this system has no per-particle randomness at all. The layer index is the residue itself |
| `VisTag` | **10** (layer 0) and **11** (layers 1–2), both row-declared |

Stage outputs written:

| Output | Layer 0 `Glow_01` | Layer 1 `Projectile_Bright` | Layer 2 `Projectile_Dark` |
|---|---|---|---|
| `Position` | `(-52.048, 0, 0)` | `(-77.6262, 0, 0)` | `(-194.751, 0, 0)` |
| `Color` | `(1, 0.194618, 0.021219, 0.5)` | `(1, 0.552046, 0.147, 1)` | `(0.1, 0.0171441, 0.00865005, 0.2)` |
| `Size` | `(80, 300)` | `(20, 200)` | `(35, 500)` |
| `Dynamic` | `[1, 0, 0, 0]` | `[0, 0, 0, 0]` | `[0, 0, 0, 0]` |
| `Velocity` | `(0.01, 0, 0)` | `(0.01, 0, 0)` | `(0.01, 0, 0)` |
| `VisTag` | 10 | 11 | 11 |
| `Scale`, `Orientation`, `MeshIndex`, `Rotation`, `SpriteAlignment`, `SpriteFacing` | default | default | default |

**Age decides exactly one thing: death.** There is no curve anywhere in this system, so the behavior is
a pure function of the layer index. Past the source's own 10 s the behavior writes zero colour, size and
scale — the mirror of `Particle State`'s *Kill Particles When Lifetime Has Elapsed*. The cadence row's
particle lifetime is the same 10 s today, so that branch is unreachable in practice; it exists so a
future retune of the row cannot leave a bullet trail hanging in the air.

**The three offsets, not the colours, are the effect.** All three sprites are velocity-aligned along
local +X and stacked backwards along −X, the dark tail reaching 500 units of streak from 195 units back.
That is the entire visual difference from `NS_Arrow_Projectile`, whose equivalents are 150 and 66.

**`Add Velocity = (0.01, 0, 0)` is reproduced for its direction only.** At 0.01 units/second the particle
does not measurably move in 10 s; the vector exists so `VelocityAligned` has an axis. Dropping it would
degenerate the alignment and collapse every streak.

---

## 9. CkUsf translation

Both looks are parameterizations of the existing `DissolveAdd` family (`/CkUsf/Looks/DissolveAdd.ush`).
**No shader math was written or changed for this port** — the family's semantics (additive dissolve,
clamped shape pan) are settled and behavior 17 depends on them.

| Look | State | Values |
|---|---|---|
| `PartDisAdd04` | reused verbatim from NS_BasicAttack | `SparkStreak` shape + dissolve, `Brightness` 6 |
| `PartDisAdd01` | **NEW** | `SoftParticle` shape + dissolve, `Brightness` 1, `OpacityBoldness` **0.5**, every other family parameter at its inert default |

`PartDisAdd01` is the family's REFERENCE instance — every scalar sits at the parent's default — which is
why its delta row in §4 is empty and why the inherited-pair rule matters here: an absent delta means the
`M_VFX_DisAdd_Part01` value, never zero.

### The one new plumbing this needed

`OpacityBoldness` was hardcoded to `1.0` inside `CkUsf::Usf_DissolveAddParams` because all five
NS_BasicAttack instances resolve 1. `PartDisAdd01` is the first that does not, so the helper gained a
**trailing** `InOpacityBoldness = 1.0` parameter — trailing so the five existing looks are unchanged, and
because AngelScript permits defaults only on trailing parameters.

§6.7 asked whether the family shader actually multiplies it. **It does** — `DissolveAdd.ush` ends with
`O.Opacity = saturate(Shape * Mask * In.ParticleColor.a * OpacityBoldness)`, so 0.5 genuinely halves this
layer's coverage rather than being inert.

---

## 10. Copied asset destinations

**None.** No texture, mesh or material is copied from the source pack: the two textures are procedural
stand-ins and both materials are recreated as CkUsf looks. The recreation therefore has no
`/Game/Vefects` reference of any kind, and the gym addresses the ORIGINAL by path string only.

---

## 11. Runtime binding path

```
UCk_Utils_Particles_UE::Spawn_BehaviorAtLocation(ctx, 18, Location, Rotation, Scale)
  └─ ck::particles::Get_BehaviorTemplateSystemObjectPath(18) → PS_CkParticles_Template_ProjectileTrio
  └─ SpawnSystemAtLocation → SetIntParameter("User.BehaviorId", 18)
  └─ ck::particles::Get_BehaviorLookName(18) → NAME_None  (nothing to bind)
       └─ both looks are already baked into the row's renderers (VisTag 10 / 11)
```

The gym pair's `TextureName` is `NAME_None` for exactly that reason: an explicit texture binds a material
instance to `User.SpriteMaterial`, which only the shared camera/custom-facing sprites read — behavior 18
writes neither tag, so it would be an inert argument that merely looked meaningful.

---

## 12. Exact verification procedure

### Automated

| Test | Lane | Asserts |
|---|---|---|
| `CkTests.UnitTests.CkParticles.GunshotProjectileBehavior` | any (no Niagara, no RHI, no fork) | the shared cadence row exists and carries 10 / 10 / 3, and both 18 and 19 route to it; the 3-slot partition covers exactly the two row renderers and is stable across bursts; every §5 colour, size, offset and dynamic channel on all three layers; **every layer is CONSTANT across its whole life and across seeds** (the source has no curve and no randomness); all three drift at `(0.01, 0, 0)`; each layer emits nonzero colour × alpha somewhere in its life (anti-vacuity) and is alive at the last instant of the 10 s; every layer dead past 10 s. Cannot pass vacuously — the pre-switch default VisTag is 0 and the layer assertions need 10/11 |
| `CkTests.UnitTests.CkParticles.RosterSanity` | any | picks 18 up from `NumBehaviors`: it routes to a template the cadence table declares, the row's renderers sit above the shared band and name looks, and its outputs are finite/renderable across the age-lifetime-seed sweep |
| `CkTests.UnitTests.CkUsf.GeneratesUsableMasters` | `--no-nullrhi` | `PartDisAdd01` actually compiles, not merely generates |
| `CkTests.UnitTests.CkUsf.NiagaraSpriteContract` | `--no-nullrhi` | the new look's sprite opt-in produces the sprite usage flag and connected particle pins |
| `Ck_AutoTest_Particles_SpawnAllBehaviors` | `--no-nullrhi` | id 18 spawns a live component through the new template |
| `CkAutoTest_VfxExamples_PairStationsSpawn` | `--no-nullrhi` | the GUNSHOT PROJECTILE pair spawns both sides |

**Source-transcription self-check (ran 2026-08-02, scratchpad only).** A Python script parsed the corpus
sidecar, the `.ush` and the CPU mirror independently and compared all three: **max error 0.0** on every
layer's Position / Color / Size / Dynamic / Velocity, and GPU↔CPU lockstep 0.0 on every field. That
checks transcription, not rendering — it cannot see a shader that fails to compile.

Regeneration order matters and is not obvious: **the CkUsf looks must exist before the templates are
rebuilt**, because the row renderers resolve their look masters at build time. Build → generate looks →
regenerate templates → gates, and `grep -ac ExecuteStage` on the NEW
`PS_CkParticles_Template_ProjectileTrio.uasset` must be non-zero.

**Know which gate holds which line.** `GunshotProjectileBehavior` is the only test that checks this
behavior's *correctness*; everything else is an existence check. None of them covers the GPU `.ush` — it
cannot be executed headlessly, so GPU/CPU lockstep stays a review obligation (plus the transcription
check above). And none substitutes for the visual gate below.

### `[EDITOR-VERIFY]` — visual fidelity gate (human, ~10 min)

Neither Niagara graph is opened at any point. The original asset stays read-only.

1. Regenerate, in this order: `Ck_Usf_GenerateLooks` in the console, then Editor Subsystems →
   `CkParticles_GeneratorSubsystem` → **Create Template System**.
2. PIE the **VfxExamples** gym and go to the **GUNSHOT PROJECTILE** pair. A "add the Vefects content
   plugin" placard on the original pedestal means the pack is absent — fix that before comparing.
3. `Ck_GymVfxExamples_RestartAll` so both sides start from t=0 together.
4. View from the side (the streaks run along local X) and then from behind.

   | # | Criterion | Expected |
   |---|---|---|
   | a | Layer count | three overlapping streaks, nothing else |
   | b | Stack order along −X | glow head nearest the origin, bright tail behind it, dark tail furthest back |
   | c | Tail length | the dark tail is by far the longest — roughly 2.5× the bright one |
   | d | Streak axis | all three lie along the same axis (local +X) and are edge-on-thin, not round |
   | e | Palette | red-hot head, orange bright tail, near-black dark tail — hotter and redder than the Arrow pair |
   | f | Head erosion | the glow head is visibly eaten by a STATIC noise pattern that does not pan or crawl |
   | g | Head coverage | the head reads as a soft, half-strength glow, not a solid blob (`OpacityBoldness` 0.5) |
   | h | Stability over time | nothing moves, brightens, fades or flickers across the whole 10 s |
   | i | Restart | both sides vanish and re-appear together on the 10 s boundary |
   | j | Under rotation / scale | rotate the actor 45°, set scale 2.0 — both respond the same way |

5. Record every mismatch in §13 with the timestamp and criterion letter.

---

## 13. Confirmed fidelity differences or intentional deviations

**The visual gate in §12 has NOT been executed and no lane has been run.** Everything below is a
*known* difference derived from the source data, or *unverified*.

### Known differences — deliberate

1. **Procedural textures instead of the hand paints.** `SoftParticle` and `SparkStreak` are analytic
   stand-ins measured off the corpus PNGs (§7); the residual gap is hand-paint idiosyncrasy plus the
   measured residuals recorded in NS_BasicAttack §13.
2. **`Opacty_DepthFade` (20 on Part01, 30 on Part04) is not wired.** CkUsf surface looks have no
   scene-depth input. Visible only where a sprite intersects geometry — on a stationary pedestal it is
   not visible at all. Same gap the Lightning and Slash recipes record.
3. **`Opacty_Step` (0), `Opacty_StepAdd` (0.1), `GradientMap_Displacement` (0.1), `Glow_Intensity` (1),
   `Core_Power` (1), `Core_Intensity` (0), `Gradient_Invert` (0.5 / 0), `CamOffset` (0) are not family
   parameters.** All resolve to values expected to be inert on these two instances `[inferred]`, but the
   omission is real and sibling effects in this batch DO drive several of them.
4. **`DistortTex` is `SoftParticle`, not `T_VFX_Noise_02`'s stand-in.** The family helper binds the
   distortion sampler to the same asset as the dissolve one. `Distortion_Intensity` is 0 on both
   instances, so the branch is dead and the choice cannot be observed.
5. **The recreation loops; the source completes.** The source is `Loop Once` + *Complete (let emitters
   finish then kill the system)*; the CkParticles template loops its 10 s row forever. The A/B harness
   re-arms the original on `OnSystemFinished`, so the two pedestals stay in phase — but a one-frame gap
   at each loop boundary is possible on the Ck side, where the source instead dies and is respawned.
6. **World space.** Both sides are local-space, so this port has no C12 gap on the pedestal. Attached to
   a MOVING spawner the source's trail would still be local-space and would follow rigidly; nothing here
   models a world-space trail, and the gym cannot exercise the difference.

### Unverified

- Every visual criterion in §12. Nothing has been rendered.
- Whether `OpacityBoldness` 0.5 reads as intended once the head is composited additively against the
  two tails — the arithmetic is confirmed (§9), the perceptual result is not.
- Whether three coincident static sprites at these sizes over-brighten the same way the original's do;
  the corrected §2 says both sides now carry the same three particles, so they should.

---

## 14. Reusable lessons for future effects

1. **`Life Cycle Mode = System` silently voids every emitter Loop row.** This sheet recorded a 1.0 s
   loop from the emitter stack and was wrong by 10×; only the v3 `systemState` export caught it. Read
   the system stack FIRST on every remaining port — [P0-D1]/[P0-D3] exist because of this.
2. **A corrected cadence can change the particle COUNT, not just the timing.** "10 overlapping
   generations, 30 live particles" became "one generation, 3 particles". Any recipe that reasons about
   additive over-brightening from stacked generations must re-derive it after reconciliation.
3. **A sheet's inert-value reading is worth re-checking even when it turns out right.** The v3 exporter
   confirmed `Lifetime Min/Max 0.2/0.4` as `inertValues` — the sheet's guess held, but only mechanically
   is that knowable.
4. **A row is a cadence, not an effect.** Two systems with identical loop/lifetime/burst share one
   template; the per-layer constants live in the behaviors. Naming the row for the cadence shape
   (`ProjectileTrio`) rather than for the first effect that needed it is what makes that reusable.
5. **A camera-facing layer and a velocity-aligned layer can coexist without new renderer kinds** — one
   rides `User.SpriteMaterial` on the shared VisTag 0, the rest ride row renderers. See
   NS_Arrow_Projectile §8. That is worth remembering before reaching for capability C1.
6. **A constants-only behavior needs a CONSTANCY test, not a keyframe test.** With no curve to sample,
   the strongest assertion is "identical at every age and every seed" — which is exactly the assertion
   that catches an accidental `CkParticles_Rand` or a leaked `NormalizedAge`.

