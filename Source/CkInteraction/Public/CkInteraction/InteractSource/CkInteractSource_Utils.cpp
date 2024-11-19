#include "CkInteractSource_Utils.h"

#include "CkInteraction/InteractSource/CkInteractSource_Fragment.h"
#include "CkInteraction/CkInteraction_Log.h"
#include "CkInteraction/Interaction/CkInteraction_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractSource_UE::
    Add(
        FCk_Handle& InInteractSourceOwner,
        const FCk_Fragment_InteractSource_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_InteractSource
{
    auto NewInteractSourceEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_InteractSource>(InInteractSourceOwner);

    NewInteractSourceEntity.Add<ck::FFragment_InteractSource_Params>(InParams);
    NewInteractSourceEntity.Add<ck::FFragment_InteractSource_Current>();
    NewInteractSourceEntity.Add<ck::FTag_InteractSource_RequiresSetup>();

    UCk_Utils_GameplayLabel_UE::Add(NewInteractSourceEntity, InParams.Get_InteractionChannel());

    RecordOfInteractSources_Utils::AddIfMissing(InInteractSourceOwner, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfInteractSources_Utils::Request_Connect(InInteractSourceOwner, NewInteractSourceEntity);

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::interaction::VeryVerbose
        (
            TEXT("Skipping creation of InteractSource Rep Fragment on Entity [{}] because it's set to [{}]"),
            InInteractSourceOwner,
            InReplicates
        );

        return {};
    }

    TryAddReplicatedFragment<UCk_Fragment_InteractSource_Rep>(InInteractSourceOwner);
    return NewInteractSourceEntity;
}

auto
    UCk_Utils_InteractSource_UE::
    AddMultiple(
        FCk_Handle& InInteractSourceOwner,
        const FCk_Fragment_MultipleInteractSource_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> TArray<FCk_Handle_InteractSource>
{
    return ck::algo::Transform<TArray<FCk_Handle_InteractSource>>(
        InParams.Get_InteractSourceParams(), [&](const FCk_Fragment_InteractSource_ParamsData& InParam)
    {
        return Add(InInteractSourceOwner, InParam, InReplicates);
    });
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_InteractSource_UE, FCk_Handle_InteractSource,
    ck::FFragment_InteractSource_Params, ck::FFragment_InteractSource_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractSource_UE::
    Request_CancelInteraction(
        FCk_Handle_InteractSource& InInteractSource,
        const FCk_Request_InteractSource_CancelInteraction& InRequest)
    -> FCk_Handle_InteractSource
{
    InInteractSource.AddOrGet<ck::FFragment_InteractSource_Requests>()._Requests.Emplace(InRequest);
    return InInteractSource;
}

auto
    UCk_Utils_InteractSource_UE::
    Request_CancelAllInteractions(
        FCk_Handle_InteractSource& InInteractSource)
    -> FCk_Handle_InteractSource
{
    auto& Requests = InInteractSource.AddOrGet<ck::FFragment_InteractSource_Requests>()._Requests;

    for (const auto& SignalPair : InInteractSource.Get<ck::FFragment_InteractSource_Current>()._InteractionFinishedSignals)
    {
        const auto& CurrentInteraction = SignalPair.Key;
        Request_CancelInteraction(InInteractSource,
            FCk_Request_InteractSource_CancelInteraction{
                UCk_Utils_Interaction_UE::Get_InteractionTarget(CurrentInteraction)});
    }
    return InInteractSource;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractSource_UE::
    Request_StartInteraction(
        FCk_Handle_InteractSource& InInteractSource,
        const FCk_Request_InteractSource_StartInteraction& InRequest)
    -> FCk_Handle_InteractSource
{
    InInteractSource.AddOrGet<ck::FFragment_InteractSource_Requests>()._Requests.Emplace(InRequest);
    InInteractSource.Get<ck::FFragment_InteractSource_Current>()._InteractionsPendingAdd.Emplace(InRequest.Get_Interaction());
    return InInteractSource;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractSource_UE::
    Get_InteractionChannel(
        const FCk_Handle_InteractSource& InHandle)
    -> const FGameplayTag&
{
    return InHandle.Get<ck::FFragment_InteractSource_Params>().Get_Params().Get_InteractionChannel();
}

auto
    UCk_Utils_InteractSource_UE::
    Get_InteractionCountPerSourcePolicy(
        const FCk_Handle_InteractSource& InHandle)
    -> ECk_InteractionSource_ConcurrentInteractionsPolicy
{
    return InHandle.Get<ck::FFragment_InteractSource_Params>().Get_Params().Get_ConcurrentInteractionsPolicy();
}

auto
    UCk_Utils_InteractSource_UE::
    Get_CurrentInteractions(
        const FCk_Handle_InteractSource& InHandle)
    -> TArray<FCk_Handle_Interaction>
{
    auto ToRet = TArray<FCk_Handle_Interaction>{};
    InHandle.Get<ck::FFragment_InteractSource_Current>()._InteractionFinishedSignals.GetKeys(ToRet);
    return ToRet;
}

auto
    UCk_Utils_InteractSource_UE::
    Get_PendingInteractions(
        const FCk_Handle_InteractSource& InHandle)
    -> TArray<FCk_Handle_Interaction>
{
    return InHandle.Get<ck::FFragment_InteractSource_Current>()._InteractionsPendingAdd;
}

auto
    UCk_Utils_InteractSource_UE::
    TryGet_CurrentInteractionsByTarget(
        const FCk_Handle_InteractSource& InHandle,
        const FCk_Handle& InTarget)
    -> FCk_Handle_Interaction
{
    for (auto& InSignalPair : InHandle.Get<ck::FFragment_InteractSource_Current>()._InteractionFinishedSignals)
    {
        auto& InInteraction = InSignalPair.Key;
        if (UCk_Utils_Interaction_UE::Get_InteractionTarget(InInteraction) == InTarget)
        { return InInteraction; }
    }
    return {};
}

auto
    UCk_Utils_InteractSource_UE::
    TryGet(
        const FCk_Handle& InInteractSourceOwner,
        FGameplayTag InInteractionChannel)
    -> FCk_Handle_InteractSource
{
    return RecordOfInteractSources_Utils::Get_ValidEntry_If(InInteractSourceOwner,
        ck::algo::MatchesGameplayLabelExact{InInteractionChannel});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractSource_UE::
    BindTo_OnNewInteraction(
        FCk_Handle_InteractSource& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_InteractSource_OnNewInteraction& InDelegate)
    -> FCk_Handle_InteractSource
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_InteractSource_OnNewInteraction, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_InteractSource_UE::
    UnbindFrom_OnNewInteraction(
        FCk_Handle_InteractSource& InHandle,
        const FCk_Delegate_InteractSource_OnNewInteraction& InDelegate)
    -> FCk_Handle_InteractSource
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_InteractSource_OnNewInteraction, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_InteractSource_UE::
    BindTo_OnInteractionFinished(
        FCk_Handle_InteractSource& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_InteractSource_OnInteractionFinished& InDelegate)
    -> FCk_Handle_InteractSource
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_InteractSource_OnInteractionFinished, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_InteractSource_UE::
    UnbindFrom_OnInteractionFinished(
        FCk_Handle_InteractSource& InHandle,
        const FCk_Delegate_InteractSource_OnInteractionFinished& InDelegate)
    -> FCk_Handle_InteractSource
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_InteractSource_OnInteractionFinished, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------