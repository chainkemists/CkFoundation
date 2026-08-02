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
    inline constexpr int32 NumBehaviors = 30;                    // ids [0 .. NumBehaviors-1]
    inline constexpr int32 LastBehaviorId = NumBehaviors - 1;

    // --------------------------------------------------------------------------------------------------------------
    // Row-level renderer overrides.
    //
    // The shared renderer set (VisTags 0..SharedRendererVisTag_Max) is behavior-agnostic on purpose: every
    // template carries it, so nothing effect-specific may be added to it. A recreation whose source draws through
    // renderers the shared set cannot express declares its OWN renderers on its cadence row instead, and the
    // builder emits them for that row only — no other template gains a renderer it never asked for.
    //
    // Four kinds cover what a recreation actually needs beyond the shared set: a mesh renderer carrying ONE named
    // generated mesh drawn with ONE named CkUsf look, and the three sprite quads Niagara distinguishes by its
    // alignment/facing pair. Every kind binds the look master EXPLICITLY (not through User.SpriteMaterial),
    // because a row that declares several renderers needs a different material on each and one user parameter
    // cannot carry them.
    // ----------------------------------------------------------------------------------------------------------
    enum class ECk_ParticlesRenderer_Kind : uint8
    {
        Mesh,                  // MeshName -> SM_CkParticles_<MeshName>; Particles.Scale + MeshOrientation apply
        CameraFacingSprite,    // billboarded at the camera; Particles.SpriteRotation spins it in screen space
        VelocityAlignedSprite, // stretch is Particles.SpriteSize.y along motion
        CustomFacingSprite,    // quad fixed in SIM space: SpriteAlignment is its up axis, SpriteFacing its normal
    };

    struct FCk_ParticlesRendererSpec
    {
        ECk_ParticlesRenderer_Kind Kind;
        int32        VisTag;
        const TCHAR* MeshName; // Mesh kind only
        const TCHAR* LookName; // generated CkUsf master, via Get_GeneratedLookMasterObjectPath

        // Flipbook grid the bound look's textures are laid out on. (0,0) leaves Niagara's own (1,1) SubUV default
        // untouched, so a row that declares no sheet never divides its quad; anything else makes the renderer read
        // Particles.SubImageIndex over an X*Y sheet whose valid frames are 0 .. (X*Y - 1).
        FIntPoint    SubImageSize = FIntPoint(0, 0);
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

    // Vefects NS_Gunshot_Projectile + NS_Arrow_Projectile: two streaked sprite looks shared by both recreations.
    // Only the DissolveAdd instances differ per layer, so one pair of renderers serves the whole cadence row and
    // each behavior tags only the ones it draws (recipes Cookbook/NS_Gunshot_Projectile.md §8,
    // Cookbook/NS_Arrow_Projectile.md §8).
    inline auto Get_ProjectileTrioRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 10, nullptr, TEXT("PartDisAdd01") },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 11, nullptr, TEXT("PartDisAdd04") },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_Fire: two round core glows on one look, three sub-UV flame puffs on a 2x2 sheet, and the
    // velocity-aligned sparkles on the Part04 look the Slash row already established
    // (recipe Cookbook/NS_Fire.md §6.2).
    inline auto Get_FireBurstRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    12, nullptr, TEXT("PartDisAdd01")   },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    13, nullptr, TEXT("FlamesDisAdd01"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 14, nullptr, TEXT("PartDisAdd04")   },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_FireBall_Hit: thirteen renderers for seventeen enabled emitters — ten camera-facing looks (three
    // emitters share Part01 and two share Part03_Bright), one velocity-aligned sprite, and two mesh carriers whose
    // source renderers face VELOCITY, which the behavior reproduces by writing an orientation from its own
    // velocity (recipe Cookbook/NS_FireBall_Hit.md §6.2).
    inline auto Get_FireBallHitRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    15, nullptr,         TEXT("PartDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    16, nullptr,         TEXT("PartDisAdd01Bright")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    17, nullptr,         TEXT("PartDisAdd03Bright")},
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 18, nullptr,         TEXT("PartDisAdd04")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    19, nullptr,         TEXT("RingDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    20, nullptr,         TEXT("RainbowDisAdd")     },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    21, nullptr,         TEXT("StarDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    22, nullptr,         TEXT("FlareDisAdd01")     },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    23, nullptr,         TEXT("ImpactDisAdd01")    },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    24, nullptr,         TEXT("FlamesDisAdd01"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    25, nullptr,         TEXT("SmokeDisAdd01")     },
            { ECk_ParticlesRenderer_Kind::Mesh,                  26, TEXT("Spike"),   TEXT("FlatAdd02")         },
            { ECk_ParticlesRenderer_Kind::Mesh,                  27, TEXT("Card"),    TEXT("LightStripDisAdd")  },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_Gunshot_Hit: nine renderers for eleven emitters (three glows share Part01), one of them a
    // velocity-aligned sub-UV sheet — the effect's dominant element — and one mesh carrier
    // (recipe Cookbook/NS_Gunshot_Hit.md §6.2).
    inline auto Get_GunshotHitRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    28, nullptr,       TEXT("PartDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    29, nullptr,       TEXT("PartDisAdd02")      },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 30, nullptr,       TEXT("PartDisAdd04")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    31, nullptr,       TEXT("PartDisAdd03Bright")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    32, nullptr,       TEXT("StarDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 33, nullptr,       TEXT("ImpactDisAdd02"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    34, nullptr,       TEXT("ImpactDisAdd01")    },
            { ECk_ParticlesRenderer_Kind::Mesh,                  35, TEXT("Spike"), TEXT("FlatAdd02")         },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    36, nullptr,       TEXT("PartDisAdd01Bright")},
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_Arrow_Cast: thirteen renderers for fifteen emitters (Part01 serves three of them) — ten
    // camera-facing looks, one velocity-aligned sprite, two mesh carriers whose source renderers face VELOCITY,
    // and the batch's only tube. The last row is a 2x2 sub-UV sheet (recipe Cookbook/NS_Arrow_Cast.md §6.2).
    inline auto Get_ArrowCastRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    37, nullptr,          TEXT("PartDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    38, nullptr,          TEXT("PartDisAdd02")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    39, nullptr,          TEXT("RainbowDisAdd")     },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 40, nullptr,          TEXT("PartDisAdd04")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    41, nullptr,          TEXT("RingDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    42, nullptr,          TEXT("PartDisAdd01Bright")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    43, nullptr,          TEXT("ImpactDisAdd01")    },
            { ECk_ParticlesRenderer_Kind::Mesh,                  44, TEXT("Spike"),    TEXT("FlatAdd02")         },
            { ECk_ParticlesRenderer_Kind::Mesh,                  45, TEXT("Card"),     TEXT("LightStripDisAdd")  },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    46, nullptr,          TEXT("StarDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    47, nullptr,          TEXT("StarDisAdd02")      },
            { ECk_ParticlesRenderer_Kind::Mesh,                  48, TEXT("Cylinder"), TEXT("WindDisAdd02Mesh")  },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    49, nullptr,          TEXT("WindDisAdd01"), FIntPoint(2, 2) },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_Arrow_Hit: the Cast set minus the two Wind rows, plus the CUSTOM-FACING ring — the second half
    // of the source's ring pair, drawn flat in sim space where its twin billboards. Every look here is already
    // carried by another row (recipe Cookbook/NS_Arrow_Hit.md §6.2).
    inline auto Get_ArrowHitRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    50, nullptr,       TEXT("PartDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    51, nullptr,       TEXT("PartDisAdd02")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    52, nullptr,       TEXT("RainbowDisAdd")     },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 53, nullptr,       TEXT("PartDisAdd04")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    54, nullptr,       TEXT("RingDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    55, nullptr,       TEXT("PartDisAdd01Bright")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    56, nullptr,       TEXT("ImpactDisAdd01")    },
            { ECk_ParticlesRenderer_Kind::Mesh,                  57, TEXT("Spike"), TEXT("FlatAdd02")         },
            { ECk_ParticlesRenderer_Kind::Mesh,                  58, TEXT("Card"),  TEXT("LightStripDisAdd")  },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    59, nullptr,       TEXT("StarDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    60, nullptr,       TEXT("StarDisAdd02")      },
            { ECk_ParticlesRenderer_Kind::CustomFacingSprite,    61, nullptr,       TEXT("RingDisAdd01")      },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_Bomb_Spawn: nine renderers for fourteen emitters — Part01 alone serves five of them. The mesh
    // row is the cookbook's only PROP: an opaque toon-banded stand-in rather than a particle carrier
    // (recipe Cookbook/NS_Bomb_Spawn.md §6.2).
    inline auto Get_BombSpawnRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 62, nullptr,       TEXT("PartDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 63, nullptr,       TEXT("RainbowDisAdd")     },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 64, nullptr,       TEXT("PartDisAdd01Bright")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 65, nullptr,       TEXT("RingDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 66, nullptr,       TEXT("PartDisAdd02")      },
            { ECk_ParticlesRenderer_Kind::Mesh,               67, TEXT("Bomb"),  TEXT("BombToon")          },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 68, nullptr,       TEXT("StarDisAdd03")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 69, nullptr,       TEXT("PartDisAdd03")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 70, nullptr,       TEXT("PartDisAdd03Bright")},
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_PickupLoop: six renderers for nine emitters — four bomb glows share two paints, and the ring is
    // the only instance in the system with a LIVE distortion branch (recipe Cookbook/NS_PickupLoop.md §6.2).
    inline auto Get_PickupLoopRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 71, nullptr, TEXT("PartDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 72, nullptr, TEXT("PartDisAdd01Bright")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 73, nullptr, TEXT("PartDisAdd02")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 74, nullptr, TEXT("RingDisAdd03")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 75, nullptr, TEXT("StarDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 76, nullptr, TEXT("StarDisAdd02")      },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_HealLoop: seven renderers for nine emitters, and the cookbook's only pair whose emitter names and
    // materials are SWAPPED — emitter Star01 draws with the Star02 paint and vice versa. Every look is already
    // carried by another row (recipe Cookbook/NS_HealLoop.md §6.2).
    inline auto Get_HealLoopRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    77, nullptr, TEXT("RainbowDisAdd")     },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    78, nullptr, TEXT("PartDisAdd01Bright")},
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 79, nullptr, TEXT("PartDisAdd04")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    80, nullptr, TEXT("StarDisAdd02")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    81, nullptr, TEXT("StarDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    82, nullptr, TEXT("PartDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    83, nullptr, TEXT("PartDisAdd02")      },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_BuffLoop: seven renderers for nine emitters — two of them velocity-aligned, and the arrow paint is
    // the batch's only new sprite look (recipe Cookbook/NS_BuffLoop.md §6.2).
    inline auto Get_BuffLoopRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    84, nullptr, TEXT("PartDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    85, nullptr, TEXT("RainbowDisAdd")     },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 86, nullptr, TEXT("ArrowsDisAdd")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    87, nullptr, TEXT("StarDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 88, nullptr, TEXT("PartDisAdd04")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    89, nullptr, TEXT("PartDisAdd01Bright")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    90, nullptr, TEXT("PartDisAdd02")      },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_DebuffLoop: six renderers for nine emitters — three glows share one paint and the two arrow
    // streams share one look, differing only in their colour curve. One row is a 2x2 sub-UV sheet
    // (recipe Cookbook/NS_DebuffLoop.md §6.2).
    inline auto Get_DebuffLoopRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    91, nullptr, TEXT("PartDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    92, nullptr, TEXT("PartDisAdd01Bright")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    93, nullptr, TEXT("RingDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    94, nullptr, TEXT("FlamesDisAdd01"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    95, nullptr, TEXT("PartDisAdd02")      },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 96, nullptr, TEXT("ArrowsDisAdd")      },
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
    // A row declares its spawn cadence as a burst, a continuous rate, or BOTH — source emitters do all three, and
    // the two stacks compose on one emitter. A row that declares NEITHER (BurstCount 0, SpawnRate 0) is the legacy
    // seed template: its loop, lifetime and rate all come from the emitter factory defaults, so its cadence fields
    // are unused.
    // --------------------------------------------------------------------------------------------------------------
    struct FCk_ParticlesTemplateSpec
    {
        const TCHAR* AssetName;
        float        LoopDuration;
        float        ParticleLifetime;
        int32        BurstCount;

        // Empty on every row that is satisfied by the shared renderer set.
        TArrayView<const FCk_ParticlesRendererSpec> RendererOverrides;

        // Particles per second, spawned continuously across the loop. Fractional values are fine — Niagara carries
        // the sub-particle remainder across frames. Trails the renderer list so adding it left every existing row's
        // aggregate initializer untouched.
        float        SpawnRate = 0.0f;
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
            // Vefects NS_Gunshot_Projectile and NS_Arrow_Projectile: three emitters burst one particle each at
            // t=0 of a Loop-Once 10 s SYSTEM, every particle living the full 10 s. Named for the cadence, not for
            // either effect — both recreations route here and differ only in per-layer constants.
            { TEXT("PS_CkParticles_Template_ProjectileTrio"), 10.0f, 10.0f, 3, Get_ProjectileTrioRendererSpecs() },
            // The three Vefects hit/burst ports. All three sources are Loop-Once 2.0 s SYSTEMS, so they share a
            // loop duration and differ only in how long their longest layer lives and how many particles a firing
            // carries — which is exactly what a cadence row is, so each needs its own.
            // Lifetime is max over layers of (spawn delay + resolved lifetime).
            { TEXT("PS_CkParticles_Template_FireBurst"),   2.0f, 1.0f,  10, Get_FireBurstRendererSpecs()   },
            { TEXT("PS_CkParticles_Template_FireBallHit"), 2.0f, 1.34f, 47, Get_FireBallHitRendererSpecs() },
            { TEXT("PS_CkParticles_Template_GunshotHit"),  2.0f, 0.65f, 40, Get_GunshotHitRendererSpecs()  },
            // The Arrow cast/hit pair and the bomb spawn. Same Loop-Once 2.0 s system as the three above; the
            // Cast row is the longest-lived in the cookbook because its two Wind layers run 1.5 s off a 0.05 s
            // beat, where every other layer in all three is gone inside 0.55 s.
            { TEXT("PS_CkParticles_Template_ArrowCast"),   2.0f, 1.55f, 42, Get_ArrowCastRendererSpecs()   },
            { TEXT("PS_CkParticles_Template_ArrowHit"),    2.0f, 0.55f, 34, Get_ArrowHitRendererSpecs()    },
            { TEXT("PS_CkParticles_Template_BombSpawn"),   2.0f, 1.05f, 28, Get_BombSpawnRendererSpecs()   },
            // The four Vefects "Loop" ports. Their sources are INFINITE systems that spawn only through Spawn Rate
            // — no burst module anywhere — so each row states a rate and leaves BurstCount at zero. Lifetime is the
            // longest layer's resolved life, and the loop is the SYSTEM's own (1.0 s on HealLoop, 2.0 s on the
            // other three); on a rate-only source the loop wraps Emitter.Age rather than gating a spawn.
            { TEXT("PS_CkParticles_Template_PickupLoop"),  2.0f, 4.0f, 0, Get_PickupLoopRendererSpecs(), 27.5f },
            { TEXT("PS_CkParticles_Template_HealLoop"),    1.0f, 2.0f, 0, Get_HealLoopRendererSpecs(),   34.5f },
            { TEXT("PS_CkParticles_Template_BuffLoop"),    2.0f, 2.0f, 0, Get_BuffLoopRendererSpecs(),   48.0f },
            { TEXT("PS_CkParticles_Template_DebuffLoop"),  2.0f, 2.0f, 0, Get_DebuffLoopRendererSpecs(), 36.0f },
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

    inline auto Get_ProjectileTrioTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_ProjectileTrio"));
    }

    inline auto Get_FireBurstTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_FireBurst"));
    }

    inline auto Get_FireBallHitTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_FireBallHit"));
    }

    inline auto Get_GunshotHitTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_GunshotHit"));
    }

    inline auto Get_ArrowCastTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_ArrowCast"));
    }

    inline auto Get_ArrowHitTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_ArrowHit"));
    }

    inline auto Get_BombSpawnTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_BombSpawn"));
    }

    inline auto Get_PickupLoopTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_PickupLoop"));
    }

    inline auto Get_HealLoopTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_HealLoop"));
    }

    inline auto Get_BuffLoopTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_BuffLoop"));
    }

    inline auto Get_DebuffLoopTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_DebuffLoop"));
    }

    // Which template a behavior spawns through. A recreation whose source cadence differs from every existing row
    // gets its own row and is named here; the multi-particle one-shots keep the shared burst template.
    inline auto Get_BehaviorTemplateSystemObjectPath(const int32 InBehaviorId) -> FString
    {
        switch (InBehaviorId)
        {
            case 7:  return Get_SlashTemplateSystemObjectPath();       // Slash — 1.0s loop, 0.5s lifetime, 19 burst
            case 17: return Get_SingleBurstTemplateSystemObjectPath(); // LightningRange — 1.0s loop, 1.1s lifetime, 1
            // GunshotProjectile + ArrowProjectile share one row — 10s loop, 10s lifetime, burst 3.
            case 18:
            case 19: return Get_ProjectileTrioTemplateSystemObjectPath();
            case 20: return Get_FireBurstTemplateSystemObjectPath();   // FireBurst    — 2.0s loop, 1.0s,  burst 10
            case 21: return Get_FireBallHitTemplateSystemObjectPath(); // FireBallHit  — 2.0s loop, 1.34s, burst 47
            case 22: return Get_GunshotHitTemplateSystemObjectPath();  // GunshotHit   — 2.0s loop, 0.65s, burst 40
            case 23: return Get_ArrowCastTemplateSystemObjectPath();   // ArrowCast    — 2.0s loop, 1.55s, burst 42
            case 24: return Get_ArrowHitTemplateSystemObjectPath();    // ArrowHit     — 2.0s loop, 0.55s, burst 34
            case 25: return Get_BombSpawnTemplateSystemObjectPath();   // BombSpawn    — 2.0s loop, 1.05s, burst 28
            // The rate-only Loop sources: a continuous stream, no burst.
            case 26: return Get_PickupLoopTemplateSystemObjectPath();  // PickupLoop   — 2.0s loop, 4.0s, rate 27.5/s
            case 27: return Get_HealLoopTemplateSystemObjectPath();    // HealLoop     — 1.0s loop, 2.0s, rate 34.5/s
            case 28: return Get_BuffLoopTemplateSystemObjectPath();    // BuffLoop     — 2.0s loop, 2.0s, rate 48/s
            case 29: return Get_DebuffLoopTemplateSystemObjectPath();  // DebuffLoop   — 2.0s loop, 2.0s, rate 36/s
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
            // ArrowProjectile's Glow_01 is the ONLY layer in the projectile pair that is camera-facing rather
            // than velocity-aligned, so it draws on the shared VisTag 0 sprite — whose material is exactly this
            // User.SpriteMaterial binding. Its other two layers ride the row's own Part04 renderer, and the
            // Gunshot recreation (18) keeps NAME_None because every one of its layers is velocity-aligned.
            case 19: return FName(TEXT("PartDisAdd01")); // Vefects M_VFX_DisAdd_Part01 recreation
            default: return NAME_None;
        }
    }

    inline auto Get_GeneratedLookMasterObjectPath(const FName InLookName) -> FString
    {
        return FString::Printf(TEXT("/CkFoundation/CkUsf/GeneratedLooks/M_CkUsf_Look_%s.M_CkUsf_Look_%s"),
            *InLookName.ToString(), *InLookName.ToString());
    }
}
