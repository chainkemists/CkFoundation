#include "CkEntityReplicationDriver_Utils.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_BuildRecipe.h" // FFragment_BuildRecipe retention (definition-built entity recipe)
#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h" // FTag_RepFragments_PendingApply
#include "CkEcs/Persistence/CkPersistenceHydration.h" // FFragment_PendingHydration
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h" // Get_WorldForEntity (recipe holder outer)
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h" // FTag_DefinitionBuild_InProgress (construction-window stamp for labeled children)

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Algorithms/CkAlgorithms.h" // ck::algo::AnyOf (undrained-dependents recursion)

#include "Misc/ScopeExit.h" // ON_SCOPE_EXIT (DefinitionBuild construction-window tag removal)

#if UE_WITH_IRIS
#include <Iris/ReplicationSystem/ReplicationSystem.h>
#include <Net/Iris/ReplicationSystem/EngineReplicationBridge.h>
#endif

#include <Engine/PackageMapClient.h>
#include <Engine/NetDriver.h>
#include <numeric>

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Utils_EntityReplicationDriver_UE::
    TryAdd(
        FCk_Handle& InHandle)
    -> ECk_AddedOrNot
{
    const auto HasNetParams = UCk_Utils_Net_UE::Has(InHandle);
    const auto EntityReplication = HasNetParams ? UCk_Utils_Net_UE::Get_EntityReplication(InHandle) : ECk_Replication::DoesNotReplicate;
    const auto IsHost = UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InHandle);
    const auto HasOwningActor = UCk_Utils_OwningActor_UE::TryGet_Entity_OwningActor_InOwnershipChain(InHandle);

    ck::ecs::VeryVerbose(TEXT("[REP_DEBUG] ReplicationDriver::Add — Handle=[{}] HasNetParams=[{}] EntityReplication=[{}] IsHost=[{}] HasOwningActorInChain=[{}]"),
        InHandle, HasNetParams, EntityReplication, IsHost, ck::IsValid(HasOwningActor));

    const auto AddedOrNot = UCk_Utils_Net_UE::TryAddReplicatedFragment<UCk_Fragment_EntityReplicationDriver_Rep>(InHandle);

    ck::ecs::VeryVerbose(TEXT("[REP_DEBUG] ReplicationDriver::Add — Result=[{}]"), AddedOrNot);

    if (AddedOrNot == ECk_AddedOrNot::Added)
    {
        InHandle._ReplicationDriver = InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();
        InHandle.Add<TWeakObjectPtr<UCk_Ecs_ReplicatedObject_UE>>(InHandle._ReplicationDriver);
    }
    return AddedOrNot;
}

// Cycle-safe DFS over the lifetime-dependents subtree. Lifetime ownership is a TREE, so a revisit means
// the graph is corrupt: truncating there keeps the recursion from overflowing the stack (which kills the
// process with NO crash dump) and the ensure names the entity. An acyclic graph never hits the visited set.
static auto
    DoCount_ReplicationDriversIncludingDependents(
        const FCk_Handle& InHandle,
        TSet<FCk_Entity>& InOutVisited)
    -> int32
{
    if (NOT UCk_Utils_EntityReplicationDriver_UE::Has(InHandle))
    { return 0; }

    auto AlreadyVisited = false;
    InOutVisited.Add(InHandle.Get_Entity(), &AlreadyVisited);

    if (AlreadyVisited)
    {
        CK_TRIGGER_ENSURE(TEXT("Cyclic LifetimeDependents detected at Entity [{}] while counting replication "
            "drivers — the lifetime-ownership graph must be a tree. Truncating to avoid infinite recursion."),
            InHandle);
        return 0;
    }

    const auto& Dependents = UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InHandle);

    return std::accumulate(Dependents.begin(), Dependents.end(), 1, [&](const int32 Value, const FCk_Handle& Dependent)
    {
        return Value + DoCount_ReplicationDriversIncludingDependents(Dependent, InOutVisited);
    });
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Get_NumOfReplicationDriversIncludingDependents(
        const FCk_Handle& InHandle)
    -> int32
{
    auto Visited = TSet<FCk_Entity>{};
    return DoCount_ReplicationDriversIncludingDependents(InHandle, Visited);
}

// Cycle-safe DFS mirroring DoCount above, except it does NOT prune at non-driver entities: hydration runs
// on every net mode, so a non-replicated dependent's queued hydration must still block the aggregate fire.
static auto
    DoGet_HasUndrainedReplicatedFragments_Recursive(
        const FCk_Handle& InHandle,
        TSet<FCk_Entity>& InOutVisited)
    -> bool
{
    auto AlreadyVisited = false;
    InOutVisited.Add(InHandle.Get_Entity(), &AlreadyVisited);

    if (AlreadyVisited)
    {
        CK_TRIGGER_ENSURE(TEXT("Cyclic LifetimeDependents detected at Entity [{}] while checking for undrained "
            "replicated fragments — the lifetime-ownership graph must be a tree. Truncating to avoid infinite recursion."),
            InHandle);
        return false;
    }

    if (InHandle.Has<ck::FTag_RepFragments_PendingApply>() ||
        InHandle.Has<ck::FFragment_PendingHydration>())
    { return true; }

    return ck::algo::AnyOf(UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InHandle),
        [&](const auto& InDependent) { return DoGet_HasUndrainedReplicatedFragments_Recursive(InDependent, InOutVisited); });
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Get_HasUndrainedReplicatedFragments_IncludingDependents(
        const FCk_Handle& InHandle)
    -> bool
{
    auto Visited = TSet<FCk_Entity>{};
    return DoGet_HasUndrainedReplicatedFragments_Recursive(InHandle, Visited);
}

// Diagnostic twin of the DFS above: returns the FIRST offender instead of a bool. Cycle truncation is
// silent here because the bool twin already ensures on cycles.
static auto
    DoGet_FirstUndrainedEntity_Recursive(
        const FCk_Handle& InHandle,
        TSet<FCk_Entity>& InOutVisited)
    -> FCk_Handle
{
    auto AlreadyVisited = false;
    InOutVisited.Add(InHandle.Get_Entity(), &AlreadyVisited);

    if (AlreadyVisited)
    { return {}; }

    if (InHandle.Has<ck::FTag_RepFragments_PendingApply>() ||
        InHandle.Has<ck::FFragment_PendingHydration>())
    { return InHandle; }

    for (const auto& Dependent : UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InHandle))
    {
        if (const auto Found = DoGet_FirstUndrainedEntity_Recursive(Dependent, InOutVisited);
            ck::IsValid(Found))
        { return Found; }
    }

    return {};
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    TryGet_FirstUndrainedReplicatedFragmentsEntity_IncludingDependents(
        const FCk_Handle& InHandle)
    -> FCk_Handle
{
    auto Visited = TSet<FCk_Entity>{};
    return DoGet_FirstUndrainedEntity_Recursive(InHandle, Visited);
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_BuildAndReplicate(
        FCk_Handle& InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo)
    -> FCk_Handle
{
    return Request_BuildAndReplicate_Multiple(InHandle, { InConstructionInfo });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_BuildAndReplicate_Multiple(
        FCk_Handle& InHandle,
        const TArray<FCk_EntityReplicationDriver_ConstructionInfo>& InConstructionInfos)
    -> FCk_Handle
{
    return Request_TryBuildAndReplicate(InHandle, InConstructionInfos, [](FCk_Handle){});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_TryBuildAndReplicate(
        FCk_Handle& InHandle,
        const FCk_EntityReplicationDriver_ConstructionInfo& InConstructionInfo,
        const std::function<void(FCk_Handle)>& InFunc_OnCreateEntityBeforeBuild)
    -> FCk_Handle
{
    return Request_TryBuildAndReplicate(InHandle, TArray<FCk_EntityReplicationDriver_ConstructionInfo>{ InConstructionInfo }, InFunc_OnCreateEntityBeforeBuild);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_TryBuildAndReplicate(
        FCk_Handle& InHandle,
        const TArray<FCk_EntityReplicationDriver_ConstructionInfo>& InConstructionInfos,
        const std::function<void(FCk_Handle)>& InFunc_OnCreateEntityBeforeBuild)
    -> FCk_Handle
{
    if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InHandle))
    { return {}; }

    CK_ENSURE_IF_NOT(NOT InConstructionInfos.IsEmpty(),
        TEXT("Unable to ReplicateEntity as ConstructionInfos is empty"))
    { return {}; }

    for (const auto& ConstructionInfo : InConstructionInfos)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(ConstructionInfo.Get_ConstructionScript()),
            TEXT("Unable to ReplicateEntity as ConstructionScript is [{}]"),
            ConstructionInfo.Get_ConstructionScript())
        { return {}; }
    }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, InFunc_OnCreateEntityBeforeBuild);

    UCk_Utils_Net_UE::Copy(InHandle, NewEntity);
    TryAdd(NewEntity);

    // The tag marks the SYNCHRONOUS construction window: children composed inline by the scripts below get
    // stamped ConstructSpawned (and so persist), while any child composed later at runtime must NOT be. A
    // definition-built entity has no EntityScript fragment, so without this its labeled children lose their state.
    {
        NewEntity.Add<ck::FTag_DefinitionBuild_InProgress>();
        ON_SCOPE_EXIT { NewEntity.Remove<ck::FTag_DefinitionBuild_InProgress>(); };

        for (const auto& ConstructionInfo : InConstructionInfos)
        {
            if (ck::IsValid(ConstructionInfo.Get_ConstructionScriptArchetype()))
            {
                ConstructionInfo.Get_ConstructionScriptArchetype()->Construct(NewEntity);
            }
            else
            {
                ConstructionInfo.Get_ConstructionScript()->GetDefaultObject<UCk_Entity_ConstructionScript_PDA>()->Construct(
                    NewEntity);
            }
        }
    }

    // Construct consumes the ConstructionInfos, so retain them for save capture. Stamped BEFORE the
    // DoesNotReplicate early-return so non-replicated definition-built entities are captured too.
    if (const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(NewEntity);
        ck::IsValid(World))
    {
        auto* Recipe = NewObject<UCk_BuildRecipe_UE>(World);
        Recipe->Populate(InConstructionInfos);
        NewEntity.Add<ck::FFragment_BuildRecipe>(TStrongObjectPtr<UCk_BuildRecipe_UE>{Recipe});
    }

    // Non-replicated owners: entity was built successfully, skip replication setup.
    if (UCk_Utils_Net_UE::Get_EntityReplication(InHandle) == ECk_Replication::DoesNotReplicate)
    { return NewEntity; }

    CK_ENSURE_IF_NOT(InHandle.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>(),
        TEXT("Entity [{}] does NOT have a ReplicationDriver. Unable to proceed with Replication."),
        InHandle)
    { return NewEntity; }

    switch (const auto NetMode = UCk_Utils_Net_UE::Get_EntityNetMode(InHandle))
    {
        case ECk_Net_NetModeType::Host:
        case ECk_Net_NetModeType::ClientAndHost:
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
                    InConstructionInfos,
                    FCk_EntityReplicationDriver_ReplicateObjects_Data{FCk_ReplicatedObjects::ToWeak(ReplicatedObjects.Get_ReplicatedObjects())}
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
        {
            CK_INVALID_ENUM(NetMode);
            break;
        }
    }

    return NewEntity;
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_TryReplicateExisting(
        FCk_Handle& InExistingEntity,
        FCk_Handle& InReplicatedOwner,
        const TArray<FCk_EntityReplicationDriver_ConstructionInfo>& InConstructionInfos)
    -> void
{
    if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InReplicatedOwner))
    { return; }

    CK_ENSURE_IF_NOT(NOT InConstructionInfos.IsEmpty(),
        TEXT("Unable to ReplicateExisting [{}] as ConstructionInfos is empty"), InExistingEntity)
    { return; }

    for (const auto& ConstructionInfo : InConstructionInfos)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(ConstructionInfo.Get_ConstructionScript()),
            TEXT("Unable to ReplicateExisting [{}] as ConstructionScript is [{}]"),
            InExistingEntity, ConstructionInfo.Get_ConstructionScript())
        { return; }
    }

    UCk_Utils_Net_UE::Copy(InReplicatedOwner, InExistingEntity);
    TryAdd(InExistingEntity);

    if (UCk_Utils_Net_UE::Get_EntityReplication(InReplicatedOwner) == ECk_Replication::DoesNotReplicate)
    { return; }

    CK_ENSURE_IF_NOT(InReplicatedOwner.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>(),
        TEXT("Owner [{}] does NOT have a ReplicationDriver. Unable to ReplicateExisting [{}]."),
        InReplicatedOwner, InExistingEntity)
    { return; }

    switch (const auto NetMode = UCk_Utils_Net_UE::Get_EntityNetMode(InReplicatedOwner))
    {
        case ECk_Net_NetModeType::Host:
        case ECk_Net_NetModeType::ClientAndHost:
        {
            const auto& RepDriver = InExistingEntity.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();
            const auto& ReplicatedObjects = UCk_Utils_ReplicatedObjects_UE::Get_ReplicatedObjects(InExistingEntity);
            const auto& IsOwningEntityDriverDependentOnThis = InReplicatedOwner.Has<ck::FTag_EntityJustCreated>();

            const auto& DependentRepDriversAddedDuringConstruction = RepDriver->Get_ExpectedNumberOfDependentReplicationDrivers();

            RepDriver->Set_ExpectedNumberOfDependentReplicationDrivers(
                Get_NumOfReplicationDriversIncludingDependents(InExistingEntity) +
                DependentRepDriversAddedDuringConstruction);

            RepDriver->Set_ReplicationData
            (
                FCk_EntityReplicationDriver_ReplicationData
                {
                    InConstructionInfos,
                    FCk_EntityReplicationDriver_ReplicateObjects_Data{FCk_ReplicatedObjects::ToWeak(ReplicatedObjects.Get_ReplicatedObjects())}
                }
                .Set_OwningEntityDriver(InReplicatedOwner.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>())
                .Set_IsOwningEntityDriverDependentOnThis(IsOwningEntityDriverDependentOnThis)
            );
            ck::UUtils_Signal_OnReplicationComplete::Broadcast(InExistingEntity, ck::MakePayload(InExistingEntity));
            ck::UUtils_Signal_OnDependentsReplicationComplete::Broadcast(InExistingEntity, ck::MakePayload(InExistingEntity));

            break;
        }
        case ECk_Net_NetModeType::Unknown:
        default:
        {
            CK_INVALID_ENUM(NetMode);
            break;
        }
    }
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Request_Replicate(
        FCk_Handle& InHandleToReplicate,
        FCk_Handle InReplicatedOwner,
        TSubclassOf<UCk_EntityScript_UE> InEntityScript,
        const FInstancedStruct& InSpawnParams,
        const TOptional<FCk_Handle>& InReplicatedContextOwnerOverride)
    -> void
{
    if (const auto ScriptStruct = InSpawnParams.GetScriptStruct();
        ck::IsValid(ScriptStruct))
    {
        CK_ENSURE_IF_NOT(ck::IsValid(ScriptStruct) && ScriptStruct->IsNameStableForNetworking(),
            TEXT("SpawnParams ScriptStruct [{}] does NOT have a stable name for networking. Is this a runtime defined struct? "
            "OR is this a Struct defined in a scripting language that does not override IsNameStableForNetworking? "
            "Handle [{}] CANNOT be replicated correctly."), ScriptStruct, InHandleToReplicate)
        { return; }
    }

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
        case ECk_Net_NetModeType::ClientAndHost:
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
                    FCk_EntityReplicationDriver_ReplicateObjects_Data{FCk_ReplicatedObjects::ToWeak(ReplicatedObjects.Get_ReplicatedObjects())}
                }
                .Set_IsOwningEntityDriverDependentOnThis(IsOwningEntityDriverDependentOnThis)
                .Set_ContextOwnerOverride(InReplicatedContextOwnerOverride.Get(FCk_Handle{}))
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
    Get_IsReplicationComplete(
        const FCk_Handle& InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle),
        TEXT("Handle [{}] does NOT have an EntityReplicationDriver OR it is still new/setting-up"), InHandle)
    { return false; }

    return ck::UUtils_Signal_OnReplicationComplete_PostFireUnbind::HasFiredAtLeastOnce(InHandle);
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Get_IsReplicationCompleteAllDependents(
        const FCk_Handle& InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle),
        TEXT("Handle [{}] does NOT have an EntityReplicationDriver OR it is still new/setting-up"), InHandle)
    { return false; }

    return ck::UUtils_Signal_OnDependentsReplicationComplete_PostFireUnbind::HasFiredAtLeastOnce(InHandle);
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Promise_OnReplicationComplete(
        FCk_Handle& InEntity,
        const FCk_Delegate_EntityReplicationDriver_OnReplicationComplete& InDelegate)
    -> void
{
    CK_SIGNAL_BIND_PROMISE(ck::UUtils_Signal_OnReplicationComplete, InEntity, InDelegate);
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Promise_OnReplicationCompleteAllDependents(
        FCk_Handle& InEntity,
        const FCk_Delegate_EntityReplicationDriver_OnReplicationComplete& InDelegate)
    -> void
{
    CK_SIGNAL_BIND_PROMISE(ck::UUtils_Signal_OnDependentsReplicationComplete, InEntity, InDelegate);
}

auto
    UCk_Utils_EntityReplicationDriver_UE::
    Get_ReplicatedHandleForWorld(
        const FCk_Handle& InHandle,
        const UWorld* InHandleOwningWorld,
        const UWorld* InTargetWorld)
    -> FCk_Handle
{
    QUICK_SCOPE_CYCLE_COUNTER(Get_ReplicatedHandleForWorld)

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Handle input is NOT valid!"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandleOwningWorld), TEXT("Handle owning world input is NOT valid!"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InTargetWorld), TEXT("Target world input is NOT valid!"))
    { return {}; }

    if (InHandleOwningWorld == InTargetWorld)
    { return InHandle; }

    const auto NetDriver = InHandleOwningWorld->GetNetDriver();
    if (ck::Is_NOT_Valid(NetDriver))
    { return {}; }

    if (NOT InHandle.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>())
    { return {}; }

    const auto& ReplicationDriver = InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();

    if (ck::Is_NOT_Valid(ReplicationDriver))
    { return {}; }

    if (NetDriver->IsUsingIrisReplication())
    {
#if UE_WITH_IRIS
        const auto SelectedNetDriver = InTargetWorld->GetNetDriver();
        if (ck::Is_NOT_Valid(SelectedNetDriver))
        { return {}; }

        const auto ReplicationSystem = NetDriver->GetReplicationSystem();
        if (ck::Is_NOT_Valid(ReplicationSystem))
        { return {}; }

        const auto SelectedReplicationSystem = SelectedNetDriver->GetReplicationSystem();
        if (ck::Is_NOT_Valid(SelectedReplicationSystem))
        { return {}; }

        const auto WorldRepBridge = ReplicationSystem->GetReplicationBridgeAs<UEngineReplicationBridge>();
        if (ck::Is_NOT_Valid(WorldRepBridge))
        { return {}; }

        const auto SelectedWorldRepBridge = SelectedReplicationSystem->GetReplicationBridgeAs<UEngineReplicationBridge>();
        if (ck::Is_NOT_Valid(SelectedWorldRepBridge))
        { return {}; }

        const auto RefHandle = WorldRepBridge->GetReplicatedRefHandle(ReplicationDriver.Get());
        const auto SelectedWorldRepDriver = Cast<UCk_Ecs_ReplicatedObject_UE>(SelectedWorldRepBridge->GetReplicatedObject(RefHandle));

        if (ck::Is_NOT_Valid(SelectedWorldRepDriver))
        { return {}; }

        return SelectedWorldRepDriver->Get_AssociatedEntity();

#else
        CK_ENSURE_IF_NOT(false, TEXT("Net Driver has Iris enabled without UE_WITH_IRIS enabled, this should NOT be possible!"))
        { return {}; }
#endif
    }
    else
    {
        if (ck::Is_NOT_Valid(InHandleOwningWorld))
        { return {}; }

        const auto& WorldNetDriver = InHandleOwningWorld->GetNetDriver();
        if (ck::Is_NOT_Valid(WorldNetDriver))
        { return {}; }

        const auto& WorldGuidCache = WorldNetDriver->GuidCache;
        if (ck::Is_NOT_Valid(WorldGuidCache))
        { return {}; }

        const auto WorldNetGuid = WorldGuidCache->GetNetGUID(ReplicationDriver.Get());
        if (ck::Is_NOT_Valid(WorldNetGuid))
        { return {}; }

        const auto SelectedWorldNetDriver = InTargetWorld->GetNetDriver();
        if (ck::Is_NOT_Valid(SelectedWorldNetDriver))
        { return {}; }

        const auto& SelectedWorldGuidCache = SelectedWorldNetDriver->GuidCache;
        if (ck::Is_NOT_Valid(SelectedWorldGuidCache))
        { return {}; }

        constexpr auto IgnoreMustBeMapped = true;
        const auto SelectedWorldRepDriver = Cast<UCk_Ecs_ReplicatedObject_UE>(SelectedWorldGuidCache->GetObjectFromNetGUID(WorldNetGuid, IgnoreMustBeMapped));
        if (ck::Is_NOT_Valid(SelectedWorldRepDriver))
        { return {}; }

        return SelectedWorldRepDriver->Get_AssociatedEntity();
    }
}

// --------------------------------------------------------------------------------------------------------------------
