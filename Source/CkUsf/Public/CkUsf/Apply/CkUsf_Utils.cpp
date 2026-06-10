#include "CkUsf/Apply/CkUsf_Utils.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition.h"
#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
#include "CkUsf_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "RHIGlobals.h"
#include "Engine/Texture.h"
#include "Camera/CameraComponent.h"
#include "Components/PostProcessComponent.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Usf_UE::
    Get_LookMasterMaterial(
        const UCkUsf_LookDefinition* InLook)
    -> UMaterialInterface*
{
    if (ck::Is_NOT_Valid(InLook, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::usf::Warning(TEXT("Null look passed to Get_LookMasterMaterial"));
        return nullptr;
    }

    const auto ObjPath = ck::usf::Get_GeneratedMasterObjectPath(InLook->Get_EffectiveLookName());
    auto* Mat = LoadObject<UMaterialInterface>(nullptr, *ObjPath);

    if (ck::Is_NOT_Valid(Mat, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::usf::Warning(TEXT("No generated master for look [{}] at [{}] - run Generate Look Materials"),
            InLook->Get_EffectiveLookName(), ObjPath);
        return nullptr;
    }

    // A master whose shaders failed to compile renders as the engine's checkered DefaultMaterial
    // with no error at the point of use — name the look loudly instead. Generation-time validation
    // can miss permutations the generating session never compiled (e.g. ray-tracing hit shaders
    // when RT was enabled after the look was generated).
    if (auto* AsMaterial = Mat->GetMaterial();
        AsMaterial != nullptr && AsMaterial->IsCompilingOrHadCompileError(GMaxRHIShaderPlatform))
    {
        ck::usf::Warning(
            TEXT("Look [{}] master material has shader compile errors (or is still compiling) — it will "
                 "render as the checkered DefaultMaterial. See LogShaderCompilers for the HLSL error, "
                 "then re-run Generate Look Materials or 'RecompileShaders Material {}'."),
            InLook->Get_EffectiveLookName(), AsMaterial->GetName());
    }
    return Mat;
}

auto
    UCk_Utils_Usf_UE::
    Create_MID_ForLook(
        const UCkUsf_LookDefinition* InLook,
        UObject* InOuter)
    -> UMaterialInstanceDynamic*
{
    auto* Master = Get_LookMasterMaterial(InLook);
    if (ck::Is_NOT_Valid(Master, ck::IsValid_Policy_NullptrOnly{}))
    { return nullptr; }

    auto* MID = UMaterialInstanceDynamic::Create(Master, InOuter);

    for (const auto& P : InLook->_Parameters)
    {
        switch (P._Type)
        {
            case ECk_Usf_ParamType::Scalar:
                MID->SetScalarParameterValue(P._Name, P._DefaultScalar);
                break;
            case ECk_Usf_ParamType::Vector:
                MID->SetVectorParameterValue(P._Name, P._DefaultVector);
                break;
            case ECk_Usf_ParamType::Texture2D:
            case ECk_Usf_ParamType::TextureCube:
                if (P._DefaultTexturePath.IsEmpty() == false)
                {
                    if (auto* T = LoadObject<UTexture>(nullptr, *P._DefaultTexturePath))
                    { MID->SetTextureParameterValue(P._Name, T); }
                }
                break;
        }
    }
    return MID;
}

auto
    UCk_Utils_Usf_UE::
    Set_Scalar(
        UMaterialInstanceDynamic* InMID, FName InName, float InValue)
    -> void
{
    if (ck::IsValid(InMID, ck::IsValid_Policy_NullptrOnly{}))
    { InMID->SetScalarParameterValue(InName, InValue); }
}

auto
    UCk_Utils_Usf_UE::
    Set_Vector(
        UMaterialInstanceDynamic* InMID, FName InName, FLinearColor InValue)
    -> void
{
    if (ck::IsValid(InMID, ck::IsValid_Policy_NullptrOnly{}))
    { InMID->SetVectorParameterValue(InName, InValue); }
}

auto
    UCk_Utils_Usf_UE::
    Set_Texture(
        UMaterialInstanceDynamic* InMID, FName InName, UTexture* InValue)
    -> void
{
    if (ck::IsValid(InMID, ck::IsValid_Policy_NullptrOnly{}))
    { InMID->SetTextureParameterValue(InName, InValue); }
}

auto
    UCk_Utils_Usf_UE::
    Apply_PostProcess_ToCamera(
        UCameraComponent* InCamera,
        const UCkUsf_LookDefinition* InLook)
    -> UMaterialInstanceDynamic*
{
    if (ck::Is_NOT_Valid(InCamera, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::usf::Warning(TEXT("Apply_PostProcess_ToCamera: null camera"));
        return nullptr;
    }

    auto* MID = Create_MID_ForLook(InLook, InCamera);
    if (ck::Is_NOT_Valid(MID, ck::IsValid_Policy_NullptrOnly{}))
    { return nullptr; }

    constexpr auto Weight = 1.0f;
    InCamera->PostProcessSettings.AddBlendable(MID, Weight);
    return MID;
}

auto
    UCk_Utils_Usf_UE::
    Apply_PostProcess_ToComponent(
        UPostProcessComponent* InPostProcess,
        const UCkUsf_LookDefinition* InLook)
    -> UMaterialInstanceDynamic*
{
    if (ck::Is_NOT_Valid(InPostProcess, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::usf::Warning(TEXT("Apply_PostProcess_ToComponent: null post-process component"));
        return nullptr;
    }

    auto* MID = Create_MID_ForLook(InLook, InPostProcess);
    if (ck::Is_NOT_Valid(MID, ck::IsValid_Policy_NullptrOnly{}))
    { return nullptr; }

    constexpr auto Weight = 1.0f;
    InPostProcess->Settings.AddBlendable(MID, Weight);
    return MID;
}

// --------------------------------------------------------------------------------------------------------------------
