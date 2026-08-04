#pragma once

#include "CoreMinimal.h"

namespace ck::particles_editor
{
    // Tops up the per-behavior tuning DataAsset library under /CkFoundation/CkParticles/Tuning/ — one
    // UCkParticles_TuningDefinition per roster id, named by ck::particles::Get_BehaviorTuningAssetObjectPath, which
    // the runtime spawn path resolves for every behavior it spawns.
    //
    // NEVER overwrites: an existing asset carries a human's tuning and is left byte-untouched, so this is safe to
    // run on every regeneration. Only missing ones are created, with identity defaults. Unlike the template build
    // this is plain asset authoring, so it is NOT gated on CK_WITH_PARTICLES and runs on a stock engine.
    CKPARTICLESEDITOR_API auto Generate_AllTuningAssets() -> void;
}
