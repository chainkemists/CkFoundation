#include "CkEntityReplicationDriver_Utils.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkNet/CkNet_Log.h"
#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

#include <numeric>

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Utils_EntityReplicationDriver_UE::
    Add(
        FCk_Handle& InHandle)
    -> ECk_AddedOrNot
{
    const auto AddedOrNot = TryAddReplicatedFragment<UCk_Fragment_EntityReplicationDriver_Rep>(InHandle);
    if (AddedOrNot != ECk_AddedOrNot::NotAdded)
    {
        InHandle._ReplicationDriver = InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();
        InHandle.Add<TWeakObjectPtr<UCk_Ecs_ReplicatedObject_UE>>(InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>());
    }
    return AddedOrNot;
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Get_NumOfReplicationDriversIncludingDependents(
        const FCk_Handle& InHandle)
    -> int32
{
    if (Has(InHandle))
    {
        const auto& Dependents = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InHandle);

        const auto Total = std::accumulate(Dependents.begin(), Dependents.end(), 1, [&](int32 Value, FCk_Handle Dependent)
        {
            return Value + Get_NumOfReplicationDriversIncludingDependents(Dependent);
        });

        return Total;
    }

    return 0;
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_BuildAndReplicate(
        FCk_Handle& InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo)
    -> FCk_Handle
{
    return Request_TryBuildAndReplicate(InHandle, InConstructionInfo, [](FCk_Handle){});
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_TryReplicateAbility(
        FCk_Handle& InHandle,
        const UCk_Entity_ConstructionScript_PDA* InConstructionScript,
        const TSubclassOf<UCk_DataAsset_PDA>& InAbilityScriptClass,
        const FCk_Handle& InAbilitySource)
    -> FCk_Handle
{
    if (UCk_Utils_Net_UE::Get_EntityReplication(InHandle) == ECk_Replication::DoesNotReplicate)
    { return {}; }

    if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InHandle))
    { return {};}

    CK_ENSURE_IF_NOT(InHandle.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>(),
        TEXT("Entity [{}] does NOT have a ReplicationDriver. Unable to proceed with Replication of Entity with ConstructionScript [{}]"),
        InHandle,
        InConstructionScript)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InConstructionScript),
        TEXT("Unable to ReplicateEntity as ConstructionScript is [{}]"),
        InConstructionScript)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    UCk_Utils_Net_UE::Copy(InHandle, NewEntity);

    if (Add(NewEntity) == ECk_AddedOrNot::NotAdded)
    { return {}; }

    InConstructionScript->Construct(NewEntity, {});

    switch(const auto NetMode = UCk_Utils_Net_UE::Get_EntityNetMode(InHandle))
    {
        case ECk_Net_NetModeType::Host:
        {
            const auto& RepDriver = NewEntity.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();
            const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(NewEntity);

            RepDriver->Set_ReplicationData_Ability(FCk_EntityReplicationDriver_AbilityData
            {
                InAbilityScriptClass,
                InAbilitySource,
                FCk_EntityReplicationDriver_ReplicateObjects_Data{ReplicatedObjects.Get_ReplicatedObjects()}
            }.Set_OwningEntityDriver(InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>()));

            ck::UUtils_Signal_OnReplicationComplete::Broadcast(NewEntity, ck::MakePayload(NewEntity));
            ck::UUtils_Signal_OnDependentsReplicationComplete::Broadcast(NewEntity, ck::MakePayload(NewEntity));

            break;
        }
        case ECk_Net_NetModeType::None:
        default:
            CK_INVALID_ENUM(NetMode);
            break;
    }

    return NewEntity;
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_TryBuildAndReplicate(
        FCk_Handle& InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo,
        const std::function<void(FCk_Handle)>& InFunc_OnCreateEntityBeforeBuild)
    -> FCk_Handle
{
    if (UCk_Utils_Net_UE::Get_EntityReplication(InHandle) == ECk_Replication::DoesNotReplicate)
    { return {}; }

    if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InHandle))
    { return {};}

    CK_ENSURE_IF_NOT(InHandle.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>(),
        TEXT("Entity [{}] does NOT have a ReplicationDriver. Unable to proceed with Replication of Entity with ConstructionScript [{}]"),
        InHandle,
        InConstructionInfo.Get_ConstructionScript())
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InConstructionInfo.Get_ConstructionScript()),
        TEXT("Unable to ReplicateEntity as ConstructionScript is [{}]"),
        InConstructionInfo.Get_ConstructionScript())
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
    if (InFunc_OnCreateEntityBeforeBuild)
    { InFunc_OnCreateEntityBeforeBuild(NewEntity); }

    UCk_Utils_Net_UE::Copy(InHandle, NewEntity);

    if (Add(NewEntity) == ECk_AddedOrNot::NotAdded)
    { return {}; }

    InConstructionInfo.Get_ConstructionScript()->GetDefaultObject<UCk_Entity_ConstructionScript_PDA>()->Construct(
        NewEntity, InConstructionInfo.Get_OptionalParams());

    switch(const auto NetMode = UCk_Utils_Net_UE::Get_EntityNetMode(InHandle))
    {
        case ECk_Net_NetModeType::Host:
        {
            const auto& RepDriver = NewEntity.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();
            const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(NewEntity);

            RepDriver->Set_ExpectedNumberOfDependentReplicationDrivers(Get_NumOfReplicationDriversIncludingDependents(InHandle));

            RepDriver->Set_ReplicationData
            (
                FCk_EntityReplicationDriver_ReplicationData
                {
                    InConstructionInfo,
                    FCk_EntityReplicationDriver_ReplicateObjects_Data{ReplicatedObjects.Get_ReplicatedObjects()}
                }.Set_OwningEntityDriver(InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>())
            );
            ck::UUtils_Signal_OnReplicationComplete::Broadcast(NewEntity, ck::MakePayload(NewEntity));
            ck::UUtils_Signal_OnDependentsReplicationComplete::Broadcast(NewEntity, ck::MakePayload(NewEntity));

            break;
        }
        case ECk_Net_NetModeType::None:
        default:
            CK_INVALID_ENUM(NetMode);
            break;
    }

    return NewEntity;
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_ReplicateEntityOnReplicatedActor(
        FCk_Handle& InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo_ReplicatedActor& InConstructionInfo)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    const auto& RepDriver = InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();

    RepDriver->Set_ExpectedNumberOfDependentReplicationDrivers(Get_NumOfReplicationDriversIncludingDependents(InHandle));
    RepDriver->Set_ReplicationData_ReplicatedActor(InConstructionInfo);

    ck::UUtils_Signal_OnReplicationComplete::Broadcast(InHandle, ck::MakePayload(InHandle));
    ck::UUtils_Signal_OnDependentsReplicationComplete::Broadcast(InHandle, ck::MakePayload(InHandle));
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_ReplicateEntityOnNonReplicatedActor(
        FCk_Handle& InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo_NonReplicatedActor& InConstructionInfo) -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    const auto& RepDriver = InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();

    switch(const auto NetMode = UCk_Utils_Net_UE::Get_EntityNetMode(InHandle))
    {
    case ECk_Net_NetModeType::None:
        break;
    case ECk_Net_NetModeType::Client:
        RepDriver->Request_Replicate_NonReplicatedActor(InConstructionInfo);
        break;
    case ECk_Net_NetModeType::Host:
        RepDriver->Set_ReplicationData_NonReplicatedActor(InConstructionInfo);
        break;
    default: ;
    }
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Has(
        const FCk_Handle& InEntity)
    -> bool
{
    return InEntity.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Ensure(
        const FCk_Handle& InEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InEntity),
        TEXT("Handle [{}] does NOT have [{}]"),
        InEntity, ck::TypeToString<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>)
    { return false; }

    return true;
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Get_IsReplicationCompleteAllDependents(
        const FCk_Handle& InHandle)
    -> bool
{
    if (NOT Has(InHandle))
    { return true; }

    return InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>()->Get_IsReplicationCompleteOnAllDependents();
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Promise_OnReplicationComplete(
        FCk_Handle& InEntity,
        const FCk_Delegate_EntityReplicationDriver_OnReplicationComplete& InDelegate)
    -> void
{
    if (NOT Has(InEntity))
    {
        std::ignore = InDelegate.ExecuteIfBound(InEntity);
        return;
    }

    ck::UUtils_Signal_OnReplicationComplete_PostFireUnbind::Bind(InEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Promise_OnReplicationCompleteAllDependents(
        FCk_Handle& InEntity,
        const FCk_Delegate_EntityReplicationDriver_OnReplicationComplete& InDelegate)
    -> void
{
    if (NOT Has(InEntity))
    {
        std::ignore = InDelegate.ExecuteIfBound(InEntity);
        return;
    }

    ck::UUtils_Signal_OnDependentsReplicationComplete_PostFireUnbind::Bind(
        InEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Make_HandleReplicator(
        FCk_Handle& InHandle)
    -> FCk_HandleReplicator
{
    auto EntityReplicator = FCk_HandleReplicator{};

    TryUpdateReplicatedFragment<UCk_Fragment_EntityReplicationDriver_Rep>(InHandle,
    [&](UCk_Fragment_EntityReplicationDriver_Rep* InRepDriver)
    {
        EntityReplicator = FCk_HandleReplicator{InHandle, InRepDriver};
    });

    CK_LOG_ERROR_IF_NOT(ck::net, ck::IsValid(EntityReplicator.Get_Handle_RepObj()),
        TEXT("Entity [{}] CANNOT be replicated since it does NOT have a valid [{}]"), InHandle,
        ck::Get_RuntimeTypeToString<UCk_Fragment_EntityReplicationDriver_Rep>())
    { return {}; }

    return EntityReplicator;
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Break_HandleReplicator(
        const FCk_HandleReplicator& InHandleReplicator,
        FCk_Handle& OutHandle)
    -> void
{
    OutHandle = InHandleReplicator.Get_Handle();
}

// --------------------------------------------------------------------------------------------------------------------
