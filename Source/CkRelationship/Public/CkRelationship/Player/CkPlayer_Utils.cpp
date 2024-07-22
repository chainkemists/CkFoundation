#include "CkPlayer_Utils.h"

#include "CkRelationship/CkRelationship_Log.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(Player, UCk_Utils_Player_UE, FCk_Handle_Player, ck::FFragment_PlayerInfo)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Player_UE::
    Add(
        FCk_Handle& InHandle,
        ECk_Player_ID InPlayerID,
        ECk_Replication InReplicates)
    -> FCk_Handle_Player
{
    switch (InPlayerID)
    {
        case ECk_Player_ID::Zero: { Assign<ECk_Player_ID::Zero>(InHandle); break; }
        case ECk_Player_ID::One: { Assign<ECk_Player_ID::One>(InHandle); break; }
        case ECk_Player_ID::Two: { Assign<ECk_Player_ID::Two>(InHandle); break; }
        case ECk_Player_ID::Three: { Assign<ECk_Player_ID::Three>(InHandle); break; }
        case ECk_Player_ID::Four: { Assign<ECk_Player_ID::Four>(InHandle); break; }
        case ECk_Player_ID::Five: { Assign<ECk_Player_ID::Five>(InHandle); break; }
        case ECk_Player_ID::Six: { Assign<ECk_Player_ID::Six>(InHandle); break; }
        case ECk_Player_ID::Seven: { Assign<ECk_Player_ID::Seven>(InHandle); break; }
        case ECk_Player_ID::Eight: { Assign<ECk_Player_ID::Eight>(InHandle); break; }
        case ECk_Player_ID::Unassigned: { Assign<ECk_Player_ID::Unassigned>(InHandle); break; }
        default:
        {
           CK_INVALID_ENUM(InPlayerID);
           break;
        }
    }

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::relationship::VeryVerbose
        (
            TEXT("Skipping creation of Player Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );
    }
    else
    {
        UCk_Utils_Ecs_Net_UE::TryAddReplicatedFragment<UCk_Fragment_Player_Rep>(InHandle);
    }

    return Cast(InHandle);
}

auto
    UCk_Utils_Player_UE::
    Assign(
        FCk_Handle& InHandle,
        ECk_Player_ID InPlayerID,
        ECk_Replication InReplicates)
    -> FCk_Handle_Player
{
    const auto OldID = Has(InHandle) ? Get_ID(Cast(InHandle)) : ECk_Player_ID::Unassigned;

    if (OldID == InPlayerID)
    { return Cast(InHandle); } // TODO: performing the cast twice (on in Get_ID above), can we fix this?

    if (Has(InHandle))
    {
        auto PlayerHandle = Cast(InHandle);
        DoRemoveExistingPlayer(PlayerHandle);
    }

    switch (InPlayerID)
    {
        case ECk_Player_ID::Zero: { Assign<ECk_Player_ID::Zero>(InHandle); break; }
        case ECk_Player_ID::One: { Assign<ECk_Player_ID::One>(InHandle); break; }
        case ECk_Player_ID::Two: { Assign<ECk_Player_ID::Two>(InHandle); break; }
        case ECk_Player_ID::Three: { Assign<ECk_Player_ID::Three>(InHandle); break; }
        case ECk_Player_ID::Four: { Assign<ECk_Player_ID::Four>(InHandle); break; }
        case ECk_Player_ID::Five: { Assign<ECk_Player_ID::Five>(InHandle); break; }
        case ECk_Player_ID::Six: { Assign<ECk_Player_ID::Six>(InHandle); break; }
        case ECk_Player_ID::Seven: { Assign<ECk_Player_ID::Seven>(InHandle); break; }
        case ECk_Player_ID::Eight: { Assign<ECk_Player_ID::Eight>(InHandle); break; }
        case ECk_Player_ID::Unassigned: { Assign<ECk_Player_ID::Eight>(InHandle); break; }
        case ECk_Player_ID::Unassigned: { Assign<ECk_Player_ID::Unassigned>(InHandle); break; }
        default:
        {
           CK_INVALID_ENUM(InPlayerID);
           break;
        }
    }

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_Player_Rep>(InHandle, [&](UCk_Fragment_Player_Rep* InRepComp)
    {
        InRepComp->Broadcast_Assign(InPlayerID);
    });

    ck::UUtils_Signal_PlayerChanged::Broadcast(InHandle, ck::MakePayload(TryGet_Entity_Player_InOwnershipChain(InHandle), OldID, InPlayerID));

    return Cast(InHandle);
}

auto
    UCk_Utils_Player_UE::
    Unassign(
        FCk_Handle_Player& InHandle)
    -> FCk_Handle
{
    return Assign(InHandle, ECk_Player_ID::Unassigned);
}

auto
    UCk_Utils_Player_UE::
    Get_IsAssignedTo(
        const FCk_Handle_Player& InHandle,
        ECk_Player_ID InPlayerID)
    -> bool
{
    switch (InPlayerID)
    {
        case ECk_Player_ID::Zero: { return Get_IsAssignedTo<ECk_Player_ID::Zero>(InHandle); }
        case ECk_Player_ID::One: { return Get_IsAssignedTo<ECk_Player_ID::One>(InHandle); }
        case ECk_Player_ID::Two: { return Get_IsAssignedTo<ECk_Player_ID::Two>(InHandle); }
        case ECk_Player_ID::Three: { return Get_IsAssignedTo<ECk_Player_ID::Three>(InHandle); }
        case ECk_Player_ID::Four: { return Get_IsAssignedTo<ECk_Player_ID::Four>(InHandle); }
        case ECk_Player_ID::Five: { return Get_IsAssignedTo<ECk_Player_ID::Five>(InHandle); }
        case ECk_Player_ID::Six: { return Get_IsAssignedTo<ECk_Player_ID::Six>(InHandle); }
        case ECk_Player_ID::Seven: { return Get_IsAssignedTo<ECk_Player_ID::Seven>(InHandle); }
        case ECk_Player_ID::Eight: { return Get_IsAssignedTo<ECk_Player_ID::Eight>(InHandle); }
        case ECk_Player_ID::Unassigned: { return Get_IsAssignedTo<ECk_Player_ID::Unassigned>(InHandle); }
        default:
        {
           CK_INVALID_ENUM(InPlayerID);
           return {};
        }
    }
}

auto
    UCk_Utils_Player_UE::
    Get_IsSame(
        const FCk_Handle_Player& InHandleA,
        const FCk_Handle_Player& InHandleB)
    -> bool
{
    return Get_ID(InHandleA) == Get_ID(InHandleB);
}

auto
    UCk_Utils_Player_UE::
    Get_ID(
        const FCk_Handle_Player& InHandle)
    -> ECk_Player_ID
{
    using namespace ck;

    if (InHandle.Has<FTag_PlayerID<ECk_Player_ID::Zero>>())
    { return ECk_Player_ID::Zero; }

    if (InHandle.Has<FTag_PlayerID<ECk_Player_ID::One>>())
    { return ECk_Player_ID::One; }

    if (InHandle.Has<FTag_PlayerID<ECk_Player_ID::Two>>())
    { return ECk_Player_ID::Two; }

    if (InHandle.Has<FTag_PlayerID<ECk_Player_ID::Three>>())
    { return ECk_Player_ID::Three; }

    if (InHandle.Has<FTag_PlayerID<ECk_Player_ID::Four>>())
    { return ECk_Player_ID::Four; }

    if (InHandle.Has<FTag_PlayerID<ECk_Player_ID::Five>>())
    { return ECk_Player_ID::Five; }

    if (InHandle.Has<FTag_PlayerID<ECk_Player_ID::Six>>())
    { return ECk_Player_ID::Six; }

    if (InHandle.Has<FTag_PlayerID<ECk_Player_ID::Seven>>())
    { return ECk_Player_ID::Seven; }

    if (InHandle.Has<FTag_PlayerID<ECk_Player_ID::Eight>>())
    { return ECk_Player_ID::Eight; }

    if (InHandle.Has<FTag_PlayerID<ECk_Player_ID::Unassigned>>())
    { return ECk_Player_ID::Unassigned; }

    CK_TRIGGER_ENSURE(TEXT("Entity [{}] has been assigned an out of range Player ID"), InHandle);

    return ECk_Player_ID::Unassigned;
}

auto
    UCk_Utils_Player_UE::
    TryGet_Entity_Player_InOwnershipChain(
        const FCk_Handle& InHandle)
    -> FCk_Handle_Player
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
    UCk_Utils_Player_UE::
    ForEachEntity_OnOpposingPlayer(
        FCk_Handle& InHandle,
        ECk_Player_ID InPlayer,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Player>
{
    auto Entities = TArray<FCk_Handle_Player>{};

    ForEachEntity_OnOpposingPlayer(InHandle, InPlayer, [&](FCk_Entity InPlayerEntity)
    {
        auto PlayerHandle = Cast(ck::MakeHandle(InPlayerEntity, InHandle));

        if (InDelegate.IsBound())
        { InDelegate.Execute(PlayerHandle, InOptionalPayload); }
        else
        { Entities.Emplace(PlayerHandle); }
    });

    return Entities;
}

auto
    UCk_Utils_Player_UE::
    ForEachEntity_OnSamePlayer(
        FCk_Handle& InHandle,
        ECk_Player_ID InPlayer,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Player>
{
    auto Entities = TArray<FCk_Handle_Player>{};

    ForEachEntity_OnSamePlayer(InHandle, InPlayer, [&](FCk_Entity InPlayerEntity)
    {
        auto PlayerHandle = Cast(ck::MakeHandle(InPlayerEntity, InHandle));

        if (InDelegate.IsBound())
        { InDelegate.Execute(PlayerHandle, InOptionalPayload); }
        else
        { Entities.Emplace(PlayerHandle); }
    });

    return Entities;
}

auto
    UCk_Utils_Player_UE::
    DoRemoveExistingPlayer(
        FCk_Handle_Player& InHandle)
    -> void
{
    using namespace ck;

    switch (const auto& PlayerID = Get_ID(InHandle))
    {
        case ECk_Player_ID::Zero: { InHandle.Remove<FTag_PlayerID<ECk_Player_ID::Zero>>(); break; }
        case ECk_Player_ID::One: { InHandle.Remove<FTag_PlayerID<ECk_Player_ID::One>>(); break; }
        case ECk_Player_ID::Two: { InHandle.Remove<FTag_PlayerID<ECk_Player_ID::Two>>(); break; }
        case ECk_Player_ID::Three: { InHandle.Remove<FTag_PlayerID<ECk_Player_ID::Three>>(); break; }
        case ECk_Player_ID::Four: { InHandle.Remove<FTag_PlayerID<ECk_Player_ID::Four>>(); break; }
        case ECk_Player_ID::Five: { InHandle.Remove<FTag_PlayerID<ECk_Player_ID::Five>>(); break; }
        case ECk_Player_ID::Six: { InHandle.Remove<FTag_PlayerID<ECk_Player_ID::Six>>(); break; }
        case ECk_Player_ID::Seven: { InHandle.Remove<FTag_PlayerID<ECk_Player_ID::Seven>>(); break; }
        case ECk_Player_ID::Eight: { InHandle.Remove<FTag_PlayerID<ECk_Player_ID::Eight>>(); break; }
        case ECk_Player_ID::Unassigned: { InHandle.Remove<FTag_PlayerID<ECk_Player_ID::Unassigned>>(); break; }
        default:
        {
            CK_INVALID_ENUM(PlayerID);
            break;
        }
    }

    InHandle.Remove<ck::FFragment_PlayerInfo>();
}

// --------------------------------------------------------------------------------------------------------------------
