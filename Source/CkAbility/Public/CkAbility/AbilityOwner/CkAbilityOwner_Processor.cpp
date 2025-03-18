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
            const FFragment_AbilityOwner_Params& InParams) const
        -> void
    {
        for (const auto& Params = InParams.Get_Params(); const auto& DefaultAbility : Params.Get_DefaultAbilities())
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

        for (const auto& Params = InParams.Get_Params(); const auto& DefaultAbilityInstance : Params.Get_DefaultAbilities_Instanced())
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
            FFragment_AbilityOwner_Current&  InCurrent,
            FFragment_AbilityOwner_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InHandle.Remove<MarkedDirtyBy>();

        algo::ForEach(RequestsCopy, ck::Visitor([&](const auto& InRequestVariant)
        {
            DoHandleRequest(InHandle, InCurrent, InRequestVariant);

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
            FFragment_AbilityOwner_Current& InCurrent,
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


            const auto& RequestAddAndGive = ck::FFragment_Ability_RequestAddAndGive{InAbilityOwnerEntity, AbilitySource, OptionalPayload};
            InRequest.Request_TransferHandleToOther(RequestAddAndGive);
            UCk_Utils_Ability_UE::Request_AddAndGiveAbility(AbilityToAddAndGive, RequestAddAndGive);

            return ECk_AbilityOwner_AbilityGivenOrNot::Given;
        }();

        if (InRequest.Get_IsRequestHandleValid())
        {
            UUtils_Signal_AbilityOwner_OnAbilityGivenOrNot::Broadcast(
                InRequest.GetAndDestroyRequestHandle(),
                MakePayload(InAbilityOwnerEntity, AbilityToAddAndGive, InRequest.Get_OptionalPayload(), AbilityGivenOrNot));
        }
    }

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
            const FCk_Request_AbilityOwner_TransferExistingAbility& InRequest) const
        -> void
    {
        auto AbilityToTransfer = InRequest.Get_Ability();
        auto TransferTarget = InRequest.Get_TransferTarget();

        auto RequestTransferExisting = ck::FFragment_Ability_RequestTransferExisting_Initiate{InAbilityOwnerEntity, TransferTarget, AbilityToTransfer};
        InRequest.Request_TransferHandleToOther(RequestTransferExisting);
        UCk_Utils_Ability_UE::Request_TransferExisting_Initiate(AbilityToTransfer, RequestTransferExisting);
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

                auto RequestGive = ck::FFragment_Ability_RequestGive{AbilityOwnerEntity, AbilitySource, OptionalPayload};
                // not adding the tag FTag_AbilityOwner_PendingSubAbilityOperation here since it is being added below in the if statements
                InRequest.Request_TransferHandleToOther(RequestGive);
                UCk_Utils_Ability_UE::Request_GiveAbility(AbilityEntity, RequestGive, ECk_AbilityRequest_PendingSubabilityPolicy::DontAddTag);
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

                    if (InRequest.Get_ConstructionPhase() == ECk_ConstructionPhase::AfterConstruction)
                    {
                        InAbilityOwnerEntity.Add<FTag_AbilityOwner_PendingSubAbilityOperation>();
                    }
                    else
                    {
                        CK_TRIGGER_ENSURE_IF(NOT InAbilityOwnerEntity.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
                            TEXT("Expected AbilityOwner [{}] to have PendingSubAbilities tag before adding sub-abilities"), InAbilityOwnerEntity)
                        { }
                    }
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
                if (InRequest.Get_ConstructionPhase() == ECk_ConstructionPhase::AfterConstruction)
                {
                    InAbilityOwnerEntity.Add<FTag_AbilityOwner_PendingSubAbilityOperation>();
                }
                else
                {
                    CK_TRIGGER_ENSURE_IF(NOT InAbilityOwnerEntity.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
                        TEXT("Expected AbilityOwner [{}] to have PendingSubAbilities tag before adding sub-abilities"), InAbilityOwnerEntity)
                    { }
                }
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
            if (InRequest.Get_ConstructionPhase() == ECk_ConstructionPhase::DuringConstruction)
            {
                InAbilityOwnerEntity.Add<FTag_AbilityOwner_RemovePendingSubAbilityOperation>();
            }

            UUtils_Signal_AbilityOwner_OnAbilityGivenOrNot::Broadcast(
                InRequest.GetAndDestroyRequestHandle(),
                MakePayload(InAbilityOwnerEntity, FCk_Handle_Ability{}, InRequest.Get_OptionalPayload(), ECk_AbilityOwner_AbilityGivenOrNot::NotGiven));
        }
    }

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
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

                auto RequestGive = ck::FFragment_Ability_RequestGive{AbilityOwnerEntity, AbilitySource, {}};
                InRequest.Request_TransferHandleToOther(RequestGive);
                UCk_Utils_Ability_UE::Request_AddAndGiveAbility(AbilityEntity, RequestGive);
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
                InRequest.GetAndDestroyRequestHandle(), MakePayload(InAbilityOwnerEntity, FCk_Handle_Ability{}, FCk_Ability_Payload_OnGranted{},
                    ECk_AbilityOwner_AbilityGivenOrNot::NotGiven));
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_HandleRequests::
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
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

                InCurrent.RemoveTags(InAbilityOwnerEntity, GrantedTags);
            }

            auto RequestRevoke = ck::FFragment_Ability_RequestRevoke{InAbilityOwnerEntity, InRequest.Get_DestructionPolicy()};
            InRequest.Request_TransferHandleToOther(RequestRevoke);
            UCk_Utils_Ability_UE::Request_RevokeAbility(InAbilityEntity, RequestRevoke);
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
            FFragment_AbilityOwner_Current& InCurrent,
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
                        MakePayload(InAbilityOwnerEntity, InAbilityToActivateEntity, InRequest.Get_OptionalPayload(), ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_AbilityNotFound));
                }

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
                        MakePayload(InAbilityOwnerEntity, InAbilityToActivateEntity, InRequest.Get_OptionalPayload(), ECk_AbilityOwner_AbilityActivatedOrNot::NotActivated_FailedChecks));
                }
                return;
            }

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
                        { DoHandleRequest(InAbilityOwnerEntity, InCurrent, FCk_Request_AbilityOwner_DeactivateAbility{InAbilityToActivateEntity}); }

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

                auto RequestActivate = ck::FFragment_Ability_RequestActivate{InAbilityOwnerEntity, InRequest.Get_OptionalPayload()};
                InRequest.Request_TransferHandleToOther(RequestActivate);
                UCk_Utils_Ability_UE::Request_ActivateAbility(InAbilityToActivateEntity, RequestActivate);

                return ECk_AbilityOwner_AbilityActivatedOrNot::Activated;
            }();
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
            FFragment_AbilityOwner_Current& InCurrent,
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

            auto RequestDeactivate = ck::FFragment_Ability_RequestDeactivate{InAbilityOwnerEntity};
            InRequest.Request_TransferHandleToOther(RequestDeactivate);
            UCk_Utils_Ability_UE::Request_DeactivateAbility(InAbilityEntity, RequestDeactivate);
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
            FFragment_AbilityOwner_Current& InCurrent,
            const FCk_Request_AbilityOwner_CancelSubAbilities& InRequest) const
        -> void
    {
        UCk_Utils_AbilityOwner_UE::ForEach_Ability_If
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

                DoHandleRequest(InAbilityOwnerEntity, InCurrent, FCk_Request_AbilityOwner_DeactivateAbility{InAbilityEntityToCancel});
            },
            [&](const FCk_Handle_Ability& InAbilityEntityToCancel)
            {
                // Only cancel if directly owned ability, ignore abilities owned only by extension
                return UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAbilityEntityToCancel) == InAbilityOwnerEntity;
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
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_AbilityOwner_Current& InCurrent) const
        -> void
    {
        // Purposefully not in tick since this tag could be added to an entity during the tick so we shouldn't clear it from all entities
        InHandle.Remove<MarkedDirtyBy>();

        const auto& ActiveTags = InCurrent.Get_ActiveTags(InHandle);

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

        if (InCurrent.Get_AreActiveTagsDifferentFromPreviousTags(InHandle))
        {
            const auto& PreviousTags = InCurrent.Get_PreviousTags(InHandle);

            auto ActiveTagsList = TArray<FGameplayTag>{};
            auto PreviousTagsList = TArray<FGameplayTag>{};

            ActiveTags.GetGameplayTagArray(ActiveTagsList);
            PreviousTags.GetGameplayTagArray(PreviousTagsList);

            const auto& TagsAdded = FGameplayTagContainer::CreateFromArray(algo::Except(ActiveTagsList, PreviousTagsList));
            const auto& TagsRemoved = FGameplayTagContainer::CreateFromArray(algo::Except(PreviousTagsList, ActiveTagsList));

            UUtils_Signal_AbilityOwner_OnTagsUpdated::Broadcast(InHandle,
                MakePayload(InHandle, PreviousTags, ActiveTags, TagsRemoved, TagsAdded));
        }

        InCurrent.UpdatePreviousTags();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_ResolvePendingOperationTags::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FTag_AbilityOwner_RemovePendingSubAbilityOperation& InCountedTag) const
            -> void
    {
        auto NumPendingOperations = InCountedTag.Get_Count();

        while (NumPendingOperations --> 0)
        {
            InHandle.Remove<FTag_AbilityOwner_PendingSubAbilityOperation>();
            InHandle.Remove<FTag_AbilityOwner_RemovePendingSubAbilityOperation>();
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_ResolvePendingOperationTags_DEBUG::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FTag_AbilityOwner_PendingSubAbilityOperation& InCountedTag) const
            -> void
    {
        ability::Log(TEXT("AbilityOwner [{}] has [{}] Pending Sub-Ability Operations"), InHandle, InCountedTag.Get_Count());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AbilityOwner_DeferClientRequestUntilReady::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            FFragment_AbilityOwner_Current& InCurrent,
            FFragment_AbilityOwner_DeferredClientRequests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InHandle.Remove<MarkedDirtyBy>();

        algo::ForEach(RequestsCopy, ck::Visitor([&](const auto& InRequestVariant)
        {
            DoHandleRequest(InHandle, InCurrent, InRequestVariant);

            // TODO: Add formatter for each request and track which one was responsible for destroying entity
        }));

        if (ck::Is_NOT_Valid(InHandle))
        {
            ability::Verbose(TEXT("Ability Entity [{}] was Marked Pending Kill during the handling of its requests."), InHandle);
            return;
        }
    }

    auto
        FProcessor_AbilityOwner_DeferClientRequestUntilReady::
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
            const FCk_Request_AbilityOwner_TransferExistingAbility& InRequest) const
        -> void
    {
        using RecordOfAbilities_Utils = UCk_Utils_AbilityOwner_UE::RecordOfAbilities_Utils;

        if (ck::Is_NOT_Valid(UCk_Utils_Ability_UE::Cast(InRequest.Get_Ability())))
        {
            UCk_Utils_AbilityOwner_UE::Request_TransferExistingAbility_DeferUntilReadyOnClient(InAbilityOwnerEntity, InRequest);
            return;
        }

        if (ck::Is_NOT_Valid(UCk_Utils_AbilityOwner_UE::Cast(InRequest.Get_TransferTarget())))
        {
            UCk_Utils_AbilityOwner_UE::Request_TransferExistingAbility_DeferUntilReadyOnClient(InAbilityOwnerEntity, InRequest);
            return;
        }

        if (NOT RecordOfAbilities_Utils::Get_ContainsEntry(InAbilityOwnerEntity, InRequest.Get_Ability()))
        {
            UCk_Utils_AbilityOwner_UE::Request_TransferExistingAbility_DeferUntilReadyOnClient(InAbilityOwnerEntity, InRequest);
            return;
        }

        UCk_Utils_AbilityOwner_UE::Request_TransferExistingAbility(InAbilityOwnerEntity, InRequest, {});
    }

    auto
        FProcessor_AbilityOwner_DeferClientRequestUntilReady::
        DoHandleRequest(
            HandleType& InAbilityOwnerEntity,
            FFragment_AbilityOwner_Current& InCurrent,
            const FCk_Request_AbilityOwner_RevokeAbility& InRequest) const
        -> void
    {
        using RecordOfAbilities_Utils = UCk_Utils_AbilityOwner_UE::RecordOfAbilities_Utils;

        CK_ENSURE_IF_NOT(InRequest.Get_SearchPolicy() == ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle,
            TEXT("Waiting for RevokeAbility Client request to be fully replicated on [{}], but it is a request that use the Search Policy [{}] instead of [{}]!"),
            InAbilityOwnerEntity, InRequest.Get_SearchPolicy(), ECk_AbilityOwner_AbilitySearch_Policy::SearchByHandle)
        {
            UCk_Utils_AbilityOwner_UE::Request_RevokeAbility(InAbilityOwnerEntity, InRequest, {});
            return;
        }

        if (ck::Is_NOT_Valid(UCk_Utils_Ability_UE::Cast(InRequest.Get_AbilityHandle())))
        {
            UCk_Utils_AbilityOwner_UE::Request_RevokeAbility_DeferUntilReadyOnClient(InAbilityOwnerEntity, InRequest);
            return;
        }

        if (NOT RecordOfAbilities_Utils::Get_ContainsEntry(InAbilityOwnerEntity, InRequest.Get_AbilityHandle()))
        {
            UCk_Utils_AbilityOwner_UE::Request_RevokeAbility_DeferUntilReadyOnClient(InAbilityOwnerEntity, InRequest);
            return;
        }

        UCk_Utils_AbilityOwner_UE::Request_RevokeAbility(InAbilityOwnerEntity, InRequest, {});
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

        UCk_Utils_AbilityOwner_UE::ForEach_Ability_If(InHandle, [&](FCk_Handle_Ability InAbilityHandle)
        {
            UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(InHandle, FCk_Request_AbilityOwner_DeactivateAbility{InAbilityHandle}, {});
            UCk_Utils_AbilityOwner_UE::Request_RevokeAbility(InHandle, FCk_Request_AbilityOwner_RevokeAbility{InAbilityHandle}, {});
        },
        [&](FCk_Handle_Ability InAbilityHandle)
        {
            return UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAbilityHandle) == InHandle;
        });
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------