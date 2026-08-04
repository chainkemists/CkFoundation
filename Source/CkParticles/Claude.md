# CkParticles

**Purpose:** Author Niagara particle/system **logic in C++ + USF/USH**, not the Niagara graph editor. A custom
Data Interface exposes one generic, pure `ExecuteStage` function; the actual behavior is HLSL in `.ush` (GPU)
mirrored by C++ (CPU). The template Niagara System — built **entirely from C++** — calls `ExecuteStage` with a
per-instance `User.BehaviorId`, so one asset renders any behavior selected by an int.

This is the particle analog of **CkUsf** (which authors materials via `.ush` instead of the material editor).

**Depends on:** `Core`, `CoreUObject`, `Engine`, `RenderCore`, `RHI`, `Projects`, `GameplayTags`, `Niagara`, `CkCore`, `CkLog`.
**Engine:** built/verified against the Chainkemists **UE 5.7** AngelScript fork. The Niagara DI API is version-sensitive
(`GetFunctionsInternal`, context/`GPUParamInfo` HLSL hooks, `AppendTemplateHLSL`); re-verify against engine source on a
major bump.

> ### Forked-engine requirement — `CK_WITH_PARTICLES` is an AUTHORING gate, not a runtime one
> **Runtime works on any engine, stock or forked.** The DI, the `.ush` behaviors, the CPU mirror and the spawn
> utils are all ungated — every `#if CK_WITH_PARTICLES` lives in **CkParticlesEditor**. A template generated once
> on a fork-enabled engine and committed is a compiled Niagara asset; it runs identically on retail. Nothing about
> shipping or consuming CkParticles requires an engine change.
>
> What needs the fork is **regenerating the template assets**. Building the behavior-call module from C++ uses a
> few NiagaraEditor pin-authoring symbols stock UE does not export
> (`UNiagaraNodeWithDynamicPins::RequestNewTypedPin` / `AddParameter` / `AddParameterPin`). The fork tags them
> `NIAGARAEDITOR_API` and ships a marker header
> `Engine/Plugins/FX/Niagara/Source/NiagaraEditor/Public/CkNiagaraAuthoring.h`; both `CkParticles*.Build.cs` probe
> for that file and define `CK_WITH_PARTICLES` accordingly. The engine edit is additive-only and safe to
> merge-forward, but it is **opt-in and not required** — treat it as a content-authoring tool, like needing the
> editor to bake assets.
>
> Practical consequence: on a stock engine, **do not regenerate**. `Build_AllTemplateSystems` refuses (see below)
> precisely because a regen there would overwrite good committed templates with inert ones.
>
> *(Reimplementing the module build with only exported API is not currently viable: `RequestNewTypedPin` repurposes
> the node's "Add" pin and then calls the protected virtual `OnNewTypedPinAdded`, which is where the parameter-map
> nodes actually register the parameter. `UEdGraphSchema_Niagara::TypeDefinitionToPinType` IS exported, so only the
> registration step blocks it. The engine-change-free alternative would be to ship the behavior-call module as one
> committed generic Niagara module-script asset added via `Add_ModuleFromAssetPath` — it is behavior-agnostic, since
> behaviors are selected by `User.BehaviorId` inside the `.ush`, so per-effect authoring would stay pure code.)*

---

## Architecture

```
UCkParticles_DataInterface (UNiagaraDataInterface; pure stage fn, one per-instance datum)
  └─ ExecuteStage(BehaviorId, DeltaTime, Age, Lifetime, Position, Velocity, Seed, EmitterAge, Tuning)
        -> OutPosition, OutVelocity, OutColor, OutSize, OutScale, OutOrientation (quat),
           OutDynamic (float4 -> Particles.DynamicMaterialParameter), OutRotation (sprite degrees),
           OutMeshIndex, OutVisTag, OutSpriteAlignment, OutSpriteFacing
       GPU: GetParameterDefinitionHLSL -> AppendTemplateHLSL(CkParticles_DataInterfaceTemplate.ush)
            template #includes CkParticles_Behaviors.ush -> Behaviors/*.ush   (logic lives here)
       CPU: VMExecuteStage -> NDICkParticlesLocal::ExecuteStage_CPU            (mirror of the same switch)
```

**Renderer selection (2026-07-12 fidelity evolution):** each behavior writes `VisTag` per particle and the
template's renderers draw only their tagged particles — `0` camera sprite (per-effect texture via
`User.SpriteMaterial`, the legacy default), `1` velocity-aligned sprite (streaks/tracers; stretch =
`Size.y`), `2` smoke sprite (translucent `M_CkParticles_SoftSmoke`, `Rotation` applies), `3` carrier mesh
(`MeshIndex` picks SM_CkParticles_ **Sweep/Tube/Shell/Disc**; row-declared mesh renderers name their
carrier instead; `Scale` + `Orientation` apply; the meshes carry
`M_CkParticles_SweepErode` / `M_CkParticles_FresnelShell`), `4` **custom-facing sprite** — a quad fixed in sim
space rather than billboarded, with `SpriteAlignment` as its up axis and `SpriteFacing` as its plane normal
(ground decals / range rings, matching Niagara's CustomAlignment + CustomFacingVector pair). VisTag 4 shares
`User.SpriteMaterial` with VisTag 0, so a behavior-bound CkUsf look reaches either without callers knowing.
**Both attributes must be written**: a missing `Particles.SpriteAlignment` makes CustomAlignment silently fall
back to Unaligned, so `CkParticles_DefaultOutput` seeds a valid Z-up pair rather than zeros.

**VisTags 0–4 are the SHARED set — nothing behavior-specific may be added to it**, because every template
carries it. A recreation whose source draws through renderers the shared set cannot express declares its own
on its **cadence row** instead (`FCk_ParticlesRendererSpec` + `FCk_ParticlesTemplateSpec::RendererOverrides`
in the naming header). **Five kinds** cover what a recreation needs beyond the shared set: `Mesh` (one named
generated mesh drawn with one named CkUsf look; `Scale` + `Orientation` apply), the three sprite quads
Niagara distinguishes by its alignment/facing pair — `CameraFacingSprite`, `VelocityAlignedSprite` and
`CustomFacingSprite` — and `Ribbon`. A sprite/mesh kind may additionally declare a `SubImageSize` flipbook
grid, which makes its renderer read `Particles.SubImageIndex` over an X×Y sheet; a row that declares none
divides nothing (a ribbon renderer has no `SubImageSize` at all). A `Mesh` kind may additionally declare
`MeshFacingMode` (`Default` / `Velocity` / `CameraPosition`, mirroring `ENiagaraMeshFacingMode`) and
`MeshScale` (a constant multiplier on the carrier, on top of `Particles.Scale`); both default to Niagara's
own defaults, and RosterSanity rejects either on a non-mesh kind because the builder writes them nowhere
else. `CameraPosition` is the mode a behavior cannot fake — the stage has no camera.

**Ribbons ride a SECOND emitter.** `UNiagaraRibbonRendererProperties` carries no `RendererVisibility`, so a
ribbon renderer cannot be VisTag-gated and one added to the shared emitter would link *every* particle on the
template into ribbons. A ribbon-bearing row therefore declares
`FCk_ParticlesTemplateSpec::RibbonEmitter` (its own burst/rate + `Ribbon`-kind renderers); the builder emits a
second GPU emitter on the row's own loop and lifetime, wired to the same DI, `User.BehaviorId` and
`User.SpriteMaterial`. The two populations are told apart by a **seed bank**: the ribbon emitter's graph adds
`ck::particles::RibbonSeedBase` to the `UniqueID → Seed` wire, so a behavior reads the bank off the Seed it
already receives (`CkParticles_IsRibbonSeed` / `CkParticles_LocalSeed`, lockstepped GPU/CPU) and the DI
signature never moves. Ribbon width reads `Particles.SpriteSize` (one float at its offset, i.e. `Size.x`) and
the ribbon id rides `Particles.MeshIndex` — inert on ribbons, which have no carrier mesh.
Row renderers bind their look master **explicitly**
(`bOverrideMaterials` / `Material`) rather than through `User.SpriteMaterial`, because one user parameter
cannot carry several materials — so a behavior drawing ONLY through row renderers keeps
`Get_BehaviorLookName` at `NAME_None`. They are emitted for that row's template only. Behavior 7 (Slash)
owns 5–9: four crescent-mesh slash layers and one spark sprite. Behaviors 18/19 (the Vefects projectile
pair) share 10–11 on one cadence row; every Vefects port from 20 up owns its own contiguous band above
those — 12–14 (20), 15–27 (21), 28–36 (22), 37–49 (23), 50–61 (24), 62–70 (25), 71–76 (26), 77–83 (27),
84–90 (28), 91–96 (29), 97–104 (30), 105–112 (31), 113–118 (32), 119–130 (33), 131–146 (34) and
147–156 (35), 157–163 (36, the last of them the ribbon emitter's), 164–166 (37), 167–174 (38),
175–184 (39), 185–197 (40 AND 41 — the palette twins share their band), 198–209 (42 and 43),
210–224 (44), 225–241 (45) and 242–245 (46) — and 19
additionally binds a look, because its one camera-facing layer draws on the shared VisTag 0 where
`User.SpriteMaterial` is the only material channel.
**The roster-wide ceiling is DERIVED** — `ck::particles::Get_RosterVisTag_Max()` walks the
cadence table on top of `SharedRendererVisTag_Max`; tests read it and never restate a literal.
Ordering consequence: **generate the CkUsf looks BEFORE rebuilding templates** — row renderers resolve their
masters at build time, and a miss logs an Error rather than failing the build.
`Dynamic` drives the mesh/smoke materials:
x dissolve, y distortion, z UV-pan, w emissive boost — the exact idiom the marketplace "DissolveAdd"
materials animate via curves (Vefects M_VFX_DisAdd_Slash01 et al).

`Seed` is the particle's `Particles.UniqueID`. Hash it for per-particle randomness — `Common.ush` provides
`CkParticles_Rand(Seed, Salt)` → [0,1) and `CkParticles_RandDir(Seed)` → unit vector (24-bit math, bit-identical
between the GPU `.ush` and the C++ CPU mirror).

Shaders (`Source/CkParticles/Shaders/CkParticles/`, virtual path `/CkParticles`):
- `Common.ush` — `FCkParticles_StageInput/Output`, `CkParticles_NormalizedAge`, `CkParticles_Rand`, `CkParticles_RandDir`.
- `Behaviors/Behavior_*.ush` — Gravity (0), Swirl (1), Explosion (2), Fire (3), Fireworks (4), Galaxy (5),
  Beam (6, directional — aim via spawn rotation), Slash (7, the faithful NS_BasicAttack re-port: 19-layer burst on a procedural crescent), Nova (8, shockwave ring),
  and the **marketplace recreations** (2026-07-12, derived from the VFX corpus translation sheets —
  `Saved/CkVfxCorpus/analysis/` in the dev host): MuzzleFlash (9, +X barrel), ImpactBurst (10, +Z normal),
  Tracer (11, +X forward), SmokePlume (12), SparksBurst (13), GroundRing (14), LightningStrike (15),
  AuraSwirl (16). Multi-layer marketplace effects compress into one behavior via Seed-branching
  (`k = Rand(Seed, 0)` picks the layer), and one-shot archetypes replay their arc via
  `frac(Age/Cycle + phase)` — continuous-spawn template today; a burst-spawn template variant would
  make them true one-shots.
- `CkParticles_Behaviors.ush` — `#include`s behaviors + `CkParticles_ExecuteStage` dispatch (the switch).
- `CkParticles_DataInterfaceTemplate.ush` — the DI's generated GPU function wrapper.

---

## Recreating an existing Niagara effect — the cookbook

Recreations of real marketplace/reference effects are documented as **recipes** under
[`Cookbook/`](Cookbook/) — [`Cookbook/README.md`](Cookbook/README.md) defines the recipe schema
(provenance → archaeology → CkParticles/CkUsf translation → verification → fidelity gaps → lessons)
and the corpus-regeneration command that produces the evidence.

**Read the README before recreating anything**, and write the recipe alongside the code, not after.
Shipped recipes:

| Recipe | Source | Behavior |
|---|---|---|
| [`NS_Lightning_Range.md`](Cookbook/NS_Lightning_Range.md) | Vefects `NS_Lightning_Range` + `M_VFX_DisAdd_Ring04` | `LightningRange` (17) |
| [`NS_BasicAttack.md`](Cookbook/NS_BasicAttack.md) | Vefects `NS_BasicAttack` + the `M_VFX_DisAdd_{Slash01,Slash02,Slash04,Pan_Wind02,Part04}` set | `Slash` (7) |
| [`NS_Gunshot_Projectile.md`](Cookbook/NS_Gunshot_Projectile.md) | Vefects `NS_Gunshot_Projectile` + `M_VFX_DisAdd_{Part01,Part04}` | `GunshotProjectile` (18) |
| [`NS_Arrow_Projectile.md`](Cookbook/NS_Arrow_Projectile.md) | Vefects `NS_Arrow_Projectile` + the same two instances | `ArrowProjectile` (19) |
| [`NS_Fire.md`](Cookbook/NS_Fire.md) | Vefects `NS_Fire` + `M_VFX_DisAdd_{Part01,Part04,Flames01}` | `FireBurst` (20) |
| [`NS_FireBall_Hit.md`](Cookbook/NS_FireBall_Hit.md) | Vefects `NS_FireBall_Hit` + 12 `M_VFX_DisAdd_*` instances + `M_VFX_FlatAdd` | `FireBallHit` (21) |
| [`NS_Gunshot_Hit.md`](Cookbook/NS_Gunshot_Hit.md) | Vefects `NS_Gunshot_Hit` + 8 `M_VFX_DisAdd_*` instances + `M_VFX_FlatAdd` | `GunshotHit` (22) |
| [`NS_Arrow_Cast.md`](Cookbook/NS_Arrow_Cast.md) | Vefects `NS_Arrow_Cast` + 12 `M_VFX_DisAdd_*` instances + `M_VFX_FlatAdd` | `ArrowCast` (23) |
| [`NS_Arrow_Hit.md`](Cookbook/NS_Arrow_Hit.md) | Vefects `NS_Arrow_Hit` + 10 `M_VFX_DisAdd_*` instances + `M_VFX_FlatAdd` | `ArrowHit` (24) |
| [`NS_Bomb_Spawn.md`](Cookbook/NS_Bomb_Spawn.md) | Vefects `NS_Bomb_Spawn` + 8 `M_VFX_DisAdd_*` instances + `MI_VFX_Bomb` | `BombSpawn` (25) |
| [`NS_PickupLoop.md`](Cookbook/NS_PickupLoop.md) | Vefects `NS_PickupLoop` + 6 `M_VFX_DisAdd_*` instances | `PickupLoop` (26) |
| [`NS_HealLoop.md`](Cookbook/NS_HealLoop.md) | Vefects `NS_HealLoop` + 7 `M_VFX_DisAdd_*` instances | `HealLoop` (27) |
| [`NS_BuffLoop.md`](Cookbook/NS_BuffLoop.md) | Vefects `NS_BuffLoop` + 7 `M_VFX_DisAdd_*` instances | `BuffLoop` (28) |
| [`NS_DebuffLoop.md`](Cookbook/NS_DebuffLoop.md) | Vefects `NS_DebuffLoop` + 6 `M_VFX_DisAdd_*` instances | `DebuffLoop` (29) |
| [`NS_PickupCast.md`](Cookbook/NS_PickupCast.md) | Vefects `NS_PickupCast` + 8 `M_VFX_DisAdd_*` instances | `PickupCast` (30) |
| [`NS_HealCast.md`](Cookbook/NS_HealCast.md) | Vefects `NS_HealCast` + 8 `M_VFX_DisAdd_*` instances | `HealCast` (31) |
| [`NS_DebuffCast.md`](Cookbook/NS_DebuffCast.md) | Vefects `NS_DebuffCast` + 6 `M_VFX_DisAdd_*` instances + `SM_VFX_Slash02` | `DebuffCast` (32) |
| [`NS_Gunshot_Cast.md`](Cookbook/NS_Gunshot_Cast.md) | Vefects `NS_Gunshot_Cast` + 10 `M_VFX_DisAdd_*` instances + `M_VFX_FlatAdd` | `GunshotCast` (33) |
| [`NS_FireBall_Cast.md`](Cookbook/NS_FireBall_Cast.md) | Vefects `NS_FireBall_Cast` + 15 `M_VFX_DisAdd_*` instances + `M_VFX_FlatAdd` | `FireBallCast` (34) |
| [`NS_Lightning_Cast.md`](Cookbook/NS_Lightning_Cast.md) | Vefects `NS_Lightning_Cast` + 10 `M_VFX_DisAdd_*` instances | `LightningCast` (35) |
| [`NS_FireBall_Projectile.md`](Cookbook/NS_FireBall_Projectile.md) | Vefects `NS_FireBall_Projectile` + 5 `M_VFX_DisAdd_*` instances + `M_VFX_FlatAdd` | `FireBallProjectile` (36) |
| [`NS_Bomb_Projectile.md`](Cookbook/NS_Bomb_Projectile.md) | Vefects `NS_Bomb_Projectile` + `M_VFX_DisAdd_{Part01,Trail01}` + `MI_VFX_Bomb` | `BombProjectile` (37) |
| [`NS_BuffCast.md`](Cookbook/NS_BuffCast.md) | Vefects `NS_BuffCast` + 8 `M_VFX_DisAdd_*` instances | `BuffCast` (38) |
| [`NS_Lightning_Muzzle.md`](Cookbook/NS_Lightning_Muzzle.md) | Vefects `NS_Lightning_Muzzle` + 9 `M_VFX_DisAdd_*` instances + `M_VFX_FlatAdd` | `LightningMuzzle` (39) |
| [`NS_ExplosionGround.md`](Cookbook/NS_ExplosionGround.md) | Vefects `NS_ExplosionGround` + 10 `M_VFX_DisAdd_*` instances + `M_VFX_FlatAdd` | `ExplosionGround` (40) |
| [`NS_ExplosionIceGround.md`](Cookbook/NS_ExplosionIceGround.md) | Vefects `NS_ExplosionIceGround` — the same asset set, recoloured | `ExplosionGroundIce` (41) |
| [`NS_ExplosionOmni.md`](Cookbook/NS_ExplosionOmni.md) | Vefects `NS_ExplosionOmni` + 9 `M_VFX_DisAdd_*` instances + `M_VFX_FlatAdd` | `ExplosionOmni` (42) |
| [`NS_ExplosionIceOmni.md`](Cookbook/NS_ExplosionIceOmni.md) | Vefects `NS_ExplosionIceOmni` — the same asset set, recoloured | `ExplosionOmniIce` (43) |
| [`NS_Bomb_Explosion.md`](Cookbook/NS_Bomb_Explosion.md) | Vefects `NS_Bomb_Explosion` + 9 `M_VFX_DisAdd_*` + `M_VFX_FlatAdd` + 3 `MI_VFX_FresnelBomb_*` | `BombExplosion` (44) |
| [`NS_Lightning_Hit.md`](Cookbook/NS_Lightning_Hit.md) | Vefects `NS_Lightning_Hit` + 12 `M_VFX_DisAdd_*` + `M_VFX_FlatAdd` — the widest system in the pack, and the only port that added ZERO new assets of any kind | `LightningHit` (45) |
| [`NS_Dash.md`](Cookbook/NS_Dash.md) | Vefects `NS_Dash` + `M_VFX_DisAdd_{Wind01,Wind02,Wind03,Part02}` + `SM_VFX_{Ring01,Ring04}` — the pack's only `Life Cycle Mode = Self` emitter and its only mixed-coordinate-space system | `Dash` (46) |

---

## Adding a new behavior (pure C++/USF — no asset edits)

1. Author `Shaders/CkParticles/Behaviors/Behavior_<Name>.ush` with
   `FCkParticles_StageOutput CkParticles_Behavior_<Name>(FCkParticles_StageInput In)`.
2. `#include` it in `CkParticles_Behaviors.ush` and add a branch to `CkParticles_ExecuteStage` for the new id.
3. Mirror the exact math in `NDICkParticlesLocal::ExecuteStage_CPU` (`CkParticles_DataInterface.cpp`).
   **GPU and CPU MUST stay in lockstep** (same id, same math, same hashing).
4. Add the new `.ush` to `NDICkParticlesLocal::DependentShaderFiles` so `AppendCompileHash` busts the shader cache.

5. Bump `ck::particles::NumBehaviors` (Naming header) so roster-driven tests and gyms pick the behavior up.
6. Pick the behavior's **template** in `ck::particles::Get_BehaviorTemplateSystemObjectPath`:
   - continuous-rate seed (the default),
   - `PS_CkParticles_Template_Burst` via `Get_BehaviorUsesBurstTemplate` — one instantaneous 96-burst per
     ~1.2 s loop on a real `Age`; surplus burst particles a behavior doesn't use must be hidden
     (`Color/Size/Scale = 0`),
   - or a **new cadence row** in `Get_TemplateSpecs()` when recreating a source whose loop/lifetime/count
     differ. Add the row; do not approximate onto the nearest template and do not fake the cadence with
     `frac(Age/Cycle)` inside the behavior.
7. If the behavior's look is a hand-authored shader rather than a procedural texture, bind its generated
   CkUsf master in `ck::particles::Get_BehaviorLookName` — the spawn path does the rest.

Then select it at runtime (`Spawn_BehaviorAtLocation(..., id, ...)`) or via a `UCkParticles_ScriptDefinition`'s
`_BehaviorId` + regenerate. **Self-driving** behaviors (write an absolute `O.Position` from Age/Seed, like Swirl /
Explosion / Galaxy) rely on the emitter being in **local space** — the template sets this, so they render where the
system is spawned. **Velocity-integrating** behaviors (Gravity) work in either space.

---

## Create Template System (fully code-built — no manual step)

**Editor → Editor Subsystems → `CkParticles_GeneratorSubsystem` → `Create Template System`** (or headless:
env `CK_PARTICLES_REBUILD_TEMPLATES=1` + toolbox `--test --test-pattern RebuildTemplateAssets`). With
`CK_WITH_PARTICLES=1` this runs the full asset pipeline — procedural textures → VFX master materials
(`CkParticles_MaterialGenerator.cpp`) → carrier meshes (`CkParticles_MeshGenerator.cpp`, MeshDescription-built) →
every cadence-table template — entirely from C++
(`CkParticles_TemplateBuilder.cpp`):

- GPU emitter, **local space**, generous **fixed bounds** (`±3000`) so self-driving behaviors render at the spawn
  location and the system isn't frustum-culled.
- `User.BehaviorId` int + the DI wired as `User.ParticleScript`.
- The **behavior-call module** in Particle Update — built via the exported pin API: an Input parameter map fans to a
  Map Get (reads `User.ParticleScript`/`User.BehaviorId`, `Engine.DeltaTime`, `Particles.Age`/`Lifetime`/`Position`/
  `Velocity`/`UniqueID`) → the DI `ExecuteStage` member call → a Map Set (`Particles.Position`/`Velocity`/`Color`).
  The map threads through Map **Set** (`Source→Dest`); Map Get only taps it (it has no map output).
- Bakes the procedural VFX textures and assigns the master material (below) to the sprite renderer.

Idempotent — re-run any time; it overwrites in place.

**Baked-behavior specialization (2026-08-04).** Each template's ExecuteStage call node carries a
`CkBakedIds` function specifier (that template's behavior ids joined with `_`), stamped by the builder on
the NODE's `FunctionSpecifiers` map (the `Signature` copy alone never reaches compile — both compile
bridges overwrite it from the node map). `GetParameterDefinitionHLSL` reads it from
`ParamInfo.GeneratedFunctions` and emits `CkParticles_DataInterfaceTemplate_Baked.ush` with ONLY those
behaviors' includes; no specifier = the legacy full-corpus wrapper (a full-corpus compute PSO costs
~170-200 s of AMD driver compile on first render — the VFX-select freeze). Per-instance DI state CANNOT
carry this: Niagara runs DI codegen on a transient class-CDO duplicate, never the `User.ParticleScript`
instance. The specifier rides the graph hash, so a regen re-keys the DDC. After a regen,
`grep -ac CkBakedIds <template>.uasset` must be non-zero on every template (alongside the `ExecuteStage`
and `CkTuning` checks). Wrapper-file comments must never spell a substitution token in braces —
`FString::Format` substitutes inside comments and multi-line values spill out as bare HLSL.

**After a regen, run the STABILIZE lane, then verify** — env `CK_PARTICLES_STABILIZE=1` + toolbox
`--test --no-nullrhi --test-pattern PrewarmTemplates`, then the same command once more WITHOUT the env var and
confirm its log line says `out-of-sync at load: 0` for the CPU-sim set. The lane loads every distinct template
path across the roster (deduped) plus every original Niagara system under `/Game/Vefects`, drives their compiles
to completion (GPU shader maps included when stabilizing, hence `--no-nullrhi`), and — only under the env var —
RESAVES each system that arrived out of sync, persisting the post-load-fixup compile ids and VM data the way the
engine's own resave commandlet does. Without the env var it saves nothing and only prewarms/reports.

Why this exists (measured 2026-08-03): `UNiagaraSystem::PostLoad` re-checks `AreScriptAndSourceSynchronized()`
on EVERY load, and an asset saved by the builder (which never went through load-time graph fixups) or saved
before a `.ush` hash change fails that check in every session — the "Preparing… Niagara Systems" toast and the
first-activation stall, forever, until stabilized. Two traps inside the lane's history: saving on VM readiness
alone re-persists an asset whose GPU shader map is still compiling (still fails the load check), so the
stabilize wait mirrors the FULL emitter-side check; and a `.ush` edit after a regen invalidates every template's
saved ids — stabilize again after ANY shader edit. Residual, not fixable by resave: under the engine-default
`AsyncTasks` compilation mode the GPU shader map lives in DDC and is fetched after PostLoad's check runs, so
GPU-SIM systems (all 32 templates) still re-arm a lazy on-demand resolve each session — with stabilized ids
that costs milliseconds per system on a warm DDC (the CPU-sim Vefects originals pass the check outright and go
fully silent). Eliminating even that would take an engine-fork change to the PostLoad shader-sync check.

> **Regeneration REFUSES on a non-fork engine.** With `CK_WITH_PARTICLES=0` there is no behavior-call module,
> so every template written would be **inert**: the DI is never invoked and nothing renders. Those assets still
> save and still load, so the failure is invisible to any test that only checks existence — it cost a real
> regression once (a regen on a non-fork machine silently stripped the module out of the committed templates,
> 437KB → 368KB, and the whole test suite stayed green). `Build_AllTemplateSystems` therefore logs an Error and
> returns false rather than overwriting. **Check `ExecuteStage` is present in a template before trusting it:**
> `grep -ac ExecuteStage PS_CkParticles_Template.uasset` must be **non-zero** (39 on all three templates as of
> 2026-08-01; earlier wording said "~35" — the count tracks the builder, so treat only ZERO as the failure).

---

## Procedural VFX textures + master material

**`Generate VFX Textures`** (same subsystem; also called by `Create Template System`) bakes a `UTexture2D` library
under `/CkFoundation/CkParticles/Textures/` **purely from noise/SDF/radial math** (`CkParticles_TextureGenerator.cpp`)
— no imported art: `Glow`, `Flare` (star), `Smoke` (FBM + erosion in A), `Electric` (ridged), `Streak`, `Ring` (SDF),
`SweepStreak`, `TileNoise`; the NS_BasicAttack stand-in set `SlashArc01` / `SlashArc02` / `WindBand` /
`SoftParticle` / `SparkStreak`; and the Vefects hit/impact set `SoftParticleBright` / `SoftParticleFine` /
`RingUneven` / `RingFlare` / `StarFour` / `LightStrip` / `Cloud04` / `Cloud05` / `TileNoiseCoarse` /
`TileNoiseBanded` / `ImpactStar`; and the arrow/bomb set `StarFourTight` / `StarFourSplit` / `WindBandMid`; and the idle-loop set's single
addition, `ArrowChevron` — the library's only POLYGONAL mask, an SDF over a measured quadrilateral; and the
cast set's single addition, `LensSheet` — the only 2x2 atlas in the library that is a plain decaying puff
rather than a directional burst; and the attack-cast set's single addition, `LightningSheet` — the only atlas
whose four frames are INDEPENDENT paintings rather than one shape stepping, so its frame index reseeds the
field instead of advancing it; and the projectile-trail set's single addition, `TileNoiseSparse` — the only
noise in the library with a hard black FLOOR (42.8% of it is exactly zero), so a dissolve driven by it clears
in patches rather than eroding everywhere at once; and the event-ribbon set's three, `LinearRamp` — the only
texture that is a TRANSCRIPTION rather than a stand-in, because `T_VFX_Gradient_02` measures as exactly
`1 - u` — plus `LightningBolt` and `LightningBand`, the first paints whose whole structure is carried by
MEASURED PROFILES (a wandering centre line, a per-row peak and width, a cross-section, a band shape) rather
than by a fitted closed form; and the explosion set's three, `ExpGroundScorch` — the paint whose NAME is
the trap, since `T_VFX_Star_04` has no dominant angular harmonic at all and is a lumpy blob rather than the
four-point star `StarFour` reproduces — plus `GradientTrapezoid`, the library's second TRANSCRIPTION, and
`TileNoiseFine`, the only noise that needs TWO Fbm scales because its autocorrelation tail belongs to
neither a fine nor a coarse one alone.
Their constants come from characteristics MEASURED off the corpus PNGs
(profiles, streak counts, falloff exponents, band splits — never copied pixels; see each recipe's §7).

Three of them are not plain masks, and the **kind** decides that (`ECk_VfxTextureKind`): `Mask` is the
512² greyscale default; `MaskSheet` lays the same out as a 2×2 flipbook of 256² frames (`WindSheet`,
`ImpactSheet`), read by a row renderer that declares a matching `SubImageSize`; `ColorLut` is a 512×2 sRGB
colour ramp sampled along u (`LutWhite` — the family's inert white default — and `LutRainbow`).
Masks are **grayscale** (so Particle Color tints them), `SRGB=false`, uncompressed (`TC_VectorDisplacementmap`).

`M_CkParticles_VfxMaster` samples a **`BaseTexture`** parameter × **Particle Color**, additive + unlit (sampler type
auto-picked via `GetSamplerTypeForTexture`). Per-effect looks come from swapping `BaseTexture` — material instances or
a per-component override (`UNiagaraComponent::SetVariableMaterial`) bound to a `User.*` material param.

---

## Generating per-ScriptDefinition systems

1. Author a `UCkParticles_ScriptDefinition` (set `_ScriptName`, `_BehaviorId`, optionally `_TemplateSystem`).
2. Subsystem → **Generate Particle Systems** (or `ck::particles_editor::Generate_AllParticleSystems()`).
3. Writes `PS_CkParticles_<ScriptName>` under `/CkFoundation/CkParticles/GeneratedSystems/` (template duplicate with
   `User.BehaviorId` patched). Use when you want a baked per-effect asset; for ad-hoc spawns, spawn the template
   directly and set `User.BehaviorId` per component.

---

## Runtime spawn

**Behavior roster (the `BehaviorId` a caller passes):** Gravity=0, Swirl=1, Explosion=2, Fire=3, Fireworks=4,
Galaxy=5, Beam=6, Slash=7, Nova=8, MuzzleFlash=9, ImpactBurst=10, Tracer=11, SmokePlume=12, SparksBurst=13,
GroundRing=14, LightningStrike=15, AuraSwirl=16, LightningRange=17, GunshotProjectile=18, ArrowProjectile=19,
FireBurst=20, FireBallHit=21, GunshotHit=22, ArrowCast=23, ArrowHit=24, BombSpawn=25, PickupLoop=26,
HealLoop=27, BuffLoop=28, DebuffLoop=29, PickupCast=30, HealCast=31, DebuffCast=32, GunshotCast=33,
FireBallCast=34, LightningCast=35, FireBallProjectile=36, BombProjectile=37, BuffCast=38,
LightningMuzzle=39, ExplosionGround=40, ExplosionGroundIce=41, ExplosionOmni=42,
ExplosionOmniIce=43, BombExplosion=44, LightningHit=45, Dash=46.

Ids 40-43 are the explosion FAMILY: two structural variants of one effect in two palettes, sharing one
implementation (`Behaviors/Behavior_ExplosionShared.ush` + one `Explosion_Run` in the CPU mirror) behind
four thin entry points. A palette twin still needs its own id because a behavior id resolves to exactly one
template path and that path is the spawn contract — but it declares the SAME renderers and the same
cadence as its original, so their two rows share one renderer-spec function and one VisTag band.

`FireBurst` (20) is the Vefects `NS_Fire` re-port and is unrelated to `Fire` (3), the procedural rising column
that predates the cookbook — the two only share a source-asset name.

The roster SIZE has one definition — `ck::particles::NumBehaviors`, exposed to BP/AS as
`UCk_Utils_Particles_UE::Get_NumBehaviors()`. Tests and gyms iterate that; never re-state a maximum id.

**Aim-axis conventions** (these are baked into the behavior math — spawn rotation aims them):
MuzzleFlash/Tracer forward = **+X**; ImpactBurst surface normal = **+Z**;
GroundRing/LightningStrike/AuraSwirl ground plane = local **XY**; Beam travels down **+X**.

**Per-instance tuning (`User.CkTuning`, 2026-08-03).** Every template exposes a float4 user parameter
(default identity `(1,1,1,1)`) that the stage applies CENTRALLY — in `CkParticles_ExecuteStage`
(`CkParticles_Behaviors.ush`) and its CPU mirror, never inside a behavior: x scales `Size`+`Scale`,
y scales `Color.rgb`, z scales `Color.a`, w pre-scales `Age`+`DeltaTime` (playback rate; Niagara still
retires particles at their real lifetime, so w>1 finishes the arc early and holds, w<1 gets cut).
Behaviors stay untouched — corpus-measured constants remain the defaults, tuning is a bounded layer on
top. Author a `UCkParticles_TuningDefinition` DataAsset (four floats) and spawn via
`Spawn_BehaviorAtLocation_Tuned(..., Tuning)`, or retune a live component with
`Request_ApplyTuning(Component, Tuning)` / `Request_ApplyTuningValues(Component, x, y, z, w)`
(null asset = identity).

**Per-part tuning (the DI's per-instance block, 2026-08-03).** `User.CkTuning` tunes a whole system; per-part
tuning tunes the LAYERS inside it, addressed by the VisTag each behavior writes.
`FCkParticles_PartTuningBlock` (`DataInterface/CkParticles_PartTuning.h`) is a fixed budget of
`MaxTunedParts` (24) parts × 5 float4s — size/stretch/mesh-scale/speed, tint+alpha, per-axis `Dynamic`,
rotation offset + visibility window, position offset — plus a `BandStart`. Shared VisTags 0–4 occupy rows
0–4; a behavior's own band maps onto row `5 + (VisTag - BandStart)`. A tag outside `[0, 24)` is left untuned.
It is applied CENTRALLY, after `CkParticles_ExecuteStage` returns, in the DI's **template wrapper**
(`CkParticles_DataInterfaceTemplate.ush` — the wrapper, not `CkParticles_Behaviors.ush`, because that is where
the DI's shader parameters are in scope), mirrored line-for-line by `NDICkParticlesLocal::ExecuteStage_CPU`.
The block cannot ride a User parameter (Niagara has no float4-array user type), so it lives in a
component-keyed module registry — `UCk_Utils_Particles_UE::Request_ApplyPartTuningBlock` /
`Request_ResetPartTuning`, C++-facing (the reflected way in is a tuning asset's part rows, applied by
`Request_ApplyTuning`, which reads the behavior id back off the component's `User.BehaviorId`) — and reaches the GPU as the DI's per-instance data
(`InitPerInstanceData` / `PerInstanceTick` resolve `FNiagaraSystemInstance::GetAttachComponent()`) → the RT
proxy → `SetShaderParameters`. **A missing entry is the IDENTITY, never zeros** — a zero block would hide
every particle.

**Part rows in the tuning DataAsset.** `UCkParticles_TuningDefinition` carries the four global floats *plus* a
`_Parts` array of `FCkParticles_PartTuning_AssetRow` — one row per layer the behavior draws, each with size /
stretch / mesh-scale / speed, tint + alpha, dissolve / distortion / UV-pan / emissive, a rotation offset, a
visibility window and a position offset. `_GlobalTint` has no slot in the `User.CkTuning` float4, so it is folded
into every row's tint by `Get_AsPartTuningBlock`, which is also the only path per-part values reach the DI.
The row ROSTER is generator-owned (`EditFixedSize` — values are editable, rows are not): **Generate Tuning Assets**
fills it from `ck::particles::Get_BehaviorTunableParts` (the 5 shared VisTags, then the behavior's cadence-row band
in VisTag order, ribbon renderers included) and RECONCILES it on every later run — rows are matched by **VisTag**,
missing ones added, undeclared ones removed with a log line, names refreshed, values never touched. A row whose
VisTag the behavior no longer declares is skipped at conversion with a warning rather than aliased onto a
neighbour, and `Get_PartTuningRowIndex` now answers `INDEX_NONE` for the whole interval
`(SharedRendererVisTag_Max, BandStart)` — a tag there belongs to no part of the behavior, and the old band
arithmetic ran it backwards onto a SHARED row.

**Per-behavior tuning is a CONVENTION path, resolved at spawn.** Every behavior may own a
`UCkParticles_TuningDefinition` at
`/CkFoundation/CkParticles/Tuning/DA_CkParticles_Tuning_<Name>` (`<Name>` =
`ck::particles::Get_BehaviorName(id)`, path =
`Get_BehaviorTuningAssetObjectPath(id)`); the shared spawn path loads it and applies it to every
component it spawns. An absent asset is legitimate and silent — it renders the identity. An explicit
`Spawn_BehaviorAtLocation_Tuned(..., Tuning)` OVERRIDES the asset; a null `InTuning` there falls back
to it rather than writing the identity over it.
**`Generate Tuning Assets`** (same subsystem; also called at the end of `Create Template System`, and
headless via env `CK_PARTICLES_GENERATE_TUNING=1` + toolbox `--test --test-pattern GenerateTuningAssets`)
creates only the MISSING assets with identity values — an existing one is a designer's tuning and is
**never** overwritten, which is what makes running it on every regen safe.
The VfxExamples gym's `Ck_GymVfxExamples_Tune` is an OVERLAY on top of the asset; `..._TuneReset` drops
the overlay and restarts the pair, because only a respawn brings the asset's values back.
That gym also DEFERS a pair spawn behind a visible `COMPILING <pair>` HUD line rather than blocking the game
thread — it polls `UCk_Utils_Particles_UE::Get_IsBehaviorTemplateReady(id)` each frame and spawns both sides
together once the template answers ready, so a cold first run after a regen leaves the editor live and the
selector usable.

Adding a NEW input to `ExecuteStage` requires the four-site lockstep edit
(function spec, `.ush` template, VM binding, CPU mirror) plus the builder's Map Get wiring, and a full
template regen — see the tuning change for the exemplar.

**Sprite material fallback:** `User.SpriteMaterial` (`ck::particles::Get_SpriteMaterialParameterName`) overrides
the sprite renderer's material per component. When unset, the renderer's own Material is used — a miss renders
the default glow, never an invisible effect.

**Behavior → CkUsf look binding.** A behavior whose visual identity is a hand-authored shader (rather than a
procedural texture) binds a generated CkUsf master in `ck::particles::Get_BehaviorLookName`. The spawn path
resolves it and binds it through the SAME `User.SpriteMaterial` param, so callers pass only a behavior id and
**no caller ever patches the material after spawning**. An explicit `InTextureName` still wins, leaving the
texture path unchanged for behaviors that use it. CkParticles deliberately does not depend on CkUsf — the
generated-master path convention is mirrored in the naming header and a test asserts it still resolves.

**Template cadence is a TABLE, not code.** `ck::particles::Get_TemplateSpecs()` lists one row per cadence
(asset name, loop duration, particle lifetime, burst count, renderer overrides, spawn rate) and the editor
builder emits one template per row. A row may declare a burst, a continuous rate, or BOTH; declaring
neither is the legacy seed template, whose cadence comes from the emitter factory defaults:

| Template | Loop | Lifetime | Spawn | Used by |
|---|---|---|---|---|
| `PS_CkParticles_Template` | — | — | continuous | the default roster |
| `PS_CkParticles_Template_Burst` | 1.2 s | 1.2 s | 96 | the multi-particle one-shots (10, 13, 14, 15) |
| `PS_CkParticles_Template_Single` | 1.0 s | 1.1 s | 1 | one-sprite, one-second-loop recreations (17) |
| `PS_CkParticles_Template_Slash` | 1.0 s | 0.5 s | 19 | Vefects `NS_BasicAttack`'s exact cadence (7); declares 5 row renderers |
| `PS_CkParticles_Template_ProjectileTrio` | 10 s | 10 s | 3 | Vefects `NS_Gunshot_Projectile` + `NS_Arrow_Projectile` (18, 19) — a Loop-Once 10 s SYSTEM; declares 2 row renderers |
| `PS_CkParticles_Template_FireBurst` | 2 s | 1 s | 10 | Vefects `NS_Fire` (20) — declares 3 row renderers, one of them a 2×2 sub-UV sheet |
| `PS_CkParticles_Template_FireBallHit` | 2 s | 1.34 s | 47 | Vefects `NS_FireBall_Hit` (21) — declares 13 row renderers |
| `PS_CkParticles_Template_GunshotHit` | 2 s | 0.65 s | 40 | Vefects `NS_Gunshot_Hit` (22) — declares 9 row renderers |
| `PS_CkParticles_Template_ArrowCast` | 2 s | 1.55 s | 42 | Vefects `NS_Arrow_Cast` (23) — declares 13 row renderers, one a 2×2 sub-UV sheet |
| `PS_CkParticles_Template_ArrowHit` | 2 s | 0.55 s | 34 | Vefects `NS_Arrow_Hit` (24) — declares 12 row renderers, one custom-facing |
| `PS_CkParticles_Template_BombSpawn` | 2 s | 1.05 s | 28 | Vefects `NS_Bomb_Spawn` (25) — declares 9 row renderers, one an opaque prop mesh |
| `PS_CkParticles_Template_PickupLoop` | 2 s | 4 s | rate 27.5/s | Vefects `NS_PickupLoop` (26) — declares 6 row renderers |
| `PS_CkParticles_Template_HealLoop` | 1 s | 2 s | rate 34.5/s | Vefects `NS_HealLoop` (27) — declares 7 row renderers |
| `PS_CkParticles_Template_BuffLoop` | 2 s | 2 s | rate 48/s | Vefects `NS_BuffLoop` (28) — declares 7 row renderers |
| `PS_CkParticles_Template_DebuffLoop` | 2 s | 2 s | rate 36/s | Vefects `NS_DebuffLoop` (29) — declares 6 row renderers, one a 2x2 sub-UV sheet |
| `PS_CkParticles_Template_PickupCast` | 2 s | 1.05 s | 22 | Vefects `NS_PickupCast` (30) — declares 8 row renderers, every one camera-facing |
| `PS_CkParticles_Template_HealCast` | 2 s | 1.55 s | 17 **+ rate 50/s** | Vefects `NS_HealCast` (31) — the first row to carry BOTH spawn stacks; 8 row renderers, one a velocity-aligned 2x2 sheet |
| `PS_CkParticles_Template_DebuffCast` | 2 s | 2 s | 30 **+ rate 65/s** | Vefects `NS_DebuffCast` (32) — 6 row renderers, one a 2x2 sheet and one the claw mesh |
| `PS_CkParticles_Template_GunshotCast` | 2 s | 1.55 s | 40 | Vefects `NS_Gunshot_Cast` (33) — 12 row renderers, THREE of them 2x2 sheets |
| `PS_CkParticles_Template_FireBallCast` | 2 s | **2.05 s** | 50 | Vefects `NS_FireBall_Cast` (34) — 16 row renderers; the only row whose lifetime exceeds its loop |
| `PS_CkParticles_Template_LightningCast` | 2 s | 1.55 s | 30 **+ rate 40/s** | Vefects `NS_Lightning_Cast` (35) — 10 row renderers; the row rate is a PEAK the behavior thins |
| `PS_CkParticles_Template_FireBallProjectile` | 10 s | 10 s | 15 **+ rate 408/s** | Vefects `NS_FireBall_Projectile` (36) — 6 row renderers + a RIBBON emitter at 100 points/s carrying the mirrored trail pair |
| `PS_CkParticles_Template_BombProjectile` | **2.5 s** | 2.5 s | 4 | Vefects `NS_Bomb_Projectile` (37) — 2 row renderers + a ribbon emitter whose 17-point BURST is placed by arc length, not by time |
| `PS_CkParticles_Template_BuffCast` | 2 s | 1.5 s | 23 | Vefects `NS_BuffCast` (38) — 7 row renderers + a ribbon emitter bursting 301 EVENT samples across seven strands |
| `PS_CkParticles_Template_LightningMuzzle` | 2 s | 0.6 s | 24 | Vefects `NS_Lightning_Muzzle` (39) — 9 row renderers, three of them meshes, + a ribbon emitter carrying the arc PAIR |
| `PS_CkParticles_Template_ExplosionGround` | 2 s | 1.5 s | 70 | Vefects `NS_ExplosionGround` (40) — 12 row renderers + a ribbon emitter bursting 301 event samples across seven strands |
| `PS_CkParticles_Template_ExplosionGroundIce` | 2 s | 1.5 s | 70 | Vefects `NS_ExplosionIceGround` (41) — the palette twin: same renderers, same cadence, its own template |
| `PS_CkParticles_Template_ExplosionOmni` | 2 s | 1.3 s | 65 | Vefects `NS_ExplosionOmni` (42) — 11 row renderers + the same ribbon emitter |
| `PS_CkParticles_Template_ExplosionOmniIce` | 2 s | 1.3 s | 65 | Vefects `NS_ExplosionIceOmni` (43) — the second palette twin |
| `PS_CkParticles_Template_BombExplosion` | 2 s | 0.5 s | **162** | Vefects `NS_Bomb_Explosion` (44) — the cookbook's largest burst; 15 row renderers, SEVEN of them meshes, and the only row that uses every C8 facing mode |
| `PS_CkParticles_Template_LightningHit` | 2 s | 1.3 s | 84 | Vefects `NS_Lightning_Hit` (45) — 16 row renderers, the widest spread in the cookbook (two 2x2 sheets in Niagara's TWO different sub-UV modes, two custom-facing ground quads, four meshes over three carriers) + a ribbon emitter carrying the same arc PAIR NS_Lightning_Muzzle draws |
| `PS_CkParticles_Template_Dash` | 2 s | 1.55 s | 19 **+ rate 50/s** | Vefects `NS_Dash` (46) — 4 row renderers, one per source emitter: two meshes (the shared Cylinder and this row's own Cone), one 2x2 sub-UV camera quad and one velocity-aligned streak |

Rows verified 2026-08-01 against `ck::particles::Get_TemplateSpecs()` and against `Add_SpawnEmitterStack`,
which reads `LoopDuration` / `ParticleLifetime` / `BurstCount` straight off the spec — so the table above is
the generator's input, not a transcription of it. A row may additionally declare its own renderers (see the
VisTag paragraph above). The first three templates were regenerated 2026-08-01 and each carries a non-zero
`ExecuteStage` count (39).

Recreating a source whose cadence differs adds a ROW — never an approximation onto the nearest template, and
never a `frac(Age/Cycle)` fake inside the behavior. `Get_BehaviorTemplateSystemObjectPath(id)` is the single
resolver the spawn path calls.

`UCk_Utils_Particles_UE` (runtime module, BlueprintCallable / AngelScript-callable):
- `Spawn_BehaviorAtLocation(WorldContext, BehaviorId, Location, Rotation, Scale)` — spawns the seed template and sets
  `User.BehaviorId`.
- `Spawn_SystemAtLocation(WorldContext, System, BehaviorId, ...)` — same for an explicit (e.g. generated) system.

The **CkParticles gym** lives in CkTests (`Script/CkParticles/`, registered as "Particles" in `CkTests_GymRegistry.as`):
**one station per behavior, EXCEPT the faithful Vefects ports (7, 17, 18, 19)** — those live in the **VfxExamples gym**
(`Script/CkVfxExamples/`), which shows each port as a PAIR of pedestals: the CkParticles recreation next to the
ORIGINAL Niagara system, soft-loaded by path string at runtime (no package dependency — an absent Vefects install
shows a placard instead). In-PIE exec there: `Ck_GymVfxExamples_RestartAll` (re-fires both sides in sync).
The Particles gym's remaining stations spawn each behavior's template with a fitting procedural texture; the
marketplace-recreation stations credit their exemplars in the station description. In-PIE exec:
`Ck_GymParticles_RestartAll`. The composition pattern for richer VFX (spells/trails) is multiple
`Spawn_BehaviorAtLocation` calls at one transform. Automated coverage: the PIE autotest
`CkAutoTest_Particles_SpawnAllBehaviors.as` loops the FULL roster (including the ports) and asserts a live
component per id; `CkAutoTest_VfxExamples_PairStationsSpawn.as` covers the pair harness.
**Never restate a maximum id in a gym or test** — that is exactly the drift `NumBehaviors` exists to prevent.

---

## Anti-patterns

1. Don't edit generated `PS_CkParticles_*` systems by hand — regeneration overwrites them. Edit the template
   (renderer/material) or the `.ush` (logic).
2. Don't hand-author the behavior-call module — it's code-built now. Re-run `Create Template System` instead.
3. Don't let the GPU `.ush` and CPU `ExecuteStage_CPU` diverge — two implementations of one behavior.
4. CPU sims can't run HLSL; only the C++ mirror runs there. If a behavior is GPU-only, document it.
