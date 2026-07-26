#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkParticles_Utils.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

// --------------------------------------------------------------------------------------------------------------------
// Runtime spawn helpers: they wrap the standard Niagara spawn path and set User.BehaviorId. The behavior roster
// and each behavior's aim-axis convention live in CkParticles/CLAUDE.md.
// --------------------------------------------------------------------------------------------------------------------
UCLASS()
class CKPARTICLES_API UCk_Utils_Particles_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Spawns the CkParticles seed template (PS_CkParticles_Template) at a world location, selecting the behavior by
    // Id via User.BehaviorId. Returns the spawned component (nullptr if the template is missing or spawn fails).
    UFUNCTION(BlueprintCallable, Category = "Ck|Particles",
        meta = (WorldContext = "InWorldContextObject"),
        DisplayName = "[Ck][Particles] Spawn Behavior At Location")
    static UNiagaraComponent*
    Spawn_BehaviorAtLocation(
        const UObject* InWorldContextObject,
        int32 InBehaviorId,
        FVector InLocation,
        FRotator InRotation = FRotator::ZeroRotator,
        FVector InScale = FVector(1.f),
        FName InTextureName = NAME_None);

    // Spawns an explicit CkParticles system (e.g. a generated PS_CkParticles_<Name>) instead of the seed template,
    // still selecting the behavior by Id. Use when you have a per-effect generated asset.
    UFUNCTION(BlueprintCallable, Category = "Ck|Particles",
        meta = (WorldContext = "InWorldContextObject"),
        DisplayName = "[Ck][Particles] Spawn System At Location")
    static UNiagaraComponent*
    Spawn_SystemAtLocation(
        const UObject* InWorldContextObject,
        UNiagaraSystem* InSystem,
        int32 InBehaviorId,
        FVector InLocation,
        FRotator InRotation = FRotator::ZeroRotator,
        FVector InScale = FVector(1.f),
        FName InTextureName = NAME_None);
};
