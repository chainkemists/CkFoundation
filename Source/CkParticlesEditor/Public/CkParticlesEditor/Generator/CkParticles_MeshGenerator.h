#pragma once

#include "CoreMinimal.h"

namespace ck::particles_editor
{
    // Code-builds the VFX carrier meshes (UStaticMesh assets) entirely from parametric math — the shaped
    // geometry the 2026-07-12 corpus analysis showed 44% of marketplace systems render through — saved under
    // /CkFoundation/CkParticles/Meshes/ and consumed by the template's mesh renderer (Particles.MeshIndex):
    //
    //   [0] SM_CkParticles_Sweep — crescent arc band in the XY plane centered on +X (the anime-slash carrier;
    //       modeled on marketplace SM_VFX_Slash01). UV: U across the band, V along the arc — the SweepErode
    //       material pans V so the streak texture travels along the swing.
    //   [1] SM_CkParticles_Tube  — open cylinder along +X (beams/columns). UV: U around, V along the length.
    //   [2] SM_CkParticles_Shell — sphere (domes/shockwaves/auras; FresnelShell material). UV: spherical.
    //   [3] SM_CkParticles_Disc  — flat ring in the XY plane (ground rings/AoE). UV: U around, V radial.
    //
    // Slot materials reference the VFX master materials, so run AFTER Generate_AllVfxMaterials.
    // Editor-only. Idempotent — overwrites in place.
    CKPARTICLESEDITOR_API auto Generate_AllVfxMeshes() -> void;
}
