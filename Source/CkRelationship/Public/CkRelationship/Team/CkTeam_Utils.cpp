#include "CkTeam_Utils.h"

#include "CkRelationship/CkRelationship_Log.h"
#include "CkRelationship/Team/CkTeam_Fragment.h"
#include "CkRelationship/Team/CkTeam_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Team_UE, FCk_Handle_Team, ck::FFragment_TeamInfo)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Team_UE::
    Add(
        FCk_Handle& InHandle,
        ECk_Team_ID InTeamID,
        ECk_Replication InReplicates)
    -> FCk_Handle_Team
{
    switch (InTeamID)
    {
        case ECk_Team_ID::Zero:       { Assign<ECk_Team_ID::Zero>(InHandle); break; }
        case ECk_Team_ID::One:        { Assign<ECk_Team_ID::One>(InHandle); break; }
        case ECk_Team_ID::Two:        { Assign<ECk_Team_ID::Two>(InHandle); break; }
        case ECk_Team_ID::Three:      { Assign<ECk_Team_ID::Three>(InHandle); break; }
        case ECk_Team_ID::Four:       { Assign<ECk_Team_ID::Four>(InHandle); break; }
        case ECk_Team_ID::Five:       { Assign<ECk_Team_ID::Five>(InHandle); break; }
        case ECk_Team_ID::Six:        { Assign<ECk_Team_ID::Six>(InHandle); break; }
        case ECk_Team_ID::Seven:      { Assign<ECk_Team_ID::Seven>(InHandle); break; }
        case ECk_Team_ID::Eight:      { Assign<ECk_Team_ID::Eight>(InHandle); break; }
        case ECk_Team_ID::Unassigned: { Assign<ECk_Team_ID::Unassigned>(InHandle); break; }
        default:
        {
           CK_INVALID_ENUM(InTeamID);
           break;
        }
    }

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::relationship::VeryVerbose
        (
            TEXT("Skipping creation of Team Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );
    }
    else
    {
        UCk_Utils_Ecs_Net_UE::TryAddReplicatedFragment<UCk_Fragment_Team_Rep>(InHandle);
    }

    return Cast(InHandle);
}

auto
    UCk_Utils_Team_UE::
    Assign(
        FCk_Handle_Team& InHandle,
        ECk_Team_ID InTeamID)
    -> FCk_Handle_Team
{
    const auto OldID = Has(InHandle) ? Get_ID(Cast(InHandle)) : ECk_Team_ID::Unassigned;

    if (OldID == InTeamID)
    { return Cast(InHandle); } // TODO: performing the cast twice (on in Get_ID above), can we fix this?

    if (Has(InHandle))
    {
        auto TeamHandle = Cast(InHandle);
        DoRemoveExistingTeam(TeamHandle);
    }

    switch (InTeamID)
    {
        case ECk_Team_ID::Zero:       { Assign<ECk_Team_ID::Zero>(InHandle); break; }
        case ECk_Team_ID::One:        { Assign<ECk_Team_ID::One>(InHandle); break; }
        case ECk_Team_ID::Two:        { Assign<ECk_Team_ID::Two>(InHandle); break; }
        case ECk_Team_ID::Three:      { Assign<ECk_Team_ID::Three>(InHandle); break; }
        case ECk_Team_ID::Four:       { Assign<ECk_Team_ID::Four>(InHandle); break; }
        case ECk_Team_ID::Five:       { Assign<ECk_Team_ID::Five>(InHandle); break; }
        case ECk_Team_ID::Six:        { Assign<ECk_Team_ID::Six>(InHandle); break; }
        case ECk_Team_ID::Seven:      { Assign<ECk_Team_ID::Seven>(InHandle); break; }
        case ECk_Team_ID::Eight:      { Assign<ECk_Team_ID::Eight>(InHandle); break; }
        case ECk_Team_ID::Unassigned: { Assign<ECk_Team_ID::Unassigned>(InHandle); break; }
        default:
        {
           CK_INVALID_ENUM(InTeamID);
           break;
        }
    }

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_Team_Rep>(InHandle, [&](UCk_Fragment_Team_Rep* InRepComp)
    {
        InRepComp->Broadcast_Assign(InTeamID);
    });

    ck::UUtils_Signal_TeamChanged::Broadcast(InHandle, ck::MakePayload(TryGet_Entity_Team_InOwnershipChain(InHandle), OldID, InTeamID));

    return Cast(InHandle);
}

auto
    UCk_Utils_Team_UE::
    Unassign(
        FCk_Handle_Team& InHandle)
    -> FCk_Handle_Team
{
    InHandle.Remove<ck::FTag_OnTeamAssigned_Setup>();
    return Assign(InHandle, ECk_Team_ID::Unassigned);
}

auto
    UCk_Utils_Team_UE::
    Get_IsAssignedTo(
        const FCk_Handle_Team& InHandle,
        ECk_Team_ID InTeamID)
    -> bool
{
    switch (InTeamID)
    {
        case ECk_Team_ID::Zero:       { return Get_IsAssignedTo<ECk_Team_ID::Zero>(InHandle); }
        case ECk_Team_ID::One:        { return Get_IsAssignedTo<ECk_Team_ID::One>(InHandle); }
        case ECk_Team_ID::Two:        { return Get_IsAssignedTo<ECk_Team_ID::Two>(InHandle); }
        case ECk_Team_ID::Three:      { return Get_IsAssignedTo<ECk_Team_ID::Three>(InHandle); }
        case ECk_Team_ID::Four:       { return Get_IsAssignedTo<ECk_Team_ID::Four>(InHandle); }
        case ECk_Team_ID::Five:       { return Get_IsAssignedTo<ECk_Team_ID::Five>(InHandle); }
        case ECk_Team_ID::Six:        { return Get_IsAssignedTo<ECk_Team_ID::Six>(InHandle); }
        case ECk_Team_ID::Seven:      { return Get_IsAssignedTo<ECk_Team_ID::Seven>(InHandle); }
        case ECk_Team_ID::Eight:      { return Get_IsAssignedTo<ECk_Team_ID::Eight>(InHandle); }
        case ECk_Team_ID::Unassigned: { return Get_IsAssignedTo<ECk_Team_ID::Unassigned>(InHandle); }
        default:
        {
           CK_INVALID_ENUM(InTeamID);
           return {};
        }
    }
}

auto
    UCk_Utils_Team_UE::
    Get_IsSame(
        const FCk_Handle_Team& InHandleA,
        const FCk_Handle_Team& InHandleB)
    -> bool
{
    return Get_ID(InHandleA) == Get_ID(InHandleB);
}

auto
    UCk_Utils_Team_UE::
    Get_ID(
        const FCk_Handle_Team& InHandle)
    -> ECk_Team_ID
{
    using namespace ck;

    if (InHandle.Has<FTag_TeamID<ECk_Team_ID::Zero>>())       { return ECk_Team_ID::Zero; }
    if (InHandle.Has<FTag_TeamID<ECk_Team_ID::One>>())        { return ECk_Team_ID::One; }
    if (InHandle.Has<FTag_TeamID<ECk_Team_ID::Two>>())        { return ECk_Team_ID::Two; }
    if (InHandle.Has<FTag_TeamID<ECk_Team_ID::Three>>())      { return ECk_Team_ID::Three; }
    if (InHandle.Has<FTag_TeamID<ECk_Team_ID::Four>>())       { return ECk_Team_ID::Four; }
    if (InHandle.Has<FTag_TeamID<ECk_Team_ID::Five>>())       { return ECk_Team_ID::Five; }
    if (InHandle.Has<FTag_TeamID<ECk_Team_ID::Six>>())        { return ECk_Team_ID::Six; }
    if (InHandle.Has<FTag_TeamID<ECk_Team_ID::Seven>>())      { return ECk_Team_ID::Seven; }
    if (InHandle.Has<FTag_TeamID<ECk_Team_ID::Eight>>())      { return ECk_Team_ID::Eight; }
    if (InHandle.Has<FTag_TeamID<ECk_Team_ID::Unassigned>>()) { return ECk_Team_ID::Unassigned; }

    CK_TRIGGER_ENSURE(TEXT("Entity [{}] has been assigned an out of range Team ID"), InHandle);

    return ECk_Team_ID::Unassigned;
}

auto
    UCk_Utils_Team_UE::
    TryGet_Entity_Team_InOwnershipChain(
        const FCk_Handle& InHandle)
    -> FCk_Handle_Team
{
    auto MaybeOwner = UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(InHandle,
    [&](const FCk_Handle& Handle)
    {
        return Has(Handle);
    });

    if (ck::Is_NOT_Valid(MaybeOwner))
    { return {}; }

    return Cast(MaybeOwner);
}

auto
    UCk_Utils_Team_UE::
    Conv_TeamToGenericTeam(
        const FCk_Handle_Team& InHandle)
    -> FGenericTeamId
{
    const auto ID = Get_ID(InHandle);
    return FGenericTeamId{static_cast<uint8>(ID)};
}

auto
    UCk_Utils_Team_UE::
    Conv_TeamEnumToGenericTeam(
        const FGenericTeamId& InGenericTeamId)
    -> ECk_Team_ID
{
    CK_ENSURE_IF_NOT(InGenericTeamId.GetId() <= static_cast<uint8>(ECk_Team_ID::Unassigned),
        TEXT("Unable to Convert GenericTeamId [{}] to ECk_Team_ID because the generic ID is out of range of [{}]"),
        InGenericTeamId, ck::Get_RuntimeTypeToString<ECk_Team_ID>())
    { return ECk_Team_ID::Unassigned; }

    return static_cast<ECk_Team_ID>(InGenericTeamId.GetId());
}

auto
    UCk_Utils_Team_UE::
    ForEachEntity_OnOpposingTeam(
        FCk_Handle& InHandle,
        ECk_Team_ID InTeam,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Team>
{
    auto Entities = TArray<FCk_Handle_Team>{};

    ForEachEntity_OnOpposingTeam(InHandle, InTeam, [&](FCk_Entity InTeamEntity)
    {
        auto TeamHandle = Cast(ck::MakeHandle(InTeamEntity, InHandle));

        if (InDelegate.IsBound())
        { InDelegate.Execute(TeamHandle, InOptionalPayload); }
        else
        { Entities.Emplace(TeamHandle); }
    });

    return Entities;
}

auto
    UCk_Utils_Team_UE::
    ForEachEntity_OnSameTeam(
        FCk_Handle& InHandle,
        ECk_Team_ID InTeam,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Team>
{
    auto Entities = TArray<FCk_Handle_Team>{};

    ForEachEntity_OnSameTeam(InHandle, InTeam, [&](FCk_Entity InTeamEntity)
    {
        auto TeamHandle = Cast(ck::MakeHandle(InTeamEntity, InHandle));

        if (InDelegate.IsBound())
        { InDelegate.Execute(TeamHandle, InOptionalPayload); }
        else
        { Entities.Emplace(TeamHandle); }
    });

    return Entities;
}

auto
    UCk_Utils_Team_UE::
    BindTo_OnTeamChanged(
        FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_TeamChanged& InDelegate)
    -> FCk_Handle_Team
{
    auto TeamHandle = TryGet_Entity_Team_InOwnershipChain(InHandle);

    CK_ENSURE_IF_NOT(ck::IsValid(TeamHandle),
        TEXT("Unable to BindTo_OnTeamChanged on Entity [{}] as there is no Team assigned in ownership chain.{}"),
        InHandle, ck::Context(InDelegate.GetFunctionName()))
    { return {}; }

    CK_SIGNAL_BIND(ck::UUtils_Signal_TeamChanged, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    return TeamHandle;
}

auto
    UCk_Utils_Team_UE::
    UnbindFrom_OnTeamChanged(
        FCk_Handle& InHandle,
        const FCk_Delegate_TeamChanged& InDelegate)
    -> FCk_Handle_Team
{
    auto TeamHandle = TryGet_Entity_Team_InOwnershipChain(InHandle);

    CK_ENSURE_IF_NOT(ck::IsValid(TeamHandle),
        TEXT("Unable to UnbindFrom_OnTeamChanged on Entity [{}] as there is no Team assigned in ownership chain.{}"),
        InHandle, ck::Context(InDelegate.GetFunctionName()))
    { return {}; }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_TeamChanged, InHandle, InDelegate);
    return TeamHandle;
}

auto
    UCk_Utils_Team_UE::
    DoRemoveExistingTeam(
        FCk_Handle_Team& InHandle)
    -> void
{
    using namespace ck;

    switch (const auto& TeamID = Get_ID(InHandle))
    {
        case ECk_Team_ID::Zero: { InHandle.Remove<FTag_TeamID<ECk_Team_ID::Zero>>(); break; }
        case ECk_Team_ID::One: { InHandle.Remove<FTag_TeamID<ECk_Team_ID::One>>(); break; }
        case ECk_Team_ID::Two: { InHandle.Remove<FTag_TeamID<ECk_Team_ID::Two>>(); break; }
        case ECk_Team_ID::Three: { InHandle.Remove<FTag_TeamID<ECk_Team_ID::Three>>(); break; }
        case ECk_Team_ID::Four: { InHandle.Remove<FTag_TeamID<ECk_Team_ID::Four>>(); break; }
        case ECk_Team_ID::Five: { InHandle.Remove<FTag_TeamID<ECk_Team_ID::Five>>(); break; }
        case ECk_Team_ID::Six: { InHandle.Remove<FTag_TeamID<ECk_Team_ID::Six>>(); break; }
        case ECk_Team_ID::Seven: { InHandle.Remove<FTag_TeamID<ECk_Team_ID::Seven>>(); break; }
        case ECk_Team_ID::Eight: { InHandle.Remove<FTag_TeamID<ECk_Team_ID::Eight>>(); break; }
        case ECk_Team_ID::Unassigned: { InHandle.Remove<FTag_TeamID<ECk_Team_ID::Unassigned>>(); break; }
        default:
        {
            CK_INVALID_ENUM(TeamID);
            break;
        }
    }

    InHandle.Remove<ck::FFragment_TeamInfo>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Team_Listener_UE::
    BindTo_OnTeamAssignedToAnyEntity(
        FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_TeamAssigned& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_TeamAssigned, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    DoAddTeamListener(InHandle);
    return InHandle;
}

auto
UCk_Utils_Team_Listener_UE::
UnbindFrom_OnTeamAssigned(
    FCk_Handle& InHandle,
    const FCk_Delegate_TeamAssigned& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_TeamAssigned, InHandle, InDelegate);
    // TODO: figure out a bullet-proof way to remove the FTag_TeamListener if ALL the delegates have been unbound
    return InHandle;
}

auto
    UCk_Utils_Team_Listener_UE::
    BindTo_OnTeamAssignedToAnyEntity_OnSameTeam(
        FCk_Handle_Team& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_TeamAssigned& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_TeamAssigned_OnSameTeam, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    DoAddTeamListener(InHandle);
    return InHandle;
}

auto
    UCk_Utils_Team_Listener_UE::
    UnbindFrom_OnTeamAssignedToAnyEntity_OnSameTeam(
        FCk_Handle_Team& InHandle,
        const FCk_Delegate_TeamAssigned& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_TeamAssigned_OnSameTeam, InHandle, InDelegate);
    // TODO: figure out a bullet-proof way to remove the FTag_TeamListener if ALL the delegates have been unbound
    return InHandle;
}

auto
UCk_Utils_Team_Listener_UE::
BindTo_OnTeamAssignedToAnyEntity_OnOpposingTeam(
    FCk_Handle_Team& InHandle,
    ECk_Signal_BindingPolicy InBindingPolicy,
    ECk_Signal_PostFireBehavior InPostFireBehavior,
    const FCk_Delegate_TeamAssigned& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_TeamAssigned_OnOpposingTeam, InHandle, InDelegate, InBindingPolicy, InPostFireBehavior);
    DoAddTeamListener(InHandle);
    return InHandle;
}

auto
UCk_Utils_Team_Listener_UE::
UnbindFrom_OnTeamAssignedToAnyEntity_OnOpposingTeam(
    FCk_Handle_Team& InHandle,
    const FCk_Delegate_TeamAssigned& InDelegate)
    -> FCk_Handle
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_TeamAssigned_OnOpposingTeam, InHandle, InDelegate);
    // TODO: figure out a bullet-proof way to remove the FTag_TeamListener if ALL the delegates have been unbound
    return InHandle;
}

auto
    UCk_Utils_Team_Listener_UE::
    DoAddTeamListener(
        FCk_Handle& InHandle)
    -> void
{
    if (NOT InHandle.Has<ck::FTag_TeamListener>())
    {
        InHandle.AddOrGet<ck::FTag_TeamListener_Setup>();
        InHandle.AddOrGet<ck::FTag_TeamListener>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
