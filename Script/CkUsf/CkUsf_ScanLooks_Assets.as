// Language=angelscript

//============================================================================
// "CHIP GRID" SCAN — the CkUsf port of the AdvancedScan post-process
//============================================================================
//
// Source: /Game/Developers/Stephen/AdvancedScan/PP_ChipGrid_Initialize, plus the
// five MF_ScanEffect_* material functions it calls. Every default and every
// parameter GROUP below is read from those .ckexport sidecars — the groups are
// the source material's own (`props.Group` on each parameter node), not names
// invented here, so the generated master's parameter panel reads the way the
// original does.
//
// Two shape changes the CkUsf contract forces, and nothing else:
//
//  1. A Vector param arrives in the .ush as a float3 (the generator swizzles
//     .rgb), so the source's "Colour And Amount" float4s are split into a
//     Vector + a Scalar. `Fresnel - Color And Amount` RGBA(0,0.5,1,20) becomes
//     FresnelTint (0,0.5,1) + FresnelAmount 20, and so on for all seven pairs.
//     Both halves carry the same _Group, so they still sit together.
//  2. Param names must be HLSL identifiers, so the source's spaced/hyphenated
//     names lose their punctuation ("Distortion - Outer Amount" ->
//     DistortionOuterAmount). Order is preserved and the validator enforces it
//     positionally against CkUsf_PP_ScanChipGrid.
//
//============================================================================

namespace CkUsf
{
    FCk_Usf_ParamDesc Usf_ScalarIn(FName InName, float InDefault, FName InGroup, int InSort = 32)
    {
        FCk_Usf_ParamDesc Param = CkUsf::Usf_Scalar(InName, InDefault);
        Param._Group = InGroup;
        Param._SortPriority = InSort;
        return Param;
    }

    FCk_Usf_ParamDesc Usf_VectorIn(FName InName, FLinearColor InDefault, FName InGroup, int InSort = 32)
    {
        FCk_Usf_ParamDesc Param = CkUsf::Usf_Vector(InName, InDefault);
        Param._Group = InGroup;
        Param._SortPriority = InSort;
        return Param;
    }

    // Usf_ParticlesTexture builds a /CkFoundation/CkParticles/Textures/... path; this one takes the
    // object path verbatim, which any look outside that folder needs.
    FCk_Usf_ParamDesc Usf_TextureIn(FName InName, FString InObjectPath, FName InGroup, int InSort = 32)
    {
        FCk_Usf_ParamDesc Param;
        Param._Name = InName;
        Param._Type = ECk_Usf_ParamType::Texture2D;
        Param._DefaultTexturePath = InObjectPath;
        Param._Group = InGroup;
        Param._SortPriority = InSort;
        return Param;
    }
}

//============================================================================

namespace CkUsf
{
    asset ScanChipGrid of UCkUsf_LookDefinition
    {
        _UshIncludePath  = "/CkUsf/Looks/ScanChipGrid.ush";
        _UshFunctionName = n"CkUsf_PP_ScanChipGrid";
        _Domain          = ECk_Usf_Domain::PostProcess;
        _LookName        = n"ScanChipGrid";

        // Pre-TSR/TAA, for the same reason SolidOutline is: the Outline layer thresholds the world-NORMAL
        // buffer, which is rendered with the TAA-jittered projection every frame — threshold that after
        // the resolve and the outline shimmers on a stationary camera. Pre-TAA is also pre-tonemap, which
        // is the colour space the boost amounts of 20 were authored against (they read as a bright rim
        // there, and as clipped white on a tonemapped 0..1 image), and it lets bloom see the wavefront.
        _BlendableLocation = ECk_Usf_BlendableLocation::SceneColorAfterDOF;

        // _SceneTextures left empty = the default trio (SceneColor / SceneDepth / SceneNormal), which is
        // exactly what this look taps. Declaring them is what legalizes its raw SceneTextureLookup()s.

        // The source's seven StaticSwitchParameters become defines — a static switch IS a permutation,
        // and CkUsf spells permutations as _Defines. Flip one off and regenerate to drop that layer.
        _Defines.Add("CKUSF_SCAN_USE_VOXELIZE=1");
        _Defines.Add("CKUSF_SCAN_USE_OUTLINE=1");
        _Defines.Add("CKUSF_SCAN_USE_FRESNEL=1");
        _Defines.Add("CKUSF_SCAN_USE_OVERLAY=1");
        _Defines.Add("CKUSF_SCAN_USE_GRID=1");
        _Defines.Add("CKUSF_SCAN_USE_DOTS=1");
        _Defines.Add("CKUSF_SCAN_USE_FLASH=1");
        // MF_ScanEffect_NormalFresnel's "Facets - Use" static bool, which the master leaves at its
        // default (off). Set to 1 to quantize the scene normal before the Fresnel dot.
        _Defines.Add("CKUSF_SCAN_FRESNEL_FACETS=0");

        // ---- Scan: what the source reads off MPC_ChipGrid_Initialize, plus the sphere that lives
        //      inside MF_ScanEffect_ExpansionRing (the master wires only its Position input) ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"NormalizedTime", 0.0,    n"Scan", 0));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"GridCellSize",   64.0,   n"Scan", 1));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"ScreenCellSize", 64.0,   n"Scan", 2));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"FadeIn",         0.0,    n"Scan", 3));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"ScanOrigin", FLinearColor(0.0, 0.0, 0.0, 1.0), n"Scan", 4));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"ScanRange",      3000.0, n"Scan", 5));

        // ---- Curves: CA_ScanEffect_PP, row C_ScanEffect_PP_Speed, sampled at NormalizedTime.
        //      .r drives the distortion time, .g the expansion alpha, .b the flash ----
        _Parameters.Add(CkUsf::Usf_TextureIn(n"CurveAtlas",
            "/Game/Developers/Stephen/AdvancedScan/Curves/CA_ScanEffect_PP.CA_ScanEffect_PP", n"Curves", 0));

        _Parameters.Add(CkUsf::Usf_ScalarIn(n"CurveAtlasRow", 0.0, n"Curves", 1));

        // ---- Distortion ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"DistortionThickness",   0.4,  n"Distortion", 0));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"DistortionFalloff",     2.0,  n"Distortion", 1));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"DistortionOuterAmount", 0.25, n"Distortion", 2));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"DistortionInnerAmount", 0.5,  n"Distortion", 3));

        // ---- Boost ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"BoostFade",                0.975, n"Boost", 0));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"BoostRandomize",           0.0,   n"Boost", 1));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"BoostThicknessMultiplier", 4.0,   n"Boost", 2));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"BoostFadeFalloff",         4.0,   n"Boost", 3));

        // ---- Voxelize ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"VoxelizeSubdivisions", 6.0, n"Voxelize", 0));

        // ---- Grid ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"GridSubdivisions", 2.0, n"Grid", 0));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"GridThinness",     0.5, n"Grid", 1));

        // ---- Dots ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"DotsThickness",      0.0175, n"Dots", 0));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"DotsBoostThickness", 0.175,  n"Dots", 1));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"DotsPulse",          8.0,    n"Dots", 2));

        // ---- Base ("Base - Color & Value" RGBA(0.8, 0.9, 1, 0.9)) ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"BaseDesaturation", 0.1, n"Base", 1));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"BaseTint",  FLinearColor(0.8, 0.9, 1.0, 1.0), n"Base", 0));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"BaseValue", 0.9, n"Base", 2));

        // ---- Fresnel ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"FresnelFalloff", 2.0,  n"Fresnel", 2));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"FresnelPulse",   0.05, n"Fresnel", 3));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"FresnelTint",        FLinearColor(0.0, 0.5, 1.0, 1.0), n"Fresnel", 0));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"FresnelAmount",      0.1,  n"Fresnel", 1));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"FresnelBoostTint",   FLinearColor(0.0, 0.5, 1.0, 1.0), n"Fresnel", 4));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"FresnelBoostAmount", 20.0, n"Fresnel", 5));

        // ---- Outline ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"OutlinePulse", 1.0, n"Outline", 2));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"OutlineTint",        FLinearColor(0.056424, 0.111545, 0.166667, 1.0), n"Outline", 0));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"OutlineAmount",      0.2,  n"Outline", 1));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"OutlineBoostTint",   FLinearColor(0.0, 0.5, 1.0, 1.0), n"Outline", 3));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"OutlineBoostAmount", 20.0, n"Outline", 4));

        // ---- Grid / Dots colour ("Grid" RGBA(0.203966, 0.370213, 0.536458, 0.25)) ----
        _Parameters.Add(CkUsf::Usf_VectorIn(n"GridTint",        FLinearColor(0.203966, 0.370213, 0.536458, 1.0), n"Grid", 2));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"GridAmount",      0.25, n"Grid", 3));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"DotsTint",        FLinearColor(0.0, 0.5, 1.0, 1.0), n"Dots", 3));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"DotsAmount",      0.25, n"Dots", 4));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"DotsBoostTint",   FLinearColor(0.0, 0.5, 1.0, 1.0), n"Dots", 5));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"DotsBoostAmount", 20.0, n"Dots", 6));

        // ---- Background / Overlay / Flash ----
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"BackgroundRandomize",  1.0,  n"Background", 0));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"BackgroundTint",       FLinearColor(0.0, 0.5, 1.0, 1.0), n"Background", 1));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"BackgroundBrightness", 0.05, n"Background", 2));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"OverlayTint",          FLinearColor(0.04, 0.038333, 0.038333, 1.0), n"Overlay", 0));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"OverlayOpacity",       0.25, n"Overlay", 1));
        _Parameters.Add(CkUsf::Usf_VectorIn(n"FlashTint",            FLinearColor(0.0, 0.5, 1.0, 1.0), n"Flash", 0));
        _Parameters.Add(CkUsf::Usf_ScalarIn(n"FlashBrightness",      0.1,  n"Flash", 1));
    }
}
