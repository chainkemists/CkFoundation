#pragma once

#include "CoreMinimal.h"

namespace ck::particles_editor
{
    // Code-builds the VFX carrier meshes from parametric math under /CkFoundation/CkParticles/Meshes/, indexed
    // by the template mesh renderer's Particles.MeshIndex in this order: [0] Sweep (crescent arc band, U across
    // the band, V along the arc so the SweepErode material can pan the streak along the swing), [1] Tube (open
    // cylinder along +X, U around, V along the length), [2] Shell (sphere), [3] Disc (flat ring, U around,
    // V radial). Editor-only, idempotent. Run AFTER Generate_AllVfxMaterials: the mesh slots reference them.
    CKPARTICLESEDITOR_API auto Generate_AllVfxMeshes() -> void;
}
