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
        // ROOT FAILURE (surfaced loudly on purpose). A replicated EntityReplicationDriver was constructed on
        // a client before its outer Actor had a UWorld. _AssociatedEntity is created ONLY here, so it stays
        // permanently invalid — any child entity-driver that lists this one as its owner will park forever
        // (the park queues in the OnReps below are never drained, and this owner never becomes valid). This
        // is not expected to occur; if it ever does it explains a silently-missing replicated subtree on the
        // client (followed ~10s later by PendingReplicationRetry timeouts). Investigated 2026-06: never
        // reproduced across same-actor + cross-actor burst stress — see the EntityReplicationDriver.Net tests.
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

    for (auto& Entry : _Fragments._Items)
    {
        const auto* Handler = FCk_ReplicatedFragmentHandlerRegistry::Resolve(Entry.Data.GetScriptStruct());
        if (Handler == nullptr || NOT Handler->OnAdd)
        { continue; }

        Handler->OnAdd(Entity, Entry.Data);
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

    // wait on the owning entity to fully replicate
    if (ck::Is_NOT_Valid(OwningEntity))
    {
        // The owner driver has no valid associated entity. On the client a driver's _AssociatedEntity never
        // recovers (it is only ever set in the ctor), so this child cannot be set up now or later —
        // _PendingChildEntityConstructions is never drained. Surfaced loudly so a real occurrence is visible
        // rather than a silent missing subtree. See the ctor no-world warning above for the root cause.
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
    {
        if (ck::IsValid(ConstructionInfo.Get_ConstructionScriptArchetype()))
        {
            ConstructionInfo.Get_ConstructionScriptArchetype()->Construct(_AssociatedEntity);
        }
        else
        {
            ConstructionInfo.Get_ConstructionScript()->GetDefaultObject<UCk_Entity_ConstructionScript_PDA>()->Construct(
                _AssociatedEntity);
        }
    }

    UCk_Utils_ReplicatedObjects_UE::Add(_AssociatedEntity, FCk_ReplicatedObjects{}.
        Set_ReplicatedObjects(FCk_ReplicatedObjects::ToStrong(_ReplicationData.Get_ReplicatedObjectsData().Get_Objects())));

    // --------------------------------------------------------------------------------------------------------------------

    // Make sure to call this on "self" since the # of dependent rep driver include "self" as well
    DoAdd_SyncedDependentReplicationDriver();

    // This is necessary in case this Replicated Entity was built from inside the OwningEntity's ConstructionScript.
    // If that is the case, then this Replicated Entity is a dependent
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
            // The owner driver has no valid associated entity. On the client a driver's _AssociatedEntity
            // never recovers, so this child entity-script cannot be set up now or later —
            // _PendingChildEntityConstructions is never drained. Surfaced loudly so a real occurrence is
            // visible rather than a silent missing subtree. See the ctor no-world warning for the root cause.
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

    // Re-home the ContextOwner the client copy resolves to. Request_SetupEntityWithLifetimeOwner above
    // inherited it from the lifetime owner (for the non-self-referencing case, the ActorRelay channel),
    // which is the regression. An unset (invalid) override means the authority resolved the entity as
    // its own ContextOwner, so map it back to self; otherwise adopt the replicated override entity.
    if (const auto& ContextOwnerOverride = _ReplicationData_EntityScript.Get_ContextOwnerOverride();
        ck::IsValid(ContextOwnerOverride))
    { UCk_Utils_ContextOwner_UE::Request_Override(_AssociatedEntity, ContextOwnerOverride); }
    else
    { UCk_Utils_ContextOwner_UE::Request_OverrideToSelf(_AssociatedEntity); }

    // On clients, the entity's ownership chain may not resolve to a World yet
    // (the owning entity hasn't been fully constructed). Add the World directly
    // so Get_WorldForEntity can resolve without walking the chain.
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
        // This is necessary in case this Replicated Entity was built from inside the OwningEntity's ConstructionScript.
        // If that is the case, then this Replicated Entity is a dependent
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

    // wait on the owning entity to fully replicate
    if (ck::Is_NOT_Valid(OwningEntity))
    {
        // The owner driver has no valid associated entity. On the client a driver's _AssociatedEntity never
        // recovers, so this child ability cannot be set up now or later — _PendingChildAbilityEntityConstructions
        // is never drained. Surfaced loudly so a real occurrence is visible. See the ctor no-world warning.
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

    // For Abilities, we have to pass the information for construction to the Ability Processor. This will be removed once
    // the processor has had the chance to construct the Entity correctly
    _AssociatedEntity.Add<FCk_EntityReplicationDriver_AbilityData>(_ReplicationData_Ability);

    UCk_Utils_ReplicatedObjects_UE::Add(_AssociatedEntity, FCk_ReplicatedObjects{}.
        Set_ReplicatedObjects(FCk_ReplicatedObjects::ToStrong(_ReplicationData_Ability.Get_ReplicatedObjectsData().Get_Objects())));

    // --------------------------------------------------------------------------------------------------------------------

    // NOTE: The #SyncedDrivers count is NOT incremented here. Instead, it is handled in the FProcessor_AbilityOwner_HandleRequests processor.
    // This ensures that the increment occurs only after the replicated ability has been created and assigned.
    // Incrementing prematurely would cause the ReplicationComplete and ReplicationCompleteAllDependents signals to fire too early.
    // If the replicated ability is added as an EntityExtension, any attempts to manipulate extended features (e.g., Attributes)
    // would fail because those features would not yet exist.
    //_ReplicationData_Ability.Get_OwningEntityDriver()->DoAdd_SyncedDependentReplicationDriver();
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