#include "CkEntityReplicationDriver_Fragment.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include "Net/Core/PushModel/PushModel.h"

#include <Engine/World.h>

#include <Net/UnrealNetwork.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_Fragment_EntityReplicationDriver_Rep::
    UCk_Fragment_EntityReplicationDriver_Rep(
        const FObjectInitializer& InObjInitializer)
    : Super(InObjInitializer)
{
    _Fragments._OwningDriver = this;

    if (IsTemplate())
    { return; }

    auto World = UObject::GetWorld();

    if (ck::Is_NOT_Valid(World))
    {
        // ROOT FAILURE, loud on purpose: _AssociatedEntity is created ONLY here, so a driver built before
        // its outer Actor had a UWorld stays permanently invalid and every child driver that names it as
        // owner parks forever in a queue nothing ever drains. See CkEcs/CLAUDE.md.
        ck::ecs::Warning(TEXT("EntityReplicationDriver constructed with NO valid UWorld (Outer=[{}]). Its associated "
            "entity can never be created, permanently stranding any dependent child drivers on the client."), GetOuter());
        return;
    }

    // Creating via registry since we want 'Request_SetupEntityWithLifetimeOwner' to execute during the OnRep_XYZ
    auto TransientHandle = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(World);
    auto RegistryView = TransientHandle.Get_RegistryView();
    _AssociatedEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(RegistryView);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Get_IsReplicationCompleteOnAllDependents() const
        -> bool
{
    if (const auto Entity = Get_AssociatedEntity();
        UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(Entity))
    { return true; }

    return Get_ExpectedNumberOfDependentReplicationDrivers() == _NumSyncedDependentReplicationDrivers;
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_Custom, REPNOTIFY_OnChanged, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _ReplicationData, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _ReplicationData_EntityScript, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _ReplicationData_Ability, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _ExpectedNumberOfDependentReplicationDrivers, Params);

    constexpr auto FragmentParams = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _Fragments, FragmentParams);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    PostLink()
    -> void
{
    auto Entity = Get_AssociatedEntity();
    if (ck::Is_NOT_Valid(Entity))
    { return; }

    // Link is pure bookkeeping: FProcessor_ReplicatedFragments_Dispatch applies these entries later
    // in the frame, after OnConstructed-driven composition has run.
    for (auto& Entry : _Fragments._Items)
    {
        const auto* Handler = FCk_PersistenceHandlerRegistry::Resolve(Entry.Data.GetScriptStruct());
        if (Handler == nullptr || NOT Handler->NetApply)
        { continue; }

        Entry._PendingApply = true;
        Entry._PendingSinceRealTimeSeconds = 0.0;
        Entity.AddOrGet<ck::FTag_RepFragments_PendingApply>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    GetReplicatedCustomConditionState(
        FCustomPropertyConditionState& OutActiveState) const
    -> void
{
    Super::GetReplicatedCustomConditionState(OutActiveState);

    DOREPCUSTOMCONDITION_ACTIVE_FAST(ThisType, _ReplicationData, ck::IsValid(Get_AssociatedEntity()));
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    OnRep_ReplicationData()
    -> void
{
    if ([[maybe_unused]] const auto ShouldSkipIfAllObjectsAreNotYetResolved =
        AnyOf(_ReplicationData.Get_ReplicatedObjectsData().Get_Objects(), ck::algo::Is_NOT_Valid{}))
    { return; }

    // --------------------------------------------------------------------------------------------------------------------

    const auto& OwningEntityDriver = _ReplicationData.Get_OwningEntityDriver();

    CK_ENSURE_IF_NOT(ck::IsValid(OwningEntityDriver),
        TEXT("OwningEntityDriver is NOT valid. Somehow the ReplicationDriver was NOT added to the OwningEntity but WAS "
            "added to the child Entity.{}"), ck::Context(this))
    { return; }

    const auto OwningEntity = OwningEntityDriver->Get_AssociatedEntity();

    if (ck::Is_NOT_Valid(OwningEntity))
    {
        // NOT a wait: a client driver's _AssociatedEntity is only ever set in its ctor, so this child can
        // never be set up and the queue below is never drained. Root cause is the ctor no-world warning.
        ck::ecs::Warning(TEXT("Replicated child driver [{}] cannot construct: owner driver [{}] has no valid associated "
            "entity and will never recover. Child parked in a queue that is never drained."), this, OwningEntityDriver);
        OwningEntityDriver->_PendingChildEntityConstructions.Emplace(this);
        return;
    }

    // --------------------------------------------------------------------------------------------------------------------

    const auto& ConstructionInfos = _ReplicationData.Get_ConstructionInfos();

    CK_ENSURE_IF_NOT(NOT ConstructionInfos.IsEmpty(),
        TEXT("ConstructionInfos is empty. Unable to proceed with replicating the Entity.{}"), ck::Context(this))
    { return; }

    for (const auto& ConstructionInfo : ConstructionInfos)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(ConstructionInfo.Get_ConstructionScript()),
            TEXT("ConstructionScript is [{}]. Unable to proceed with replicating the Entity"),
            ConstructionInfo.Get_ConstructionScript())
        { return; }
    }

    // --------------------------------------------------------------------------------------------------------------------

    _AssociatedEntity._ReplicationDriver = this;

    UCk_Utils_EntityLifetime_UE::Request_SetupEntityWithLifetimeOwner(_AssociatedEntity, OwningEntity);

    UCk_Utils_Net_UE::Add(_AssociatedEntity, FCk_Net_ConnectionSettings
    {
        ECk_Replication::Replicates,
        ECk_Net_NetModeType::Client,
        ECk_Net_EntityNetRole::Proxy
    });

    for (const auto& ConstructionInfo : ConstructionInfos)
    { ck::entity_replication_driver::Construct_FromInfo(ConstructionInfo, _AssociatedEntity); }

    UCk_Utils_ReplicatedObjects_UE::Add(_AssociatedEntity, FCk_ReplicatedObjects{}.
        Set_ReplicatedObjects(FCk_ReplicatedObjects::ToStrong(_ReplicationData.Get_ReplicatedObjectsData().Get_Objects())));

    // --------------------------------------------------------------------------------------------------------------------

    // Make sure to call this on "self" since the # of dependent rep driver include "self" as well
    DoAdd_SyncedDependentReplicationDriver();

    // A Replicated Entity built from inside the OwningEntity's ConstructionScript is a dependent of it
    if (_ReplicationData.Get_IsOwningEntityDriverDependentOnThis())
    {
        OwningEntityDriver->DoAdd_SyncedDependentReplicationDriver();
    }
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    OnRep_ReplicationData_EntityScript() -> void
{
    if ([[maybe_unused]] const auto ShouldSkipIfAllObjectsAreNotYetResolved =
        AnyOf(_ReplicationData_EntityScript.Get_ReplicatedObjectsData().Get_Objects(), ck::algo::Is_NOT_Valid{}))
    {
        return;
    }
    const auto EntityScriptClass = _ReplicationData_EntityScript.Get_EntityScriptClass();
    const auto SpawnParams = _ReplicationData_EntityScript.Get_SpawnParams();
    const auto IsSelfReferencing = _ReplicationData_EntityScript.Get_OwningEntityDriver() == this;

    if (NOT IsSelfReferencing)
    {
        const auto OwningEntity = _ReplicationData_EntityScript.Get_OwningEntityDriver()->Get_AssociatedEntity();

        if (ck::Is_NOT_Valid(OwningEntity))
        {
            // NOT a wait: a client driver's _AssociatedEntity never recovers, so this child entity-script
            // can never be set up and the queue below is never drained. See the ctor no-world warning.
            ck::ecs::Warning(TEXT("Replicated child entity-script driver [{}] cannot construct: owner driver [{}] has no "
                "valid associated entity and will never recover. Child parked in a queue that is never drained."),
                this, _ReplicationData_EntityScript.Get_OwningEntityDriver());
            _ReplicationData_EntityScript.Get_OwningEntityDriver()->_PendingChildEntityConstructions.Emplace(this);
            return;
        }

        UCk_Utils_EntityLifetime_UE::Request_SetupEntityWithLifetimeOwner(_AssociatedEntity, OwningEntity);
    }
    else
    {
        const auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(GetWorld());
        UCk_Utils_EntityLifetime_UE::Request_SetupEntityWithLifetimeOwner(_AssociatedEntity, TransientEntity);
    }

    // Re-home the ContextOwner: the setup call above inherited it from the lifetime owner. An unset
    // override means the authority resolved the entity as its own ContextOwner, so map it back to self.
    if (const auto& ContextOwnerOverride = _ReplicationData_EntityScript.Get_ContextOwnerOverride();
        ck::IsValid(ContextOwnerOverride))
    { UCk_Utils_ContextOwner_UE::Request_Override(_AssociatedEntity, ContextOwnerOverride, {}); }
    else
    { UCk_Utils_ContextOwner_UE::Request_OverrideToSelf(_AssociatedEntity, {}); }

    // The ownership chain may not resolve to a World yet on clients, so Get_WorldForEntity is given
    // one directly rather than having to walk it.
    _AssociatedEntity.AddOrGet<TWeakObjectPtr<UWorld>>(GetWorld());

    auto ThisAsWeakPtr = TWeakObjectPtr<ThisType>{this};
    UCk_Utils_EntityScript_UE::Add(_AssociatedEntity, EntityScriptClass, SpawnParams, [ThisAsWeakPtr](FCk_Handle InHandle)
    {
        if (ck::Is_NOT_Valid(ThisAsWeakPtr))
        { return; }

        UCk_Utils_ReplicatedObjects_UE::Add(ThisAsWeakPtr->_AssociatedEntity, FCk_ReplicatedObjects{}.
            Set_ReplicatedObjects(FCk_ReplicatedObjects::ToStrong(ThisAsWeakPtr->_ReplicationData_EntityScript.Get_ReplicatedObjectsData().Get_Objects())));

        // --------------------------------------------------------------------------------------------------------------------
        // Make sure to call this on "self" since the # of dependent rep driver include "self" as well
        ThisAsWeakPtr->DoAdd_SyncedDependentReplicationDriver();
        // A Replicated Entity built from inside the OwningEntity's ConstructionScript is a dependent of it
        if (ThisAsWeakPtr->_ReplicationData_EntityScript.Get_IsOwningEntityDriverDependentOnThis())
        {
            ThisAsWeakPtr->_ReplicationData_EntityScript.Get_OwningEntityDriver()->DoAdd_SyncedDependentReplicationDriver();
        }

        if (UCk_Utils_Net_UE::Get_Replication(InHandle) == ECk_Replication::Replicates)
        {
            if (UCk_Utils_EntityReplicationDriver_UE::Get_IsReplicationCompleteAllDependents(InHandle))
            {
                InHandle.Add<ck::FTag_EntityReplicationDriver_FireOnDependentReplicationComplete>();
            }
        }
    });

    _AssociatedEntity._ReplicationDriver = this;

    UCk_Utils_Net_UE::Add(_AssociatedEntity, FCk_Net_ConnectionSettings
        {
            ECk_Replication::Replicates,
            ECk_Net_NetModeType::Client,
            ECk_Net_EntityNetRole::Proxy
        });
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    OnRep_ReplicationData_Ability()
    -> void
{
    // TODO: This is a temporary fix. We need to find a better way to handle this
    if (_AssociatedEntity.Has<ck::FFragment_LifetimeOwner>())
    { return; }

    // wait for the data to be fully replicated
    if (ck::Is_NOT_Valid(_ReplicationData_Ability.Get_AbilityScriptClass()))
    { return; }

    if ([[maybe_unused]] const auto ShouldSkipIfAllObjectsAreNotYetResolved =
        AnyOf(_ReplicationData_Ability.Get_ReplicatedObjectsData().Get_Objects(), ck::algo::Is_NOT_Valid{}))
    { return; }

    // --------------------------------------------------------------------------------------------------------------------

    const auto OwningEntity = _ReplicationData_Ability.Get_OwningEntityDriver()->Get_AssociatedEntity();

    if (ck::Is_NOT_Valid(OwningEntity))
    {
        // NOT a wait: a client driver's _AssociatedEntity never recovers, so this child ability can never
        // be set up and the queue below is never drained. See the ctor no-world warning.
        ck::ecs::Warning(TEXT("Replicated child ability driver [{}] cannot construct: owner driver [{}] has no valid "
            "associated entity and will never recover. Child parked in a queue that is never drained."),
            this, _ReplicationData_Ability.Get_OwningEntityDriver());
        _ReplicationData_Ability.Get_OwningEntityDriver()->_PendingChildAbilityEntityConstructions.Emplace(this);
        return;
    }

    ck::ecs::Verbose(TEXT("Adding Ability [{}] to [{}] with Owning Entity [{}] on Client [{}].{}"),
        _ReplicationData_Ability.Get_AbilityScriptClass(), Get_AssociatedEntity(), _ReplicationData_Ability.Get_OwningEntityDriver()->Get_AssociatedEntity(),
        GetWorld()->GetFirstLocalPlayerFromController(), this);

    // --------------------------------------------------------------------------------------------------------------------

    _AssociatedEntity._ReplicationDriver = this;

    UCk_Utils_EntityLifetime_UE::Request_SetupEntityWithLifetimeOwner(_AssociatedEntity, OwningEntity);

    UCk_Utils_Net_UE::Add(_AssociatedEntity, FCk_Net_ConnectionSettings
    {
        ECk_Replication::Replicates,
        ECk_Net_NetModeType::Client,
        ECk_Net_EntityNetRole::Proxy
    });

    // Handed to the Ability Processor for construction, which removes it once the Entity is built
    _AssociatedEntity.Add<FCk_EntityReplicationDriver_AbilityData>(_ReplicationData_Ability);

    UCk_Utils_ReplicatedObjects_UE::Add(_AssociatedEntity, FCk_ReplicatedObjects{}.
        Set_ReplicatedObjects(FCk_ReplicatedObjects::ToStrong(_ReplicationData_Ability.Get_ReplicatedObjectsData().Get_Objects())));

    // --------------------------------------------------------------------------------------------------------------------

    // The #SyncedDrivers count is deliberately NOT incremented here — FProcessor_AbilityOwner_HandleRequests
    // does it once the replicated ability exists. Incrementing now fires ReplicationComplete before an
    // extension-added ability's features exist, and manipulating them then fails.
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    OnRep_ExpectedNumberOfDependentReplicationDrivers() const
    -> void
{
    if (_NumSyncedDependentReplicationDrivers == Get_ExpectedNumberOfDependentReplicationDrivers())
    {
        auto AssociatedEntity = Get_AssociatedEntity();
        AssociatedEntity.AddOrGet<ck::FTag_EntityReplicationDriver_FireOnDependentReplicationComplete>();
    }
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    DoAdd_SyncedDependentReplicationDriver()
    -> void
{
    ++_NumSyncedDependentReplicationDrivers;

    CK_ENSURE_IF_NOT(_NumSyncedDependentReplicationDrivers <= _ExpectedNumberOfDependentReplicationDrivers,
        TEXT("The number of Synced Dependent Replication Drivers [{}] is greater than the expected amount of [{}] on Associated Entity [{}]. This is likely due to faulty logic."),
        _NumSyncedDependentReplicationDrivers,
        _ExpectedNumberOfDependentReplicationDrivers,
        Get_AssociatedEntity())
    { return; }

    if (_ExpectedNumberOfDependentReplicationDrivers == _NumSyncedDependentReplicationDrivers)
    {
        auto AssociatedEntity = Get_AssociatedEntity();
        AssociatedEntity.AddOrGet<ck::FTag_EntityReplicationDriver_FireOnDependentReplicationComplete>();
    }
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Set_ReplicationData(
        const FCk_EntityReplicationDriver_ReplicationData& InReplicationData)
    -> void
{
    _ReplicationData = InReplicationData;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _ReplicationData, this);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Set_ReplicationData_EntityScript(
        const FCk_EntityReplicationDriver_ReplicationData_EntityScript& InReplicationData)
    -> void
{
    _ReplicationData_EntityScript = InReplicationData;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _ReplicationData_EntityScript, this);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Set_ReplicationData_Ability(
        const FCk_EntityReplicationDriver_AbilityData& InReplicationData)
    -> void
{
    _ReplicationData_Ability = InReplicationData;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _ReplicationData_Ability, this);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    Set_ExpectedNumberOfDependentReplicationDrivers(
        int32 InNumOfDependents)
    -> void
{
    CK_ENSURE_IF_NOT(GetWorld()->GetNetMode() != NM_Client,
        TEXT("Setting the ExpectedNumberOfDependentReplicationDrivers is only allowed on the Server.{}"), ck::Context(this))
    { return; }

    _ExpectedNumberOfDependentReplicationDrivers = InNumOfDependents;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _ExpectedNumberOfDependentReplicationDrivers, this);
}

// --------------------------------------------------------------------------------------------------------------------
// Generic Fragment Container

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    MarkFragmentDirty(
        FCk_ReplicatedFragmentEntry& InEntry)
    -> void
{
    _Fragments.MarkItemDirty(InEntry);
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _Fragments, this);
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    FindEntry(
        const UScriptStruct* InType)
    -> FCk_ReplicatedFragmentEntry*
{
    return _Fragments._Items.FindByPredicate([InType](const FCk_ReplicatedFragmentEntry& InEntry)
    {
        return InEntry.Data.GetScriptStruct() == InType;
    });
}

auto
    UCk_Fragment_EntityReplicationDriver_Rep::
    SetFragmentData_Runtime(
        const FInstancedStruct& InData)
    -> int32
{
    if (auto* Entry = FindEntry(InData.GetScriptStruct()))
    {
        Entry->Data = InData;
        MarkFragmentDirty(*Entry);
        return _Fragments._Items.IndexOfByPredicate([Entry](const FCk_ReplicatedFragmentEntry& InEntry)
        {
            return &InEntry == Entry;
        });
    }

    auto& NewEntry = _Fragments._Items.AddDefaulted_GetRef();
    NewEntry.Data = InData;
    MarkFragmentDirty(NewEntry);

    return _Fragments._Items.Num() - 1;
}

// --------------------------------------------------------------------------------------------------------------------