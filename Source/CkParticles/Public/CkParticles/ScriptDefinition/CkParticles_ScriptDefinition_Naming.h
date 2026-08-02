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
    inline constexpr int32 NumBehaviors = 47;                    // ids [0 .. NumBehaviors-1]
    inline constexpr int32 LastBehaviorId = NumBehaviors - 1;

    // --------------------------------------------------------------------------------------------------------------
    // Row-level renderer overrides.
    //
    // The shared renderer set (VisTags 0..SharedRendererVisTag_Max) is behavior-agnostic on purpose: every
    // template carries it, so nothing effect-specific may be added to it. A recreation whose source draws through
    // renderers the shared set cannot express declares its OWN renderers on its cadence row instead, and the
    // builder emits them for that row only — no other template gains a renderer it never asked for.
    //
    // Five kinds cover what a recreation actually needs beyond the shared set: a mesh renderer carrying ONE named
    // generated mesh drawn with ONE named CkUsf look, the three sprite quads Niagara distinguishes by its
    // alignment/facing pair, and the ribbon (which a row declares on its SECOND emitter — see the ribbon-emitter
    // spec below, never in this list). Every kind binds the look master EXPLICITLY (not through
    // User.SpriteMaterial), because a row that declares several renderers needs a different material on each and
    // one user parameter cannot carry them.
    // ----------------------------------------------------------------------------------------------------------
    enum class ECk_ParticlesRenderer_Kind : uint8
    {
        Mesh,                  // MeshName -> SM_CkParticles_<MeshName>; Particles.Scale + MeshOrientation apply
        CameraFacingSprite,    // billboarded at the camera; Particles.SpriteRotation spins it in screen space
        VelocityAlignedSprite, // stretch is Particles.SpriteSize.y along motion
        CustomFacingSprite,    // quad fixed in SIM space: SpriteAlignment is its up axis, SpriteFacing its normal
        Ribbon,                // trail linking the emitter's particles in spawn order; ribbon-emitter specs ONLY
    };

    // How a Mesh-kind renderer orients its carrier, mirroring UNiagaraMeshFacingMode's first three members
    // (NiagaraMeshRendererProperties.h). Default ignores the camera and uses the particle's own orientation;
    // Velocity aligns the mesh's local +X with Particles.Velocity; CameraPosition points it at the camera —
    // the one mode a behavior cannot fake, because the stage has no camera.
    enum class ECk_ParticlesRenderer_MeshFacing : uint8
    {
        Default,
        Velocity,
        CameraPosition,
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

        // Mesh kind only, both of them, and both defaulted to what Niagara itself defaults to — so every row
        // authored before they existed emits a byte-identical renderer. A sprite or ribbon row that sets either
        // is declaring something nothing reads, which RosterSanity rejects rather than letting it read as intent.
        ECk_ParticlesRenderer_MeshFacing MeshFacingMode = ECk_ParticlesRenderer_MeshFacing::Default;

        // Constant scale applied to the carrier on top of Particles.Scale — the source's per-emitter
        // "Mesh Uniform Scale", which is a RENDERER property there and must not be folded into the behavior's
        // per-particle scale curve.
        FVector      MeshScale = FVector::OneVector;
    };

    // ----------------------------------------------------------------------------------------------------------
    // Ribbon emitter.
    //
    // A ribbon renderer has NO RendererVisibility and no RendererVisibilityTagBinding (verified against
    // NiagaraRibbonRendererProperties.h in the 5.7 fork — the whole class carries neither), so it cannot be
    // VisTag-gated the way every other kind is: one added to the shared emitter would link EVERY particle on that
    // template into ribbons. A ribbon-bearing row therefore declares a SECOND emitter that carries only the ribbon
    // spawn stack and the ribbon renderer(s) — separate emitter, separate particle population, which is Niagara's
    // own answer to the same question.
    //
    // The two populations run the SAME behavior id off the same User parameters; they are told apart by a SEED
    // BANK. The ribbon emitter's graph adds RibbonSeedBase to the UniqueID -> Seed wire, so a behavior detects the
    // bank from the Seed it already receives and no DI signature changes; its per-particle math then runs on
    // LocalSeed = Seed - RibbonSeedBase, which is the ribbon particle's own spawn index.
    //
    // Cadence: the loop duration and particle lifetime are the ROW's (both emitters run the same template clock);
    // only the spawn stack differs, so a ribbon spec states its own burst and/or rate and nothing else.
    //
    // Renderer bindings the builder makes, both onto attributes the DI ALREADY writes:
    //   - width  <- Particles.SpriteSize. RibbonWidthBinding reads ONE float at the bound attribute's offset, so
    //     binding the Vec2 gives the ribbon Size.x and a behavior sizes its trail the way it sizes a sprite.
    //   - ribbon id <- Particles.MeshIndex. A ribbon renderer separates concurrent trails by RibbonIdBinding, and
    //     MeshIndex is the only per-particle int the stage already outputs that no ribbon renderer otherwise reads
    //     (it selects a carrier mesh, and ribbons have none). A ribbon behavior writes its trail index there.
    // Link order stays Niagara's default bLinkOrderUseUniqueID, so link order IS spawn order IS path order.
    // A ribbon renderer has no SubImageSize at all, so no ribbon row can carry a flipbook.
    // ----------------------------------------------------------------------------------------------------------
    struct FCk_ParticlesRibbonEmitterSpec
    {
        float SpawnRate  = 0.0f;
        int32 BurstCount = 0;

        // Ribbon-kind renderers only. Empty means the row declares no ribbon emitter at all.
        TArrayView<const FCk_ParticlesRendererSpec> Renderers;

        auto Get_IsDeclared() const -> bool { return Renderers.Num() > 0; }
    };

    // Bit 30: high enough that no plausible Particles.UniqueID reaches it, low enough that base + id stays a
    // POSITIVE int32 (the Seed wire is an int pin, and a negative seed would hash into the main emitter's bank).
    // Mirrored as CKPARTICLES_RIBBON_SEED_BASE in Shaders/CkParticles/Common.ush and in the CPU mirror's
    // NDICkParticlesLocal::IsRibbonSeed / LocalSeed — the three must move together.
    inline constexpr int32 RibbonSeedBase = 0x40000000;

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

    // Vefects NS_PickupCast: eight renderers for eleven emitters — Part01 alone serves three of them and the two
    // concentric shockwave rings share a ninth. Every layer is camera-facing; the source has no mesh, no ribbon
    // and no sub-UV anywhere (recipe Cookbook/NS_PickupCast.md §6.2).
    inline auto Get_PickupCastRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,  97, nullptr, TEXT("PartDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,  98, nullptr, TEXT("PartDisAdd02")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,  99, nullptr, TEXT("RainbowDisAdd")     },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 100, nullptr, TEXT("PartDisAdd01Bright")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 101, nullptr, TEXT("RingDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 102, nullptr, TEXT("PartDisAdd03Bright")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 103, nullptr, TEXT("StarDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 104, nullptr, TEXT("StarDisAdd02")      },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_HealCast: eight renderers for nine emitters — the two body glows share Part01, the emitter names
    // and their star paints are SWAPPED (emitter Star01 draws with the Star02 paint), and the lens layer is the
    // cookbook's first sub-UV sheet on a VELOCITY-ALIGNED quad rather than a camera-facing one
    // (recipe Cookbook/NS_HealCast.md §6.2).
    inline auto Get_HealCastRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    105, nullptr, TEXT("PartDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    106, nullptr, TEXT("RainbowDisAdd")     },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    107, nullptr, TEXT("RingDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    108, nullptr, TEXT("PartDisAdd01Bright")},
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 109, nullptr, TEXT("PartDisAdd04")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    110, nullptr, TEXT("StarDisAdd02")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    111, nullptr, TEXT("StarDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 112, nullptr, TEXT("PartDisAdd07"), FIntPoint(2, 2) },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_DebuffCast: six renderers for the six ENABLED emitters — the source disables its whole warm
    // glow stack. One velocity-aligned arrow, three camera-facing sprites, a 2x2 sub-UV flame sheet, and the
    // batch's only mesh: the claw carrier whose UV streak sweeps along its own arc
    // (recipe Cookbook/NS_DebuffCast.md §6.2).
    inline auto Get_DebuffCastRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 113, nullptr,            TEXT("ArrowsDisAdd")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    114, nullptr,            TEXT("PartDisAdd01Bright")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    115, nullptr,            TEXT("RingDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    116, nullptr,            TEXT("PartDisAdd03Bright")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    117, nullptr,            TEXT("FlamesDisAdd01"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::Mesh,                  118, TEXT("SlashClaw"),  TEXT("SlashDisAdd04")     },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_Gunshot_Cast: twelve renderers for the fourteen ENABLED emitters — Part01 serves three glows and
    // the wind paint is drawn twice, once billboarded and once velocity-aligned, because the source draws the same
    // material on two different quads. THREE of the twelve are 2x2 sub-UV sheets, the heaviest flipbook load in the
    // cookbook (recipe Cookbook/NS_Gunshot_Cast.md §6.2).
    inline auto Get_GunshotCastRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    119, nullptr,          TEXT("PartDisAdd01")          },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    120, nullptr,          TEXT("PartDisAdd02")          },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 121, nullptr,          TEXT("PartDisAdd04")          },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    122, nullptr,          TEXT("PartDisAdd03Bright")    },
            { ECk_ParticlesRenderer_Kind::Mesh,                  123, TEXT("Spike"),    TEXT("FlatAdd02")             },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 124, nullptr,          TEXT("LightStripDisAddSprite")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    125, nullptr,          TEXT("StarDisAdd01")          },
            { ECk_ParticlesRenderer_Kind::Mesh,                  126, TEXT("Cylinder"), TEXT("WindDisAdd02Mesh")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    127, nullptr,          TEXT("WindDisAdd01"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 128, nullptr,          TEXT("WindDisAdd01"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 129, nullptr,          TEXT("ImpactDisAdd02"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    130, nullptr,          TEXT("PartDisAdd01Bright")    },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_FireBall_Cast: sixteen renderers for twenty-six emitters — the cookbook's largest system, and the
    // one where consolidation by material matters most (Part01 alone serves seven emitters, Part03_Bright four).
    // Two mesh carriers, one velocity-aligned sprite, two 2x2 sheets, and thirteen camera-facing paints
    // (recipe Cookbook/NS_FireBall_Cast.md §6.2).
    inline auto Get_FireBallCastRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    131, nullptr,          TEXT("PartDisAdd01")          },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    132, nullptr,          TEXT("PartDisAdd01Bright")    },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    133, nullptr,          TEXT("PartDisAdd03Bright")    },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 134, nullptr,          TEXT("PartDisAdd04")          },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    135, nullptr,          TEXT("RainbowDisAdd")         },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    136, nullptr,          TEXT("RingDisAdd01")          },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    137, nullptr,          TEXT("StarDisAdd01")          },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    138, nullptr,          TEXT("StarDisAdd02")          },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    139, nullptr,          TEXT("StarDisAdd03")          },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    140, nullptr,          TEXT("WindDisAdd01"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::Mesh,                  141, TEXT("Cylinder"), TEXT("WindDisAdd02Mesh")      },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 142, nullptr,          TEXT("LightStripDisAddSprite")},
            { ECk_ParticlesRenderer_Kind::Mesh,                  143, TEXT("Spike"),    TEXT("FlatAdd02")             },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    144, nullptr,          TEXT("FlamesDisAdd01"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    145, nullptr,          TEXT("SmokeDisAdd01")         },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    146, nullptr,          TEXT("FlareDisAdd01")         },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_Lightning_Cast: ten renderers for the nineteen ENABLED emitters — no mesh anywhere, one
    // velocity-aligned sprite, and one 2x2 sheet carrying the bolt paint that is the port's only new look
    // (recipe Cookbook/NS_Lightning_Cast.md §6.2).
    inline auto Get_LightningCastRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    147, nullptr, TEXT("PartDisAdd01")       },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    148, nullptr, TEXT("PartDisAdd02")       },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    149, nullptr, TEXT("RainbowDisAdd")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    150, nullptr, TEXT("RingDisAdd01")       },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    151, nullptr, TEXT("PartDisAdd01Bright") },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 152, nullptr, TEXT("PartDisAdd04")       },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    153, nullptr, TEXT("StarDisAdd02")       },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    154, nullptr, TEXT("PartDisAdd03Bright") },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    155, nullptr, TEXT("StarDisAdd03")       },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    156, nullptr, TEXT("LightningDisAdd02"), FIntPoint(2, 2) },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_FireBall_Projectile: six renderers for the seven sprite emitters — the two identically-painted
    // core streaks share one velocity-aligned row. The seventh and eighth source emitters are the mirrored ribbon
    // pair, which lives on the ribbon spec below (recipe Cookbook/NS_FireBall_Projectile.md §6.2).
    inline auto Get_FireBallProjectileRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 157, nullptr, TEXT("PartDisAdd04")      },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 158, nullptr, TEXT("PartDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    159, nullptr, TEXT("PartDisAdd03Bright")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    160, nullptr, TEXT("PartDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    161, nullptr, TEXT("FlamesDisAdd01"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    162, nullptr, TEXT("SmokeDisAdd01")     },
        };
        return MakeArrayView(Specs);
    }

    // The fireball's trail: ONE ribbon renderer carrying BOTH source ribbons. Trail_01 and Trail_02 are the same
    // material and the same curves, mirrored only in the sign of their curl strength, so they differ by ribbon ID
    // rather than by renderer — which is exactly what RibbonIdBinding (Particles.MeshIndex) exists for.
    inline auto Get_FireBallProjectileRibbonRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::Ribbon, 163, nullptr, TEXT("TrailFlatAdd") },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_Bomb_Projectile: two renderers for the two burst emitters — the coincident glow shells and the
    // toon prop, both already carried by the NS_Bomb_Spawn row (recipe Cookbook/NS_Bomb_Projectile.md §6.2).
    inline auto Get_BombProjectileRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite, 164, nullptr,      TEXT("PartDisAdd01") },
            { ECk_ParticlesRenderer_Kind::Mesh,               165, TEXT("Bomb"), TEXT("BombToon")     },
        };
        return MakeArrayView(Specs);
    }

    // The bomb's trail: the cookbook's only DISTANCE-driven ribbon. Its points are placed along the leader's own
    // path by arc length rather than by time (CkParticles_ArcLengthTable / ArcLengthTime), so the burst count is a
    // capacity — one point per arc-length sample — and not a rate.
    inline auto Get_BombProjectileRibbonRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::Ribbon, 166, nullptr, TEXT("TrailDisAdd01") },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_BuffCast: seven renderers for the nine sprite emitters (Part01 serves both large glows and the
    // Arrows paint serves both chevrons). The tenth source emitter is the event-spawned trail, which lives on the
    // ribbon spec below (recipe Cookbook/NS_BuffCast.md §6.2).
    inline auto Get_BuffCastRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    167, nullptr, TEXT("PartDisAdd01")  },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    168, nullptr, TEXT("PartDisAdd02")  },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    169, nullptr, TEXT("RainbowDisAdd") },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 170, nullptr, TEXT("ArrowsDisAdd")  },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    171, nullptr, TEXT("StarDisAdd01")  },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 172, nullptr, TEXT("PartDisAdd04")  },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    173, nullptr, TEXT("RingDisAdd01")  },
        };
        return MakeArrayView(Specs);
    }

    // The buff's sparkle trails: ONE ribbon renderer carrying SEVEN ribbons, one behind each sparkle. The source
    // handler applies the emitting particle's Ribbon ID, so the strands are separated by RibbonIdBinding
    // (Particles.MeshIndex) rather than by seven renderers.
    inline auto Get_BuffCastRibbonRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::Ribbon, 174, nullptr, TEXT("TrailDisAdd03") },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_Lightning_Muzzle: nine renderers for the thirteen enabled sprite/mesh emitters — Part02 serves
    // three of them and Part01 two — plus the row's THREE meshes, which is the heaviest mesh load in the cookbook.
    // The two arc ribbons live on the ribbon spec below (recipe Cookbook/NS_Lightning_Muzzle.md §6.2).
    inline auto Get_LightningMuzzleRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    175, nullptr,       TEXT("PartDisAdd01")       },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    176, nullptr,       TEXT("PartDisAdd02")       },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    177, nullptr,       TEXT("PartDisAdd01Bright") },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 178, nullptr,       TEXT("PartDisAdd04")       },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    179, nullptr,       TEXT("LightningDisAdd02"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    180, nullptr,       TEXT("PartDisAdd03Bright") },
            { ECk_ParticlesRenderer_Kind::Mesh,                  181, TEXT("Spike"), TEXT("FlatAdd02")          },
            { ECk_ParticlesRenderer_Kind::Mesh,                  182, TEXT("Card"),  TEXT("LightStripDisAdd")   },
            { ECk_ParticlesRenderer_Kind::Mesh,                  183, TEXT("Card"),  TEXT("LightningDisAdd01")  },
        };
        return MakeArrayView(Specs);
    }

    // The muzzle's arc pair: ONE ribbon renderer carrying TWO ribbons. LightningArc_01 and LightningArc_02 draw the
    // same M_VFX_DisAdd_Flat02 and differ only in count, width, launch speed and curl frequency, so they are
    // separated by ribbon id exactly as the fireball's mirrored trails are.
    inline auto Get_LightningMuzzleRibbonRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::Ribbon, 184, nullptr, TEXT("FlatAdd02Ribbon") },
        };
        return MakeArrayView(Specs);
    }

    // ----------------------------------------------------------------------------------------------------------
    // The Vefects EXPLOSION family — four systems, two structural variants, two palettes.
    //
    // NS_ExplosionGround and NS_ExplosionIceGround are the same system recoloured, and so are NS_ExplosionOmni
    // and NS_ExplosionIceOmni: a full textual diff of either pair produces ZERO renderer, material, mesh,
    // spawn-shape, count or module-structure differences. A palette twin therefore declares the SAME renderer
    // set as its fire original — literally this function — and differs only in the template asset it is built
    // into, because a behavior id resolves to exactly one template path and the spawn contract is that path.
    //
    // A VisTag is compared against Particles.VisibilityTag WITHIN one emitter of one system, so two templates
    // may carry the same numbers; the twins do, which is what lets the shared behavior include name one set of
    // tag constants instead of taking them as a parameter.
    // ----------------------------------------------------------------------------------------------------------

    // Twelve renderers for the eighteen emitters of BOTH Ground variants. Part01 alone serves five of them
    // across two quad kinds — three custom-facing glows and two billboarded ones — and the two smoke emitters
    // share one paint. The mesh pair is the batch's first use of a renderer-level MeshScale: the source's
    // per-emitter Mesh Uniform Scale 0.8 is a constant on the carrier, so it does not belong in the behavior's
    // animated scale curve (recipe Cookbook/NS_ExplosionGround.md §6.2).
    inline auto Get_ExplosionGroundRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    185, nullptr,          TEXT("PartDisAdd01")   },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    186, nullptr,          TEXT("FlareDisAdd01")  },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    187, nullptr,          TEXT("StarDisAdd01")   },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    188, nullptr,          TEXT("SmokeDisAdd01")  },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    189, nullptr,          TEXT("RingDisAdd01")   },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    190, nullptr,          TEXT("RainbowDisAdd")  },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    191, nullptr,          TEXT("FlamesDisAdd01"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 192, nullptr,          TEXT("PartDisAdd04")   },
            { ECk_ParticlesRenderer_Kind::CustomFacingSprite,    193, nullptr,          TEXT("PartDisAdd01")   },
            { ECk_ParticlesRenderer_Kind::CustomFacingSprite,    194, nullptr,          TEXT("ExpGroundMarkDisAdd") },
            { ECk_ParticlesRenderer_Kind::Mesh,                  195, TEXT("UvSphere"), TEXT("FlatAdd02"), FIntPoint(0, 0),
              ECk_ParticlesRenderer_MeshFacing::Default, FVector(0.8, 0.8, 0.8) },
            { ECk_ParticlesRenderer_Kind::Mesh,                  196, TEXT("Spike"),    TEXT("FlatAdd02")      },
        };
        return MakeArrayView(Specs);
    }

    // The ground explosion's sparkle trails: ONE ribbon renderer carrying SEVEN ribbons, one behind each
    // Sparkles_02 particle. The source's handler applies the emitting particle's Ribbon ID, so the strands are
    // separated by RibbonIdBinding (Particles.MeshIndex) rather than by seven renderers — the NS_BuffCast shape.
    inline auto Get_ExplosionGroundRibbonRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::Ribbon, 197, nullptr, TEXT("TrailDisAdd03") },
        };
        return MakeArrayView(Specs);
    }

    // Eleven renderers for the fifteen emitters of BOTH Omni variants — the Ground set minus the scorch decal,
    // whose emitter does not exist here, and with only one custom-facing glow instead of three.
    inline auto Get_ExplosionOmniRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    198, nullptr,          TEXT("PartDisAdd01")   },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    199, nullptr,          TEXT("FlareDisAdd01")  },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    200, nullptr,          TEXT("StarDisAdd01")   },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    201, nullptr,          TEXT("SmokeDisAdd01")  },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    202, nullptr,          TEXT("RingDisAdd01")   },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    203, nullptr,          TEXT("RainbowDisAdd")  },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    204, nullptr,          TEXT("FlamesDisAdd01"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 205, nullptr,          TEXT("PartDisAdd04")   },
            { ECk_ParticlesRenderer_Kind::CustomFacingSprite,    206, nullptr,          TEXT("PartDisAdd01")   },
            { ECk_ParticlesRenderer_Kind::Mesh,                  207, TEXT("UvSphere"), TEXT("FlatAdd02"), FIntPoint(0, 0),
              ECk_ParticlesRenderer_MeshFacing::Default, FVector(0.8, 0.8, 0.8) },
            { ECk_ParticlesRenderer_Kind::Mesh,                  208, TEXT("Spike"),    TEXT("FlatAdd02")      },
        };
        return MakeArrayView(Specs);
    }

    inline auto Get_ExplosionOmniRibbonRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::Ribbon, 209, nullptr, TEXT("TrailDisAdd03") },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_Bomb_Explosion: fifteen renderers for twenty-three emitters, and the heaviest MESH load in the
    // cookbook — seven of the fifteen. It is also the only row that consumes every C8 facing mode: the lightning
    // card faces VELOCITY and all five bubbles face CAMERAPOSITION, which is the one orientation a behavior
    // cannot fake because the stage has no camera. The four sphere bubbles share one carrier (SM_VFX_Sphere01
    // and SM_VFX_Sphere02 measure identical) and differ only in look and in their renderer-level mesh scale
    // (recipe Cookbook/NS_Bomb_Explosion.md §6.2).
    inline auto Get_BombExplosionRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    210, nullptr,             TEXT("PartDisAdd01")    },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    211, nullptr,             TEXT("PartDisAdd03")    },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    212, nullptr,             TEXT("PartDisAdd02")    },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    213, nullptr,             TEXT("FlareDisAdd01")   },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    214, nullptr,             TEXT("RingDisAdd01")    },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    215, nullptr,             TEXT("ImpactDisAdd01")  },
            { ECk_ParticlesRenderer_Kind::CustomFacingSprite,    216, nullptr,             TEXT("PartDisAdd01")    },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 217, nullptr,             TEXT("PartDisAdd04")    },
            { ECk_ParticlesRenderer_Kind::Mesh,                  218, TEXT("Spike"),       TEXT("FlatAdd02")       },
            { ECk_ParticlesRenderer_Kind::Mesh,                  219, TEXT("Card"),        TEXT("LightStripDisAdd"), FIntPoint(0, 0),
              ECk_ParticlesRenderer_MeshFacing::Velocity },
            { ECk_ParticlesRenderer_Kind::Mesh,                  220, TEXT("UvSphere"),    TEXT("ExpBubbleNoiseDisAdd"), FIntPoint(0, 0),
              ECk_ParticlesRenderer_MeshFacing::CameraPosition, FVector(2.0, 2.0, 2.0) },
            { ECk_ParticlesRenderer_Kind::Mesh,                  221, TEXT("UvSphere"),    TEXT("ExpFresnelBomb01"), FIntPoint(0, 0),
              ECk_ParticlesRenderer_MeshFacing::CameraPosition, FVector(2.0, 2.0, 2.0) },
            { ECk_ParticlesRenderer_Kind::Mesh,                  222, TEXT("FlatAnnulus"), TEXT("ExpBubbleOutDisAdd"), FIntPoint(0, 0),
              ECk_ParticlesRenderer_MeshFacing::CameraPosition, FVector(3.0, 3.0, 3.0) },
            { ECk_ParticlesRenderer_Kind::Mesh,                  223, TEXT("UvSphere"),    TEXT("ExpFresnelBomb02"), FIntPoint(0, 0),
              ECk_ParticlesRenderer_MeshFacing::CameraPosition, FVector(2.0, 2.0, 2.0) },
            { ECk_ParticlesRenderer_Kind::Mesh,                  224, TEXT("UvSphere"),    TEXT("ExpFresnelBomb03"), FIntPoint(0, 0),
              ECk_ParticlesRenderer_MeshFacing::CameraPosition, FVector(2.0, 2.0, 2.0) },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_Lightning_Hit: sixteen renderers for the twenty sprite/mesh emitters, the widest spread in the
    // cookbook. Part01 alone serves four of them across TWO quad kinds (the two flat ground decals and the two
    // round glows), the Lightning paint serves both bolt emitters and the Flames paint both puff emitters, and
    // the pyramid carrier is declared TWICE because the source draws it once pointed along velocity and once
    // sitting still. The arc pair lives on the ribbon spec below (recipe Cookbook/NS_Lightning_Hit.md §6.2).
    inline auto Get_LightningHitRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::CustomFacingSprite,    225, nullptr,          TEXT("PartDisAdd01")       },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    226, nullptr,          TEXT("PartDisAdd01")       },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    227, nullptr,          TEXT("PartDisAdd01Bright") },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    228, nullptr,          TEXT("FlareDisAdd01")      },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 229, nullptr,          TEXT("PartDisAdd04")       },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    230, nullptr,          TEXT("LightningDisAdd02"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::Mesh,                  231, TEXT("Spike"),    TEXT("FlatAdd02")          },
            { ECk_ParticlesRenderer_Kind::Mesh,                  232, TEXT("Card"),     TEXT("LightStripDisAdd")   },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    233, nullptr,          TEXT("ImpactDisAdd01")     },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    234, nullptr,          TEXT("RainbowDisAdd")      },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    235, nullptr,          TEXT("RingDisAdd01")       },
            { ECk_ParticlesRenderer_Kind::CustomFacingSprite,    236, nullptr,          TEXT("ExpGroundMarkDisAdd")},
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    237, nullptr,          TEXT("StarDisAdd01")       },
            { ECk_ParticlesRenderer_Kind::Mesh,                  238, TEXT("UvSphere"), TEXT("FlatAdd02"), FIntPoint(0, 0),
              ECk_ParticlesRenderer_MeshFacing::Default, FVector(0.8, 0.8, 0.8) },
            { ECk_ParticlesRenderer_Kind::Mesh,                  239, TEXT("Spike"),    TEXT("FlatAdd02")          },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    240, nullptr,          TEXT("FlamesDisAdd01"), FIntPoint(2, 2) },
        };
        return MakeArrayView(Specs);
    }

    // The hit's arc pair: the SAME two emitters NS_Lightning_Muzzle carries, verified byte-identical against
    // the corpus, so they share that row's shape exactly — ONE ribbon renderer carrying two ribbons split by
    // ribbon id (which rides MeshIndex).
    inline auto Get_LightningHitRibbonRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::Ribbon, 241, nullptr, TEXT("FlatAdd02Ribbon") },
        };
        return MakeArrayView(Specs);
    }

    // Vefects NS_Dash: four renderers for four emitters, one per source material — the cookbook's only row
    // where every emitter draws its own paint. Two of them are meshes: the open cylinder NS_Arrow_Cast's wind
    // tube already carries, and the truncated cone this port adds. The sprite pair is the same 2x2 wind sheet
    // that row draws, plus the velocity-aligned speed lines (recipe Cookbook/NS_Dash.md §6.2).
    inline auto Get_DashRendererSpecs() -> TArrayView<const FCk_ParticlesRendererSpec>
    {
        static const FCk_ParticlesRendererSpec Specs[] =
        {
            { ECk_ParticlesRenderer_Kind::Mesh,                  242, TEXT("Cylinder"), TEXT("WindDisAdd02Mesh") },
            { ECk_ParticlesRenderer_Kind::CameraFacingSprite,    243, nullptr,          TEXT("WindDisAdd01"), FIntPoint(2, 2) },
            { ECk_ParticlesRenderer_Kind::Mesh,                  244, TEXT("Cone"),     TEXT("WindDisAdd03")     },
            { ECk_ParticlesRenderer_Kind::VelocityAlignedSprite, 245, nullptr,          TEXT("PartDisAdd02")     },
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

        // The row's SECOND emitter, carrying its ribbon spawn stack and ribbon renderers (see the spec above).
        // Default-empty, and trailing for the same reason SpawnRate is: a row that draws no trail states nothing.
        FCk_ParticlesRibbonEmitterSpec RibbonEmitter;
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
            // The three Vefects "Cast" ports. All three sources are Loop-Once 2.0 s SYSTEMS again, but two of
            // them mix cadences: emitters governed by the system burst once, while `Life Cycle Mode = Self`
            // emitters fire a burst AND a spawn rate over their own 0.2-0.3 s window. Those rows carry BOTH
            // stacks; the behavior splits the two populations by spawn phase and gates the streamed ones on the
            // window. PickupCast bursts only, so its rate stays zero.
            { TEXT("PS_CkParticles_Template_PickupCast"),  2.0f, 1.05f, 22, Get_PickupCastRendererSpecs()        },
            { TEXT("PS_CkParticles_Template_HealCast"),    2.0f, 1.55f, 17, Get_HealCastRendererSpecs(),   50.0f },
            { TEXT("PS_CkParticles_Template_DebuffCast"),  2.0f, 2.0f,  30, Get_DebuffCastRendererSpecs(), 65.0f },
            // The three Vefects attack Casts. Same Loop-Once 2.0 s system again. Gunshot and FireBall burst only
            // — every one of their emitters is a Spawn Burst Instantaneous, including the `Self / Once` ones,
            // and a burst-only one-shot fires exactly once per activation, which is what a template burst is.
            // Lightning is the first row whose SOURCE rate is not constant: its bolt emitter's Spawn Rate falls
            // 20 -> 0 across a 0.5 s window, so the row declares the PEAK and the behavior thins the stream.
            // FireBall's lifetime EXCEEDS its loop ([P0-D5]) — its wind layers spawn at 0.55 and live 1.5 s.
            { TEXT("PS_CkParticles_Template_GunshotCast"),   2.0f, 1.55f, 40, Get_GunshotCastRendererSpecs()          },
            { TEXT("PS_CkParticles_Template_FireBallCast"),  2.0f, 2.05f, 50, Get_FireBallCastRendererSpecs()         },
            { TEXT("PS_CkParticles_Template_LightningCast"), 2.0f, 1.55f, 30, Get_LightningCastRendererSpecs(), 40.0f },
            // The two Vefects projectile TRAILS — the cookbook's first rows to declare a ribbon emitter.
            //
            // FireBall is the heaviest row here: a 10 s Self-governed loop (every emitter runs its own 10 s
            // cycle, and the system agrees), fifteen burst particles, and 408 sprites a second on top. Its
            // ribbon emitter streams 100 points a second, 50 for each of the mirrored pair. Bomb is a Loop-Once
            // 2.5 s system — the only 2.5 in the cookbook, matching its own 2.5 s lifetimes — bursting four.
            // Its ribbon spec states a BURST because the source spawns per unit of travel, not per second: the
            // behavior places its 17 points along the leader's path by arc length.
            { TEXT("PS_CkParticles_Template_FireBallProjectile"), 10.0f, 10.0f, 15,
              Get_FireBallProjectileRendererSpecs(), 408.0f,
              { 100.0f, 0, Get_FireBallProjectileRibbonRendererSpecs() } },
            { TEXT("PS_CkParticles_Template_BombProjectile"), 2.5f, 2.5f, 4,
              Get_BombProjectileRendererSpecs(), 0.0f,
              { 0.0f, 17, Get_BombProjectileRibbonRendererSpecs() } },
            // The two Vefects EVENT-driven ribbon rows, and the pair that closes the trail phase. Both sources are
            // Loop-Once 2.0 s systems again, and on both the ribbon population is a BURST rather than a rate: a
            // trail point is a SAMPLE of a leader path at a solved time, so the count is a capacity and the times
            // come out of the behavior. BuffCast bursts 43 points for each of its seven sparkles — the source's
            // every-frame location events at the 60 Hz they are quoted against. LightningMuzzle bursts the arc
            // pair's six burst points plus the twelve each that its falling 80 -> 0 spawn rate inverts to.
            { TEXT("PS_CkParticles_Template_BuffCast"), 2.0f, 1.5f, 23,
              Get_BuffCastRendererSpecs(), 0.0f,
              { 0.0f, 301, Get_BuffCastRibbonRendererSpecs() } },
            { TEXT("PS_CkParticles_Template_LightningMuzzle"), 2.0f, 0.6f, 24,
              Get_LightningMuzzleRendererSpecs(), 0.0f,
              { 0.0f, 30, Get_LightningMuzzleRibbonRendererSpecs() } },
            // The Vefects EXPLOSION family. Four rows for four systems: a palette twin needs its own row because
            // a behavior id resolves to exactly one template path, but it declares the SAME renderers and the
            // SAME cadence as its fire original, so the two rows differ only in AssetName. All four sources are
            // Loop-Once 2.0 s systems; the Ground pair's longest layer is the 1.5 s scorch decal and the Omni
            // pair's is the 1.3 s smoke, which is the only cadence number the Ground/Omni axis moves.
            //
            // Each carries a ribbon emitter bursting 301 points — seven strands of 43 samples, the source's
            // every-frame location events at the 60 Hz they are quoted against, which is the same count and the
            // same derivation NS_BuffCast's trail uses.
            { TEXT("PS_CkParticles_Template_ExplosionGround"), 2.0f, 1.5f, 70,
              Get_ExplosionGroundRendererSpecs(), 0.0f,
              { 0.0f, 301, Get_ExplosionGroundRibbonRendererSpecs() } },
            { TEXT("PS_CkParticles_Template_ExplosionGroundIce"), 2.0f, 1.5f, 70,
              Get_ExplosionGroundRendererSpecs(), 0.0f,
              { 0.0f, 301, Get_ExplosionGroundRibbonRendererSpecs() } },
            { TEXT("PS_CkParticles_Template_ExplosionOmni"), 2.0f, 1.3f, 65,
              Get_ExplosionOmniRendererSpecs(), 0.0f,
              { 0.0f, 301, Get_ExplosionOmniRibbonRendererSpecs() } },
            { TEXT("PS_CkParticles_Template_ExplosionOmniIce"), 2.0f, 1.3f, 65,
              Get_ExplosionOmniRendererSpecs(), 0.0f,
              { 0.0f, 301, Get_ExplosionOmniRibbonRendererSpecs() } },
            // NS_Bomb_Explosion: the cookbook's largest single burst at 162, over by t = 0.5 s of a 2.0 s loop.
            // The count needs no new mechanism — Add_SpawnEmitterStack writes BurstCount as one int, and the
            // BuffCast ribbon emitter already drives that same code path at 301.
            { TEXT("PS_CkParticles_Template_BombExplosion"), 2.0f, 0.5f, 162,
              Get_BombExplosionRendererSpecs() },
            // NS_Lightning_Hit: twenty-two emitters, the widest system in the cookbook. Loop-Once 2.0 s system
            // again; the longest layer is Star02 at its resolved 1.3 s maximum. Its burst is the twenty
            // sprite/mesh emitters' own counts (81) plus the three solved release points of Lightning_02, whose
            // source emitter carries a falling Spawn Rate and NO burst module at all. The arcs' six burst
            // particles are NOT in that number — they are ribbon particles on the row's second emitter, which
            // bursts their thirty points exactly as NS_Lightning_Muzzle's does.
            { TEXT("PS_CkParticles_Template_LightningHit"), 2.0f, 1.3f, 84,
              Get_LightningHitRendererSpecs(), 0.0f,
              { 0.0f, 30, Get_LightningHitRibbonRendererSpecs() } },
            // NS_Dash: a Loop-Once 2.0 s system again, and the third row whose source carries BOTH spawn
            // stacks on one emitter. Its burst is the four emitters' own counts (1 + 4 + 1 + 13 = 19) and
            // its rate is Add_Lines' own 50/s, which the behavior gates on that emitter's 0.3 s
            // `Life Cycle Mode = Self / Once` window. Its lifetime is the wind tube's 0.05 s beat plus its
            // 1.5 s life — the same 1.55 the ArrowCast row carries for the same wind pair.
            { TEXT("PS_CkParticles_Template_Dash"), 2.0f, 1.55f, 19,
              Get_DashRendererSpecs(), 50.0f },
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

            // Niagara does not gate a ribbon renderer on the tag, but its particles still write one, so the
            // ribbon emitter's renderers allocate from the same ledger as every other row renderer.
            for (const auto& Renderer : Spec.RibbonEmitter.Renderers)
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

    inline auto Get_PickupCastTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_PickupCast"));
    }

    inline auto Get_HealCastTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_HealCast"));
    }

    inline auto Get_DebuffCastTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_DebuffCast"));
    }

    inline auto Get_GunshotCastTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_GunshotCast"));
    }

    inline auto Get_FireBallCastTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_FireBallCast"));
    }

    inline auto Get_LightningCastTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_LightningCast"));
    }

    inline auto Get_FireBallProjectileTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_FireBallProjectile"));
    }

    inline auto Get_BombProjectileTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_BombProjectile"));
    }

    inline auto Get_BuffCastTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_BuffCast"));
    }

    inline auto Get_LightningMuzzleTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_LightningMuzzle"));
    }

    inline auto Get_ExplosionGroundTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_ExplosionGround"));
    }

    inline auto Get_ExplosionGroundIceTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_ExplosionGroundIce"));
    }

    inline auto Get_ExplosionOmniTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_ExplosionOmni"));
    }

    inline auto Get_ExplosionOmniIceTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_ExplosionOmniIce"));
    }

    inline auto Get_BombExplosionTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_BombExplosion"));
    }

    inline auto Get_LightningHitTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_LightningHit"));
    }

    inline auto Get_DashTemplateSystemObjectPath() -> FString
    {
        return Get_TemplateSystemObjectPath(TEXT("PS_CkParticles_Template_Dash"));
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
            // The Cast sources: a burst, and on two of the three a spawn rate composed on top of it.
            case 30: return Get_PickupCastTemplateSystemObjectPath();  // PickupCast   — 2.0s loop, 1.05s, burst 22
            case 31: return Get_HealCastTemplateSystemObjectPath();    // HealCast     — 2.0s loop, 1.55s, 17 + 50/s
            case 32: return Get_DebuffCastTemplateSystemObjectPath();  // DebuffCast   — 2.0s loop, 2.0s,  30 + 65/s
            // The three attack Casts. Only Lightning streams.
            case 33: return Get_GunshotCastTemplateSystemObjectPath();   // GunshotCast   — 2.0s loop, 1.55s, burst 40
            case 34: return Get_FireBallCastTemplateSystemObjectPath();  // FireBallCast  — 2.0s loop, 2.05s, burst 50
            case 35: return Get_LightningCastTemplateSystemObjectPath(); // LightningCast — 2.0s loop, 1.55s, 30 + 40/s
            // The two projectile trails. Both rows carry a ribbon emitter on top of the sprite/mesh one.
            case 36: return Get_FireBallProjectileTemplateSystemObjectPath(); // 10s loop, 10s, 15 + 408/s + ribbon 100/s
            case 37: return Get_BombProjectileTemplateSystemObjectPath();     // 2.5s loop, 2.5s, 4 + ribbon burst 17
            // The two event-driven ribbon rows. Both ribbon emitters burst: their points are samples of a leader
            // path taken at times the behavior solves, not a stream.
            case 38: return Get_BuffCastTemplateSystemObjectPath();        // 2.0s loop, 1.5s, 23 + ribbon burst 301
            case 39: return Get_LightningMuzzleTemplateSystemObjectPath(); // 2.0s loop, 0.6s, 24 + ribbon burst 30
            // The explosion family. A palette twin takes its OWN template path — that path IS the spawn
            // contract — even though its row is otherwise identical to its fire original's.
            case 40: return Get_ExplosionGroundTemplateSystemObjectPath();    // 2.0s loop, 1.5s, 70 + ribbon 301
            case 41: return Get_ExplosionGroundIceTemplateSystemObjectPath(); // the Ground palette twin
            case 42: return Get_ExplosionOmniTemplateSystemObjectPath();      // 2.0s loop, 1.3s, 65 + ribbon 301
            case 43: return Get_ExplosionOmniIceTemplateSystemObjectPath();   // the Omni palette twin
            case 44: return Get_BombExplosionTemplateSystemObjectPath();      // 2.0s loop, 0.5s, burst 162
            // The everything-effect: every renderer class the pipeline has, on one row.
            case 45: return Get_LightningHitTemplateSystemObjectPath();       // 2.0s loop, 1.3s, 84 + ribbon 30
            case 46: return Get_DashTemplateSystemObjectPath();               // 2.0s loop, 1.55s, 19 + 50/s
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
