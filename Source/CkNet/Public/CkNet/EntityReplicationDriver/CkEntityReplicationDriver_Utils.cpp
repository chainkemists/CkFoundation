#include "CkEntityReplicationDriver_Utils.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"

#include <numeric>

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Utils_EntityReplicationDriver_UE::
    Add(
        const FCk_Handle InHandle)
    -> void
{
    TryAddReplicatedFragment<UCk_Fragment_EntityReplicationDriver_Rep>(InHandle);
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Get_NumOfReplicationDriversIncludingDependents(
        FCk_Handle InHandle)
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
    Request_Replicate(
        FCk_Handle InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo)
    -> void
{
    CK_ENSURE_IF_NOT(InHandle.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>(),
        TEXT("Entity [{}] does NOT have a ReplicationDriver. Unable to proceed with Replication of Entity with ConstructionScript [{}]"),
        InHandle,
        InConstructionInfo.Get_ConstructionScript())
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InConstructionInfo.Get_ConstructionScript()),
        TEXT("Unable to ReplicateEntity as ConstructionScript is [{}]"),
        InConstructionInfo.Get_ConstructionScript())
    { return; }

    const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
    Add(NewEntity);
    UCk_Utils_Net_UE::Copy(InHandle, NewEntity);
    InConstructionInfo.Get_ConstructionScript()->GetDefaultObject<UCk_EntityBridge_ConstructionScript_PDA>()->Construct(NewEntity);

    switch(const auto NetMode = UCk_Utils_Net_UE::Get_EntityNetMode(InHandle))
    {
        case ECk_Net_NetModeType::Client:
        {
            InHandle.AddOrGet<ck::FFragment_ReplicationDriver_Requests>()._Requests.Emplace(
                FCk_EntityReplicationDriver_ConstructionInfo{InConstructionInfo}
                .Set_OriginalEntity(NewEntity.Get_Entity()));
            break;
        }
        case ECk_Net_NetModeType::Host:
        case ECk_Net_NetModeType::Server:
        {
            const auto& RepDriver = NewEntity.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();
            const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(NewEntity);

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
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_ReplicateEntityOnReplicatedActor(
        FCk_Handle InHandle,
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
    Has(
        FCk_Handle InEntity)
    -> bool
{
    return InEntity.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Ensure(
        FCk_Handle InEntity)
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
        FCk_Handle InHandle)
    -> bool
{
    if (NOT Ensure(InHandle))
    { return false; }

    return InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>()->Get_IsReplicationCompleteOnAllDependents();
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Promise_OnReplicationComplete(
        FCk_Handle                                                        InEntity,
        const FCk_Delegate_EntityReplicationDriver_OnReplicationComplete& InDelegate)
    -> void
{
    if (NOT Ensure(InEntity))
    { return; }

    ck::UUtils_Signal_OnReplicationComplete::Bind(InEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Promise_OnReplicationCompleteAllDependents(
        FCk_Handle                                                        InEntity,
        const FCk_Delegate_EntityReplicationDriver_OnReplicationComplete& InDelegate)
    -> void
{
    if (NOT Ensure(InEntity))
    { return; }

    ck::UUtils_Signal_OnDependentsReplicationComplete::Bind(InEntity, InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

// --------------------------------------------------------------------------------------------------------------------
