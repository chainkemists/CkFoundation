#pragma once

#include "CkCore/Types/DataAsset/CkDataAsset.h"

#include "CkEcs/Handle/CkHandle.h"

#include <GameplayTagContainer.h>

#include "CkAbilityCue_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityBridge_Config_Base_PDA;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKABILITY_API UCk_AbilityCue_Config_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_AbilityCue_Config_PDA);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        meta=(AllowPrivateAccess))
    FGameplayTag _CueName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced,
        meta=(AllowPrivateAccess))
    TObjectPtr<UCk_EntityBridge_Config_Base_PDA> _EntityConfig;

public:
    // TODO: prevent saving the asset if the following checks do NOT pass:
    // - the asset is created in a folder that does NOT have a AbilityCue_Aggregator
    // - the asset has a valid CueName
    // - the CueName is not shared with any other Cue

public:
    CK_PROPERTY_GET(_CueName);
    CK_PROPERTY_GET(_EntityConfig);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKABILITY_API UCk_AbilityCue_Aggregator_PDA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_AbilityCue_Aggregator_PDA);

public:
    using ConfigType = UCk_AbilityCue_Config_PDA;

public:
    auto Request_Populate() -> void;

public:
    auto PreSave(FObjectPreSaveContext ObjectSaveContext) -> void override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess))
    TArray<FSoftObjectPath> _AbilityCues;

public:
    CK_PROPERTY_GET(_AbilityCues);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Request_AbilityCue_Spawn
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AbilityCue_Spawn);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FGameplayTag _AbilityCueName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    TObjectPtr<UObject> _WorldContextObject;

public:
    CK_PROPERTY_GET(_AbilityCueName);
    CK_PROPERTY_GET(_WorldContextObject);
};

// --------------------------------------------------------------------------------------------------------------------
