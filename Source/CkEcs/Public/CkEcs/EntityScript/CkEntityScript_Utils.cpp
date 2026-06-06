#include "CkEntityScript_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include "Misc/MTAccessDetector.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Net/CkNet_Fragment.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h" // M2b-1 reconstitution-in-progress gate

#include <UObject/ObjectPtr.h>

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_EntityScript_UE, FCk_Handle_EntityScript, ck::FFragment_EntityScript_Current);

// --------------------------------------------------------------------------------------------------------------------

// M2b-1: while a CkSnapshot load is reconstituting the entity's world, the snapshot is the sole creator — a
// respawned bridged actor's BeginPlay would call Request_SpawnEntity to make a DUPLICATE of the entity the snapshot
// re-bridges. Suppress the spawn entirely (no entity created → no features added → no half-built entity reaches the
// feature Setup processors). The respawn pass owns the real binding. The flag lives on the CkEcs EcsWorld subsystem
// (reachable here, unlike the CkSnapshot subsystem which is a higher tier).
static auto
    DoIs_WorldReconstituting(
        const FCk_Handle& InLifetimeOwner) -> bool
{
    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InLifetimeOwner);
    if (ck::Is_NOT_Valid(World))
    { return false; }

    auto* EcsWorld = World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    return ck::IsValid(EcsWorld) && EcsWorld->Get_IsReconstitutionInProgress();
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
        FInstancedStruct InSpawnParams)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntityScriptClass),
        TEXT("EntityScriptClass [{}] is INVALID. Unable to SpawnEntity using LifetimeOwner [{}]."),
        InEntityScriptClass, InLifetimeOwner)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InLifetimeOwner),
        TEXT("LifetimeOwner is INVALID. Unable to SpawnEntity using EntityScriptClass [{}]."),
        InEntityScriptClass)
    { return {}; }

    if (DoIs_WorldReconstituting(InLifetimeOwner))
    { return {}; }

    if (const auto DefaultObject = InEntityScriptClass->GetDefaultObject<UCk_EntityScript_UE>();
        ck::IsValid(DefaultObject))
    {
        if (DefaultObject->Get_EffectiveReplication() == ECk_Replication::Replicates &&
            UCk_Utils_Net_UE::Get_EntityNetMode(InLifetimeOwner) == ECk_Net_NetModeType::Client)
        {
            auto PendingEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);
            auto& PendingFragment = InLifetimeOwner.AddOrGet<ck::FFragment_PendingReplication>();
            PendingFragment.Add(InEntityScriptClass.Get(), PendingEntity, InSpawnParams);
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

    auto CDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_EntityScript_UE>(InEntityScriptClass);
    return Add(NewEntity, MakeWeakObjectPtr(CDO), InSpawnParams);
}

auto
    UCk_Utils_EntityScript_UE::
    Request_SpawnEntity_Archetype(
        FCk_Handle& InLifetimeOwner,
        UCk_EntityScript_UE* InEntityScriptClassArchetype,
        FInstancedStruct InSpawnParams)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntityScriptClassArchetype),
        TEXT("EntityScriptClass [{}] is INVALID. Unable to SpawnEntity using LifetimeOwner [{}]."),
        InEntityScriptClassArchetype, InLifetimeOwner)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InLifetimeOwner),
        TEXT("LifetimeOwner is INVALID. Unable to SpawnEntity using EntityScriptClass [{}]."),
        InEntityScriptClassArchetype)
    { return {}; }

    if (DoIs_WorldReconstituting(InLifetimeOwner))
    { return {}; }

    if (InEntityScriptClassArchetype->Get_EffectiveReplication() == ECk_Replication::Replicates &&
        UCk_Utils_Net_UE::Get_EntityNetMode(InLifetimeOwner) == ECk_Net_NetModeType::Client)
    {
        auto PendingEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);
        auto& PendingFragment = InLifetimeOwner.AddOrGet<ck::FFragment_PendingReplication>();
        PendingFragment.Add(InEntityScriptClassArchetype->GetClass(), PendingEntity, InSpawnParams);
        return FCk_Handle_PendingEntityScript{PendingEntity};
    }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);

    CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, NewEntity,
        TEXT("Request_SpawnEntity called with class: {}"), InEntityScriptClassArchetype);

    return Add(NewEntity, InEntityScriptClassArchetype, InSpawnParams);
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
        const FCk_EntityScript_PostConstruction_Func& InOptionalFunc)
    -> FCk_Handle_PendingEntityScript
{
    return Add(InScriptEntity, InEntityScriptClass.GetDefaultObject(), InSpawnParams, InOptionalFunc);
}

auto
    UCk_Utils_EntityScript_UE::
    Add(
        FCk_Handle& InScriptEntity,
        const TWeakObjectPtr<UCk_EntityScript_UE>& InEntityScriptClassArchetype,
        const FInstancedStruct& InSpawnParams,
        const FCk_EntityScript_PostConstruction_Func& InOptionalFunc)
    -> FCk_Handle_PendingEntityScript
{
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

    // Derive the new entity's Net_Params from the archetype's effective replication intent
    // combined with the TransientEntity's NetMode / NetRole context. Get_EffectiveReplication
    // is the virtual hook subclasses override to reconcile the CDO default with runtime state
    // (e.g. WithActor returns DoesNotReplicate when its OwningActor isn't replicated). Handles
    // the transient-owner case where Request_CreateEntity skips Net_Params inheritance, and
    // also covers direct Add() callers (SM condition/state/transition attach, any non-spawn
    // flow that attaches a script to an existing transient-owned entity). When the entity
    // already inherited Net_Params from a non-transient lifetime owner, respect that.
    if (NOT InScriptEntity.Has<ck::FFragment_Net_Params>())
    {
        const auto TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InScriptEntity);
        const auto& Settings = TransientEntity.Get<ck::FFragment_Net_Params>().Get_ConnectionSettings();

        auto Replication = InEntityScriptClassArchetype->Get_EffectiveReplication();
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

    const auto Request = FCk_Request_EntityScript_SpawnEntity{
                             InScriptEntity,
                             UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InScriptEntity),
                             InEntityScriptClassArchetype}
                        .Set_SpawnParams(InSpawnParams)
                        .Set_PostConstruction_Func(InOptionalFunc);

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

            if (ck::Is_NOT_Valid(EntityScriptProp, ck::IsValid_Policy_NullptrOnly{}))
            {
                // BP must have all properties in struct, c++ classes may not
                CK_ENSURE_IF_NOT(NOT InEntityScript->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint),
                    TEXT("Failed to find ExposedOnSpawn Property [{}] on BP Entity Script [{}]. Cannot inject this SpawnParam"),
                    PropertyName,
                    InEntityScript)
                {}
                continue;
            }

            auto* EntityScriptPropAddr = EntityScriptProp->ContainerPtrToValuePtr<uint8>(InEntityScript);
            const auto* SpawnParamsPropAddr = SpawnParamsProp->ContainerPtrToValuePtr<uint8>(SpawnParamsData);

#if ENABLE_MT_DETECTOR
            // ---- Delegate MRSW bypass ----
            // Delegates contain an FMRSWRecursiveAccessDetector at the start of their base class
            // (TDelegateAccessHandlerBase). When delegate data passes through FInstancedStruct,
            // raw memory copies can leave the detector in a stale "writer active" state, causing
            // false-positive race-detection ensures on every subsequent access.
            // Fix: initialize the destination (clean detector state), then memcpy only the data
            // fields (Object, FunctionName) that live after the detector.
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

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PendingEntityScript_UE::
    Promise_OnConstructed(
        FCk_Handle_PendingEntityScript& InPendingEntityScript,
        const FCk_Delegate_EntityScript_Constructed& InDelegate)
    -> void
{
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
