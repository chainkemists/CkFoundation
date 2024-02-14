#pragma once

#include "CkAbility/Ability/CkAbility_Fragment_Data.h"

#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcs/Handle/CkHandle.h"

#include <InstancedStruct.h>

#include "CkAbilityOwner_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_AbilityOwner_AbilitySearch_Policy : uint8
{
    SearchByClass,
    SearchByHandle
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_AbilityOwner_AbilitySearch_Policy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak="/Script/CkEcs.Ck_Utils_Handle_UE:Conv_HandleTypeSafeToHandle"))
struct CKABILITY_API FCk_Handle_AbilityOwner : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AbilityOwner); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AbilityOwner);

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
    FCk_Handle _ContextEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FInstancedStruct _EventData;

public:
    CK_PROPERTY_GET(_EventName);
    CK_PROPERTY(_ContextEntity);
    CK_PROPERTY(_EventData);

    CK_DEFINE_CONSTRUCTORS(FCk_AbilityOwner_Event, _EventName);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Fragment_AbilityOwner_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AbilityOwner_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<TSubclassOf<class UCk_Ability_Script_PDA>> _DefaultAbilities;

public:
    CK_PROPERTY_GET(_DefaultAbilities);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_AbilityOwner_ParamsData, _DefaultAbilities);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKABILITY_API FCk_Request_AbilityOwner_GiveAbility
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AbilityOwner_GiveAbility);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<class UCk_Ability_Script_PDA> _AbilityScriptClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Ability_Payload_OnGranted _OptionalPayload;

public:
    CK_PROPERTY_GET(_AbilityScriptClass)
    CK_PROPERTY(_OptionalPayload)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AbilityOwner_GiveAbility, _AbilityScriptClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta= (HasNativeMake))
struct CKABILITY_API FCk_Request_AbilityOwner_RevokeAbility
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AbilityOwner_RevokeAbility);

public:
    FCk_Request_AbilityOwner_RevokeAbility() = default;

    explicit
    FCk_Request_AbilityOwner_RevokeAbility(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass);

    explicit
    FCk_Request_AbilityOwner_RevokeAbility(
        FCk_Handle_Ability InAbilityHandle);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_AbilityOwner_AbilitySearch_Policy _SearchPolicy = ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition="_SearchPolicy == ECk_AbilityOwner_AbilitySearchPolicy::SearchByName"))
    TSubclassOf<UCk_Ability_Script_PDA> _AbilityClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition="_SearchPolicy == ECk_AbilityOwner_AbilitySearchPolicy::SearchByHandle"))
    FCk_Handle_Ability _AbilityHandle;

public:
    CK_PROPERTY_GET(_SearchPolicy);
    CK_PROPERTY_GET(_AbilityClass);
    CK_PROPERTY_GET(_AbilityHandle);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta= (HasNativeMake))
struct CKABILITY_API FCk_Request_AbilityOwner_ActivateAbility
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AbilityOwner_ActivateAbility);

public:
    FCk_Request_AbilityOwner_ActivateAbility() = default;

    explicit FCk_Request_AbilityOwner_ActivateAbility(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass);

    explicit FCk_Request_AbilityOwner_ActivateAbility(
        FCk_Handle_Ability InAbilityHandle);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_AbilityOwner_AbilitySearch_Policy _SearchPolicy = ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition="_SearchPolicy == ECk_AbilityOwner_AbilitySearchPolicy::SearchByName"))
    TSubclassOf<UCk_Ability_Script_PDA> _AbilityClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition="_SearchPolicy == ECk_AbilityOwner_AbilitySearchPolicy::SearchByHandle"))
    FCk_Handle_Ability _AbilityHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Ability_Payload_OnActivate _OptionalPayload;

public:
    CK_PROPERTY_GET(_SearchPolicy);
    CK_PROPERTY_GET(_AbilityClass);
    CK_PROPERTY_GET(_AbilityHandle);
    CK_PROPERTY(_OptionalPayload);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta= (HasNativeMake))
struct CKABILITY_API FCk_Request_AbilityOwner_DeactivateAbility
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AbilityOwner_DeactivateAbility);

public:
    FCk_Request_AbilityOwner_DeactivateAbility() = default;

    explicit
    FCk_Request_AbilityOwner_DeactivateAbility(
        TSubclassOf<UCk_Ability_Script_PDA> InAbilityClass);

    explicit
    FCk_Request_AbilityOwner_DeactivateAbility(
        FCk_Handle_Ability InAbilityHandle);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_AbilityOwner_AbilitySearch_Policy _SearchPolicy = ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition="_SearchPolicy == ECk_AbilityOwner_AbilitySearchPolicy::SearchByClass"))
    TSubclassOf<UCk_Ability_Script_PDA> _AbilityClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                  EditCondition="_SearchPolicy == ECk_AbilityOwner_AbilitySearchPolicy::SearchByHandle"))
    FCk_Handle_Ability _AbilityHandle;

public:
    CK_PROPERTY_GET(_SearchPolicy);
    CK_PROPERTY_GET(_AbilityClass);
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

    CK_DEFINE_CONSTRUCTORS(FCk_Request_AbilityOwner_SendEvent, _Event);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_AbilityOwner_Events,
    FCk_Handle_AbilityOwner, InHandle,
    const TArray<FCk_AbilityOwner_Event>&, InEvents);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_AbilityOwner_Events_MC,
    FCk_Handle_AbilityOwner, InHandle,
    const TArray<FCk_AbilityOwner_Event>&, InEvents);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_AbilityOwner_OnTagsUpdated,
    FCk_Handle_AbilityOwner, InHandle,
    const FGameplayTagContainer&, InActiveTags);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_AbilityOwner_OnTagsUpdated_MC,
    FCk_Handle_AbilityOwner, InHandle,
    const FGameplayTagContainer&, InActiveTags);

// --------------------------------------------------------------------------------------------------------------------
