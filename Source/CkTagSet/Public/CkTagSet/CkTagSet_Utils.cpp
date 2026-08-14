#include "CkTagSet_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_TagSet_UE,
    FCk_Handle_TagSet,
    ck::FFragment_TagSet);

// --------------------------------------------------------------------------------------------------------------------
// Creation
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    Add(
        FCk_Handle& InHandle,
        FGameplayTagContainer InInitialTags,
        ECk_Replication InReplicates)
    -> FCk_Handle_TagSet
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Add: Invalid handle [{}]."), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_TagSet>(InInitialTags);

    if (InReplicates == ECk_Replication::Replicates)
    {
        // InHandle, never its LifetimeOwner: the container must be co-located with FFragment_TagSet.
        // See CkTagSet/CLAUDE.md § Persistence.
        UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_TagSet>(InHandle);

        if (InInitialTags.Num() > 0)
        {
            auto TagSetHandle = CastChecked(InHandle);
            Request_TryReplicateTagSet(TagSetHandle);
        }
    }

    return CastChecked(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    Create(
        FCk_Handle& InOwner,
        FGameplayTagContainer InInitialTags,
        ECk_Replication InReplicates)
    -> FCk_Handle_TagSet
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    return Add(NewEntity, InInitialTags, InReplicates);
}

// --------------------------------------------------------------------------------------------------------------------
// Queries
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    Get_Tags(
        const FCk_Handle_TagSet& InTagSet)
    -> FGameplayTagContainer
{
    return InTagSet.Get<ck::FFragment_TagSet>().Get_Tags();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    Get_NumTags(
        const FCk_Handle_TagSet& InTagSet)
    -> int32
{
    return InTagSet.Get<ck::FFragment_TagSet>().Get_Tags().Num();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    HasTag(
        const FCk_Handle_TagSet& InTagSet,
        FGameplayTag InTag)
    -> bool
{
    return InTagSet.Get<ck::FFragment_TagSet>().Get_Tags().HasTag(InTag);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    HasTagExact(
        const FCk_Handle_TagSet& InTagSet,
        FGameplayTag InTag)
    -> bool
{
    return InTagSet.Get<ck::FFragment_TagSet>().Get_Tags().HasTagExact(InTag);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    HasAny(
        const FCk_Handle_TagSet& InTagSet,
        FGameplayTagContainer InTags)
    -> bool
{
    return InTagSet.Get<ck::FFragment_TagSet>().Get_Tags().HasAny(InTags);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    HasAnyExact(
        const FCk_Handle_TagSet& InTagSet,
        FGameplayTagContainer InTags)
    -> bool
{
    return InTagSet.Get<ck::FFragment_TagSet>().Get_Tags().HasAnyExact(InTags);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    HasAll(
        const FCk_Handle_TagSet& InTagSet,
        FGameplayTagContainer InTags)
    -> bool
{
    return InTagSet.Get<ck::FFragment_TagSet>().Get_Tags().HasAll(InTags);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    HasAllExact(
        const FCk_Handle_TagSet& InTagSet,
        FGameplayTagContainer InTags)
    -> bool
{
    return InTagSet.Get<ck::FFragment_TagSet>().Get_Tags().HasAllExact(InTags);
}

// --------------------------------------------------------------------------------------------------------------------
// Requests (Authority Only)
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    Request_AddTags(
        FCk_Handle_TagSet& InTagSet,
        const FGameplayTagContainer& InTags,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    const auto TagSetIsValid = ck::IsValid(InTagSet);
    CK_ENSURE_IF_NOT(TagSetIsValid,
        TEXT("Request_AddTags: Invalid TagSet handle [{}].{}"), InTagSet, ck::Context(InTagSet))
    {
        InDelegate.ExecuteIfBound(InTagSet, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InTagSet);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_AddTags: No authority over entity [{}].{}"), InTagSet, ck::Context(InTagSet))
    {
        InDelegate.ExecuteIfBound(InTagSet, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    const auto Request = FCk_Request_TagSet_AddTags{InTags};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InTagSet.AddOrGet<ck::FFragment_TagSet_Requests>()._Requests.Emplace(Request);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    Request_AddTag(
        FCk_Handle_TagSet& InTagSet,
        FGameplayTag InTag,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    auto Container = FGameplayTagContainer{};
    Container.AddTag(InTag);
    Request_AddTags(InTagSet, Container, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    Request_RemoveTags(
        FCk_Handle_TagSet& InTagSet,
        const FGameplayTagContainer& InTags,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    const auto TagSetIsValid = ck::IsValid(InTagSet);
    CK_ENSURE_IF_NOT(TagSetIsValid,
        TEXT("Request_RemoveTags: Invalid TagSet handle [{}].{}"), InTagSet, ck::Context(InTagSet))
    {
        InDelegate.ExecuteIfBound(InTagSet, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    const auto HasAuthority = UCk_Utils_Net_UE::Get_HasAuthority(InTagSet);
    CK_ENSURE_IF_NOT(HasAuthority,
        TEXT("Request_RemoveTags: No authority over entity [{}].{}"), InTagSet, ck::Context(InTagSet))
    {
        InDelegate.ExecuteIfBound(InTagSet, ECk_Request_OperationResult::Failed_NotEnqueued);
        return;
    }

    const auto Request = FCk_Request_TagSet_RemoveTags{InTags};

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    InTagSet.AddOrGet<ck::FFragment_TagSet_Requests>()._Requests.Emplace(Request);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    Request_RemoveTag(
        FCk_Handle_TagSet& InTagSet,
        FGameplayTag InTag,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> void
{
    auto Container = FGameplayTagContainer{};
    Container.AddTag(InTag);
    Request_RemoveTags(InTagSet, Container, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------
// Signals
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    BindTo_OnTagsChanged(
        FCk_Handle_TagSet& InTagSet,
        const FCk_Delegate_TagSet_OnTagsChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> void
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_TagSet_OnTagsChanged, InTagSet, InDelegate, InBindingPolicy, InPostFireBehavior);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    UnbindFrom_OnTagsChanged(
        FCk_Handle_TagSet& InTagSet,
        const FCk_Delegate_TagSet_OnTagsChanged& InDelegate)
    -> void
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_TagSet_OnTagsChanged, InTagSet, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------
// Internal
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TagSet_UE::
    Request_TryReplicateTagSet(
        FCk_Handle_TagSet& InTagSet)
    -> void
{
    InTagSet.AddOrGet<ck::FTag_TagSet_MayRequireReplication>();
}

// --------------------------------------------------------------------------------------------------------------------
