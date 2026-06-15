#pragma once

#include "CoreMinimal.h"

class UNiagaraSystem;

namespace ck::particles_editor
{
    // Builds the seed template Niagara System entirely from C++ (no hand-authored asset):
    //   empty system -> GPU emitter (default spawn/init modules + sprite renderer) -> User.BehaviorId int ->
    //   a "Set Particles.Position/Velocity/Color" module whose inputs are custom-HLSL expressions calling the
    //   CkParticles behavior .ush (logic stays in USF). Saves to the default template path.
    //
    // FRAGILE: this drives mostly-editor-only Niagara graph APIs. Re-verify against engine source on a major bump.
    // Returns nullptr on failure (logged).
    CKPARTICLESEDITOR_API auto Build_TemplateSystem() -> UNiagaraSystem*;
}
