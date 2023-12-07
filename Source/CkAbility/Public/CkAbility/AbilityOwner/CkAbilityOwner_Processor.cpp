#include "CkAbilityOwner_Processor.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

#include "CkUnreal/EntityBridge/CkEntityBridge_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_AbilityOwner_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AbilityOwner_Params& InAbilityOwnerParams) const
        -> void
    {
        UCk_Utils_Ability_UE::RecordOfAbilities_Utils::AddIfMissing(InHandle);

        const auto& Params = InAbilityOwnerParams.Get_Params();

        for (const auto& DefaultAbility : Params.Get_DefaultAbilities())
        {
            CK_ENSURE_IF_NOT(ck::IsValid(DefaultAbility), TEXT("Entity [{}] has an INVALID default Ability in its Params!"), InHandle)
            { continue; }

            UCk_Utils_AbilityOwner_UE::Request_GiveAbility(InHandle, FCk_Request_AbilityOwner_GiveAbility{DefaultAbility});
        }

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_HandleEvents::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AbilityOwner_Events&  InAbilityOwnerEvents) const
        -> void
    {
        UUtils_Signal_AbilityOwner_Events::Broadcast(InHandle, MakePayload(InHandle, InAbilityOwnerEvents.Get_Events()));
        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AbilityOwner_Current&  InAbilityOwnerComp,
            FFragment_AbilityOwner_Requests& InAbilityRequestsComp) const
        -> void
    {
        const auto RequestsCopy = InAbilityRequestsComp._Requests;
        InAbilityRequestsComp._Requests.Reset();

        algo::ForEach(RequestsCopy, ck::Visitor([&](const auto& InRequestVariant)
        {
            DoHandleRequest(InHandle, InAbilityOwnerComp, InRequestVariant);
        }));

        if (InAbilityRequestsComp.Get_Requests().IsEmpty())
        {
            InHandle.Remove<MarkedDirtyBy>();
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_GiveAbility& InRequest) const
        -> void
    {
        const auto& AbilityEntityConfig = InRequest.Get_AbilityEntityConfig();
        CK_ENSURE_IF_NOT(ck::IsValid(AbilityEntityConfig),
            TEXT("Cannot give Ability to Ability Owner [{}] because it has an INVALID Entity Config PDA"),
            InAbilityOwnerEntity)
        { return; }

        const auto& AbilityConstructionScript = Cast<UCk_Ability_ConstructionScript_PDA>(AbilityEntityConfig->Get_EntityConstructionScript());
        CK_ENSURE_IF_NOT(ck::IsValid(AbilityConstructionScript),
            TEXT("Cannot give Ability to Ability Owner [{}] because it has an INVALID Construction Script"),
            InAbilityOwnerEntity)
        { return; }

        const auto& AbilityParams = AbilityConstructionScript->Get_AbilityParams();
        CK_ENSURE_IF_NOT(ck::IsValid(AbilityParams),
            TEXT("Cannot give Ability to Ability Owner [{}] because it has INVALID Params"),
            InAbilityOwnerEntity)
        { return; }

        const auto ReplicationType = AbilityParams.Get_Data().Get_NetworkSettings().Get_ReplicationType();

        if (NOT UCk_Utils_Net_UE::Get_IsEntityRoleMatching(InAbilityOwnerEntity, ReplicationType))
        {
            ability::Verbose
            (
                TEXT("Skipping Giving Ability [{}] because ReplicationType [{}] does not match for Entity [{}]"),
                AbilityEntityConfig,
                ReplicationType,
                InAbilityOwnerEntity
            );

            return;
        }

        const auto& AbilityName = AbilityParams.Get_Data().Get_AbilityName();

        const auto& AlreadyHasAbilityWithName = UCk_Utils_Ability_UE::Has(InAbilityOwnerEntity, AbilityName);

        CK_ENSURE_IF_NOT(NOT AlreadyHasAbilityWithName, TEXT("Cannot give Ability [{}] to Ability Owner [{}] because it already has it"), AbilityName, InAbilityOwnerEntity)
        { return; }

        const auto PostAbilityCreationFunc = [InAbilityOwnerEntity, AbilityName](const FCk_Handle& InAbilityEntity) -> void
        {
            ability::VeryVerbose
            (
                TEXT("Giving Ability [Name: {} | Entity: {}] to Ability Owner [{}]"),
                AbilityName,
                InAbilityEntity,
                InAbilityOwnerEntity
            );

            UCk_Utils_Ability_UE::DoGive(InAbilityOwnerEntity, InAbilityEntity);

            if (const auto& ActivationPolicy = UCk_Utils_Ability_UE::Get_ActivationSettings(InAbilityEntity, AbilityName).Get_ActivationPolicy();
                ActivationPolicy == ECk_Ability_ActivationPolicy::ActivateOnGranted)
            {
                // TODO: Activation Context Entity for SelfActivating Abilities is the Owner of the Ability
                UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(InAbilityOwnerEntity,
                    FCk_Request_AbilityOwner_ActivateAbility{InAbilityEntity, InAbilityOwnerEntity});
            }
        };

        UCk_Utils_EntityBridge_UE::Request_Spawn(InAbilityOwnerEntity, FCk_Request_EntityBridge_SpawnEntity
            {AbilityEntityConfig, [](auto){}, PostAbilityCreationFunc});
    }

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_RevokeAbility& InRequest) const
        -> void
    {
        const auto& DoRevokeAbility = [&](const FCk_Handle& InAbilityEntity, const FGameplayTag& InAbilityName) -> void
        {
            ability::VeryVerbose
            (
                TEXT("Revoking Ability [Name: {} | Entity: {}] on Ability Owner [{}]"),
                InAbilityEntity,
                InAbilityEntity,
                InAbilityOwnerEntity
            );

            UCk_Utils_Ability_UE::DoRevoke(InAbilityOwnerEntity, InAbilityEntity);
        };

        switch (const auto& SearchPolicy = InRequest.Get_SearchPolicy())
        {
            case ECk_AbilityOwner_AbilitySearchPolicy::SearchByName:
            {
                const auto& FoundAbilityEntity = DoFindAbilityByName(InAbilityOwnerEntity, InRequest.Get_AbilityName());

                if (ck::Is_NOT_Valid(FoundAbilityEntity))
                { break; }

                DoRevokeAbility(FoundAbilityEntity, InRequest.Get_AbilityName());

                break;
            }
            case ECk_AbilityOwner_AbilitySearchPolicy::SearchByHandle:
            {
                const auto& FoundAbilityEntity = DoFindAbilityByHandle(InAbilityOwnerEntity, InRequest.Get_AbilityHandle());

                if (ck::Is_NOT_Valid(FoundAbilityEntity))
                { break; }

                DoRevokeAbility(FoundAbilityEntity, InRequest.Get_AbilityName());

                break;
            }
            default:
            {
                CK_INVALID_ENUM(SearchPolicy);
                break;
            }
        }
    }

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_ActivateAbility& InRequest) const
        -> void
    {
        const auto& DoGet_AreActivationRequirementsMet = [&](const FCk_Ability_ActivationSettings& InAbilityActivationSettings)
        {
            const auto ActivationRequirements = InAbilityActivationSettings.Get_ActivationRequiredTags();
            const auto ActivationBlockers = InAbilityActivationSettings.Get_ActivationBlockedTags();

            const auto& ActivateTags = InAbilityOwnerComp.Get_ActiveTags();

            return ActivateTags.HasAllExact(ActivationRequirements) && NOT ActivateTags.HasAny(ActivationBlockers);
        };

        const auto& DoTryActivateAbility = [&](const FCk_Handle& InAbilityToActivateEntity, const FGameplayTag& InAbilityToActivateName) -> void
        {
            if (NOT UCk_Utils_Ability_UE::DoGet_CanActivate(InAbilityToActivateEntity))
            { return; }

            const auto& AbilityActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(InAbilityToActivateEntity, InAbilityToActivateName);

            if (NOT DoGet_AreActivationRequirementsMet(AbilityActivationSettings))
            {
                ability::VeryVerbose
                (
                    TEXT("Failed to Activate Ability Ability [Name: {} | Entity: {}] on Ability Owner [{}] "
                         "because the Activation Requirements are NOT met"),
                    InAbilityToActivateName,
                    InAbilityToActivateEntity,
                    InAbilityOwnerEntity
                );

                return;
            }

            const auto& GrantedTags = AbilityActivationSettings.Get_ActivationOwnerGrantedTags();
            InAbilityOwnerComp.AppendTags(GrantedTags);

            ability::VeryVerbose
            (
                TEXT("Activating Ability [Name: {} | Entity: {}] on Ability Owner [{}] and Granting Tags [{}]"),
                InAbilityToActivateName,
                InAbilityToActivateEntity,
                InAbilityOwnerEntity,
                GrantedTags
            );

            // Cancel All Abilities that are cancelled by the newly granted tags
            UCk_Utils_Ability_UE::RecordOfAbilities_Utils::ForEachEntryIf
            (
                InAbilityOwnerEntity,
                [&](const FCk_Handle& InAbilityEntityToCancel)
                {
                    ability::VeryVerbose
                    (
                        TEXT("Cancelling Ability [Name: {} | Entity: {}] after Activating Ability [Name: {} | Entity: {}] on Ability Owner [{}]"),
                        UCk_Utils_GameplayLabel_UE::Get_Label(InAbilityEntityToCancel),
                        InAbilityEntityToCancel,
                        InAbilityToActivateName,
                        InAbilityToActivateEntity,
                        InAbilityOwnerEntity
                    );

                    UCk_Utils_AbilityOwner_UE::Request_EndAbility(InAbilityOwnerEntity,
                        FCk_Request_AbilityOwner_EndAbility{InAbilityEntityToCancel});
                },
                algo::MatchesAnyAbilityActivationCancelledTags{GrantedTags}
            );

            UCk_Utils_Ability_UE::DoActivate(InAbilityToActivateEntity);
        };

        switch (const auto& SearchPolicy = InRequest.Get_SearchPolicy())
        {
            case ECk_AbilityOwner_AbilitySearchPolicy::SearchByName:
            {
                const auto& FoundAbilityEntity = DoFindAbilityByName(InAbilityOwnerEntity, InRequest.Get_AbilityName());

                if (ck::Is_NOT_Valid(FoundAbilityEntity))
                { break; }

                DoTryActivateAbility(FoundAbilityEntity, UCk_Utils_GameplayLabel_UE::Get_Label(FoundAbilityEntity));

                break;
            }
            case ECk_AbilityOwner_AbilitySearchPolicy::SearchByHandle:
            {
                const auto& FoundAbilityEntity = DoFindAbilityByHandle(InAbilityOwnerEntity, InRequest.Get_AbilityHandle());

                if (ck::Is_NOT_Valid(FoundAbilityEntity))
                { break; }

                DoTryActivateAbility(FoundAbilityEntity, UCk_Utils_GameplayLabel_UE::Get_Label(FoundAbilityEntity));

                break;
            }
            default:
            {
                CK_INVALID_ENUM(SearchPolicy);
                break;
            }
        }
    }

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_EndAbility& InRequest) const
        -> void
    {
        const auto& DoEndAbility = [&](const FCk_Handle& InAbilityEntity, const FGameplayTag& InAbilityName) -> void
        {
            if (UCk_Utils_Ability_UE::Get_Status(InAbilityEntity, InAbilityName) == ECk_Ability_Status::NotActive)
            { return; }

            const auto& AbilityActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(InAbilityEntity, InAbilityName);
            const auto& GrantedTags = AbilityActivationSettings.Get_ActivationOwnerGrantedTags();

            InAbilityOwnerComp.RemoveTags(GrantedTags);

            ability::VeryVerbose
            (
                TEXT("Ending Ability [Name: {} | Entity: {}] from Ability Owner [{}] and Remove Tags [{}]"),
                InAbilityName,
                InAbilityEntity,
                InAbilityOwnerEntity,
                GrantedTags
            );

            UCk_Utils_Ability_UE::DoEnd(InAbilityEntity);
        };

        switch (const auto& SearchPolicy = InRequest.Get_SearchPolicy())
        {
            case ECk_AbilityOwner_AbilitySearchPolicy::SearchByName:
            {
                const auto& FoundAbilityEntity = DoFindAbilityByName(InAbilityOwnerEntity, InRequest.Get_AbilityName());

                if (ck::Is_NOT_Valid(FoundAbilityEntity))
                { break; }

                DoEndAbility(FoundAbilityEntity, InRequest.Get_AbilityName());

                break;
            }
            case ECk_AbilityOwner_AbilitySearchPolicy::SearchByHandle:
            {
                const auto& FoundAbilityEntity = DoFindAbilityByHandle(InAbilityOwnerEntity, InRequest.Get_AbilityHandle());

                if (ck::Is_NOT_Valid(FoundAbilityEntity))
                { break; }

                DoEndAbility(FoundAbilityEntity, UCk_Utils_GameplayLabel_UE::Get_Label(FoundAbilityEntity));

                break;
            }
            default:
            {
                CK_INVALID_ENUM(SearchPolicy);
                break;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoFindAbilityByName(
            const FCk_Handle& InAbilityOwnerEntity,
            const FGameplayTag& InAbilityName) const
         -> FCk_Handle
    {
        const auto& FoundAbilityWithName = UCk_Utils_Ability_UE::RecordOfAbilities_Utils::Get_RecordEntryIf
        (
            InAbilityOwnerEntity,
            algo::MatchesGameplayLabel{InAbilityName}
        );

        CK_ENSURE_IF_NOT(ck::IsValid(FoundAbilityWithName),
            TEXT("Ability Owner [{}] does NOT have Ability [{}]"),
            InAbilityOwnerEntity,
            InAbilityName)
        { return {}; }

        return FoundAbilityWithName;
    }

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoFindAbilityByHandle(
            const FCk_Handle& InAbilityOwnerEntity,
            const FCk_Handle& InAbilityEntity) const
         -> FCk_Handle
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Ability_UE::Has(InAbilityEntity),
            TEXT("Entity [{}] is NOT a Ability"),
            InAbilityEntity,
            InAbilityOwnerEntity)
        { return {}; }

        const auto& HasAbilityWithEntity = UCk_Utils_Ability_UE::RecordOfAbilities_Utils::Get_HasRecordEntry
        (
            InAbilityOwnerEntity,
            algo::MatchesEntityHandle{InAbilityEntity}
        );

        CK_ENSURE_IF_NOT(HasAbilityWithEntity,
            TEXT("Ability Owner [{}] does NOT have Ability [{}]"),
            InAbilityOwnerEntity,
            InAbilityEntity)
        { return {}; }

        return InAbilityEntity;
    }
}

// --------------------------------------------------------------------------------------------------------------------
