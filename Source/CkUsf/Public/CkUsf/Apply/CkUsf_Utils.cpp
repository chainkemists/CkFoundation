#include "CkUsf/Apply/CkUsf_Utils.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition.h"
#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"
#include "CkUsf_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

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
            case ECk_Usf_ParamType::Texture:
                if (auto* T = P._DefaultTexture.LoadSynchronous())
                { MID->SetTextureParameterValue(P._Name, T); }
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

// --------------------------------------------------------------------------------------------------------------------
