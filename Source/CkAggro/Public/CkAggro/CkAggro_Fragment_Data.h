#pragma once

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkAggro_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_Fragment_Aggro_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Aggro_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    TSubclassOf<UCk_Entity_ConstructionScript_PDA> _ConstructionScript;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ScoreStartingValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    ECk_MinMax _MinMax = ECk_MinMax::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MinMax == ECk_MinMax::Min || _MinMax == ECk_MinMax::MinMax"))
    float _MinValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MinMax == ECk_MinMax::Max || _MinMax == ECk_MinMax::MinMax"))
    float _MaxValue = 0.0f;

public:
    auto
    Get_MinValue() const -> float;

    auto
    Get_MaxValue() const -> float;

public:
    CK_PROPERTY_GET(_ConstructionScript);
    CK_PROPERTY_GET(_ScoreStartingValue);
    CK_PROPERTY_GET(_MinMax);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKAGGRO_API FCk_Handle_Aggro : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Aggro); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Aggro);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Aggro_FloatAttribute_Name);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Aggro_OnNewAggroAdded,
    FCk_Handle, InAggroOwner,
    FCk_Handle_Aggro, InNewAggro);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Aggro_OnNewAggroAdded_MC,
    FCk_Handle, InAggroOwner,
    FCk_Handle_Aggro, InNewAggro);

// --------------------------------------------------------------------------------------------------------------------
