#pragma once

#include "CoreMinimal.h"

namespace ck::particles_editor
{
    // Code-builds the VFX master materials (SweepErode / SoftSmoke / FresnelShell) under
    // /CkFoundation/CkParticles/Materials/. DynamicParameter semantics match FCkParticles_StageOutput.Dynamic
    // (x dissolve, y distortion, z pan, w boost), defaulted to zero so a behavior that writes nothing still
    // gets the un-eroded, un-panned look. Editor-only, idempotent — overwrites in place.
    // Run AFTER Generate_AllVfxTextures: the materials sample those textures.
    CKPARTICLESEDITOR_API auto Generate_AllVfxMaterials() -> void;
}
