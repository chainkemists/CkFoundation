#pragma once

#include "CoreMinimal.h"

class UNiagaraSystem;

namespace ck::particles_editor
{
    // Builds the seed template Niagara Systems entirely from C++ (no hand-authored assets): GPU emitter, the
    // renderer set selected per particle by Particles.VisibilityTag, User.BehaviorId, and a code-built module
    // calling the CkParticles DI's ExecuteStage (logic stays in USF + the C++ CPU mirror). Two ship: the
    // continuous-rate seed (PS_CkParticles_Template) and the one-burst-per-loop variant (..._Burst).
    //
    // FRAGILE: this drives mostly-editor-only Niagara graph APIs. Re-verify against engine source on a major bump.

    // Full pipeline: procedural textures -> master materials -> carrier meshes -> both template systems.
    // Returns true only when every stage succeeded. Idempotent.
    CKPARTICLESEDITOR_API auto Build_AllTemplateSystems() -> bool;

    // Legacy single-entry: runs the full pipeline and returns the continuous seed template (nullptr on failure).
    CKPARTICLESEDITOR_API auto Build_TemplateSystem() -> UNiagaraSystem*;
}
