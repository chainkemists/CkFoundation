#include "CkParticles/Utils/CkParticles_Utils.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

#include "Materials/MaterialInterface.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CkParticles_Utils)

// --------------------------------------------------------------------------------------------------------------------

UNiagaraComponent*
    UCk_Utils_Particles_UE::
    Spawn_BehaviorAtLocation(
        const UObject* InWorldContextObject,
        int32          InBehaviorId,
        FVector        InLocation,
        FRotator       InRotation,
        FVector        InScale,
        FName          InTextureName)
{
    const auto TemplatePath = ck::particles::Get_BehaviorUsesBurstTemplate(InBehaviorId)
        ? ck::particles::Get_BurstTemplateSystemObjectPath()
        : ck::particles::Get_DefaultTemplateSystemObjectPath();
    auto* System = LoadObject<UNiagaraSystem>(nullptr, *TemplatePath);
    return Spawn_SystemAtLocation(InWorldContextObject, System, InBehaviorId, InLocation, InRotation, InScale, InTextureName);
}

UNiagaraComponent*
    UCk_Utils_Particles_UE::
    Spawn_SystemAtLocation(
        const UObject* InWorldContextObject,
        UNiagaraSystem* InSystem,
        int32           InBehaviorId,
        FVector         InLocation,
        FRotator        InRotation,
        FVector         InScale,
        FName           InTextureName)
{
    if (NOT IsValid(InWorldContextObject)) { return nullptr; }
    if (NOT IsValid(InSystem)) { return nullptr; }

    auto* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (NOT IsValid(World)) { return nullptr; }

    constexpr auto bAutoDestroy   = false;
    constexpr auto bAutoActivate  = true;
    constexpr auto PoolMethod     = ENCPoolMethod::None;
    constexpr auto bPreCullCheck  = true;

    auto* Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        World, InSystem, InLocation, InRotation, InScale,
        bAutoDestroy, bAutoActivate, PoolMethod, bPreCullCheck);

    if (IsValid(Component))
    {
        Component->SetIntParameter(TEXT("User.BehaviorId"), InBehaviorId);

        // Per-effect texture: swap the sprite material instance bound to the renderer's User param. Null-tolerant —
        // a missing instance leaves the renderer's default (Glow) material, never invisible.
        if (InTextureName != NAME_None)
        {
            const auto MicPath = ck::particles::Get_TextureMaterialInstanceObjectPath(InTextureName);
            if (auto* Mic = LoadObject<UMaterialInterface>(nullptr, *MicPath))
            { Component->SetVariableMaterial(ck::particles::Get_SpriteMaterialParameterName(), Mic); }
        }
    }

    return Component;
}
