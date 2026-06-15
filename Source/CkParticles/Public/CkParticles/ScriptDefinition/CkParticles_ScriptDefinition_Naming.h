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
}
