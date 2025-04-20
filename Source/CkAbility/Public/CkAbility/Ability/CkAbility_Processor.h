#pragma once

#include "CkAbility/Ability/CkAbility_Fragment.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_Ability_AddReplicated : public ck_exp::TProcessor<
            FProcessor_Ability_AddReplicated,
            FCk_Handle_Ability,
            FCk_EntityReplicationDriver_AbilityData,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FCk_EntityReplicationDriver_AbilityData& InReplicatedAbility) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_Ability_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Ability_HandleRequests,
            FCk_Handle_Ability,
            FFragment_Ability_Requests,
            TExclude<FTag_AbilityOwner_PendingSubAbilityOperation>,
            TExclude<FFragment_AbilityOwner_Requests>,
            TExclude<FFragment_AbilityOwner_Events>,
            TExclude<FTag_AbilityOwner_TagsUpdated>,
            TExclude<FTag_AbilityOwner_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Ability_Requests;

    public:
        using TProcessor::TProcessor;

    private:
        enum class EAbilityProcessor_ForEachRequestResult : uint8
        {
            Continue,
            Stop
        };

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Ability_Requests& InAbilityRequests) const -> void;

    public:
        static auto
        DoHandleRequest(
            HandleType& InAbilityEntity,
            const FFragment_Ability_RequestAddAndGive& InRequest)
            -> EAbilityProcessor_ForEachRequestResult;

        static auto
        DoHandleRequest(
            HandleType& InAbilityEntity,
            const FFragment_Ability_RequestTransferExisting_Initiate& InRequest)
            -> EAbilityProcessor_ForEachRequestResult;

        static auto
        DoHandleRequest(
            HandleType& InAbilityEntity,
            const FFragment_Ability_RequestTransferExisting_SwapOwner& InRequest)
            -> EAbilityProcessor_ForEachRequestResult;

        static auto
        DoHandleRequest(
            HandleType& InAbilityEntity,
            const FFragment_Ability_RequestTransferExisting_Finalize& InRequest)
            -> EAbilityProcessor_ForEachRequestResult;

        static auto
        DoHandleRequest(
            HandleType& InAbilityEntity,
            const FFragment_Ability_RequestGive& InRequest)
            -> EAbilityProcessor_ForEachRequestResult;

        static auto
        DoHandleRequest(
            HandleType& InAbilityEntity,
            const FFragment_Ability_RequestRevoke& InRequest)
            -> EAbilityProcessor_ForEachRequestResult;

        static auto
        DoHandleRequest(
            HandleType& InAbilityEntity,
            const FFragment_Ability_RequestActivate& InRequest)
            -> EAbilityProcessor_ForEachRequestResult;

        static auto
        DoHandleRequest(
            HandleType& InAbilityEntity,
            const FFragment_Ability_RequestDeactivate& InRequest)
            -> EAbilityProcessor_ForEachRequestResult;
    };


    // --------------------------------------------------------------------------------------------------------------------

    class CKABILITY_API FProcessor_Ability_Teardown : public ck_exp::TProcessor<
            FProcessor_Ability_Teardown,
            FCk_Handle_Ability,
            FFragment_Ability_Current,
            FTag_Ability_Given,
            CK_IF_INITIATE_CONFIRM_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Ability_Current& InCurrent) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
