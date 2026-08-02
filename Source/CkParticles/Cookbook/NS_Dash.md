# Translation sheet: NS_Dash (Vefects Anime VFX)

Schema and evidence-tag conventions: [README.md](README.md). Exemplars: [NS_BasicAttack.md](NS_BasicAttack.md),
[NS_Lightning_Range.md](NS_Lightning_Range.md).

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior, no `.ush`, no look, no mesh, no texture, no cadence row, no test, no gym station exists
for this effect. No behavior id is allocated. Nothing has been rendered or looked at. Sections 1–6 are
archaeology and a plan; everything in them comes from the extracted corpus and is tagged `[corpus]`.

**This is the only system in the batch with a MIXED coordinate space and a MIXED spawn mode.** Three
emitters are local-space and one is world-space; one emitter carries a burst **and** a continuous
spawn rate **and** its own `Self` life cycle. Both facts are load-bearing for a dash effect and both
are §6 capability items. Read §6.0 before scheduling.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Dash` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| User parameters | **none** — `userParameters: []` `[corpus]` |
| Behavior id | **not allocated** — take the next free id at implementation time from `ck::particles::NumBehaviors` |

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

**`[unresolved: the units of `Rotation` and `Rotation Rate`]`** — `Rotation (0, 0.25, 0)` reads as a
quarter turn about Y if the field is in revolutions and as 0.25° if it is in degrees. Niagara's
`Initial/Update Mesh Orientation` modules take **rotations** (1.0 = a full turn) for both
`[inferred]`, which makes 0.25 and 0.75 a quarter and three-quarter turn — a plausible pair for
"same mesh, rolled differently". Confirm before writing the quaternion.

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

## 6. Translation plan (CkParticles / CkUsf)

### 6.0 Capability-gap callout — READ BEFORE SCHEDULING

**No ribbon, no light renderer, no events, no collision, no GPU sim, no user parameters** — the three
heaviest explosion-family gaps are all absent. What remains:

| # | Gap | Why the pipeline can't express it | Cheapest honest options |
|---|---|---|---|
| **G3** | **Sub-UV flipbook** — `Wind_Smokes` renders `SubUV: 2x2` with `Sub UVAnimation` in mode **Random**, frames 0–3 | No `SubImageIndex` output on `FCkParticles_StageOutput`; the shared sprite renderers declare no `SubImageSize`; no CkUsf look samples a flipbook atlas | (a) Bake a single smoke texture and drop the flipbook — visible as less per-particle variety across only 4 particles; (b) add `SubImageIndex` to the DI contract + `SubImageSize` on the renderer spec + atlas UV maths in the look. **Shared with `NS_Fire`** — do it once for both |
| **G6** | **Burst AND continuous rate in one emitter** — `Add_Lines` runs `Spawn Burst Instantaneous` (13 at t = 0) **and** `Spawn Rate` (50 /s) simultaneously | `FCk_ParticlesTemplateSpec` has ONE `BurstCount`; `BurstCount 0` selects the continuous spawn-rate stack, any other value selects the burst stack. **The template cannot do both** | (a) Approximate: burst `13 + round(50 × 0.3) = 28` at t = 0 and stagger the extra 15 slots' effective spawn times inside the behavior via a per-slot delay (the `NS_BasicAttack.md` §5 spawn-delay technique generalized). Cheap, and visually close since the rate runs for only 0.3 s. (b) Extend the template builder to emit both stacks on one row. **(a) is the honest recommendation** — but it IS an approximation and belongs in the fidelity-gap section, not hidden |
| **G7** | **Mixed coordinate space** — three local-space emitters + one WORLD-space emitter, in one system | A CkParticles template is a single local-space emitter. Both prior recreations recorded local-vs-world as a deviation (`NS_BasicAttack.md` §13.2), but there the whole source was world-space, so the deviation was uniform | This one is **not** uniform and it is **not** cosmetic: `Add_Lines` being world-space is what makes speed lines stay behind a moving dasher while the wind shapes ride with it. Forcing everything local makes the lines travel WITH the actor — the opposite of the intended read. Options: (a) accept it and record it prominently; (b) spawn two components (one local for the wind, one world for the lines) from one call; (c) have the behavior write world-relative positions for the line layer using the component's own transform, which the DI does not currently expose. **Decide before implementing — this is the fidelity risk of this port.** |
| **G8** | **Emitter-level `Life Cycle Mode = Self`** — `Add_Lines` runs its own `Once` / 0.3 s loop while the other three run the system's | The cadence row has one loop duration for the whole template | Fold into the single 1.0 s loop and give the line layer a 0.3 s window inside it (hide past `age > 0.3`). Mechanically the same as the per-layer-lifetime treatment; call it out because "Once" means it does **not** repeat every loop in the source, and a naive port will re-fire it every second |

Items that are **work, not gaps**:

- **Animated mesh orientation** (`Update Mesh Orientation`, rates 0.3 and 0.05 about +X) — the
  behavior writes a time-varying `O.Orientation` quaternion. Straightforward, but see §2.3's
  `[unresolved]` on the rotation units.
- **Renderer-level mesh scale (1, 1, 5)** on both mesh emitters — `FCk_ParticlesRendererSpec` has no
  mesh-scale field, but the multiplier folds into the behavior's `O.Scale` exactly. Fold it; do not
  add a field for it.
- **Linear drag with random per-particle drag and mass** (`Add_Lines`) — first-order drag integrates
  in closed form (`v(t) = v0·exp(−k·t/m)`), so both position and velocity stay exact and GPU/CPU
  lockstep is preserved. Do **not** step-integrate it (`NS_BasicAttack.md` §14.7).
- **Speed-dependent sprite size** (`Scale Sprite Size by Speed`) — the behavior already computes
  velocity in closed form, so the size scale is a direct evaluation of §5's 3-key curve at
  `|v| / 1500`.

**Loop authority — RESOLVED `[corpus-v3]`.** `Wind_01`, `Wind_Smokes` and `Wind_Speed` are
`Life Cycle Mode = System`, so per [P0-D1] the system's `Loop Once / 2.0 s` drives them and their
emitter-local `Infinite / 1.0 s` rows are inert. `Add_Lines` (`Self`, `Once`, 0.3 s) genuinely does
run its own — which is why the distinction matters here. *(Was `[unresolved]`; the working figure
was the emitters' 1.0 s.)*

### 6.1 Cadence row

**A new row is required.**

```
{ TEXT("PS_CkParticles_Template_Dash"), 2.0f, 1.5f, 34, Get_DashRendererSpecs() }
```

Per [P0-D3]: loop = the system loop duration, lifetime = max resolved lifetime, burst = §2 counts.

- `LoopDuration` **2.0** `[corpus-v3]` — the system's `Once` loop duration. *Was 1.0, taken from the
  three wind emitters' inert Loop rows.*
- `ParticleLifetime` **1.5** — `Wind_01`, the longest-lived layer (max resolved). Every shorter layer
  writes zero colour, zero size and zero scale past its own lifetime (`NS_BasicAttack.md` §8).
  **No lifetime ambiguity in this system, confirmed `[corpus-v3]`** — `Add_Lines` is the only
  randomized-lifetime emitter, `Lifetime Mode = Random` with no override, so its 0.8–1.0 range drives
  (§5.4).
- `BurstCount` **34** = 1 (`Wind_01`) + 4 (`Wind_Smokes`) + 1 (`Wind_Speed`) + 28 (`Add_Lines`:
  13 burst + ~15 from the 50 /s rate over its 0.3 s window, per §6.0 G6). Layer index = `Seed % 34`
  (double-modulo): 0 `Wind_01`, 1–4 `Wind_Smokes`, 5 `Wind_Speed`, 6–33 `Add_Lines`.
  **The 28 is the approximation** — record it in the fidelity-gap section, do not present it as the
  source's number.
- **Spawn-time offsets are layer state**: `Wind_01` and `Wind_Speed` at 0.05, `Wind_Smokes` at 0.1,
  `Add_Lines` slots 6–18 at 0 and slots 19–33 staggered across 0 .. 0.3 s. Each hides
  (colour/size/scale 0) before its delay and runs its curves on `(age − delay) / lifetime`.

### 6.2 Renderer / VisTag needs

Four distinct materials across four emitters, so **four row-declared renderers**
(`NS_BasicAttack.md` §8.1 — one `User.SpriteMaterial` cannot carry four materials):

| Row renderer | Kind | Mesh | Look | Source emitter |
|---|---|---|---|---|
| 1 | **`Mesh`** (exists) | `Cylinder` (new, §6.3) | `WindDisAdd02` (new) | `Wind_01` |
| 2 | **`CameraFacingSprite`** (NEW KIND) | — | `WindDisAdd01` (new) | `Wind_Smokes` |
| 3 | **`Mesh`** (exists) | `Cone` (new, §6.3) | `WindDisAdd03` (new) | `Wind_Speed` |
| 4 | **`VelocityAlignedSprite`** (exists) | — | `PartDisAdd02` (new) | `Add_Lines` |

**One new renderer kind is required**: `CameraFacingSprite` with an explicitly bound look. Same
requirement as `NS_Fire` §6.2 and the explosion family — do it once.

Name collision to avoid: **behavior 7 already ships a look called `WindDisAdd02`**
(`NS_BasicAttack.md` §9, a parameterization of `M_VFX_DisAdd_Pan_Wind02`). This system's `Wind_01`
draws `M_VFX_DisAdd_Wind02`, a **different material**. Pick a distinct look name — the existing one is
bound to behavior 7's row renderers and must not be repointed.

VisTags: four, allocated above `Get_RosterVisTag_Max()` (9 as of 2026-08-01). Read the ceiling from
`Get_RosterVisTag_Max()`; never restate a literal (`NS_BasicAttack.md` §14.4).

### 6.3 Mesh needs

Two procedural carriers, generated from §3's measurements — never imported:

- **`SM_CkParticles_Cylinder`** — open tube, radius 1, height 1 (scaled by the behavior), wall
  thickness 0.005 of the radius (or a single sheet with `_TwoSided`, the `NS_BasicAttack.md` §13.5
  precedent), 128 triangles. **UV: `v` along the axis, v = 0 at the TOP, v = 1 at the BOTTOM;
  `u` around the circumference with the seam at −Y.** The v direction is load-bearing — the dissolve
  and the panning `Color_Speed_X` run along it.
  **Check the existing `Tube` carrier first**; if it already matches, this costs a UV assertion rather
  than a new mesh.
- **`SM_CkParticles_Cone`** — truncated cone, narrow-end radius **0.5396** and wide-end radius **1.0**
  over a height of **0.328** (§3's 53.96 / 100 / 32.30 normalized), open at both ends, 128 triangles.
  **UV: v = 0 at the NARROW end, v = 1 at the WIDE end; u around the circumference, seam at −Y.**

### 6.4 Look needs

Four new `DissolveAdd` parameterizations (names must not collide with behavior 7's — §6.2):

| Look | ShapeTex | DissolveTex | Brightness | Notable, from §4 |
|---|---|---|---|---|
| `DashWindDisAdd01` | (new Wind01 bake) | TileNoise | **3** | `DissolveSpeed` (0, −0.15); `DistortIntensity` **0.5**; `CoreIntensity` **1** |
| `DashWindDisAdd02` | (new Wind02 bake) | (same) | **7** | `DissolveScale` (0.7, 0.95); `DissolveSpeed` (−0.1, 0); `DistortScale_Y` 0.6; `DistortSpeed` (0.1, 0.1); `Color_Speed_X` **−0.3**; `DistortIntensity` **1** |
| `DashWindDisAdd03` | (new Wind02 bake) | (same) | **2** | `MainTexScale` (**4, 0.2**), `MainTexOffset_Y` 0.35; `ColorScale` (3, 0.15) + `ColorOffset` (0.5, 0.41) + `Color_Speed_X` −0.3; `DissolveScale` (3, 0.15) + `DissolveOffset` (0.5, 0.41); `CoreIntensity` **20** |
| `DashPartDisAdd02` | (new Part02 bake) | (same) | **1** | `DistortIntensity` 0; `GlowIntensity` **0.3**; `OpacityBoldness` **0.5** |

The family entry point needs **five new parameters** to serve these faithfully: `CoreIntensity`,
`GlowIntensity`, `MainTexOffset`, `DissolveOffset`, and a **separate `ColorTex` sampler with its own
scale/offset/speed**. The last is the significant one: the current look folds `Main_Tex` and
`Color_Tex` into one `ShapeTex`, which is a no-op on every prior recreation but **not** on `Wind03`,
where the two are tiled differently (4, 0.2 vs 3, 0.15). All five are inert on existing looks, so the
extension follows the `NS_BasicAttack.md` §9 precedent: extend the shared shader, then prove every
existing look regenerates unchanged.

### 6.5 Texture needs

Three new measurement-driven bakes (§4.1): stand-ins for `T_VFX_Wind_01` (shared with `NS_Fire`),
`T_VFX_Wind_02` (drives both mesh emitters — the most important one) and `T_VFX_Part_02`
(clamp-addressed, so a single centred feature). `T_VFX_Noise_02` is already served by
`T_CkParticles_TileNoise`. Parameterize from measurements of the corpus PNGs the way
`NS_BasicAttack.md` §7 did — derive numbers, never copy pixels.

### 6.6 Behavior id

**Do NOT allocate an id in this document.** At implementation time take the next free id from
`ck::particles::NumBehaviors` (18 as of 2026-08-01, ids 0..17) and bump it.

**Aim-axis convention to declare**: this effect's forward is **−X** (§2.1 — the constant velocity, the
random box, the cone axis and the position offset all point −X, and both carriers are oriented onto
+X). The roster's existing conventions are MuzzleFlash/Tracer/Beam forward = **+X**
(`CkParticles/CLAUDE.md`). Either negate into the roster convention (+X forward, effect trails −X — the
natural reading for "dash forward") or document the exception. **Do not leave it undeclared** — an
undeclared aim axis is a defect that only shows up when a caller passes a rotation.

### 6.7 Known deviations already implied

- **Mixed coordinate space collapses to local** — see §6.0 G7. **This is the largest fidelity risk in
  this port** and it is not the usual cosmetic local-vs-world note: it inverts the speed lines'
  behaviour relative to a moving actor.
- **`Add_Lines`' continuous 50 /s spawn is approximated as 15 staggered burst slots** (§6.0 G6, §6.1).
- **`Add_Lines`' `Once` life cycle becomes a per-loop 0.3 s window** (§6.0 G8) — in the source it
  fires once, in the recreation it re-fires every loop.
- **`Opacty_DepthFade` (20 on all four materials) is dropped** — CkUsf surface looks do not wire
  scene depth.
- **`Color_Tex`'s independent tiling** is lost unless §6.4's `ColorTex` parameter is added; on
  `Wind03` that is a visible difference, not a no-op.
- **Mesh wall thickness** — `SM_VFX_Ring01`'s 0.5-unit wall on a radius-100 tube is 0.5%. Building it
  as a single `_TwoSided` sheet renders identically and halves the triangle count, the same call
  `NS_BasicAttack.md` §13.5 made for the crescent.

---

## 7+. Reserved for implementation.
