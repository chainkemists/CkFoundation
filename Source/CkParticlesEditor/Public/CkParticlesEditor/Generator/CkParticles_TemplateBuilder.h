#pragma once

#include "CoreMinimal.h"

class UNiagaraSystem;

namespace ck::particles_editor
{
    // Builds the seed template Niagara Systems entirely from C++ (no hand-authored assets):
    //   empty system -> GPU emitter -> renderer set selected per particle via Particles.VisibilityTag
    //   (0 camera sprite, 1 velocity-aligned sprite, 2 smoke sprite, 3 mesh w/ Particles.MeshIndex carriers)
    //   -> User.BehaviorId int -> a code-built module calling the CkParticles DI's ExecuteStage and writing
    //   Position/Velocity/Color/SpriteSize/Scale/MeshOrientation/DynamicMaterialParameter/SpriteRotation/
    //   MeshIndex/VisibilityTag (logic stays in USF + the C++ CPU mirror).
    //
    // Two templates ship: the continuous-rate seed (PS_CkParticles_Template) and the burst variant
    // (PS_CkParticles_Template_Burst — one instantaneous burst per loop, for the one-shot archetypes the
    // 2026-07-12 corpus analysis showed dominate marketplace VFX at 91%).
    //
    // FRAGILE: this drives mostly-editor-only Niagara graph APIs. Re-verify against engine source on a major bump.

    // Full pipeline: procedural textures -> master materials -> carrier meshes -> both template systems.
    // Returns true when every stage succeeded. Idempotent.
    CKPARTICLESEDITOR_API auto Build_AllTemplateSystems() -> bool;

    // Legacy single-entry: runs the full pipeline and returns the continuous seed template (nullptr on failure).
    CKPARTICLESEDITOR_API auto Build_TemplateSystem() -> UNiagaraSystem*;
}
