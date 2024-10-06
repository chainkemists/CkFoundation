#pragma once

#include "CkAggro/CkAggro_Fragment_Data.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkAggroOwner_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKAGGRO_API FCk_Handle_AggroOwner : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AggroOwner); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AggroOwner);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_Aggro_Filter_Distance
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Aggro_Filter_Distance);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    float _AggroRange = 100.0f;

public:
    CK_PROPERTY_GET(_AggroRange);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_Aggro_Filter_LoS
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Aggro_Filter_LoS);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_Fragment_AggroOwner_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AggroOwner_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    bool _FilterByDistance = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess, EditCondition="_FilterByDistance"))
    FCk_Aggro_Filter_Distance _AggroFilter_Distance;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    bool _FilterByLoS = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess, EditCondition="_FilterByLoS"))
    FCk_Aggro_Filter_LoS _AggroFilter_LoS;

public:
    CK_PROPERTY_GET(_FilterByDistance);
    CK_PROPERTY_GET(_AggroFilter_Distance);

    CK_PROPERTY_GET(_FilterByLoS);
    CK_PROPERTY_GET(_AggroFilter_LoS);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Aggro_OnNewAggroAdded,
    FCk_Handle_AggroOwner, InAggroOwner,
    FCk_Handle_Aggro, InNewAggro);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Aggro_OnNewAggroAdded_MC,
    FCk_Handle_AggroOwner, InAggroOwner,
    FCk_Handle_Aggro, InNewAggro);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Aggro_OnAggroChanged,
    FCk_Handle_AggroOwner, InAggroOwner,
    FCk_Handle_Aggro, InPrevAggro,
    FCk_Handle_Aggro, InNewAggro);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_Aggro_OnAggroChanged_MC,
    FCk_Handle_AggroOwner, InAggroOwner,
    FCk_Handle_Aggro, InPrevAggro,
    FCk_Handle_Aggro, InNewAggro);

// --------------------------------------------------------------------------------------------------------------------
