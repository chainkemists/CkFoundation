#pragma once

#include "CoreMinimal.h"

class UCkParticles_ScriptDefinition;
class UNiagaraSystem;

namespace ck::particles_editor
{
    struct FGenerateResult
    {
        int32 NumGenerated = 0;
        int32 NumSkipped = 0;
        TArray<FString> Errors;
    };

    // Discovers all UCkParticles_ScriptDefinition assets and generates/refreshes a system per definition.
    CKPARTICLESEDITOR_API auto Generate_AllParticleSystems() -> FGenerateResult;

    // Generates/refreshes one system by duplicating the definition's template and patching BehaviorId.
    // Returns nullptr on failure (logged).
    CKPARTICLESEDITOR_API auto Generate_ParticleSystem(UCkParticles_ScriptDefinition* InDef) -> UNiagaraSystem*;
}
