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

> ### Forked-engine requirement — `CK_WITH_PARTICLES`
> The fully code-built template (below) needs a few NiagaraEditor pin-authoring symbols stock UE does not export
> (`UNiagaraNodeWithDynamicPins::RequestNewTypedPin` / `AddParameter` / `AddParameterPin`). The fork tags them
> `NIAGARAEDITOR_API` and ships a marker header
> `Engine/Plugins/FX/Niagara/Source/NiagaraEditor/Public/CkNiagaraAuthoring.h`. Both `CkParticles*.Build.cs` probe for
> that file and define `CK_WITH_PARTICLES=1`. On a **stock engine** the marker is absent → `CK_WITH_PARTICLES=0` → the
> editor template builder skips the code-built behavior module so CkFoundation still **builds cleanly** (the seed
> template is just an empty GPU emitter there). The engine edit is additive-only; safe to merge-forward.

---

## Architecture

```
UCkParticles_DataInterface (UNiagaraDataInterface, stateless)
  └─ ExecuteStage(BehaviorId, DeltaTime, Age, Lifetime, Position, Velocity, Seed)
        -> OutPosition, OutVelocity, OutColor, OutSize, OutScale, OutOrientation (quat),
           OutDynamic (float4 -> Particles.DynamicMaterialParameter), OutRotation (sprite degrees),
           OutMeshIndex, OutVisTag
       GPU: GetParameterDefinitionHLSL -> AppendTemplateHLSL(CkParticles_DataInterfaceTemplate.ush)
            template #includes CkParticles_Behaviors.ush -> Behaviors/*.ush   (logic lives here)
       CPU: VMExecuteStage -> NDICkParticlesLocal::ExecuteStage_CPU            (mirror of the same switch)
```

**Renderer selection (2026-07-12 fidelity evolution):** each behavior writes `VisTag` per particle and the
template's renderers draw only their tagged particles — `0` camera sprite (per-effect texture via
`User.SpriteMaterial`, the legacy default), `1` velocity-aligned sprite (streaks/tracers; stretch =
`Size.y`), `2` smoke sprite (translucent `M_CkParticles_SoftSmoke`, `Rotation` applies), `3` carrier mesh
(`MeshIndex` picks SM_CkParticles_ **Sweep/Tube/Shell/Disc**; `Scale` + `Orientation` apply; the meshes carry
`M_CkParticles_SweepErode` / `M_CkParticles_FresnelShell`). `Dynamic` drives the mesh/smoke materials:
x dissolve, y distortion, z UV-pan, w emissive boost — the exact idiom the marketplace "DissolveAdd"
materials animate via curves (Vefects M_VFX_DisAdd_Slash01 et al).

`Seed` is the particle's `Particles.UniqueID`. Hash it for per-particle randomness — `Common.ush` provides
`CkParticles_Rand(Seed, Salt)` → [0,1) and `CkParticles_RandDir(Seed)` → unit vector (24-bit math, bit-identical
between the GPU `.ush` and the C++ CPU mirror).

Shaders (`Source/CkParticles/Shaders/CkParticles/`, virtual path `/CkParticles`):
- `Common.ush` — `FCkParticles_StageInput/Output`, `CkParticles_NormalizedAge`, `CkParticles_Rand`, `CkParticles_RandDir`.
- `Behaviors/Behavior_*.ush` — Gravity (0), Swirl (1), Explosion (2), Fire (3), Fireworks (4), Galaxy (5),
  Beam (6, directional — aim via spawn rotation), Slash (7, arc crescent), Nova (8, shockwave ring),
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

## Adding a new behavior (pure C++/USF — no asset edits)

1. Author `Shaders/CkParticles/Behaviors/Behavior_<Name>.ush` with
   `FCkParticles_StageOutput CkParticles_Behavior_<Name>(FCkParticles_StageInput In)`.
2. `#include` it in `CkParticles_Behaviors.ush` and add a branch to `CkParticles_ExecuteStage` for the new id.
3. Mirror the exact math in `NDICkParticlesLocal::ExecuteStage_CPU` (`CkParticles_DataInterface.cpp`).
   **GPU and CPU MUST stay in lockstep** (same id, same math, same hashing).
4. Add the new `.ush` to `NDICkParticlesLocal::DependentShaderFiles` so `AppendCompileHash` busts the shader cache.

Then select it at runtime (`Spawn_BehaviorAtLocation(..., id, ...)`) or via a `UCkParticles_ScriptDefinition`'s
`_BehaviorId` + regenerate. One-shot archetypes should also be added to
`ck::particles::Get_BehaviorUsesBurstTemplate` (Naming header) so the spawn util routes them through
`PS_CkParticles_Template_Burst` (one instantaneous 96-burst per ~1.2 s loop, real `Age`) instead of the
continuous-rate seed; surplus burst particles a behavior doesn't use must be hidden (`Color/Size/Scale = 0`). **Self-driving** behaviors (write an absolute `O.Position` from Age/Seed, like Swirl /
Explosion / Galaxy) rely on the emitter being in **local space** — the template sets this, so they render where the
system is spawned. **Velocity-integrating** behaviors (Gravity) work in either space.

---

## Create Template System (fully code-built — no manual step)

**Editor → Editor Subsystems → `CkParticles_GeneratorSubsystem` → `Create Template System`** (or headless:
env `CK_PARTICLES_REBUILD_TEMPLATES=1` + toolbox `--test --test-pattern RebuildTemplateAssets`). With
`CK_WITH_PARTICLES=1` this runs the full asset pipeline — procedural textures → VFX master materials
(`CkParticles_MaterialGenerator.cpp`) → carrier meshes (`CkParticles_MeshGenerator.cpp`, MeshDescription-built) →
BOTH templates (`PS_CkParticles_Template` continuous + `PS_CkParticles_Template_Burst`) — entirely from C++
(`CkParticles_TemplateBuilder.cpp`):

- GPU emitter, **local space**, generous **fixed bounds** (`±3000`) so self-driving behaviors render at the spawn
  location and the system isn't frustum-culled.
- `User.BehaviorId` int + the DI wired as `User.ParticleScript`.
- The **behavior-call module** in Particle Update — built via the exported pin API: an Input parameter map fans to a
  Map Get (reads `User.ParticleScript`/`User.BehaviorId`, `Engine.DeltaTime`, `Particles.Age`/`Lifetime`/`Position`/
  `Velocity`/`UniqueID`) → the DI `ExecuteStage` member call → a Map Set (`Particles.Position`/`Velocity`/`Color`).
  The map threads through Map **Set** (`Source→Dest`); Map Get only taps it (it has no map output).
- Bakes the procedural VFX textures and assigns the master material (below) to the sprite renderer.

Idempotent — re-run any time; it overwrites in place. (On a stock engine the behavior module is skipped.)

---

## Procedural VFX textures + master material

**`Generate VFX Textures`** (same subsystem; also called by `Create Template System`) bakes a `UTexture2D` library
under `/CkFoundation/CkParticles/Textures/` **purely from noise/SDF/radial math** (`CkParticles_TextureGenerator.cpp`)
— no imported art: `Glow`, `Flare` (star), `Smoke` (FBM + erosion in A), `Electric` (ridged), `Streak`, `Ring` (SDF).
They are **grayscale** (so Particle Color tints them), `SRGB=false`, uncompressed (`TC_VectorDisplacementmap`).

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
GroundRing=14, LightningStrike=15, AuraSwirl=16.

**Aim-axis conventions** (these are baked into the behavior math — spawn rotation aims them):
MuzzleFlash/Tracer forward = **+X**; ImpactBurst surface normal = **+Z**;
GroundRing/LightningStrike/AuraSwirl ground plane = local **XY**; Beam travels down **+X**.

**Sprite material fallback:** `User.SpriteMaterial` (`ck::particles::Get_SpriteMaterialParameterName`) overrides
the sprite renderer's material per component. When unset, the renderer's own Material is used — a miss renders
the default glow, never an invisible effect.

`UCk_Utils_Particles_UE` (runtime module, BlueprintCallable / AngelScript-callable):
- `Spawn_BehaviorAtLocation(WorldContext, BehaviorId, Location, Rotation, Scale)` — spawns the seed template and sets
  `User.BehaviorId`.
- `Spawn_SystemAtLocation(WorldContext, System, BehaviorId, ...)` — same for an explicit (e.g. generated) system.

The **CkParticles gym** lives in CkTests (`Script/CkParticles/`, registered as "Particles" in `CkTests_GymRegistry.as`):
one station per behavior (0–16), each spawning the seed template with a fitting procedural texture; the recreation
stations (9–16) credit their marketplace exemplars in the station description. In-PIE exec:
`Ck_GymParticles_RestartAll`. The composition pattern for richer VFX (spells/trails) is multiple
`Spawn_BehaviorAtLocation` calls at one transform. Automated coverage: the PIE autotest
`CkAutoTest_Particles_SpawnAllBehaviors.as` spawns every id 0–16 and asserts a live component.

---

## Anti-patterns

1. Don't edit generated `PS_CkParticles_*` systems by hand — regeneration overwrites them. Edit the template
   (renderer/material) or the `.ush` (logic).
2. Don't hand-author the behavior-call module — it's code-built now. Re-run `Create Template System` instead.
3. Don't let the GPU `.ush` and CPU `ExecuteStage_CPU` diverge — two implementations of one behavior.
4. CPU sims can't run HLSL; only the C++ mirror runs there. If a behavior is GPU-only, document it.
