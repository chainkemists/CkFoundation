#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Engine/DataAsset.h>
#include <GameplayTagContainer.h>

#include "CkNavFilterDefinition_DataAsset.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Provider-neutral description of a navigation query filter. Each provider adapter compiles this
// into its own native form; nothing here names an engine type.
USTRUCT(BlueprintType)
struct CKNAVIGATION_API FCk_NavFilter_Definition
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_NavFilter_Definition);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _RequiredAreaTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _ExcludedAreaTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TMap<FGameplayTag, float> _AreaCostMultipliers;

public:
    CK_PROPERTY(_RequiredAreaTags);
    CK_PROPERTY(_ExcludedAreaTags);
    CK_PROPERTY(_AreaCostMultipliers);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKNAVIGATION_API UCk_NavFilterDefinition_DataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_NavFilterDefinition_DataAsset);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_NavFilter_Definition _Definition;

public:
    CK_PROPERTY_GET(_Definition);
};

// --------------------------------------------------------------------------------------------------------------------
