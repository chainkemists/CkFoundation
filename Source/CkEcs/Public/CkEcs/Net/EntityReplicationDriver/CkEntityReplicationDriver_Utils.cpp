#include "CkEntityReplicationDriver_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"

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
        InHandle.Add<TWeakObjectPtr<UCk_Ecs_ReplicatedObject_UE>>(InHandle._ReplicationDriver);
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
        const FCk_Handle& InAbilitySource,
        ECk_ConstructionPhase InAbilityConstructionPhase)
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

    InConstructionScript->Construct(NewEntity, {}, InAbilityScriptClass->ClassDefaultObject);

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
                FCk_EntityReplicationDriver_ReplicateObjects_Data{ReplicatedObjects.Get_ReplicatedObjects()},
                InAbilityConstructionPhase
            }.Set_OwningEntityDriver(InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>()));

            ck::UUtils_Signal_OnReplicationComplete::Broadcast(NewEntity, ck::MakePayload(NewEntity));
            ck::UUtils_Signal_OnDependentsReplicationComplete::Broadcast(NewEntity, ck::MakePayload(NewEntity));

            break;
        }
        case ECk_Net_NetModeType::Unknown:
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

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, InFunc_OnCreateEntityBeforeBuild);

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
            const auto& IsOwningEntityDriverDependentOnThis = InHandle.Has<ck::FTag_EntityJustCreated>();

            const auto& DependentRepDriversAddedDuringConstruction = RepDriver->Get_ExpectedNumberOfDependentReplicationDrivers();

            RepDriver->Set_ExpectedNumberOfDependentReplicationDrivers(Get_NumOfReplicationDriversIncludingDependents(NewEntity) + DependentRepDriversAddedDuringConstruction);

            RepDriver->Set_ReplicationData
            (
                FCk_EntityReplicationDriver_ReplicationData
                {
                    InConstructionInfo,
                    FCk_EntityReplicationDriver_ReplicateObjects_Data{ReplicatedObjects.Get_ReplicatedObjects()}
                }
                .Set_OwningEntityDriver(InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>())
                .Set_IsOwningEntityDriverDependentOnThis(IsOwningEntityDriverDependentOnThis)
            );
            ck::UUtils_Signal_OnReplicationComplete::Broadcast(NewEntity, ck::MakePayload(NewEntity));
            ck::UUtils_Signal_OnDependentsReplicationComplete::Broadcast(NewEntity, ck::MakePayload(NewEntity));

            break;
        }
        case ECk_Net_NetModeType::Unknown:
        default:
            CK_INVALID_ENUM(NetMode);
            break;
    }

    return NewEntity;
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_Replicate(
        FCk_Handle& InHandleToReplicate,
        FCk_Handle InReplicatedOwner,
        TSubclassOf<UCk_EntityScript_UE> InEntityScript,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    CK_ENSURE_IF_NOT(InHandleToReplicate.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>(),
        TEXT("Handle [{}] does NOT have a ReplicationDriver. Unable to proceed with Replication of the handle."),
        InHandleToReplicate)
    { return; }

    CK_ENSURE_IF_NOT(InReplicatedOwner.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>(),
        TEXT("Owner [{}] does NOT have a ReplicationDriver. Unable to proceed with Replication of Entity with EntityScript [{}]"),
        InReplicatedOwner,
        InEntityScript)
    { return; }

    switch(const auto NetMode = UCk_Utils_Net_UE::Get_EntityNetMode(InReplicatedOwner))
    {
        case ECk_Net_NetModeType::Host:
        {
            const auto& RepDriver = InHandleToReplicate.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();
            const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(InHandleToReplicate);
            const auto& IsOwningEntityDriverDependentOnThis = InReplicatedOwner.Has<ck::FTag_EntityJustCreated>();

            const auto& DependentRepDriversAddedDuringConstruction = RepDriver->Get_ExpectedNumberOfDependentReplicationDrivers();

            RepDriver->Set_ExpectedNumberOfDependentReplicationDrivers(
                Get_NumOfReplicationDriversIncludingDependents(InHandleToReplicate) +
                DependentRepDriversAddedDuringConstruction);

            RepDriver->Set_ReplicationData_EntityScript
            (
                FCk_EntityReplicationDriver_ReplicationData_EntityScript
                {
                    InEntityScript,
                    InSpawnParams,
                    InReplicatedOwner.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>(),
                    FCk_EntityReplicationDriver_ReplicateObjects_Data{ReplicatedObjects.Get_ReplicatedObjects()}
                }
                .Set_IsOwningEntityDriverDependentOnThis(IsOwningEntityDriverDependentOnThis)
            );

            break;
        }
        case ECk_Net_NetModeType::Unknown:
        default:
            CK_INVALID_ENUM(NetMode);
            break;
    }
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

    const auto& DependentRepDriversAddedDuringConstruction = RepDriver->Get_ExpectedNumberOfDependentReplicationDrivers();

    RepDriver->Set_ExpectedNumberOfDependentReplicationDrivers(Get_NumOfReplicationDriversIncludingDependents(InHandle) + DependentRepDriversAddedDuringConstruction);
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
    case ECk_Net_NetModeType::Unknown:
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

// --------------------------------------------------------------------------------------------------------------------
