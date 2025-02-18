#include "CkInteraction_Utils.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkLabel/CkLabel_Utils.h"

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
    auto NewInteractionEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Interaction>(InHandle);

    NewInteractionEntity.Add<ck::FFragment_Interaction_Params>(InParams);
    NewInteractionEntity.Add<ck::FFragment_Interaction_Current>();

    if (InParams.Get_CompletionPolicy() == ECk_Interaction_CompletionPolicy::Timed)
    {
        const auto& DurationRefillAttributeParams = FCk_Fragment_FloatAttributeRefill_ParamsData{
            TAG_InteractionDurationRefill_FloatAttribute_Name,
            1.0f}
            .Set_StartingState(ECk_Attribute_RefillState::Running)
            .Set_RefillBehavior(ECk_Attribute_Refill_Policy::Variable);

        const auto DurationAttributeParams = FCk_Fragment_FloatAttribute_ParamsData{
            TAG_InteractionDuration_FloatAttribute_Name,
            0.0f}
            .Set_MinMax(ECk_MinMax::MinMax)
            .Set_MinValue(0.0f)
            .Set_MaxValue(InParams.Get_InteractionDuration().Get_Seconds())
            .Set_EnableRefill(true)
            .Set_RefillParams(DurationRefillAttributeParams);

        UCk_Utils_FloatAttribute_UE::Add(NewInteractionEntity, DurationAttributeParams, ECk_Replication::DoesNotReplicate);
    }

    UCk_Utils_GameplayLabel_UE::Add(NewInteractionEntity, InParams.Get_InteractionChannel());
    UCk_Utils_Handle_UE::Set_DebugName(NewInteractionEntity, *ck::Format_UE(TEXT("Interaction: Source [{}] Target [{}]"), InParams.Get_Source(), InParams.Get_Target()));

    UCk_Entity_ConstructionScript_PDA::Request_Construct(NewInteractionEntity, InParams.Get_ConstructionScript(), {});

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
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_Interaction_OnInteractionFinished& InDelegate)
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
        const FCk_Handle_Interaction& InHandle,
        ECk_MinMaxCurrent InAttributeComponent)
    -> FCk_Time
{
    if (const auto& DurationAttribute = Get_InteractionDurationAttribute(InHandle);
        ck::IsValid(DurationAttribute))
    { return FCk_Time{UCk_Utils_FloatAttribute_UE::Get_FinalValue(DurationAttribute, InAttributeComponent)}; }

    CK_ENSURE_IF_NOT(InAttributeComponent == ECk_MinMaxCurrent::Max, TEXT("Trying to get interaction duration attribute from interaction [{}] before attribute has been initialized!"),
        InHandle)
    { return {}; }

    return InHandle.Get<ck::FFragment_Interaction_Params>().Get_Params().Get_InteractionDuration();
}

auto
    UCk_Utils_Interaction_UE::
    Get_InteractionDurationAttribute(
        const FCk_Handle_Interaction& InHandle)
    -> FCk_Handle_FloatAttribute
{
    return UCk_Utils_FloatAttribute_UE::TryGet(InHandle, TAG_InteractionDuration_FloatAttribute_Name);
}

// --------------------------------------------------------------------------------------------------------------------