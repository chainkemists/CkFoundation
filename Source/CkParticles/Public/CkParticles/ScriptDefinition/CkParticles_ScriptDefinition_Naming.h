#pragma once

#include "CoreMinimal.h"

namespace ck::particles
{
    // Generated systems live in CkFoundation plugin content under GeneratedSystems/.
    inline auto Get_GeneratedSystemPackagePath(const FName InScriptName) -> FString
    {
        return FString::Printf(TEXT("/CkFoundation/CkParticles/GeneratedSystems/PS_CkParticles_%s"),
            *InScriptName.ToString());
    }

    inline auto Get_GeneratedSystemObjectPath(const FName InScriptName) -> FString
    {
        const auto Pkg = Get_GeneratedSystemPackagePath(InScriptName);
        return FString::Printf(TEXT("%s.PS_CkParticles_%s"), *Pkg, *InScriptName.ToString());
    }

    // Seed template the generator duplicates when a UCkParticles_ScriptDefinition leaves _TemplateSystem unset.
    // This is the one hand-authored Niagara System (see the seed-template setup notes).
    inline auto Get_DefaultTemplateSystemObjectPath() -> FString
    {
        return TEXT("/CkFoundation/CkParticles/Templates/PS_CkParticles_Template.PS_CkParticles_Template");
    }

    // Burst-spawn variant of the seed template (one instantaneous burst per ~1.2s loop instead of a continuous
    // rate) — the one-shot archetypes (slash/impact/muzzle/...) spawn through this. Same DI, same BehaviorId
    // contract, same renderer set.
    inline auto Get_BurstTemplateSystemObjectPath() -> FString
    {
        return TEXT("/CkFoundation/CkParticles/Templates/PS_CkParticles_Template_Burst.PS_CkParticles_Template_Burst");
    }

    // Code-built VFX master materials (Generate_AllVfxMaterials), e.g. "SweepErode"/"SoftSmoke"/"FresnelShell".
    inline auto Get_VfxMasterMaterialObjectPath(const FName InName) -> FString
    {
        return FString::Printf(TEXT("/CkFoundation/CkParticles/Materials/M_CkParticles_%s.M_CkParticles_%s"),
            *InName.ToString(), *InName.ToString());
    }

    // Code-built VFX carrier meshes (Generate_AllVfxMeshes), e.g. "Sweep"/"Tube"/"Shell"/"Disc".
    inline auto Get_VfxMeshObjectPath(const FName InName) -> FString
    {
        return FString::Printf(TEXT("/CkFoundation/CkParticles/Meshes/SM_CkParticles_%s.SM_CkParticles_%s"),
            *InName.ToString(), *InName.ToString());
    }

    // Procedural VFX textures (baked by Generate_AllVfxTextures). InName is the suffix, e.g. "Glow"/"Smoke"/"Electric".
    inline auto Get_VfxTextureObjectPath(const FName InName) -> FString
    {
        return FString::Printf(TEXT("/CkFoundation/CkParticles/Textures/T_CkParticles_%s.T_CkParticles_%s"),
            *InName.ToString(), *InName.ToString());
    }

    // Per-texture material instances of the VFX master (built by Create Template System, one per baked texture).
    inline auto Get_TextureMaterialInstanceObjectPath(const FName InName) -> FString
    {
        return FString::Printf(TEXT("/CkFoundation/CkParticles/Materials/MI_CkParticles_%s.MI_CkParticles_%s"),
            *InName.ToString(), *InName.ToString());
    }

    // The sprite renderer's material is bound to this system User parameter, so a spawn can swap the per-effect
    // texture (material instance) per component via UNiagaraComponent::SetVariableMaterial. Falls back to the
    // renderer's direct Material if unset — so a miss renders the default glow, never invisible.
    inline auto Get_SpriteMaterialParameterName() -> FName { return FName(TEXT("User.SpriteMaterial")); }

    // One-shot archetypes (a single arc per ~1.2s loop) spawn through the burst template; everything else uses
    // the continuous-rate seed. Spawn_BehaviorAtLocation routes automatically — keep this table in sync with the
    // behavior roster in CkParticles_Behaviors.ush.
    inline auto Get_BehaviorUsesBurstTemplate(const int32 InBehaviorId) -> bool
    {
        switch (InBehaviorId)
        {
            case 7:  // Slash
            case 10: // ImpactBurst
            case 13: // SparksBurst
            case 14: // GroundRing
            case 15: // LightningStrike
                return true;
            default:
                return false;
        }
    }
}
