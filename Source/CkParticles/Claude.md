# CkParticles

**Purpose:** Author Niagara particle/system **logic in C++ + USF/USH**, not the Niagara graph editor. A custom
Data Interface exposes one generic, pure `ExecuteStage` function; the actual behavior is HLSL in `.ush` (GPU)
mirrored by C++ (CPU). Thin "template" Niagara System assets call `ExecuteStage` with a per-asset `BehaviorId`,
and `CkParticlesEditor` stamps out one system per `UCkParticles_ScriptDefinition` by duplicating the template.

This is the particle analog of **CkUsf** (which authors materials via `.ush` instead of the material editor).

**Depends on:** `Core`, `CoreUObject`, `Engine`, `RenderCore`, `RHI`, `Projects`, `GameplayTags`, `Niagara`, `CkCore`, `CkLog`.
**Engine:** built/verified against the UE **5.7** fork (`E:\UE_5.7`). The Niagara DI API is version-sensitive
(`GetFunctionsInternal`, context/`GPUParamInfo` HLSL hooks, `AppendTemplateHLSL`); re-verify against engine
source on a major bump.

---

## Architecture

```
UCkParticles_DataInterface (UNiagaraDataInterface, stateless)
  └─ ExecuteStage(BehaviorId, DeltaTime, Age, Lifetime, Position, Velocity) -> OutPosition, OutVelocity, OutColor
       GPU: GetParameterDefinitionHLSL -> AppendTemplateHLSL(CkParticles_DataInterfaceTemplate.ush)
            template #includes CkParticles_Behaviors.ush -> Behaviors/*.ush   (logic lives here)
       CPU: VMExecuteStage -> NDICkParticlesLocal::ExecuteStage_CPU            (mirror of the same switch)
```

Shaders (`Source/CkParticles/Shaders/CkParticles/`, mapped to virtual path `/CkParticles`):
- `Common.ush` — `FCkParticles_StageInput/Output` + helpers.
- `Behaviors/Behavior_Gravity.ush` (id 0), `Behaviors/Behavior_Swirl.ush` (id 1).
- `CkParticles_Behaviors.ush` — `#include`s behaviors + `CkParticles_ExecuteStage` dispatch (the switch).
- `CkParticles_DataInterfaceTemplate.ush` — the DI's generated GPU function wrapper.

---

## Adding a new behavior (pure C++/USF — no asset edits)

1. Author `Shaders/CkParticles/Behaviors/Behavior_<Name>.ush` with
   `FCkParticles_StageOutput CkParticles_Behavior_<Name>(FCkParticles_StageInput In)`.
2. `#include` it in `CkParticles_Behaviors.ush` and add a branch to `CkParticles_ExecuteStage` for the new id.
3. Mirror the exact math in `NDICkParticlesLocal::ExecuteStage_CPU` (`CkParticles_DataInterface.cpp`) for the
   CPU path. **GPU and CPU MUST stay in lockstep.**
4. Add the new `.ush` to `NDICkParticlesLocal::DependentShaderFiles` so `AppendCompileHash` busts the cache.

Then point a `UCkParticles_ScriptDefinition` at the new `BehaviorId` and regenerate.

---

## Generating systems

1. Author a `UCkParticles_ScriptDefinition` (set `_ScriptName`, `_BehaviorId`, optionally `_TemplateSystem`).
2. Editor → **Editor Subsystems** → `CkParticles_GeneratorSubsystem` → **Generate Particle Systems**
   (or call `ck::particles_editor::Generate_AllParticleSystems()`).
3. One `PS_CkParticles_<ScriptName>` is written under
   `/CkFoundation/CkParticles/GeneratedSystems/` (duplicate of the template, `User.BehaviorId` patched).

---

## ONE-TIME: create the seed template system (Hybrid: code scaffold + one manual module)

The whole system is built around a single template Niagara System. ~90% of it is built from C++ by the
**`Create Template System`** button; the one piece that must be added in-editor is the behavior-call module,
because the NiagaraEditor APIs needed to build it (`SetCustomExpressionForFunctionInput`,
`UNiagaraGraph::FindOutputNode`, `UNiagaraNodeCustomHlsl::VirtualIncludeFilePaths`) are not exported to external
modules. Do this once:

1. **Editor → Editor Subsystems → `CkParticles_GeneratorSubsystem` → `Create Template System`.**
   Writes `/CkFoundation/CkParticles/Templates/PS_CkParticles_Template` with: a GPU emitter, sprite renderer,
   default spawn/init modules, a `User.BehaviorId` int, and the DI already wired as `User.ParticleScript`.
   (Matches `ck::particles::Get_DefaultTemplateSystemObjectPath()`.)
2. Open `PS_CkParticles_Template`. In **Particle Update**, add a **Scratch Pad** module (`+ → New Scratch Pad
   Module`). In its graph:
   - Map-get `User.ParticleScript` (already present — type **"Ck Particle Script"**), `User.BehaviorId`,
     `Engine.DeltaTime`, `Particles.Age`, `Particles.Lifetime`, `Particles.Position`, `Particles.Velocity`.
   - Drag off `User.ParticleScript` → call **ExecuteStage**, wiring the gets above into its inputs.
   - Map-set `Particles.Position ← OutPosition`, `Particles.Velocity ← OutVelocity`, `Particles.Color ← OutColor`.
   - Place it **after** the default modules that write Position/Velocity (last writer wins).
3. **Apply** the scratch module and **Save**. Particles should fall (BehaviorId 0 = Gravity) in the preview — that
   confirms the GPU `.ush` compiled.

> Re-running `Create Template System` rebuilds the scaffold and **wipes the manual module** — only re-run it if you
> change the scaffold; otherwise edit the template directly.
>
> A `UCkParticles_ScriptDefinition` may override `_TemplateSystem` to duplicate a different template (e.g. a mesh
> renderer variant). The default path above is used when it's left unset.

---

## Anti-patterns

1. Don't edit generated `PS_CkParticles_*` systems by hand — regeneration overwrites them. Edit the template
   (shape/renderer) or the `.ush` (logic).
2. Don't let the GPU `.ush` and CPU `ExecuteStage_CPU` diverge — they are two implementations of one behavior.
3. CPU sims can't run HLSL; only the C++ mirror runs there. If a behavior is GPU-only, document it.
