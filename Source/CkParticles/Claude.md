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
        -> OutPosition, OutVelocity, OutColor
       GPU: GetParameterDefinitionHLSL -> AppendTemplateHLSL(CkParticles_DataInterfaceTemplate.ush)
            template #includes CkParticles_Behaviors.ush -> Behaviors/*.ush   (logic lives here)
       CPU: VMExecuteStage -> NDICkParticlesLocal::ExecuteStage_CPU            (mirror of the same switch)
```

`Seed` is the particle's `Particles.UniqueID`. Hash it for per-particle randomness — `Common.ush` provides
`CkParticles_Rand(Seed, Salt)` → [0,1) and `CkParticles_RandDir(Seed)` → unit vector (24-bit math, bit-identical
between the GPU `.ush` and the C++ CPU mirror).

Shaders (`Source/CkParticles/Shaders/CkParticles/`, virtual path `/CkParticles`):
- `Common.ush` — `FCkParticles_StageInput/Output`, `CkParticles_NormalizedAge`, `CkParticles_Rand`, `CkParticles_RandDir`.
- `Behaviors/Behavior_*.ush` — Gravity (0), Swirl (1), Explosion (2), Fire (3), Fireworks (4), Galaxy (5),
  Beam (6, directional — aim via spawn rotation), Slash (7, arc crescent), Nova (8, shockwave ring).
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
`_BehaviorId` + regenerate. **Self-driving** behaviors (write an absolute `O.Position` from Age/Seed, like Swirl /
Explosion / Galaxy) rely on the emitter being in **local space** — the template sets this, so they render where the
system is spawned. **Velocity-integrating** behaviors (Gravity) work in either space.

---

## Create Template System (fully code-built — no manual step)

**Editor → Editor Subsystems → `CkParticles_GeneratorSubsystem` → `Create Template System`.** With
`CK_WITH_PARTICLES=1` this builds `/CkFoundation/CkParticles/Templates/PS_CkParticles_Template` entirely from C++
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

`UCk_Utils_Particles_UE` (runtime module, BlueprintCallable / AngelScript-callable):
- `Spawn_BehaviorAtLocation(WorldContext, BehaviorId, Location, Rotation, Scale)` — spawns the seed template and sets
  `User.BehaviorId`.
- `Spawn_SystemAtLocation(WorldContext, System, BehaviorId, ...)` — same for an explicit (e.g. generated) system.

The **CkParticles gym** lives in CkTests (`Script/CkParticles/`, registered as "Particles" in `CkTests_GymRegistry.as`):
one station per behavior plus a composite **Spell Cast** station that **layers** behaviors (Explosion + Beam) at one
point — the composition pattern for richer VFX (spells/trails) is just multiple `Spawn_BehaviorAtLocation` calls at one
transform. Cycle to it with `Ck_Gym_GoTo Particles`.

---

## Anti-patterns

1. Don't edit generated `PS_CkParticles_*` systems by hand — regeneration overwrites them. Edit the template
   (renderer/material) or the `.ush` (logic).
2. Don't hand-author the behavior-call module — it's code-built now. Re-run `Create Template System` instead.
3. Don't let the GPU `.ush` and CPU `ExecuteStage_CPU` diverge — two implementations of one behavior.
4. CPU sims can't run HLSL; only the C++ mirror runs there. If a behavior is GPU-only, document it.
