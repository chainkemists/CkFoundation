# Recipe: NS_Lightning_Range → `LightningRange` (BehaviorId 17)

Schema and evidence-tag conventions: [README.md](README.md).

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
> This recipe recreates the **`Anime_VFX/Shared/Skills`** one, which is the fixed target and the
> variant with no user parameters.

> ### Environment trap that invalidates material archaeology
> `[corpus]` On this dev host the pack was copied one level too deep
> (`Content/Vefects/Vefects/Anime_VFX/…`), so every intra-pack reference dangled. The symptoms were
> **silent and misleading**, not loud: the system's renderer exported as `material: "<none>"`, the
> corpus produced no `materials/` or `textures/` trees at all, and a direct material export reported
> `BLEND_Opaque` + `MSM_DefaultLit` + `usedTextures: []` — plausible-looking values that are simply
> the fallbacks a material instance returns when its parent fails to load. Ground truth
> (`BLEND_Translucent`, `MSM_Unlit`) was only visible in the `.uasset` name table.
>
> **Before trusting any material read, confirm the renderer material is non-null.** A `<none>`
> renderer material on a system that visibly has one means the mount is wrong, not that the material
> is absent.

---

## 2. Visual intent in plain language

A spell **range-indicator ring** — a large, thin, bright circle that marks an area on the ground.

It pops in warm white, snaps almost immediately (within the first ~6% of its life) to a saturated
blue, and holds that blue while a noise dissolve chews irregular gaps out of the ring's line. It
spends most of its life dim and gappy, then over the last third swells sharply back to full opacity
before dying. The whole cycle takes 1.1 s and restarts every 1.0 s, so consecutive rings overlap by
about a tenth of a second and the effect reads as a continuous, flickering pulse rather than a
sequence of discrete blinks.

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
| Loop count limit | 1 | `SpawnBurst_Instantaneous.Loop Count Limit` |
| Loop behavior | **Infinite** | `Emitter State → Loop Behavior` |
| Loop duration | **1.0 s** (Fixed) | `EmitterState.Loop Duration` |
| Particle lifetime | **1.1 s** | `InitializeParticle.Lifetime` |
| Kill on lifetime | true | `Particle State` |
| Space | **Local** | `LocalSpace: true` |
| Position mode | Simulation Position, offset `(0,0,0)` | `InitializeParticle` |
| Bounds | Dynamic | emitter |

**Lifetime exceeds the loop by 0.1 s on purpose** — generations overlap slightly. Rounding lifetime
down to the loop duration would remove that overlap and visibly change the pulse.

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
| `VisTag` | `0` (camera sprite renderer) — see the fidelity gap in §13 |

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
| `Scale`, `MeshIndex` | no | left at default — not a mesh behavior |

---

## 9. CkUsf translation

| | |
|---|---|
| Look | `RingDissolveAdd` |
| Entry point | `CkUsf_Look_RingDissolveAdd` in `/CkUsf/Looks/RingDissolveAdd.ush` |
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
| x | `dissolve` | erosion threshold; more negative = more intact |
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
| `CkTests.UnitTests.CkParticles.LightningRangeAuthoring` | any (renderer-free) | 17 is in the roster; routes to the single-burst template; the cadence row is 1.0/1.1/1; the look binds and its master resolves and declares sprite usage; both imported textures resolve from plugin content; **neither the template nor the look master has any `/Game/Vefects` package dependency** |
| `CkTests.UnitTests.CkUsf.NiagaraSpriteContract` | `--no-nullrhi` | the look generates; the master declares `bUsedWithNiagaraSprites`; `ParticleColor` and all four `DynParam*` pins are *connected* (not merely declared); regeneration is idempotent; **every non-particle look gained neither the flag nor the pins** |
| `Ck_AutoTest_Particles_SpawnAllBehaviors` | `--no-nullrhi` | every id in `Get_NumBehaviors()` spawns a live component |

The spawn test **self-skips under `-nullrhi`** (`FApp::CanEverRender()` is false, so Niagara refuses
to create components). A green default lane therefore proves authored state, **not** that anything
rendered.

```bash
./CkAuto/UnrealToolbox.exe --build --config=Development --target=Editor --test --test-pattern Particles --no-nullrhi --output=Saved/Logs/BuildTest-Particles.log --project=<project-root>
```

### `[EDITOR-VERIFY]` — visual fidelity gate (human, ~10 min)

Neither Niagara graph is opened at any point.

1. Regenerate assets (once, after any behavior/look edit):
   - `Ck_Usf_GenerateLooks RingDissolveAdd` in the editor console.
   - Editor Subsystems → `CkParticles_GeneratorSubsystem` → **Create Template System**.
2. **Baseline:** in an empty level, drag `/Game/Vefects/Anime_VFX/Shared/Skills/NS_Lightning_Range`
   into the viewport at the origin. *Read-only — do not open or edit it.* If it renders untextured,
   the pack mount is wrong (see §1) — fix that before comparing anything.
3. **Recreation:** PIE the CkParticles gym, go to the **LIGHTNING RANGE (17)** station. Re-arm with
   the console exec `Ck_GymParticles_RestartAll`.
4. Put both on screen at the same scale: place the recreation at the origin too, and view from
   ~1500 units up at a ~45° pitch so the ground plane is legible.
5. Compare, pausing at **t ≈ 0.0**, **0.4**, and **1.0 s** into a cycle (`slomo 0.1` makes the 1.1 s
   life readable; remember to `slomo 1` after):

   | # | Criterion | Expected |
   |---|---|---|
   | a | Shape | thin circular outline, not a disc or a blob |
   | b | Apparent diameter | equal at equal distance (≈630 units of visible ring) |
   | c | Orientation / facing | **known difference — see §13** |
   | d | Initial flash colour (t≈0) | warm white, near `(1.0, 0.90, 0.52)` |
   | e | Colour progression | snaps to blue `(0.27, 0.10, 1.0)` within the first ~1/16 of the life, then holds |
   | f | Dissolve direction & edge | irregular gaps opening in the line, edges soft not hard-clipped; gaps grow over the first 40% then stop growing |
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

1. **Facing / orientation (the significant one).** The source renders as a quad lying **flat in the
   local XY plane** (`CustomAlignment` + `CustomFacingVector`, identity quaternion) — a ground ring.
   The recreation uses `VisTag 0`, the **camera-facing** sprite renderer, because the template has no
   custom-facing sprite renderer. **Expect the recreation to face the camera while the original stays
   flat on the ground** — criterion (c) will differ, and so may (b) at oblique angles.
   *Fix, deliberately out of this slice's scope:* add a custom-facing sprite renderer to the template
   as a new `VisTag`, driven by the existing `Orientation` output.
2. **Distortion branch omitted.** `Distortion_Intensity` resolves to 0 on this instance, so the
   branch cannot contribute. The look still exposes `distortion` (channel y) for reuse.
3. **Core-colour branch inert.** `core_color` is −0.5 and `saturate` clamps it to 0. Implemented but
   never engaged by this effect.
4. **`GradientShape` / `GradientMap` chain omitted.** `GradientMap_Tex` is a white pixel and the
   gradient scale/speed are 1/0, so the chain is not separable from a constant here `[inferred]`.
   `T_VFX_Noise_02` was therefore not copied.
5. **`DepthFade` omitted.** The source sets `Opacty_DepthFade = 10`, which softens the ring where it
   intersects geometry. CkUsf surface looks do not wire scene depth, so this is dropped. Visible only
   where the ring cuts through world geometry.
6. **`_TwoSided = true`** on the look, versus `twoSided: false` on the source. Irrelevant for a
   camera-facing sprite; it becomes relevant if difference 1 is fixed and the ring is viewed from
   below.

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
   is the cheap tiebreaker.
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
10. **Sprite facing is a template capability, not a shader one.** A source using `CustomAlignment` /
   `CustomFacingVector` cannot be matched by a camera-facing renderer no matter how good the
   material is. Check the renderer's alignment/facing pair in §6 *before* promising fidelity.
