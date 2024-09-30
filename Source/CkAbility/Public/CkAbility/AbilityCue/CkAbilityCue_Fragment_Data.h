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
        meta=(AllowPrivateAccess, ExposeOnSpawn))
    FGameplayTag _CueName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced,
        meta=(AllowPrivateAccess, ExposeOnSpawn))
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

UCLASS(Abstract)
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

public:
    UFUNCTION(BlueprintImplementableEvent)
    UCk_AbilityCue_Config_PDA*
    Get_ConfigForCue(TSubclassOf<class UCk_Ability_Script_PDA> InBlueprint);

private:
    UPROPERTY(Transient, EditInstanceOnly,
        meta=(AllowPrivateAccess, DisplayThumbnail="false", ForceInlineRow))
    TMap<FGameplayTag, FSoftObjectPath> _AbilityCues;

    UPROPERTY(Transient, EditInstanceOnly,
        meta=(AllowPrivateAccess, DisplayThumbnail="false", ForceInlineRow))
    TMap<FGameplayTag, TObjectPtr<UCk_AbilityCue_Config_PDA>> _AbilityCueConfigs;

public:
    CK_PROPERTY_GET(_AbilityCues);
    CK_PROPERTY_GET(_AbilityCueConfigs);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/CkAbility.Ck_Utils_AbilityCue_UE:Make_AbilityCue_Params"))
struct CKABILITY_API FCk_AbilityCue_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AbilityCue_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FVector _Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FVector _Normal = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, NotReplicated,
        meta=(AllowPrivateAccess))
    FCk_Handle _Instigator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, NotReplicated,
        meta=(AllowPrivateAccess))
    FCk_Handle _EffectCauser;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FInstancedStruct _CustomData;

public:
    CK_PROPERTY(_Location);
    CK_PROPERTY(_Normal);
    CK_PROPERTY(_Instigator);
    CK_PROPERTY(_EffectCauser);
    CK_PROPERTY(_CustomData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Request_AbilityCue_Spawn
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AbilityCue_Spawn);

public:
    FCk_Request_AbilityCue_Spawn() = default;
    FCk_Request_AbilityCue_Spawn(
        const FGameplayTag& InAbilityCueName,
        UObject* InWorldContextObject);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FGameplayTag _AbilityCueName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    TObjectPtr<UObject> _WorldContextObject;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    FCk_AbilityCue_Params _ReplicatedParams;

public:
    CK_PROPERTY_GET(_AbilityCueName);
    CK_PROPERTY_GET(_WorldContextObject);
    CK_PROPERTY(_ReplicatedParams);
};

// --------------------------------------------------------------------------------------------------------------------
