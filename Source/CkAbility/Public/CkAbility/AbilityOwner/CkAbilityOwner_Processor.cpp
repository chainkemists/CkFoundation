#include "CkAbilityOwner_Processor.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Script.h"
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

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_AbilityOwner_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_AbilityOwner_Params& InAbilityOwnerParams) const
        -> void
    {
        for (const auto& Params = InAbilityOwnerParams.Get_Params(); const auto& DefaultAbility : Params.Get_DefaultAbilities())
        {
            CK_ENSURE_IF_NOT(ck::IsValid(DefaultAbility), TEXT("Entity [{}] has an INVALID default Ability in its Params!"), InHandle)
            { continue; }

            UCk_Utils_AbilityOwner_UE::Request_GiveAbility(InHandle, FCk_Request_AbilityOwner_GiveAbility{DefaultAbility, InHandle}, {});
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
        const auto AbilityGivenOrNot = [&]() -> ECk_AbilityOwner_AbilityGivenOrNot
        {
            const auto& AbilityScriptClass = InRequest.Get_AbilityScriptClass();
            CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptClass),
                TEXT("Cannot GIVE Ability to Ability Owner [{}] because it has an INVALID Ability Script Class"),
                InAbilityOwnerEntity)
            { return ECk_AbilityOwner_AbilityGivenOrNot::NotGiven; }

            const auto& AbilityEntityConfig = UCk_Utils_Ability_UE::DoCreate_AbilityEntityConfig(
                UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAbilityOwnerEntity), AbilityScriptClass);

            CK_ENSURE_IF_NOT(ck::IsValid(AbilityEntityConfig),
                TEXT("Cannot GIVE Ability to Ability Owner [{}] because the created Ability Entity Config for [{}] is INVALID"),
                InAbilityOwnerEntity,
                AbilityScriptClass)
            { return ECk_AbilityOwner_AbilityGivenOrNot::NotGiven; }

            const auto& AbilityConstructionScript = Cast<UCk_Ability_ConstructionScript_PDA>(AbilityEntityConfig->Get_EntityConstructionScript());
            CK_ENSURE_IF_NOT(ck::IsValid(AbilityConstructionScript),
                TEXT("Cannot GIVE Ability to Ability Owner [{}] because it has an INVALID Construction Script"),
                InAbilityOwnerEntity)
            { return ECk_AbilityOwner_AbilityGivenOrNot::NotGiven; }

            const auto& AbilityParams = AbilityConstructionScript->Get_AbilityParams();
            CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptClass),
                TEXT("Cannot GIVE Ability to Ability Owner [{}] using Construction Script [{}] because the ScriptClass [{}] is INVALID"),
                InAbilityOwnerEntity,
                AbilityConstructionScript,
                AbilityScriptClass)
            { return ECk_AbilityOwner_AbilityGivenOrNot::NotGiven; }

            const auto& AbilityData = AbilityParams.Get_Data();

            if (const auto& ReplicationType = AbilityData.Get_NetworkSettings().Get_ReplicationType();
                NOT UCk_Utils_Net_UE::Get_IsEntityRoleMatching(InAbilityOwnerEntity, ReplicationType))
            {
                ability::Verbose
                (
                    TEXT("Skipping Giving Ability [{}] with Script [{}] because ReplicationType [{}] does not match for Entity [{}]"),
                    AbilityEntityConfig,
                    AbilityScriptClass,
                    ReplicationType,
                    InAbilityOwnerEntity
                );

                return ECk_AbilityOwner_AbilityGivenOrNot::NotGiven;
            }

            const auto& AbilitySource = InRequest.Get_AbilitySource();

            if (NOT UCk_Utils_Ability_UE::DoGet_CanBeGiven(AbilityScriptClass, InAbilityOwnerEntity, AbilitySource))
            {
                ability::Verbose
                (
                    TEXT("Skipping Giving Ability [{}] with Script [{}] because CanBeGiven returned false"),
                    AbilityEntityConfig,
                    AbilityScriptClass,
                    InAbilityOwnerEntity
                );

                UCk_Utils_Ability_UE::DoOnNotGiven(AbilityScriptClass, InAbilityOwnerEntity, AbilitySource);
                return ECk_AbilityOwner_AbilityGivenOrNot::NotGiven;
            }

            const auto& OptionalPayload = InRequest.Get_OptionalPayload();

            const auto PostAbilityCreationFunc =
            [InAbilityOwnerEntity, AbilityScriptClass, AbilityParams, OptionalPayload, AbilitySource](FCk_Handle& InEntity) -> void
            {
                // TODO: Since the construction of the Ability entity is deferred, if multiple Give requests of the same
                // script class are processed in the same frame, it is possible for the CanBeGiven to NOT return the correct value
                // This check here is a temporary (and potentially expensive) workaround, but we should handle this case better
                if (NOT UCk_Utils_Ability_UE::DoGet_CanBeGiven(AbilityScriptClass, InAbilityOwnerEntity, AbilitySource))
                {
                    UCk_Utils_Ability_UE::DoOnNotGiven(AbilityScriptClass, InAbilityOwnerEntity, AbilitySource);
                    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InEntity);
                    return;
                }

                auto AbilityEntity = UCk_Utils_Ability_UE::CastChecked(InEntity);
                auto AbilityOwnerEntity = InAbilityOwnerEntity;

                ability::VeryVerbose
                (
                    TEXT("Giving Ability [Class: {} | Entity: {}] to Ability Owner [{}]"),
                    AbilityScriptClass,
                    AbilityEntity,
                    AbilityOwnerEntity
                );

                UCk_Utils_Handle_UE::Set_DebugName(AbilityEntity,
                    UCk_Utils_Debug_UE::Get_DebugName(AbilityParams.Get_AbilityScriptClass(), ECk_DebugNameVerbosity_Policy::Compact));

                {
                    const auto& AbilityOnGiveSettings = UCk_Utils_Ability_UE::Get_OnGiveSettings(AbilityEntity);
                    const auto& GrantedTags = AbilityOnGiveSettings.Get_OnGiveSettingsOnOwner().Get_GrantTagsOnAbilityOwner();

                    // HACK: need a non-const handle as we're unable to make the lambda mutable
                    auto NonConstAbilityOwnerEntity = InAbilityOwnerEntity;
                    auto& AbilityOwnerComp = NonConstAbilityOwnerEntity.Get<FFragment_AbilityOwner_Current>();
                    AbilityOwnerComp.AppendTags(InAbilityOwnerEntity, GrantedTags);

                    if (AbilityOwnerComp.Get_AreActiveTagsDifferentThanPreviousTags(InAbilityOwnerEntity))
                    {
                        UCk_Utils_AbilityOwner_UE::Request_TagsUpdated(NonConstAbilityOwnerEntity);
                    }
                }

                UCk_Utils_Ability_UE::DoGive(AbilityOwnerEntity, AbilityEntity, AbilitySource, OptionalPayload);

                UUtils_Signal_AbilityOwner_OnAbilityGivenOrNot::Broadcast(
                    AbilityOwnerEntity, MakePayload(AbilityOwnerEntity, AbilityEntity, ECk_AbilityOwner_AbilityGivenOrNot::Given));

                if (const auto& ActivationPolicy = UCk_Utils_Ability_UE::Get_ActivationSettings(AbilityEntity).Get_ActivationPolicy();
                    ActivationPolicy == ECk_Ability_Activation_Policy::ActivateOnGranted)
                {
                    UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(
                        AbilityOwnerEntity,
                        FCk_Request_AbilityOwner_ActivateAbility{AbilityEntity}
                        .Set_OptionalPayload(FCk_Ability_Payload_OnActivate{}.Set_ContextEntity(AbilityOwnerEntity)),
                        {});
                }
            };

            UCk_Utils_EntityBridge_UE::Request_Spawn(InAbilityOwnerEntity,
                FCk_Request_EntityBridge_SpawnEntity{AbilityEntityConfig}
                .Set_PostSpawnFunc(PostAbilityCreationFunc),
                {},
                {});

            return ECk_AbilityOwner_AbilityGivenOrNot::Given;
        }();

        if (AbilityGivenOrNot == ECk_AbilityOwner_AbilityGivenOrNot::NotGiven)
        {
            UUtils_Signal_AbilityOwner_OnAbilityGivenOrNot::Broadcast(
                InAbilityOwnerEntity, MakePayload(InAbilityOwnerEntity, FCk_Handle_Ability{}, ECk_AbilityOwner_AbilityGivenOrNot::NotGiven));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

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
            if (ck::Is_NOT_Valid(InAbilityEntity))
            {
                UUtils_Signal_AbilityOwner_OnAbilityRevokedOrNot::Broadcast(
                    InAbilityOwnerEntity, MakePayload(InAbilityOwnerEntity, FCk_Handle_Ability{}, ECk_AbilityOwner_AbilityRevokedOrNot::NotRevoked));

                return;
            }

            ability::VeryVerbose
            (
                TEXT("Revoking Ability [Name: {} | Entity: {}] on Ability Owner [{}]"),
                InAbilityEntity,
                InAbilityEntity,
                InAbilityOwnerEntity
            );

            {
                const auto& AbilityOnGiveSettings = UCk_Utils_Ability_UE::Get_OnGiveSettings(InAbilityEntity);
                const auto& GrantedTags = AbilityOnGiveSettings.Get_OnGiveSettingsOnOwner().Get_GrantTagsOnAbilityOwner();

                InAbilityOwnerComp.RemoveTags(InAbilityOwnerEntity, GrantedTags);

                if (InAbilityOwnerComp.Get_AreActiveTagsDifferentThanPreviousTags(InAbilityOwnerEntity))
                {
                    UCk_Utils_AbilityOwner_UE::Request_TagsUpdated(InAbilityOwnerEntity);
                }
            }

            UCk_Utils_Ability_UE::DoRevoke(InAbilityOwnerEntity, InAbilityEntity);

            UUtils_Signal_AbilityOwner_OnAbilityRevokedOrNot::Broadcast(
                InAbilityOwnerEntity, MakePayload(InAbilityOwnerEntity, InAbilityEntity, ECk_AbilityOwner_AbilityRevokedOrNot::Revoked));
        };

        switch (const auto& SearchPolicy = InRequest.Get_SearchPolicy())
        {
            case ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass:
            {
                auto FoundAbilityEntity = DoFindAbilityByClass(InAbilityOwnerEntity, InRequest.Get_AbilityClass());

                DoRevokeAbility(FoundAbilityEntity, InRequest.Get_AbilityClass());

                break;
            }
            case ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle:
            {
                auto FoundAbilityEntity = DoFindAbilityByHandle(InAbilityOwnerEntity, InRequest.Get_AbilityHandle());

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

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_ActivateAbility& InRequest) const
        -> void
    {
        const auto& DoTryActivateAbility = [&](FCk_Handle_Ability& InAbilityToActivateEntity) -> void
        {
            if (ck::Is_NOT_Valid(InAbilityToActivateEntity))
            {
                UUtils_Signal_AbilityOwner_OnAbilityActivatedOrNot::Broadcast(
                    InAbilityOwnerEntity, MakePayload(InAbilityOwnerEntity, InAbilityToActivateEntity, ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_AbilityNotFound));
                return;
            }

            const auto AbilityActivatedOrNot = [&]() -> ECk_AbilityOwner_AbilityActivatedOrNot
            {
                if (UCk_Utils_Ability_UE::Get_Status(InAbilityToActivateEntity) == ECk_Ability_Status::Active)
                { return ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_FailedChecks; }

                const auto& AbilityToActivateName = UCk_Utils_GameplayLabel_UE::Get_Label(InAbilityToActivateEntity);
                const auto& AbilityActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(InAbilityToActivateEntity);
                const auto& GrantedTags = AbilityActivationSettings.Get_ActivationSettingsOnOwner().Get_GrantTagsOnAbilityOwner();

                switch (const auto& CanActivateAbility = UCk_Utils_Ability_UE::Get_CanActivate(InAbilityToActivateEntity))
                {
                    case ECk_Ability_ActivationRequirementsResult::RequirementsMet:
                    {
                        ability::Verbose
                        (
                            TEXT("ACTIVATING Ability [Name: {} | Entity: {}] on Ability Owner [{}] and Granting Tags [{}]"),
                            AbilityToActivateName,
                            InAbilityToActivateEntity,
                            InAbilityOwnerEntity,
                            GrantedTags
                        );

                        InAbilityOwnerComp.AppendTags(InAbilityOwnerEntity, GrantedTags);

                        if (InAbilityOwnerComp.Get_AreActiveTagsDifferentThanPreviousTags(InAbilityOwnerEntity))
                        {
                            UCk_Utils_AbilityOwner_UE::Request_TagsUpdated(InAbilityOwnerEntity);
                        }

                        break;
                    }
                    case ECk_Ability_ActivationRequirementsResult::RequirementsMet_ButAlreadyActive:
                    {
                        ability::Verbose
                        (
                            TEXT("Failed to ACTIVATE Ability [Name: {} | Entity: {}] on Ability Owner [{}]! "
                                 "The Activation Requirements are met BUT the Ability is ALREADY ACTIVE"),
                            AbilityToActivateName,
                            InAbilityToActivateEntity,
                            InAbilityOwnerEntity
                        );

                        return ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_FailedChecks;
                    }
                    case ECk_Ability_ActivationRequirementsResult::RequirementsNotMet_OnOwner:
                    {
                        ability::Verbose
                        (
                            TEXT("Failed to ACTIVATE Ability [Name: {} | Entity: {}] on Ability Owner [{}] "
                                 "because the Activation Requirements on ABILITY OWNER are NOT met"),
                            AbilityToActivateName,
                            InAbilityToActivateEntity,
                            InAbilityOwnerEntity
                        );

                        return ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_FailedChecks;
                    }
                    case ECk_Ability_ActivationRequirementsResult::RequirementsNotMet_OnSelf:
                    {
                        ability::Verbose
                        (
                            TEXT("Failed to ACTIVATE Ability [Name: {} | Entity: {}] on Ability Owner [{}] "
                                 "because the Activation Requirements on ABILITY ITSELF are NOT met"),
                            AbilityToActivateName,
                            InAbilityToActivateEntity,
                            InAbilityOwnerEntity
                        );

                        return ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_FailedChecks;
                    }
                    case ECk_Ability_ActivationRequirementsResult::RequirementsNotMet_OnOwnerAndSelf:
                    {
                        ability::Verbose
                        (
                            TEXT("Failed to ACTIVATE Ability [Name: {} | Entity: {}] on Ability Owner [{}] "
                                 "because the Activation Requirements on ABILITY OWNER and ABILITY ITSELF are NOT met"),
                            AbilityToActivateName,
                            InAbilityToActivateEntity,
                            InAbilityOwnerEntity
                        );

                        return ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_FailedChecks;
                    }
                    default:
                    {
                        CK_INVALID_ENUM(CanActivateAbility);
                        return ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_FailedChecks;
                    }
                }

                // Try Deactivate our own Ability if we have one
                if (UCk_Utils_Ability_UE::Has(InAbilityOwnerEntity))
                {
                    if (const auto Condition = algo::MatchesAnyAbilityActivationCancelledTags{GrantedTags};
                        Condition(InAbilityOwnerEntity))
                    {
                        auto MyOwner = UCk_Utils_AbilityOwner_UE::CastChecked(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAbilityOwnerEntity));

                        const auto AbilityOwnerAsAbility = UCk_Utils_Ability_UE::CastChecked(InAbilityOwnerEntity);
                        UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(MyOwner,
                            FCk_Request_AbilityOwner_DeactivateAbility{AbilityOwnerAsAbility}, {});
                    }
                }

                // Cancel All Abilities that are cancelled by the newly granted tags
                // TODO: this is repeated multiple times in this file, move to a common function
                // TODO: See if the new system TagsChanged can help replace this section of code
                UCk_Utils_AbilityOwner_UE::ForEach_Ability_If
                (
                    InAbilityOwnerEntity,
                    [&](const FCk_Handle_Ability& InAbilityEntityToCancel)
                    {
                        ability::Verbose
                        (
                            TEXT("CANCELLING Ability [Name: {} | Entity: {}] after Activating Ability [Name: {} | Entity: {}] on Ability Owner [{}]"),
                            UCk_Utils_GameplayLabel_UE::Get_Label(InAbilityEntityToCancel),
                            InAbilityEntityToCancel,
                            AbilityToActivateName,
                            InAbilityToActivateEntity,
                            InAbilityOwnerEntity
                        );

                        UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(InAbilityOwnerEntity,
                            FCk_Request_AbilityOwner_DeactivateAbility{InAbilityEntityToCancel}, {});
                    },
                    algo::MatchesAnyAbilityActivationCancelledTags{GrantedTags}
                );

                UCk_Utils_Ability_UE::DoActivate(InAbilityOwnerEntity, InAbilityToActivateEntity, InRequest.Get_OptionalPayload());

                return ECk_AbilityOwner_AbilityActivatedOrNot::Activated;
            }();

            UUtils_Signal_AbilityOwner_OnAbilityActivatedOrNot::Broadcast(
                    InAbilityOwnerEntity, MakePayload(InAbilityOwnerEntity, InAbilityToActivateEntity, AbilityActivatedOrNot));

            auto& RequestsComp = InAbilityOwnerEntity.Get<FFragment_AbilityOwner_Requests>();
            const auto NumNewRequests = RequestsComp.Get_Requests().Num();

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

                ability::Verbose(TEXT("DEACTIVATING Ability [{}] on Ability Owner [{}] IMMEDIATELY"),
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

                DoTryActivateAbility(FoundAbilityEntity);

                break;
            }
            case ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle:
            {
                auto FoundAbilityEntity = DoFindAbilityByHandle(InAbilityOwnerEntity, InRequest.Get_AbilityHandle());

                DoTryActivateAbility(FoundAbilityEntity);

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
        DoHandleRequest(
            HandleType InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_DeactivateAbility& InRequest) const
        -> void
    {
        const auto& DoDeactivateAbility = [&](FCk_Handle_Ability& InAbilityEntity, const TSubclassOf<UCk_Ability_Script_PDA>& InAbilityClass) -> void
        {
            if (ck::Is_NOT_Valid(InAbilityEntity))
            {
                UUtils_Signal_AbilityOwner_OnAbilityDeactivatedOrNot::Broadcast(
                    InAbilityOwnerEntity, MakePayload(InAbilityOwnerEntity, FCk_Handle_Ability{}, ECk_AbilityOwner_AbilityDeactivatedOrNot::NotDeactivated_AbilityNotFound));

                return;
            }

            if (UCk_Utils_Ability_UE::Get_Status(InAbilityEntity) == ECk_Ability_Status::NotActive)
            {
                UUtils_Signal_AbilityOwner_OnAbilityDeactivatedOrNot::Broadcast(
                    InAbilityOwnerEntity, MakePayload(InAbilityOwnerEntity, FCk_Handle_Ability{}, ECk_AbilityOwner_AbilityDeactivatedOrNot::NotDeactivated_FailedChecks));

                return;
            }

            const auto& AbilityActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(InAbilityEntity);
            const auto& GrantedTags = AbilityActivationSettings.Get_ActivationSettingsOnOwner().Get_GrantTagsOnAbilityOwner();

            InAbilityOwnerComp.RemoveTags(InAbilityOwnerEntity, GrantedTags);

            if (InAbilityOwnerComp.Get_AreActiveTagsDifferentThanPreviousTags(InAbilityOwnerEntity))
            {
                UCk_Utils_AbilityOwner_UE::Request_TagsUpdated(InAbilityOwnerEntity);
            }

            ability::VeryVerbose
            (
                TEXT("DEACTIVATING Ability [Name: {} | Entity: {}] from Ability Owner [{}] and Removing Tags [{}]"),
                InAbilityClass,
                InAbilityEntity,
                InAbilityOwnerEntity,
                GrantedTags
            );

            UCk_Utils_Ability_UE::DoDeactivate(InAbilityOwnerEntity, InAbilityEntity);

            UUtils_Signal_AbilityOwner_OnAbilityDeactivatedOrNot::Broadcast(
                    InAbilityOwnerEntity, MakePayload(InAbilityOwnerEntity, InAbilityEntity, ECk_AbilityOwner_AbilityDeactivatedOrNot::Deactivated));
        };

        switch (const auto& SearchPolicy = InRequest.Get_SearchPolicy())
        {
            case ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass:
            {
                auto FoundAbilityEntity = DoFindAbilityByClass(InAbilityOwnerEntity, InRequest.Get_AbilityClass());

                DoDeactivateAbility(FoundAbilityEntity, InRequest.Get_AbilityClass());

                break;
            }
            case ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle:
            {
                auto FoundAbilityEntity = DoFindAbilityByHandle(InAbilityOwnerEntity, InRequest.Get_AbilityHandle());

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
            FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
            const TSubclassOf<UCk_Ability_Script_PDA>& InAbilityClass)
        -> FCk_Handle_Ability
    {
        const auto& FoundAbilityWithName = UCk_Utils_Ability_UE::RecordOfAbilities_Utils::Get_ValidEntry_If
        (
            InAbilityOwnerEntity,
            [InAbilityClass](const FCk_Handle_Ability& InAbilityHandle)
            { return UCk_Utils_Ability_UE::Get_ScriptClass(InAbilityHandle) == InAbilityClass; }
        );

        if (ck::Is_NOT_Valid(FoundAbilityWithName))
        {
            ability::Verbose(TEXT("Failed to Find Ability [{}] in Ability Owner [{}]"), InAbilityClass, InAbilityOwnerEntity);
            return {};
        }

        return FoundAbilityWithName;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoFindAbilityByHandle(
            const FCk_Handle_AbilityOwner& InAbilityOwnerEntity,
            const FCk_Handle_Ability& InAbilityEntity)
        -> FCk_Handle_Ability
    {
        const auto& HasAbilityWithEntity = UCk_Utils_Ability_UE::RecordOfAbilities_Utils::Get_ContainsEntry
        (
            InAbilityOwnerEntity,
            InAbilityEntity
        );

        if (NOT HasAbilityWithEntity)
        {
            ability::Verbose(TEXT("Failed to Find Ability [{}] in Ability Owner [{}]"), InAbilityEntity, InAbilityOwnerEntity);
            return {};
        }

        return InAbilityEntity;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_TagsUpdated::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        Super::Tick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_TagsUpdated::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_AbilityOwner_Current& InAbilityOwnerComp) const
        -> void
    {
        // Cancel All Abilities that are cancelled by the newly granted tags
        // TODO: this is repeated multiple times in this file, move to a common function
        UCk_Utils_AbilityOwner_UE::ForEach_Ability_If
        (
            InHandle,
            [&](const FCk_Handle_Ability& InAbilityEntityToCancel)
            {
                ability::Verbose
                (
                    TEXT("CANCELLING Ability [Name: {} | Entity: {}] after Tags Changed on Ability Owner [{}]"),
                    UCk_Utils_GameplayLabel_UE::Get_Label(InAbilityEntityToCancel),
                    InAbilityEntityToCancel,
                    InHandle
                );

                UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(InHandle,
                    FCk_Request_AbilityOwner_DeactivateAbility{InAbilityEntityToCancel}, {});
            },
            algo::MatchesAnyAbilityActivationCancelledTags{InAbilityOwnerComp.Get_ActiveTags(InHandle)}
        );

        if (InAbilityOwnerComp.Get_AreActiveTagsDifferentThanPreviousTags(InHandle))
        {
            UUtils_Signal_AbilityOwner_OnTagsUpdated::Broadcast(InHandle,
                MakePayload(InHandle, InAbilityOwnerComp.Get_ActiveTags(InHandle)));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            const HandleType& InHandle,
            const FFragment_AbilityOwner_Current& InCurrent) const
        -> void
    {
        // if we are an EntityExtension, then inform our ExtensionOwner of potentially updated tags
        InCurrent.DoTry_TagsUpdatedOnExtensionOwner(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------
