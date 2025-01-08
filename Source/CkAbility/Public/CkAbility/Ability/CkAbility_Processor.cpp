#include "CkAbility_Processor.h"

#include "CkAbility_Script.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"
#include "CkAbility/Settings/CkAbility_Settings.h"
#include "CkAbility/Subsystem/CkAbility_Subsystem.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Ability_AddReplicated::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FCk_EntityReplicationDriver_AbilityData& InReplicatedAbility) const
            -> void
    {
        auto AbilityOwner = UCk_Utils_AbilityOwner_UE::Cast(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle));

        // it is possible that the AbilityOwner is NOT replicated yet
        if (ck::Is_NOT_Valid(AbilityOwner))
        {
            return;
        }

        auto AbilityScriptClass = TSubclassOf<UCk_Ability_Script_PDA>{InReplicatedAbility.Get_AbilityScriptClass()};

        CK_ENSURE_IF_NOT(ck::IsValid(AbilityScriptClass),
            TEXT(
                "Expected a valid AbilityScriptClass for Entity [{}]. This means that either the AbilityScriptClass was always nullptr OR "
                "the AbilityScriptClass is not network stable"), InHandle)
        {
            return;
        }

        UCk_Utils_AbilityOwner_UE::Request_GiveReplicatedAbility(AbilityOwner,
            FCk_Request_AbilityOwner_GiveReplicatedAbility{
                AbilityScriptClass,
                InHandle,
                InReplicatedAbility.Get_AbilitySource()
            }
            .Set_ConstructionPhase(InReplicatedAbility.Get_AbilityConstructionPhase()));

        InHandle.Remove<FCk_EntityReplicationDriver_AbilityData>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Ability_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Ability_Requests& InAbilityRequests) const
            -> void
    {
        const auto RequestsCopy = InAbilityRequests.Get_Requests();
        InHandle.Remove<MarkedDirtyBy>();

        algo::ForEach(RequestsCopy, ck::Visitor([&](
            const auto& InRequestVariant)
            {
                DoHandleRequest(InHandle, InRequestVariant);
            }));

        if (ck::Is_NOT_Valid(InHandle))
        {
            ability::Verbose(TEXT("Ability Entity [{}] was Marked Pending Kill during the handling of its requests."),
                InHandle);
        }
    }

    auto
        FProcessor_Ability_HandleRequests::
        DoHandleRequest(
            HandleType& InAbilityEntity,
            const FFragment_Ability_RequestAddAndGive& InRequest) const
            -> void
    {
        using RecordOfAbilities_Utils = UCk_Utils_AbilityOwner_UE::RecordOfAbilities_Utils;

        auto AbilityOwnerEntity = InRequest.Get_AbilityOwner();

        CK_ENSURE(AbilityOwnerEntity.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
            TEXT("AbilityOwner [{}] does NOT have Pending Operations tag"), AbilityOwnerEntity);

        //AbilityOwnerEntity.Add<ck::FTag_AbilityOwner_RemovePendingSubAbilityOperation>();
        //AbilityOwnerEntity.AddOrGet<DEBUG_PendingSubAbilityOperations>()._Abilities.Emplace(std::make_pair(TEXT("AddAndGive"), InAbilityEntity));

        // --------------------------------------------------------------------------------------------------------------------

        CK_ENSURE_IF_NOT(NOT RecordOfAbilities_Utils::Get_ContainsEntry(AbilityOwnerEntity, InAbilityEntity),
            TEXT("Cannot ADD and GIVE Ability [{}] to Ability Owner [{}] because it already has this Ability"),
            InAbilityEntity, AbilityOwnerEntity)
        { return; }

        const auto& CurrentOwnerOfAbilityToAddAndGive = UCk_Utils_Ability_UE::TryGet_Owner(InAbilityEntity);

        CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(CurrentOwnerOfAbilityToAddAndGive),
            TEXT("Cannot ADD and GIVE Ability [{}] to Ability Owner [{}] because it still belongs to Ability Owner [{}]"),
            InAbilityEntity, AbilityOwnerEntity, CurrentOwnerOfAbilityToAddAndGive)
        { return; }

        DoHandleRequest(InAbilityEntity, FFragment_Ability_RequestGive{AbilityOwnerEntity, InRequest.Get_AbilitySource(), InRequest.Get_Payload()});
    }

    auto
        FProcessor_Ability_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Ability_RequestGive& InRequest) const
            -> void
    {
        auto AbilityOwnerEntity = InRequest.Get_AbilityOwner();

        CK_ENSURE(AbilityOwnerEntity.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
            TEXT("AbilityOwner [{}] does NOT have Pending Operations tag"), AbilityOwnerEntity);

        AbilityOwnerEntity.Add<ck::FTag_AbilityOwner_RemovePendingSubAbilityOperation>();
        AbilityOwnerEntity.AddOrGet<DEBUG_PendingSubAbilityOperations>()._Abilities.Emplace(std::make_pair(TEXT("RequestGive"), InHandle));

        // --------------------------------------------------------------------------------------------------------------------

        using RecordOfAbilities_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAbilities>;
        using AbilitySource_Utils = ck::TUtils_EntityHolder<ck::FFragment_Ability_Source>;

        RecordOfAbilities_Utils::Request_Connect(AbilityOwnerEntity, InHandle);
        AbilitySource_Utils::AddOrReplace(InHandle, AbilityOwnerEntity);

        const auto Script = InHandle.Get<ck::FFragment_Ability_Current>().Get_AbilityScript().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT("AbilityScript for Handle [{}] with AbilityOwner [{}] is INVALID. Unable to GIVE the Ability properly"
            ),
            InHandle,
            AbilityOwnerEntity)
        {
            return;
        }

        Script->_AbilityOwnerHandle = AbilityOwnerEntity;
        Script->_AbilityHandle = InHandle;

        Script->OnGiveAbility(InRequest.Get_Payload());

        // --------------------------------------------------------------------------------------------------------------------

        if (InRequest.Get_IsRequestHandleValid())
        {
            UUtils_Signal_AbilityOwner_OnAbilityGivenOrNot::Broadcast(
                InRequest.GetAndDestroyRequestHandle(),
                MakePayload(AbilityOwnerEntity, InHandle, ECk_AbilityOwner_AbilityGivenOrNot::Given));
        }

        UUtils_Signal_AbilityOwner_OnAbilityGiven::Broadcast(
            AbilityOwnerEntity,
            MakePayload(AbilityOwnerEntity, InHandle));

        if (const auto& ActivationPolicy = UCk_Utils_Ability_UE::Get_ActivationSettings(InHandle).
            Get_ActivationPolicy(); ActivationPolicy == ECk_Ability_Activation_Policy::ActivateOnGranted)
        {
            UCk_Utils_AbilityOwner_UE::Request_TryActivateAbility(
                AbilityOwnerEntity,
                FCk_Request_AbilityOwner_ActivateAbility{InHandle}
                .Set_OptionalPayload(FCk_Ability_Payload_OnActivate{}.Set_ContextEntity(AbilityOwnerEntity)),
                {});
        }
    }

    auto
        FProcessor_Ability_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Ability_RequestRevoke& InRequest) const
            -> void
    {
        using RecordOfAbilities_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAbilities>;

        auto AbilityOwnerEntity = InRequest.Get_AbilityOwner();

        CK_ENSURE(AbilityOwnerEntity.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
            TEXT("AbilityOwner [{}] does NOT have Pending Operations tag"), AbilityOwnerEntity);

        AbilityOwnerEntity.Add<ck::FTag_AbilityOwner_RemovePendingSubAbilityOperation>();
        AbilityOwnerEntity.AddOrGet<DEBUG_PendingSubAbilityOperations>()._Abilities.Emplace(std::make_pair(TEXT("RequestRevoke"), InHandle));

        const auto DestructionPolicy = InRequest.Get_DestructionPolicy();

        // --------------------------------------------------------------------------------------------------------------------

        const auto& Current = InHandle.Get<ck::FFragment_Ability_Current>();
        if (Current.Get_Status() == ECk_Ability_Status::Active)
        {
            UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(AbilityOwnerEntity,
                FCk_Request_AbilityOwner_DeactivateAbility{InHandle}, {});
            UCk_Utils_AbilityOwner_UE::Request_RevokeAbility(AbilityOwnerEntity,
                FCk_Request_AbilityOwner_RevokeAbility{InHandle}.Set_DestructionPolicy(DestructionPolicy), {});

            return;
        }

        const auto Script = Current.Get_AbilityScript().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT(
                "AbilityScript for Handle [{}] with AbilityOwner [{}] is INVALID. Unable to REVOKE the Ability properly"
            ),
            InHandle,
            AbilityOwnerEntity)
        {
            return;
        }

        Script->OnRevokeAbility();
        Script->_AbilityOwnerHandle = {};

        if (RecordOfAbilities_Utils::Get_ContainsEntry(AbilityOwnerEntity, InHandle))
        {
            RecordOfAbilities_Utils::Request_Disconnect(AbilityOwnerEntity, InHandle);
        }

        // NOTE: Because abilities can be granted through Entity Extensions, only proceed with Ability destruction if
        // the Ability was granted to the Ability Owner directly and NOT by extension (which means the Ability Owner
        // is also the lifetime owner of the Ability)
        if (DestructionPolicy == ECk_AbilityOwner_DestructionOnRevoke_Policy::DestroyOnRevoke &&
            UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle) == AbilityOwnerEntity)
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);

            const auto CurrentWorld = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

            CK_ENSURE_IF_NOT(ck::IsValid(CurrentWorld), TEXT("Invalid World for Ability Entity [{}]"), InHandle)
            {
                return;
            }

            UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(CurrentWorld)->Request_UntrackAbilityScript(Script);
            UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(CurrentWorld)->Request_UntrackAbilityScript(
                Current.Get_AbilityScript_DefaultInstance().Get());
        }

        // --------------------------------------------------------------------------------------------------------------------

        if (InRequest.Get_IsRequestHandleValid())
        {
            UUtils_Signal_AbilityOwner_OnAbilityRevokedOrNot::Broadcast(
                InRequest.GetAndDestroyRequestHandle(),
                MakePayload(AbilityOwnerEntity, InHandle, ECk_AbilityOwner_AbilityRevokedOrNot::Revoked));
        }

        UUtils_Signal_AbilityOwner_OnAbilityRevoked::Broadcast(
            AbilityOwnerEntity,
            MakePayload(AbilityOwnerEntity, InHandle));
    }

    auto
        FProcessor_Ability_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Ability_RequestActivate& InRequest) const
            -> void
    {
        auto AbilityOwnerEntity = InRequest.Get_AbilityOwner();

        CK_ENSURE(AbilityOwnerEntity.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
            TEXT("AbilityOwner [{}] does NOT have Pending Operations tag"), AbilityOwnerEntity);

        AbilityOwnerEntity.Add<ck::FTag_AbilityOwner_RemovePendingSubAbilityOperation>();
        AbilityOwnerEntity.AddOrGet<DEBUG_PendingSubAbilityOperations>()._Abilities.Emplace(std::make_pair(TEXT("RequestActivate"), InHandle));

        // --------------------------------------------------------------------------------------------------------------------

        auto& AbilityCurrent = InHandle.Get<ck::FFragment_Ability_Current>();
        const auto Script = AbilityCurrent.Get_AbilityScript().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT("Cannot Activate Ability with Entity [{}] because its AbilityScript is INVALID"),
            InHandle)
        {
            return;
        }

        if (AbilityCurrent.Get_Status() == ECk_Ability_Status::Active)
        {
            return;
        }

        const auto& AbilityParams = InHandle.Get<ck::FFragment_Ability_Params>().Get_Params();
        const auto& AbilityInstancingPolicy = AbilityParams.Get_Data().Get_InstancingPolicy();
        AbilityCurrent._Status = ECk_Ability_Status::Active;

        CK_ENSURE_IF_NOT([&]
            {
            if (AbilityInstancingPolicy != ECk_Ability_InstancingPolicy::InstancedPerAbilityActivation)
            { return true; }

            const auto IsSameAbilityEntity = Script->_AbilityHandle == InHandle;
            const auto IsSameAbilityOwnerEntity = Script->_AbilityOwnerHandle == AbilityOwnerEntity;

            return IsSameAbilityEntity && IsSameAbilityOwnerEntity;
            }(),

            TEXT(
                "Ability Script [{}] that is NOT a CDO was GIVEN with {} [{}] and {} [{}] but ACTIVATED with Ability [{}] and Ability Owner [{}]\n"
                "This is supported/expected on a CDO, but NOT for instanced Abilities"),
            Script,
            ck::IsValid(Script->_AbilityHandle) ? TEXT("VALID Ability") : TEXT("INVALID Ability"),
            Script->_AbilityHandle,
            ck::IsValid(Script->_AbilityOwnerHandle) ? TEXT("VALID Ability Owner") : TEXT("INVALID Ability Owner"),
            Script->_AbilityOwnerHandle,
            InHandle,
            AbilityOwnerEntity) {}

        // It is possible that between Give and Activate, the Ability Owning & Handle were changed
        // if the Ability is a CDO AND multiple activate/deactivate requests happened in the same frame.
        // This is partly due to the fact that we process deactivation requests immediately after activation
        // IFF activation + deactivation happened in the same frame
        Script->_AbilityHandle = InHandle;
        Script->_AbilityOwnerHandle = AbilityOwnerEntity;

        Script->OnActivateAbility(InRequest.Get_Payload());

        ck::UUtils_Signal_OnAbilityActivated::Broadcast(InHandle, ck::MakePayload(InHandle, InRequest.Get_Payload()));

        // --------------------------------------------------------------------------------------------------------------------

        if (InRequest.Get_IsRequestHandleValid())
        {
            UUtils_Signal_AbilityOwner_OnAbilityActivatedOrNot::Broadcast(
                InRequest.GetAndDestroyRequestHandle(),
                MakePayload(AbilityOwnerEntity, InHandle, ECk_AbilityOwner_AbilityActivatedOrNot::Activated));
        }

        UUtils_Signal_AbilityOwner_OnAbilityActivated::Broadcast(
            AbilityOwnerEntity,
            MakePayload(AbilityOwnerEntity, InHandle));
    }

    auto
        FProcessor_Ability_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Ability_RequestDeactivate& InRequest) const
            -> void
    {
        auto AbilityOwnerEntity = InRequest.Get_AbilityOwner();

        CK_ENSURE(AbilityOwnerEntity.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
            TEXT("AbilityOwner [{}] does NOT have Pending Operations tag"), AbilityOwnerEntity);

        AbilityOwnerEntity.Add<ck::FTag_AbilityOwner_RemovePendingSubAbilityOperation>();
        AbilityOwnerEntity.AddOrGet<DEBUG_PendingSubAbilityOperations>()._Abilities.Emplace(std::make_pair(TEXT("RequestDeactivate"), InHandle));

        // --------------------------------------------------------------------------------------------------------------------

        auto& AbilityCurrent = InHandle.Get<ck::FFragment_Ability_Current,
            ck::IsValid_Policy_IncludePendingKill>();
        const auto& AbilityParams = InHandle.Get<ck::FFragment_Ability_Params,
            ck::IsValid_Policy_IncludePendingKill>().Get_Params();
        auto Script = AbilityCurrent.Get_AbilityScript().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT("Cannot Deactivate Ability with Entity [{}] because its AbilityScript is INVALID"),
            InHandle)
        {
            return;
        }

        if (AbilityCurrent.Get_Status() == ECk_Ability_Status::NotActive)
        {
            return;
        }

        AbilityCurrent._Status = ECk_Ability_Status::NotActive;
        Script->OnDeactivateAbility();

        ck::UUtils_Signal_OnAbilityDeactivated::Broadcast(InHandle, ck::MakePayload(InHandle));

        if (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle))
        {
            return;
        }

        // NOTE: If we reset the ability script properties on DEACTIVATE, we are potentially doing a cleanup for nothing
        // if the ability is not activated again. If we do it on ACTIVATE then we are going to perform a cleanup for nothing
        // the very first time it is activated
        if (AbilityParams.Get_Data().Get_InstancingPolicy() ==
            ECk_Ability_InstancingPolicy::InstancedPerAbilityActivation)
        {
            switch (const auto& RecyclingPolicy = UCk_Utils_Ability_Settings_UE::Get_AbilityRecyclingPolicy())
            {
                case ECk_Ability_RecyclingPolicy::Recycle:
                {
                    ck::ability::VeryVerbose
                    (
                        TEXT("Recycling Ability Script [{}] with Entity [{}] for Ability Owner [{}]"),
                        Script,
                        InHandle,
                        AbilityOwnerEntity
                    );

                    UCk_Utils_Object_UE::Request_CopyAllProperties(FCk_Utils_Object_CopyAllProperties_Params{}
                                                                   .Set_Destination(Script)
                                                                   .Set_Source(
                                                                       AbilityCurrent.
                                                                       Get_AbilityScript_DefaultInstance().Get()));

                    break;
                }
                case ECk_Ability_RecyclingPolicy::DoNotRecycle:
                {
                    ck::ability::VeryVerbose
                    (
                        TEXT("Instancing new Ability Script [{}] with Entity [{}] for Ability Owner [{}]"),
                        Script,
                        InHandle,
                        AbilityOwnerEntity
                    );

                    const auto& AbilityScriptClass = AbilityParams.Get_AbilityScriptClass();

                    UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(AbilityCurrent.Get_AbilityScript()->GetWorld())->
                        Request_UntrackAbilityScript(AbilityCurrent.Get_AbilityScript().Get());

                    Script = UCk_Utils_Object_UE::Request_CreateNewObject<UCk_Ability_Script_PDA>
                    (
                        UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(AbilityOwnerEntity),
                        AbilityScriptClass,
                        AbilityCurrent.Get_AbilityScript_DefaultInstance().Get(),
                        nullptr
                    );

                    UCk_Utils_Ability_Subsystem_UE::Get_Subsystem(AbilityCurrent.Get_AbilityScript()->GetWorld())->
                        Request_TrackAbilityScript(AbilityCurrent.Get_AbilityScript().Get());

                    break;
                }
                default:
                {
                    CK_INVALID_ENUM(RecyclingPolicy);
                    return;
                }
            }

            if (ck::IsValid(Script))
            {
                Script->_AbilityHandle = InHandle;
                Script->_AbilityOwnerHandle = AbilityOwnerEntity;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Ability_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_Ability_Current& InCurrent) const
            -> void
    {
        // there is nothing to teardown if the Ability is already Inactive
        if (InCurrent.Get_Status() == ECk_Ability_Status::NotActive)
        {
            return;
        }

        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto AbilityOwner = UCk_Utils_AbilityOwner_UE::CastChecked(LifetimeOwner);

        ability::Verbose
        (
            TEXT("FORCE DEACTIVATING Ability [Name: {} | Entity: {}] with AbilityOwner [{}] that is {}"),
            UCk_Utils_GameplayLabel_UE::Get_Label(InHandle), InHandle, AbilityOwner,
            ck::IsValid(LifetimeOwner) ? TEXT("VALID") : TEXT("PENDING DESTROY")
        );

        UCk_Utils_Ability_UE::DoDeactivate(AbilityOwner, InHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------
