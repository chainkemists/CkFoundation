# Recipe: NS_Lightning_Range → `LightningRange` (BehaviorId 17)

Schema and evidence-tag conventions: [README.md](README.md).

---

## Completion state — READ FIRST

**Every authored piece exists, the template is generated and proven non-inert, and the tests now compile
and run. NOTHING has been rendered or visually compared.**

| Piece | State |
|---|---|
| Behavior `.ush` + CPU mirror | authored |
| CkUsf look + Niagara sprite contract | authored; the `RingDissolveAdd` master is generated on disk |
| Runtime binding, cadence row, imported textures | authored; both `/Game/Vefects` textures imported into plugin content |
| VfxExamples gym pair (`Gym.VfxExamples.LightningRange.*`) | authored; **compiled and exercised** — the pair (recreation + original side by side) runs under `CkAutoTest_VfxExamples_PairStationsSpawn`, and the roster loop in `Ck_AutoTest_Particles_SpawnAllBehaviors` covers id 17. Never driven interactively by a human. (The station moved out of the Particles gym 2026-08-01 — faithful ports live only in VfxExamples) |
| `PS_CkParticles_Template_Single` asset | **GENERATED** (2026-08-01) and verified non-inert — see below |
| The four §12 tests | **COMPILED AND RUN** (2026-08-01) — see *Test results* |
| Anything rendered or visually compared | **not done** |

### The template is generated and non-inert — verified 2026-08-01

Regenerated headlessly on a fork-enabled engine (`CK_WITH_PARTICLES=1`) via
`CK_PARTICLES_REBUILD_TEMPLATES=1` + `--test --test-pattern RebuildTemplateAssets --no-nullrhi`.
`Ck.Particles.RebuildTemplateAssets` passed (1/1), which is also the proof the fork is active:
`Build_AllTemplateSystems` REFUSES under `CK_WITH_PARTICLES=0`, so a green run cannot happen without it.

Inertness check — `grep -ac ExecuteStage <template>.uasset`, expect non-zero, **never 0**:

| Template | Count | State |
|---|---|---|
| `PS_CkParticles_Template` | **39** | regenerated in place (already committed) |
| `PS_CkParticles_Template_Burst` | **39** | regenerated in place (already committed) |
| `PS_CkParticles_Template_Single` | **39** | **NEW** — untracked, awaiting commit |

> Older wording here and in `CkParticles/CLAUDE.md` said "expect ~35". The current builder emits **39**.
> The number is not the contract — *non-zero* is. Do not treat 35 as a target.

Regeneration is idempotent-in-place across the WHOLE asset pipeline, not just the templates, so the run
also rewrote the procedural textures, the VFX master + per-texture material instances, and the four
carrier meshes. Expect ~26 modified `.uasset` files plus the one new template in the diff — that is
normal for any `Create Template System` run, not evidence that something else changed.

### Test results — first compile and first run, 2026-08-01

The dev-merge that used to head this section is **done**: `feature/particles-cookbook` is rebased onto
`origin/dev`, CkTests and CkGameplayDebugger compile again, and the four §12 tests have now been built
and executed on the fork-enabled engine.

Lane of record — `--test --test-pattern Particles --no-nullrhi --discover-fresh --parallel 1`:
**7 total, 7 passed, 0 failed** (34 s).

| Test | Result |
|---|---|
| `CkTests.UnitTests.CkParticles.LightningRangeBehavior` | **pass** |
| `CkTests.UnitTests.CkParticles.RosterSanity` | **pass** |
| `CkTests.UnitTests.CkParticles.LightningRangeAuthoring` | **pass** — RAN, did not skip (`CK_WITH_PARTICLES=1`) |
| `Ck_AutoTest_Particles_SpawnAllBehaviors` (roster now covers id 17) | **pass** |
| `Ck.Particles.RebuildTemplateAssets` | **pass** |

`CkTests.UnitTests.CkUsf.NiagaraSpriteContract` lives in the CkUsf lane —
`--test --test-pattern CkUsf --no-nullrhi --discover-fresh --parallel 1`: **4 total, 4 passed, 0 failed**
(50 s). It RAN rather than self-skipping, and `GeneratesUsableMasters` passed alongside it, which is what
proves the `RingDissolveAdd` shader actually compiles.

Two things the first run cost, both recorded so the next recreation does not pay them again:

- **CkTests needed a direct `Niagara` dependency.** The authoring gate loads a `UNiagaraSystem`, and
  inheriting Niagara transitively through `CkParticles` did not put its import lib on the link
  (`LNK2019` on `Z_Construct_UClass_UNiagaraSystem_NoRegister`).
- **`GetUserParameters` returns the redirection store's SHORTENED keys** (`BehaviorId`), not the
  `User.`-qualified names the template is authored with. Comparing the raw keys matches nothing and the
  gate reds against a perfectly good asset; `FNiagaraUserRedirectionParameterStore::MakeUserVariable` is
  the engine's own normaliser back.

Test-lane pattern note: `--test-pattern CkParticles` silently misses two of the five rows —
`Ck.Particles.RebuildTemplateAssets` and `Ck_AutoTest_Particles_SpawnAllBehaviors` do not contain that
substring. Use `Particles`, as §12 already prescribes.

Run these **serially** (`--parallel 1`). Under the auto-sized 4-lane default the concurrent editors
contend for one project's `Saved/`, and the AutoTest harness escalates the resulting stray
`LogZenServiceInstance` / `LogFileInfo` engine Errors into failures of whatever test is in flight — reds
with no assertion behind them.

### To finish

1. Run the VfxExamples gym pair (`Gym.VfxExamples.LightningRange.*`) interactively once. Its code path
   is exercised by the autotests, but no human has driven the station.
2. Commit the generated `PS_CkParticles_Template_Single.uasset` and the regenerated pipeline assets.
3. Execute the `[EDITOR-VERIFY]` gate in §12 and fill in §13 from what is actually observed.

Until step 3, **no fidelity claim in this document has been confirmed by looking at the effect.**

---

## 1. Source system and provenance

| | |
|---|---|
| Source object | `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Lightning_Range` |
| Pack | Vefects — *Anime VFX* (third-party marketplace content) |
| Renderer material | `/Game/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_Ring04` |
| Behavior | `LightningRange` = **17** |
| CkUsf look | `RingDissolveAdd` |

Corpus evidence (regenerate per [README.md](README.md); `Saved/` is machine-local):

- `systems/Vefects/Anime_VFX/Shared/Skills/NS_Lightning_Range.json` / `.txt`
- `materials/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_Ring04.json`
- `textures/Vefects/Anime_VFX/Shared/Textures/T_VFX_{Ring_04,Noise_04,Noise_02}.{png,json}`

**The source Niagara asset was never opened in the Niagara editor.** Every fact below is `[corpus]`
unless tagged otherwise.

> ### Two systems share this name — take the right one
> `[corpus]` The pack ships a second `NS_Lightning_Range` at
> `Vefects/Anime_Stylized_VFX/VFX/Particles/`. It is a **different, parameterized** system: it adds
> `User.Color 01` / `User.Lifetime` / `User.Scale`, drives size through a `Multiply Float`, adds a
> `Sphere Location` module, and links its colour curve to the user param instead of inlining it.
> It also renders through a **different material**, `Anime_Stylized_VFX/Shared/Materials/MI_VFX_Ring_04`
> — the fastest one-line discriminator `[corpus, re-verified 2026-08-01]`.
> This recipe recreates the **`Anime_VFX/Shared/Skills`** one, which is the fixed target and the
> variant with no user parameters.

> ### Environment trap that invalidates material archaeology
> `[corpus]` On this dev host the pack **had been** copied one level too deep
> (`Content/Vefects/Vefects/Anime_VFX/…`), so every intra-pack reference dangled. The symptoms were
> **silent and misleading**, not loud: the system's renderer exported as `material: "<none>"`, the
> corpus produced no `materials/` or `textures/` trees at all, and a direct material export reported
> `BLEND_Opaque` + `MSM_DefaultLit` — plausible-looking values that are simply
> the fallbacks a material instance returns when its parent fails to load. Ground truth
> (`BLEND_Translucent`, `MSM_Unlit`) was only visible in the `.uasset` name table.
>
> **Before trusting any material read, confirm the renderer material is non-null.** A `<none>`
> renderer material on a system that visibly has one means the mount is wrong, not that the material
> is absent.
>
> #### Mount FIXED and archaeology RE-VERIFIED against a healthy export — 2026-08-01
> The pack now sits at `Content/Vefects/Anime_VFX/…` with no double-nesting. The corpus was
> regenerated from scratch (`Ck.AssetExporter.ExportVfxCorpus`, 1 total / 1 passed, 28 s, zero
> engine Errors, zero ensures, zero AngelScript errors): **62 systems, 81 materials, 72 textures,
> 20 meshes, 0 failures.** The renderer now exports
> `material: /Game/Vefects/Anime_VFX/Shared/Materials/M_VFX_DisAdd_Ring04.M_VFX_DisAdd_Ring04`, and
> both `materials/` and `textures/` trees exist with `M_VFX_DisAdd_Ring04.json`,
> `T_VFX_Ring_04.{png,json}` and `T_VFX_Noise_04.{png,json}` in them.
>
> **Every `[corpus]` fact in §3–§7 re-read against the clean export and every one matched, unchanged**
> — one enabled CPU sprite emitter, local space, empty user-parameter list, `Spawn Burst
> Instantaneous` count 1 at t=0, Infinite/Fixed 1.0 s loop, lifetime 1.1 s, Uniform sprite size 700,
> `CustomAlignment` + `CustomFacingVector`, the dissolve ramp `(0, −1)L → (0.4, 0.2)L`, the colour
> snap at `0.0630213` with all six R/G/B key values, the three alpha keys, the dynamic-param channel
> names, the full six-entry `textureParams` set, and the material's `MD_Surface` /
> `BLEND_Translucent` / `MSM_Unlit` / `twoSided: false` / `EmissiveColor`+`Opacity` /
> `Brightness 30` / `Distortion_Intensity 0` / `core_color −0.5` / `Opacty_DepthFade 10` /
> `GradientMap_Tex = T_VFX_WhitePixel` (confirmed 1×1) / `GradientShape` scale 1 speed 0. The
> expression histogram matches count-for-count. **The name-table archaeology was correct: no shader
> constant, curve endpoint, size, or lifetime this recreation ships is invalidated.**
>
> **One symptom listed above was WRONG, and is struck from the list here.** `usedTextures: []` is
> *not* diagnostic of a broken mount — it is empty in the healthy export too, in **all 81** exported
> materials, because `FCk_MaterialExporter` reads a `UMaterialInterface::GetUsedTextures` overload
> that yields nothing for these instances. Read **`textureParams`** instead; it resolved all six
> parameters correctly. The two symptoms that ARE diagnostic remain: a `<none>` renderer material,
> and a corpus with no `materials/` / `textures/` trees.

---

## 2. Visual intent in plain language

A spell **range-indicator ring** — a large, thin, bright circle that marks an area on the ground.

It pops in warm white, snaps almost immediately (within the first ~6% of its life) to a saturated
blue, and holds that blue while the ring ASSEMBLES through a noise dissolve — irregular gaps that
close over the first 40% of the life as the dissolve ramp rises from −1 to 0.2 (§13.6: the family's
channel is signed integrity added to the noise, so −1 = fully dissolved and the rising ramp is a
resolve-in, not a chew-out). It spends mid-life dim but complete, then over the last third swells
sharply back to full opacity before dying. The whole cycle takes 1.1 s and restarts every 1.0 s, so
consecutive rings overlap by about a tenth of a second and the effect reads as a continuous,
flickering pulse rather than a sequence of discrete blinks.

---

## 3. Emitter inventory

| Emitter | Enabled | Sim | Renderer | Notes |
|---|---|---|---|---|
| `Flare_01` | yes | **CPU** | `NiagaraSpriteRendererProperties` | the only emitter |

`[corpus]` One emitter, one renderer. `Determinism: false`, `Bounds: Dynamic`.

---

## 4. Spawn mode, cadence, lifetime, coordinate space, and bounds

| Fact | Value | Source key |
|---|---|---|
| Spawn module | `Spawn Burst Instantaneous` | Emitter Update stack |
| Spawn count | **1** | `SpawnBurst_Instantaneous.Spawn Count` |
| Spawn time | **0** | `SpawnBurst_Instantaneous.Spawn Time` |
| Loop count limit | 1 — **inert**, `UseLoopCountLimit = false` | `SpawnBurst_Instantaneous.Loop Count Limit` |
| Loop behavior | **Infinite** | `Emitter State → Loop Behavior` — ⚠ evidence caveat below |
| Loop duration | **1.0 s** (Fixed) | `EmitterState.Loop Duration` — ⚠ evidence caveat below |

> ⚠ **Evidence caveat (2026-08-01, from the porting-plan sweep):** this emitter's
> `Life Cycle Mode = System`, which makes the emitter-level Loop rows INERT — the system's own
> loop stack governs, and the exporter does not export the system stack. The 1.0 s cadence
> therefore rests on corroborating evidence, not this row: the original visibly re-fires every
> ~1 s in the VfxExamples gym and never fires `OnSystemFinished` (observed 2026-08-01). The
> cadence row stands; the provenance of these two table rows is weaker than the table implies.
| Particle lifetime | **1.1 s** | `InitializeParticle.Lifetime` |
| Kill on lifetime | true | `Particle State` |
| Space | **Local** | `LocalSpace: true` |
| Position mode | Simulation Position, offset `(0,0,0)` | `InitializeParticle` |
| Bounds | Dynamic | emitter |

**Lifetime exceeds the loop by 0.1 s on purpose** — generations overlap slightly. Rounding lifetime
down to the loop duration would remove that overlap and visibly change the pulse.

**The stored `Loop Count Limit = 1` never fires** `[corpus, 2026-08-01]` — the burst module's
`UseLoopCountLimit` is `false`, so the value is an authored leftover. The emitter really does burst
once per loop, forever; a reader who took the row at face value would wrongly expect a single ring.

---

## 5. Niagara module and curve facts extracted from the corpus

All curves sample **NormalizedAge** (0→1 over the 1.1 s life). `C` = constant key, `L` = linear key.

**Colour (`Color` module, `Color from Curve`)** — an almost-instant hue snap at `t ≈ 0.063`:

| Channel | t = 0 | t = 0.0630213 | ≥ 0.0630213 |
|---|---|---|---|
| R | 1.0 `C` | 0.266356 `L` | held |
| G | 0.89627 `C` | 0.102242 `L` | held |
| B | 0.520996 `C` | 1.0 `L` | held |

**Alpha** — three keys, flat then a late swell:

| t | 0 → 0.206364 | 0.693235 | 0.95397 → 1 |
|---|---|---|---|
| A | 0.15 (held) | 0.2 | 1.0 (held) |

**Dynamic Material Parameters** (`Write Parameter Index 0 = true`; indices 1–3 off):

| Channel | Value | Source |
|---|---|---|
| Param 1 (x) | `Float from Curve`: (0, **−1**) `L` → (0.4, **0.2**) `L`, then held | override pin |
| Param 2 (y) | 0 | `DynamicMaterialParameters.Index 0 Param 2` |
| Param 3 (z) | 0 | `DynamicMaterialParameters.Index 0 Param 3` |
| Param 4 (w) | **−0.5** | `DynamicMaterialParameters.Index 0 Param 4` |

**Other authored values:** `InitializeParticle.Color = RGBA(1,1,1,1)` (the curve overrides it),
`Uniform Sprite Size = 700` (`Sprite Size Mode = Uniform`), `Color.Scale Color = (1,1,1)`,
`Color.Scale Alpha = 1`, `FloatFromCurve.Scale Curve = 1`.

**Align Sprite to Mesh Orientation:** Alignment Vector `(0,1,0)`, Facing Vector `(0,0,1)`,
Orientation Quaternion `quat(0,0,0,1)` (identity).

---

## 6. Renderer type, orientation, size, and sorting

`[corpus]` `NiagaraSpriteRendererProperties`
· Material `M_VFX_DisAdd_Ring04`
· Alignment **`CustomAlignment`**
· Facing **`CustomFacingVector`**
· Sort `None`.

With the identity orientation quaternion plus facing `(0,0,1)` / alignment `(0,1,0)`, the quad lies
**flat in the local XY plane facing +Z** — a ground ring, *not* camera-facing. Uniform size 700 means
a 700-unit quad; the ring drawn in the texture occupies ~90% of it, so the visible ring is ≈630 units
across `[inferred — from the texture's radius; confirm visually]`.

---

## 7. Material, texture, and mesh dependency graph

```
NS_Lightning_Range
└── M_VFX_DisAdd_Ring04                    (instance of ↓)
    └── Parents/M_VFX_DissolveAdd          (the DissolveAdd master graph)
        ├── Main_Tex          = T_VFX_Ring_04
        ├── Color_Tex        = T_VFX_Ring_04
        ├── Dissolve_Tex     = T_VFX_Noise_04
        ├── Distortion_Tex   = T_VFX_Noise_04
        ├── GradientShape_Tex = T_VFX_Noise_02
        └── GradientMap_Tex  = T_VFX_WhitePixel
```

Material base properties `[corpus]`: `MD_Surface`, **`BLEND_Translucent`**, **`MSM_Unlit`**,
`twoSided: false`, connected outputs **`EmissiveColor` + `Opacity`** only.

Dynamic-parameter channel names, declared by the material itself: **`dissolve`, `distortion`,
`offset`, `core_color`** — these are what fix the channel contract in §9.

Expression histogram (the "tricks" list): `ScalarParameter ×41`, `DynamicParameter ×8`,
`Multiply/Add/AppendVector ×18` each, `Saturate ×12`, `Panner ×5`, `TextureSampleParameter2D ×6`,
`ParticleColor ×1`, `DepthFade ×1`, `SmoothStep ×1`, `StaticSwitch ×1`.

**Effective parameter values on this instance** (these decide what is actually required):

| Parameter | Value | Consequence |
|---|---|---|
| `Brightness` | **30** | the additive-looking punch; dominant |
| `Distortion_Intensity` | **0** | **distortion is entirely off** |
| `Dissolve_Speed_X/Y` | 0.2 | dissolve noise pans slowly |
| `Dissolve`, `Dissolve_Invert` | 0, 0 | threshold comes purely from the dynamic param |
| `MainTex_Speed_X/Y`, `_Offset_X/Y` | 0 | the ring itself never pans |
| `Color_Core` | `RGBA(1,1,1,0)` | white core… |
| `Color_CoreDifferent` | 1 | …but gated by the `core_color` channel, which is **−0.5** → **off** |
| `Opacity_Boldness` | 1 | no opacity shaping |
| `Opacty_DepthFade` | 10 | soft intersection with scene geometry |
| `GradientMap_Tex` | `T_VFX_WhitePixel` | sampling white = **no-op gradient map** |

### Dependency audit — what the recreation actually needs

| Texture | Verdict | Reason |
|---|---|---|
| `T_VFX_Ring_04` | **REQUIRED** | `Main_Tex`/`Color_Tex` — the ring shape itself |
| `T_VFX_Noise_04` | **REQUIRED** | `Dissolve_Tex`; also `Distortion_Tex`, but that branch is dead (`Distortion_Intensity = 0`) |
| `T_VFX_Noise_02` | not copied | `GradientShape_Tex`; scale 1 / speed 0 and the gradient map is a white pixel, so its contribution is not separable from a constant `[inferred]` |
| `T_VFX_WhitePixel` | not copied | a white pixel — a no-op sample |
| `T_VFX_Part_01` | not copied | appears in the material's name table but **not** in any resolved texture parameter — a parent default this instance overrides |

Texture metadata `[corpus]`: both required textures are 512×512, `sRGB: false`, `TC_Alpha`,
`TA_Wrap`/`TA_Wrap` (`T_VFX_Ring_04` `TSF_G8`, `T_VFX_Noise_04` `TSF_G16`) — greyscale masks, so
Particle Color does all the tinting.

`T_VFX_Ring_04` is a **thin bright circle outline** on black, radius ≈0.45 of the image, line width
≈2–3 px `[visual — read from the exported PNG]`.

**No meshes.** Sprite renderer only.

---

## 8. CkParticles translation

| | |
|---|---|
| Behavior ID | **17** — next after the 0–16 roster |
| Template | **`PS_CkParticles_Template_Single`** (new cadence row) |
| Seeds / salts | **none** — the source spawns exactly one particle with no randomness; the behavior is a pure function of Age |
| `VisTag` | `4` (custom-facing sprite renderer — added for this recreation) |

**The cadence required a template change, and it is recorded rather than hidden.** The shared burst
template is Loop Duration 1.2 s / lifetime 1.2 s / **96** particles; the source is 1.0 s / 1.1 s /
**1**. Approximating it with the shared template would have been wrong in all three numbers.

The smallest reusable fix: the three cadence numbers moved out of the builder and into a **table** in
`CkParticles_ScriptDefinition_Naming.h` (`ck::particles::Get_TemplateSpecs()`), and the builder emits
one template per row. Existing rows keep their exact previous values, so the continuous and shared
burst templates are unchanged. A future recreation with a new cadence adds a row — not another
hand-maintained build call.

Stage outputs written:

| Output | Written | Value |
|---|---|---|
| `Position` | yes | `(0,0,0)` — local space, zero offset |
| `Velocity` | yes | zero — the ring never moves |
| `Color` | yes | the RGB snap + 3-key alpha curve of §5 |
| `Size` | yes | `(700, 700)` constant |
| `Dynamic` | yes | `x` = dissolve curve, `y` = 0, `z` = 0, `w` = −0.5 |
| `Orientation` | yes | identity |
| `Rotation` | yes | 0 |
| `SpriteAlignment` | yes | `(0,1,0)` — the source's Sprite Alignment Vector |
| `SpriteFacing` | yes | `(0,0,1)` — the source's Sprite Facing Vector |
| `Scale`, `MeshIndex` | no | left at default — not a mesh behavior |

**This recreation added `VisTag 4`, a custom-facing sprite renderer.** The template previously had no way
to express a quad fixed in sim space, so a first pass rendered the ring camera-facing — visibly wrong against
a source that lies flat on the ground. Rather than leave that as a permanent fidelity gap, the DI contract
gained `SpriteAlignment` + `SpriteFacing` (float3 each, GPU and CPU in lockstep) and the template gained a
renderer with Niagara's `CustomAlignment` + `CustomFacingVector` pair. It shares `User.SpriteMaterial` with
VisTag 0, so the bound look reaches it with no change at any call site.

Engine trap that shaped this: **a missing `Particles.SpriteAlignment` makes `CustomAlignment` silently fall
back to Unaligned** (`NiagaraSpriteRendererProperties.h`). The attribute must therefore always be written, and
`CkParticles_DefaultOutput` seeds a valid Z-up pair rather than zeros — a degenerate pair collapses the sprite
instead of failing visibly.

---

## 9. CkUsf translation

| | |
|---|---|
| Look | `RingDissolveAdd` |
| Entry point | `CkUsf_Look_DissolveAdd` in `/CkUsf/Looks/DissolveAdd.ush` — the shared family shader (2026-08-01: the NS_BasicAttack port folded this look's own `RingDissolveAdd.ush` into it and appended six parameters this instance resolves inert; look name, master path and every binding are unchanged) |
| Domain / blend | `SurfaceUnlit` / `Translucent` — matching `MD_Surface` + `BLEND_Translucent` + `MSM_Unlit` |
| Outputs | `EmissiveColor` + `Opacity` only, matching the source's connected outputs |

`_Parameters`, in declaration order (the validator enforces this against the HLSL signature):

| # | Param | Type | Default | Source |
|---|---|---|---|---|
| 1 | `ShapeTex` | Texture2D | imported `T_VFX_Ring_04` | `Main_Tex` |
| 2 | `DissolveTex` | Texture2D | imported `T_VFX_Noise_04` | `Dissolve_Tex` |
| 3 | `DistortTex` | Texture2D | imported `T_VFX_Noise_04` | `Distortion_Tex` |
| 4 | `CoreColor` | Vector | `(1,1,1)` | `Color_Core` |
| 5 | `Brightness` | Scalar | **30** | `Brightness` |
| 6 | `DissolveSpeed` | Scalar | 0.2 | `Dissolve_Speed_X/Y` |
| 7 | `DissolveEdge` | Scalar | 0.15 | the parent's `SmoothStep` width `[inferred]` |
| 8 | `DistortScale` | Scalar | 0.1 | `Distortion_Scale_X/Y` |
| 9 | `OpacityBoldness` | Scalar | 1.0 | `Opacity_Boldness` |

**ParticleColor** supplies both the tint (`.rgb`) and the alpha term (`.a`) — the behavior's colour
curve reaches the shader entirely through it.

**Dynamic parameter channels** — named after the source material's own declarations so the emitter
and the look agree by construction:

| Channel | Name | Meaning |
|---|---|---|
| x | `dissolve` | signed integrity ADDED to the dissolve noise; +1 = fully intact, −1 = fully dissolved (§13.6) |
| y | `distortion` | UV distortion strength (0 for this effect) |
| z | `offset` | UV pan offset (0 for this effect) |
| w | `core_color` | blend toward `CoreColor`; ≤ 0 disables it (−0.5 here) |

Emissive = `Tint × Shape × Mask × Brightness`; Opacity = `saturate(Shape × Mask × ParticleColor.a ×
OpacityBoldness)`. Textures are sampled through `CkUsf_SampleTexture2D` (frequency-safe), never raw
`.Sample()`.

### New CkUsf capability this required

CkUsf had no Niagara-facing surface at all. Added, all **opt-in and defaulted off** so every existing
look regenerates unchanged:

- `_UsedWithNiagaraSprites` → bakes `bUsedWithNiagaraSprites` (`Material.h:721`) onto the master.
  Ribbon and mesh-particle usages were deliberately **not** added.
- `_ParticleColor` → wires `UMaterialExpressionParticleColor` into `In.ParticleColor`.
- `_ParticleDynamicParameter` → wires `UMaterialExpressionDynamicParameter` into `In.DynamicParameter`.
- `_ParticleDynamicParameterNames` → channel names for readability in the generated master.

---

## 10. Copied asset destinations

| Source | Destination | Why |
|---|---|---|
| `/Game/Vefects/Anime_VFX/Shared/Textures/T_VFX_Ring_04` | `/CkFoundation/CkParticles/Imported/Vefects/NS_Lightning_Range/T_VFX_Ring_04` | the ring shape |
| `/Game/Vefects/Anime_VFX/Shared/Textures/T_VFX_Noise_04` | `…/NS_Lightning_Range/T_VFX_Noise_04` | the dissolve noise |

Copied by `Import_SourceTextures()` in `CkParticles_TemplateBuilder.cpp`, which runs inside the
asset-rebuild pass. It is **skip-if-present** and **skip-if-source-absent**, so it imports once on a
dev host that has the pack mounted and is inert everywhere else.

**No materials were copied** — `M_VFX_DisAdd_Ring04`'s behavior is recreated as the CkUsf look.

---

## 11. Runtime binding path

```
UCk_Utils_Particles_UE::Spawn_BehaviorAtLocation(ctx, 17, Location, Rotation, Scale)
  └─ ck::particles::Get_BehaviorTemplateSystemObjectPath(17)  →  PS_CkParticles_Template_Single
  └─ SpawnSystemAtLocation → SetIntParameter("User.BehaviorId", 17)
  └─ ck::particles::Get_BehaviorLookName(17)  →  "RingDissolveAdd"
       └─ Get_GeneratedLookMasterObjectPath  →  /CkFoundation/CkUsf/GeneratedLooks/M_CkUsf_Look_RingDissolveAdd
       └─ SetVariableMaterial("User.SpriteMaterial", master)
```

The look binds through the **same** `User.SpriteMaterial` mechanism the procedural-texture material
instances use, and the resolution lives in the central naming header beside the rest of the behavior
metadata. Callers pass a behavior id and nothing else; **the gym does not patch the material after
spawning.** An explicit `InTextureName` still wins, so the texture path is unchanged for behaviors
0–16.

CkParticles deliberately does **not** take a dependency on CkUsf (it would drag in CkEcs + CkGraphics
for a path string). The CkUsf path convention is mirrored, and the authoring test asserts the
mirrored path resolves — so a convention change fails loudly instead of silently rendering the
default material.

---

## 12. Exact verification procedure

### Automated

| Test | Lane | Asserts |
|---|---|---|
| `CkTests.UnitTests.CkParticles.LightningRangeBehavior` | any (no Niagara, no RHI, no fork) | **the numbers**: the colour keys at spawn and at the snap plus that they HOLD to death, all three alpha keys plus that the late move is a swell not a fade and that alpha never decreases, the dissolve ramp and that it never reverses, the inert distortion/offset/core channels, zero position/velocity/rotation and identity orientation, `VisTag 4` with a non-degenerate alignment/facing pair, and Seed-independence. Cannot pass vacuously — the pre-switch default size is ~8–24 units, so asserting 700 proves the switch reached case 17 |
| `CkTests.UnitTests.CkParticles.RosterSanity` | any (no Niagara, no RHI, no fork) | every behavior, across ages/lifetimes/seeds, produces renderable output: finite everywhere, non-negative size, **non-negative** alpha, normalized orientation, `VisTag` inside the renderer set, valid `MeshIndex` where read, non-degenerate sprite vectors on VisTag 4 — and every id routes to a template the cadence table declares. Also pins `NumBehaviors == 18` and behavior 17's template + look bindings. **Alpha's UPPER bound is deliberately not asserted**: behavior 13's rare flash branch multiplies alpha by 1.5 (`case 13`, the `k >= 0.985` layer), which the renderer clamps harmlessly — an `alpha <= 1` assertion would red on pre-existing behavior rather than on a regression. Tightening it means changing behavior 13 first |
| `CkTests.UnitTests.CkParticles.LightningRangeAuthoring` | metadata half: any engine. Asset half: needs `CK_WITH_PARTICLES=1` (skips otherwise, FAILS on a fork engine) | 17 is in the roster; routes to the single-burst template; the cadence row is 1.0/1.1/1; the `_Single` template loads and exposes `User.BehaviorId`, `User.ParticleScript` and `User.SpriteMaterial`; the look's generated master resolves through the MIRRORED CkUsf path convention; both imported textures resolve from plugin content; **neither the template nor the look master has any `/Game/Vefects` package dependency**. It does NOT assert the master's sprite-usage flag — that is `NiagaraSpriteContract`'s line, and duplicating it here would need a CkUsf dependency this gate does not want |
| `CkTests.UnitTests.CkUsf.NiagaraSpriteContract` | `--no-nullrhi` (self-skips otherwise, like `GeneratesUsableMasters`) | the look definition opts in (so nothing below passes vacuously); the look generates; the master declares `bUsedWithNiagaraSprites` and NOT the ribbon/mesh-particle usages; `ParticleColor`, `ParticleAlpha` and all four `DynParam*` Custom inputs are *connected* (not merely declared); the DynamicParameter node carries the source's own `dissolve`/`distortion`/`offset`/`core_color` channel names; regeneration is idempotent and leaves exactly ONE ParticleColor and ONE DynamicParameter node; **every look that did not opt in gained neither the flag nor the pins** |
| `Ck_AutoTest_Particles_SpawnAllBehaviors` | `--no-nullrhi` | every id in `Get_NumBehaviors()` spawns a live component |

The spawn test **self-skips under `-nullrhi`** (`FApp::CanEverRender()` is false, so Niagara refuses
to create components). A green default lane therefore proves authored state, **not** that anything
rendered.

> **All five rows above have now been compiled and run** (2026-08-01) — results and the two defects the
> first compile exposed are in *Completion state → Test results*. Run the lane serially; see the note
> there on why the parallel default produces reds with no assertion behind them.

**Know which gate holds which line.** The two CPU-mirror tests are the only ones that check behavior
*correctness*; everything else checks existence, and existence checks pass against a behavior that does
nothing. Neither covers the GPU `.ush` — it cannot be executed headlessly, so GPU/CPU lockstep stays a
review obligation. And none of them substitutes for the visual gate below, **which has NOT been
executed** — no statement anywhere in this recipe is backed by looking at the effect.

```bash
./CkAuto/UnrealToolbox.exe --build --config=Development --target=Editor --test --test-pattern Particles --no-nullrhi --discover-fresh --parallel 1 --output=Saved/Logs/BuildTest-Particles.log --project=<project-root>
```

### `[EDITOR-VERIFY]` — visual fidelity gate (human, ~10 min)

Neither Niagara graph is opened at any point.

1. Regenerate assets (once, after any behavior/look edit):
   - `Ck_Usf_GenerateLooks RingDissolveAdd` in the editor console.
   - Editor Subsystems → `CkParticles_GeneratorSubsystem` → **Create Template System**.
2. **Both sides at once:** PIE the **VfxExamples** gym (CkTests `Script/CkVfxExamples/`, registered in
   `CkTests_GymRegistry.as`) and go to the **LIGHTNING RANGE** pair — the recreation and the original
   `NS_Lightning_Range` spawn on adjacent pedestals at the same scale. The original is soft-loaded at
   runtime; if its pedestal shows the "add the Vefects content plugin" placard, the pack is absent or
   the mount is wrong (see §1) — fix that before comparing anything. If it renders untextured, same
   diagnosis. *The original asset stays read-only — never open it in the Niagara editor.*
3. Re-arm both sides in sync with the console exec `Ck_GymVfxExamples_RestartAll` so the t=0 flashes
   coincide.
4. View from ~1500 units up at a ~45° pitch so the ground plane is legible.
5. Compare, pausing at **t ≈ 0.0**, **0.4**, and **1.0 s** into a cycle (`slomo 0.1` makes the 1.1 s
   life readable; remember to `slomo 1` after):

   | # | Criterion | Expected |
   |---|---|---|
   | a | Shape | thin circular outline, not a disc or a blob |
   | b | Apparent diameter | equal at equal distance (≈630 units of visible ring) |
   | c | Orientation / facing | **known difference — see §13** |
   | d | Initial flash colour (t≈0) | warm white, near `(1.0, 0.90, 0.52)` |
   | e | Colour progression | snaps to blue `(0.27, 0.10, 1.0)` within the first ~1/16 of the life, then holds |
   | f | Dissolve direction & edge | irregular gaps CLOSING as the ring assembles, edges soft not hard-clipped; the line is complete by 40% of the life and stays complete (§13.6) |
   | g | Opacity progression (t≈0.4 vs 1.0) | dim and flat mid-life; a pronounced swell to full brightness in the last third |
   | h | Lifetime | one ring visibly persists ~1.1 s |
   | i | Repeat cadence | a new ring every 1.0 s, overlapping the previous by ~0.1 s |
   | j | Under rotation | rotate the actor 45° in yaw and pitch — both should respond the same way |
   | k | Under scale | set actor scale 2.0 — both rings should double |

6. Record every mismatch in §13, with the timestamp and criterion letter.

---

## 13. Confirmed fidelity differences or intentional deviations

**The visual gate in §12 has NOT been executed** — this session could not launch the editor for a
visual comparison. Everything below is therefore either a *known* difference derived from the source
data, or *unverified*. The effect is **source-verified, not visually verified**.

### Known differences — deliberate

1. **Distortion branch omitted.** `Distortion_Intensity` resolves to 0 on this instance, so the
   branch cannot contribute. The look still exposes `distortion` (channel y) for reuse.
2. **Core-colour branch inert.** `core_color` is −0.5 and `saturate` clamps it to 0. Implemented but
   never engaged by this effect.
3. **`GradientShape` / `GradientMap` chain omitted.** `GradientMap_Tex` is a white pixel and the
   gradient scale/speed are 1/0, so the chain is not separable from a constant here `[inferred]`.
   `T_VFX_Noise_02` was therefore not copied.
4. **`DepthFade` omitted.** The source sets `Opacty_DepthFade = 10`, which softens the ring where it
   intersects geometry. CkUsf surface looks do not wire scene depth, so this is dropped. Visible only
   where the ring cuts through world geometry.
5. **`_TwoSided = true`** on the look, versus `twoSided: false` on the source. This is a deliberate
   divergence, and it is load-bearing now that the ring lies flat: a one-sided ground quad disappears
   when the camera drops below its plane. Verify the original's behaviour from underneath before
   matching it — it may rely on the sprite renderer's own facing rather than material two-sidedness.
6. **The family's dissolve sense was corrected (2026-08-02)** after the NS_BasicAttack A/B gate:
   `DissolveAdd.ush` now computes `Mask = smoothstep(0, DissolveEdge, Noise + dissolve)` — the
   channel is signed integrity ADDED to the noise (+1 fully intact, −1 fully dissolved), which is
   the only reading consistent with every corpus instance (the previous direct-threshold reading
   rendered the slash family's dissolve=1 instances invisible; see NS_BasicAttack.md §13). For THIS
   effect the corpus dissolve ramp `(0, −1) → (0.4, 0.2)` is unchanged, but its meaning inverts:
   the ring RESOLVES IN through noise gaps over the first 40% of its life and is a complete line
   afterwards, rather than starting intact and chewing gaps out. §2 and criterion (f) were updated
   to match. Unverified visually, like everything else in §12 step 5.

7. **The family's shape pan is now CLAMPED (2026-08-02) — provably inert for this effect.** The same
   NS_BasicAttack A/B found the `offset` channel wrapping rather than clamping (`DissolveAdd.ush`
   applied no `frac` and no clamp, so addressing fell to the baked textures' default `TA_Wrap`), which
   made the slash layers sweep the arc twice per life. The shader now samples the shape at a
   `saturate`d panned U and masks it with `step(0, uP) * step(uP, 1)`; the dissolve noise keeps its
   time-driven wrap. **Behavior 17's appearance is unchanged**, and not merely by argument: this
   effect drives `offset` at **0** for its whole life, `MainTexScale` is (1, 1), `DistortIntensity` is
   0 so nothing perturbs the coordinate, and the sprite's own UV is in [0, 1] — the sampled coordinate
   therefore never leaves [0, 1], where clamp and wrap are the same lookup and the mask is 1. Measured
   over 4096 samples including u = 0 and u = 1 exactly, across every texture in the family: max
   absolute difference **0.0**, and the mask never zeroes an in-range sample. Nothing in §2, §12 or
   criterion (f) changes.

**The facing gap is CLOSED.** An earlier pass rendered the ring camera-facing; the recreation now uses the
custom-facing sprite renderer (`VisTag 4`) added for it, matching the source's `CustomAlignment` +
`CustomFacingVector` pair. This is confirmed at the authoring level (the behavior writes `VisTag 4` and both
sprite vectors, the renderer is built with the matching enums) but **not yet confirmed visually** — criterion
(c) is exactly what the editor gate must now check.

### Unverified

- Every criterion in §12 step 5. In particular the `DissolveEdge = 0.15` softness is an **inferred**
  stand-in for the parent graph's `SmoothStep` width — it is the most likely value to need tuning,
  and it is a one-word change in `CkUsf_Looks_Assets.as`.
- Whether `Brightness = 30` reads the same through CkUsf's translucent-unlit master as through the
  source's DissolveAdd graph. Both are unlit + translucent, but the source's exact composite order is
  not fully reconstructed from an expression histogram.

---

## 14. Reusable lessons for future effects

1. **Confirm the renderer material is non-null before believing any material fact.** A `<none>`
   renderer material means a broken reference chain, and a material instance whose parent failed to
   load reports *plausible defaults* (`Opaque`/`DefaultLit`), not an error. The `.uasset` name table
   is the cheap tiebreaker. **`usedTextures: []` is NOT part of that symptom set** — it is empty on
   every material the exporter writes, healthy or not; `textureParams` is the array that carries the
   resolved textures (corrected 2026-08-01 against a clean corpus — see §1).
2. **Read the material's *effective* parameter values before writing the shader.** Half of
   `M_VFX_DissolveAdd`'s feature set — distortion, core colour, gradient map — is switched off by
   this instance's values. Recreating the master graph faithfully would have been wasted work and a
   worse match than recreating the *instance*.
3. **The material declares its own dynamic-parameter channel names.** Take them verbatim
   (`dissolve`/`distortion`/`offset`/`core_color`); it makes the emitter↔shader contract
   self-documenting and removes a whole class of channel-order bug.
4. **Cadence is data, not code.** Loop duration / lifetime / burst count belong in a table, so
   matching a source exactly costs a row instead of a new hand-written builder call. Check cadence
   *before* writing HLSL — it is the cheapest fidelity win and the most expensive to retrofit.
5. **Particle material nodes do not hand you the float4 you expect.**
   `UMaterialExpressionParticleColor`'s output 0 is **`RGB` (float3)** with alpha on a separate `A`
   output; `UMaterialExpressionDynamicParameter` exposes **four scalar outputs**, not one float4, and
   needs `GetOutputs()` called after setting `ParamNames` or every by-name connect silently no-ops.
   Assemble float4s in generated code rather than with `AppendVector` chains (which have failed under
   SM6 in this codebase before). Both mistakes surface as a generated-`Material.ush` type error that
   **names no look** — trace back through the `SHADER FAILED TO COMPILE` line that does.
6. **A material that generates is not a material that compiles.** `Generate_LookMaterial` returns a
   valid object whose shaders failed; only the `Generate_AllLookMaterials().Errors` path reports
   compile failures. A wiring test passing therefore proves nothing about the HLSL — this exact gap
   let a broken shader through until `GeneratesUsableMasters` caught it. Know which of your gates
   actually holds the line.
7. **Make new generator inputs opt-in and prove the negative.** The regression that matters is not
   "does the new look work" but "did every existing look change". Assert the untouched case.
8. **A greyscale mask + Particle Color + a large Brightness is the whole DissolveAdd idiom.** Expect
   to reuse `RingDissolveAdd` for the rest of the `M_VFX_DisAdd_*` family by swapping `ShapeTex`.
9. **Adding a look re-gates every other look.** `GeneratesUsableMasters` regenerates *all* looks, so a
   broken new one fails tests that look unrelated (it also took down `MultiPassRendersToTexture` here).
   Read the failure text before assuming you broke the subsystem it names.
10. **Templates can only be regenerated on a FORK-enabled engine, and getting this wrong is silent.**
    Without `CkNiagaraAuthoring.h` the builder has no pin-authoring API, so `CK_WITH_PARTICLES=0` and the
    behavior-call module is skipped. The templates still write, still save, still load — and render
    NOTHING, because the DI is never invoked. This regressed the committed templates once
    (`ExecuteStage` 35 → 0, 437KB → 368KB) while every test stayed green, because existence checks and
    "component spawned" checks both pass against an inert template. `Build_AllTemplateSystems` now
    refuses under that define. **Before trusting any template, run
    `grep -ac ExecuteStage <template>.uasset` — expect ~35, never 0.**
11. **Match the gate to the claim.** "The asset regenerated" and "a component spawned" are existence
    checks; neither says a particle moved or a pixel lit. When a whole green suite is compatible with
    the feature doing nothing at all, the suite is measuring the wrong thing — and a visual gate is
    not optional garnish, it is the only check that closes that hole.
10. **Sprite facing is a template capability, not a shader one.** A source using `CustomAlignment` /
   `CustomFacingVector` cannot be matched by a camera-facing renderer no matter how good the
   material is. Check the renderer's alignment/facing pair in §6 *before* promising fidelity.
