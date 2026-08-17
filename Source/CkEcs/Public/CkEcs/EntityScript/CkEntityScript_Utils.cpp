#include "CkEntityScript_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Reflection/CkReflection_Utils.h"
#include "CkCore/Validation/CkUntracedStructSafety.h"

#include "Misc/MTAccessDetector.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h" // ck::FTag_EntityJustCreated
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript.h"             // UCk_EntityScript_UE (complete type)
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Net/CkNet_Fragment.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/CkEcsLog.h"                       // load-gate spawn suppression Warning
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h" // load-gate + spawn-window queries

#include <Engine/BlueprintGeneratedClass.h>
#include <UObject/ObjectPtr.h>

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_EntityScript_UE, FCk_Handle_EntityScript, ck::FFragment_EntityScript_Current);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_entity_script_utils
{
    // While a CkSnapshot load owns the world (the load gate is active), the loader is the SOLE creator of
    // world population: saved rows respawn through its own window (FCk_ScopedLoaderSpawnWindow) and replayed
    // constructions re-create their ConstructSpawned children. Any other spawn is world POLICY reacting to
    // the half-rebuilt world — a population census reading near-zero, an adopt-or-spawn task whose adopt scan
    // ran before the loader materialized the restored copy. Admitting those double-populates the world and
    // the next capture saves both copies (the save-inflation incident, 2026-07-29: +77 NPCs and a doubled
    // StoreDriver subordinate family in one save->load->save cycle). Suppressed spawns return an INVALID
    // pending handle; Promise_OnConstructed no-ops on it and the completion delegate reports
    // Failed_NotEnqueued — reconcile-shaped callers converge on their next real-world evaluation.
    auto Get_IsSpawnSuppressedByLoadGate(const FCk_Handle& InLifetimeOwner, const UObject* InEntityScriptClassOrArchetype) -> bool
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InLifetimeOwner);
        if (ck::Is_NOT_Valid(World))
        { return false; }

        const auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
        if (ck::Is_NOT_Valid(EcsWorld))
        { return false; }

        if (NOT EcsWorld->Get_IsLoadGateActive())
        { return false; }

        if (EcsWorld->Get_IsInLoaderSpawnWindow())
        { return false; }

        if (EcsWorld->Get_IsInRendezvousSpawnWindow())
        { return false; }

        if (UCk_Utils_EntityLifetime_UE::Get_IsInsideConstructionWindow(InLifetimeOwner))
        { return false; }

        ck::ecs::Warning(TEXT("Request_SpawnEntity of [{}] under [{}] SUPPRESSED: a CkSnapshot load owns the world. "
            "This spawn is a world-policy decision made against the half-rebuilt world — the loader respawns what "
            "the save recorded; spawn-on-demand logic must re-evaluate once the world is coherent. If this spawn "
            "re-creates IDENTITY-BEARING content the loader adopts (a SaveKey / adopt-label rendezvous target), "
            "declare it: Request_SpawnEntity_LoadRendezvous, or FCk_ScopedRendezvousSpawnWindow in C++."),
            InEntityScriptClassOrArchetype, InLifetimeOwner);
        return true;
    }

    auto ValidateRetainedSpawnParams(const TCHAR* InOperation, const FInstancedStruct& InSpawnParams) -> bool
    {
        if (NOT InSpawnParams.IsValid())
        { return true; }

        const auto* ScriptStruct = InSpawnParams.GetScriptStruct();
        const auto HasScriptStruct = ScriptStruct != nullptr;
        CK_ENSURE_IF_NOT(HasScriptStruct,
            TEXT("{} rejected EntityScript spawn params without a reflected struct type"), InOperation)
        { }
        if (NOT HasScriptStruct)
        { return false; }

        const auto Safety = ck::Analyze_UntracedStructSafety(ScriptStruct);
        const auto IsSpawnParamsSafe = Safety.IsGcIndependent();
        CK_ENSURE_IF_NOT(IsSpawnParamsSafe,
            TEXT("{} rejected unsafe EntityScript spawn params [{}]; [{}]: {}"),
            InOperation,
            ScriptStruct->GetName(),
            Safety.FailurePath,
            Safety.FailureReason)
        { }

        if (NOT IsSpawnParamsSafe)
        { return false; }

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityScript_UE::
    Get_ScriptClass(
        const FCk_Handle_EntityScript& InHandle)
    -> TSubclassOf<UCk_EntityScript_UE>
{
    const auto& Current = InHandle.Get<ck::FFragment_EntityScript_Current>();

    CK_ENSURE_IF_NOT(ck::IsValid(Current.Get_Script()), TEXT("The EntityScript [{}] for Handle [{}] is NOT valid"),
        Current.Get_Script(), InHandle)
    { return {}; }

    return InHandle.Get<ck::FFragment_EntityScript_Current>().Get_Script()->GetClass();
}

auto
    UCk_Utils_EntityScript_UE::
    Request_SpawnEntity(
        FCk_Handle& InLifetimeOwner,
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass,
        FInstancedStruct InSpawnParams,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_PendingEntityScript
{
    if (NOT ck_entity_script_utils::ValidateRetainedSpawnParams(TEXT("Request_SpawnEntity"), InSpawnParams))
    {
        InDelegate.ExecuteIfBound(InLifetimeOwner, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    const auto EntityScriptClassIsValid = ck::IsValid(InEntityScriptClass);
    CK_ENSURE_IF_NOT(EntityScriptClassIsValid,
        TEXT("EntityScriptClass [{}] is INVALID. Unable to SpawnEntity using LifetimeOwner [{}]."),
        InEntityScriptClass, InLifetimeOwner)
    {
        InDelegate.ExecuteIfBound(InLifetimeOwner, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    const auto LifetimeOwnerIsValid = ck::IsValid(InLifetimeOwner);
    CK_ENSURE_IF_NOT(LifetimeOwnerIsValid,
        TEXT("LifetimeOwner is INVALID. Unable to SpawnEntity using EntityScriptClass [{}]."),
        InEntityScriptClass)
    {
        InDelegate.ExecuteIfBound(InLifetimeOwner, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    if (ck_entity_script_utils::Get_IsSpawnSuppressedByLoadGate(InLifetimeOwner, InEntityScriptClass))
    {
        InDelegate.ExecuteIfBound(InLifetimeOwner, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    if (const auto DefaultObject = InEntityScriptClass->GetDefaultObject<UCk_EntityScript_UE>();
        ck::IsValid(DefaultObject))
    {
        if (DefaultObject->Get_EffectiveReplication() == ECk_Replication::Replicates &&
            UCk_Utils_Net_UE::Get_EntityNetMode(InLifetimeOwner) == ECk_Net_NetModeType::Client)
        {
            const auto& PendingEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);
            auto& PendingFragment = InLifetimeOwner.AddOrGet<ck::FFragment_PendingReplication>();
            PendingFragment.Add(InEntityScriptClass.Get(), PendingEntity, InSpawnParams);

            // Completion is LOCAL-machine: on a client the authority owns this spawn and nothing is queued
            // here, so there is no local processing to report. Promise_OnConstructed on the returned pending
            // handle is what resolves when the replicated entity arrives.
            InDelegate.ExecuteIfBound(PendingEntity, ECk_Request_OperationResult::Failed_NotEnqueued);

            return FCk_Handle_PendingEntityScript{PendingEntity};
        }
    }
    else
    {
        CK_ENSURE(ck::IsValid(DefaultObject),
            TEXT("The EntityScriptClass [{}] does NOT have a ClassDefaultObject (or could not load it). This "
                "may result in Replicated EntityScripts to have an additional copy on the clients!"), InEntityScriptClass);
    }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);

    CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, NewEntity,
        TEXT("Request_SpawnEntity called with class: {}"), InEntityScriptClass);

    const auto CDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_EntityScript_UE>(InEntityScriptClass);
    return Add(NewEntity, MakeWeakObjectPtr(CDO), InSpawnParams, nullptr, InDelegate);
}

auto
    UCk_Utils_EntityScript_UE::
    Request_SpawnEntity_LoadRendezvous(
        FCk_Handle& InLifetimeOwner,
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass,
        FInstancedStruct InSpawnParams,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_PendingEntityScript
{
    auto EcsWorld = static_cast<UCk_EcsWorld_Subsystem_UE*>(nullptr);
    if (const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InLifetimeOwner);
        ck::IsValid(World))
    { EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>(); }

    const auto RendezvousWindow = FCk_ScopedRendezvousSpawnWindow{EcsWorld};
    return Request_SpawnEntity(InLifetimeOwner, InEntityScriptClass, InSpawnParams, InDelegate);
}

auto
    UCk_Utils_EntityScript_UE::
    Request_SpawnEntity_Archetype(
        FCk_Handle& InLifetimeOwner,
        UCk_EntityScript_UE* InEntityScriptClassArchetype,
        FInstancedStruct InSpawnParams,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_PendingEntityScript
{
    if (NOT ck_entity_script_utils::ValidateRetainedSpawnParams(TEXT("Request_SpawnEntity_Archetype"), InSpawnParams))
    {
        InDelegate.ExecuteIfBound(InLifetimeOwner, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    const auto ArchetypeIsValid = ck::IsValid(InEntityScriptClassArchetype);
    CK_ENSURE_IF_NOT(ArchetypeIsValid,
        TEXT("EntityScriptClass [{}] is INVALID. Unable to SpawnEntity using LifetimeOwner [{}]."),
        InEntityScriptClassArchetype, InLifetimeOwner)
    {
        InDelegate.ExecuteIfBound(InLifetimeOwner, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    const auto LifetimeOwnerIsValid = ck::IsValid(InLifetimeOwner);
    CK_ENSURE_IF_NOT(LifetimeOwnerIsValid,
        TEXT("LifetimeOwner is INVALID. Unable to SpawnEntity using EntityScriptClass [{}]."),
        InEntityScriptClassArchetype)
    {
        InDelegate.ExecuteIfBound(InLifetimeOwner, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    if (ck_entity_script_utils::Get_IsSpawnSuppressedByLoadGate(InLifetimeOwner, InEntityScriptClassArchetype))
    {
        InDelegate.ExecuteIfBound(InLifetimeOwner, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    if (InEntityScriptClassArchetype->Get_EffectiveReplication() == ECk_Replication::Replicates &&
        UCk_Utils_Net_UE::Get_EntityNetMode(InLifetimeOwner) == ECk_Net_NetModeType::Client)
    {
        const auto& PendingEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);
        auto& PendingFragment = InLifetimeOwner.AddOrGet<ck::FFragment_PendingReplication>();
        PendingFragment.Add(InEntityScriptClassArchetype->GetClass(), PendingEntity, InSpawnParams);

        // See Request_SpawnEntity: nothing is queued locally on a client, so there is no local outcome.
        InDelegate.ExecuteIfBound(PendingEntity, ECk_Request_OperationResult::Failed_NotEnqueued);

        return FCk_Handle_PendingEntityScript{PendingEntity};
    }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);

    CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, NewEntity,
        TEXT("Request_SpawnEntity called with class: {}"), InEntityScriptClassArchetype);

    return Add(NewEntity, InEntityScriptClassArchetype, InSpawnParams, nullptr, InDelegate);
}

auto
    UCk_Utils_EntityScript_UE::
    TryGet_Entity_EntityScript_InOwnershipChain(
        const FCk_Handle& InHandle)
    -> FCk_Handle
{
    return UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(InHandle,
    [&](const FCk_Handle& Handle)
    {
        return Handle.Has<ck::FFragment_EntityScript_Current>();
    });
}

auto
    UCk_Utils_EntityScript_UE::
    Add(
        FCk_Handle& InScriptEntity,
        const TSubclassOf<UCk_EntityScript_UE>& InEntityScriptClass,
        const FInstancedStruct& InSpawnParams,
        const FCk_EntityScript_PostConstruction_Func& InOptionalFunc,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_PendingEntityScript
{
    return Add(InScriptEntity, InEntityScriptClass.GetDefaultObject(), InSpawnParams, InOptionalFunc, InDelegate);
}

auto
    UCk_Utils_EntityScript_UE::
    Add(
        FCk_Handle& InScriptEntity,
        const TWeakObjectPtr<UCk_EntityScript_UE>& InEntityScriptClassArchetype,
        const FInstancedStruct& InSpawnParams,
        const FCk_EntityScript_PostConstruction_Func& InOptionalFunc,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_PendingEntityScript
{
    if (NOT ck_entity_script_utils::ValidateRetainedSpawnParams(TEXT("Add EntityScript"), InSpawnParams))
    {
        InDelegate.ExecuteIfBound(InScriptEntity, ECk_Request_OperationResult::Failed_NotEnqueued);
        return {};
    }

    CK_ENSURE_IF_NOT(ck::IsValid(InScriptEntity),
        TEXT("Cannot Add EntityScript [{}] to an INVALID Entity. Aborting to avoid operating on a dead Registry handle."),
        InEntityScriptClassArchetype)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InEntityScriptClassArchetype), TEXT("Invalid EntityScript supplied, cannot request to Spawn Entity"))
    { return {}; }

    CK_ENSURE_IF_NOT(NOT Has(InScriptEntity),
        TEXT("Entity [{}] ALREADY has an EntityScript"), InScriptEntity)
    { return {}; }

    UCk_Utils_Handle_UE::Set_DebugName(InScriptEntity, *ck::Format_UE(TEXT("{}"), InEntityScriptClassArchetype));

    // Derive Net_Params from the archetype's effective replication intent combined with the TransientEntity's
    // NetMode/NetRole. Covers the transient-owner case (Request_CreateEntity skips Net_Params inheritance) and
    // direct Add() callers. When the entity already inherited Net_Params from a non-transient owner, respect that.
    if (NOT InScriptEntity.Has<ck::FFragment_Net_Params>())
    {
        const auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InScriptEntity);
        const auto& Settings = TransientEntity.Get<ck::FFragment_Net_Params>().Get_ConnectionSettings();

        const auto Replication = InEntityScriptClassArchetype->Get_EffectiveReplication();
        auto NetRole = Settings.Get_NetRole();

        if (Replication == ECk_Replication::Replicates
            && Settings.Get_NetMode() == ECk_Net_NetModeType::Client)
        {
            NetRole = ECk_Net_EntityNetRole::Proxy;
        }

        UCk_Utils_Net_UE::Add(InScriptEntity, FCk_Net_ConnectionSettings{
            Replication, Settings.Get_NetMode(), NetRole});
    }

    CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, InScriptEntity,
        TEXT("Add() creating request entity for class: {}"), InEntityScriptClassArchetype);

    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InScriptEntity);

    const auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InScriptEntity);

    const auto Request = FCk_Request_EntityScript_SpawnEntity{
                             InScriptEntity,
                             LifetimeOwner,
                             InEntityScriptClassArchetype}
                        .Set_ContextOwner(LifetimeOwner)
                        .Set_SpawnParams(InSpawnParams)
                        .Set_PostConstruction_Func(InOptionalFunc);

    if (InDelegate.IsBound())
    { Request.Set_CompletionDelegate(InDelegate); }

    RequestEntity.Add<ck::FFragment_EntityScript_RequestSpawnEntity>(Request);

    CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, InScriptEntity,
        TEXT("Spawn request entity created, awaiting processing"));

    return FCk_Handle_PendingEntityScript{InScriptEntity};
}

auto
    UCk_Utils_EntityScript_UE::
    TryInjectEntityScriptSpawnParams(
        UCk_EntityScript_UE* InEntityScript,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    if (ck::Is_NOT_Valid(InEntityScript) || ck::Is_NOT_Valid(InSpawnParams))
    { return; }

    if (const auto& SpawnParamsStruct = InSpawnParams.GetScriptStruct();
        ck::IsValid(SpawnParamsStruct))
    {
        QUICK_SCOPE_CYCLE_COUNTER(TryInjectEntityScriptSpawnParams)
        const auto& SpawnParamsData = InSpawnParams.GetMemory();

        for (TFieldIterator<FProperty> PropIt(SpawnParamsStruct, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
        {
            const auto* SpawnParamsProp = *PropIt;

            const auto& PropertyName = UCk_Utils_Reflection_UE::Get_SanitizedUserDefinedPropertyName(SpawnParamsProp);
            const auto* EntityScriptProp = UCk_Utils_Reflection_UE::Get_PropertyBySanitizedName(InEntityScript, PropertyName);

            if (ck::Is_NOT_Valid(EntityScriptProp))
            {
                // BP must have all properties in struct; code-authored (C++/AS) classes may legitimately receive
                // a shared struct and ignore extra fields. CLASS_CompiledFromBlueprint cannot be used to tell
                // them apart — the AS engine fork sets that flag on AngelScript classes too.
                CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(::Cast<UBlueprintGeneratedClass>(InEntityScript->GetClass()), ck::IsValid_Policy_NullptrOnly{}),
                    TEXT("Failed to find ExposedOnSpawn Property [{}] on BP Entity Script [{}]. Cannot inject this SpawnParam"),
                    PropertyName,
                    InEntityScript)
                {}
                continue;
            }

            auto* EntityScriptPropAddr = EntityScriptProp->ContainerPtrToValuePtr<uint8>(InEntityScript);
            const auto* SpawnParamsPropAddr = SpawnParamsProp->ContainerPtrToValuePtr<uint8>(SpawnParamsData);

            // A request may retain a weak UObject reference even when the constructed script owns the object
            // strongly. Resolve the weak identity only here, while writing into the GC-traced EntityScript
            // UObject — this avoids retaining a bare actor address in the EnTT request queue.
            if (const auto* SpawnWeakObjectProp = CastField<FWeakObjectProperty>(SpawnParamsProp))
            {
                if (const auto* ScriptObjectProp = CastField<FObjectPropertyBase>(EntityScriptProp))
                {
                    ScriptObjectProp->SetObjectPropertyValue(
                        EntityScriptPropAddr,
                        SpawnWeakObjectProp->GetObjectPropertyValue(SpawnParamsPropAddr));
                    continue;
                }
            }

#if ENABLE_MT_DETECTOR
            // ---- Delegate MRSW bypass ----
            // Delegates start with an FMRSWRecursiveAccessDetector; a raw memory copy through FInstancedStruct
            // can leave it in a stale "writer active" state, causing false-positive race ensures. Initialize the
            // destination for a clean detector, then memcpy only the data fields that live after it.
            if (CastField<FDelegateProperty>(SpawnParamsProp))
            {
                const auto PropertySize = SpawnParamsProp->GetSize();
                EntityScriptProp->InitializeValue(EntityScriptPropAddr);

                constexpr auto DetectorSize = sizeof(FMRSWRecursiveAccessDetector);
                const auto DataSize = PropertySize - DetectorSize;
                if (DataSize > 0)
                {
                    FMemory::Memcpy(
                        EntityScriptPropAddr + DetectorSize,
                        SpawnParamsPropAddr + DetectorSize,
                        DataSize);
                }
                continue;
            }
#endif

            EntityScriptProp->CopyCompleteValue(EntityScriptPropAddr, SpawnParamsPropAddr);
        }
    }
}

auto
    UCk_Utils_EntityScript_UE::
    Request_ReplicateEntityScript(
        FCk_Handle& InHandle,
        const FCk_Handle& InReplicatedOwner,
        UCk_EntityScript_UE* InEntityScript,
        const FInstancedStruct& InSpawnParams,
        const TOptional<FCk_Handle>& InContextOwnerOverride)
    -> void
{
    if (NOT ck_entity_script_utils::ValidateRetainedSpawnParams(TEXT("Request_ReplicateEntityScript"), InSpawnParams))
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(InEntityScript),
        TEXT("Cannot replicate EntityScript for [{}]: the EntityScript instance is invalid"), InHandle)
    { return; }

    auto ReplicatedOwner = InReplicatedOwner;
    const auto IsSelfOwned = ReplicatedOwner == InHandle;

    if (NOT IsSelfOwned)
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_Replication(ReplicatedOwner) == ECk_Replication::Replicates,
            TEXT("Cannot replicate EntityScript for [{}]: replicated owner [{}] does NOT replicate"),
            InHandle, ReplicatedOwner)
        { return; }
    }

    CK_ENSURE_IF_NOT(InHandle.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>(),
        TEXT("Cannot replicate EntityScript for [{}]: entity has no ReplicationDriver"), InHandle)
    { return; }

    const auto& OwnerReplicationDriver = IsSelfOwned
        ? InHandle.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>()
        : ReplicatedOwner.Get<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>();

    CK_ENSURE_IF_NOT(ck::IsValid(OwnerReplicationDriver),
        TEXT("Cannot replicate EntityScript for [{}]: owner [{}] ReplicationDriver is invalid"),
        InHandle, ReplicatedOwner)
    { return; }

    if (ReplicatedOwner.Has<ck::FTag_EntityJustCreated>())
    {
        OwnerReplicationDriver->Set_ExpectedNumberOfDependentReplicationDrivers(
            OwnerReplicationDriver->Get_ExpectedNumberOfDependentReplicationDrivers() + 1);
    }

    UCk_Utils_EntityReplicationDriver_UE::Request_Replicate(
        InHandle, ReplicatedOwner, InEntityScript->GetClass(), InSpawnParams, InContextOwnerOverride);

    InHandle.Add<ck::FTag_EntityReplicationDriver_FireOnDependentReplicationComplete>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PendingEntityScript_UE::
    Promise_OnConstructed(
        FCk_Handle_PendingEntityScript& InPendingEntityScript,
        const FCk_Delegate_EntityScript_Constructed& InDelegate)
    -> void
{
    // Request_SpawnEntity returns an INVALID pending handle when the spawn was suppressed (notably during a
    // CkSnapshot load's world reconstitution) or after it already ensured on a bad class/owner. Binding would
    // then AddOrGet the signal fragment on a TOMBSTONE handle and ensure; no-op is correct — nothing spawned.
    if (ck::Is_NOT_Valid(InPendingEntityScript.Get_EntityUnderConstruction()))
    { return; }

    ck::UUtils_Signal_OnConstructed_PostFireUnbind::Bind(
        InPendingEntityScript.Get_EntityUnderConstruction(), InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

auto
    UCk_Utils_PendingEntityScript_UE::
    Get_IsValid(
        const FCk_Handle_PendingEntityScript& InHandle)
    -> bool
{
    return ck::IsValid(InHandle);
}

auto
    UCk_Utils_PendingEntityScript_UE::
    Request_DestroyEntity(
        FCk_Handle_PendingEntityScript& InHandle,
        ECk_EntityLifetime_DestructionBehavior InDestructionBehavior)
    -> void
{
    auto EntityUnderConstruction = InHandle.Get_EntityUnderConstruction();
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(EntityUnderConstruction, InDestructionBehavior);
}

// --------------------------------------------------------------------------------------------------------------------
