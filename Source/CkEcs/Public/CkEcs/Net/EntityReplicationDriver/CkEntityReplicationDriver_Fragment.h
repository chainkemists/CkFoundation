#pragma once

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment_Data.h"
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkEntityReplicationDriver_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKECS_API,
        OnReplicationComplete,
        FCk_Delegate_EntityReplicationDriver_OnReplicationComplete,
        FCk_Handle);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKECS_API,
        OnDependentsReplicationComplete,
        FCk_Delegate_EntityReplicationDriver_OnReplicationComplete,
        FCk_Handle);
}

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityReplicationDriver_UE;

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_EntityReplicationDriver_FireOnDependentReplicationComplete);

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_ReplicationDriver_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ReplicationDriver_Requests);

    public:
        friend class FProcessor_ReplicationDriver_HandleRequests;
        friend class UCk_Utils_EntityReplicationDriver_UE;

    public:
        using RequestType = FCk_Request_ReplicationDriver_ReplicateEntity;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    class FProcessor_AbilityOwner_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKECS_API UCk_Fragment_EntityReplicationDriver_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_EntityReplicationDriver_Rep);

public:
    friend class ck::FProcessor_AbilityOwner_HandleRequests;

public:
    UCk_Fragment_EntityReplicationDriver_Rep(
        const FObjectInitializer&);

public:
    auto
    Get_IsReplicationCompleteOnAllDependents() const -> bool;

private:
    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

public:
    UFUNCTION(Server, Reliable)
    void
    Request_Replicate_NonReplicatedActor(
        FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor InConstructionInfo);

public:
    auto
    GetReplicatedCustomConditionState(
        FCustomPropertyConditionState& OutActiveState) const -> void override;

private:
    UFUNCTION()
    void
    OnRep_ReplicationData();

    UFUNCTION()
    void
    OnRep_ReplicationData_EntityScript();

    UFUNCTION()
    void
    OnRep_ReplicationData_Ability();

    UFUNCTION()
    void
    OnRep_ReplicationData_ReplicatedActor();

    UFUNCTION()
    void
    OnRep_ReplicationData_NonReplicatedActor();

    UFUNCTION()
    void
    OnRep_ExpectedNumberOfDependentReplicationDrivers() const;

private:
    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData)
    FCk_EntityReplicationDriver_ReplicationData _ReplicationData;

    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData_EntityScript)
    FCk_EntityReplicationDriver_ReplicationData_EntityScript _ReplicationData_EntityScript;

    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData_Ability)
    FCk_EntityReplicationDriver_AbilityData _ReplicationData_Ability;

    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData_ReplicatedActor)
    FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor _ReplicationData_ReplicatedActor;

    UPROPERTY(ReplicatedUsing = OnRep_ReplicationData_NonReplicatedActor)
    FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor _ReplicationData_NonReplicatedActor;

    UPROPERTY(ReplicatedUsing = OnRep_ExpectedNumberOfDependentReplicationDrivers)
    int32 _ExpectedNumberOfDependentReplicationDrivers = 0;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>> _PendingChildEntityConstructions;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>> _PendingChildAbilityEntityConstructions;

private:
    auto
    DoAdd_SyncedDependentReplicationDriver() -> void;

private:
    UPROPERTY(Transient)
    int32 _NumSyncedDependentReplicationDrivers = 0;

public:
    auto
    Set_ReplicationData(
        const FCk_EntityReplicationDriver_ReplicationData& InReplicationData) -> void;

    auto
    Set_ReplicationData_EntityScript(
        const FCk_EntityReplicationDriver_ReplicationData_EntityScript& InReplicationData) -> void;

    auto
    Set_ReplicationData_Ability(
        const FCk_EntityReplicationDriver_AbilityData& InReplicationData) -> void;

    auto
    Set_ReplicationData_ReplicatedActor(
        const FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor& InReplicationData) -> void;

    auto
    Set_ReplicationData_NonReplicatedActor(
        const FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor& InReplicationData) -> void;

    auto
    Set_ExpectedNumberOfDependentReplicationDrivers(
        int32 InNumOfDependents) -> void;

public:
    // TODO: reduce the exposure of this variable
    CK_PROPERTY_GET(_ReplicationData);
    CK_PROPERTY_GET(_ReplicationData_Ability);
    CK_PROPERTY_GET(_ReplicationData_ReplicatedActor);
    CK_PROPERTY_GET(_ReplicationData_NonReplicatedActor);
    CK_PROPERTY_GET(_ExpectedNumberOfDependentReplicationDrivers);

public:
    // --- Generic fragment container (replicated via FFastArraySerializer) ---

    template<typename TDataStruct>
    auto
    SetFragmentData(
        const TDataStruct& InData) -> int32;

    auto
    FindEntry(
        const UScriptStruct* InType) -> FCk_ReplicatedFragmentEntry*;

    auto
    MarkFragmentDirty(
        FCk_ReplicatedFragmentEntry& InEntry) -> void;

    /** Re-invokes OnAdd handlers for all container entries.
     *  Called after entity construction so handlers can find child entities that didn't exist
     *  when the initial PostReplicatedAdd fired. */
    auto
    ReplayPendingContainerHandlers() -> void;

private:
    UPROPERTY(Replicated)
    FCk_ReplicatedFragmentArray _Fragments;
};

// --------------------------------------------------------------------------------------------------------------------
// SetFragmentData template definition

template<typename TDataStruct>
auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    SetFragmentData(
        const TDataStruct& InData)
    -> int32
{
    auto* Entry = FindEntry(TDataStruct::StaticStruct());

    if (Entry != nullptr)
    {
        Entry->Data.GetMutable<TDataStruct>() = InData;
        _Fragments.MarkItemDirty(*Entry);
        MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _Fragments, this);
        return _Fragments._Items.IndexOfByPredicate([Entry](const FCk_ReplicatedFragmentEntry& E) { return &E == Entry; });
    }

    auto& NewEntry = _Fragments._Items.AddDefaulted_GetRef();
    NewEntry.Data.InitializeAs<TDataStruct>(InData);
    _Fragments.MarkItemDirty(NewEntry);
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _Fragments, this);

    return _Fragments._Items.Num() - 1;
}

// --------------------------------------------------------------------------------------------------------------------
