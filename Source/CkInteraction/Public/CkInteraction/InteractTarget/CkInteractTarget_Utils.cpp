#include "CkInteractTarget_Utils.h"

#include "CkInteraction/InteractTarget/CkInteractTarget_Fragment.h"
#include "CkInteraction/InteractTarget/CkInteractTarget_ConstructionScript.h"
#include "CkInteraction/CkInteraction_Log.h"
#include "CkInteraction/InteractSource/CkInteractSource_Utils.h"
#include "CkInteraction/Interaction/CkInteraction_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_interact_target
{
    // Walk the lifetime chain from InStart (inclusive) to the nearest entity that owns a replication
    // driver — the only valid owner for Request_BuildAndReplicate. The InteractTarget owner handed to
    // Add (e.g. an Interactable probe-node) usually has none; its replicated ancestor (the entity-script
    // root) does. Guards against a malformed/cyclic chain (Get_LifetimeOwner returns self at the root).
    static auto
        DoFind_DriverBearingOwner(
            const FCk_Handle& InStart)
        -> FCk_Handle
    {
        auto Current = InStart;
        auto Guard = 0;

        while (ck::IsValid(Current) && Guard++ < 64)
        {
            if (UCk_Utils_EntityReplicationDriver_UE::Has(Current))
            { return Current; }

            const auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Current);
            if (Owner == Current)
            { break; }

            Current = Owner;
        }

        return {};
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractTarget_UE::
    Add(
        FCk_Handle& InInteractTargetOwner,
        const FCk_Fragment_InteractTarget_ParamsData& InParams,
        ECk_Replication InReplicates,
        TSubclassOf<UCk_InteractTarget_ConstructionScript> InReplicatedConstructionScript,
        const FInstancedStruct& InConstructionConfig,
        const UObject* InWorldContextObject)
    -> FCk_Handle_InteractTarget
{
    // Replicated path: a supplied construction-script class is the REAL opt-in (not the InReplicates
    // enum, whose default is Replicates and is relied on by every plain caller). When set, net-link the
    // InteractTarget via Request_BuildAndReplicate so its construction-script-folded SM replicates. The
    // class — net-stable — is the recipe the client re-runs to rebuild IT+SM; a runtime archetype would
    // null out on the client. Build under the nearest driver-bearing ancestor; the host returns a valid
    // InteractTarget, the client returns invalid (build is host-only) and receives the replicated copy.
    if (InReplicates == ECk_Replication::Replicates && ck::IsValid(InReplicatedConstructionScript))
    {
        // Host-only build. On a client this Add runs again during the entity-script reconstruction, but
        // the replicated InteractTarget arrives via replication — building a local duplicate here would
        // shadow it. Return invalid so the caller skips local wiring and waits for the replicated copy.
        if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InInteractTargetOwner))
        { return {}; }

        if (auto DriverOwner = ck_interact_target::DoFind_DriverBearingOwner(InInteractTargetOwner);
            ck::IsValid(DriverOwner))
        {
            // Carry the optional by-value recipe so the GENERIC UCk_InteractTarget_ConstructionScript can
            // build from it on both host and client; empty config => the supplied subclass CDO is used.
            const auto CtorInfo = FCk_EntityReplicationDriver_ConstructionInfo{InReplicatedConstructionScript}
                .Set_ConstructionConfig(InConstructionConfig);
            auto NewReplicatedTarget = UCk_Utils_EntityReplicationDriver_UE::Request_BuildAndReplicate(DriverOwner, CtorInfo);

            if (ck::Is_NOT_Valid(NewReplicatedTarget))
            { return {}; }

            auto NewReplicatedTargetTyped = Cast(NewReplicatedTarget);

            // Connect into the owner's Record (lifetime-independent) so Get_AllInteractTargets finds it —
            // the target was built under DriverOwner, not necessarily under InInteractTargetOwner.
            RecordOfInteractTargets_Utils::AddIfMissing(InInteractTargetOwner, ECk_Record_EntryHandlingPolicy::Default);
            RecordOfInteractTargets_Utils::Request_Connect(InInteractTargetOwner, NewReplicatedTargetTyped);

            return NewReplicatedTargetTyped;
        }

        ck::interaction::Warning(TEXT("InteractTarget replication requested for owner [{}] but no driver-bearing "
            "entity exists in its lifetime chain — falling back to a non-replicated InteractTarget."), InInteractTargetOwner);
    }

    auto NewInteractTargetEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_InteractTarget>(InInteractTargetOwner);

    auto FixedParams = InParams;

    if (ck::IsValid(InWorldContextObject))
    {
        auto* const ContextClass = InWorldContextObject->GetClass();

        if (auto& InteractRef = FixedParams.Get_CanInteractWithRef();
            InteractRef.IsSelfContext())
        {
            InteractRef.SetExternalMember(InteractRef.GetMemberName(), ContextClass);
        }
    }

    NewInteractTargetEntity.Add<ck::FFragment_InteractTarget_Params>(FixedParams);
    NewInteractTargetEntity.Add<ck::FFragment_InteractTarget_Current>();
    NewInteractTargetEntity.Add<ck::FTag_InteractTarget_RequiresSetup>();

    UCk_Utils_GameplayLabel_UE::Add(NewInteractTargetEntity, InParams.Get_InteractionChannel());

    RecordOfInteractTargets_Utils::AddIfMissing(InInteractTargetOwner, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfInteractTargets_Utils::Request_Connect(InInteractTargetOwner, NewInteractTargetEntity);

    return NewInteractTargetEntity;
}

auto
    UCk_Utils_InteractTarget_UE::
    AddMultiple(
        FCk_Handle& InInteractTargetOwner,
        const FCk_Fragment_MultipleInteractTarget_ParamsData& InParams,
        ECk_Replication InReplicates,
        const UObject* InWorldContextObject)
    -> TArray<FCk_Handle_InteractTarget>
{
    return ck::algo::Transform<TArray<FCk_Handle_InteractTarget>>(
        InParams.Get_InteractTargetParams(), [&](const FCk_Fragment_InteractTarget_ParamsData& InParam)
    {
        return Add(InInteractTargetOwner, InParam, InReplicates, nullptr, FInstancedStruct{}, InWorldContextObject);
    });
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_InteractTarget_UE, FCk_Handle_InteractTarget,
    ck::FFragment_InteractTarget_Params, ck::FFragment_InteractTarget_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InteractTarget_UE::
    Request_StartInteraction(
        FCk_Handle_InteractTarget& InInteractTarget,
        const FCk_Try_InteractTarget_StartInteraction& InRequest)
    -> FCk_Handle_InteractTarget
{
    InInteractTarget.AddOrGet<ck::FFragment_InteractTarget_Requests>()._Requests.Emplace(InRequest);
    return InInteractTarget;
}

auto
    UCk_Utils_InteractTarget_UE::
    Request_CancelInteraction(
        FCk_Handle_InteractTarget& InInteractTarget,
        const FCk_Request_InteractTarget_CancelInteraction& InRequest)
    -> FCk_Handle_InteractTarget
{
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

        // If no multiple interactions, don't allow interactions if any are current or pending
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
        // No duplicate interactions
        return ECk_CanInteractWithResult::AlreadyExists;
    }

    const auto& Params = InTarget.Get<ck::FFragment_InteractTarget_Params>().Get_Params();

    // If no multiple interactions, don't allow interactions there are
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
    QUICK_SCOPE_CYCLE_COUNTER(TryGet)
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
            Params.Get_CanInteractWithRef(), InTarget, InInteractSource, InInteractInstigator);
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
        const FMemberReference& InRef,
        FCk_Handle_InteractTarget InTarget,
        FCk_Handle InInteractSource,
        FCk_Handle InInteractInstigator)
    -> TOptional<bool>
{
    auto* const MemberClass = InRef.GetMemberParentClass();

    if (ck::Is_NOT_Valid(MemberClass))
    { return {}; }

    auto* const Function = InRef.ResolveMember<UFunction>(MemberClass);

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
