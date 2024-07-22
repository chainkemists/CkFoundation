#pragma once

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"

#include "CkNet/CkNet_Utils.h"

#include "CkRelationship/Player/CkPlayer_Fragment.h"
#include "CkRelationship/Player/CkPlayer_Fragment_Data.h"

#include "CkPlayer_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKRELATIONSHIP_API UCk_Utils_Player_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Player_UE);

public:
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Player);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Relationship|Player",
              DisplayName="[Ck][Player] Add Feature")
    static FCk_Handle_Player
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Player_ID InTeamID = ECk_Player_ID::Unassigned,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Relationship|Player",
              meta = (Keywords = "add, feature"),
              DisplayName="[Ck][Player] Assign to Player")
    static FCk_Handle_Player
    Assign(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Player_ID InPlayerID,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Relationship|Player",
              meta = (Keywords = "remove, feature"),
              DisplayName="[Ck][Player] Unassign from Player")
    static FCk_Handle
    Unassign(
        UPARAM(ref) FCk_Handle_Player& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Relationship|Player",
              DisplayName = "[Ck][Player] Is Assigned to Player")
    static bool
    Get_IsAssignedTo(
        const FCk_Handle_Player& InHandle,
        ECk_Player_ID InPlayerID);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Relationship|Player",
              DisplayName = "[Ck][Player] Is Assigned To Same Player")
    static bool
    Get_IsSame(
        const FCk_Handle_Player& InHandleA,
        const FCk_Handle_Player& InHandleB);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Relationship|Player",
              DisplayName = "[Ck][Player] Get Player ID")
    static ECk_Player_ID
    Get_ID(
        const FCk_Handle_Player& InHandle);

public:
    UFUNCTION(BlueprintPure,
              Category = "|Ck|Utils|Relationship|Player",
              DisplayName="[Ck][Player] Try Get Entity With Player In Ownership Chain")
    static FCk_Handle_Player
    TryGet_Entity_Player_InOwnershipChain(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "|Ck|Utils|Relationship|Player",
              DisplayName="[Ck][Player] For Each Opposing Player Entity",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_Player>
    ForEachEntity_OnOpposingPlayer(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Player_ID InPlayer,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "|Ck|Utils|Relationship|Player",
              DisplayName="[Ck][Player] For Each Same Player Entity",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_Player>
    ForEachEntity_OnSamePlayer(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Player_ID InTeam,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);

public:
    template <typename T_Func>
    static auto
    ForEachEntity_OnOpposingPlayer(
        FCk_Handle& InHandle,
        ECk_Player_ID InPlayer,
        T_Func InFunc) -> void;

    template <typename T_Func>
    static auto
    ForEachEntity_OnSamePlayer(
        FCk_Handle& InHandle,
        ECk_Player_ID InPlayer,
        T_Func InFunc) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Relationship|Player",
              DisplayName="[Ck][Player] Has Player Assigned To")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Relationship|Player",
        DisplayName="[Ck][Player] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Player
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Relationship|Player",
        DisplayName="[Ck][Player] Handle -> Player Handle",
        meta = (CompactNodeTitle = "<AsPlayer>", BlueprintAutocast))
    static FCk_Handle_Player
    DoCastChecked(
        FCk_Handle InHandle);

public:
    template <ECk_Player_ID T_ID>
    static auto
    Assign(
        FCk_Handle& InHandle) -> void;

    template <ECk_Player_ID T_ID>
    static auto
    Get_IsAssignedTo(
        const FCk_Handle& InHandle) -> bool;

    template <ECk_Player_ID T_ID, typename T_Func, typename... T_Components>
    static auto
    ForEachEntity(
        FCk_Registry& InRegistry,
        T_Func InFunc) -> void;

    template <typename T_Func, typename... T_Components>
    static auto
    ForEachEntity(
        FCk_Registry& InRegistry,
        ECk_Player_ID InPlayerID,
        T_Func InFunc) -> void;

private:
    static auto
    DoRemoveExistingPlayer(
        FCk_Handle_Player& InHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <typename T_Func>
auto
    UCk_Utils_Player_UE::
    ForEachEntity_OnOpposingPlayer(
        FCk_Handle& InHandle,
        ECk_Player_ID InPlayer,
        T_Func InFunc)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Handle [{}] is INVALID."), InHandle)
    { return; }

    const auto ForEachEntityOnPlayer = [&]<ECk_Player_ID T_PlayerId>()
    {
        if (InPlayer == T_PlayerId)
        { return; }

        ForEachEntity<T_PlayerId>(**InHandle, InFunc);
    };

    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Zero>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::One>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Two>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Three>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Four>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Five>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Six>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Seven>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Eight>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Unassigned>();
}

template <typename T_Func>
auto
    UCk_Utils_Player_UE::
    ForEachEntity_OnSamePlayer(
        FCk_Handle& InHandle,
        ECk_Player_ID InPlayer,
        T_Func InFunc)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Handle [{}] is INVALID."), InHandle)
    { return; }

    const auto ForEachEntityOnPlayer = [&]<ECk_Player_ID T_PlayerId>()
    {
        if (InPlayer != T_PlayerId)
        { return; }

        ForEachEntity<T_PlayerId>(**InHandle, InFunc);
    };

    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Zero>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::One>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Two>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Three>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Four>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Five>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Six>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Seven>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Eight>();
    ForEachEntityOnPlayer.template operator()<ECk_Player_ID::Unassigned>();
}

template <ECk_Player_ID T_ID>
auto
    UCk_Utils_Player_UE::
    Assign(
        FCk_Handle& InHandle)
    -> void
{
    InHandle.Add<ck::FTag_PlayerID<T_ID>>();
    InHandle.AddOrGet<ck::FFragment_PlayerInfo>(T_ID);
}

template <ECk_Player_ID T_ID>
auto
    UCk_Utils_Player_UE::
    Get_IsAssignedTo(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FTag_PlayerID<T_ID>>();
}

template <ECk_Player_ID T_ID, typename T_Func, typename ... T_Components>
auto
    UCk_Utils_Player_UE::
    ForEachEntity(
        FCk_Registry& InRegistry,
        T_Func InFunc)
    -> void
{
    InRegistry.View<ck::FTag_PlayerID<T_ID>, T_Components...>().ForEach(InFunc);
}

template <typename T_Func, typename ... T_Components>
auto
    UCk_Utils_Player_UE::
    ForEachEntity(
        FCk_Registry& InRegistry,
        ECk_Player_ID InPlayerID,
        T_Func InFunc)
     -> void
{
    switch (InPlayerID)
    {
        case ECk_Player_ID::Zero: { ForEachEntity<ECk_Player_ID::Zero>(InRegistry, InFunc); break; }
        case ECk_Player_ID::One: { ForEachEntity<ECk_Player_ID::One>(InRegistry, InFunc); break; }
        case ECk_Player_ID::Two: { ForEachEntity<ECk_Player_ID::Two>(InRegistry, InFunc); break; }
        case ECk_Player_ID::Three: { ForEachEntity<ECk_Player_ID::Three>(InRegistry, InFunc); break; }
        case ECk_Player_ID::Four: { ForEachEntity<ECk_Player_ID::Four>(InRegistry, InFunc); break; }
        case ECk_Player_ID::Five: { ForEachEntity<ECk_Player_ID::Five>(InRegistry, InFunc); break; }
        case ECk_Player_ID::Six: { ForEachEntity<ECk_Player_ID::Six>(InRegistry, InFunc); break; }
        case ECk_Player_ID::Seven: { ForEachEntity<ECk_Player_ID::Seven>(InRegistry, InFunc); break; }
        case ECk_Player_ID::Eight: { ForEachEntity<ECk_Player_ID::Eight>(InRegistry, InFunc); break; }
        case ECk_Player_ID::Unassigned: { ForEachEntity<ECk_Player_ID::Unassigned>(InRegistry, InFunc); break; }
        default:
        {
           CK_INVALID_ENUM(InPlayerID);
           break;
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------
