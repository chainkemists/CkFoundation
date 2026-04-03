#include "CkInteraction_Utils.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"

#include "CkInteraction/Interaction/CkInteraction_EntityScript.h"
#include "CkInteraction/Interaction/CkInteraction_Fragment.h"
#include "CkInteraction/CkInteraction_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Interaction_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Interaction_ParamsData& InParams)
    -> FCk_Handle_Interaction
{
    const auto SpawnParams = FInstancedStruct::Make(InParams);
    auto PendingEntity = UCk_Utils_EntityScript_UE::Request_SpawnEntity(
        InHandle, UCk_Interaction_EntityScript::StaticClass(), SpawnParams);

    CK_ENSURE_IF_NOT(ck::IsValid(PendingEntity),
        TEXT("Unable to add Interaction to Handle [{}]: Request_SpawnEntity failed"),
        InHandle)
    { return {}; }

    auto NewInteractionEntity = ck::StaticCast<FCk_Handle_Interaction>(
        PendingEntity.Get_EntityUnderConstruction());

    RecordOfInteractions_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfInteractions_Utils::Request_Connect(InHandle, NewInteractionEntity);

    return NewInteractionEntity;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Interaction_UE, FCk_Handle_Interaction,
    ck::FFragment_Interaction_Params, ck::FFragment_Interaction_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Interaction_UE::
    Request_EndInteraction(
        FCk_Handle_Interaction& InInteraction,
        const FCk_Request_Interaction_EndInteraction& InRequest)
    -> FCk_Handle_Interaction
{
    InInteraction.AddOrGet<ck::FFragment_Interaction_Requests>()._Requests.Emplace(InRequest);
    return InInteraction;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Interaction_UE::
    BindTo_OnInteractionFinished(
        FCk_Handle_Interaction& InHandle,
        const FCk_Delegate_Interaction_OnInteractionFinished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Interaction
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_Interaction_OnInteractionFinished, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_Interaction_UE::
    UnbindFrom_OnInteractionFinished(
        FCk_Handle_Interaction& InHandle,
        const FCk_Delegate_Interaction_OnInteractionFinished& InDelegate)
    -> FCk_Handle_Interaction
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_Interaction_OnInteractionFinished, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_Interaction_UE::
    TryGet(
        const FCk_Handle& InInteractionOwner,
        const FCk_Handle& InSource,
        const FCk_Handle& InTarget,
        FGameplayTag InInteractionChannel)
    -> FCk_Handle_Interaction
{
    QUICK_SCOPE_CYCLE_COUNTER(TryGet)
    return RecordOfInteractions_Utils::Get_ValidEntry_If(InInteractionOwner,
    [&](const FCk_Handle_Interaction& InHandle) -> bool
    {
        return Get_InteractionSource(InHandle) == InSource &&
            Get_InteractionTarget(InHandle) == InTarget &&
            Get_InteractionChannel(InHandle).MatchesTagExact(InInteractionChannel);
    });
}

auto
    UCk_Utils_Interaction_UE::
    ForEach(
        FCk_Handle& InInteractionOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Interaction>
{
    auto ToRet = TArray<FCk_Handle_Interaction>{};

    ForEach(InInteractionOwner, [&](const FCk_Handle_Interaction& InInteraction)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InInteraction, InOptionalPayload); }
        else
        { ToRet.Emplace(InInteraction); }
    });

    return ToRet;
}

auto
    UCk_Utils_Interaction_UE::
    ForEach(
        FCk_Handle& InInteractionOwner,
        const TFunction<void(FCk_Handle_Interaction)>& InFunc)
    -> void
{
    RecordOfInteractions_Utils::ForEach_ValidEntry(InInteractionOwner, InFunc);
}

auto
    UCk_Utils_Interaction_UE::
    Get_InteractionDistance(
        const FCk_Handle_Interaction& InHandle)
    -> float
{
    const auto& InteractionSourceActor = Get_InteractionSourceActor(InHandle);
    const auto& InteractionTargetActor = Get_InteractionTargetActor(InHandle);

    return FVector::Distance(InteractionSourceActor->GetActorLocation(), InteractionTargetActor->GetActorLocation());
}

auto
    UCk_Utils_Interaction_UE::
    Get_InteractionSource(
        const FCk_Handle_Interaction& InHandle)
    -> FCk_Handle
{
    return InHandle.Get<ck::FFragment_Interaction_Params>().Get_Params().Get_Source();
}

auto
    UCk_Utils_Interaction_UE::
    Get_InteractionSourceActor(
        const FCk_Handle_Interaction& InHandle)
    -> AActor*
{
    const auto& InteractionSource = Get_InteractionSource(InHandle);

    if (ck::Is_NOT_Valid(InteractionSource))
    { return {}; }

    const auto& ActorEntity = UCk_Utils_OwningActor_UE::TryGet_Entity_OwningActor_InOwnershipChain(InteractionSource);

    if (ck::Is_NOT_Valid(ActorEntity))
    { return {}; }

    return UCk_Utils_OwningActor_UE::Get_EntityOwningActor(ActorEntity);
}

auto
    UCk_Utils_Interaction_UE::
    Get_InteractionInstigator(
        const FCk_Handle_Interaction& InHandle)
    -> FCk_Handle
{
    return InHandle.Get<ck::FFragment_Interaction_Params>().Get_Params().Get_Instigator();
}

auto
    UCk_Utils_Interaction_UE::
    Get_InteractionTarget(
        const FCk_Handle_Interaction& InHandle)
    -> FCk_Handle
{
    return InHandle.Get<ck::FFragment_Interaction_Params>().Get_Params().Get_Target();
}

auto
    UCk_Utils_Interaction_UE::
    Get_InteractionTargetActor(
        const FCk_Handle_Interaction& InHandle)
    -> AActor*
{
    const auto& InteractionTarget = Get_InteractionTarget(InHandle);

    if (ck::Is_NOT_Valid(InteractionTarget))
    { return {}; }

    const auto& ActorEntity = UCk_Utils_OwningActor_UE::TryGet_Entity_OwningActor_InOwnershipChain(InteractionTarget);

    if (ck::Is_NOT_Valid(ActorEntity))
    { return {}; }

    return UCk_Utils_OwningActor_UE::Get_EntityOwningActor(ActorEntity);
}

auto
    UCk_Utils_Interaction_UE::
    Get_InteractionChannel(
        const FCk_Handle_Interaction& InHandle)
    -> const FGameplayTag&
{
    return InHandle.Get<ck::FFragment_Interaction_Params>().Get_Params().Get_InteractionChannel();
}

auto
    UCk_Utils_Interaction_UE::
    Get_InteractionCompletionPolicy(
        const FCk_Handle_Interaction& InHandle)
    -> ECk_Interaction_CompletionPolicy
{
    return InHandle.Get<ck::FFragment_Interaction_Params>().Get_Params().Get_CompletionPolicy();
}

auto
    UCk_Utils_Interaction_UE::
    Get_InteractionDuration(
        const FCk_Handle_Interaction& InHandle)
    -> FCk_Time
{
    const auto& TimeAttribute = Get_InteractionTimeAttribute(InHandle);
    return FCk_Time{UCk_Utils_FloatAttribute_UE::Get_FinalValue(TimeAttribute, ECk_MinMaxCurrent::Max)};
}

auto
    UCk_Utils_Interaction_UE::
    Get_InteractionTimeElapsed(
        const FCk_Handle_Interaction& InHandle)
    -> FCk_Time
{
    const auto& TimeAttribute = Get_InteractionTimeAttribute(InHandle);
    return FCk_Time{UCk_Utils_FloatAttribute_UE::Get_FinalValue(TimeAttribute, ECk_MinMaxCurrent::Current)};
}

auto
    UCk_Utils_Interaction_UE::
    Get_InteractionTimeAttribute(
        const FCk_Handle_Interaction& InHandle)
    -> FCk_Handle_FloatAttribute
{
    return UCk_Utils_FloatAttribute_UE::TryGet(InHandle, TAG_InteractionTime_FloatAttribute_Name);
}

// --------------------------------------------------------------------------------------------------------------------
