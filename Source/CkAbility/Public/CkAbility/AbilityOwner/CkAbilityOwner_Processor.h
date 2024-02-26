#pragma once

#include "CkAbility/Ability/CkAbility_Fragment.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKABILITY_API FProcessor_AbilityOwner_Setup : public ck_exp::TProcessor<
            FProcessor_AbilityOwner_Setup,
            FCk_Handle_AbilityOwner,
            FFragment_AbilityOwner_Params,
            FTag_AbilityOwner_NeedsSetup,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_AbilityOwner_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto Tick(
            FCk_Time InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_AbilityOwner_Params& InAbilityOwnerParams) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_AbilityOwner_HandleEvents : public ck_exp::TProcessor<
            FProcessor_AbilityOwner_HandleEvents,
            FCk_Handle_AbilityOwner,
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
            HandleType& InHandle,
            const FFragment_AbilityOwner_Events&  InAbilityOwnerEvents) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_AbilityOwner_HandleRequests : public ck_exp::TProcessor<
            FProcessor_AbilityOwner_HandleRequests,
            FCk_Handle_AbilityOwner,
            FFragment_AbilityOwner_Current,
            FFragment_AbilityOwner_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_AbilityOwner_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            FFragment_AbilityOwner_Requests& InAbilityRequestsComp) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_GiveAbility& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_RevokeAbility& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_ActivateAbility& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_DeactivateAbility& InRequest) const -> void;

    private:
        static auto
        DoFindAbilityByClass(
            FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
            const TSubclassOf<UCk_Ability_Script_PDA>& InAbilityClass) -> FCk_Handle_Ability;

        static auto
        DoFindAbilityByHandle(
            const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
            const FCk_Handle_Ability& InAbilityEntity) -> FCk_Handle_Ability;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_AbilityOwner_TagsUpdated : public ck_exp::TProcessor<
            FProcessor_AbilityOwner_TagsUpdated,
            FCk_Handle_AbilityOwner,
            FFragment_AbilityOwner_Current,
            FTag_AbilityOwner_TagsUpdated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Super = TProcessor;
        using MarkedDirtyBy = FTag_AbilityOwner_TagsUpdated;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        Tick(
            TimeType InDeltaT) -> void;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_AbilityOwner_Current& InAbilityOwnerComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_AbilityOwner_Teardown : public ck_exp::TProcessor<
            FProcessor_AbilityOwner_Teardown,
            FCk_Handle_AbilityOwner,
            FFragment_AbilityOwner_Current,
            CK_IF_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_AbilityOwner_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
