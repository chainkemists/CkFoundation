#include "CkEntityScript_Utils.h"

#include "CkEntityScript.h"
#include "CkGenericEntityScript.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Reflection/CkReflection_Utils.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "Misc/MTAccessDetector.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/DependencyInjection/CkDependencyProvider_GameInstance_Subsystem.h"
#include "CkEcs/DependencyInjection/CkDependencyProvider_InjectionCache.h"
#include "CkEcs/DependencyInjection/CkDependencyProvider_World_Subsystem.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Net/CkNet_Fragment.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include <UObject/ObjectPtr.h>

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_EntityScript_UE, FCk_Handle_EntityScript, ck::FFragment_EntityScript_Current);

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
//                         Dependency-Injection pass + finish-construction
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    // Builds a pending-resolution callback that writes the resolved handle
    // into the script's property and (when ALL of its injection sites are
    // now resolved) enqueues the FinishDeferredConstruct request.
    //
    // Captured by value where lifetime-safe; the script lives in a
    // StrongObjectPtr on the AwaitingDependencies fragment, so a weak
    // pointer here is fine. The entity reference is captured by value
    // (FCk_Handle is cheap).
    auto
    MakePendingCallback(
        UCk_EntityScript_UE* InScript,
        FCk_Handle InEntity,
        FStructProperty* InProperty,
        const TArray<FCk_InjectionSite>& InPlan)
        -> TFunction<void(const FCk_Handle&)>
    {
        const TWeakObjectPtr<UCk_EntityScript_UE> WeakScript = InScript;
        return [WeakScript, Entity = InEntity, Property = InProperty, Plan = InPlan]
               (const FCk_Handle& InResolved) mutable -> void
        {
            auto* Script = WeakScript.Get();
            if (Script == nullptr)
            { return; }

            if (ck::Is_NOT_Valid(Entity))
            { return; }

            // Write the resolved value into the script's property.
            if (Property != nullptr)
            {
                auto* HandlePtr = Property->ContainerPtrToValuePtr<FCk_Handle>(Script);
                *HandlePtr = InResolved;
            }

            // Re-check ALL sites on this script. If any are still invalid
            // (other pending dependencies haven't landed yet), we leave the
            // entity in the awaiting state.
            for (const auto& Site : Plan)
            {
                if (Site._PropertyOnScript == nullptr)
                { continue; }

                const auto* Ptr = Site._PropertyOnScript->ContainerPtrToValuePtr<FCk_Handle>(Script);
                if (ck::Is_NOT_Valid(*Ptr))
                { return; }
            }

            // All resolved. Transition out of the awaiting state and enqueue
            // the finish-construct request.
            if (NOT Entity.Has<ck::FFragment_EntityScript_AwaitingDependencies>())
            { return; }

            const auto& Awaiting = Entity.Get<ck::FFragment_EntityScript_AwaitingDependencies>();
            const auto LifetimeOwner = Awaiting.Get_LifetimeOwner();
            const auto SpawnParams   = Awaiting.Get_OriginalSpawnParams();
            const auto PostFunc      = Awaiting.Get_PostConstruction_Func();

            Entity.Remove<ck::FTag_EntityScript_AwaitingDependencies>();
            Entity.Remove<ck::FFragment_EntityScript_AwaitingDependencies>();
            Entity.Add<ck::FRequest_EntityScript_FinishDeferredConstruct>(
                Script, LifetimeOwner, SpawnParams, PostFunc);
        };
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityScript_UE::
    TryInjectEntityScriptDependencies(
        UCk_EntityScript_UE* InEntityScript,
        const FCk_Handle& InEntity)
    -> bool
{
    if (ck::Is_NOT_Valid(InEntityScript) || ck::Is_NOT_Valid(InEntity))
    { return true; }

    const auto& Plan = FCk_InjectionCache::GetOrBuild(InEntityScript->GetClass());
    if (Plan.IsEmpty())
    { return true; }

    auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InEntity);
    CK_ENSURE_IF_NOT(ck::IsValid(World),
        TEXT("No World for entity [{}] during dependency-injection pass on script [{}]"),
        InEntity, InEntityScript)
    { return false; }

    auto* WorldSubsystem        = World->GetSubsystem<UCk_DependencyProvider_World_Subsystem_UE>();
    auto* GameInstance          = World->GetGameInstance();
    auto* GameInstanceSubsystem = ck::IsValid(GameInstance)
        ? GameInstance->GetSubsystem<UCk_DependencyProvider_GameInstance_Subsystem_UE>()
        : nullptr;

    auto AllResolved = true;

    for (const auto& Site : Plan)
    {
        auto* HandlePtr = Site._PropertyOnScript->ContainerPtrToValuePtr<FCk_Handle>(InEntityScript);

        // Caller-supplied value wins — TryInjectEntityScriptSpawnParams already
        // ran upstream, so a non-empty handle here came from the caller's
        // SpawnParams. Match the testability story from the plan.
        if (ck::IsValid(*HandlePtr))
        { continue; }

        auto Resolved = FCk_Handle{};
        switch (Site._Scope)
        {
            case ECk_DependencyProvider_Scope::GameInstance:
            {
                if (GameInstanceSubsystem != nullptr)
                { Resolved = GameInstanceSubsystem->Resolve(Site._HandleType); }
                break;
            }
            case ECk_DependencyProvider_Scope::World:
            default:
            {
                if (WorldSubsystem != nullptr)
                { Resolved = WorldSubsystem->Resolve(Site._HandleType); }
                break;
            }
        }

        if (ck::IsValid(Resolved))
        {
            *HandlePtr = Resolved;
            continue;
        }

        // Missed. Register a pending callback against the appropriate
        // subsystem and mark this resolve as still in-flight.
        AllResolved = false;

        auto Pending = UCk_DependencyProvider_World_Subsystem_UE::FPendingResolution{
            ._Script     = InEntityScript,
            ._Entity     = InEntity,
            ._OnResolved = MakePendingCallback(InEntityScript, InEntity, Site._PropertyOnScript, Plan)
        };

        switch (Site._Scope)
        {
            case ECk_DependencyProvider_Scope::GameInstance:
            {
                if (GameInstanceSubsystem != nullptr)
                {
                    GameInstanceSubsystem->RegisterPending(Site._HandleType,
                        UCk_DependencyProvider_GameInstance_Subsystem_UE::FPendingResolution{
                            ._Script     = Pending._Script,
                            ._Entity     = Pending._Entity,
                            ._OnResolved = Pending._OnResolved
                        });
                }
                break;
            }
            case ECk_DependencyProvider_Scope::World:
            default:
            {
                if (WorldSubsystem != nullptr)
                { WorldSubsystem->RegisterPending(Site._HandleType, MoveTemp(Pending)); }
                break;
            }
        }
    }

    return AllResolved;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityScript_UE::
    DoFinishConstructionFlow(
        UCk_EntityScript_UE* InEntityScript,
        FCk_Handle& InEntity,
        const FCk_Handle& InLifetimeOwner,
        const FInstancedStruct& InSpawnParams,
        const FCk_EntityScript_PostConstruction_Func& InPostConstruction)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntityScript) && ck::IsValid(InEntity),
        TEXT("DoFinishConstructionFlow called with invalid Script [{}] or Entity [{}]"),
        InEntityScript, InEntity)
    { return; }

    // Net_Params is established upstream. Catch any caller that bypasses Add.
    CK_ENSURE_IF_NOT(InEntity.Has<ck::FFragment_Net_Params>(),
        TEXT("Entity [{}] is missing NetParams in DoFinishConstructionFlow. Script: [{}]."),
        InEntity, InEntityScript) {}

    // ---- Replication Driver (before Construct) ------------------------------------------------
    UCk_Utils_EntityReplicationDriver_UE::TryAdd(InEntity);

    // ---- Construct ----------------------------------------------------------------------------
    switch (InEntityScript->Construct(InEntity, InSpawnParams))
    {
        case ECk_EntityScript_ConstructionFlow::Finished:
        {
            // GET_FUNCTION_NAME_CHECKED would trip protected-access here
            // (DoContinueConstruction is protected on UCk_GenericEntityScript_UE
            // and UCk_Utils_EntityScript_UE is not a friend). The literal
            // FName is stable — the only consumer is IsFunctionImplementedInScript.
            static const auto ContinueConstructionFuncName = FName{TEXT("DoContinueConstruction")};
            CK_ENSURE_IF_NOT(NOT InEntityScript->GetClass()->IsFunctionImplementedInScript(ContinueConstructionFuncName),
                TEXT("EntityScript [{}] Construction is FINISHED, but the script implements ContinueConstruction."),
                InEntity) {}

            CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, InEntity,
                TEXT("Construct() returned Finished (deferred path)"));
            InEntity.Add<ck::FTag_EntityScript_FinishConstruction>();
            break;
        }
        case ECk_EntityScript_ConstructionFlow::Continue:
        {
            CK_CALLSTACK_RECORD_MSG(ck::FFragment_EntityScript_Current, InEntity,
                TEXT("Construct() returned Continue (deferred path)"));
            InEntity.Add<ck::FTag_EntityScript_ContinueConstruction>();
            break;
        }
    }

    // ---- Replication Infrastructure (after Construct) -----------------------------------------
    if (InEntityScript->Get_EffectiveReplication() == ECk_Replication::Replicates)
    {
        const auto HasOwningActor = UCk_Utils_OwningActor_UE::Has(InEntity);

        if (HasOwningActor)
        {
            const auto* OwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InEntity);
            const auto* World = OwningActor->GetWorld();

            const auto IsNetworkedAuthority =
                World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer);

            if (IsNetworkedAuthority)
            {
                InEntity.Add<ck::FRequest_EntityScript_Replicate>(
                    InEntity, InSpawnParams, InEntityScript);
            }
        }
        else
        {
            auto ReplicatedOwner = InLifetimeOwner;
            const auto IsHost = UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(ReplicatedOwner);

            if (IsHost)
            {
                InEntity.Add<ck::FRequest_EntityScript_Replicate>(
                    ReplicatedOwner, InSpawnParams, InEntityScript);
            }
        }
    }

    if (InPostConstruction)
    { InPostConstruction(InEntity); }
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
