#include "CkEntityScript_Utils.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include "Misc/MTAccessDetector.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
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
        if (DefaultObject->Get_Replication() == ECk_Replication::Replicates &&
            UCk_Utils_Net_UE::Get_EntityNetMode(InLifetimeOwner) == ECk_Net_NetModeType::Client)
        { return {}; }
    }
    else
    {
        CK_ENSURE(ck::IsValid(DefaultObject),
            TEXT("The EntityScriptClass [{}] does NOT have a ClassDefaultObject (or could not load it). This "
                "may result in Replicated EntityScripts to have an additional copy on the clients!"), InEntityScriptClass);
    }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);

    // Request_CreateEntity does NOT copy NetParams if the Lifetime Owner is a transient Entity
    // (which should probably be revisited). For now, we manually copy the NetParams
    if (UCk_Utils_EntityLifetime_UE::Get_IsTransientEntity(InLifetimeOwner))
    { UCk_Utils_Net_UE::Copy(InLifetimeOwner, NewEntity); }

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

    if (InEntityScriptClassArchetype->Get_Replication() == ECk_Replication::Replicates &&
        UCk_Utils_Net_UE::Get_EntityNetMode(InLifetimeOwner) == ECk_Net_NetModeType::Client)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);

    // Request_CreateEntity does NOT copy NetParams if the Lifetime Owner is a transient Entity
    // (which should probably be revisited). For now, we manually copy the NetParams
    if (UCk_Utils_EntityLifetime_UE::Get_IsTransientEntity(InLifetimeOwner))
    { UCk_Utils_Net_UE::Copy(InLifetimeOwner, NewEntity); }

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
