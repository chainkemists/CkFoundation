#pragma once

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkAggro_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Aggro_ExclusionPolicy : uint8
{
    IgnoreExcluded,
    All
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Aggro_ExclusionPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKAGGRO_API FCk_Handle_Aggro : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Aggro); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Aggro);

// --------------------------------------------------------------------------------------------------------------------

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Aggro_FloatAttribute_Name);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_Fragment_Aggro_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Aggro_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_Entity_ConstructionScript_PDA> _ConstructionScript;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ScoreStartingValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
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
    CK_PROPERTY(_ConstructionScript);
    CK_PROPERTY(_ScoreStartingValue);
    CK_PROPERTY(_MinMax);

    // _MinValue / _MaxValue intentionally use only the setter macro — the
    // matching getters (Get_MinValue / Get_MaxValue) are hand-authored above
    // because they emit a diagnostic when _MinMax doesn't include the bound
    // being queried. CK_PROPERTY would duplicate the getter and lose that
    // diagnostic, so we expose only the Set side.
    CK_PROPERTY_SET(_MinValue);
    CK_PROPERTY_SET(_MaxValue);
};

// --------------------------------------------------------------------------------------------------------------------
