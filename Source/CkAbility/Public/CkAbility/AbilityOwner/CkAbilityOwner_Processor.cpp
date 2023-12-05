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
            TimeType                             InDeltaT,
            HandleType                           InHandle,
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
        FProcessor_AbilityOwner_HandleRequests::
        ForEachEntity(
            TimeType                         InDeltaT,
            HandleType                       InHandle,
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

        const auto& AbilityName = AbilityParams.Get_Data().Get_AbilityName();

        const auto& AlreadyHasAbilityWithName = UCk_Utils_Ability_UE::Has(InAbilityOwnerEntity, AbilityName);

        CK_ENSURE_IF_NOT(NOT AlreadyHasAbilityWithName, TEXT("Cannot give Ability [{}] to Ability Owner [{}] because it already has it"), AbilityName, InAbilityOwnerEntity)
        { return; }

        // TODO: Validate that deferring the creation of the ability does not cause a problem. If it does, we will have to create it here

        const auto PostAbilityCreationFunc = [InAbilityOwnerEntity, AbilityName](const FCk_Handle& InAbilityEntity) -> void
        {
            ability::VeryVerbose
            (
                TEXT("Granting Ability [Name: {} | Entity: {}] to Ability Owner [{}]"),
                AbilityName,
                InAbilityEntity,
                InAbilityOwnerEntity
            );

            UCk_Utils_Ability_UE::RecordOfAbilities_Utils::Request_Connect(InAbilityOwnerEntity, InAbilityEntity);

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

            UCk_Utils_Ability_UE::RecordOfAbilities_Utils::Request_Disconnect(InAbilityEntity, InAbilityOwnerEntity);

            UCk_Utils_AbilityOwner_UE::Request_EndAbility(InAbilityOwnerEntity, FCk_Request_AbilityOwner_EndAbility{InAbilityEntity});

            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InAbilityEntity);
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

        const auto& DoTryActivateAbility = [&](const FCk_Handle& InAbilityEntity, const FGameplayTag& InAbilityName) -> void
        {
            if (UCk_Utils_Ability_UE::Get_Status(InAbilityEntity, InAbilityName) == ECk_Ability_Status::Active)
            { return; }

            const auto& AbilityActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(InAbilityEntity, InAbilityName);

            if (NOT DoGet_AreActivationRequirementsMet(AbilityActivationSettings))
            {
                ability::VeryVerbose
                (
                    TEXT("Failed to Activate Ability Ability [Name: {} | Entity: {}] on Ability Owner [{}] "
                        "because the Activation Requirements are NOT met"),
                    InAbilityName,
                    InAbilityEntity,
                    InAbilityOwnerEntity
                );

                return;
            }

            // const auto& abilityCostSettings = UTT_Utils_Ability_UE::Get_CostSettings(InAbilityEntity);

            // // Check and Apply Ability Cost
            // if (abilityCostSettings.Get_HasCost())
            // {
            //     const auto& abilityCost = abilityCostSettings.Get_AbilityCost();
            //     CK_ENSURE_IF_NOT(ck::IsValid(abilityCost),
            //         TEXT("Ability Ability [Name: {} | Entity: {}] on Ability Owner [{}] has an INVALID Cost"), AbilityName, InAbilityEntity, InAbilityOwnerEntity)
            //     { return; }

            //     if (NOT abilityCost->CheckCost(InAbilityOwnerEntity, InAbilityEntity))
            //     {
            //         ability::VeryVerbose
            //         (
            //             TEXT("Failed to Activate Ability Ability [Name: {} | Entity: {}] on Ability Owner [{}] because the Cost cannot be afforded"),
            //             AbilityName,
            //             InAbilityEntity,
            //             InAbilityOwnerEntity
            //         );

            //         return;
            //     }

            //     abilityCost->ApplyCost(InAbilityOwnerEntity, InAbilityEntity);
            // }

            const auto& GrantedTags = AbilityActivationSettings.Get_ActivationOwnerGrantedTags();

            InAbilityOwnerComp._ActiveTags.AppendTags(GrantedTags);

            // Cancel All Abilities that are cancelled by the newly granted tags
            UCk_Utils_Ability_UE::RecordOfAbilities_Utils::ForEachEntryIf
            (
                InAbilityOwnerEntity,
                [InAbilityOwnerEntity](const FCk_Handle& InAbilityEntity)
                {
                    UCk_Utils_AbilityOwner_UE::Request_EndAbility(InAbilityOwnerEntity,
                        FCk_Request_AbilityOwner_EndAbility{InAbilityEntity});
                },
                algo::MatchesAnyAbilityActivationCancelledTags{GrantedTags}
            );

            ability::VeryVerbose
            (
                TEXT("Activating Ability [Name: {} | Entity: {}] on Ability Owner [{}]"),
                InAbilityName,
                InAbilityEntity,
                InAbilityOwnerEntity
            );

            UCk_Utils_Ability_UE::Activate(InAbilityEntity);

            // TODO: Call Activate/End instead (or in addition to) firing a Signal
            // UTT_UtilsSignal_Ability_OnActivated::Invoke(InAbilityEntity, MakePayload(InAbilityEntity, InAbilityOwnerEntity));
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

            InAbilityOwnerComp._ActiveTags.RemoveTags(GrantedTags);

            ability::VeryVerbose
            (
                TEXT("Ending Ability [Name: {} | Entity: {}] from Ability Owner [{}]"),
                InAbilityName,
                InAbilityEntity,
                InAbilityOwnerEntity
            );

            UCk_Utils_Ability_UE::End(InAbilityEntity);
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
