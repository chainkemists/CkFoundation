#include "CkInteractTarget_Utils.h"

#include "CkInteraction/InteractTarget/CkInteractTarget_Fragment.h"
#include "CkInteraction/CkInteraction_Log.h"
#include "CkInteraction/CkInteraction_Stats.h"
#include "CkInteraction/InteractSource/CkInteractSource_Utils.h"
#include "CkInteraction/Interaction/CkInteraction_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Interaction::CanInteractWith"), STAT_Interaction_CanInteractWith, STATGROUP_CkInteraction);
DECLARE_CYCLE_STAT(TEXT("Interaction::CustomCanInteract ProcessEvent"), STAT_Interaction_CustomCanInteract_ProcessEvent, STATGROUP_CkInteraction);
DECLARE_CYCLE_STAT(TEXT("InteractTarget::TryGet"), STAT_Interaction_InteractTarget_TryGet, STATGROUP_CkInteraction);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractTarget_UE::
    ResolveSetupData(
        const FCk_InteractTarget_SetupData& InSetupData,
        const UObject* InWorldContextObject)
    -> FCk_InteractTarget_Spec
{
    auto Params = InSetupData.Params;
    Params._CanInteractWithReference = Make_CanInteractWithReference(
        InSetupData.CanInteractWithRef, InWorldContextObject);
    return Params;
}

auto
    UCk_Utils_InteractTarget_UE::
    Add(
        FCk_Handle& InInteractTargetOwner,
        const FCk_InteractTarget_Spec& InParams,
        ECk_Replication InReplicates,
        const UObject* InWorldContextObject)
    -> FCk_Handle_InteractTarget
{
    auto NewInteractTargetEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_InteractTarget>(InInteractTargetOwner);

    NewInteractTargetEntity.Add<ck::FFragment_InteractTarget_Params>(InParams);
    NewInteractTargetEntity.Add<ck::FFragment_InteractTarget_Current>();
    NewInteractTargetEntity.Add<ck::FTag_InteractTarget_RequiresSetup>();

    UCk_Utils_GameplayLabel_UE::Add(NewInteractTargetEntity, InParams.Get_InteractionChannel());

    RecordOfInteractTargets_Utils::AddIfMissing(InInteractTargetOwner, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfInteractTargets_Utils::Request_Connect(InInteractTargetOwner, NewInteractTargetEntity);

    return NewInteractTargetEntity;
}

auto
    UCk_Utils_InteractTarget_UE::
    AddFromSetupData(
        FCk_Handle& InInteractTargetOwner,
        const FCk_InteractTarget_SetupData& InSetupData,
        const ECk_Replication InReplicates,
        const UObject* InWorldContextObject)
    -> FCk_Handle_InteractTarget
{
    auto Params = ResolveSetupData(InSetupData, InWorldContextObject);
    return Add(InInteractTargetOwner, Params, InReplicates, InWorldContextObject);
}

auto
    UCk_Utils_InteractTarget_UE::
    AddMultiple(
        FCk_Handle& InInteractTargetOwner,
        const FCk_MultipleInteractTarget_Spec& InParams,
        ECk_Replication InReplicates,
        const UObject* InWorldContextObject)
    -> TArray<FCk_Handle_InteractTarget>
{
    return ck::algo::Transform<TArray<FCk_Handle_InteractTarget>>(
        InParams.Get_InteractTargetParams(), [&](const FCk_InteractTarget_Spec& InParam)
    {
        return Add(InInteractTargetOwner, InParam, InReplicates, InWorldContextObject);
    });
}

auto
    UCk_Utils_InteractTarget_UE::
    ResolveMultipleSetupData(
        const FCk_MultipleInteractTarget_SetupData& InSetupData,
        const UObject* InWorldContextObject)
    -> FCk_MultipleInteractTarget_Spec
{
    auto Params = TArray<FCk_InteractTarget_Spec>{};
    Params.Reserve(InSetupData.InteractTargets.Num());

    for (const auto& Entry : InSetupData.InteractTargets)
    {
        Params.Emplace(ResolveSetupData(Entry, InWorldContextObject));
    }

    return FCk_MultipleInteractTarget_Spec{MoveTemp(Params)};
}

auto
    UCk_Utils_InteractTarget_UE::
    AddMultipleFromSetupData(
        FCk_Handle& InInteractTargetOwner,
        const FCk_MultipleInteractTarget_SetupData& InSetupData,
        const ECk_Replication InReplicates,
        const UObject* InWorldContextObject)
    -> TArray<FCk_Handle_InteractTarget>
{
    auto Params = ResolveMultipleSetupData(InSetupData, InWorldContextObject);
    return AddMultiple(InInteractTargetOwner, Params, InReplicates, InWorldContextObject);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_InteractTarget_UE, FCk_Handle_InteractTarget,
    ck::FFragment_InteractTarget_Params, ck::FFragment_InteractTarget_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractTarget_UE::
    Request_StartInteraction(
        FCk_Handle_InteractTarget& InInteractTarget,
        const FCk_Try_InteractTarget_StartInteraction& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_InteractTarget
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InInteractTarget.AddOrGet<ck::FFragment_InteractTarget_Requests>()._Requests.Emplace(InRequest);
    return InInteractTarget;
}

auto
    UCk_Utils_InteractTarget_UE::
    Request_CancelInteraction(
        FCk_Handle_InteractTarget& InInteractTarget,
        const FCk_Request_InteractTarget_CancelInteraction& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_InteractTarget
{
    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InInteractTarget.AddOrGet<ck::FFragment_InteractTarget_Requests>()._Requests.Emplace(InRequest);
    return InInteractTarget;
}

auto
    UCk_Utils_InteractTarget_UE::
    Request_CancelAllInteractions(
        FCk_Handle_InteractTarget& InInteractTarget)
    -> FCk_Handle_InteractTarget
{
    auto& Requests = InInteractTarget.AddOrGet<ck::FFragment_InteractTarget_Requests>()._Requests;

    UCk_Utils_Interaction_UE::ForEach(InInteractTarget, [&](const FCk_Handle_Interaction& InInteraction)
    {
        const auto& Source = UCk_Utils_Interaction_UE::Get_InteractionSource(InInteraction);
        Requests.Emplace(FCk_Request_InteractTarget_CancelInteraction{Source});
    });
    return InInteractTarget;
}

auto
    UCk_Utils_InteractTarget_UE::
    Get_Enabled(
        const FCk_Handle_InteractTarget& InHandle)
    -> ECk_EnableDisable
{
    return InHandle.Get<ck::FFragment_InteractTarget_Current>()._Enabled;
}

auto
    UCk_Utils_InteractTarget_UE::
    Set_Enabled(
        FCk_Handle_InteractTarget& InHandle,
        ECk_EnableDisable InEnabled)
    -> void
{
    InHandle.Get<ck::FFragment_InteractTarget_Current>()._Enabled = InEnabled;

    if (InEnabled == ECk_EnableDisable::Disable)
    {
        Request_CancelAllInteractions(InHandle);
    }
}

auto
    UCk_Utils_InteractTarget_UE::
    Get_InteractionChannel(
        const FCk_Handle_InteractTarget& InHandle)
    -> const FGameplayTag&
{
    return InHandle.Get<ck::FFragment_InteractTarget_Params>().Get_Params().Get_InteractionChannel();
}

auto
    UCk_Utils_InteractTarget_UE::
    Get_CanInteractWith(
        const FCk_Handle_InteractTarget& InTarget,
        const FCk_Handle& InSource)
    -> ECk_CanInteractWithResult
{
    SCOPE_CYCLE_COUNTER(STAT_Interaction_CanInteractWith);

    if (ck::Is_NOT_Valid(InTarget))
    { return ECk_CanInteractWithResult::TargetNotValid; }

    if (ck::Is_NOT_Valid(InSource))
    { return ECk_CanInteractWithResult::SourceNotValid; }

    if (Get_Enabled(InTarget) == ECk_EnableDisable::Disable)
    { return ECk_CanInteractWithResult::TargetDisabled; }

    const auto& InteractSource = UCk_Utils_InteractSource_UE::Cast(InSource);
    if (ck::IsValid(InteractSource))
    {
        const auto& SourceChannel = UCk_Utils_InteractSource_UE::Get_InteractionChannel(InteractSource);

        if (const auto& TargetChannel = Get_InteractionChannel(InTarget);
            NOT TargetChannel.MatchesTagExact(SourceChannel))
        { return ECk_CanInteractWithResult::ChannelMismatch; }

        if (UCk_Utils_InteractSource_UE::Get_ConcurrentInteractionsPolicy(InteractSource) == ECk_InteractionSource_ConcurrentInteractionsPolicy::SingleInteraction)
        {
            if (UCk_Utils_InteractSource_UE::Get_CurrentInteractions(InteractSource).Num() + UCk_Utils_InteractSource_UE::Get_PendingInteractions(InteractSource).Num() > 0)
            { return ECk_CanInteractWithResult::SourceRejectedSecondInteraction; }
        }
    }

    // TODO: This only works on auth currently, will need to duplicate interact targets to allow clients to filter this way
    if (const auto& MatchingInteraction = UCk_Utils_Interaction_UE::TryGet(InTarget, InteractSource, InTarget, Get_InteractionChannel(InTarget));
        ck::IsValid(MatchingInteraction))
    {
        return ECk_CanInteractWithResult::AlreadyExists;
    }

    const auto& Params = InTarget.Get<ck::FFragment_InteractTarget_Params>().Get_Params();

    if (Params.Get_ConcurrentInteractionsPolicy() == ECk_InteractionTarget_ConcurrentInteractionsPolicy::SingleInteraction &&
        UCk_Utils_Interaction_UE::RecordOfInteractions_Utils::Get_ValidEntriesCount(InTarget) > 0)
    { return ECk_CanInteractWithResult::TargetRejectedSecondInteraction; }

    // TODO: Evaluate if sending the source as the instigator is correct/necessary
    if (NOT Get_PassesCustomCanInteractWith(InTarget, InSource, InSource))
    { return ECk_CanInteractWithResult::CustomValidationFailed; }

    return ECk_CanInteractWithResult::CanInteractWith;
}

auto
    UCk_Utils_InteractTarget_UE::
    Get_ConcurrentInteractionsPolicy(
        const FCk_Handle_InteractTarget& InTarget)
    -> ECk_InteractionTarget_ConcurrentInteractionsPolicy
{
    return InTarget.Get<ck::FFragment_InteractTarget_Params>().Get_Params().Get_ConcurrentInteractionsPolicy();
}

auto
    UCk_Utils_InteractTarget_UE::
    Get_InteractionCompletionPolicy(
        const FCk_Handle_InteractTarget& InTarget)
    -> ECk_Interaction_CompletionPolicy
{
    return InTarget.Get<ck::FFragment_InteractTarget_Params>().Get_Params().Get_CompletionPolicy();
}

auto
    UCk_Utils_InteractTarget_UE::
    Get_InteractionDuration(
        const FCk_Handle_InteractTarget& InTarget)
    -> FCk_Time
{
    return InTarget.Get<ck::FFragment_InteractTarget_Params>().Get_Params().Get_InteractionDuration();
}

auto
    UCk_Utils_InteractTarget_UE::
    Get_CurrentInteractions(
        FCk_Handle_InteractTarget& InHandle)
    -> TArray<FCk_Handle_Interaction>
{
    return UCk_Utils_Interaction_UE::ForEach(InHandle, {}, {});
}

auto
    UCk_Utils_InteractTarget_UE::
    TryGet(
        const FCk_Handle& InInteractTargetOwner,
        FGameplayTag InInteractionChannel)
    -> FCk_Handle_InteractTarget
{
    SCOPE_CYCLE_COUNTER(STAT_Interaction_InteractTarget_TryGet);
    return RecordOfInteractTargets_Utils::Get_ValidEntry_ByTag(InInteractTargetOwner, InInteractionChannel);
}

auto
    UCk_Utils_InteractTarget_UE::
    TryGet_Interaction(
        const FCk_Handle_InteractTarget& InTarget,
        const FCk_Handle& InInteractSource)
    -> FCk_Handle_Interaction
{
    return UCk_Utils_Interaction_UE::TryGet(InTarget, InInteractSource, InTarget, Get_InteractionChannel(InTarget));
}

auto
    UCk_Utils_InteractTarget_UE::
    ForEach_InteractTarget(
        const FCk_Handle& InInteractTargetOwner,
        FInstancedStruct InOptionalPayload,
        FCk_Lambda_InHandle InDelegate)
    -> TArray<FCk_Handle_InteractTarget>
{
    using HandleType = FCk_Handle_InteractTarget;
    auto Ret = TArray<HandleType>{};

    ForEach_InteractTarget(InInteractTargetOwner, [&](HandleType InHandle)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InHandle, InOptionalPayload); }

        Ret.Emplace(InHandle);
    });

    return Ret;
}

auto
    UCk_Utils_InteractTarget_UE::
    ForEach_InteractTarget(
        const FCk_Handle& InInteractTargetOwner,
        const TFunction<void(FCk_Handle_InteractTarget)>& InFunc)
    -> void
{
    RecordOfInteractTargets_Utils::ForEach_ValidEntry(InInteractTargetOwner, InFunc);
}

// --------------------------------------------------------------------------------------------------------------------
// Custom Validation
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractTarget_UE::
    Get_PassesCustomCanInteractWith(
        const FCk_Handle_InteractTarget& InTarget,
        const FCk_Handle& InInteractSource,
        const FCk_Handle& InInteractInstigator)
    -> bool
{
    if (ck::Is_NOT_Valid(InTarget))
    { return true; }

    const auto& Params = InTarget.Get<ck::FFragment_InteractTarget_Params>().Get_Params();

    if (const auto& NativeDelegate = Params.Get_CustomCanInteractWith();
        NativeDelegate.IsBound())
    {
        if (NOT NativeDelegate.Execute(InTarget, InInteractSource, InInteractInstigator))
        { return false; }
    }

    if (const auto& DynamicDelegate = Params.Get_CustomCanInteractWithDynamic();
        DynamicDelegate.IsBound())
    {
        auto Result = true;
        DynamicDelegate.ExecuteIfBound(InTarget, InInteractSource, InInteractInstigator, Result);

        if (NOT Result)
        { return false; }
    }

    if (const auto MemberRefResult = Resolve_CanInteractWith(
            Params.Get_CanInteractWithReference(), InTarget, InInteractSource, InInteractInstigator);
        MemberRefResult.IsSet())
    {
        if (NOT MemberRefResult.GetValue())
        { return false; }
    }

    return true;
}

auto
    UCk_Utils_InteractTarget_UE::
    Resolve_CanInteractWith(
        const FCk_InteractTarget_CanInteractWithReference& InRef,
        FCk_Handle_InteractTarget InTarget,
        FCk_Handle InInteractSource,
        FCk_Handle InInteractInstigator)
    -> TOptional<bool>
{
    SCOPE_CYCLE_COUNTER(STAT_Interaction_CustomCanInteract_ProcessEvent);

    auto* const MemberClass = InRef.Resolve_MemberClass();

    if (ck::Is_NOT_Valid(MemberClass))
    { return {}; }

    auto RuntimeReference = FMemberReference{};
    RuntimeReference.SetExternalMember(InRef.Get_MemberName(), MemberClass, InRef.Get_MemberGuid());

    auto* const Function = RuntimeReference.ResolveMember<UFunction>(MemberClass);

    if (ck::Is_NOT_Valid(Function))
    { return {}; }

    struct
    {
        FCk_Handle_InteractTarget Target;
        FCk_Handle InteractSource;
        FCk_Handle InteractInstigator;
        bool ReturnValue = false;
    } Args = { MoveTemp(InTarget), MoveTemp(InInteractSource), MoveTemp(InInteractInstigator) };

    auto* const Context = MemberClass->GetDefaultObject();
    Context->ProcessEvent(Function, &Args);

    return Args.ReturnValue;
}

auto
    UCk_Utils_InteractTarget_UE::
    Make_CanInteractWithReference(
        const FMemberReference& InRef,
        const UObject* InWorldContextObject)
    -> FCk_InteractTarget_CanInteractWithReference
{
    auto* MemberClass = InRef.GetMemberParentClass();
    if (InRef.IsSelfContext() && ck::IsValid(InWorldContextObject))
    {
        MemberClass = InWorldContextObject->GetClass();
    }

    if (ck::Is_NOT_Valid(MemberClass))
    { return {}; }

    const auto MemberGuid = InRef.IsSelfContext() ? FGuid{} : InRef.GetMemberGuid();
    return {InRef.GetMemberName(), MemberGuid, MemberClass};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractTarget_UE::
    BindTo_OnNewInteraction(
        FCk_Handle_InteractTarget& InHandle,
        const FCk_Delegate_InteractTarget_OnNewInteraction& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_InteractTarget
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_InteractTarget_OnNewInteraction, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_InteractTarget_UE::
    UnbindFrom_OnNewInteraction(
        FCk_Handle_InteractTarget& InHandle,
        const FCk_Delegate_InteractTarget_OnNewInteraction& InDelegate)
    -> FCk_Handle_InteractTarget
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_InteractTarget_OnNewInteraction, InHandle, InDelegate);
    return InHandle;
}

auto
    UCk_Utils_InteractTarget_UE::
    BindTo_OnInteractionFinished(
        FCk_Handle_InteractTarget& InHandle,
        const FCk_Delegate_InteractTarget_OnInteractionFinished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_InteractTarget
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_InteractTarget_OnInteractionFinished, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InHandle;
}

auto
    UCk_Utils_InteractTarget_UE::
    UnbindFrom_OnInteractionFinished(
        FCk_Handle_InteractTarget& InHandle,
        const FCk_Delegate_InteractTarget_OnInteractionFinished& InDelegate)
    -> FCk_Handle_InteractTarget
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_InteractTarget_OnInteractionFinished, InHandle, InDelegate);
    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
