#pragma once
#include "CkAbility/Ability/CkAbility_Fragment_Data.h"

#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkProvider/CkProvider_Data.h"

#include "CkAbilityOwner_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AbilityOwner_AbilitySearchPolicy : uint8
{
    SearchByName,
    SearchByHandle
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AbilityOwner_AbilitySearchPolicy);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType)
class CKABILITY_API UCk_Ability_EventData_UE : public UCk_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ability_EventData_UE);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_AbilityOwner_Event
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AbilityOwner_Event);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FGameplayTag _EventName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Ability_EventData_UE> _EventData;

public:
    CK_PROPERTY_GET(_EventName);
    CK_PROPERTY_GET(_EventData);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Fragment_AbilityOwner_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AbilityOwner_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced,
              meta = (AllowPrivateAccess = true))
    TArray<const class UCk_Ability_EntityConfig_PDA*> _DefaultAbilities;

public:
    CK_PROPERTY_GET(_DefaultAbilities);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Request_AbilityOwner_GiveAbility
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AbilityOwner_GiveAbility);

public:
    FCk_Request_AbilityOwner_GiveAbility() = default;

    explicit
    FCk_Request_AbilityOwner_GiveAbility(
        const UCk_Ability_EntityConfig_PDA* InAbilityEntityConfig);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<const UCk_Ability_EntityConfig_PDA> _AbilityEntityConfig;

public:
    CK_PROPERTY_GET(_AbilityEntityConfig)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Request_AbilityOwner_RevokeAbility
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AbilityOwner_RevokeAbility);

public:
    FCk_Request_AbilityOwner_RevokeAbility() = default;

    explicit
    FCk_Request_AbilityOwner_RevokeAbility(
        FGameplayTag InAbilityName);

    explicit
    FCk_Request_AbilityOwner_RevokeAbility(
        FCk_Handle InAbilityHandle);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_AbilityOwner_AbilitySearchPolicy _SearchPolicy = ECk_AbilityOwner_AbilitySearchPolicy::SearchByName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition="_SearchPolicy==ECk_AbilityOwner_AbilitySearchPolicy::SearchByName",
                  GameplayTagFilter = "GameplayAbility"))
    FGameplayTag _AbilityName = FGameplayTag::EmptyTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition="_SearchPolicy==ECk_AbilityOwner_AbilitySearchPolicy::SearchByHandle"))
    FCk_Handle _AbilityHandle;

public:
    CK_PROPERTY_GET(_SearchPolicy);
    CK_PROPERTY_GET(_AbilityName);
    CK_PROPERTY_GET(_AbilityHandle);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Request_AbilityOwner_ActivateAbility
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AbilityOwner_ActivateAbility);

public:
    FCk_Request_AbilityOwner_ActivateAbility() = default;

    explicit
    FCk_Request_AbilityOwner_ActivateAbility(
        FGameplayTag InAbilityName);

    explicit
    FCk_Request_AbilityOwner_ActivateAbility(
        FCk_Handle InAbilityHandle);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_AbilityOwner_AbilitySearchPolicy _SearchPolicy = ECk_AbilityOwner_AbilitySearchPolicy::SearchByName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition="_SearchPolicy==ECk_GameplayAbilityOwner_AbilitySearchPolicy::SearchByName",
                  GameplayTagFilter = "GameplayAbility"))
    FGameplayTag _AbilityName = FGameplayTag::EmptyTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition="_SearchPolicy==ECk_GameplayAbilityOwner_AbilitySearchPolicy::SearchByHandle"))
    FCk_Handle _AbilityHandle;

public:
    CK_PROPERTY_GET(_SearchPolicy);
    CK_PROPERTY_GET(_AbilityName);
    CK_PROPERTY_GET(_AbilityHandle);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Request_AbilityOwner_DeactivateAbility
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AbilityOwner_DeactivateAbility);

public:
    FCk_Request_AbilityOwner_DeactivateAbility() = default;

    explicit
    FCk_Request_AbilityOwner_DeactivateAbility(
        FGameplayTag InAbilityName);

    explicit
    FCk_Request_AbilityOwner_DeactivateAbility(
        FCk_Handle InAbilityHandle);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_AbilityOwner_AbilitySearchPolicy _SearchPolicy = ECk_AbilityOwner_AbilitySearchPolicy::SearchByName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition="_SearchPolicy==ECk_AbilityOwner_AbilitySearchPolicy::SearchByName",
                  GameplayTagFilter = "GameplayAbility"))
    FGameplayTag _AbilityName = FGameplayTag::EmptyTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition="_SearchPolicy==ECk_AbilityOwner_AbilitySearchPolicy::SearchByHandle"))
    FCk_Handle _AbilityHandle;

public:
    CK_PROPERTY_GET(_SearchPolicy);
    CK_PROPERTY_GET(_AbilityName);
    CK_PROPERTY_GET(_AbilityHandle);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Request_AbilityOwner_SendEvent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AbilityOwner_SendEvent);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AbilityOwner_Event _Event;

public:
    CK_PROPERTY_GET(_Event);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_AbilityOwner_Events,
    FCk_Handle, InHandle,
    const TArray<FCk_AbilityOwner_Event>&,
    InEvents);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_AbilityOwner_Events_MC,
    FCk_Handle, InHandle,
    const TArray<FCk_AbilityOwner_Event>&,
    InEvents);

// --------------------------------------------------------------------------------------------------------------------
