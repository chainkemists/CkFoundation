#include "CkAbilityOwner_Processor.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Script.h"
#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"
#include "CkAbility/Subsystem/CkAbility_Subsystem.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEntityBridge/CkEntityBridge_Utils.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_AbilityOwner_EnsureAllAppended::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FCk_Fragment_AbilityOwner_ParamsData& InExtraParams) const
        -> void
    {
        CK_ENSURE(UCk_Utils_AbilityOwner_UE::Has(InHandle),
            TEXT("Handle [{}] has pending Abilities to Append but does not have an AbilityOwner Feature!"), InHandle);

        InHandle.Remove<FCk_Fragment_AbilityOwner_ParamsData>();
    }

    // --------------------------------------------------------------------------------------------------------------------

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

            UCk_Utils_AbilityOwner_UE::Request_GiveAbility
            (
                InHandle,
                FCk_Request_AbilityOwner_GiveAbility{DefaultAbility, InHandle}
                .Set_ConstructionPhase(ECk_ConstructionPhase::DuringConstruction),
                {}
            );

            InHandle.Add<FTag_AbilityOwner_PendingSubAbilityOperation>();
        }

        for (const auto& Params = InAbilityOwnerParams.Get_Params(); const auto& DefaultAbilityInstance : Params.Get_DefaultAbilities_Instanced())
        {
            CK_ENSURE_IF_NOT(ck::IsValid(DefaultAbilityInstance),
                TEXT("Entity [{}] has an INVALID default Ability INSTANCE in its Params! This can only happen for Sub-Abilities"), InHandle)
            { continue; }

            UCk_Utils_AbilityOwner_UE::Request_GiveAbility
            (
                InHandle,
                FCk_Request_AbilityOwner_GiveAbility{DefaultAbilityInstance->GetClass(), InHandle}
                .Set_AbilityScriptArchetype(DefaultAbilityInstance)
                .Set_ConstructionPhase(ECk_ConstructionPhase::DuringConstruction),
                {}
            );

            InHandle.Add<FTag_AbilityOwner_PendingSubAbilityOperation>();
        }

        // It's possible that we have pending replication info
        // This code is in Setup instead of Add since we need to have added the default abilities first
        if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InHandle))
        {
            if (UCk_Utils_Ecs_Net_UE::Get_HasReplicatedFragment<UCk_Fragment_AbilityOwner_Rep>(InHandle))
            {
                InHandle.Try_Transform<TObjectPtr<UCk_Fragment_AbilityOwner_Rep>>(
                [&](const TObjectPtr<UCk_Fragment_AbilityOwner_Rep>& InRepComp)
                {
                    InRepComp->Request_TryUpdateReplicatedFragment();
                });
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_HandleEvents::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_AbilityOwner_Events&  InAbilityOwnerEvents)
        -> void
    {
        const auto CopiedEvents = InAbilityOwnerEvents.Get_Events();
        InHandle.Remove<MarkedDirtyBy>();

        for (const auto& Event : CopiedEvents)
        {
            UUtils_Signal_AbilityOwner_SingleEvent::Broadcast(InHandle, MakePayload(InHandle, Event));
        }

        UUtils_Signal_AbilityOwner_Events::Broadcast(InHandle, MakePayload(InHandle, CopiedEvents));
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
        InHandle.Remove<MarkedDirtyBy>();

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
    }

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_AddAndGiveExistingAbility& InRequest) const
        -> void
    {
        using RecordOfAbilities_Utils = UCk_Utils_AbilityOwner_UE::RecordOfAbilities_Utils;
        auto AbilityToAddAndGive = InRequest.Get_Ability();

        const auto AbilityGivenOrNot = [&]() -> ECk_AbilityOwner_AbilityGivenOrNot
        {
            const auto& AbilityToAddAndGiveLifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(AbilityToAddAndGive);

            CK_ENSURE_IF_NOT(ck::IsValid(AbilityToAddAndGive),
                TEXT("INVALID Ability to ADD and GIVE to Ability Owner [{}]"),
                InAbilityOwnerEntity)
            { return ECk_AbilityOwner_AbilityGivenOrNot::NotGiven; }

            CK_ENSURE_IF_NOT(NOT RecordOfAbilities_Utils::Get_ContainsEntry(InAbilityOwnerEntity, AbilityToAddAndGive),
                TEXT("Cannot ADD and GIVE Ability [{}] to Ability Owner [{}] because it already has this Ability"),
                AbilityToAddAndGive, InAbilityOwnerEntity)
            { return ECk_AbilityOwner_AbilityGivenOrNot::NotGiven; }

            const auto& CurrentOwnerOfAbilityToAddAndGive = UCk_Utils_Ability_UE::TryGet_Owner(AbilityToAddAndGive);

            CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(CurrentOwnerOfAbilityToAddAndGive),
                TEXT("Cannot ADD and GIVE Ability [{}] to Ability Owner [{}] because it still belongs to Ability Owner [{}]"),
                AbilityToAddAndGive, InAbilityOwnerEntity, CurrentOwnerOfAbilityToAddAndGive)
            { return ECk_AbilityOwner_AbilityGivenOrNot::NotGiven; }

            CK_ENSURE_IF_NOT(InAbilityOwnerEntity == AbilityToAddAndGiveLifetimeOwner,
                TEXT("Cannot ADD and GIVE Ability [{}] to Ability Owner [{}] because it's LifetimeOwner belongs to [{}], which is NOT the Ability Owner"),
                AbilityToAddAndGive, InAbilityOwnerEntity, AbilityToAddAndGiveLifetimeOwner)
            { return ECk_AbilityOwner_AbilityGivenOrNot::NotGiven; }

            const auto& AbilitySource = InRequest.Get_AbilitySource();
            const auto& OptionalPayload = InRequest.Get_OptionalPayload();

            ability::VeryVerbose
            (
                TEXT("Giving Ability [Class: {} | Entity: {}] to Ability Owner [{}]"),
                UCk_Utils_Ability_UE::Get_ScriptClass(AbilityToAddAndGive),
                AbilityToAddAndGive,
                InAbilityOwnerEntity
            );

            {
                const auto& AbilityOnGiveSettings = UCk_Utils_Ability_UE::Get_OnGiveSettings(AbilityToAddAndGive);
                const auto& GrantedTags = AbilityOnGiveSettings.Get_OnGiveSettingsOnOwner().Get_GrantTagsOnAbilityOwner();

                // HACK: need a non-const handle as we're unable to make the lambda mutable
                auto NonConstAbilityOwnerEntity = InAbilityOwnerEntity;
                auto& AbilityOwnerComp = NonConstAbilityOwnerEntity.Get<FFragment_AbilityOwner_Current>();
                AbilityOwnerComp.AppendTags(InAbilityOwnerEntity, GrantedTags);
            }

            InAbilityOwnerEntity.Add<FTag_AbilityOwner_PendingSubAbilityOperation>();

            auto RequestGive = ck::FFragment_Ability_RequestGive{AbilitySource, OptionalPayload};
            InRequest.Request_TransferHandleToOther(RequestGive);
            AbilityToAddAndGive.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(RequestGive);

            //UCk_Utils_Ability_UE::DoGive(InAbilityOwnerEntity, AbilityToAddAndGive, AbilitySource, OptionalPayload);

            //if (const auto& ActivationPolicy = UCk_Utils_Ability_UE::Get_ActivationSettings(AbilityToAddAndGive).Get_ActivationPolicy();
            //    ActivationPolicy == ECk_Ability_Activation_Policy::ActivateOnGranted)
            //{
            //    UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(
            //        InAbilityOwnerEntity,
            //        FCk_Request_AbilityOwner_ActivateAbility{AbilityToAddAndGive}
            //        .Set_OptionalPayload(FCk_Ability_Payload_OnActivate{}.Set_ContextEntity(InAbilityOwnerEntity)),
            //        {});
            //}

            return ECk_AbilityOwner_AbilityGivenOrNot::Given;
        }();

        if (InRequest.Get_IsRequestHandleValid())
        {
            UUtils_Signal_AbilityOwner_OnAbilityGivenOrNot::Broadcast(
                InRequest.GetAndDestroyRequestHandle(),
                MakePayload(InAbilityOwnerEntity, AbilityToAddAndGive, AbilityGivenOrNot));
        }

        //if (AbilityGivenOrNot == ECk_AbilityOwner_AbilityGivenOrNot::Given)
        //{
        //    UUtils_Signal_AbilityOwner_OnAbilityGiven::Broadcast(
        //        InAbilityOwnerEntity,
        //        MakePayload(InAbilityOwnerEntity, AbilityToAddAndGive));
        //}
    }

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_TransferExistingAbility& InRequest) const
        -> void
    {
        using RecordOfAbilities_Utils = UCk_Utils_AbilityOwner_UE::RecordOfAbilities_Utils;

        auto AbilityToTransfer = InRequest.Get_Ability();
        auto TransferTarget = InRequest.Get_TransferTarget();

        const auto& AbilityTransferredOrNot = [&]() -> ECk_AbilityOwner_AbilityTransferredOrNot
        {
            CK_ENSURE_IF_NOT(ck::IsValid(AbilityToTransfer),
                TEXT("INVALID Ability to TRANSFER from Ability Owner [{}]"),
                InAbilityOwnerEntity)
            { return ECk_AbilityOwner_AbilityTransferredOrNot::NotTransferred; }

            CK_ENSURE_IF_NOT(ck::IsValid(TransferTarget), TEXT("INVALID Target to TRANSFER Ability [{}] from Ability Owner [{}] to"),
                AbilityToTransfer, InAbilityOwnerEntity)
            { return ECk_AbilityOwner_AbilityTransferredOrNot::NotTransferred; }

            if (TransferTarget == InAbilityOwnerEntity)
            { return ECk_AbilityOwner_AbilityTransferredOrNot::NotTransferred; }

            CK_ENSURE_IF_NOT(RecordOfAbilities_Utils::Get_ContainsEntry(InAbilityOwnerEntity, AbilityToTransfer),
                TEXT("Cannot TRANSFER Ability [{}] from Ability Owner [{}] to [{}] because it does NOT have such an ability"),
                AbilityToTransfer, InAbilityOwnerEntity, TransferTarget)
            { return ECk_AbilityOwner_AbilityTransferredOrNot::NotTransferred; }

            DoHandleRequest(InAbilityOwnerEntity, InAbilityOwnerComp,
                FCk_Request_AbilityOwner_DeactivateAbility{AbilityToTransfer});

            DoHandleRequest(InAbilityOwnerEntity, InAbilityOwnerComp,
                FCk_Request_AbilityOwner_RevokeAbility{AbilityToTransfer}
                .Set_DestructionPolicy(ECk_AbilityOwner_DestructionOnRevoke_Policy::DoNotDestroyOnRevoke));

            CK_ENSURE_IF_NOT(NOT RecordOfAbilities_Utils::Get_ContainsEntry(TransferTarget, AbilityToTransfer),
                TEXT("Cannot TRANSFER Ability [{}] from Ability Owner [{}] to [{}] because the recipient already has this ability"
                     "A request to remove the ability from the source was made, but the recipient somehow still has the ability (possibly by extension?)"),
                AbilityToTransfer, InAbilityOwnerEntity, TransferTarget)
            { return ECk_AbilityOwner_AbilityTransferredOrNot::NotTransferred; }

            UCk_Utils_EntityLifetime_UE::Request_TransferLifetimeOwner(AbilityToTransfer, TransferTarget);

            DoHandleRequest(TransferTarget, TransferTarget.Get<FFragment_AbilityOwner_Current>(),
                FCk_Request_AbilityOwner_AddAndGiveExistingAbility
                {
                    AbilityToTransfer,
                    InRequest.Get_AbilitySource()
                }.Set_OptionalPayload(InRequest.Get_OptionalPayload()));

            return ECk_AbilityOwner_AbilityTransferredOrNot::Transferred;
        }();

        if (InRequest.Get_IsRequestHandleValid())
        {
            UUtils_Signal_AbilityOwner_OnAbilityTransferredOrNot::Broadcast(
                InRequest.GetAndDestroyRequestHandle(),
                MakePayload(InAbilityOwnerEntity, TransferTarget, AbilityToTransfer, AbilityTransferredOrNot));
        }

        if (AbilityTransferredOrNot == ECk_AbilityOwner_AbilityTransferredOrNot::Transferred)
        {
            UUtils_Signal_AbilityOwner_OnAbilityTransferred::Broadcast(
                InAbilityOwnerEntity,
                MakePayload(InAbilityOwnerEntity, TransferTarget, AbilityToTransfer));
        }
    }

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
                UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAbilityOwnerEntity), AbilityScriptClass, InRequest.Get_AbilityScriptArchetype());

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
            [InAbilityOwnerEntity, AbilityScriptClass, AbilityParams, OptionalPayload, AbilitySource, AbilityEntityConfig, InRequest]
            (FCk_Handle& InEntity) -> void
            {
                UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(AbilityEntityConfig->GetWorld())->Request_UntrackAbilityEntityConfig(AbilityEntityConfig);

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

                {
                    const auto& AbilityOnGiveSettings = UCk_Utils_Ability_UE::Get_OnGiveSettings(AbilityEntity);
                    const auto& GrantedTags = AbilityOnGiveSettings.Get_OnGiveSettingsOnOwner().Get_GrantTagsOnAbilityOwner();

                    // HACK: need a non-const handle as we're unable to make the lambda mutable
                    auto NonConstAbilityOwnerEntity = InAbilityOwnerEntity;
                    auto& AbilityOwnerComp = NonConstAbilityOwnerEntity.Get<FFragment_AbilityOwner_Current>();
                    AbilityOwnerComp.AppendTags(NonConstAbilityOwnerEntity, GrantedTags);
                }

                if (InRequest.Get_ConstructionPhase() == ECk_ConstructionPhase::AfterConstruction)
                {
                    AbilityOwnerEntity.Add<FTag_AbilityOwner_PendingSubAbilityOperation>();
                }
                else
                {
                    CK_TRIGGER_ENSURE_IF(NOT InAbilityOwnerEntity.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
                        TEXT("Expected AbilityOwner [{}] to have PendingSubAbilities tag before adding sub-abilities"), InAbilityOwnerEntity)
                    { }
                }

                auto RequestGive = ck::FFragment_Ability_RequestGive{AbilitySource, OptionalPayload};
                InRequest.Request_TransferHandleToOther(RequestGive);
                AbilityEntity.Add<ck::FFragment_Ability_RequestGive>(RequestGive);

                //UCk_Utils_Ability_UE::DoGive(AbilityOwnerEntity, AbilityEntity, AbilitySource, OptionalPayload);

                //if (InRequest.Get_IsRequestHandleValid())
                //{
                //    UUtils_Signal_AbilityOwner_OnAbilityGivenOrNot::Broadcast(
                //        InRequest.GetAndDestroyRequestHandle(),
                //        MakePayload(AbilityOwnerEntity, AbilityEntity, ECk_AbilityOwner_AbilityGivenOrNot::Given));
                //}

                //UUtils_Signal_AbilityOwner_OnAbilityGiven::Broadcast(
                //    AbilityOwnerEntity,
                //    MakePayload(AbilityOwnerEntity, AbilityEntity));

                //if (const auto& ActivationPolicy = UCk_Utils_Ability_UE::Get_ActivationSettings(AbilityEntity).Get_ActivationPolicy();
                //    ActivationPolicy == ECk_Ability_Activation_Policy::ActivateOnGranted)
                //{
                //    UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(
                //        AbilityOwnerEntity,
                //        FCk_Request_AbilityOwner_ActivateAbility{AbilityEntity}
                //        .Set_OptionalPayload(FCk_Ability_Payload_OnActivate{}.Set_ContextEntity(AbilityOwnerEntity)),
                //        {});
                //}
            };

            UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(AbilityEntityConfig->GetWorld())->Request_TrackAbilityEntityConfig(AbilityEntityConfig);

            if (AbilityData.Get_NetworkSettings().Get_FeatureReplicationPolicy() == ECk_Ability_FeatureReplication_Policy::ReplicateAbilityFeatures)
            {
                if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InAbilityOwnerEntity))
                {
                    auto AbilityEntity = UCk_Utils_EntityReplicationDriver_UE::Request_TryReplicateAbility
                    (
                        InAbilityOwnerEntity,
                        AbilityConstructionScript,
                        AbilityScriptClass,
                        AbilitySource,
                        InRequest.Get_ConstructionPhase()
                    );

                    PostAbilityCreationFunc(AbilityEntity);
                }
                else
                {
                     ability::Verbose
                    (
                        TEXT("Skipping Giving Ability [{}] with Script [{}] because AbilityOwner [{}] NetMode is NOT host but the feature replication policy is set to [{}]"),
                        AbilityEntityConfig,
                        AbilityScriptClass,
                        InAbilityOwnerEntity,
                        AbilityData.Get_NetworkSettings().Get_FeatureReplicationPolicy()
                    );

                    return ECk_AbilityOwner_AbilityGivenOrNot::NotGiven;
                }
            }
            else
            {
                UCk_Utils_EntityBridge_UE::Request_Spawn(InAbilityOwnerEntity,
                    FCk_Request_EntityBridge_SpawnEntity{AbilityEntityConfig}.Set_OptionalObjectConstructionScript(InRequest.Get_AbilityScriptClass()->ClassDefaultObject)
                    .Set_PostSpawnFunc(PostAbilityCreationFunc),
                    {},
                    {});
            }

            return ECk_AbilityOwner_AbilityGivenOrNot::Given;
        }();

        if (AbilityGivenOrNot == ECk_AbilityOwner_AbilityGivenOrNot::NotGiven && InRequest.Get_IsRequestHandleValid())
        {
            UUtils_Signal_AbilityOwner_OnAbilityGivenOrNot::Broadcast(
                InRequest.GetAndDestroyRequestHandle(),
                MakePayload(InAbilityOwnerEntity, FCk_Handle_Ability{}, ECk_AbilityOwner_AbilityGivenOrNot::NotGiven));
        }
    }

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp,
            const FCk_Request_AbilityOwner_GiveReplicatedAbility& InRequest) const
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
                UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAbilityOwnerEntity), AbilityScriptClass, InRequest.Get_AbilityScriptArchetype());

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

            CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptClass),
                TEXT("Cannot GIVE Ability to Ability Owner [{}] using Construction Script [{}] because the ScriptClass [{}] is INVALID"),
                InAbilityOwnerEntity,
                AbilityConstructionScript,
                AbilityScriptClass)
            { return ECk_AbilityOwner_AbilityGivenOrNot::NotGiven; }

            const auto& AbilityParams = AbilityConstructionScript->Get_AbilityParams();
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

            const auto PostAbilityCreationFunc =
            [InAbilityOwnerEntity, AbilityScriptClass, AbilitySource, AbilityEntityConfig, InRequest](FCk_Handle& InEntity) -> void
            {
                UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(AbilityEntityConfig->GetWorld())->Request_UntrackAbilityEntityConfig(AbilityEntityConfig);

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

                {
                    const auto& AbilityOnGiveSettings = UCk_Utils_Ability_UE::Get_OnGiveSettings(AbilityEntity);
                    const auto& GrantedTags = AbilityOnGiveSettings.Get_OnGiveSettingsOnOwner().Get_GrantTagsOnAbilityOwner();

                    // HACK: need a non-const handle as we're unable to make the lambda mutable
                    auto NonConstAbilityOwnerEntity = InAbilityOwnerEntity;
                    auto& AbilityOwnerComp = NonConstAbilityOwnerEntity.Get<FFragment_AbilityOwner_Current>();
                    AbilityOwnerComp.AppendTags(NonConstAbilityOwnerEntity, GrantedTags);
                }

                if (InRequest.Get_ConstructionPhase() == ECk_ConstructionPhase::AfterConstruction)
                {
                    AbilityOwnerEntity.Add<FTag_AbilityOwner_PendingSubAbilityOperation>();
                }
                else
                {
                    CK_TRIGGER_ENSURE_IF(NOT InAbilityOwnerEntity.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
                        TEXT("Expected AbilityOwner [{}] to have PendingSubAbilities tag before adding sub-abilities"), InAbilityOwnerEntity)
                    { }
                }

                auto RequestGive = ck::FFragment_Ability_RequestGive{AbilitySource, {}};
                InRequest.Request_TransferHandleToOther(RequestGive);
                AbilityEntity.Add<ck::FFragment_Ability_RequestGive>(RequestGive);

                //UCk_Utils_Ability_UE::DoGive(AbilityOwnerEntity, AbilityEntity, AbilitySource, {});

                //if (InRequest.Get_IsRequestHandleValid())
                //{
                //    UUtils_Signal_AbilityOwner_OnAbilityGivenOrNot::Broadcast(
                //        InRequest.GetAndDestroyRequestHandle(),
                //        MakePayload(AbilityOwnerEntity, AbilityEntity, ECk_AbilityOwner_AbilityGivenOrNot::Given));
                //}

                //UUtils_Signal_AbilityOwner_OnAbilityGiven::Broadcast(
                //    AbilityOwnerEntity,
                //    MakePayload(AbilityOwnerEntity, AbilityEntity));

                //if (const auto& ActivationPolicy = UCk_Utils_Ability_UE::Get_ActivationSettings(AbilityEntity).Get_ActivationPolicy();
                //    ActivationPolicy == ECk_Ability_Activation_Policy::ActivateOnGranted)
                //{
                //    UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(
                //        AbilityOwnerEntity,
                //        FCk_Request_AbilityOwner_ActivateAbility{AbilityEntity}
                //        .Set_OptionalPayload(FCk_Ability_Payload_OnActivate{}.Set_ContextEntity(AbilityOwnerEntity)),
                //        {});
                //}
            };

            UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(AbilityEntityConfig->GetWorld())->Request_TrackAbilityEntityConfig(AbilityEntityConfig);

            auto ReplicatedAbilityEntity = InRequest.Get_ReplicatedEntityToUse();
            AbilityConstructionScript->Construct(ReplicatedAbilityEntity, {}, InRequest.Get_AbilityScriptClass()->ClassDefaultObject);
            PostAbilityCreationFunc(ReplicatedAbilityEntity);

            return ECk_AbilityOwner_AbilityGivenOrNot::Given;
        }();

        // HACK: The #SyncedDrivers count is incremented here instead of doing so inside the RepDriverFragment.
        // This ensures that the increment occurs only after the replicated ability has been created and assigned.
        // Incrementing prematurely would cause the ReplicationComplete and ReplicationCompleteAllDependents signals to fire too early.
        // If the replicated ability is added as an EntityExtension, any attempts to manipulate extended features (e.g., Attributes)
        // would fail because those features would not yet exist.
        [&]()
        {
            if (InRequest.Get_ConstructionPhase() != ECk_ConstructionPhase::DuringConstruction)
            { return; }

            if (NOT UCk_Utils_Ecs_Net_UE::Get_HasReplicatedFragment<UCk_Fragment_AbilityOwner_Rep>(InAbilityOwnerEntity))
            { return; }

            const auto& ReplicatedAbilityEntity = InRequest.Get_ReplicatedEntityToUse();
            const auto& RepDriver_ReplicatedAbilityEntity = ReplicatedAbilityEntity.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();

            CK_ENSURE_IF_NOT(ck::IsValid(RepDriver_ReplicatedAbilityEntity),
                TEXT("The Replication Driver for the Replication Ability [{}] Entity is INVALID\n"
                     "Unable to increment the count of its Owning Entity Driver's Synced Dependent Replication Drivers\n"
                     "This might cause the Owning Entity's ReplicationComplete/ReplicationComplete_AllDependent signals to never fire"),
                ReplicatedAbilityEntity)
            { return; }

            const auto& ReplicationData_Ability = RepDriver_ReplicatedAbilityEntity->Get_ReplicationData_Ability();
            const auto& RepDriver_ReplicatedAbilityOwningEntity = ReplicationData_Ability.Get_OwningEntityDriver();

            CK_ENSURE_IF_NOT(ck::IsValid(RepDriver_ReplicatedAbilityOwningEntity),
                TEXT("The Replication Driver for the Replication Ability [{}] OWNING Entity is INVALID\n"
                     "Unable to increment the count of its Owning Entity Driver's Synced Dependent Replication Drivers\n"
                     "This might cause the Owning Entity's ReplicationComplete/ReplicationComplete_AllDependent signals to never fire"),
                ReplicatedAbilityEntity)
            { return; }

            RepDriver_ReplicatedAbilityOwningEntity->DoAdd_SyncedDependentReplicationDriver();
        }();

        if (AbilityGivenOrNot == ECk_AbilityOwner_AbilityGivenOrNot::NotGiven && InRequest.Get_IsRequestHandleValid())
        {
            UUtils_Signal_AbilityOwner_OnAbilityGivenOrNot::Broadcast(
                InRequest.GetAndDestroyRequestHandle(), MakePayload(InAbilityOwnerEntity, FCk_Handle_Ability{},
                    ECk_AbilityOwner_AbilityGivenOrNot::NotGiven));
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
                if (InRequest.Get_IsRequestHandleValid())
                {
                    UUtils_Signal_AbilityOwner_OnAbilityRevokedOrNot::Broadcast(
                        InRequest.GetAndDestroyRequestHandle(),
                        MakePayload(InAbilityOwnerEntity, FCk_Handle_Ability{}, ECk_AbilityOwner_AbilityRevokedOrNot::NotRevoked));
                }

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
            }

            InAbilityOwnerEntity.Add<FTag_AbilityOwner_PendingSubAbilityOperation>();

            auto RequestRevoke = ck::FFragment_Ability_RequestRevoke{InRequest.Get_DestructionPolicy()};
            InRequest.Request_TransferHandleToOther(RequestRevoke);
            InAbilityEntity.Add<ck::FFragment_Ability_RequestRevoke>(RequestRevoke);

            //UCk_Utils_Ability_UE::DoRevoke(InAbilityOwnerEntity, InAbilityEntity, InRequest.Get_DestructionPolicy());

            //if (InRequest.Get_IsRequestHandleValid())
            //{
            //    UUtils_Signal_AbilityOwner_OnAbilityRevokedOrNot::Broadcast(
            //        InRequest.GetAndDestroyRequestHandle(),
            //        MakePayload(InAbilityOwnerEntity, InAbilityEntity, ECk_AbilityOwner_AbilityRevokedOrNot::Revoked));
            //}

            //UUtils_Signal_AbilityOwner_OnAbilityRevoked::Broadcast(
            //    InAbilityOwnerEntity,
            //    MakePayload(InAbilityOwnerEntity, InAbilityEntity));
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
                if (InRequest.Get_IsRequestHandleValid())
                {
                    UUtils_Signal_AbilityOwner_OnAbilityActivatedOrNot::Broadcast(
                        InRequest.GetAndDestroyRequestHandle(),
                        MakePayload(InAbilityOwnerEntity, InAbilityToActivateEntity, ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_AbilityNotFound));
                }

                return;
            }

            if (InAbilityToActivateEntity.Has<ck::FFragment_AbilityOwner_Requests>())
            {
                ability::Verbose
                (
                    TEXT("NOT Activating Ability [Entity: {}] on Ability Owner [{}] because there are Sub-Abilities with pending requests"),
                    InAbilityToActivateEntity,
                    InAbilityOwnerEntity
                );

                UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(InAbilityOwnerEntity, InRequest, {});
                return;
            }

            if (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InAbilityToActivateEntity, ECk_EntityLifetime_DestructionPhase::InitiatedOrConfirmed))
            {
                ability::Verbose
                (
                    TEXT("NOT Activating Ability [Entity: {}] on Ability Owner [{}] because the Ability has Initiated Or Confirmed Destruction."),
                    InAbilityToActivateEntity,
                    InAbilityOwnerEntity
                );

                if (InRequest.Get_IsRequestHandleValid())
                {
                    UUtils_Signal_AbilityOwner_OnAbilityActivatedOrNot::Broadcast(
                        InRequest.GetAndDestroyRequestHandle(),
                        MakePayload(InAbilityOwnerEntity, InAbilityToActivateEntity, ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_FailedChecks));
                }
                return;
            }

            auto& RequestsComp = InAbilityOwnerEntity.AddOrGet<FFragment_AbilityOwner_Requests>();
            const auto NumRequestsBeforeActivation = RequestsComp.Get_Requests().Num();

            const auto AbilityActivatedOrNot = [&]() -> ECk_AbilityOwner_AbilityActivatedOrNot
            {
                const auto& AbilityToActivateName = UCk_Utils_GameplayLabel_UE::Get_Label(InAbilityToActivateEntity);
                const auto& AbilityActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(InAbilityToActivateEntity);
                const auto& GrantedTags = AbilityActivationSettings.Get_ActivationSettingsOnOwner().Get_GrantTagsOnAbilityOwner();

                const auto& AbilityCurrent = InAbilityToActivateEntity.Get<ck::FFragment_Ability_Current>();
                const auto& Script         = AbilityCurrent.Get_AbilityScript();

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

                        break;
                    }
                    case ECk_Ability_ActivationRequirementsResult::RequirementsMet_ButAlreadyActive:
                    {
                        if (AbilityActivationSettings.Get_ReactivationPolicy() == ECk_Ability_Reactivation_Policy::OnlyActivateIfCurrentlyDeactivated)
                        {
                            ability::Verbose
                            (
                                TEXT("Failed to ACTIVATE Ability [Name: {} | Entity: {}] on Ability Owner [{}]! "
                                     "The Activation Requirements are met BUT the Ability is ALREADY ACTIVE"),
                                AbilityToActivateName,
                                InAbilityToActivateEntity,
                                InAbilityOwnerEntity
                            );

                            Script->OnAbilityNotActivated(FCk_Ability_NotActivated_Info{InAbilityToActivateEntity, CanActivateAbility});

                            return ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_FailedChecks;
                        }

                        ability::Verbose
                        (
                            TEXT("RE-ACTIVATING Ability [Name: {} | Entity: {}] on Ability Owner [{}] and Granting Tags [{}]"),
                            AbilityToActivateName,
                            InAbilityToActivateEntity,
                            InAbilityOwnerEntity,
                            GrantedTags
                        );

                        if (AbilityActivationSettings.Get_ReactivationPolicy() != ECk_Ability_Reactivation_Policy::AllowActivationIfAlreadyActiveDoNotDeactivate)
                        { DoHandleRequest(InAbilityOwnerEntity, InAbilityOwnerComp, FCk_Request_AbilityOwner_DeactivateAbility{InAbilityToActivateEntity}); }

                        Script->OnReactivateAbility(InRequest.Get_OptionalPayload());

                        break;
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

                        Script->OnAbilityNotActivated(FCk_Ability_NotActivated_Info{InAbilityToActivateEntity, CanActivateAbility});

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

                        Script->OnAbilityNotActivated(FCk_Ability_NotActivated_Info{InAbilityToActivateEntity, CanActivateAbility});

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

                        Script->OnAbilityNotActivated(FCk_Ability_NotActivated_Info{InAbilityToActivateEntity, CanActivateAbility});

                        return ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_FailedChecks;
                    }
                    case ECk_Ability_ActivationRequirementsResult::RequirementsNotMet_BlockedByOwner:
                    {
                        ability::Verbose
                        (
                            TEXT("Failed to ACTIVATE Ability [Name: {} | Entity: {}] on Ability Owner [{}] "
                                 "because the Ability Owner has BLOCKED all SubAbilities"),
                            AbilityToActivateName,
                            InAbilityToActivateEntity,
                            InAbilityOwnerEntity
                        );

                        Script->OnAbilityNotActivated(FCk_Ability_NotActivated_Info{InAbilityToActivateEntity, CanActivateAbility});

                        return ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_FailedChecks;
                    }
                    default:
                    {
                        CK_INVALID_ENUM(CanActivateAbility);
                        Script->OnAbilityNotActivated(FCk_Ability_NotActivated_Info{InAbilityToActivateEntity, CanActivateAbility});
                        return ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_FailedChecks;
                    }
                }

                InAbilityOwnerComp.AppendTags(InAbilityOwnerEntity, GrantedTags);

                // Try Deactivate our own Ability if we have one
                if (UCk_Utils_Ability_UE::Has(InAbilityOwnerEntity))
                {
                    if (const auto Condition = algo::MatchesAnyAbilityActivationCancelledTagsOnSelf{GrantedTags};
                        Condition(InAbilityOwnerEntity))
                    {
                        auto MyOwner = UCk_Utils_AbilityOwner_UE::CastChecked(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAbilityOwnerEntity));
                        auto& MyOwnerAbilityOwnerCurrent = MyOwner.Get<FFragment_AbilityOwner_Current>();

                        const auto AbilityOwnerAsAbility = UCk_Utils_Ability_UE::CastChecked(InAbilityOwnerEntity);
                        DoHandleRequest(MyOwner, MyOwnerAbilityOwnerCurrent, FCk_Request_AbilityOwner_DeactivateAbility{AbilityOwnerAsAbility});
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

                        DoHandleRequest(InAbilityOwnerEntity, InAbilityOwnerComp, FCk_Request_AbilityOwner_DeactivateAbility{InAbilityEntityToCancel});
                    },
                    algo::MatchesAnyAbilityActivationCancelledTagsOnOwner{GrantedTags}
                );

                InAbilityOwnerEntity.Add<FTag_AbilityOwner_PendingSubAbilityOperation>();

                auto RequestActivate = ck::FFragment_Ability_RequestActivate{InRequest.Get_OptionalPayload()};
                InRequest.Request_TransferHandleToOther(RequestActivate);
                InAbilityToActivateEntity.Add<ck::FFragment_Ability_RequestActivate>(RequestActivate);

                //UCk_Utils_Ability_UE::DoActivate(InAbilityOwnerEntity, InAbilityToActivateEntity, InRequest.Get_OptionalPayload());

                return ECk_AbilityOwner_AbilityActivatedOrNot::Activated;
            }();

            //if (InRequest.Get_IsRequestHandleValid())
            //{
            //    UUtils_Signal_AbilityOwner_OnAbilityActivatedOrNot::Broadcast(
            //            InRequest.GetAndDestroyRequestHandle(),
            //        MakePayload(InAbilityOwnerEntity, InAbilityToActivateEntity, AbilityActivatedOrNot));
            //}

            //if (AbilityActivatedOrNot == ECk_AbilityOwner_AbilityActivatedOrNot::Activated)
            //{
            //    UUtils_Signal_AbilityOwner_OnAbilityActivated::Broadcast(
            //            InAbilityOwnerEntity,
            //        MakePayload(InAbilityOwnerEntity, InAbilityToActivateEntity));
            //}

            //// It's possible that we already have a deactivation request, if yes, process it immediately and remove it from pending requests
            //// This fixes issues caused by waiting until the deactivation request is processed to do tag changes that may be necessary for allowing/blocking subsequent requests
            //const auto ProcessPossibleDeactivationRequest = [&]
            //{
            //    if (RequestsComp.Get_Requests().IsEmpty())
            //    { return; }

            //    if (const auto NewRequestsAfterActivate = RequestsComp.Get_Requests().Num() - NumRequestsBeforeActivation;
            //        NewRequestsAfterActivate == 0)
            //    { return; }

            //    // Try to search newly added request for a deactivation for the same ability we just activated.
            //    // This isn't always guaranteed to be the last/only request
            //    const auto MatchingRequestIndex = [&]() -> int32
            //    {
            //        for (auto Itr = NumRequestsBeforeActivation; Itr < RequestsComp.Get_Requests().Num(); ++Itr)
            //        {
            //            const auto RequestVariant = &RequestsComp.Get_Requests()[Itr];
            //            const auto PendingRequest = std::get_if<FCk_Request_AbilityOwner_DeactivateAbility>(RequestVariant);

            //            if (ck::Is_NOT_Valid(PendingRequest, IsValid_Policy_NullptrOnly{}))
            //            { continue; }

            //            if (PendingRequest->Get_AbilityHandle() == InAbilityToActivateEntity)
            //            { return Itr; }
            //        }

            //        return INDEX_NONE;
            //    }();

            //    if (MatchingRequestIndex == INDEX_NONE)
            //    { return; }

            //    auto RequestToProcessImmediately = RequestsComp._Requests[MatchingRequestIndex];
            //    RequestsComp._Requests.RemoveAt(MatchingRequestIndex);

            //    const auto PendingDeactivationRequest = std::get_if<FCk_Request_AbilityOwner_DeactivateAbility>(&RequestToProcessImmediately);

            //    CK_ENSURE_IF_NOT(ck::IsValid(PendingDeactivationRequest, IsValid_Policy_NullptrOnly{}),
            //        TEXT("Pending Deactivation Request for ability [{}] is NOT valid even though it's validity was just checked, this should never be possible"),
            //        InAbilityToActivateEntity)
            //    { return; }

            //    ability::Verbose(TEXT("DEACTIVATING Ability [{}] on Ability Owner [{}] IMMEDIATELY"),
            //        InAbilityToActivateEntity, InAbilityOwnerEntity);

            //    DoHandleRequest(InAbilityOwnerEntity, InAbilityOwnerComp, *PendingDeactivationRequest);
            //}; ProcessPossibleDeactivationRequest();
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
        const auto& DoDeactivateAbility = [&](FCk_Handle_Ability& InAbilityEntity) -> void
        {
            if (ck::Is_NOT_Valid(InAbilityEntity))
            {
                if (InRequest.Get_IsRequestHandleValid())
                {
                    UUtils_Signal_AbilityOwner_OnAbilityDeactivatedOrNot::Broadcast(
                        InRequest.GetAndDestroyRequestHandle(),
                        MakePayload(InAbilityOwnerEntity, InAbilityEntity, ECk_AbilityOwner_AbilityDeactivatedOrNot::NotDeactivated_AbilityNotFound));
                }

                return;
            }

            if (UCk_Utils_Ability_UE::Get_Status(InAbilityEntity) == ECk_Ability_Status::NotActive)
            {
                if (InRequest.Get_IsRequestHandleValid())
                {
                    UUtils_Signal_AbilityOwner_OnAbilityDeactivatedOrNot::Broadcast(
                        InRequest.GetAndDestroyRequestHandle(),
                        MakePayload(InAbilityOwnerEntity, InAbilityEntity, ECk_AbilityOwner_AbilityDeactivatedOrNot::NotDeactivated_FailedChecks));
                }

                return;
            }

            const auto& AbilityActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(InAbilityEntity);
            const auto& GrantedTags = AbilityActivationSettings.Get_ActivationSettingsOnOwner().Get_GrantTagsOnAbilityOwner();

            InAbilityOwnerComp.RemoveTags(InAbilityOwnerEntity, GrantedTags);

            ability::VeryVerbose
            (
                TEXT("DEACTIVATING Ability [Name: {} | Entity: {}] from Ability Owner [{}] and Removing Tags [{}]"),
                UCk_Utils_Ability_UE::Get_ScriptClass(InAbilityEntity),
                InAbilityEntity,
                InAbilityOwnerEntity,
                GrantedTags
            );

            InAbilityOwnerEntity.Add<FTag_AbilityOwner_PendingSubAbilityOperation>();

            auto RequestDeactivate = ck::FFragment_Ability_RequestDeactivate{};
            InRequest.Request_TransferHandleToOther(RequestDeactivate);
            InAbilityEntity.Add<ck::FFragment_Ability_RequestDeactivate>(RequestDeactivate);

            //UCk_Utils_Ability_UE::DoDeactivate(InAbilityOwnerEntity, InAbilityEntity);

            //if (InRequest.Get_IsRequestHandleValid())
            //{
            //    UUtils_Signal_AbilityOwner_OnAbilityDeactivatedOrNot::Broadcast(
            //        InRequest.GetAndDestroyRequestHandle(),
            //        MakePayload(InAbilityOwnerEntity, InAbilityEntity, ECk_AbilityOwner_AbilityDeactivatedOrNot::Deactivated));
            //}

            //UUtils_Signal_AbilityOwner_OnAbilityDeactivated::Broadcast(
            //    InAbilityOwnerEntity, MakePayload(InAbilityOwnerEntity, InAbilityEntity));
        };

        switch (const auto& SearchPolicy = InRequest.Get_SearchPolicy())
        {
            case ECk_AbilityOwner_AbilitySearch_Policy::SearchByClass:
            {
                auto FoundAbilityEntity = DoFindAbilityByClass(InAbilityOwnerEntity, InRequest.Get_AbilityClass());

                DoDeactivateAbility(FoundAbilityEntity);

                break;
            }
            case ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle:
            {
                auto FoundAbilityEntity = DoFindAbilityByHandle(InAbilityOwnerEntity, InRequest.Get_AbilityHandle());

                DoDeactivateAbility(FoundAbilityEntity);

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
            const FCk_Request_AbilityOwner_CancelSubAbilities& InRequest) const
        -> void
    {
        UCk_Utils_AbilityOwner_UE::ForEach_Ability
        (
            InAbilityOwnerEntity,
            [&](const FCk_Handle_Ability& InAbilityEntityToCancel)
            {
                ability::Verbose
                (
                    TEXT("CANCELLING Ability [Name: {} | Entity: {}] because Ability Owner [{}] requested it"),
                    UCk_Utils_GameplayLabel_UE::Get_Label(InAbilityEntityToCancel),
                    InAbilityEntityToCancel,
                    InAbilityOwnerEntity
                );

                if (auto MaybeAbilityOwner = UCk_Utils_AbilityOwner_UE::Cast(InAbilityEntityToCancel);
                    ck::IsValid(MaybeAbilityOwner))
                {
                    UCk_Utils_AbilityOwner_UE::Request_CancelAllSubAbilities(MaybeAbilityOwner);
                }

                DoHandleRequest(InAbilityOwnerEntity, InAbilityOwnerComp, FCk_Request_AbilityOwner_DeactivateAbility{InAbilityEntityToCancel});
            }
        );
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
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_TagsUpdated::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_AbilityOwner_Current& InAbilityOwnerComp) const
        -> void
    {
        const auto& ActiveTags = InAbilityOwnerComp.Get_ActiveTags(InHandle);

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

                // It's possible that we are the Entity Extension Owner and we are NOT the direct owner of the Ability
                if (UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAbilityEntityToCancel) != InHandle)
                { return; }

                UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(InHandle,
                    FCk_Request_AbilityOwner_DeactivateAbility{InAbilityEntityToCancel}, {});
            },
            algo::MatchesAnyAbilityActivationCancelledTagsOnOwner{ActiveTags}
        );

        if (InAbilityOwnerComp.Get_AreActiveTagsDifferentThanPreviousTags(InHandle))
        {
            const auto& PreviousTags = InAbilityOwnerComp.Get_PreviousTags(InHandle);

            auto ActiveTagsList = TArray<FGameplayTag>{};
            auto PreviousTagsList = TArray<FGameplayTag>{};

            ActiveTags.GetGameplayTagArray(ActiveTagsList);
            PreviousTags.GetGameplayTagArray(PreviousTagsList);

            const auto& TagsAdded = FGameplayTagContainer::CreateFromArray(algo::Except(ActiveTagsList, PreviousTagsList));
            const auto& TagsRemoved = FGameplayTagContainer::CreateFromArray(algo::Except(PreviousTagsList, ActiveTagsList));

            UUtils_Signal_AbilityOwner_OnTagsUpdated::Broadcast(InHandle,
                MakePayload(InHandle, PreviousTags, ActiveTags, TagsRemoved, TagsAdded));
        }

        InAbilityOwnerComp.UpdatePreviousTags();
        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_AbilityOwner_Current& InCurrent) const
        -> void
    {
        // If we are an EntityExtension, then inform our ExtensionOwner of potentially updated tags
        InCurrent.DoTry_TagsUpdatedOnExtensionOwner(InHandle);

        UCk_Utils_AbilityOwner_UE::ForEach_Ability(InHandle, [&](FCk_Handle_Ability InAbilityHandle)
        {
            UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(InHandle, FCk_Request_AbilityOwner_DeactivateAbility{InAbilityHandle}, {});
            UCk_Utils_AbilityOwner_UE::Request_RevokeAbility(InHandle, FCk_Request_AbilityOwner_RevokeAbility{InAbilityHandle}, {});
        });
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------