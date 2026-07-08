#pragma once

#include "CoreMinimal.h"

namespace ck::particles_editor
{
    // Bakes a starter library of procedural VFX textures (UTexture2D assets) entirely from C++ — no painted art, no
    // imported images. Each is built from noise / SDF / radial math (see CkParticles_TextureGenerator.cpp) and saved
    // under /CkFoundation/CkParticles/Textures/. These are the building blocks the VFX master materials combine
    // (shape x detail x erosion x ParticleColor). Editor-only (uses FTextureSource). Idempotent — overwrites in place.
    CKPARTICLESEDITOR_API auto Generate_AllVfxTextures() -> void;
}
