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
    // Row-level renderer overrides.
    //
    // The shared renderer set (VisTags 0..SharedRendererVisTag_Max) is behavior-agnostic on purpose: every
    // template carries it, so nothing effect-specific may be added to it. A recreation whose source draws through
    // renderers the shared set cannot express declares its OWN renderers on its cadence row instead, and the
    // builder emits them for that row only — no other template gains a renderer it never asked for.
    //
    // Two kinds cover what a recreation actually needs beyond the shared set: a mesh renderer carrying ONE named
    // generated mesh drawn with ONE named CkUsf look, and a velocity-aligned sprite drawn with a look. Both bind
    // the look master EXPLICITLY (not through User.SpriteMaterial), because a row that declares several renderers
    // needs a different material on each and one user parameter cannot carry them.
    // ----------------------------------------------------------------------------------------------------------
    enum class ECk_ParticlesRenderer_Kind : uint8
    {
        Mesh,                  // MeshName -> SM_CkParticles_<MeshName>; Particles.Scale + MeshOrientation apply
        VelocityAlignedSprite, // stretch is Particles.SpriteSize.y along motion
    };

    struct FCk_ParticlesRendererSpec
    {
        ECk_ParticlesRenderer_Kind Kind;
        int32        VisTag;
        const TCHAR* MeshName; // Mesh kind only
        const TCHAR* LookName; // generated CkUsf master, via Get_GeneratedLookMasterObjectPath
    };

    // Vefects NS_BasicAttack: four crescent-mesh slash layers, each with its own DissolveAdd look, plus the
    // velocity-aligned spark sprite. VisTags continue after the shared set (recipe Cookbook/NS_BasicAttack.md §6).
    inline auto Get_SlashRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::Mesh,                  5, TEXT("Crescent"), TEXT("SlashDisAdd01") },
            { ECk_ParticlesRenderer_Kind::Mesh,                  6, TEXT("Crescent"), TEXT("SlashDisAdd02") },
            { ECk_ParticlesRenderer_Kind::Mesh,                  7, TEXT("Crescent"), TEXT("SlashDisAdd04") },
            { ECk_ParticlesRenderer_Kind::Mesh,                  8, TEXT("Crescent"), TEXT("WindDisAdd02")  },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 9, nullptr,          TEXT("PartDisAdd04")  },
        };
        return MakeArrayView(Specs);
    }

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

        // Empty on every row that is satisfied by the shared renderer set.
        TArrayView<const FCk_ParticlesRendererSpec> RendererOverrides;
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
            // Vefects NS_BasicAttack: five emitters burst together at loop start, 19 particles per loop, longest
            // layer 0.5 s on a 1.0 s loop. Its five renderers are row-declared rather than shared.
            { TEXT("PS_CkParticles_Template_Slash"),  1.0f, 0.5f, 19, Get_SlashRendererSpecs() },
        };
        return MakeArrayView(Specs);
    }

    // ----------------------------------------------------------------------------------------------------------
    // VisTag bounds. The shared set every template carries is 0..SharedRendererVisTag_Max; row-declared renderers
    // allocate above it. A VisTag past the roster maximum draws through NO renderer — invisible, and silent — so
    // tests bound against Get_RosterVisTag_Max() rather than restating a literal that then drifts.
    // ----------------------------------------------------------------------------------------------------------
    inline constexpr int32 SharedRendererVisTag_Max = 4;

    inline auto Get_RosterVisTag_Max() -> int32
    {
        auto Max = SharedRendererVisTag_Max;
        for (const auto& Spec : Get_TemplateSpecs())
        {
            for (const auto& Renderer : Spec.RendererOverrides)
            { Max = FMath::Max(Max, Renderer.VisTag); }
        }
        return Max;
    }

    inline auto Get_TemplateSystemObjectPath(const TCHAR* InAssetName) -> FString
    {
        return FString::Printf(TEXT("/CkFoundation/CkParticles/Templates/%s.%s"), InAssetName, InAssetName);
    }

    inline auto Get_SingleBurstTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_Single"));
    }

    inline auto Get_SlashTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_Slash"));
    }

    // Which template a behavior spawns through. A recreation whose source cadence differs from every existing row
    // gets its own row and is named here; the multi-particle one-shots keep the shared burst template.
    inline auto Get_BehaviorTemplateSystemObjectPath(const int32 InBehaviorId) -> FString
    {
        switch (InBehaviorId)
        {
            case 7:  return Get_SlashTemplateSystemObjectPath();       // Slash — 1.0s loop, 0.5s lifetime, 19 burst
            case 17: return Get_SingleBurstTemplateSystemObjectPath(); // LightningRange — 1.0s loop, 1.1s lifetime, 1
            default: break;
        }

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
    // A behavior drawing through ROW-DECLARED renderers stays NAME_None: those renderers each bind their own look
    // explicitly (Get_SlashRendererSpecs), because one User.SpriteMaterial cannot carry five different materials.
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
