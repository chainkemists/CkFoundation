# Recipe: NS_Dash → CkParticles (IMPLEMENTED)

Schema and evidence-tag conventions: [README.md](README.md). Exemplars: [NS_BasicAttack.md](NS_BasicAttack.md),
[NS_Lightning_Range.md](NS_Lightning_Range.md).

## Completion state — READ FIRST

**Status: IMPLEMENTED as `BehaviorId 46` (2026-08-02) — the true final port of the pack's Skills set.**

`Behavior_Dash.ush` + `ExecuteStage_CPU` case 46, the `PS_CkParticles_Template_Dash` cadence row
(2.0 s loop / 1.55 s lifetime / burst 19 **+ rate 50 per second**), four row renderers on VisTags
**242–245**, `Test_Particles_DashBehavior.cpp`, and the VfxExamples gym pair.

**It added ONE look and ONE mesh, and ZERO textures.** Three of its four material instances were
already carried by earlier ports — matched by INSTANCE against the corpus rather than by name — and
all three of the "new bakes required" its §6.5 called for turned out to be already served by measured
stand-ins. Only `M_VFX_DisAdd_Wind03` and `SM_VFX_Ring04` are unique to this system in the whole pack
(§10, §8).

**It is the pack's only system with a MIXED coordinate space and a MIXED spawn mode.** Three emitters
are local-space and one is world-space; one emitter carries a burst **and** a continuous spawn rate
**and** its own `Self` life cycle. The second of those is expressed exactly (the C2 both-stacks row);
the first is the C12 non-goal and is recorded in §13.1.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Dash` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| User parameters | **none** — `userParameters: []` `[corpus]` |
| Behavior id | **46** (`Dash`) — allocated at implementation time from `ck::particles::NumBehaviors`, per [C-D6] |

Corpus evidence (regenerate per [README.md](README.md); `Saved/` is machine-local and gitignored):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Dash.{json,txt}`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_{Wind01,Wind02,Wind03,Part02}.json`
- `meshes/Vefects/Anime_VFX/Shared/Meshes/SM_VFX_{Ring01,Ring04}.{json,obj}`
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Wind_01,Wind_02,Part_02,Noise_02}.json`

**The source Niagara asset was never opened, in the Niagara editor or otherwise.** Every fact below is
`[corpus]` unless tagged otherwise.

> ### ⚠ TWO SYSTEMS SHARE THIS EXACT NAME — take the right one
> `[corpus]` The pack ships a second `NS_Dash` at
> `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Dash`. **This is a true name collision, the same trap
> `NS_Lightning_Range.md` §1 documents.**
>
> **Fastest one-line discriminator `[corpus]`: the stylized sibling exposes a `User.*` parameter block
> — `User.Lines Color 01` = RGBA(0.726575, 0.816055, 1, 1), `User.Scale Overall` = 1,
> `User.Wind Color 01` = RGBA(0.0742136, 0.0886556, 0.111932, 1), `User.Wind Color 02` =
> RGBA(0.630208, 0.751225, 1, 1) — and renders through `MI_VFX_Glow_02`, `MI_VFX_Wind_01`,
> `MI_VFX_Wind_02`, `MI_VFX_Wind_03`. This target has an EMPTY user-parameter list and renders through
> `M_VFX_DisAdd_Wind01/02/03` and `M_VFX_DisAdd_Part02`.**
>
> As with `NS_Fire`, the stylized sibling's `User.Wind Color 01` and `User.Lines Color 01` values are
> **byte-identical** to this system's `Wind_01` colour curve and `Add_Lines` random-colour range — it
> is a parameterized fork of the same effect. Check the user-parameter list, not the look of the
> numbers.

> ### Mesh naming skew — `SM_VFX_Ring01` and `SM_VFX_Ring04` are NOT rings
> `[corpus, measured]` Both carrier meshes are named `Ring*` and neither is a flat ring: `Ring01` is an
> open **cylinder** and `Ring04` is a truncated **cone** (§3). The names are pack-internal; do not
> infer geometry from them, and do not confuse `SM_VFX_Ring04` (a mesh) with `M_VFX_DisAdd_Ring04`
> (the material `NS_Lightning_Range` recreates) or `T_VFX_Ring_04` (a texture). Three unrelated assets,
> one number.

---

## 2. System anatomy `[corpus]`

**4 CPU emitters, all enabled, all bounds Dynamic, `determinism: false`.
6 particles from the three wind emitters, plus a 13-particle burst and a continuous 50 /s rate from
`Add_Lines` on its own 0.3 s self-loop** (≈ 28 particles in the first 0.3 s).

**System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`.**
Per [P0-D1] this RULES the three `Life Cycle Mode = System` wind emitters — their stored
`Infinite / 1.0 s` rows are inert, and they burst **once** over a single 2.0 s cycle rather than
looping every second. *(Was read as a 1.0 s infinite loop.)* `Add_Lines` is `Life Cycle Mode = Self`,
so its own `Once / 0.3 s` row stays LIVE.

**Coordinate space is MIXED and that is the point of the effect** `[corpus]`:

| Emitter | `localSpace` |
|---|---|
| `Wind_01` | **true** (local) |
| `Wind_Smokes` | **true** (local) |
| `Wind_Speed` | **true** (local) |
| `Add_Lines` | **false** (WORLD) |

The three wind shapes ride with the dashing actor; the speed lines are left behind in world space.

| # | Emitter | Count | Spawn t | Life cycle / loop | Lifetime | Renderer | Mesh (renderer scale) | Material |
|---|---|---|---|---|---|---|---|---|
| 1 | `Wind_01` | 1 | 0.05 | System, Infinite / 1.0 | **1.5** | Mesh, Facing Default | `SM_VFX_Ring01` **(1, 1, 5)** | `M_VFX_DisAdd_Wind02` |
| 2 | `Wind_Smokes` | 4 | 0.1 | System, Infinite / 1.0 | 1.0 | Sprite, Unaligned + FaceCamera, **SubUV 2×2** | — | `M_VFX_DisAdd_Wind01` |
| 3 | `Wind_Speed` | 1 | 0.05 | System, Infinite / 1.0 | 0.6 | Mesh, Facing Default | `SM_VFX_Ring04` **(1, 1, 5)** | `M_VFX_DisAdd_Wind03` |
| 4 | `Add_Lines` | **13 burst + 50 /s rate** | 0 (burst) | **Self**, **Once** / **0.3** | rand **0.8–1.0** | Sprite, **VelocityAligned** + FaceCamera | — | `M_VFX_DisAdd_Part02` |

`Wind_01`, `Wind_Smokes` and `Wind_Speed` carry `UseLoopCountLimit = false`, so their stored
`Loop Count Limit = 1` is an **inert authored leftover** (same trap `NS_Lightning_Range.md` §4
records). `Add_Lines` also carries it, and there it is likewise inert — its `Loop Behavior = Once`
under `Life Cycle Mode = Self` is what makes it fire once.

**`Add_Lines` is the only emitter in the entire batch with `Life Cycle Mode = Self`.** Every other
emitter across all six systems is `System`. That matters: for `Add_Lines` the emitter-local
`Loop Behavior = Once` / `Loop Duration = 0.3` **are** the values that run, whereas for the other
three the system life cycle drives — which `[corpus-v3]` is `Once / 2.0 s` (see the system-loop
block above).

### 2.1 Spawn shapes, velocity and forces `[corpus]`

| Emitter | Location | Velocity | Forces |
|---|---|---|---|
| `Wind_01` | Simulation Position, offset (0,0,0), `UsePositionOffset = false` | **Add Velocity = (−150, 0, 0)** constant, `Scale Added Velocity (1,1,1)` | — |
| `Wind_Smokes` | Simulation Position with **`Position Offset (−20, 0, 0)`**, `UsePositionOffset = true` | **Add Velocity = `Random Range Vector 001`**, min **(−100, −10, −10)** / max **(−300, 10, 10)** | — |
| `Wind_Speed` | Simulation Position, offset (0,0,0) | — (no velocity module) | — |
| `Add_Lines` | **Sphere Location**, radius **50**, `Sphere Distribution = Random`, `Surface Only = false`, `UseNonUniformScale = false`, `UseOffset = false`, offset (0,0,0), non-uniform scale (1,1,1) | **Add Velocity in Cone**: cone angle **10°**, **cone axis (−1, 0, 0)**, `Velocity Distribution Along Cone Axis 0.75`, `Velocity Falloff Away From Cone Axis 0.333`, **`Use Velocity Falloff On Cone Axis = true`**, strength = `Random Range Float 002` rand **350–750** | **Drag** (`Use Linear Drag = true`, `Use Rotational Drag = false`), drag = `Random Range Float` rand **0.8–1.2** |

**Everything travels −X.** The constant `(−150, 0, 0)`, the `(−300..−100, ±10, ±10)` random box, the
`(−1, 0, 0)` cone axis and the `(−20, 0, 0)` position offset all point the same way. The dash reads as
motion toward +X with the effect trailing to −X.

All four run `Solve Forces and Velocity` with `Clamp Velocity = false`, `Limit Acceleration = false`,
`Rotational Solver Is Enabled = true`, `Acceleration Limit 9999`, `Speed Limit 1000` — the limits are
authored but **not engaged**, so treat them as inert. (`Wind_Speed` has no velocity, so its solver is
a no-op.)

`Add_Lines` additionally sets `Mass Mode = Random`, `Mass Min 0.75 / Max 2` — mass only matters
because linear drag is on.

### 2.2 No events, no ribbons, no lights `[corpus]`

`stacks` is length 3 on every emitter and no `Generate Location Event` appears. `renderers` is
length 1 on every emitter; two are `NiagaraMeshRendererProperties`, two are
`NiagaraSpriteRendererProperties`. **No ribbon renderer and no light renderer in this system** — the
two heaviest explosion-family gaps are absent here.

### 2.3 Mesh orientation is animated `[corpus]`

Both mesh emitters set an initial orientation **and** rotate it over life:

| Emitter | `Initial Mesh Orientation` | `Update Mesh Orientation` |
|---|---|---|
| `Wind_01` | Coordinate space **Mesh**, `Use Orientation Vector = true`, `Use Rotation Vector = true`, `Orientation Axis (1, 0, 0)`, `Orientation Vector (1, 0, 0)`, **`Rotation (0, 0.25, 0)`** | Coordinate space **Simulation**, `Rotation Rate` **0.3**, `Rotation Vector (1, 0, 0)` |
| `Wind_Speed` | Coordinate space **Mesh**, `Use Orientation Vector = true`, `Use Rotation Vector = true`, `Orientation Axis (1, 0, 0)`, `Orientation Vector (1, 0, 0)`, **`Rotation (0, 0.75, 0)`** | Coordinate space **Simulation**, `Rotation Rate` **0.05**, `Rotation Vector (1, 0, 0)` |

Both carriers are built along **+Z** (§3) and both are oriented onto **world +X** by the
`Orientation Axis (1,0,0)` → `Orientation Vector (1,0,0)` pair, then given an extra roll
(`Rotation (0, 0.25, 0)` / `(0, 0.75, 0)`) and a continuous spin about **+X** at 0.3 / 0.05 turns per
second `[inferred — the units of Niagara's `Rotation Rate` are revolutions per second; the exporter
records the number, not the unit]`.

**RESOLVED — the units are TURNS `[P6-A4]`.** `Rotation (0, 0.25, 0)` is a quarter turn about Y and
`Rotation Rate 0.3` is 0.3 turns per second. This is not a fresh inference: `NS_Arrow_Cast`'s own
`Wind_01` emitter carries the IDENTICAL pair — `Rotation (0, 0.25, 0)` with `Rotation Rate 0.3` on the
same `SM_VFX_Ring01` carrier — and behavior 23 shipped and was gated on the turns reading
(`Behavior_ArrowCast.ush`, the WINDMESH layer). A degrees reading would leave the tube's own +Z axis
pointing at the sky instead of down the -X travel direction, which is visibly wrong on both systems.
*(Was `[unresolved]`.)*

---

## 3. Mesh geometry `[corpus, measured from the .obj]`

Two carrier meshes, both 132 verts / 128 tris / 2 UV sets. Both `.json` files report the section
material as `M_VFX_DisAdd_Slash01` — that is the mesh asset's *default* slot material, **overridden by
the emitter's renderer material**; it is not what draws.

### `SM_VFX_Ring01` — an open CYLINDER (used by `Wind_01`)

- Bounds X ±100, Y ±100, **Z 0.0000 .. 50.0000**.
- **Radius is constant at 99.50 .. 100.00 at EVERY angle bucket and EVERY height** — a thin-walled
  tube, wall thickness **0.5**, radius 100, height 50, open at both ends. No taper.
- **UV: `v` runs ALONG the axis — `corr(v, z) = −1.000` exactly. v = 0 is the TOP (mean Z 50.00),
  v = 1 is the BOTTOM (mean Z 0.00).** `u` wraps the circumference (u = 0 and u = 1 both land at
  angle −90°, i.e. the seam is at −Y). uv0 covers 0..1 fully.
- Renderer scale **(1, 1, 5)** × `Mesh Uniform Scale 0.3` × the size curve (peaks at 2 in X/Y and
  **5 in Z**, §5) ⇒ at peak the tube is ≈ 100 × 0.3 × 2 = **60 units in radius** and
  ≈ 50 × 5 × 0.3 × 5 = **375 units long**. A long, thin wind tube around the dash axis.

### `SM_VFX_Ring04` — a truncated CONE / funnel (used by `Wind_Speed`)

- Bounds X ±100, Y ±100, **Z −0.5000 .. 32.2965**.
- **Radius grows with height**: `corr(v, radius) = +1.000` and `corr(v, z) = +1.000`. The narrow end
  is radius **53.96** at Z ≈ −0.25; the wide end is radius **100.00** at Z ≈ 32.05. Constant profile
  at every angle bucket — a clean truncated cone, open at both ends.
- **UV: v = 0 at the NARROW end, v = 1 at the WIDE end**; `u` wraps the circumference with the seam at
  angle −90°, as in `Ring01`. uv0 covers 0..1 fully.
- Renderer scale **(1, 1, 5)** × `Mesh Scale (0.4, 0.4, 0.3)` (non-uniform) × the size curve (peaks at
  2 uniformly, §5) ⇒ at peak ≈ 100 × 0.4 × 2 = **80 units** at the wide end and
  ≈ 32.3 × 5 × 0.3 × 2 = **97 units** long. A short flared skirt — the "speed cone".

Neither shape exists in the CkParticles mesh generator today (`Sweep` / `Tube` / `Shell` / `Disc` +
`Crescent`). **Check `Tube` first** — if the existing `Tube` carrier is an open cylinder with
axis-aligned v, `Ring01` may need no new mesh at all, only a UV check.

---

## 4. Material family and per-instance deltas `[corpus]`

**One family, four instances.** All four are instances of
`/Game/Vefects/Anime_VFX/Shared/Materials/Parents/M_VFX_DissolveAdd` — the family the CkUsf
`DissolveAdd` look **already implements** (`/CkUsf/Looks/DissolveAdd.ush`, entry
`CkUsf_Look_DissolveAdd`; see `NS_BasicAttack.md` §9).

All four: `MD_Surface` / `BLEND_Translucent` / `MSM_Unlit`, `twoSided: false`, connected outputs
**`EmissiveColor` + `Opacity`** only, dynamic parameters **`[dissolve, distortion, offset,
core_color]`**.

Deltas stated **against `M_VFX_DisAdd_Slash01`**, the family reference `NS_BasicAttack.md` §4
documents (Brightness 5, `Dissolve_Speed` (0.3, −0.1), `Distortion_Intensity` 1, `Opacty_DepthFade` 20,
`GradientMap_Displacement` 0.1, `Opacty_StepAdd` 0.1, `Opacity_Boldness` 1, `Gradient_Invert` 0,
`Core_Intensity` 0, `Glow_Intensity` 1, unit scales/offsets/speeds, `Color_Core` RGBA(1,1,1,0),
`GradientMap_Tex` = `T_VFX_WhitePixel`,
`Dissolve_Tex`/`Distortion_Tex`/`GradientShape_Tex` = `T_VFX_Noise_02`). Anything not listed is
unchanged.

| Material | Main / Color tex | Dissolve tex | Brightness | Other deltas vs Slash01 |
|---|---|---|---|---|
| `Wind01` (smokes) | `T_VFX_Wind_01` | *(Noise_02)* | **3** | `Dissolve_Speed` (**0, −0.15**); `Distortion_Intensity` **0.5**; `Core_Intensity` **1** |
| `Wind02` (`Wind_01` tube) | `T_VFX_Wind_02` | `T_VFX_Wind_02` | **7** | `Color_Speed_X` **−0.3**; `Dissolve_Scale` (**0.7, 0.95**); `Dissolve_Speed` (**−0.1, 0**); `Distortion_Scale_Y` **0.6**; `Distortion_Speed` (**0.1, 0.1**) — note `Distortion_Intensity` stays at the family's **1** |
| `Wind03` (`Wind_Speed` cone) | `T_VFX_Wind_02` | `T_VFX_Wind_02` | **2** | `Color_Scale` (**3, 0.15**); `Color_Offset` (**0.5, 0.41**); `Color_Speed_X` **−0.3**; `Dissolve_Scale` (**3, 0.15**); `Dissolve_Offset` (**0.5, 0.41**); `Dissolve_Speed` (**−0.1, 0**); `Distortion_Intensity` **0**; `MainTex_Scale` (**4, 0.2**); `MainTex_Offset_Y` **0.35**; **`Core_Intensity` 20** |
| `Part02` (speed lines) | `T_VFX_Part_02` | `T_VFX_Part_02` | **1** | `Dissolve_Speed` (0, 0); `Distortion_Intensity` **0**; `Glow_Intensity` **0.3**; `Gradient_Invert` **0.5**; `Opacity_Boldness` **0.5** |

**`Wind03` is the most heavily re-tiled instance in the whole batch.** Its `MainTex_Scale (4, 0.2)`
plus `Dissolve_Scale (3, 0.15)` plus their offsets stretch a 512² wind texture into a long thin
streak wrapped around the cone; its `Core_Intensity 20` is an order of magnitude above anything else
measured here. Both are load-bearing for the "speed cone" read and neither is plumbed today.

**Family parameters this system exercises that the CkUsf `DissolveAdd` look does NOT plumb today:**

- `Color_Scale_X/Y`, `Color_Offset_X/Y`, `Color_Speed_X/Y` — the family samples a *separate*
  `Color_Tex` with its own tiling. The current look folds `Main_Tex` and `Color_Tex` into one
  `ShapeTex`; `Wind03` tiles them **differently** (`MainTex_Scale (4, 0.2)` vs
  `Color_Scale (3, 0.15)`), so folding them is a real fidelity loss here rather than a no-op.
- `Dissolve_Offset_X/Y` and `MainTex_Offset_X/Y` (both non-zero on `Wind03`).
- `Core_Intensity` (1 on `Wind01`, **20** on `Wind03`).
- `Glow_Intensity` (0.3 on `Part02`).
- `Opacty_DepthFade` (20 on all four) — the known CkUsf gap.

### 4.1 Texture dependency audit `[corpus]`

All greyscale masks (`TC_Alpha`, `sRGB = false`, `TEXTUREGROUP_World`), 512×512.

| Texture | Src fmt | Address | Used as | Procedural stand-in available today? |
|---|---|---|---|---|
| `T_VFX_Wind_01` | TSF_G16 | Wrap/Wrap | `Wind01` main + color | **PARTIAL** — `T_CkParticles_WindBand` was measured off `T_VFX_Wind_03`, a *different* texture. Needs its own measurement. Shared with `NS_Fire`'s `Flames01`. |
| `T_VFX_Wind_02` | TSF_G8 | Wrap/Wrap | `Wind02` and `Wind03` main + color + dissolve | **needs measurement** — the single most important texture in this effect (it draws both mesh emitters) |
| `T_VFX_Part_02` | TSF_G8 | **Clamp/Clamp** | `Part02` main + color + dissolve | **needs measurement** — clamp-addressed, so likely a single centred blob/streak like `T_VFX_Part_01` rather than a tiling pattern |
| `T_VFX_Noise_02` | TSF_G16 | Wrap/Wrap | `Wind01` dissolve + distortion; family-default gradient shape | **YES** — `T_CkParticles_TileNoise` (`NS_BasicAttack.md` §7) |
| `T_VFX_WhitePixel` | TSF_RGBA16, sRGB | Wrap/Wrap | gradient map on all four | **no-op** — not copied, not needed |

**Three new bakes required.**

---

## 5. Per-emitter runtime curves — EXACT keyframes `[corpus]`

All curves sample **NormalizedAge** (0→1 over that emitter's own particle lifetime). `C` = constant
key, `L` = linear key. Verbatim, including the source's own float noise (`9.74764e-10`,
`5.88215e-08` are authored zeros — reproduce them as 0).

**Three of the four emitters share one alpha envelope shape** — in, hold, out — with different hold
lengths. That shared shape is the effect's signature.

### 1. `Wind_01` — mesh (`SM_VFX_Ring01`), 1 particle, spawn t 0.05, lifetime **1.5**
- **Scale Velocity** (Vector from Curve): X, Y, Z all (0, 1)C (0.2, 0.15)C (1, 0)C
- **Color** (Color from Curve) — RGB is a **single flat key per channel**:
  R (0, **0.0742136**)C | G (0, **0.0886556**)C | B (0, **0.111932**)C |
  **A (0, 0)L (0.240266, 1)C (0.676124, 1)L (1, 0)L**
  — a near-black, very slightly blue tint under an in-hold-out alpha envelope. `Color.Scale Alpha`
  **0.3**, so peak alpha is 0.3.
- **Scale Mesh Size** (Scale Float by Curve): X (0, **1.5**)C (0.2, **2**)C | Y (0, 1.5)C (0.2, 2)C |
  **Z (0, 0.5)C (0.3, 3)C (1, 5)C** — X and Y hold at 2 past t = 0.2 (two keys only) while **Z keeps
  growing to 5×**. The tube stretches lengthwise over its whole 1.5 s life.
- **Dyn param 1 (`dissolve`)** (Float from Curve): (0, **−0.2**)C (1, **−1**)C; params 2, 3, 4 = 0
- **Update Mesh Orientation**: Simulation space, rate 0.3, vector (1, 0, 0) (§2.3)
- Initialize: `Color = RGBA(1, 0.184475, 0.386429, 1)` (overridden by the curve),
  `Mesh Uniform Scale` **0.3**, `Uniform Sprite Size 0` (inert — mesh renderer),
  `ScaleFloatByCurve.InitialValue (1, 1, 1)`, `VectorFromCurve.Scale Curve (1, 1, 1)`
- Inert leftovers: `Mesh Scale Min (0.1, 0.1, 0.5)`, `Mesh Scale Max (0.05, 0.2, 0.5)`,
  `Mesh Uniform Scale Min/Max 1/2` — the mode is `Uniform`, so only `Mesh Uniform Scale` drives

### 2. `Wind_Smokes` — sprite, SubUV 2×2, 4 particles, spawn t 0.1, lifetime **1.0**
- **Scale Velocity**: X, Y, Z all (0, 1)C (0.2, 0.3)C (1, 9.74764e-10)C
- **Color**: R (0, **0.6**)C | G (0, **0.743954**)C | B (0, **1**)C |
  **A (0, 0)L (0.240266, 1)C (0.676124, 1)L (1, 0)L** — the same alpha envelope as `Wind_01`, over a
  pale blue. `Color.Scale Alpha` **0.2**, so peak alpha is 0.2.
- **Dyn param 1 (`dissolve`)** (Float from Curve): (0, −5.88215e-08)C (1, −1)C; params 2, 3, 4 = 0
- **Scale Sprite Size** (Uniform Curve): (0, **0.5**)C (1, **1**)C — grows monotonically, never fades
  by size
- Sizes: Random **Non-Uniform**, min **(555, 130)** max **(777, 230)** — very wide, short quads
- Sprite rotation: Random, angle **−30..30°** (`Sprite Rotation Angle Min/Max`)
- `Sub UVAnimation`: mode **Random**, start frame 0, end frame 3, `SubUV Loop Count 1`
- Position offset **(−20, 0, 0)** with `UsePositionOffset = true`
- Initialize `Color = RGBA(1, 0.184475, 0.386429, 1)` (overridden by the curve)
- Inert leftovers: `Mesh Scale (1,1,1)`, `Mesh Scale Min/Max`, `Mesh Uniform Scale*`,
  `Uniform Sprite Size 0`, `Uniform Sprite Size Min/Max 130/230` — the size mode is Random
  **Non-Uniform**, so only `Sprite Size Min/Max` drives

### 3. `Wind_Speed` — mesh (`SM_VFX_Ring04`), 1 particle, spawn t 0.05, lifetime **0.6**
- **Color**: R (0, **0.597202**)C | G (0, **0.752942**)C | B (0, **1**)C |
  **A (0, 0)L (0.240266, 1)C (0.373076, 1)L (1, 0)L** — same envelope shape, **shorter hold**
  (0.373076 vs 0.676124). `Color.Scale Alpha` **0.05**, so peak alpha is 0.05 — this layer is a very
  faint wash.
- **Scale Mesh Size** (Scale Float by Curve): X, Y, **Z** all (0, 1.5)C (0.2, 2)C — uniform growth,
  then hold (two keys per channel)
- **Dynamic Material Parameters**: no curve override. **Param 1 (`dissolve`) = −0.92719 constant**;
  params 2, 3, 4 = 0
- **Update Mesh Orientation**: Simulation space, rate **0.05**, vector (1, 0, 0) (§2.3)
- **No `Scale Velocity` and no velocity module** — this layer does not move relative to its (local)
  spawn point
- Initialize: `Color = RGBA(1, 0.184475, 0.386429, 1)` (overridden), `Mesh Scale` **(0.4, 0.4, 0.3)**
  (mode Non-Uniform), `Uniform Sprite Size 0` (inert)
- Inert leftovers: `Mesh Scale Min/Max`, `Mesh Uniform Scale 0.3`, `Mesh Uniform Scale Min/Max 1/2`

### 4. `Add_Lines` — velocity-aligned sprite, **WORLD space**, 13 burst + 50 /s, lifetime rand 0.8–1.0
- **Colour is randomized at spawn, not curved**: `[override] Color = dyn:Random Range Linear Color`,
  min **RGBA(0.737095, 0.852379, 1, 0.3)** / max **RGBA(0.726575, 0.816055, 1, 0.7)** — note the min's
  R and G are *larger* than the max's (as exported; treat per-channel lo/hi). A pale blue-white with a
  randomized 0.3–0.7 alpha.
- **Scale Color** (RGB and Alpha Separately, `ScaleRGB`/`ScaleA`/`ScaleRGBA` all true):
  **Scale Alpha** = Float from Curve **(−0.00459771, 1.01132)C (1.00115, 0.00195575)C** — a linear
  fade whose keys sit slightly OUTSIDE [0, 1] on both axes (an authored "1 → 0 across the whole life"
  ramp with handles pulled past the ends; sample it clamped). `Scale RGB (1,1,1)`,
  `Scale RGBA (1,1,1,1)`.
- **Drag**: `Use Linear Drag = true`, drag = `Random Range Float` rand **0.8–1.2**;
  `Rotational Drag 1` with `Use Rotational Drag = false` (inert)
- **Scale Sprite Size by Speed**: `Sample Scale Factor By Curve = true`,
  **Scale Factor Curve (0.12466, −0.000593)C (0.394801, 0.48928)C (0.995789, 0.997444)C**,
  `Velocity Threshold` **1500**, `Min Scale Factor (0, 1)`, `Max Scale Factor (0.5, 3)`
  — the streak's length scales with |velocity| / 1500 through that curve, between (0,1) and (0.5,3).
  This is what makes fast lines long and slow ones short.
- Sizes: Random Non-Uniform, min **(30, 32)** max **(30, 32)** — i.e. **constant (30, 32)** before the
  speed scaling
- Lifetime: Mode **Random**, `Lifetime Min 0.8 / Max 1.0`. **There is NO `[override] Lifetime` on this
  emitter**, so — unlike every randomized-lifetime emitter elsewhere in this batch — the
  `Lifetime Min / Max` pair is unambiguously what drives. `[corpus]`
- Mass: Mode Random, `Mass Min 0.75 / Max 2` (matters only because linear drag is on)
- `FloatFromCurve.Scale Curve = 1`

---

## 6. Translation plan (CkParticles / CkUsf) — AS IMPLEMENTED

### 6.0 Capability gaps — all four answered, three of them by capabilities that landed after this sheet

The sheet was written 2026-08-01, before Phases 1–2 of the porting campaign. Three of its four gaps
were already closed by the time the port ran, and the fourth is the campaign's standing non-goal.

| # | Gap as written | What shipped |
|---|---|---|
| **G3** | Sub-UV flipbook — no `SubImageIndex`, no `SubImageSize`, no atlas maths | **CLOSED by C4 (Phase 1).** The row's smoke renderer declares `SubImageSize (2, 2)` and the behavior writes `SubImageIndex`; the `WindSheet` atlas already existed |
| **G6** | Burst AND continuous rate in one emitter — "the template cannot do both" | **CLOSED by C2 (Phase 2).** `Add_SpawnEmitterStack` composes both stacks on one row, so the row states burst 19 AND rate 50/s and the behavior splits the two populations by SPAWN PHASE. The sheet's own option (b) — the honest one — rather than its option (a) fold `[P6-A1]` |
| **G7** | Mixed coordinate space — three local emitters + one WORLD emitter | **NOT closed.** C12 is the campaign's declared non-goal. All four layers are LOCAL; §13.1 records it, and the option-(b)/option-(c) ideas the sheet floated stay unbuilt |
| **G8** | Emitter-level `Life Cycle Mode = Self` | **CLOSED by C5 + [P2-D5] (Phase 2).** `EmitterAge` reaches the behavior, so `Add_Lines`' 0.3 s window is a real gate on spawn phase rather than an age hack. It still RE-FIRES every loop where the source fires once (§13.2) |

Items that were **work, not gaps** — all four shipped as described:

- **Animated mesh orientation** — `CkParticles_QuatFromAxisAngle` + `CkParticles_QuatMul`, in turns (§2.3).
- **Renderer-level mesh scale (1, 1, 5)** — folded into the behavior's `O.Scale`, no new field.
- **Linear drag with random drag and mass** — closed form `v(t) = v0·exp(-k·t/m)`, asserted
  DeltaTime-independent in the test rather than merely commented (§11).
- **Speed-dependent sprite size** — a direct evaluation of §5.4's three-key curve at `|v| / 1500`.

### 6.1 Cadence row — AS SHIPPED

```
{ TEXT("PS_CkParticles_Template_Dash"), 2.0f, 1.55f, 19, Get_DashRendererSpecs(), 50.0f }
```

- `LoopDuration` **2.0** `[corpus-v3]` — the system's `Once` loop duration, unchanged from the sheet.
- `ParticleLifetime` **1.55** — `[P6-A2] correction, arithmetic class.` The sheet said 1.5 ("max
  resolved lifetime"), which is the PRE-[P0-D5] formula. [P0-D5] is max over layers of
  *(spawn delay + resolved lifetime)*: `Wind_01` is 0.05 + 1.5 = **1.55**, `Wind_Smokes` 0.1 + 1.0 =
  1.1, `Wind_Speed` 0.05 + 0.6 = 0.65, `Add_Lines` 0 + 1.0 = 1.0. `NS_Arrow_Cast`'s row is 1.55 for
  exactly this reason and for the same wind pair. At 1.5 the template would kill the tube 50 ms before
  its own fade finished.
- `BurstCount` **19** + `SpawnRate` **50.0** — `[P6-A1] correction, capability class.` The sheet's 34
  is `1 + 4 + 1 + 28`, where the 28 folds `Add_Lines`' 13-particle burst together with ~15 staggered
  stand-ins for its 50/s rate, because at authoring time a row could carry only one spawn stack. C2
  landed in Phase 2 and the fold is no longer needed: the row carries the source's exact burst
  (1 + 4 + 1 + 13 = **19**) and the source's exact rate (**50/s**), which is the sheet's own §6.0 G6
  option (b). This removes the approximation the sheet told its implementer to record as a fidelity
  gap — §6.7's second bullet no longer applies.
  Layer index for a BURST particle = `Seed % 19` (double-modulo): 0 `Wind_01`, 1–4 `Wind_Smokes`,
  5 `Wind_Speed`, 6–18 `Add_Lines`. A RATE particle is always `Add_Lines` — it is the system's only
  Spawn Rate — and is hidden if its spawn phase is past 0.3 s.
- **Spawn-time offsets are layer state**: `Wind_01` and `Wind_Speed` at 0.05, `Wind_Smokes` at 0.1,
  `Add_Lines` at 0. Each layer hides before its beat and runs its curves on `(age − delay)`.

### 6.2 Renderers and VisTags — AS SHIPPED

Four distinct materials across four emitters, so four row-declared renderers
(`Get_DashRendererSpecs`, naming header). **`CameraFacingSprite` is not a new kind** — `[P6-A6]`, it
landed with C1 in Phase 1 and eleven rows already declare one.

| VisTag | Kind | Mesh | Look | Source emitter |
|---|---|---|---|---|
| 242 | `Mesh` | `Cylinder` (existing) | `WindDisAdd02Mesh` (existing) | `Wind_01` |
| 243 | `CameraFacingSprite`, `SubImageSize (2, 2)` | — | `WindDisAdd01` (existing) | `Wind_Smokes` |
| 244 | `Mesh` | `Cone` (**new**) | `WindDisAdd03` (**new**) | `Wind_Speed` |
| 245 | `VelocityAlignedSprite` | — | `PartDisAdd02` (existing) | `Add_Lines` |

The name collision the sheet warned about is real and was avoided the way `NS_Arrow_Cast` avoided it:
behavior 7's `WindDisAdd02` is a parameterization of `M_VFX_DisAdd_Pan_Wind02` and is untouched; this
system's tube draws `M_VFX_DisAdd_Wind02`, which is `WindDisAdd02Mesh`.

### 6.3 Meshes — one reused, one new

- **`SM_CkParticles_Cylinder` — REUSED.** Checked against §3 before reuse: the generator builds radius
  100, Z 0..50, 32 segments, `v = 0` at the TOP and `u = frac(0.75 − angle/360)` — every one of §3's
  measured facts for `SM_VFX_Ring01`. Same carrier, same source mesh, same renderer scale (1, 1, 5).
- **`SM_CkParticles_Cone` — NEW** (`Surface_Cone`, `CkParticles_MeshGenerator.cpp`). Built in SOURCE
  units, as every other carrier is: narrow radius **53.956** at Z −0.25, wide radius **100.000** at
  Z 32.047, 32 columns × 1 band. `[P6-A5]` the sheet's normalized figures (0.5396 / 1.0 / 0.328) are
  not what shipped — carriers keep source dimensions so the behavior's scale curve stays the source's
  own, and 32.30/100 is 0.323 rather than 0.328 in any case. The source has exactly two vertex rings
  and nothing between them, so one band IS its topology; its 0.5-unit wall (two coaxial cone sheets
  offset in Z) is collapsed to a single two-sided sheet, the Cylinder/Card/FlatAnnulus precedent, and
  the Z values above are those two walls' midplane.

### 6.4 Looks — three reused instances, one new

`[P6-A3] correction.` The sheet asked for four new `Dash*`-prefixed looks. Three of the four source
materials are already carried, matched by INSTANCE path against the corpus rather than by name, and
each was checked value-by-value against §4 before reuse (§10). Only `M_VFX_DisAdd_Wind03` is new, and
it takes the plain name `WindDisAdd03` because nothing collides with it.

The family-parameter extension §6.4 proposed (a separate `ColorTex` sampler with its own
scale/offset/speed, `MainTexOffset`, `DissolveOffset`, `CoreIntensity`, `GlowIntensity`) was **not**
built: `DissolveAdd.ush` is shared by 36 looks across ten ports and this batch's mandate is to leave
it untouched. The consequence is real on `Wind03` and only on `Wind03` — it is the one instance in the
cookbook where folding `Color_Tex` into the shape is not a no-op — and is recorded in §13.3.

### 6.5 Textures — ZERO new bakes

`[P6-A7] correction.` The sheet said "Three new bakes required". All three were already served, which
measurement (not naming) established — see §7.

### 6.6 Behavior id and aim axis

**Id 46**, taken from `ck::particles::NumBehaviors` at implementation time (46 → 47).

**Aim axis: forward is −X, declared and transcribed rather than negated.** §2.1's four independent
facts all point the same way and the two carriers are laid onto +X by their own
`Orientation Axis (1,0,0)`. The roster's MuzzleFlash/Tracer/Beam convention is +X-forward, and this
effect reads as a dash TOWARD +X with everything trailing to −X — the natural reading. Negating would
have inverted the carriers' lay quaternions for no gain, and `NS_Arrow_Cast`/`NS_Gunshot_Cast` already
ship the identical wind pair travelling −X. Stated in the `.ush` header so a caller passing a rotation
knows which way the effect faces.

### 6.7 Known deviations — as they stand after implementation

- **Mixed coordinate space collapses to local** — still true, §13.1. Unchanged.
- ~~`Add_Lines`' continuous 50/s spawn is approximated as 15 staggered burst slots~~ — **no longer
  applies**, §6.1 `[P6-A1]`: the rate is expressed exactly.
- **`Add_Lines`' `Once` life cycle becomes a per-loop 0.3 s window** — still true, §13.2.
- **`Opacty_DepthFade` (20 on all four materials) is dropped** — still true, §13.3.
- **`Color_Tex`'s independent tiling is lost** — still true and visible on `Wind03` only, §13.3.
- **Mesh wall thickness collapsed** on both carriers — still true, §13.5.

---

## 7. Textures — ZERO new bakes, and that is a measurement result

§4.1 called for three new bakes. Every one of them was already in the library, because two
neighbouring systems paint with the same textures:

| Source paint | Already-baked stand-in | How it was established |
|---|---|---|
| `T_VFX_Wind_01` | `T_CkParticles_WindSheet` | The generator's own comment cites it as the `T_VFX_Wind_01` stand-in — a 2×2 flipbook of one wind puff, measured off the source sheet for `NS_Arrow_Cast`, whose `Wind_02` emitter draws the SAME `M_VFX_DisAdd_Wind01` instance |
| `T_VFX_Wind_02` | `T_CkParticles_WindBandMid` | Cited as the `T_VFX_Wind_02` stand-in; measured against the `Wind_03` paint and found to be the same image rolled 141 rows in v (correlation 1.0 after the roll), so it carries both |
| `T_VFX_Part_02` | `T_CkParticles_SoftParticleBright` | Cited as the `T_VFX_Part_02` stand-in — radially symmetric to machine precision, the shoulder-and-fall bell measured for `NS_Gunshot_Hit` |
| `T_VFX_Noise_02` | `T_CkParticles_TileNoise` | The family default, since `NS_BasicAttack` §7 |
| `T_VFX_WhitePixel` | `LutWhite` | The gradient chain's inert white default |

The sheet's "PARTIAL / needs measurement" column was written before the wind and hit texture sets
existed; the correction is `[P6-A7]`. **The lesson is that the check is by PAINT, not by effect** —
`T_VFX_Wind_02` reached the library through a system (`NS_Arrow_Cast`) whose name shares nothing with
this one.

## 8. Meshes — one reused, one new

- **`SM_CkParticles_Cylinder`** carries `Wind_01`. Verified against §3 (radius, height, v direction,
  seam) before reuse, not assumed from the shared source-asset name.
- **`SM_CkParticles_Cone`** is this port's only new carrier, and `SM_VFX_Ring04` is referenced by no
  other system in the pack (`grep -l SM_VFX_Ring04` over the corpus returns `NS_Dash` alone). Its
  numbers are §3's measurements, re-derived from the `.obj` at port time: 132 verts in exactly two
  rings, radius 53.956 → 100.000, Z −0.5..32.2965, `corr(v, radius) = +1.000`, seam at ±180°.

Both slot materials are the generator's `SweepErode` fallback; the row renderers override them with
their own CkUsf looks.

## 9. The behavior — `Behavior_Dash.ush` + `ExecuteStage_CPU` case 46

Four layers behind one spawn-phase split. The GPU file and the CPU mirror were transcribed
independently and their numeric-literal multisets compared: **difference 0** (244 GPU / 245 CPU, the
single CPU-only literal being the `case 46:` label — the established discount).

| Layer | Beat | Life | Motion | Notable |
|---|---|---|---|---|
| `Wind_01` (242) | 0.05 | 1.5 | constant −150 X under a 1 → 0.15 → 0 velocity-scale curve, integrated in closed form | quarter-turn lay + 0.3 turn/s spin about +X; X/Y hold at 2× while Z grows to 5× |
| `Wind_Smokes` (243) | 0.1 | 1.0 | random box (−300..−100, ±10, ±10) off a (−20, 0, 0) offset | 2×2 sub-UV in RANDOM mode; 555–777 × 130–230 quads that only grow |
| `Wind_Speed` (244) | 0.05 | 0.6 | none at all | the only layer with a CONSTANT dissolve (−0.92719) and the only envelope with the short hold |
| `Add_Lines` (245) | — | 0.8–1.0 | cone launch + closed-form linear drag | the cookbook's only drag layer, and its only `Add Velocity in Cone` |

Three shapes are worth naming:

- **The spawn-phase split** is the [P2-D5] shape HealCast established: `CkParticles_SpawnPhase` /
  `CkParticles_IsBurstSpawn` classify the particle, a burst particle takes the 19-slot modulo, and a
  rate particle is always a line and dies unborn past 0.3 s.
- **The drag is closed form**, never stepped: `v(t) = v0·e^(−λt)` and `p(t) = spawn + v0·(1 − e^(−λt))/λ`
  with `λ = drag/mass`, both randoms per particle. §11 asserts that the same age evaluated at 1/120 s
  and at 1/15 s produces bit-identical position and velocity, which a step integration cannot do.
- **The cone launch** is the one place this port infers rather than transcribes — see §13.4.

## 10. Looks — three reuses proven value-by-value, one new

Reuse was established by comparing the shipped look's parameters against the corpus instance's, not by
name. All three matches are exact on every parameter the look plumbs:

| Source instance | Shipped look | Checked |
|---|---|---|
| `M_VFX_DisAdd_Wind01` | `WindDisAdd01` (CastLooks) | ShapeTex/DissolveTex `WindSheet`/`TileNoise`; Brightness 3; Dissolve_Speed (0, −0.15); Dissolve_Scale (1, 1); Distortion_Intensity 0.5; MainTex_Scale (1, 1); Boldness 1; Gradient_Invert 0 — every one matches §4's row |
| `M_VFX_DisAdd_Wind02` | `WindDisAdd02Mesh` (CastLooks) | `WindBandMid` both; Brightness 7; Dissolve_Speed (−0.1, 0); Dissolve_Scale (0.7, 0.95); Distortion_Intensity 1; Distortion_Speed (0.1, 0.1); Distortion_Scale 1; Distortion_Tex `TileNoise` — matches, including the mesh usage flag |
| `M_VFX_DisAdd_Part02` | `PartDisAdd02` (HitLooks) | `SoftParticleBright` both; Brightness 0.3 (the source's Brightness 1 × Glow_Intensity 0.3, the family's folding rule); Opacity_Boldness 0.5; Gradient_Invert 0.5 (the helper's default, and the source's value) — matches |

**New: `WindDisAdd03`** (`CkUsf_DashLooks_Assets.as`) — `M_VFX_DisAdd_Wind03`, the speed cone.
Brightness 2, Dissolve_Speed (−0.1, 0), Dissolve_Scale (3, 0.15), MainTex_Scale (4, 0.2),
Distortion_Intensity 0 with the source's Distortion_Scale 1 and `TileNoise` behind the dead branch,
`LutWhite` / displacement 0.1 / invert 0, mesh usage flag. It is the most heavily re-tiled instance in
the pack, and the five parameters it drives that the look does not plumb are §13.3.

A velocity-aligned quad and a camera-facing one are both `UNiagaraSpriteRendererProperties`, so
`PartDisAdd02`'s sprite usage flag serves this row unchanged — the mesh-vs-sprite split
`LightStripDisAddSprite` exists for does not arise here.

## 11. Tests

`Test_Particles_DashBehavior.cpp` (`CkTests.UnitTests.CkParticles.DashBehavior`) drives the CPU mirror
directly — no Niagara system, no RHI, no forked engine. It asserts:

- **the cadence row**: template path, `NAME_None` look binding, 2.0 / 1.55 / 19 / 50, no ribbon
  emitter, four renderers, two meshes by carrier name, one 2×2 sheet on the CAMERA-facing quad, one
  velocity-aligned quad, and the four look names (three reuses + the new one);
- **the burst partition**, read while each slot is ALIVE across the loop (the batch-H lesson):
  1 / 4 / 1 / 13 exactly, zero never-drawn, zero slots that change renderer mid-life, and the modulus
  holding past one period;
- **the spawn-phase split**, against its own opposite: every streamed particle is a line, every
  particle born past 0.3 s is hidden, and one seed proves both paths — it draws the TUBE when it
  bursts and a LINE when it streams;
- **the two beats**, each asserted hidden 1 ms before and alive 10 ms after, plus that a burst line
  carries no delay at all;
- **the tube's two-axis scale** (0.6 at the t = 0.2 knot on X/Y and holding; 3.25 on Z at the same
  instant, growing past it), its three colour constants, its 0.3 alpha plateau with both ends of the
  envelope at zero, its −0.36 dissolve at the knot, its −X travel and its advancing spin;
- **the cone's stillness** (position and velocity exactly zero at twenty ages), its constant
  −0.92719 dissolve, its non-uniform 0.8 / 0.8 / 3.0 scale at the knot, and the discriminator that
  matters: at the same normalized age its envelope has left the plateau (0.05 × 0.797544) while the
  wind's is still on it (0.3);
- **the smokes**: the exact (−20, 0, 0) spawn offset, −X travel, wide-not-tall quads, ±30° rotation,
  all four flipbook frames seen and every sampled particle advancing, the 0.2 alpha plateau, and the
  0.5 → 1 uniform growth curve pinned at 1.95× over the life;
- **the lines**: spawn inside the 50-unit sphere, every launch direction inside the 10° cone
  (worst observed dot 0.997388 against cos 5° = 0.996194), speed inside the falloff-scaled 350–750
  range with its upper reach exercised, drag that slows without bending, streaks four times longer
  than wide that shorten as they slow, the recovered colour ranges with BOTH ends of each channel
  reached to within 1 % of the channel's own span (the exported min sits ABOVE the max on R and G),
  the 0.7 alpha ceiling, and the 0.8–1.0 lifetime range with both ends reached;
- **DeltaTime independence** at 1/120 s vs 1/15 s, bit-identical — the closed-form drag proof;
- **anti-vacuity** per layer (light AND extent), and **death** past the row's 1.55 s on both paths.

`Test_Particles_RosterSanity.cpp` moves 46 → 47. The AS autotests
(`CkAutoTest_Particles_SpawnAllBehaviors`, `CkAutoTest_VfxExamples_PairStationsSpawn`) pick the id and
the pair up from `NumBehaviors` and `Get_Pairs()` without an edit.

## 12. Verification — A/B protocol `[HUMAN-VERIFY]`

VfxExamples gym, station pair **DASH** (`Gym.VfxExamples.Dash.Ck` / `.Original`), spawn offset
(0, 0, 60). `Ck_GymVfxExamples_RestartAll` re-fires both sides in sync. Judge in this order, and judge
from the SIDE — everything in this effect travels −X, so a head-on view hides its whole structure:

a. **The tube's stretch.** One long cylinder around the −X axis that grows lengthwise for a second and
   a half while its cross-section stops growing at 0.2 of its life. If it inflates uniformly, the
   two-axis scale collapsed into one.
b. **The tube's spin.** 0.3 turns per second about the travel axis — slow and continuous. Watch the
   dissolve pattern rotate rather than the silhouette.
c. **The speed cone.** A very faint (peak alpha 0.05) flared skirt that does NOT move, fading out
   noticeably earlier than the wind around it. If it drifts backwards with the tube, its
   no-velocity reading was wrong.
d. **The smokes.** Four very wide, short puffs sprayed backwards, each on its own flipbook frame,
   growing to twice their spawn size. Four is the whole population — if you see a stream, the burst
   and rate paths are crossed.
e. **The speed lines.** The dominant element: ~28 pale blue-white streaks per firing (13 at once, then
   a 0.3-second stream), each launched in a tight 10° cone down −X out of a 50-unit ball, visibly
   decelerating, and getting SHORTER as they slow. If they keep a constant length, the
   speed-driven size is not running.
f. **The stream's end.** The lines must stop arriving 0.3 s in, while the wind layers are still
   alive. A stream that runs the whole loop means the window gate is off.
g. **What NOT to judge:** the speed lines are WORLD-space in the source and LOCAL here — at a
   stationary pedestal the two are identical (§13.1), so any difference you see there is not this.
   Nor is `Wind_03`'s colour-sampler tiling (§13.3): expect the cone's streak pattern to differ in
   detail from the original's and do not chase it.

## 13. Confirmed fidelity differences or intentional deviations

1. **Mixed coordinate space — the C12 non-goal, and the sharpest case of it in the pack.**
   `Add_Lines` is `localSpace: false` where the three wind layers are `true`. A CkParticles template's
   `LocalSpace` is an emitter property, so one template cannot be both; all four layers are LOCAL.
   **At a stationary pedestal the two are identical** — a world-space emitter separates from a local
   one only when the system MOVES. On a moving dasher they do not: the source leaves its speed lines
   behind in the world while the wind rides along, and the recreation drags the lines with the actor.
   This is the one port where the C12 gap changes the effect's meaning rather than its look, and it is
   the reason to revisit C12 first if it is ever revisited. The same honesty as [P3-F5]: the pedestal
   A/B cannot show it, so it is stated rather than judged.
2. **`Add_Lines`' `Life Cycle Mode = Self / Loop Once` becomes a per-loop window.** In the source that
   emitter fires ONCE, ever, per activation; here its 0.3 s window is gated inside every 2.0 s loop,
   so it re-fires each loop. The gym's re-arm loop makes the two indistinguishable; a caller who lets
   the system loop sees the difference.
3. **Five `M_VFX_DissolveAdd` parameters the CkUsf look does not plumb**, all on `Wind03` except the
   last two: `MainTex_Offset_Y` 0.35; `Dissolve_Offset` (0.5, 0.41); the separate `Color_Tex` chain
   (`Color_Scale` (3, 0.15), `Color_Offset` (0.5, 0.41), `Color_Speed_X` −0.3) — **the one instance in
   the cookbook where folding `Color_Tex` into the shape is not a no-op**, because the two are tiled
   differently (4 × 0.2 vs 3 × 0.15); `Core_Intensity` (1 on `Wind01`, **20** on `Wind03`); and
   `Opacty_DepthFade` 20 on all four instances, the known CkUsf gap. `Color_Speed_X` −0.3 and
   `Distortion_Scale_Y` 0.6 are the same pre-existing gaps `NS_Arrow_Cast` records for `Wind02`.
   Closing them means extending `DissolveAdd.ush`, which this batch deliberately did not touch.
4. **`Add Velocity in Cone` is the pack's only use of that module, and its graph is not in the
   corpus.** The exporter dumps the inputs (`Cone Angle 10`, `Cone Axis (−1,0,0)`,
   `Velocity Distribution Along Cone Axis 0.75`, `Velocity Falloff Away From Cone Axis 0.333`,
   `Use Velocity Falloff On Cone Axis true`) but not the maths behind them, so the shipped reading is
   `[inferred]`: the polar draw is uniform in `cos θ` biased toward the axis by the distribution
   exponent, and the falloff scales the SPEED linearly from 1 on the axis to 1 − 0.333 at the rim.
   **The reach of the ambiguity is bounded by the aperture** — over a 10° cone every plausible reading
   moves a direction by at most five degrees; only the speed thinning is judgeable at A/B, and it is
   at most a third. Recorded here rather than guessed at silently, the `NS_DebuffCast` §13.2 shape.
5. **Both carriers collapse their wall thickness.** `SM_VFX_Ring01` is two coaxial walls 0.5 apart in
   radius (0.5 % of it) and `SM_VFX_Ring04` two cone sheets 0.5 apart in Z; both are built as single
   two-sided sheets, halving the triangle count and rendering identically — the `NS_BasicAttack` §13.5
   call, already made for the Cylinder by `NS_Arrow_Cast`.
6. **The speed lines' width is FLOORED at zero where the source's would go very slightly negative.**
   The source's `Scale Factor Curve` starts at −0.000593 and `Min Scale Factor.x` is 0, so a line
   slowed below |v| = 187 — which happens to every heavily-dragged line in its last third — earns a
   width of about −0.009 units. The roster's own contract forbids a negative quad extent (it flips
   winding rather than shrinking, and `Test_Particles_RosterSanity` asserts it across every behavior),
   so the width is clamped. The length axis never reaches the sign change and is untouched. The
   deviation is 0.009 units on a 30-unit quad, at the one moment the layer is nearly invisible anyway
   — but it IS a deviation from the authored curve rather than a transcription of it.
7. **[P4-D2] does not apply.** `NS_Dash` has no light renderer at all `[corpus]` — `renderers` is
   length 1 on every emitter, two mesh and two sprite. Recorded so a reader comparing the family does
   not go looking for the clause.
8. **`WindDisAdd01` and `PartDisAdd02` ship `DistortScale 0.1` against the family reference's 1.0** —
   the pre-existing batch-B adjacent finding, inert on both because their `Distortion_Intensity` is
   0 and 0.5-with-no-scale-dependence respectively. Untouched here: repointing a shared look to serve
   this port would have changed four other rows.

## 14. Reusable lessons

1. **A sheet written before a capability landed encodes a WORKAROUND, and shipping it would be a
   deliberate regression.** §6.1's burst 34 exists only because a row could not carry two spawn stacks
   in August's first week; C2 landed a phase later and the sheet's own §6.0 G6 names the better option
   it unlocked. Re-read a sheet's §6 against the CURRENT capability matrix, not against its own
   recommendation. `[P6-A1]`
2. **Re-apply [P0-D5] to any §6.1 that predates it.** "Max resolved lifetime" and "max over layers of
   (delay + lifetime)" differ by exactly the beat, and the beat is what gets clipped. `[P6-A2]`
3. **Match materials and textures by INSTANCE, not by name.** Three of this port's four looks were
   already carried under names (`WindDisAdd01`, `WindDisAdd02Mesh`, `PartDisAdd02`) that mention
   neither this effect nor its emitters, and all three of its "required" new bakes were already served
   by stand-ins measured for a system whose name shares nothing with it. The check that found them was
   a grep for the source ASSET path across the corpus. `[P6-A3]`, `[P6-A7]`
4. **The inverse check is just as valuable.** `grep -l SM_VFX_Ring04` and `grep -l DisAdd_Wind03` over
   the corpus each return exactly one file, which is how the two genuinely new assets were identified
   as new in one command rather than by exhausting the library.
5. **When a module has no other user in the pack, say so and bound the inference.** `Add Velocity in
   Cone` appears once in 30 systems; its graph is unexported. Stating the reading, its bound (a 10°
   aperture) and the one judgeable consequence (speed thinning) is worth more than a confident
   transcription that cannot be checked. §13.4
6. **A DeltaTime-independence assertion is cheap and it is the only real proof of a closed form.**
   Two evaluations of the same age at different frame rates, compared bit-for-bit, catch a step
   integration that no curve assertion would.
7. **A precedent can resolve an `[unresolved]` without new evidence.** §2.3's rotation units were
   settled by finding the identical parameter pair on `NS_Arrow_Cast`'s `Wind_01` — the same carrier,
   the same numbers, already shipped and gated. Check whether a neighbouring port has already answered
   the question before designing an experiment. `[P6-A4]`
