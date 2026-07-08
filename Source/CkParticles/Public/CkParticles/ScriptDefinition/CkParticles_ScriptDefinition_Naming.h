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
}
