#pragma once

#include "CkAbility/Ability/CkAbility_Fragment.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_AbilityOwner_EnsureAllAppended : public ck_exp::TProcessor<
            FProcessor_AbilityOwner_EnsureAllAppended,
            FCk_Handle,
            FCk_Fragment_AbilityOwner_ParamsData,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_AbilityOwner_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FCk_Fragment_AbilityOwner_ParamsData& InExtraParams) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

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
            const FFragment_AbilityOwner_Params& InParams) const -> void;
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
        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_AbilityOwner_Events& InAbilityOwnerEvents) -> void;
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
            FFragment_AbilityOwner_Current& InCurrent,
            FFragment_AbilityOwner_Requests& InRequests) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
            const FCk_Request_AbilityOwner_AddAndGiveExistingAbility& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
            const FCk_Request_AbilityOwner_TransferExistingAbility& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_GiveAbility& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
            const FCk_Request_AbilityOwner_GiveReplicatedAbility& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
            const FCk_Request_AbilityOwner_RevokeAbility& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
            const FCk_Request_AbilityOwner_ActivateAbility& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
            const FCk_Request_AbilityOwner_DeactivateAbility& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
            const FCk_Request_AbilityOwner_CancelSubAbilities& InRequest) const -> void;

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
            FFragment_AbilityOwner_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_AbilityOwner_ResolvePendingOperationTags : public ck_exp::TProcessor<
            FProcessor_AbilityOwner_ResolvePendingOperationTags,
            FCk_Handle_AbilityOwner,
            FTag_AbilityOwner_RemovePendingSubAbilityOperation,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Super = TProcessor;
        using MarkedDirtyBy = FTag_AbilityOwner_RemovePendingSubAbilityOperation;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FTag_AbilityOwner_RemovePendingSubAbilityOperation& InCountedTag) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_AbilityOwner_ResolvePendingOperationTags_DEBUG : public ck_exp::TProcessor<
            FProcessor_AbilityOwner_ResolvePendingOperationTags_DEBUG,
            FCk_Handle_AbilityOwner,
            FTag_AbilityOwner_PendingSubAbilityOperation,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Super = TProcessor;
        using MarkedDirtyBy = FTag_AbilityOwner_PendingSubAbilityOperation;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FTag_AbilityOwner_PendingSubAbilityOperation& InCountedTag) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_AbilityOwner_DeferClientRequestUntilReady : public ck_exp::TProcessor<
            FProcessor_AbilityOwner_DeferClientRequestUntilReady,
            FCk_Handle_AbilityOwner,
            FFragment_AbilityOwner_Current,
            FFragment_AbilityOwner_DeferredClientRequests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Super = TProcessor;
        using MarkedDirtyBy = FFragment_AbilityOwner_DeferredClientRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_AbilityOwner_Current& InCurrent,
            FFragment_AbilityOwner_DeferredClientRequests& InRequests) const -> void;

    private:
        auto
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
            const FCk_Request_AbilityOwner_TransferExistingAbility& InRequest) const -> void;

        auto
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
            const FCk_Request_AbilityOwner_RevokeAbility& InRequest) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_AbilityOwner_Teardown : public ck_exp::TProcessor<
            FProcessor_AbilityOwner_Teardown,
            FCk_Handle_AbilityOwner,
            FFragment_AbilityOwner_Current,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_AbilityOwner_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
