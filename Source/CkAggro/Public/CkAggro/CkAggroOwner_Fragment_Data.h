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
struct FCk_Fragment_AggroOwner_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AggroOwner_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    float _AggroRange = 100.0f;

public:
    CK_PROPERTY_GET(_AggroRange);
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
