#include "CkParticlesEditor/Generator/CkParticles_MaterialGenerator.h"

#include "CkParticlesEditor_Log.h"

#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"

#include "Engine/Texture2D.h"
#include "MaterialDomain.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionDepthFade.h"
#include "Materials/MaterialExpressionDynamicParameter.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionParticleColor.h"
#include "Materials/MaterialExpressionSmoothStep.h"
#include "Materials/MaterialExpressionTextureBase.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "SceneTypes.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::particles_editor::MatGenLocal
{
    static const TCHAR* MatDir = TEXT("/CkFoundation/CkParticles/Materials");

    // Same idempotent package refresh the texture baker uses: displace any prior object, build fresh, save.
    static auto Begin_Material(const TCHAR* InName) -> UMaterial*
    {
        const FString PkgPath = FString::Printf(TEXT("%s/%s"), MatDir, InName);

        UPackage* Package = FPackageName::DoesPackageExist(PkgPath)
            ? LoadPackage(nullptr, *PkgPath, LOAD_None)
            : nullptr;
        if (Package == nullptr) { Package = CreatePackage(*PkgPath); }
        if (Package == nullptr) { return nullptr; }

        if (auto* Old = StaticFindObject(UMaterial::StaticClass(), Package, InName))
        {
            Old->ClearFlags(RF_Standalone | RF_Public);
            Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
        }

        return NewObject<UMaterial>(Package, InName, RF_Public | RF_Standalone);
    }

    static auto Finish_Material(UMaterial* InMaterial) -> bool
    {
        UMaterialEditingLibrary::LayoutMaterialExpressions(InMaterial);
        UMaterialEditingLibrary::RecompileMaterial(InMaterial);

        InMaterial->MarkPackageDirty();
        FAssetRegistryModule::AssetCreated(InMaterial);

        const auto PkgPath = InMaterial->GetOutermost()->GetName();
        const auto FileName = FPackageName::LongPackageNameToFilename(PkgPath, FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(InMaterial->GetOutermost(), InMaterial, *FileName, SaveArgs);
    }

    template <typename T_Expression>
    static auto New_Expression(UMaterial* InMaterial, int32 InX, int32 InY) -> T_Expression*
    {
        return Cast<T_Expression>(
            UMaterialEditingLibrary::CreateMaterialExpression(InMaterial, T_Expression::StaticClass(), InX, InY));
    }

    static auto New_TextureParam(
        UMaterial*   InMaterial,
        const TCHAR* InParamName,
        const FName  InVfxTextureName,
        int32        InX,
        int32        InY) -> UMaterialExpressionTextureSampleParameter2D*
    {
        auto* Sample = New_Expression<UMaterialExpressionTextureSampleParameter2D>(InMaterial, InX, InY);
        if (Sample == nullptr) { return nullptr; }

        Sample->ParameterName = InParamName;
        if (auto* Texture = LoadObject<UTexture2D>(nullptr, *ck::particles::Get_VfxTextureObjectPath(InVfxTextureName)))
        {
            Sample->Texture     = Texture;
            Sample->SamplerType = UMaterialExpressionTextureBase::GetSamplerTypeForTexture(Texture);
        }
        return Sample;
    }

    // The behavior-driven float4 (Particles.DynamicMaterialParameter). Default MUST be zero: a behavior that
    // writes nothing gets no dissolve, no pan, no boost — never an invisible or fully-eroded particle.
    static auto New_DynamicParams(UMaterial* InMaterial, int32 InX, int32 InY) -> UMaterialExpressionDynamicParameter*
    {
        auto* Dyn = New_Expression<UMaterialExpressionDynamicParameter>(InMaterial, InX, InY);
        if (Dyn == nullptr) { return nullptr; }

        Dyn->ParamNames = { TEXT("Dissolve"), TEXT("Distortion"), TEXT("PanOffset"), TEXT("Boost") };
        Dyn->DefaultValue = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

        // ConnectMaterialExpressions matches against the RAW Outputs array (MaterialEditingLibrary.cpp:98) which
        // still holds the CDO's "Param1..4" names — GetOutputs() is the engine's own sync of Outputs from
        // ParamNames (MaterialExpressions.cpp:9965). Without this, every by-name connect off this node misses:
        // inputs with const fallbacks (SmoothStep/Add) degrade silently, AppendVector hard-fails the material.
        Dyn->GetOutputs();
        return Dyn;
    }

    static auto Setup_TranslucentUnlit(UMaterial* InMaterial, bool bInTwoSided) -> void
    {
        InMaterial->MaterialDomain = MD_Surface;
        InMaterial->BlendMode      = BLEND_Translucent;
        InMaterial->SetShadingModel(MSM_Unlit);
        InMaterial->TwoSided = bInTwoSided;
        InMaterial->bUsedWithNiagaraSprites       = true;
        InMaterial->bUsedWithNiagaraMeshParticles = true;
    }

    // Dissolve = smoothstep(Threshold, Threshold + Softness, NoiseOrMask) — the marketplace erosion idiom.
    static auto Build_ErodeChain(
        UMaterial*                             InMaterial,
        UMaterialExpression*                   InThreshold,
        const TCHAR*                           InThresholdOutput,
        UMaterialExpression*                   InMask,
        const TCHAR*                           InMaskOutput,
        float                                  InSoftness) -> UMaterialExpressionSmoothStep*
    {
        auto* Softness = New_Expression<UMaterialExpressionConstant>(InMaterial, -700, 520);
        auto* MaxEdge  = New_Expression<UMaterialExpressionAdd>(InMaterial, -540, 480);
        auto* Erode    = New_Expression<UMaterialExpressionSmoothStep>(InMaterial, -380, 440);
        if (Softness == nullptr || MaxEdge == nullptr || Erode == nullptr) { return nullptr; }

        Softness->R = InSoftness;
        UMaterialEditingLibrary::ConnectMaterialExpressions(InThreshold, InThresholdOutput, MaxEdge, TEXT("A"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(Softness,    TEXT(""),          MaxEdge, TEXT("B"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(InThreshold, InThresholdOutput, Erode,   TEXT("Min"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(MaxEdge,     TEXT(""),          Erode,   TEXT("Max"));
        UMaterialEditingLibrary::ConnectMaterialExpressions(InMask,      InMaskOutput,      Erode,   TEXT("Value"));
        return Erode;
    }

    // ---- M_CkParticles_SweepErode ---------------------------------------------------------------------------------
    static auto Build_SweepErode() -> bool
    {
        auto* Mat = Begin_Material(TEXT("M_CkParticles_SweepErode"));
        if (Mat == nullptr) { return false; }
        Setup_TranslucentUnlit(Mat, /*two-sided*/ true);

        auto* Uv     = New_Expression<UMaterialExpressionTextureCoordinate>(Mat, -1500, 0);
        auto* Dyn    = New_DynamicParams(Mat, -1500, 260);
        auto* PColor = New_Expression<UMaterialExpressionParticleColor>(Mat, -1500, 560);

        // Pan the main texture along V by Dynamic.PanOffset (the sweep meshes author V along the arc/length,
        // so panning V is the "texture travels along the sweep" motion the marketplace slashes use).
        auto* PanZero = New_Expression<UMaterialExpressionConstant>(Mat, -1320, 60);
        auto* PanVec  = New_Expression<UMaterialExpressionAppendVector>(Mat, -1180, 80);
        auto* PannedUv = New_Expression<UMaterialExpressionAdd>(Mat, -1040, 20);

        // UV distortion: tileable noise auto-scrolled by a Panner, recentered, scaled by Dynamic.Distortion.
        auto* NoisePan   = New_Expression<UMaterialExpressionPanner>(Mat, -1320, -260);
        auto* NoiseTex   = New_TextureParam(Mat, TEXT("NoiseTexture"), TEXT("TileNoise"), -1180, -260);
        auto* Recenter   = New_Expression<UMaterialExpressionAdd>(Mat, -1000, -260);
        auto* HalfNeg    = New_Expression<UMaterialExpressionConstant>(Mat, -1180, -140);
        auto* WarpAmount = New_Expression<UMaterialExpressionMultiply>(Mat, -860, -240);
        auto* WarpScale  = New_Expression<UMaterialExpressionMultiply>(Mat, -720, -220);
        auto* WarpScaleK = New_Expression<UMaterialExpressionConstant>(Mat, -860, -100);
        auto* WarpVec    = New_Expression<UMaterialExpressionAppendVector>(Mat, -580, -200);
        auto* WarpedUv   = New_Expression<UMaterialExpressionAdd>(Mat, -440, -40);

        auto* MainTex     = New_TextureParam(Mat, TEXT("MainTexture"), TEXT("SweepStreak"), -280, -60);
        auto* DissolveTex = New_TextureParam(Mat, TEXT("DissolveTexture"), TEXT("TileNoise"), -700, 320);

        if (Uv == nullptr || Dyn == nullptr || PColor == nullptr || PanZero == nullptr || PanVec == nullptr ||
            PannedUv == nullptr || NoisePan == nullptr || NoiseTex == nullptr || Recenter == nullptr ||
            HalfNeg == nullptr || WarpAmount == nullptr || WarpScale == nullptr || WarpScaleK == nullptr ||
            WarpVec == nullptr || WarpedUv == nullptr || MainTex == nullptr || DissolveTex == nullptr)
        { return false; }

        PanZero->R = 0.0f;
        HalfNeg->R = -0.5f;
        WarpScaleK->R = 0.22f;
        NoisePan->SpeedX = 0.11f;
        NoisePan->SpeedY = 0.07f;

        const auto Wire = &UMaterialEditingLibrary::ConnectMaterialExpressions;
        Wire(PanZero, TEXT(""), PanVec, TEXT("A"));
        Wire(Dyn, TEXT("PanOffset"), PanVec, TEXT("B"));
        Wire(Uv, TEXT(""), PannedUv, TEXT("A"));
        Wire(PanVec, TEXT(""), PannedUv, TEXT("B"));

        Wire(Uv, TEXT(""), NoisePan, TEXT(""));
        Wire(NoisePan, TEXT(""), NoiseTex, TEXT(""));
        Wire(NoiseTex, TEXT("R"), Recenter, TEXT("A"));
        Wire(HalfNeg, TEXT(""), Recenter, TEXT("B"));
        Wire(Recenter, TEXT(""), WarpAmount, TEXT("A"));
        Wire(Dyn, TEXT("Distortion"), WarpAmount, TEXT("B"));
        Wire(WarpAmount, TEXT(""), WarpScale, TEXT("A"));
        Wire(WarpScaleK, TEXT(""), WarpScale, TEXT("B"));
        Wire(WarpScale, TEXT(""), WarpVec, TEXT("A"));
        Wire(WarpScale, TEXT(""), WarpVec, TEXT("B"));
        Wire(PannedUv, TEXT(""), WarpedUv, TEXT("A"));
        Wire(WarpVec, TEXT(""), WarpedUv, TEXT("B"));
        Wire(WarpedUv, TEXT(""), MainTex, TEXT(""));

        auto* Erode = Build_ErodeChain(Mat, Dyn, TEXT("Dissolve"), DissolveTex, TEXT("R"), 0.25f);
        if (Erode == nullptr) { return false; }

        // Emissive = MainTex.RGB x ParticleColor.RGB x (1 + Boost); Opacity = MainTex.R x Erode x Color.A, depth-faded.
        auto* One       = New_Expression<UMaterialExpressionConstant>(Mat, -280, 340);
        auto* BoostPlus = New_Expression<UMaterialExpressionAdd>(Mat, -140, 320);
        auto* TintMul   = New_Expression<UMaterialExpressionMultiply>(Mat, -100, 60);
        auto* Emissive  = New_Expression<UMaterialExpressionMultiply>(Mat, 40, 100);
        auto* OpacMulA  = New_Expression<UMaterialExpressionMultiply>(Mat, -100, 480);
        auto* OpacMulB  = New_Expression<UMaterialExpressionMultiply>(Mat, 40, 520);
        auto* Fade      = New_Expression<UMaterialExpressionDepthFade>(Mat, 180, 540);
        if (One == nullptr || BoostPlus == nullptr || TintMul == nullptr || Emissive == nullptr ||
            OpacMulA == nullptr || OpacMulB == nullptr || Fade == nullptr)
        { return false; }

        One->R = 1.0f;
        Fade->FadeDistanceDefault = 30.0f;

        Wire(One, TEXT(""), BoostPlus, TEXT("A"));
        Wire(Dyn, TEXT("Boost"), BoostPlus, TEXT("B"));
        Wire(MainTex, TEXT(""), TintMul, TEXT("A"));
        Wire(PColor, TEXT(""), TintMul, TEXT("B"));
        Wire(TintMul, TEXT(""), Emissive, TEXT("A"));
        Wire(BoostPlus, TEXT(""), Emissive, TEXT("B"));

        Wire(MainTex, TEXT("R"), OpacMulA, TEXT("A"));
        Wire(Erode, TEXT(""), OpacMulA, TEXT("B"));
        Wire(OpacMulA, TEXT(""), OpacMulB, TEXT("A"));
        Wire(PColor, TEXT("A"), OpacMulB, TEXT("B"));
        Wire(OpacMulB, TEXT(""), Fade, TEXT(""));

        UMaterialEditingLibrary::ConnectMaterialProperty(Emissive, TEXT(""), MP_EmissiveColor);
        UMaterialEditingLibrary::ConnectMaterialProperty(Fade, TEXT(""), MP_Opacity);

        return Finish_Material(Mat);
    }

    // ---- M_CkParticles_SoftSmoke ----------------------------------------------------------------------------------
    static auto Build_SoftSmoke() -> bool
    {
        auto* Mat = Begin_Material(TEXT("M_CkParticles_SoftSmoke"));
        if (Mat == nullptr) { return false; }
        Setup_TranslucentUnlit(Mat, /*two-sided*/ false);

        auto* Dyn     = New_DynamicParams(Mat, -1200, 300);
        auto* PColor  = New_Expression<UMaterialExpressionParticleColor>(Mat, -1200, 560);
        auto* MainTex = New_TextureParam(Mat, TEXT("MainTexture"), TEXT("Smoke"), -1200, 0);
        if (Dyn == nullptr || PColor == nullptr || MainTex == nullptr) { return false; }

        // Smoke bakes density into RGB and an erosion mask into A — dissolve eats along the baked mask.
        auto* Erode = Build_ErodeChain(Mat, Dyn, TEXT("Dissolve"), MainTex, TEXT("A"), 0.35f);
        if (Erode == nullptr) { return false; }

        auto* Emissive = New_Expression<UMaterialExpressionMultiply>(Mat, -200, 80);
        auto* OpacMulA = New_Expression<UMaterialExpressionMultiply>(Mat, -200, 420);
        auto* OpacMulB = New_Expression<UMaterialExpressionMultiply>(Mat, -60, 460);
        auto* Fade     = New_Expression<UMaterialExpressionDepthFade>(Mat, 80, 480);
        if (Emissive == nullptr || OpacMulA == nullptr || OpacMulB == nullptr || Fade == nullptr) { return false; }

        Fade->FadeDistanceDefault = 45.0f;

        const auto Wire = &UMaterialEditingLibrary::ConnectMaterialExpressions;
        Wire(MainTex, TEXT(""), Emissive, TEXT("A"));
        Wire(PColor, TEXT(""), Emissive, TEXT("B"));

        Wire(MainTex, TEXT("R"), OpacMulA, TEXT("A"));
        Wire(Erode, TEXT(""), OpacMulA, TEXT("B"));
        Wire(OpacMulA, TEXT(""), OpacMulB, TEXT("A"));
        Wire(PColor, TEXT("A"), OpacMulB, TEXT("B"));
        Wire(OpacMulB, TEXT(""), Fade, TEXT(""));

        UMaterialEditingLibrary::ConnectMaterialProperty(Emissive, TEXT(""), MP_EmissiveColor);
        UMaterialEditingLibrary::ConnectMaterialProperty(Fade, TEXT(""), MP_Opacity);

        return Finish_Material(Mat);
    }

    // ---- M_CkParticles_FresnelShell -------------------------------------------------------------------------------
    static auto Build_FresnelShell() -> bool
    {
        auto* Mat = Begin_Material(TEXT("M_CkParticles_FresnelShell"));
        if (Mat == nullptr) { return false; }
        Mat->MaterialDomain = MD_Surface;
        Mat->BlendMode      = BLEND_Additive;
        Mat->SetShadingModel(MSM_Unlit);
        Mat->TwoSided = true;
        Mat->bUsedWithNiagaraSprites       = true;
        Mat->bUsedWithNiagaraMeshParticles = true;

        auto* Dyn      = New_DynamicParams(Mat, -1200, 300);
        auto* PColor   = New_Expression<UMaterialExpressionParticleColor>(Mat, -1200, 560);
        auto* Fresnel  = New_Expression<UMaterialExpressionFresnel>(Mat, -1200, 0);
        auto* Uv       = New_Expression<UMaterialExpressionTextureCoordinate>(Mat, -1500, 160);
        auto* NoisePan = New_Expression<UMaterialExpressionPanner>(Mat, -1350, 160);
        auto* NoiseTex = New_TextureParam(Mat, TEXT("NoiseTexture"), TEXT("TileNoise"), -1200, 160);
        if (Dyn == nullptr || PColor == nullptr || Fresnel == nullptr || Uv == nullptr ||
            NoisePan == nullptr || NoiseTex == nullptr)
        { return false; }

        Fresnel->Exponent = 3.0f;
        Fresnel->BaseReflectFraction = 0.02f;
        NoisePan->SpeedX = 0.05f;
        NoisePan->SpeedY = 0.03f;

        const auto Wire = &UMaterialEditingLibrary::ConnectMaterialExpressions;
        Wire(Uv, TEXT(""), NoisePan, TEXT(""));
        Wire(NoisePan, TEXT(""), NoiseTex, TEXT(""));

        auto* Erode = Build_ErodeChain(Mat, Dyn, TEXT("Dissolve"), NoiseTex, TEXT("R"), 0.30f);
        if (Erode == nullptr) { return false; }

        auto* One       = New_Expression<UMaterialExpressionConstant>(Mat, -560, 120);
        auto* BoostPlus = New_Expression<UMaterialExpressionAdd>(Mat, -420, 140);
        auto* RimBoost  = New_Expression<UMaterialExpressionMultiply>(Mat, -280, 60);
        auto* Emissive  = New_Expression<UMaterialExpressionMultiply>(Mat, -140, 100);
        auto* OpacMulA  = New_Expression<UMaterialExpressionMultiply>(Mat, -280, 460);
        auto* OpacMulB  = New_Expression<UMaterialExpressionMultiply>(Mat, -140, 500);
        if (One == nullptr || BoostPlus == nullptr || RimBoost == nullptr || Emissive == nullptr ||
            OpacMulA == nullptr || OpacMulB == nullptr)
        { return false; }

        One->R = 1.0f;

        Wire(One, TEXT(""), BoostPlus, TEXT("A"));
        Wire(Dyn, TEXT("Boost"), BoostPlus, TEXT("B"));
        Wire(Fresnel, TEXT(""), RimBoost, TEXT("A"));
        Wire(BoostPlus, TEXT(""), RimBoost, TEXT("B"));
        Wire(RimBoost, TEXT(""), Emissive, TEXT("A"));
        Wire(PColor, TEXT(""), Emissive, TEXT("B"));

        Wire(Erode, TEXT(""), OpacMulA, TEXT("A"));
        Wire(PColor, TEXT("A"), OpacMulA, TEXT("B"));
        Wire(OpacMulA, TEXT(""), OpacMulB, TEXT("A"));
        Wire(Fresnel, TEXT(""), OpacMulB, TEXT("B"));

        UMaterialEditingLibrary::ConnectMaterialProperty(Emissive, TEXT(""), MP_EmissiveColor);
        UMaterialEditingLibrary::ConnectMaterialProperty(OpacMulB, TEXT(""), MP_Opacity);

        return Finish_Material(Mat);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::particles_editor
{
    auto Generate_AllVfxMaterials() -> void
    {
        using namespace MatGenLocal;

        int32 Ok = 0, Total = 0;
        const auto BuildOne = [&](const TCHAR* InName, bool (*InFn)())
        {
            ++Total;
            if (InFn()) { ++Ok; }
            else { Log(TEXT("Failed to build VFX master material: {}"), FString(InName)); }
        };

        BuildOne(TEXT("M_CkParticles_SweepErode"),   &Build_SweepErode);
        BuildOne(TEXT("M_CkParticles_SoftSmoke"),    &Build_SoftSmoke);
        BuildOne(TEXT("M_CkParticles_FresnelShell"), &Build_FresnelShell);

        Log(TEXT("Generated {}/{} CkParticles VFX master materials under {}."),
            FString::FromInt(Ok), FString::FromInt(Total), FString(MatDir));
    }
}

// --------------------------------------------------------------------------------------------------------------------
