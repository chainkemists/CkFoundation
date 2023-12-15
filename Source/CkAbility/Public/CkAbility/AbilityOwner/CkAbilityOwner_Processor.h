#pragma once

#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKABILITY_API FProcessor_AbilityOwner_Setup : public TProcessor<
            FProcessor_AbilityOwner_Setup,
            FFragment_AbilityOwner_Params,
            FTag_AbilityOwner_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_AbilityOwner_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AbilityOwner_Params& InAbilityOwnerParams) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_AbilityOwner_HandleEvents : public TProcessor<
            FProcessor_AbilityOwner_HandleEvents,
            FFragment_AbilityOwner_Events,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_AbilityOwner_Events;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AbilityOwner_Events&  InAbilityOwnerEvents) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_AbilityOwner_HandleRequests : public TProcessor<
            FProcessor_AbilityOwner_HandleRequests,
            FFragment_AbilityOwner_Current,
            FFragment_AbilityOwner_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_AbilityOwner_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            FFragment_AbilityOwner_Requests& InAbilityRequestsComp) const -> void;

    private:
        auto DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_GiveAbility& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_RevokeAbility& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_ActivateAbility& InRequest) const -> void;

        auto DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_DeactivateAbility& InRequest) const -> void;

    private:
        auto DoFindAbilityByName(
            const FCk_Handle& InAbilityOwnerEntity,
            const FGameplayTag& InAbilityName) const-> FCk_Handle;

        auto DoFindAbilityByHandle(
            const FCk_Handle& InAbilityOwnerEntity,
            const FCk_Handle& InAbilityEntity) const-> FCk_Handle;
    };
}

// --------------------------------------------------------------------------------------------------------------------
