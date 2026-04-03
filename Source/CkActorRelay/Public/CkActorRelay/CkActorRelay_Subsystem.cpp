#include "CkActorRelay_Subsystem.h"

#include "CkActorRelay_GroupSubsystem.h"

#include "CkActorRelay/CkActorRelay_Log.h"

#include "CkCore/Debug/CkDebug_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ActorRelay_Subsystem_UE::
    Get_GroupSubsystem(
        const FGameplayTag& InGroupTag) const
    -> UCk_ActorRelay_Group_Subsystem_Base_UE*
{
    auto FoundSubsystem = _RegisteredGroups.Find(InGroupTag);

    CK_ENSURE_IF_NOT(FoundSubsystem != nullptr,
        TEXT("No ActorRelay group registered for tag [{}]"), InGroupTag)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(FoundSubsystem->Get()),
        TEXT("ActorRelay group subsystem for tag [{}] is no longer valid"), InGroupTag)
    { return {}; }

    return FoundSubsystem->Get();
}

auto
    UCk_ActorRelay_Subsystem_UE::
    Request_AcquireChannel(
        const FGameplayTag& InGroupTag)
    -> FCk_ActorRelay_ChannelResult
{
    auto GroupSubsystem = Get_GroupSubsystem(InGroupTag);

    CK_ENSURE_IF_NOT(ck::IsValid(GroupSubsystem),
        TEXT("Cannot acquire channel: group subsystem for tag [{}] is invalid"), InGroupTag)
    { return {}; }

    return GroupSubsystem->Request_AcquireChannel();
}

auto
    UCk_ActorRelay_Subsystem_UE::
    Request_AcquireChannel_ForPlayer(
        const FGameplayTag& InGroupTag,
        APlayerState* InPlayerState)
    -> FCk_ActorRelay_ChannelResult
{
    auto GroupSubsystem = Get_GroupSubsystem(InGroupTag);

    CK_ENSURE_IF_NOT(ck::IsValid(GroupSubsystem),
        TEXT("Cannot acquire channel for player: group subsystem for tag [{}] is invalid"), InGroupTag)
    { return {}; }

    return GroupSubsystem->Request_AcquireChannel_ForPlayer(InPlayerState);
}

auto
    UCk_ActorRelay_Subsystem_UE::
    DoRegisterGroup(
        UCk_ActorRelay_Group_Subsystem_Base_UE* InGroupSubsystem)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InGroupSubsystem),
        TEXT("Attempting to register an invalid group subsystem"))
    { return; }

    const auto GroupTag = InGroupSubsystem->Get_GroupTag();

    CK_ENSURE_IF_NOT(GroupTag.IsValid(),
        TEXT("Attempting to register a group subsystem with an invalid GroupTag"))
    { return; }

    CK_ENSURE_IF_NOT(NOT _RegisteredGroups.Contains(GroupTag),
        TEXT("Duplicate ActorRelay group registration for tag [{}]"), GroupTag)
    { return; }

    _RegisteredGroups.Add(GroupTag, InGroupSubsystem);

    ck::actorrelay::Log(TEXT("Registered ActorRelay group [{}] with subsystem [{}]"),
        GroupTag, InGroupSubsystem->GetClass()->GetName());
}

auto
    UCk_ActorRelay_Subsystem_UE::
    DoUnregisterGroup(
        UCk_ActorRelay_Group_Subsystem_Base_UE* InGroupSubsystem)
    -> void
{
    if (ck::Is_NOT_Valid(InGroupSubsystem))
    { return; }

    const auto GroupTag = InGroupSubsystem->Get_GroupTag();

    if (NOT GroupTag.IsValid())
    { return; }

    _RegisteredGroups.Remove(GroupTag);

    ck::actorrelay::Verbose(TEXT("Unregistered ActorRelay group [{}]"), GroupTag);
}

// --------------------------------------------------------------------------------------------------------------------
