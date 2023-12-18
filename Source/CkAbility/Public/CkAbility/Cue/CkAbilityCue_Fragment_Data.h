#pragma once
#include "CkCore/Types/DataAsset/CkDataAsset.h"

 #include "CkAbilityCue_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityBridge_Config_Base_PDA;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKABILITY_API UCk_AbilityCue_Aggregator_DA : public UCk_DataAsset_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_AbilityCue_Aggregator_DA);

public:
    auto Request_Populate() -> void;

public:
    auto PreSave(FObjectPreSaveContext ObjectSaveContext) -> void override;

private:
    UPROPERTY(VisibleAnywhere, meta=(AllowPrivateAccess))
    TArray<TSoftObjectPtr<UCk_EntityBridge_Config_Base_PDA>> _AbilityCues;

public:
    CK_PROPERTY_GET(_AbilityCues);
};

// --------------------------------------------------------------------------------------------------------------------
