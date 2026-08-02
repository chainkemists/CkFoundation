# Translation sheet: NS_ExplosionIceOmni (Vefects Anime VFX)

Schema and evidence-tag conventions: [README.md](README.md). **Family reference sheets:
[NS_ExplosionGround.md](NS_ExplosionGround.md) (the batch reference) and
[NS_ExplosionOmni.md](NS_ExplosionOmni.md) (this system's structural twin) — read both first.** This
system is a **recolour of `NS_ExplosionOmni`**: identical emitter list, spawn shapes, materials,
meshes and cadence.

## Completion state — READ FIRST

**Status: PLANNED — TRANSLATION SHEET ONLY (2026-08-01). Nothing implemented.**

No behavior, no `.ush`, no look, no mesh, no texture, no cadence row, no test, no gym station. No
behavior id allocated. Nothing rendered or looked at.

**The §6 capability gaps are the SAME five as `NS_ExplosionGround.md` §6.0.** Not an S-tier port.

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_ExplosionIceOmni` |
| Pack | Vefects — *Anime VFX* |
| User parameters | **none** — `userParameters: []` `[corpus]` |
| Behavior id | **not allocated** |

Corpus evidence:

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_ExplosionIceOmni.{json,txt}`
- The same **ten** material JSONs, two mesh JSON/OBJ pairs and texture JSONs as
  [NS_ExplosionOmni.md](NS_ExplosionOmni.md) §1 (no `M_VFX_DisAdd_Star04` — no `Ground_Mark`
  emitter). **The asset dependency set is byte-for-byte the same as the Fire variant's.**

**The source Niagara asset was never opened.** Every fact below is `[corpus]` unless tagged otherwise.

> ### Ice reuses the Fire variant's assets unchanged
> `[corpus]` A full textual diff of `NS_ExplosionOmni.txt` against `NS_ExplosionIceOmni.txt` produces
> **zero** renderer, material, mesh, spawn-shape, count, spawn-time or module-structure differences.
> The differences are particle colour curves plus **five** other values (§5.0). Same finding as
> [NS_ExplosionIceGround.md](NS_ExplosionIceGround.md).

> ### Sibling-variant trap
> `[corpus]` `Vefects/Anime_Stylized_VFX/VFX/Particles/NS_Explosion_Ice_Omni` is a different,
> parameterized system. **Discriminator: the stylized sibling renders through `MI_VFX_*` instances and
> exposes `User.Glow Color 03` = RGBA(0.043735, 0.313989, 1, 1), `User.Light Color 01` =
> RGBA(0.0356013, 0.83077, 1, 1), `User.Rainbow Color 01`, `User.Ring Color 01`, … This target renders
> through `M_VFX_DisAdd_*` and has an EMPTY user-parameter list.**

> ### Naming skew — the light emitter is called `Light` here
> `[corpus]` Named **`Light`** in this system and in `NS_ExplosionGround`, but **`Glow_01001`** in
> `NS_ExplosionOmni` and `NS_ExplosionIceGround`. Byte-identical modules in all four. The naming does
> **not** track the Fire/Ice axis or the Ground/Omni axis — it is arbitrary. Do not key off it.

---

## 2. System anatomy `[corpus]`

**Identical to [NS_ExplosionOmni.md](NS_ExplosionOmni.md) §2 in every structural respect** — 15 CPU
emitters, all enabled, all WORLD space, bounds Dynamic, `determinism: false`, **65 particles per
firing** across the 14 non-ribbon emitters, every burst module carrying an inert
`Loop Count Limit = 1` under `UseLoopCountLimit = false`.

**System loop `[corpus-v3]`: `Loop Behavior = Once`, `Loop Duration = 2.0 s`, `Loop Delay = 0`,
`UseLoopDelay = false`, `Inactive Response = Complete`, `Recalculate Duration Each Loop = false`** —
identical to the other three explosion variants'. All 15 emitters are `Life Cycle Mode = System`, so
per [P0-D1] this rules and every per-emitter Loop row is inert.

Two lifetime values differ (both back to the Ground-variant value):

| Emitter | `NS_ExplosionOmni` | **NS_ExplosionIceOmni** |
|---|---|---|
| `Bubble_First_Explo` | lifetime 0.1 | **0.15** |
| `Spike01` | lifetime 0.1 | **0.15** |

Emitter #14 is named **`Light`** rather than `Glow_01001`. Everything else in the emitter table —
counts (1, 1, 7, 20, ribbon, 3, 5, 5, 1, 10, 2, 3, 1, 1, 5), spawn times, loop behaviours, renderers,
meshes, materials — is unchanged from `NS_ExplosionOmni.md` §2.

§2.1 transfers verbatim: the "omni" full-sphere spawns (`Hemisphere Z = false` on `Sparkles_02`,
`Sparkles_01`, `Smokes`, `Sparkles_02001`), `Sparkles_02`'s `Add Velocity from Point` rand 500–2000
plus `Acceleration Force (0, 0, −4000)`, `Smokes`' sphere r 50 with from-point rand 50–200,
`SmokesCenter`'s Sphere Location r 20 with non-uniform scale (1, 1, 0) and from-point rand 0–150,
`Spike01`'s isotropic orientation `Random Range Vector` min (−1,−1,−1) max (1,1,1),
`Sparkles_02001`'s Curl Noise Force (frequency 10, strength 5000, randomization (0.65, 0.125, 0.37)),
`Flames`' sphere r 50 with from-point rand 130–200.

§2.2 (event chain, `Generate Location Event` → the ribbon) also transfers verbatim, including its
**RESOLVED `[corpus-v3]`** handler contract: `LocationEvent` from `Sparkles_02`,
`executionMode = SpawnedParticles`, **1 particle per event**, unbounded events/frame,
`Receive Location Event` applying Position/Velocity/Acceleration/**Ribbon ID**; ribbon particle
lifetime **0.2 s** from `Initialize Ribbon`. *(Was `[unresolved: the event-handler stack is NOT
exported]`.)*

§2.3 (**randomized lifetimes — RESOLVED `[corpus-v3]`**) transfers verbatim from
[NS_ExplosionGround.md](NS_ExplosionGround.md) §2.3: `Sparkles_02`, `Sparkles_01`, `Smokes`,
`SmokesCenter`, `Sparkles_02001` and `Flames` each carry a `Lifetime Min / Max` pair AND an
`[override] Lifetime = dyn:Random Range Float` at **0.2 / 0.4**. Per [P0-D2]
`Lifetime Mode = Random` ⇒ **Min/Max drives and the override is inert**: `Sparkles_02` **0.4/0.7**,
`Sparkles_01` **0.2/0.4**, `Smokes` **0.7/1.3**, `SmokesCenter` **0.7/1.3**, `Sparkles_02001`
**0.4/0.7**, `Flames` **0.35/0.7**. *This sheet previously took the override as authoritative*, and
the correction **MOVES THE CADENCE ROW** — see §6.1.

**One inert export artefact worth naming so nobody chases it:** `[corpus]` this system exports
`Flare01.InitializeParticle.Mesh Uniform Scale = 1`, which the other three variants do not.
`Flare01` is a **sprite** emitter with `Mesh Scale Mode = Unset`, so the value cannot do anything. It
is an authored leftover, not a difference.

---

## 3. Mesh geometry

**Identical to [NS_ExplosionGround.md](NS_ExplosionGround.md) §3.** `SM_VFX_Sphere01`
(559 v / 960 t, radius-100 UV sphere, u longitudinal, v pole-to-pole +Z→−Z) and `SM_VFX_Spike01`
(16 v / 6 t, 4-sided pyramid, apex (0,0,200), base corners at XY radius 141.42, v = 0 at apex).

Same per-particle scaling: `Bubble_First_Explo` uniform 0.8; `Spike01` random non-uniform
min (0.2, 0.2, 0.5) / max (0.4, 0.4, 1.5).

---

## 4. Material family and per-instance deltas

**Identical to [NS_ExplosionOmni.md](NS_ExplosionOmni.md) §4** — the same ten materials at the same
parameter values: nine `M_VFX_DissolveAdd` instances (`Flames01`, `Flare01`, `Part01`, `Part04`,
`Rainbow`, `Ring01`, `Smoke01`, `Star01`, `Trail03`) plus the one `M_VFX_FlatAdd` instance
(`M_VFX_DisAdd_Flat02`). Full delta tables live in
[NS_ExplosionGround.md](NS_ExplosionGround.md) §4.

Texture set: Ground §4.3 minus `T_VFX_Star_04`. `T_VFX_LUT_Rainbow_01` (512×2, `TC_Default`, **sRGB**)
is still required by `Rainbow` and still has no procedural path.

**Nothing in the material layer is blue.**

---

## 5. Per-emitter runtime curves — EXACT keyframes `[corpus]`

All curves sample **NormalizedAge** over that emitter's own lifetime unless a `CurveIndex` says
otherwise. `C` = constant key, `L` = linear key. Verbatim.

### 5.0 Non-colour deltas vs `NS_ExplosionOmni` — the complete list `[corpus]`

| Emitter | Field | Omni | **IceOmni** |
|---|---|---|---|
| `Bubble_First_Explo` | `InitializeParticle.Lifetime` | 0.1 | **0.15** |
| `Spike01` | `InitializeParticle.Lifetime` | 0.1 | **0.15** |
| `Glow_03` | `InitializeParticle.Color` | RGBA(1, 0.153251, 0.0335489, 1) | **RGBA(0.043735, 0.313989, 1, 1)** |
| `Flames` | `Color.Scale Alpha` | 1 | **0.4** |
| `Flare01` | `InitializeParticle.Mesh Uniform Scale` | *(absent)* | **1** — inert (sprite emitter, `Mesh Scale Mode = Unset`) |

`Light`'s `InitializeParticle.Color` also changes — RGBA(1, 0.464488, 0.031026, 1) →
**RGBA(0.0356013, 0.83077, 1, 1)** — listed again in §5.14.

### 5.1–5.15 Colour curves

**Curves IDENTICAL to `NS_ExplosionOmni.md` §5 / `NS_ExplosionGround.md` §5** (transcribe from there):
every velocity-scale curve, every mesh-scale curve, every sprite-size curve, both smoke
dynamic-parameter curves, both rotation rates, and `Glow_03`'s / `Raimbow`'s / `Light`'s `Scale Color`
Vector4 curves. `Ring`'s `dissolve` curve stays at the Omni value **(0.5, −0.1)C (1, −1)C**.
`Glow_03`'s `Scale Color` keeps the Omni overdrive **(0, 5)L (1, 1)L**.

The `Color from Curve` overrides, in full:

1. **`Bubble_First_Explo`** — R (0, 0.036)C (1, 0.057)C | G (0, 0.828877)C (1, 0.298116)C | B (0, 1)C (1, 1)C | A (0, 1)C
   Mesh-scale curve unchanged: X, Y, Z all (0, 0)C (0.2, 1.5)C (1, 1)C
2. **`Flare01`** — R (0, 0.046665)C (1, 0.109462)C | G (0, 0.783538)C (1, 0.366253)C | B (0, 1)C (1, 1)C | A (0, 1)C (1, 0)L
3. **`Sparkles_02`** — R (0, 0.046665)C (1, 0.109462)C | G (0, 0.879623)C (1, 0.287441)C | B (0, 1)C (1, 1)C | A (0, 1)C
   **TWO keys per channel; the Fire variant has THREE** (its middle key at t = 0.416541 is gone).
   Velocity scale (0,1)C (0.2,0.3)C (1,3.91223e-08)C, rotation rate (0,720)C (1,0)C and the DISABLED
   `Scale Sprite Size 001` are unchanged.
4. **`Sparkles_01`** — R (0, 0.046665)C (1, 0.109462)C | G (0, 0.879623)C (1, 0.287441)C | B (0, 1)C (1, 1)C | A (0, 1)C
   Same two-key collapse. Size curves unchanged (uniform (0,0)C (0.1,1)C (1,0)C; non-uniform Y
   (0, 1)C (1, 0.6)C).
5. **`Sparkles_02_Trail`** — R (0.092967, 0.046665)C (1, 0.109462)C | G (0.092967, 0.879623)C (1, 0.287441)C | B (0.092967, 1)C (1, 1)C | A (0, 1)C (0.101419, 1)L (1, 0)L
   **`CurveIndex = linked:Emitter.Age` is ABSENT here**, as in `NS_ExplosionIceGround`. The Fire
   variants' ribbon samples its colour on *emitter* age; the Ice variants' sample on particle age.
   Ribbon-width curve unchanged: (0, 1)C (1, 0)C.
6. **`Smokes`** — R (0, 1)C (0.04588, **0.841345**)L (0.162994, 0.03434)L (0.433444, 0)L (0.591609, 0)L | G (0, 1)C (0.04588, **4.95551**)L (0.162994, 0.658375)L (0.433444, 0.318547)L (0.591609, 0)L | B (0, 1)C (0.04588, **5**)L (0.162994, 1)L (0.433444, 1)L (0.591609, 0)L | A (0.15575, 1)L (0.591609, 0.35)L
   **HDR — and this is the one place the Ice palette overdrives harder than the Fire one.**
   `NS_ExplosionIceGround`'s smoke stays inside [0,1]; here the cyan flash peaks at G 4.95551 / B 5.
   Velocity scale (0,1)C (1,0.2)C; `dissolve` (0.4, −2.46502e-08)C (1, −1)C; `core_color` (0, −1)C
   (0.3, 1)C; size (0, 0.5)C (0.2, 0.9)C (1, 1)C; rotation rate rand −30..30 — all unchanged.
7. **`SmokesCenter`** — byte-identical to `Smokes`' curve above (both HDR here).
8. **`Spike01`** — R (0, 0.036)C (1, 0.057)C | G (0, 0.828877)C (1, 0.298116)C | B (0, 1)C (1, 1)C | A (0, 1)C
   Mesh-scale curve unchanged, including the **two-key Z channel** X (0,0)C (0.2,1.5)C (1,4.17233e-08)C |
   Y (0,0)C (0.2,1.5)C (1,5.66244e-08)C | Z (0,0)C (0.2,1.5)C.
9. **`Ring`** — R (0, 1)C (0.165409, 0.147027)L (1, 0.040915)L | G (0, 1)C (0.165409, 0.89627)L (1, 0.318547)L | B (0, 1)C (0.165409, 1)L (1, 1)L | A (0, 1)C
   `dissolve` **(0.5, −0.1)C (1, −1)C**; size (0, 0.5)C (0.5, 0.975)C (1, 1)C; size 400 uniform.
10. **`Sparkles_02001`** — R (0, 0.046665)C (1, 0.109462)C | G (0, 0.879623)C (1, 0.287441)C | B (0, 1)C (1, 1)C | A (0, 1)C
    Size curves unchanged (non-uniform Y (0.5, 1)C (1, 0.4)C).
11. **`Glow_03`** — no `Color` module; `Scale Color` Vector4 R, G, B all **(0, 5)L (1, 1)L** | A (0, 1)L (1, 0)L, under `ScaleColor.Scale RGB = (100, 100, 100)`. Blue via `InitializeParticle.Color` (§5.0). Size 1300 uniform.
12. **`Glow_04`** — R (0, 0.046665)C (1, 0.109462)C | G (0, 1)C (1, 0.434154)C | B (0, 1)C (1, 1)C | A (0, 1)C (1, 0)L
    **Note: NOT the Omni variant's HDR start (0, 3)C / (0, 2.12513)C** — the Ice palette drops it back
    inside [0, 1]. 3 particles, size 800 uniform.
13. **`Raimbow`** — unchanged: no `Color` override; `Scale Color` Vector4 R, G, B each a single key (0, 0.5)L | A (0, 1)L (1, 0)L; size curve (0, 0.5)C (0.2, 0.9)C (1, 1)L; size 600 uniform, spawn t 0.05.
14. **`Light`** — unchanged curves: Vector4 R, G, B (0, 1)L (1, 1)L | A (0, 1)L (1, 0)L, plus `Scale Alpha` float (0, 1)L (1, 0)L, under `ScaleColor.Scale RGB = (1e+06, 1e+06, 1e+06)`. `InitializeParticle.Color` = **RGBA(0.0356013, 0.83077, 1, 1)**; size 9.16604 uniform; light renderer RadiusScale 10.
15. **`Flames`** — R (0.254754, 0.168269)L (0.475702, **0.1717**)L (0.858436, 0)L | G (0.254754, 0.991102)L (0.475702, **3.29188**)L (0.858436, 0.318547)L | B (0.254754, 1)L (0.475702, **5**)L (0.858436, 1)L | A (0, 0)L (0.482946, 1)L (1, 0)L
    **HDR mid key**, unlike `NS_ExplosionIceGround`'s flames (which stay inside [0,1] at the same key
    times). `Color.Scale Alpha` **0.4** (§5.0).
    Dynamic params unchanged: `dissolve` (0, −2.46502e-08)C (1, −1)C, `distortion` = **5** constant,
    `Param3WriteEnabled = true`. Velocity scale (0,1)C (1,0.2)C. Size curve (0, 0.5)C (0.2, 0.9)C
    (1, 1)C. Rotation rate rand −30..30. SubUV 2×2, mode Random, frames 0–3.

---

## 6. Translation plan (CkParticles / CkUsf)

### 6.0 Capability-gap callout

**The same five gaps as [NS_ExplosionGround.md](NS_ExplosionGround.md) §6.0, all present here
unchanged** — G1 ribbon renderer (`Sparkles_02_Trail` + `M_VFX_DisAdd_Trail03` + `Scale Ribbon
Width`), G2 light renderer (`Light`, RadiusScale 10), G3 sub-UV flipbook (`Flames`, 2×2, mode Random,
frames 0–3), G4 event generation → event-handler spawn (**now a pure capability gap** — the handler
stack IS exported `[corpus-v3]`, §2.2), and ~~G5 per-emitter cadence divergence~~ which
`[corpus-v3]` is **NOT a gap**: every emitter is `Life Cycle Mode = System`, so the stored
1.0 Infinite / 0.3 / 0.4 Once rows are all inert and the system's `Once / 2.0 s` governs uniformly.
Longest particle lifetime is **1.3 s** (§2.3 resolved), not 0.4 s.

Ground §6.0's loop-authority item applies verbatim and is likewise **RESOLVED `[corpus-v3]`**: the
system's own rows are `Loop Once / 2.0 s`.

`Curl Noise Force` and `Acceleration Force` remain **work, not gaps**.

### 6.1 Cadence row

**IDENTICAL to `NS_ExplosionOmni` §6.1** — per [P0-D3]: **2.0 s loop** `[corpus-v3]` (*was 1.0 s*),
**65** burst, same `Seed % 65` layer partition and the same layer-index ranges
(0 `Bubble`, 1 `Flare01`, 2–8 `Sparkles_02`, 9–28 `Sparkles_01`, 29–31 `Smokes`, 32–36 `SmokesCenter`,
37–41 `Spike01`, 42 `Ring`, 43–52 `Sparkles_02001`, 53–54 `Glow_03`, 55–57 `Glow_04`, 58 `Raimbow`,
59 `Light`, 60–64 `Flames`). **One row serves both Omni systems.**

`ParticleLifetime` is **1.3 s** `[corpus-v3]` — `Smokes` / `SmokesCenter`'s resolved `Lifetime Max`
(max resolved lifetime, per [P0-D3]). *Was `[unresolved]` with 0.4 s as this sheet's working
reading.* The two 0.1 → 0.15 direct-set bumps on `Bubble_First_Explo` and `Spike01` (§5.0) do not
reach that figure and so do not affect the row.

### 6.2–6.5 Renderer, mesh, look and texture needs

**IDENTICAL to `NS_ExplosionOmni` §6.2–6.5.** Ten distinct materials ⇒ ~10 row-declared renderers; the
same two new renderer kinds (`CameraFacingSprite`, `CustomFacingSprite`, each with an explicitly bound
look); the same two procedural carriers; the same nine `DissolveAdd` parameterizations plus the new
`FlatAdd` family look; the same three new `DissolveAdd` family parameters; the same texture set with
`T_VFX_LUT_Rainbow_01` unserviceable.

**Nothing in §6.2–6.5 is duplicated by porting this variant** — the whole asset set is shared with
`NS_ExplosionOmni`.

### 6.6 Behavior id — the family decision

**Do NOT allocate an id in this document.**

Same argument as [NS_ExplosionIceGround.md](NS_ExplosionIceGround.md) §6.6: this system is
`NS_ExplosionOmni` with colour curves and **five** other values changed, three of which are two
lifetimes and an inert leftover. One behavior with a colour-palette branch expresses that; two ids
duplicate a `.ush` and its C++ mirror for a palette.

Note the palette is **not a uniform hue rotation** — the Ice variants are not "the Fire curves with
R and B swapped." `Sparkles_02` and `Sparkles_01` lose a keyframe; `Glow_04` loses its HDR start
while `Smokes`/`SmokesCenter`/`Flames` gain one; the ribbon loses its `Emitter.Age` curve binding;
`Ring`'s dissolve sign differs by Ground-vs-Omni rather than by Fire-vs-Ice. **A palette branch must
carry per-layer key TABLES, not a hue transform** — that is the specific thing to get right, and the
sheets in this batch carry every key needed for it.

### 6.7 Known deviations already implied

Same as Ground §6.7 — world→local space, `Opacty_DepthFade` dropped. HDR values up to **5** on
`Smokes` / `SmokesCenter` / `Flames`, plus the ×100 (`Glow_03`) and ×1e6 (`Light`) `Scale RGB`
multipliers, must survive the colour path unclamped.

---

## 7+. Reserved for implementation.
