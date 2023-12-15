#pragma once

#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment_Data.h"

#include "CkEcsBasics/Transform/CkTransform_Processor.h"

#include "GameplayEffectTypes.h"

#include "CkAbilityOwner_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_AbilityOwner_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_AbilityOwner_NeedsSetup);

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
        FGameplayTagCountContainer _ActiveTags;

    public:
        auto Get_ActiveTags() const -> FGameplayTagContainer;
        auto Get_ActiveTagsWithCount() const -> TMap<FGameplayTag, int32>;
        auto Get_SpecificActiveTagCount(const FGameplayTag& InTag) const -> int32;

        auto AppendTags(const FGameplayTagContainer& InTagsToAdd) -> void;
        auto AddTag(const FGameplayTag& InTagToAdd) -> void;
        auto RemoveTags(const FGameplayTagContainer& InTagsToRemove) -> void;
        auto RemoveTag(const FGameplayTag& InTagToRemove) -> void;
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
        using GiveAbilityRequestType = FCk_Request_AbilityOwner_GiveAbility;
        using RevokeAbilityRequestType = FCk_Request_AbilityOwner_RevokeAbility;
        using ActivateAbilityRequestType = FCk_Request_AbilityOwner_ActivateAbility;
        using DeactivateAbilityRequestType = FCk_Request_AbilityOwner_DeactivateAbility;

        using RequestType = std::variant<GiveAbilityRequestType, RevokeAbilityRequestType,
            ActivateAbilityRequestType, DeactivateAbilityRequestType>;
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
        FCk_Delegate_AbilityOwner_Events_MC, FCk_Handle, TArray<FCk_AbilityOwner_Event>);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_AbilityOwner_Events_Replicate; }

UCLASS(Blueprintable)
class CKABILITY_API UCk_Fragment_AbilityOwner_Events_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_AbilityOwner_Events_Rep);

public:
    friend class ck::FProcessor_AbilityOwner_Events_Replicate;

public:
    auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

public:
    UFUNCTION()
    void OnRep_NewEvents();

private:
    UPROPERTY(ReplicatedUsing = OnRep_NewEvents)
    TArray<FCk_AbilityOwner_Event> _Events;
    int32 _NextEventToProcess = 0;
};

// --------------------------------------------------------------------------------------------------------------------
