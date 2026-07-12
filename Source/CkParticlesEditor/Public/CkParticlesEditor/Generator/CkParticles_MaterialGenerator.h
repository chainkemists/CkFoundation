#pragma once

#include "CoreMinimal.h"

namespace ck::particles_editor
{
    // Code-builds the VFX master materials the marketplace recipes revealed as the look-carrying machinery
    // (2026-07-12 corpus analysis: 52% of systems animate Dynamic Material Parameters), saved under
    // /CkFoundation/CkParticles/Materials/:
    //
    //   M_CkParticles_SweepErode   — the anime-slash/beam material: main texture panned along V by
    //                                DynamicParameter.z, noise-dissolved by .x, UV-distorted by .y, emissive
    //                                boosted by .w; translucent, unlit, two-sided. (Modeled on the marketplace
    //                                "DissolveAdd" family, e.g. Vefects M_VFX_DisAdd_Slash01.)
    //   M_CkParticles_SoftSmoke    — translucent depth-faded smoke: density in RGB, erosion mask in A,
    //                                dissolve threshold via DynamicParameter.x, tinted by ParticleColor.
    //   M_CkParticles_FresnelShell — additive rim-lit shell for domes/shockwaves/auras: fresnel rim x noise
    //                                erosion x ParticleColor, boosted by DynamicParameter.w.
    //
    // DynamicParameter semantics match FCkParticles_StageOutput.Dynamic (x dissolve, y distortion, z pan,
    // w boost) with a (0,0,0,0) default so a behavior that writes nothing gets the un-eroded, un-panned look.
    // Editor-only. Idempotent — overwrites in place. Run AFTER Generate_AllVfxTextures (samples those textures).
    CKPARTICLESEDITOR_API auto Generate_AllVfxMaterials() -> void;
}
