#include "CkAbility_Processor.h"

#include "CkAbility_Script.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Fragment.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Processor.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"
#include "CkAbility/Settings/CkAbility_Settings.h"
#include "CkAbility/Subsystem/CkAbility_Subsystem.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Fragment to store subabilities during transfer since we can't access them through ForEach_Ability once revoked
    struct CKABILITY_API FFragment_Ability_RevokedSubAbilitiesBeingTransferred
    {
    public:
        CK_GENERATED_BODY(FFragment_Ability_RevokedSubAbilitiesBeingTransferred);

        friend class FProcessor_Ability_HandleRequests;

    private:
        TArray<FCk_Handle_Ability> _RevokedSubAbilitiesBeingTransferred;
    };

    auto
        FProcessor_Ability_AddReplicated::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FCk_EntityReplicationDriver_AbilityData& InReplicatedAbility) const
            -> void
    {
        auto AbilityOwner = UCk_Utils_AbilityOwner_UE::Cast(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle));

        // It is possible that the AbilityOwner is NOT replicated yet
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

        auto ContinueProcessingRequests = EAbilityProcessor_ForEachRequestResult::Continue;
        algo::ForEach(RequestsCopy, ck::Visitor([&](
            const auto& InRequestVariant)
            {
                if (ContinueProcessingRequests != EAbilityProcessor_ForEachRequestResult::Continue)
                { return; }

                ContinueProcessingRequests = DoHandleRequest(InHandle, InRequestVariant);
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
            const FFragment_Ability_RequestAddAndGive& InRequest)
            -> EAbilityProcessor_ForEachRequestResult
    {
        using RecordOfAbilities_Utils = UCk_Utils_AbilityOwner_UE::RecordOfAbilities_Utils;

        auto AbilityOwnerEntity = InRequest.Get_AbilityOwner();

        CK_ENSURE(AbilityOwnerEntity.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
            TEXT("AbilityOwner [{}] does NOT have Pending Operations tag"), AbilityOwnerEntity);

        // --------------------------------------------------------------------------------------------------------------------

        CK_ENSURE_IF_NOT(NOT RecordOfAbilities_Utils::Get_ContainsEntry(AbilityOwnerEntity, InAbilityEntity),
            TEXT("Cannot ADD and GIVE Ability [{}] to Ability Owner [{}] because it already has this Ability"),
            InAbilityEntity, AbilityOwnerEntity)
        {
            return EAbilityProcessor_ForEachRequestResult::Continue;
        }

        DoHandleRequest(InAbilityEntity, FFragment_Ability_RequestGive{
            AbilityOwnerEntity,
            InRequest.Get_AbilitySource(),
            InRequest.Get_Payload()
        });

        return EAbilityProcessor_ForEachRequestResult::Continue;
    }

    auto
        FProcessor_Ability_HandleRequests::
        DoHandleRequest(
            HandleType& InAbilityEntity,
            const FFragment_Ability_RequestTransferExisting_Initiate& InRequest)
            -> EAbilityProcessor_ForEachRequestResult
    {
        using RecordOfAbilities_Utils = UCk_Utils_AbilityOwner_UE::RecordOfAbilities_Utils;

        auto AbilityOwnerEntity = InRequest.Get_AbilityOwner();
        auto AbilityToTransfer = InRequest.Get_AbilityToTransfer();
        auto TransferTarget = InRequest.Get_TransferTarget();

        AbilityOwnerEntity.Add<ck::FTag_AbilityOwner_RemovePendingSubAbilityOperation>();

        CK_ENSURE_IF_NOT(InAbilityEntity == AbilityToTransfer,
            TEXT("Trying to initialize an ability transfer for [{}] on the processor of a different ability [{}]"),
            AbilityToTransfer, InAbilityEntity)
        { return EAbilityProcessor_ForEachRequestResult::Continue; }

        const auto& AbilityCanInitiateTransferOrNot = [&]() -> ECk_AbilityOwner_AbilityTransferredOrNot
        {
            CK_ENSURE_IF_NOT(ck::IsValid(AbilityToTransfer),
                TEXT("INVALID Ability to TRANSFER from Ability Owner [{}]"),
                AbilityOwnerEntity)
            {
                return ECk_AbilityOwner_AbilityTransferredOrNot::NotTransferred;
            }

            CK_ENSURE_IF_NOT(ck::IsValid(TransferTarget),
                TEXT("INVALID Target to TRANSFER Ability [{}] from Ability Owner [{}] to"),
                AbilityToTransfer, AbilityOwnerEntity)
            {
                return ECk_AbilityOwner_AbilityTransferredOrNot::NotTransferred;
            }

            CK_ENSURE_IF_NOT(RecordOfAbilities_Utils::Get_ContainsEntry(AbilityOwnerEntity, AbilityToTransfer),
                TEXT(
                    "Cannot TRANSFER Ability [{}] from Ability Owner [{}] to [{}] because it does NOT have such an ability"
                ),
                AbilityToTransfer, AbilityOwnerEntity, TransferTarget)
            {
                return ECk_AbilityOwner_AbilityTransferredOrNot::NotTransferred;
            }

            CK_ENSURE_IF_NOT(NOT RecordOfAbilities_Utils::Get_ContainsEntry(TransferTarget, AbilityToTransfer),
                TEXT(
                    "Cannot TRANSFER Ability [{}] from Ability Owner [{}] to [{}] because the recipient already has this ability"
                    "A request to remove the ability from the source was made, but the recipient somehow still has the ability (possibly by extension?)"
                ),
                AbilityToTransfer, AbilityOwnerEntity, TransferTarget)
            {
                return ECk_AbilityOwner_AbilityTransferredOrNot::NotTransferred;
            }

            if (TransferTarget == AbilityOwnerEntity)
            {
                return ECk_AbilityOwner_AbilityTransferredOrNot::NotTransferred;
            }

            return ECk_AbilityOwner_AbilityTransferredOrNot::Transferred;
        }();

        if (AbilityCanInitiateTransferOrNot == ECk_AbilityOwner_AbilityTransferredOrNot::NotTransferred)
        {
            if (InRequest.Get_IsRequestHandleValid())
            {
                UUtils_Signal_AbilityOwner_OnAbilityTransferredOrNot::Broadcast(
                    InRequest.GetAndDestroyRequestHandle(),
                    MakePayload(AbilityOwnerEntity, TransferTarget, AbilityToTransfer, AbilityCanInitiateTransferOrNot));
            }

            if (AbilityCanInitiateTransferOrNot == ECk_AbilityOwner_AbilityTransferredOrNot::Transferred)
            {
                UUtils_Signal_AbilityOwner_OnAbilityTransferred::Broadcast(
                    AbilityOwnerEntity,
                    MakePayload(AbilityOwnerEntity, TransferTarget, AbilityToTransfer));
            }
            return EAbilityProcessor_ForEachRequestResult::Continue;
        }

        auto AbilityAsOwner = UCk_Utils_AbilityOwner_UE::Cast(AbilityToTransfer);

        if (ck::IsValid(AbilityAsOwner))
        {
            auto& SubabilitiesFragment = AbilityToTransfer.Add<FFragment_Ability_RevokedSubAbilitiesBeingTransferred>();

            UCk_Utils_AbilityOwner_UE::ForEach_Ability_InOwnershipHierarchy(AbilityAsOwner, [&](FCk_Handle_Ability InLambdaAbilityHandle)
            {
                auto LambdaAbilityOwner = UCk_Utils_Ability_UE::TryGet_Owner(InLambdaAbilityHandle);

                LambdaAbilityOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
                InLambdaAbilityHandle.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
                    FFragment_Ability_RequestDeactivate{LambdaAbilityOwner});

                LambdaAbilityOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
                InLambdaAbilityHandle.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
                    FFragment_Ability_RequestRevoke{LambdaAbilityOwner, ECk_AbilityOwner_DestructionOnRevoke_Policy::DoNotDestroyOnRevoke});

                SubabilitiesFragment._RevokedSubAbilitiesBeingTransferred.Add(InLambdaAbilityHandle);
            });
        }

        AbilityOwnerEntity.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
        AbilityToTransfer.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
            FFragment_Ability_RequestDeactivate{AbilityOwnerEntity});

        AbilityOwnerEntity.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
        AbilityToTransfer.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
            FFragment_Ability_RequestRevoke{AbilityOwnerEntity, ECk_AbilityOwner_DestructionOnRevoke_Policy::DoNotDestroyOnRevoke});

        AbilityOwnerEntity.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
        AbilityToTransfer.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
            FFragment_Ability_RequestTransferExisting_SwapOwner{AbilityOwnerEntity, TransferTarget, AbilityToTransfer});

        return EAbilityProcessor_ForEachRequestResult::Continue;
    }

    auto
        FProcessor_Ability_HandleRequests::
        DoHandleRequest(
            HandleType& InAbilityEntity,
            const FFragment_Ability_RequestTransferExisting_SwapOwner& InRequest)
        -> EAbilityProcessor_ForEachRequestResult
    {
        using RecordOfAbilities_Utils = UCk_Utils_AbilityOwner_UE::RecordOfAbilities_Utils;

        auto AbilityOwnerEntity = InRequest.Get_AbilityOwner();
        auto AbilityToTransfer = InRequest.Get_AbilityToTransfer();
        auto TransferTarget = InRequest.Get_TransferTarget();

        AbilityOwnerEntity.Add<ck::FTag_AbilityOwner_RemovePendingSubAbilityOperation>();

        CK_ENSURE_IF_NOT(InAbilityEntity == AbilityToTransfer,
            TEXT("Trying to swap ability owner in an ability transfer for [{}] on the processor of a different ability [{}]"),
            AbilityToTransfer, InAbilityEntity)
        {return EAbilityProcessor_ForEachRequestResult::Continue; }

        UCk_Utils_EntityLifetime_UE::Request_TransferLifetimeOwner(AbilityToTransfer, TransferTarget);

        auto& AbilityCurrent = AbilityToTransfer.Get<ck::FFragment_Ability_Current,
            ck::IsValid_Policy_IncludePendingKill>();
        auto Script = AbilityCurrent.Get_AbilityScript().Get();
        Script->_AbilityOwnerHandle = TransferTarget;

        // Clear all existing ability and ability owner requests. This makes sense since these requests were made assuming
        // it was under the previous ability owner, but it doesn't make sense to handle them once the ability owner changes
        auto ClearAllAbilityAndOwnerRequests = [](FCk_Handle_Ability& InAbilityHandle)
        {
            auto& AbilityRequests = InAbilityHandle.AddOrGet<ck::FFragment_Ability_Requests>()._Requests;
            for (const auto& Request : AbilityRequests)
            {
                // Even though we are clearing all requests, we need to remove the pending subabilities tag
                InAbilityHandle.Add<ck::FTag_AbilityOwner_RemovePendingSubAbilityOperation>();
            }
            AbilityRequests.Empty();

            if (auto AbilityAsOwner = UCk_Utils_AbilityOwner_UE::Cast(InAbilityHandle);
                ck::IsValid(AbilityAsOwner))
            {
                AbilityAsOwner.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Empty();
            }
        };

        ClearAllAbilityAndOwnerRequests(AbilityToTransfer);

        if (auto AbilityAsOwner = UCk_Utils_AbilityOwner_UE::Cast(AbilityToTransfer);
            ck::IsValid(AbilityAsOwner))
        {
            // Clear all ability owner requests as well for the same reason as clearing the ability requests
            AbilityAsOwner.AddOrGet<ck::FFragment_AbilityOwner_Requests>()._Requests.Empty();

            CK_ENSURE_IF_NOT(AbilityToTransfer.Has<FFragment_Ability_RevokedSubAbilitiesBeingTransferred>(),
                TEXT("Transferred ability [{}] does NOT have a framgment for revoked subabilities being transferred!"), AbilityToTransfer)
            {
                AbilityToTransfer.Add<FFragment_Ability_RevokedSubAbilitiesBeingTransferred>();
            }
            auto& SubAbilitiesFragment = AbilityToTransfer.Get<FFragment_Ability_RevokedSubAbilitiesBeingTransferred>();

            for (auto SubAbility : SubAbilitiesFragment._RevokedSubAbilitiesBeingTransferred)
            {
                ClearAllAbilityAndOwnerRequests(SubAbility);

                auto SubAbilityOwner = UCk_Utils_AbilityOwner_UE::Cast(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(SubAbility));

                SubAbilityOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
                SubAbility.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
                    FFragment_Ability_RequestAddAndGive{SubAbilityOwner, SubAbility, {}});
            }

            AbilityToTransfer.Remove<FFragment_Ability_RevokedSubAbilitiesBeingTransferred>();
        }

        TransferTarget.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
        AbilityToTransfer.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
            FFragment_Ability_RequestAddAndGive{TransferTarget, AbilityToTransfer, {}});

        TransferTarget.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
        AbilityToTransfer.AddOrGet<ck::FFragment_Ability_Requests>()._Requests.Emplace(
            FFragment_Ability_RequestTransferExisting_Finalize{AbilityOwnerEntity, TransferTarget, AbilityToTransfer});

        // Since we cleared all ability requests, we don't want to continue processing the current requests in the pump
        return EAbilityProcessor_ForEachRequestResult::Stop;
    }

    auto
        FProcessor_Ability_HandleRequests::
        DoHandleRequest(
            HandleType& InAbilityEntity,
            const FFragment_Ability_RequestTransferExisting_Finalize& InRequest)
        -> EAbilityProcessor_ForEachRequestResult
    {
        using RecordOfAbilities_Utils = UCk_Utils_AbilityOwner_UE::RecordOfAbilities_Utils;

        auto OriginalAbilityOwnerEntity = InRequest.Get_AbilityOwner();
        auto AbilityToTransfer = InRequest.Get_AbilityToTransfer();
        auto TransferTarget = InRequest.Get_TransferTarget();

        TransferTarget.Add<ck::FTag_AbilityOwner_RemovePendingSubAbilityOperation>();

        CK_ENSURE_IF_NOT(InAbilityEntity == AbilityToTransfer,
            TEXT("Trying to finalize an ability transfer for [{}] on the processor of a different ability [{}]"),
            AbilityToTransfer, InAbilityEntity)
        { return EAbilityProcessor_ForEachRequestResult::Continue; }

        if (InRequest.Get_IsRequestHandleValid())
        {
            UUtils_Signal_AbilityOwner_OnAbilityTransferredOrNot::Broadcast(
                InRequest.GetAndDestroyRequestHandle(),
                MakePayload(OriginalAbilityOwnerEntity, TransferTarget, AbilityToTransfer, ECk_AbilityOwner_AbilityTransferredOrNot::Transferred));
        }

        UUtils_Signal_AbilityOwner_OnAbilityTransferred::Broadcast(
            OriginalAbilityOwnerEntity,
            MakePayload(OriginalAbilityOwnerEntity, TransferTarget, AbilityToTransfer));

        return EAbilityProcessor_ForEachRequestResult::Continue;
    }

    auto
        FProcessor_Ability_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Ability_RequestGive& InRequest)
            -> EAbilityProcessor_ForEachRequestResult
    {
        UCk_Utils_Ability_UE::Request_MarkAbility_AsGiven(InHandle);

        auto AbilityOwnerEntity = [&]() -> FCk_Handle_AbilityOwner
        {
            const auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
            auto OwnerToReturn = InRequest.Get_AbilityOwner();

            CK_ENSURE_IF_NOT(InRequest.Get_AbilityOwner() == LifetimeOwner,
                TEXT(
                    "AbilityOwner [{}] and LifetimeOwner [{}] for [{}] MUST be the same. Forcing the change but this WILL fail in Production!"
                ),
                OwnerToReturn, LifetimeOwner, InHandle)
            {
                OwnerToReturn = UCk_Utils_AbilityOwner_UE::Cast(LifetimeOwner);
            }

            CK_ENSURE(OwnerToReturn.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
                TEXT("AbilityOwner [{}] does NOT have Pending Operations tag"), OwnerToReturn);

            OwnerToReturn.Add<ck::FTag_AbilityOwner_RemovePendingSubAbilityOperation>();
            return OwnerToReturn;
        }();

        // --------------------------------------------------------------------------------------------------------------------

        using RecordOfAbilities_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAbilities>;
        using AbilitySource_Utils = ck::TUtils_EntityHolder<ck::FFragment_Ability_Source>;

        RecordOfAbilities_Utils::Request_Connect(AbilityOwnerEntity, InHandle);
        AbilitySource_Utils::AddOrReplace(InHandle, InRequest.Get_AbilitySource());

        const auto Script = InHandle.Get<ck::FFragment_Ability_Current>().Get_AbilityScript().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT("AbilityScript for Handle [{}] with AbilityOwner [{}] is INVALID. Unable to GIVE the Ability properly"
            ),
            InHandle,
            AbilityOwnerEntity)
        {
            return EAbilityProcessor_ForEachRequestResult::Continue;
        }

        Script->_AbilityOwnerHandle = AbilityOwnerEntity;
        Script->_AbilityHandle = InHandle;

        Script->OnGiveAbility(InRequest.Get_Payload());

        // --------------------------------------------------------------------------------------------------------------------

        if (InRequest.Get_IsRequestHandleValid())
        {
            UUtils_Signal_AbilityOwner_OnAbilityGivenOrNot::Broadcast(
                InRequest.GetAndDestroyRequestHandle(),
                MakePayload(AbilityOwnerEntity, InHandle, InRequest.Get_Payload(),
                    ECk_AbilityOwner_AbilityGivenOrNot::Given));
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

        return EAbilityProcessor_ForEachRequestResult::Continue;
    }

    auto
        FProcessor_Ability_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Ability_RequestRevoke& InRequest)
            -> EAbilityProcessor_ForEachRequestResult
    {
        using RecordOfAbilities_Utils = ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfAbilities>;

        CK_ENSURE_IF_NOT(UCk_Utils_Ability_UE::Get_IsAbilityGiven(InHandle),
            TEXT("Ability [{}] is trying to revoke when it is not given, likely because it has already been revoked"), InHandle)
        { return EAbilityProcessor_ForEachRequestResult::Continue; }

        auto AbilityOwnerEntity = [&]() -> FCk_Handle_AbilityOwner
        {
            const auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
            auto OwnerToReturn = InRequest.Get_AbilityOwner();

            CK_ENSURE_IF_NOT(InRequest.Get_AbilityOwner() == LifetimeOwner,
                TEXT("AbilityOwner [{}] and LifetimeOwner [{}] for [{}] MUST be the same. Forcing the change but this WILL fail in Production!"),
                OwnerToReturn, LifetimeOwner, InHandle)
            {
                OwnerToReturn = UCk_Utils_AbilityOwner_UE::Cast(LifetimeOwner);
            }

            CK_ENSURE(OwnerToReturn.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
                TEXT("AbilityOwner [{}] does NOT have Pending Operations tag"), OwnerToReturn);

            OwnerToReturn.Add<ck::FTag_AbilityOwner_RemovePendingSubAbilityOperation>();
            return OwnerToReturn;
        }();

        const auto DestructionPolicy = InRequest.Get_DestructionPolicy();

        // --------------------------------------------------------------------------------------------------------------------

        const auto& Current = InHandle.Get<ck::FFragment_Ability_Current>();
        if (Current.Get_Status() == ECk_Ability_Status::Active)
        {
            UCk_Utils_AbilityOwner_UE::Request_DeactivateAbility(AbilityOwnerEntity,
                FCk_Request_AbilityOwner_DeactivateAbility{InHandle}, {});
            UCk_Utils_AbilityOwner_UE::Request_RevokeAbility(AbilityOwnerEntity,
                FCk_Request_AbilityOwner_RevokeAbility{InHandle}.Set_DestructionPolicy(DestructionPolicy), {});

            return EAbilityProcessor_ForEachRequestResult::Continue;
        }

        const auto Script = Current.Get_AbilityScript().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT(
                "AbilityScript for Handle [{}] with AbilityOwner [{}] is INVALID. Unable to REVOKE the Ability properly"
            ),
            InHandle,
            AbilityOwnerEntity)
        {
            return EAbilityProcessor_ForEachRequestResult::Continue;
        }

        Script->OnRevokeAbility();
        Script->_AbilityOwnerHandle = {};
        Script->_ContextEntityWithActor.Reset();

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
                return EAbilityProcessor_ForEachRequestResult::Continue;
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

        UCk_Utils_Ability_UE::Request_MarkAbility_AsNotGiven(InHandle);

        return EAbilityProcessor_ForEachRequestResult::Continue;
    }

    auto
        FProcessor_Ability_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Ability_RequestActivate& InRequest)
            -> EAbilityProcessor_ForEachRequestResult
    {
        auto AbilityOwnerEntity = [&]() -> FCk_Handle_AbilityOwner
        {
            const auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
            auto OwnerToReturn = InRequest.Get_AbilityOwner();

            CK_ENSURE_IF_NOT(InRequest.Get_AbilityOwner() == LifetimeOwner,
                TEXT(
                    "AbilityOwner [{}] and LifetimeOwner [{}] for [{}] MUST be the same. Forcing the change but this WILL fail in Production!"
                ),
                OwnerToReturn, LifetimeOwner, InHandle)
            {
                OwnerToReturn = UCk_Utils_AbilityOwner_UE::Cast(LifetimeOwner);
            }

            CK_ENSURE(OwnerToReturn.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
                TEXT("AbilityOwner [{}] does NOT have Pending Operations tag"), OwnerToReturn);

            OwnerToReturn.Add<ck::FTag_AbilityOwner_RemovePendingSubAbilityOperation>();
            return OwnerToReturn;
        }();

        if (NOT UCk_Utils_Ability_UE::Get_IsAbilityGiven(InHandle))
        { return EAbilityProcessor_ForEachRequestResult::Continue; }

        // --------------------------------------------------------------------------------------------------------------------

        auto& AbilityCurrent = InHandle.Get<ck::FFragment_Ability_Current>();
        const auto Script = AbilityCurrent.Get_AbilityScript().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT("Cannot Activate Ability with Entity [{}] because its AbilityScript is INVALID"),
            InHandle)
        {
            return EAbilityProcessor_ForEachRequestResult::Continue;
        }

        if (AbilityCurrent.Get_Status() == ECk_Ability_Status::Active)
        {
            return EAbilityProcessor_ForEachRequestResult::Continue;
        }

        if (UCk_Utils_Ability_UE::Get_CanActivate(InHandle) != ECk_Ability_ActivationRequirementsResult::RequirementsMet)
        { return EAbilityProcessor_ForEachRequestResult::Continue; }

        const auto& AbilityParams = InHandle.Get<ck::FFragment_Ability_Params>();
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

        // --------------------------------------------------------------------------------------------------------------------

        auto& AbilityOwnerCurrent = AbilityOwnerEntity.Get<ck::FFragment_AbilityOwner_Current>();

        const auto& AbilityActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(InHandle);
        const auto& GrantedTags = AbilityActivationSettings.Get_ActivationSettingsOnOwner().
                                                            Get_GrantTagsOnAbilityOwner();

        AbilityOwnerCurrent.AppendTags(AbilityOwnerEntity, GrantedTags);

        // Try Deactivating AbilityOwner as an Ability IFF the AbilityOwner is also an Ability
        if (UCk_Utils_Ability_UE::Has(AbilityOwnerEntity))
        {
            if (const auto Condition = algo::MatchesAnyAbilityActivationCancelledTagsOnSelf{GrantedTags}; Condition(
                AbilityOwnerEntity))
            {
                auto MyOwner = UCk_Utils_AbilityOwner_UE::CastChecked(
                    UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(AbilityOwnerEntity));

                auto AbilityOwnerAsAbility = UCk_Utils_Ability_UE::CastChecked(AbilityOwnerEntity);
                MyOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
                DoHandleRequest(AbilityOwnerAsAbility, FFragment_Ability_RequestDeactivate{MyOwner});
            }
        }

        // Cancel All Abilities that are cancelled by the newly granted tags
        // TODO: this is repeated multiple times in this file, move to a common function
        // TODO: See if the new system TagsChanged can help replace this section of code
        UCk_Utils_AbilityOwner_UE::ForEach_Ability_If
        (
            AbilityOwnerEntity,
            [&](
            FCk_Handle_Ability InAbilityEntityToCancel)
            {
                ability::Verbose
                (
                    TEXT("CANCELLING Ability [{}] after Activating Ability [{}] on Ability Owner [{}]"),
                    InAbilityEntityToCancel,
                    InHandle,
                    AbilityOwnerEntity
                );

                CK_ENSURE_IF_NOT(InHandle != InAbilityEntityToCancel,
                    TEXT("Tags Granted [{}] on Ability Owner [{}] as part of the Activation of Ability [{}] are causing the Ability to Deactivated itself BEFORE it fully Activated!\n"
                         "This is unexpected is very likely due to a configuration error"),
                    GrantedTags,
                    AbilityOwnerEntity,
                    InHandle)
                { return; }

                AbilityOwnerEntity.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
                DoHandleRequest(InAbilityEntityToCancel, FFragment_Ability_RequestDeactivate{AbilityOwnerEntity});
            },
            algo::MatchesAnyAbilityActivationCancelledTagsOnOwner{GrantedTags}
        );

        // --------------------------------------------------------------------------------------------------------------------

        Script->OnActivateAbility(InRequest.Get_Payload());

        ck::UUtils_Signal_OnAbilityActivated::Broadcast(InHandle, ck::MakePayload(InHandle, InRequest.Get_Payload()));

        // --------------------------------------------------------------------------------------------------------------------

        if (InRequest.Get_IsRequestHandleValid())
        {
            UUtils_Signal_AbilityOwner_OnAbilityActivatedOrNot::Broadcast(
                InRequest.GetAndDestroyRequestHandle(),
                MakePayload(AbilityOwnerEntity, InHandle, InRequest.Get_Payload(),
                    ECk_AbilityOwner_AbilityActivatedOrNot::Activated));
        }

        UUtils_Signal_AbilityOwner_OnAbilityActivated::Broadcast(
            AbilityOwnerEntity,
            MakePayload(AbilityOwnerEntity, InHandle));

        return EAbilityProcessor_ForEachRequestResult::Continue;
    }

    auto
        FProcessor_Ability_HandleRequests::
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Ability_RequestDeactivate& InRequest)
            -> EAbilityProcessor_ForEachRequestResult
    {
        const auto AbilityOwnerEntity = [&]() -> FCk_Handle_AbilityOwner
        {
            const auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
            auto OwnerToReturn = InRequest.Get_AbilityOwner();

            CK_ENSURE_IF_NOT(InRequest.Get_AbilityOwner() == LifetimeOwner,
                TEXT("AbilityOwner [{}] and LifetimeOwner [{}] for [{}] MUST be the same. Forcing the change but this WILL fail in Production!"),
                OwnerToReturn, LifetimeOwner, InHandle)
            {
                OwnerToReturn = UCk_Utils_AbilityOwner_UE::Cast(LifetimeOwner);
            }

            CK_ENSURE(OwnerToReturn.Has<FTag_AbilityOwner_PendingSubAbilityOperation>(),
                TEXT("AbilityOwner [{}] does NOT have Pending Operations tag"), OwnerToReturn);

            OwnerToReturn.Add<ck::FTag_AbilityOwner_RemovePendingSubAbilityOperation>();
            return OwnerToReturn;
        }();

        if (NOT UCk_Utils_Ability_UE::Get_IsAbilityGiven(InHandle))
        { return EAbilityProcessor_ForEachRequestResult::Continue; }

        // --------------------------------------------------------------------------------------------------------------------

        auto& AbilityCurrent = InHandle.Get<ck::FFragment_Ability_Current,
            ck::IsValid_Policy_IncludePendingKill>();
        const auto& AbilityParams = InHandle.Get<ck::FFragment_Ability_Params,
            ck::IsValid_Policy_IncludePendingKill>();
        auto Script = AbilityCurrent.Get_AbilityScript().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(Script),
            TEXT("Cannot Deactivate Ability with Entity [{}] because its AbilityScript is INVALID"),
            InHandle)
        {
            return EAbilityProcessor_ForEachRequestResult::Continue;
        }

        if (AbilityCurrent.Get_Status() == ECk_Ability_Status::NotActive)
        {
            return EAbilityProcessor_ForEachRequestResult::Continue;
        }

        // --------------------------------------------------------------------------------------------------------------------

        const auto& AbilityActivationSettings = UCk_Utils_Ability_UE::Get_ActivationSettings(InHandle);
        const auto& GrantedTags = AbilityActivationSettings.Get_ActivationSettingsOnOwner().
                                                            Get_GrantTagsOnAbilityOwner();

        {
            auto NonConstAbilityOwner = AbilityOwnerEntity;
            auto& AbilityOwnerCurrent = NonConstAbilityOwner.Get<ck::FFragment_AbilityOwner_Current>();
            AbilityOwnerCurrent.RemoveTags(NonConstAbilityOwner, GrantedTags);
        }

        ability::VeryVerbose
        (
            TEXT("DEACTIVATING Ability [Name: {} | Entity: {}] from Ability Owner [{}] and Removing Tags [{}]"),
            UCk_Utils_Ability_UE::Get_ScriptClass(InHandle),
            InHandle,
            AbilityOwnerEntity,
            GrantedTags
        );

        // --------------------------------------------------------------------------------------------------------------------

        AbilityCurrent._Status = ECk_Ability_Status::NotActive;
        Script->OnDeactivateAbility();

        ck::UUtils_Signal_OnAbilityDeactivated::Broadcast(InHandle, ck::MakePayload(InHandle));

        if (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle))
        {
            return EAbilityProcessor_ForEachRequestResult::Continue;
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
                    return EAbilityProcessor_ForEachRequestResult::Continue;
                }
            }

            if (ck::IsValid(Script))
            {
                Script->_AbilityHandle = InHandle;
                Script->_AbilityOwnerHandle = AbilityOwnerEntity;
            }
        }

        return EAbilityProcessor_ForEachRequestResult::Continue;
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
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);
        auto AbilityOwner = UCk_Utils_AbilityOwner_UE::CastChecked(LifetimeOwner);

        if (InCurrent.Get_Status() == ECk_Ability_Status::Active)
        {
            ability::Verbose
            (
                TEXT("FORCE DEACTIVATING Ability [Name: {} | Entity: {}] with AbilityOwner [{}] that is {}"),
                UCk_Utils_GameplayLabel_UE::Get_Label(InHandle), InHandle, AbilityOwner,
                ck::IsValid(LifetimeOwner) ? TEXT("VALID") : TEXT("PENDING DESTROY")
            );

            AbilityOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
            FProcessor_Ability_HandleRequests::DoHandleRequest(InHandle, FFragment_Ability_RequestDeactivate{AbilityOwner});
        }

        ability::Verbose
        (
            TEXT("FORCE REVOKING Ability [Name: {} | Entity: {}] with AbilityOwner [{}] that is {}"),
            UCk_Utils_GameplayLabel_UE::Get_Label(InHandle), InHandle, AbilityOwner,
            ck::IsValid(LifetimeOwner) ? TEXT("VALID") : TEXT("PENDING DESTROY")
        );

        AbilityOwner.Add<ck::FTag_AbilityOwner_PendingSubAbilityOperation>();
        FProcessor_Ability_HandleRequests::DoHandleRequest(InHandle, FFragment_Ability_RequestRevoke{AbilityOwner, ECk_AbilityOwner_DestructionOnRevoke_Policy::DestroyOnRevoke});
    }
}

// --------------------------------------------------------------------------------------------------------------------
