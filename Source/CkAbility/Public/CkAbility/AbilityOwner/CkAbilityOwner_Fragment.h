#pragma once

#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment_Data.h"

#include "CkEcsExt/Transform/CkTransform_Processor.h"

#include "GameplayEffectTypes.h"

#include "CkAbilityOwner_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_AbilityOwner_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG_COUNTED(FTag_AbilityOwner_BlockSubAbilities);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_AbilityOwner_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_AbilityOwner_TagsUpdated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_AbilityOwner_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_AbilityOwner_Params);

    public:
        using ParamsType = FCk_Fragment_AbilityOwner_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

        CK_DEFINE_CONSTRUCTORS(FFragment_AbilityOwner_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_AbilityOwner_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_AbilityOwner_Current);

    private:
        // Aggregate list of all tags granted by the owned active abilities
        FGameplayTagCountContainer _PreviousTags;
        FGameplayTagCountContainer _ActiveTags;

        FGameplayTagContainer _CachedActiveExtensionTags;

        FGameplayTagContainer _PreviousTags_IncludingAllEntityExtensions;
        FGameplayTagContainer _ActiveTags_IncludingAllEntityExtensions;

        FGameplayTagContainer _RelevantTagsFromAbilityOwner;

        friend class FProcessor_AbilityOwner_Teardown;
        friend class UCk_Utils_AbilityOwner_UE; // Needed for _RelevantTagsFromAbilityOwner, remove if we remove this variable

    public:
        auto Get_ActiveTags(
            const FCk_Handle_AbilityOwner& InAbilityOwner) const -> FGameplayTagContainer;
        auto Get_PreviousTags(
            const FCk_Handle_AbilityOwner& InAbilityOwner) const -> FGameplayTagContainer;
        auto Get_ActiveTagsWithCount(
            const FCk_Handle_AbilityOwner& InAbilityOwner) const -> TMap<FGameplayTag, int32>;
        auto Get_SpecificActiveTagCount(
            const FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTag& InTag) const -> int32;
        auto Get_AreActiveTagsDifferentThanPreviousTags() const -> bool;
        auto Get_AreActiveTagsDifferentThanPreviousTags(
            const FCk_Handle_AbilityOwner& InAbilityOwner) const -> bool;

        auto AppendTags(
            FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTagContainer& InTagsToAdd) -> void;
        auto AddTag(
            FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTag& InTagToAdd) -> void;
        auto RemoveTags(
            FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTagContainer& InTagsToRemove) -> void;
        auto RemoveTag(
            FCk_Handle_AbilityOwner& InAbilityOwner,
            const FGameplayTag& InTagToRemove) -> void;
        auto UpdatePreviousTags() -> void;

    private:
        auto
        Do_TagsUpdated(
            FCk_Handle_AbilityOwner& InAbilityOwner) -> void;

        auto
        Do_CacheExtensionTags(
            FCk_Handle_AbilityOwner& InAbilityOwner) -> void;

        auto
        DoTry_TagsUpdatedOnExtensionOwner(
            FCk_Handle_AbilityOwner& InAbilityOwner) -> void;

        // Count of tags in this ability owner with cached extension tags and with RelevantTagsFromAbilityOwner removed
        auto Get_ActiveTagsForExtensionCache() const -> FGameplayTagContainer;

        // Count of tags in this ability owner without extensions and with RelevantTagsFromAbilityOwner removed
        auto Get_SpecificActiveTagCountForExtension(
            const FGameplayTag& InTag) const -> int32;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_AbilityOwner_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_AbilityOwner_Requests);

    public:
        friend class FProcessor_AbilityOwner_HandleRequests;
        friend class UCk_Utils_AbilityOwner_UE;

    public:
        using AddOrGiveAbilityRequestType = FCk_Request_AbilityOwner_AddAndGiveExistingAbility;
        using TransferAbilityRequestType = FCk_Request_AbilityOwner_TransferExistingAbility;
        using GiveAbilityRequestType = FCk_Request_AbilityOwner_GiveAbility;
        using GiveAbilityReplicatedRequestType = FCk_Request_AbilityOwner_GiveReplicatedAbility;
        using RevokeAbilityRequestType = FCk_Request_AbilityOwner_RevokeAbility;
        using ActivateAbilityRequestType = FCk_Request_AbilityOwner_ActivateAbility;
        using DeactivateAbilityRequestType = FCk_Request_AbilityOwner_DeactivateAbility;
        using CancelSubAbilities = FCk_Request_AbilityOwner_CancelSubAbilities;

        using RequestType = std::variant<AddOrGiveAbilityRequestType, GiveAbilityRequestType, GiveAbilityReplicatedRequestType,
            RevokeAbilityRequestType, ActivateAbilityRequestType, DeactivateAbilityRequestType, CancelSubAbilities, TransferAbilityRequestType>;
        using RequestList = TArray<RequestType>;

    public:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKABILITY_API FFragment_AbilityOwner_Events
    {
    public:
        CK_GENERATED_BODY(FFragment_AbilityOwner_Events);

    public:
        friend class UCk_Utils_AbilityOwner_UE;
        friend class FProcessor_AbilityOwner_HandleEvents;

    public:
        using EventType = FCk_AbilityOwner_Event;
        using EventList = TArray<EventType>;

    public:
        EventList _Events;

    public:
        CK_PROPERTY_GET(_Events);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, AbilityOwner_Events,
        FCk_Delegate_AbilityOwner_Events_MC,
        FCk_Handle_AbilityOwner,
        TArray<FCk_AbilityOwner_Event>);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, AbilityOwner_SingleEvent,
        FCk_Delegate_AbilityOwner_Event_MC,
        FCk_Handle_AbilityOwner,
        FCk_AbilityOwner_Event);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, AbilityOwner_OnTagsUpdated,
        FCk_Delegate_AbilityOwner_OnTagsUpdated_MC,
        FCk_Handle_AbilityOwner,
        FGameplayTagContainer,
        FGameplayTagContainer,
        FGameplayTagContainer,
        FGameplayTagContainer);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, AbilityOwner_OnAbilityGivenOrNot,
        FCk_Delegate_AbilityOwner_OnAbilityGivenOrNot_MC,
        FCk_Handle_AbilityOwner,
        FCk_Handle_Ability,
        ECk_AbilityOwner_AbilityGivenOrNot);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, AbilityOwner_OnAbilityGiven,
        FCk_Delegate_AbilityOwner_OnAbilityGiven_MC,
        FCk_Handle_AbilityOwner,
        FCk_Handle_Ability);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, AbilityOwner_OnAbilityRevokedOrNot,
        FCk_Delegate_AbilityOwner_OnAbilityRevokedOrNot_MC,
        FCk_Handle_AbilityOwner,
        FCk_Handle_Ability,
        ECk_AbilityOwner_AbilityRevokedOrNot);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, AbilityOwner_OnAbilityRevoked,
        FCk_Delegate_AbilityOwner_OnAbilityRevoked_MC,
        FCk_Handle_AbilityOwner,
        FCk_Handle_Ability);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, AbilityOwner_OnAbilityActivatedOrNot,
        FCk_Delegate_AbilityOwner_OnAbilityActivatedOrNot_MC,
        FCk_Handle_AbilityOwner,
        FCk_Handle_Ability,
        ECk_AbilityOwner_AbilityActivatedOrNot);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, AbilityOwner_OnAbilityActivated,
        FCk_Delegate_AbilityOwner_OnAbilityActivated_MC,
        FCk_Handle_AbilityOwner,
        FCk_Handle_Ability);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, AbilityOwner_OnAbilityDeactivatedOrNot,
        FCk_Delegate_AbilityOwner_OnAbilityDeactivatedOrNot_MC,
        FCk_Handle_AbilityOwner,
        FCk_Handle_Ability,
        ECk_AbilityOwner_AbilityDeactivatedOrNot);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, AbilityOwner_OnAbilityDeactivated,
        FCk_Delegate_AbilityOwner_OnAbilityDeactivated_MC,
        FCk_Handle_AbilityOwner,
        FCk_Handle_Ability);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, AbilityOwner_OnAbilityTransferredOrNot,
        FCk_Delegate_AbilityOwner_OnAbilityTransferredOrNot_MC,
        FCk_Handle_AbilityOwner,
        FCk_Handle_AbilityOwner,
        FCk_Handle_Ability,
        ECk_AbilityOwner_AbilityTransferredOrNot);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKABILITY_API, AbilityOwner_OnAbilityTransferred,
        FCk_Delegate_AbilityOwner_OnAbilityTransferred_MC,
        FCk_Handle_AbilityOwner,
        FCk_Handle_AbilityOwner,
        FCk_Handle_Ability);
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKABILITY_API UCk_Fragment_AbilityOwner_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_AbilityOwner_Rep);

public:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>&) const -> void override;

    auto
    Request_TryUpdateReplicatedFragment()-> void;

private:
    UFUNCTION()
    void
    OnRep_PendingTransferExistingAbilityRequests();

    UFUNCTION()
    void
    OnRep_PendingGiveAbilityRequests();

    UFUNCTION()
    void
    OnRep_PendingRevokeAbilityRequests();

private:
    UPROPERTY(ReplicatedUsing = OnRep_PendingTransferExistingAbilityRequests)
    TArray<FCk_Request_AbilityOwner_TransferExistingAbility> _PendingTransferExistingAbilityRequests;
    int32 _NextPendingTransferExistingAbilityRequests = 0;

    UPROPERTY(ReplicatedUsing = OnRep_PendingGiveAbilityRequests)
    TArray<FCk_Request_AbilityOwner_GiveAbility> _PendingGiveAbilityRequests;
    int32 _NextPendingGiveAbilityRequests = 0;

    UPROPERTY(ReplicatedUsing = OnRep_PendingRevokeAbilityRequests)
    TArray<FCk_Request_AbilityOwner_RevokeAbility> _PendingRevokeAbilityRequests;
    int32 _NextPendingRevokeAbilityRequests = 0;

public:
    auto
    Request_TransferExistingAbility(
        const FCk_Request_AbilityOwner_TransferExistingAbility& InRequest) -> void;

    auto
    Request_GiveAbility(
        const FCk_Request_AbilityOwner_GiveAbility& InRequest) -> void;

    auto
    Request_RevokeAbility(
        const FCk_Request_AbilityOwner_RevokeAbility& InRequest) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
