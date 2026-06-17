#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CkParticles_ScriptDefinition.generated.h"

class UNiagaraSystem;

// --------------------------------------------------------------------------------------------------------------------
// Describes one generated particle system: which behavior it runs and which template it is built from.
// The generator (CkParticlesEditor) duplicates _TemplateSystem and patches the BehaviorId user parameter.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKPARTICLES_API UCkParticles_ScriptDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    // Logical name; defaults to the asset name if None. Drives the generated system's package path.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    FName _ScriptName = NAME_None;

    // BehaviorId handed to the DI's ExecuteStage — selects the behavior in CkParticles_Behaviors.ush.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    int32 _BehaviorId = 0;

    // The hand-authored template Niagara System the generator duplicates. If unset, the generator falls back
    // to the default template path (see CkParticles_ScriptDefinition_Naming.h).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    TSoftObjectPtr<UNiagaraSystem> _TemplateSystem;

    // Name of the int User parameter on the template that carries BehaviorId. The generator sets its default
    // on the duplicated system so the same template can drive every behavior. Fully-qualified ("User.BehaviorId");
    // a bare name ("BehaviorId") is also accepted and auto-prefixed by the generator.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    FName _BehaviorIdParameterName = TEXT("User.BehaviorId");

    UFUNCTION(BlueprintCallable, Category = "CkParticles",
              DisplayName = "[Ck][Particles] Get Effective Script Name")
    FName Get_EffectiveScriptName() const;
};
