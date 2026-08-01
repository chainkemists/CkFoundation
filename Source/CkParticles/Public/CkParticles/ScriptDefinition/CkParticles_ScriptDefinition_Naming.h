#pragma once

#include "CoreMinimal.h"

namespace ck::particles
{
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

    inline auto Get_DefaultTemplateSystemObjectPath() -> FString
    {
        return TEXT("/CkFoundation/CkParticles/Templates/PS_CkParticles_Template.PS_CkParticles_Template");
    }

    // Burst-spawn variant of the seed template — the one-shot archetypes spawn through this.
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

    // Bound to the sprite renderer's material: unset falls back to the renderer's own Material, so a miss
    // renders the default glow rather than nothing.
    inline auto Get_SpriteMaterialParameterName() -> FName { return FName(TEXT("User.SpriteMaterial")); }

    // Keep this table in sync with the behavior roster in CkParticles_Behaviors.ush.
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

    // --------------------------------------------------------------------------------------------------------------
    // Behavior roster size. ONE definition so tests, gyms and docs can iterate the roster without each restating a
    // magic maximum id that then drifts when a behavior is added.
    // --------------------------------------------------------------------------------------------------------------
    inline constexpr int32 NumBehaviors = 18;                    // ids [0 .. NumBehaviors-1]
    inline constexpr int32 LastBehaviorId = NumBehaviors - 1;

    // --------------------------------------------------------------------------------------------------------------
    // Template cadence.
    //
    // A recreation is only as faithful as its spawn cadence: the source system's loop duration, particle lifetime
    // and burst count are as load-bearing as its shader. Rather than approximate a source with the nearest existing
    // template, a recreation whose cadence differs adds a ROW here and the editor builder emits a template for it.
    //
    // BurstCount 0 means the continuous spawn-rate stack (the legacy seed template) — its lifetime and loop come
    // from the emitter factory defaults, so the cadence fields are unused for that row.
    // --------------------------------------------------------------------------------------------------------------
    struct FCk_ParticlesTemplateSpec
    {
        const TCHAR* AssetName;
        float        LoopDuration;
        float        ParticleLifetime;
        int32        BurstCount;
    };

    inline auto Get_TemplateSpecs() -> TArrayView<const FCk_ParticlesTemplateSpec>
    {
        static const FCk_ParticlesTemplateSpec Specs[] =
        {
            // Continuous seed template — spawn-rate stack, no burst module.
            { TEXT("PS_CkParticles_Template"),        0.0f, 0.0f, 0 },
            // Shared burst template for the multi-particle one-shot archetypes.
            { TEXT("PS_CkParticles_Template_Burst"),  1.2f, 1.2f, 96 },
            // Single-particle burst: Vefects NS_Lightning_Range's exact cadence (Loop Duration 1.0, Lifetime 1.1,
            // Spawn Count 1). Reusable by any recreation of a one-sprite, one-second-loop system.
            { TEXT("PS_CkParticles_Template_Single"), 1.0f, 1.1f, 1  },
        };
        return MakeArrayView(Specs);
    }

    inline auto Get_TemplateSystemObjectPath(const TCHAR* InAssetName) -> FString
    {
        return FString::Printf(TEXT("/CkFoundation/CkParticles/Templates/%s.%s"), InAssetName, InAssetName);
    }

    inline auto Get_SingleBurstTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_Single"));
    }

    // Which template a behavior spawns through. Behaviors whose source system is one instantaneous particle on a
    // fixed loop route to the single-burst template; the multi-particle one-shots keep the shared burst template.
    inline auto Get_BehaviorTemplateSystemObjectPath(const int32 InBehaviorId) -> FString
    {
        if (InBehaviorId == 17) // LightningRange — one sprite, 1.0s loop, 1.1s lifetime
        { return Get_SingleBurstTemplateSystemObjectPath(); }

        return Get_BehaviorUsesBurstTemplate(InBehaviorId)
            ? Get_BurstTemplateSystemObjectPath()
            : Get_DefaultTemplateSystemObjectPath();
    }

    // --------------------------------------------------------------------------------------------------------------
    // Behavior -> generated CkUsf look.
    //
    // A behavior whose visual identity is a hand-authored shader binds a generated CkUsf master here; the spawn path
    // resolves it and binds it through User.SpriteMaterial, the SAME mechanism the procedural-texture material
    // instances use. NAME_None means the behavior keeps the procedural-texture path.
    //
    // The path convention is CkUsf's (ck::usf::Get_GeneratedMasterObjectPath). It is mirrored rather than called so
    // CkParticles need not depend on CkUsf/CkEcs/CkGraphics; the CkParticles binding test asserts the mirrored path
    // actually resolves, so a convention change fails loudly instead of silently rendering the default material.
    // --------------------------------------------------------------------------------------------------------------
    inline auto Get_BehaviorLookName(const int32 InBehaviorId) -> FName
    {
        switch (InBehaviorId)
        {
            case 17: return FName(TEXT("RingDissolveAdd")); // Vefects M_VFX_DisAdd_Ring04 recreation
            default: return NAME_None;
        }
    }

    inline auto Get_GeneratedLookMasterObjectPath(const FName InLookName) -> FString
    {
        return FString::Printf(TEXT("/CkFoundation/CkUsf/GeneratedLooks/M_CkUsf_Look_%s.M_CkUsf_Look_%s"),
            *InLookName.ToString(), *InLookName.ToString());
    }
}
