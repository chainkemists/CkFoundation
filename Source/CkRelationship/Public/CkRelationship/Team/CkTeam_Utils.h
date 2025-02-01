#pragma once

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"

#include "CkNet/CkNet_Utils.h"

#include "CkRelationship/Team/CkTeam_Fragment.h"
#include "CkRelationship/Team/CkTeam_Fragment_Data.h"

#include "CkTeam_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKRELATIONSHIP_API UCk_Utils_Team_UE : public UCk_Utils_Ecs_Net_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Team_UE);

public:
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Team);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Relationship|Team",
              DisplayName="[Ck][Team] Add Feature")
    static FCk_Handle_Team
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Team_ID InTeamID = ECk_Team_ID::Unassigned,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Relationship|Team",
              DisplayName="[Ck][Team] Assign to Team")
    static FCk_Handle_Team
    Assign(
        UPARAM(ref) FCk_Handle_Team& InHandle,
        ECk_Team_ID InTeamID);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Relationship|Team",
              meta = (Keywords = "remove, feature"),
              DisplayName="[Ck][Team] Unassign from Team")
    static FCk_Handle_Team
    Unassign(
        UPARAM(ref) FCk_Handle_Team& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Relationship|Team",
              DisplayName = "[Ck][Team] Is Assigned to Team")
    static bool
    Get_IsAssignedTo(
        const FCk_Handle_Team& InHandle,
        ECk_Team_ID InTeamID);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Relationship|Team",
              DisplayName = "[Ck][Team] Is Assigned To Same Team")
    static bool
    Get_IsSame(
        const FCk_Handle_Team& InHandleA,
        const FCk_Handle_Team& InHandleB);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Relationship|Team",
              DisplayName = "[Ck][Team] Get Team ID")
    static ECk_Team_ID
    Get_ID(
        const FCk_Handle_Team& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "|Ck|Utils|Relationship|Team",
              DisplayName="[Ck][Team] Try Get Entity With Team In Ownership Chain")
    static FCk_Handle_Team
    TryGet_Entity_Team_InOwnershipChain(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Team -> Generic Team",
              Category = "Ck|Utils|Handle",
              meta = (CompactNodeTitle = "->", BlueprintAutocast))
    static FGenericTeamId
    Conv_TeamToGenericTeam(
        const FCk_Handle_Team& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Team -> Generic Team",
              Category = "Ck|Utils|Handle",
              meta = (CompactNodeTitle = "->", BlueprintAutocast))
    static ECk_Team_ID
    Conv_TeamEnumToGenericTeam(
        const FGenericTeamId& InGenericTeamId);

public:
    UFUNCTION(BlueprintCallable,
              Category = "|Ck|Utils|Relationship|Team",
              DisplayName="[Ck][Team] For Each Opposing Team Entity",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_Team>
    ForEachEntity_OnOpposingTeam(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Team_ID InTeam,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "|Ck|Utils|Relationship|Team",
              DisplayName="[Ck][Team] For Each Same Team Entity",
              meta=(AutoCreateRefTerm="InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_Team>
    ForEachEntity_OnSameTeam(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Team_ID InTeam,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);

public:
    template <typename T_Func>
    static auto
    ForEachEntity_OnOpposingTeam(
        FCk_Handle& InHandle,
        ECk_Team_ID InTeam,
        T_Func InFunc) -> void;

    template <typename T_Func>
    static auto
    ForEachEntity_OnSameTeam(
        FCk_Handle& InHandle,
        ECk_Team_ID InTeam,
        T_Func InFunc) -> void;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Relationship|Team",
              DisplayName="[Ck][Team] Has Team Assigned To")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Relationship|Team",
        DisplayName="[Ck][Team] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Team
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Relationship|Team",
        DisplayName="[Ck][Team] Handle -> Team Handle",
        meta = (CompactNodeTitle = "<AsTeam>", BlueprintAutocast))
    static FCk_Handle_Team
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Team Handle",
        Category = "Ck|Utils|Team",
        meta = (CompactNodeTitle = "INVALID_TeamHandle", Keywords = "make"))
    static FCk_Handle_Team
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Relationship|Team",
        DisplayName="[Ck][Team] Bind To On Team Changed")
    static FCk_Handle_Team
    BindTo_OnTeamChanged(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_TeamChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Relationship|Team",
        DisplayName="[Ck][Team] Unbind From On Team Changed")
    static FCk_Handle_Team
    UnbindFrom_OnTeamChanged(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_TeamChanged& InDelegate);

public:
    template <ECk_Team_ID T_ID>
    static auto
    Assign(
        FCk_Handle& InHandle) -> void;

    template <ECk_Team_ID T_ID>
    static auto
    Get_IsAssignedTo(
        const FCk_Handle& InHandle) -> bool;

    template <ECk_Team_ID T_ID, typename T_Func, typename... T_Components>
    static auto
    ForEachEntity(
        FCk_Registry& InRegistry,
        T_Func InFunc) -> void;

    template <typename T_Func, typename... T_Components>
    static auto
    ForEachEntity(
        FCk_Registry& InRegistry,
        ECk_Team_ID InTeamID,
        T_Func InFunc) -> void;

private:
    static auto
    DoRemoveExistingTeam(
        FCk_Handle_Team& InHandle) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <typename T_Func>
auto
    UCk_Utils_Team_UE::
    ForEachEntity_OnOpposingTeam(
        FCk_Handle& InHandle,
        ECk_Team_ID InTeam,
        T_Func InFunc)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Handle [{}] is INVALID."), InHandle)
    { return; }

    const auto ForEachEntityOnTeam = [&]<ECk_Team_ID T_TeamId>()
    {
        if (InTeam == T_TeamId)
        { return; }

        ForEachEntity<T_TeamId>(**InHandle, InFunc);
    };

    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Zero>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::One>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Two>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Three>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Four>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Five>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Six>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Seven>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Eight>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Nine>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Ten>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Eleven>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Twelve>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Unassigned>();
}

template <typename T_Func>
auto
    UCk_Utils_Team_UE::
    ForEachEntity_OnSameTeam(
        FCk_Handle& InHandle,
        ECk_Team_ID InTeam,
        T_Func InFunc)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Handle [{}] is INVALID."), InHandle)
    { return; }

    const auto ForEachEntityOnTeam = [&]<ECk_Team_ID T_TeamId>()
    {
        if (InTeam != T_TeamId)
        { return; }

        ForEachEntity<T_TeamId>(**InHandle, InFunc);
    };

    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Zero>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::One>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Two>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Three>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Four>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Five>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Six>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Seven>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Eight>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Nine>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Ten>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Eleven>();
    ForEachEntityOnTeam.template operator()<ECk_Team_ID::Twelve>();
}

// --------------------------------------------------------------------------------------------------------------------

template <ECk_Team_ID T_ID>
auto
    UCk_Utils_Team_UE::
    Assign(
        FCk_Handle& InHandle)
    -> void
{
    InHandle.Add<ck::FTag_TeamID<T_ID>>();
    InHandle.Add<ck::FTag_OnTeamAssigned_Setup>();
    InHandle.AddOrGet<ck::FFragment_TeamInfo>(T_ID);
}

template <ECk_Team_ID T_ID>
auto
    UCk_Utils_Team_UE::
    Get_IsAssignedTo(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FTag_TeamID<T_ID>>();
}

template <ECk_Team_ID T_ID, typename T_Func, typename ... T_Components>
auto
    UCk_Utils_Team_UE::
    ForEachEntity(
        FCk_Registry& InRegistry,
        T_Func InFunc)
    -> void
{
    InRegistry.View<ck::FTag_TeamID<T_ID>, T_Components...>().ForEach(InFunc);
}

template <typename T_Func, typename ... T_Components>
auto
    UCk_Utils_Team_UE::
    ForEachEntity(
        FCk_Registry& InRegistry,
        ECk_Team_ID InTeamID,
        T_Func InFunc)
     -> void
{
    switch (InTeamID)
    {
        case ECk_Team_ID::Zero: { ForEachEntity<ECk_Team_ID::Zero>(InRegistry, InFunc); break; }
        case ECk_Team_ID::One: { ForEachEntity<ECk_Team_ID::One>(InRegistry, InFunc); break; }
        case ECk_Team_ID::Two: { ForEachEntity<ECk_Team_ID::Two>(InRegistry, InFunc); break; }
        case ECk_Team_ID::Three: { ForEachEntity<ECk_Team_ID::Three>(InRegistry, InFunc); break; }
        case ECk_Team_ID::Four: { ForEachEntity<ECk_Team_ID::Four>(InRegistry, InFunc); break; }
        case ECk_Team_ID::Five: { ForEachEntity<ECk_Team_ID::Five>(InRegistry, InFunc); break; }
        case ECk_Team_ID::Six: { ForEachEntity<ECk_Team_ID::Six>(InRegistry, InFunc); break; }
        case ECk_Team_ID::Seven: { ForEachEntity<ECk_Team_ID::Seven>(InRegistry, InFunc); break; }
        case ECk_Team_ID::Eight: { ForEachEntity<ECk_Team_ID::Eight>(InRegistry, InFunc); break; }
        case ECk_Team_ID::Nine: { ForEachEntity<ECk_Team_ID::Nine>(InRegistry, InFunc); break; }
        case ECk_Team_ID::Ten: { ForEachEntity<ECk_Team_ID::Ten>(InRegistry, InFunc); break; }
        case ECk_Team_ID::Eleven: { ForEachEntity<ECk_Team_ID::Eleven>(InRegistry, InFunc); break; }
        case ECk_Team_ID::Twelve: { ForEachEntity<ECk_Team_ID::Twelve>(InRegistry, InFunc); break; }
        case ECk_Team_ID::Unassigned: { ForEachEntity<ECk_Team_ID::Unassigned>(InRegistry, InFunc); break; }
        default:
        {
           CK_INVALID_ENUM(InTeamID);
           break;
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKRELATIONSHIP_API UCk_Utils_Team_Listener_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Team_Listener_UE);

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Relationship|Team",
        DisplayName="[Ck][Team] Bind To On Team Assigned to ANY Entity")
    static FCk_Handle
    BindTo_OnTeamAssignedToAnyEntity(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_TeamAssigned& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Relationship|Team",
        DisplayName="[Ck][Team] Unbind From On Team Assigned to ANY Entity")
    static FCk_Handle
    UnbindFrom_OnTeamAssigned(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Delegate_TeamAssigned& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Relationship|Team",
        DisplayName="[Ck][Team] Bind To On Team Assigned to ANY Entity (Same Team)")
    static FCk_Handle
    BindTo_OnTeamAssignedToAnyEntity_OnSameTeam(
        UPARAM(ref) FCk_Handle_Team& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_TeamAssigned& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Relationship|Team",
        DisplayName="[Ck][Team] Unbind From On Team Assigned to ANY Entity (Same Team)")
    static FCk_Handle
    UnbindFrom_OnTeamAssignedToAnyEntity_OnSameTeam(
        UPARAM(ref) FCk_Handle_Team& InHandle,
        const FCk_Delegate_TeamAssigned& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Relationship|Team",
        DisplayName="[Ck][Team] Bind To On Team Assigned to ANY Entity (Opposing Team)")
    static FCk_Handle
    BindTo_OnTeamAssignedToAnyEntity_OnOpposingTeam(
        UPARAM(ref) FCk_Handle_Team& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_TeamAssigned& InDelegate);

    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Relationship|Team",
        DisplayName="[Ck][Team] Unbind From On Team Assigned to ANY Entity (Opposing Team)")
    static FCk_Handle
    UnbindFrom_OnTeamAssignedToAnyEntity_OnOpposingTeam(
        UPARAM(ref) FCk_Handle_Team& InHandle,
        const FCk_Delegate_TeamAssigned& InDelegate);

private:
    static auto
    DoAddTeamListener(
        FCk_Handle& InHandle) -> void;
};
