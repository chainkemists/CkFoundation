#include "CkAbilityOwner_Processor.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEntityBridge/CkEntityBridge_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_AbilityOwner_Setup::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::Tick(InDeltaT);

        _Registry.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_AbilityOwner_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
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

        // If we, as an AbilityOwner, are also an Ability, then we need to Give the contained Ability to ourselves so that
        // we have it as one of the granted abilities. This should NEVER be exposed as a concept, it's only done here to
        // simplify the following processors. The reason is that if we expose this as a concept then an ability can live
        // on 2 'levels', i.e. An entity that is an Ability will have companion abilities (same level) AND will live in
        // the list of sub-abilities (this is bad)
        if (UCk_Utils_Ability_UE::Has(InHandle))
        {
            auto AbilityHandle = UCk_Utils_Ability_UE::Conv_HandleToAbility(InHandle);
            UCk_Utils_Ability_UE::RecordOfAbilities_Utils::Request_Connect(InHandle, AbilityHandle);
            const auto Script = InHandle.Get<ck::FFragment_Ability_Current>().Get_AbilityScript();

            CK_ENSURE_IF_NOT(ck::IsValid(Script),
                TEXT("AbilityScript for Handle [{}] with AbilityOwner [{}] is INVALID. Unable to GIVE the Ability properly"),
                InHandle,
                InHandle)
            { return; }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_HandleEvents::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
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
            HandleType& InHandle,
            FFragment_AbilityOwner_Current&  InAbilityOwnerComp,
            FFragment_AbilityOwner_Requests& InAbilityRequestsComp) const
        -> void
    {
        const auto RequestsCopy = InAbilityRequestsComp._Requests;
        InAbilityRequestsComp._Requests.Reset();

        algo::ForEach(RequestsCopy, ck::Visitor([&](const auto& InRequestVariant)
        {
            DoHandleRequest(InHandle, InAbilityOwnerComp, InRequestVariant);

            // TODO: Add formatter for each request and track which one was responsible for destroying entity
        }));

        if (ck::Is_NOT_Valid(InHandle))
        {
            ability::Verbose(TEXT("Ability Entity [{}] was Marked Pending Kill during the handling of its requests."), InHandle);
            return;
        }

        if (InAbilityRequestsComp.Get_Requests().IsEmpty())
        {
            InHandle.Remove<MarkedDirtyBy>();
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_GiveAbility& InRequest) const
        -> void
    {
        const auto& AbilityEntityConfig = InRequest.Get_AbilityEntityConfig();
        CK_ENSURE_IF_NOT(ck::IsValid(AbilityEntityConfig),
            TEXT("Cannot GIVE Ability to Ability Owner [{}] because it has an INVALID Entity Config PDA"),
            InAbilityOwnerEntity)
        { return; }

        const auto& AbilityConstructionScript = Cast<UCk_Ability_ConstructionScript_PDA>(AbilityEntityConfig->Get_EntityConstructionScript());
        CK_ENSURE_IF_NOT(ck::IsValid(AbilityConstructionScript),
            TEXT("Cannot GIVE Ability to Ability Owner [{}] because it has an INVALID Construction Script"),
            InAbilityOwnerEntity)
        { return; }

        const auto& AbilityParams = AbilityConstructionScript->Get_AbilityParams();
        const auto& AbilityScriptClass = AbilityParams.Get_AbilityScriptClass();

        CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptClass),
            TEXT("Cannot GIVE Ability to Ability Owner [{}] using Construction Script [{}] because the ScriptClass [{}] is INVALID"),
            InAbilityOwnerEntity,
            AbilityConstructionScript,
            AbilityScriptClass)
        { return; }

        const auto& AbilityData = AbilityParams.Get_Data();
        const auto& ReplicationType = AbilityData.Get_NetworkSettings().Get_ReplicationType();

        if (NOT UCk_Utils_Net_UE::Get_IsEntityRoleMatching(InAbilityOwnerEntity, ReplicationType))
        {
            ability::Verbose
            (
                TEXT("Skipping Giving Ability [{}] with Script [{}] because ReplicationType [{}] does not match for Entity [{}]"),
                AbilityEntityConfig,
                AbilityScriptClass,
                ReplicationType,
                InAbilityOwnerEntity
            );

            return;
        }

        const auto& AlreadyHasAbilityWithName = UCk_Utils_AbilityOwner_UE::Has_Ability(InAbilityOwnerEntity, AbilityScriptClass);

        CK_ENSURE_IF_NOT(NOT AlreadyHasAbilityWithName, TEXT("Cannot GIVE Ability [{}] to Ability Owner [{}] because it already has it"),
            AbilityScriptClass, InAbilityOwnerEntity)
        { return; }

        const auto PostAbilityCreationFunc =
        [InAbilityOwnerEntity, AbilityScriptClass, &AbilityParams](FCk_Handle& InAbilityEntity) -> void
        {
            ability::VeryVerbose
            (
                TEXT("Giving Ability [Class: {} | Entity: {}] to Ability Owner [{}]"),
                AbilityScriptClass,
                InAbilityEntity,
                InAbilityOwnerEntity
            );

            UCk_Utils_Handle_UE::Set_DebugName(InAbilityEntity,
                UCk_Utils_Debug_UE::Get_DebugName(AbilityParams.Get_AbilityScriptClass(), ECk_DebugNameVerbosity_Policy::Compact));

            auto AbilityHandle = UCk_Utils_Ability_UE::Conv_HandleToAbility(InAbilityEntity);
            UCk_Utils_Ability_UE::DoGive(InAbilityOwnerEntity, AbilityHandle);

            if (const auto& ActivationPolicy = UCk_Utils_Ability_UE::Get_ActivationSettings(AbilityHandle).Get_ActivationPolicy();
                ActivationPolicy == ECk_Ability_Activation_Policy::ActivateOnGranted)
            {
                // TODO: Activation Context Entity for SelfActivating Abilities is the Owner of the Ability
                UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(InAbilityOwnerEntity,
                    FCk_Request_AbilityOwner_ActivateAbility{InAbilityEntity, FCk_Ability_ActivationPayload{}.Set_ContextEntity(InAbilityOwnerEntity)});
            }
        };

        UCk_Utils_EntityBridge_UE::Request_Spawn(InAbilityOwnerEntity, FCk_Request_EntityBridge_SpawnEntity
            {AbilityEntityConfig, [](auto){}, PostAbilityCreationFunc});
    }

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_RevokeAbility& InRequest) const
        -> void
    {
        const auto& DoRevokeAbility = [&](FCk_Handle_Ability& InAbilityEntity, const TSubclassOf<UCk_Ability_Script_PDA>& InAbilityClass) -> void
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
            case ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass:
            {
                auto FoundAbilityEntity = DoFindAbilityByClass(InAbilityOwnerEntity, InRequest.Get_AbilityClass());

                if (ck::Is_NOT_Valid(FoundAbilityEntity))
                { break; }

                DoRevokeAbility(FoundAbilityEntity, InRequest.Get_AbilityClass());

                break;
            }
            case ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle:
            {
                auto FoundAbilityEntity = DoFindAbilityByHandle(InAbilityOwnerEntity, InRequest.Get_AbilityHandle());

                if (ck::Is_NOT_Valid(FoundAbilityEntity))
                { break; }

                DoRevokeAbility(FoundAbilityEntity, InRequest.Get_AbilityClass());

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
        const auto& DoTryActivateAbility = [&](FCk_Handle_Ability& InAbilityToActivateEntity, const FGameplayTag& InAbilityToActivateName) -> void
        {
            const auto& AbilityActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(InAbilityToActivateEntity);
            const auto& GrantedTags = AbilityActivationSettings.Get_ActivationSettingsOnOwner().Get_GrantTagsOnAbilityOwner();

            switch (const auto& CanActivateAbility = UCk_Utils_Ability_UE::Get_CanActivate(InAbilityToActivateEntity))
            {
                case ECk_Ability_ActivationRequirementsResult::RequirementsMet:
                {
                    ability::Verbose
                    (
                        TEXT("ACTIVATING Ability [Name: {} | Entity: {}] on Ability Owner [{}] and Granting Tags [{}]"),
                        InAbilityToActivateName,
                        InAbilityToActivateEntity,
                        InAbilityOwnerEntity,
                        GrantedTags
                    );

                    InAbilityOwnerComp.AppendTags(GrantedTags);
                    UUtils_Signal_AbilityOwner_OnTagsUpdated::Broadcast(InAbilityOwnerEntity, MakePayload(InAbilityOwnerEntity, InAbilityOwnerComp.Get_ActiveTags()));

                    break;
                }
                case ECk_Ability_ActivationRequirementsResult::RequirementsMet_ButAlreadyActive:
                {
                    ability::Verbose
                    (
                        TEXT("Failed to ACTIVATE Ability [Name: {} | Entity: {}] on Ability Owner [{}]! "
                             "The Activation Requirements are met BUT the Ability is ALREADY ACTIVE"),
                        InAbilityToActivateName,
                        InAbilityToActivateEntity,
                        InAbilityOwnerEntity
                    );

                    return;
                }
                case ECk_Ability_ActivationRequirementsResult::RequirementsNotMet_OnOwner:
                {
                    ability::Verbose
                    (
                        TEXT("Failed to ACTIVATE Ability [Name: {} | Entity: {}] on Ability Owner [{}] "
                             "because the Activation Requirements on ABILITY OWNER are NOT met"),
                        InAbilityToActivateName,
                        InAbilityToActivateEntity,
                        InAbilityOwnerEntity
                    );

                    return;
                }
                case ECk_Ability_ActivationRequirementsResult::RequirementsNotMet_OnSelf:
                {
                    ability::Verbose
                    (
                        TEXT("Failed to ACTIVATE Ability [Name: {} | Entity: {}] on Ability Owner [{}] "
                             "because the Activation Requirements on ABILITY ITSELF are NOT met"),
                        InAbilityToActivateName,
                        InAbilityToActivateEntity,
                        InAbilityOwnerEntity
                    );

                    return;
                }
                case ECk_Ability_ActivationRequirementsResult::RequirementsNotMet_OnOwnerAndSelf:
                {
                    ability::Verbose
                    (
                        TEXT("Failed to ACTIVATE Ability [Name: {} | Entity: {}] on Ability Owner [{}] "
                             "because the Activation Requirements on ABILITY OWNER and ABILITY ITSELF are NOT met"),
                        InAbilityToActivateName,
                        InAbilityToActivateEntity,
                        InAbilityOwnerEntity
                    );

                    return;
                }
                default:
                {
                    CK_INVALID_ENUM(CanActivateAbility);
                    return;
                }
            }

            // Cancel All Abilities that are cancelled by the newly granted tags
            UCk_Utils_AbilityOwner_UE::ForEach_Ability_If
            (
                InAbilityOwnerEntity,
                ECk_AbilityOwner_ForEachAbility_Policy::IncludeSelfIfApplicable,
                [&](const FCk_Handle& InAbilityEntityToCancel)
                {
                    ability::Verbose
                    (
                        TEXT("CANCELLING Ability [Name: {} | Entity: {}] after Activating Ability [Name: {} | Entity: {}] on Ability Owner [{}]"),
                        UCk_Utils_GameplayLabel_UE::Get_Label(InAbilityEntityToCancel),
                        InAbilityEntityToCancel,
                        InAbilityToActivateName,
                        InAbilityToActivateEntity,
                        InAbilityOwnerEntity
                    );

                    UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(InAbilityOwnerEntity,
                        FCk_Request_AbilityOwner_DeactivateAbility{InAbilityEntityToCancel});
                },
                algo::MatchesAnyAbilityActivationCancelledTags{GrantedTags}
            );


            auto& RequestsComp = InAbilityOwnerEntity.Get<FFragment_AbilityOwner_Requests>();
            const auto NumNewRequests = RequestsComp.Get_Requests().Num();
            UCk_Utils_Ability_UE::DoActivate(InAbilityToActivateEntity, InRequest.Get_ActivationPayload());

            // it's possible that we already have a deactivation request, if yes, process it
            const auto ProcessPossibleDeactivationRequest = [&]
            {
                const auto NewRequestsAfterActivate = RequestsComp.Get_Requests().Num() - NumNewRequests;

                if (NOT NewRequestsAfterActivate)
                { return; }

                CK_ENSURE_IF_NOT(NewRequestsAfterActivate == 1,
                    TEXT("This code expects that the Requests Array size, IMMEDIATELY AFTER the Ability [{}] with Owner [{}] is Activated AND "
                        "WHILE we are potentially processing other requests, is ONE (where ONE means the Ability we just Activated, Deactivated), "
                        "in the same frame. If this Ensure is firing, this means this assumption is incorrect and this code needs to be rethought."),
                        InAbilityToActivateEntity, InAbilityOwnerEntity)
                { return; }

                auto LastRequest = RequestsComp.Get_Requests().Last();

                const auto PendingRequest = std::get_if<FCk_Request_AbilityOwner_DeactivateAbility>(&LastRequest);

                CK_ENSURE_IF_NOT(IsValid(PendingRequest, IsValid_Policy_NullptrOnly{}),
                    TEXT("Expected the PendingRequest IMMEDIATELY AFTER Activating Ability [{}] with Owner [{}] to be of type DeactivateAbility. "
                    "Because this not the case, this code needs to be rethought."), InAbilityToActivateEntity, InAbilityOwnerEntity)
                { return; }

                CK_ENSURE_IF_NOT(PendingRequest->Get_AbilityHandle() == InAbilityToActivateEntity,
                    TEXT("Expected the PendingRequest IMMEDIATELY AFTER Activating Ability [{}] with Owner [{}] to be for the SAME Ability. "
                    "Instead the Deactivation Request is for the Ability [{}]."), InAbilityToActivateEntity, InAbilityOwnerEntity,
                    PendingRequest->Get_AbilityHandle())
                { return; }

                ability::Verbose(TEXT("Deactivating Ability [{}] on Ability Owner [{}] IMMEDIATELY"),
                    InAbilityToActivateEntity, InAbilityOwnerEntity);

                RequestsComp._Requests.Reset();
                DoHandleRequest(InAbilityOwnerEntity, InAbilityOwnerComp, *PendingRequest);
            }; ProcessPossibleDeactivationRequest();
        };

        switch (const auto& SearchPolicy = InRequest.Get_SearchPolicy())
        {
            case ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass:
            {
                auto FoundAbilityEntity = DoFindAbilityByClass(InAbilityOwnerEntity, InRequest.Get_AbilityClass());

                if (ck::Is_NOT_Valid(FoundAbilityEntity))
                { break; }

                DoTryActivateAbility(FoundAbilityEntity, UCk_Utils_GameplayLabel_UE::Get_Label(FoundAbilityEntity));

                break;
            }
            case ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle:
            {
                auto FoundAbilityEntity = DoFindAbilityByHandle(InAbilityOwnerEntity, InRequest.Get_AbilityHandle());

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
            const FCk_Request_AbilityOwner_DeactivateAbility& InRequest) const
        -> void
    {
        const auto& DoDeactivateAbility = [&](FCk_Handle_Ability& InAbilityEntity, const TSubclassOf<UCk_Ability_Script_PDA>& InAbilityClass) -> void
        {
            if (UCk_Utils_Ability_UE::Get_Status(InAbilityEntity) == ECk_Ability_Status::NotActive)
            { return; }

            const auto& AbilityActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(InAbilityEntity);
            const auto& GrantedTags = AbilityActivationSettings.Get_ActivationSettingsOnOwner().Get_GrantTagsOnAbilityOwner();

            InAbilityOwnerComp.RemoveTags(GrantedTags);
            UUtils_Signal_AbilityOwner_OnTagsUpdated::Broadcast(InAbilityOwnerEntity, MakePayload(InAbilityOwnerEntity, InAbilityOwnerComp.Get_ActiveTags()));

            ability::VeryVerbose
            (
                TEXT("DEACTIVATING Ability [Name: {} | Entity: {}] from Ability Owner [{}] and Removing Tags [{}]"),
                InAbilityClass,
                InAbilityEntity,
                InAbilityOwnerEntity,
                GrantedTags
            );

            UCk_Utils_Ability_UE::DoDeactivate(InAbilityOwnerEntity, InAbilityEntity);
        };

        switch (const auto& SearchPolicy = InRequest.Get_SearchPolicy())
        {
            case ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass:
            {
                auto FoundAbilityEntity = DoFindAbilityByClass(InAbilityOwnerEntity, InRequest.Get_AbilityClass());

                if (ck::Is_NOT_Valid(FoundAbilityEntity))
                { break; }

                DoDeactivateAbility(FoundAbilityEntity, InRequest.Get_AbilityClass());

                break;
            }
            case ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle:
            {
                auto FoundAbilityEntity = DoFindAbilityByHandle(InAbilityOwnerEntity, InRequest.Get_AbilityHandle());

                if (ck::Is_NOT_Valid(FoundAbilityEntity))
                { break; }

                DoDeactivateAbility(FoundAbilityEntity, UCk_Utils_Ability_UE::Get_ScriptClass(FoundAbilityEntity));

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
        DoFindAbilityByClass(
            FCk_Handle& InAbilityOwnerEntity,
            const TSubclassOf<UCk_Ability_Script_PDA>& InAbilityClass)
        -> FCk_Handle_Ability
    {
        const auto& FoundAbilityWithName = UCk_Utils_Ability_UE::RecordOfAbilities_Utils::Get_ValidEntry_If
        (
            InAbilityOwnerEntity,
            [InAbilityClass](const FCk_Handle& InHandle)
            // TODO: remove conv when RecordOfAbilities is fully templated
            { return UCk_Utils_Ability_UE::Get_ScriptClass(UCk_Utils_Ability_UE::Conv_HandleToAbility(InHandle)) == InAbilityClass; }
        );

        CK_ENSURE_IF_NOT(ck::IsValid(FoundAbilityWithName),
            TEXT("Ability Owner [{}] does NOT have Ability [{}]"),
            InAbilityOwnerEntity,
            InAbilityClass)
        { return {}; }

        // TODO: remove conv when RecordOfAbilities is fully templated
        return UCk_Utils_Ability_UE::Conv_HandleToAbility(FoundAbilityWithName);
    }

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoFindAbilityByHandle(
            const FCk_Handle& InAbilityOwnerEntity,
            const FCk_Handle& InAbilityEntity)
        -> FCk_Handle_Ability
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Ability_UE::Has(InAbilityEntity),
            TEXT("Entity [{}] is NOT an Ability"),
            InAbilityEntity)
        { return {}; }

        const auto& HasAbilityWithEntity = UCk_Utils_Ability_UE::RecordOfAbilities_Utils::Get_HasValidEntry_If
        (
            InAbilityOwnerEntity,
            algo::MatchesEntityHandle{InAbilityEntity}
        );

        CK_ENSURE_IF_NOT(HasAbilityWithEntity,
            TEXT("Ability Owner [{}] does NOT have Ability [{}]"),
            InAbilityOwnerEntity,
            InAbilityEntity)
        { return {}; }

        // TODO: remove conv when RecordOfAbilities is fully templated
        return UCk_Utils_Ability_UE::Conv_HandleToAbility(InAbilityEntity);
    }
}

// --------------------------------------------------------------------------------------------------------------------
