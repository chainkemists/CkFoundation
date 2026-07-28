#pragma once

#include "CkEntityScript_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEntityScript_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;
class UWorld;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_EntityScript"))
class CKECS_API UCk_Utils_EntityScript_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityScript_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_EntityScript);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityScript",
              DisplayName="[Ck][EntityScript] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityScript",
        DisplayName="[Ck][EntityScript] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_EntityScript
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|EntityScript",
        DisplayName="[Ck][EntityScript] Handle -> EntityScript Handle",
        meta = (CompactNodeTitle = "<AsEntityScript>", BlueprintAutocast))
    static FCk_Handle_EntityScript
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|EntityScript",
        DisplayName = "[Ck][EntityScript] Get Invalid Handle",
        meta = (CompactNodeTitle = "INVALID_EntityScriptHandle", Keywords = "make"))
    static FCk_Handle_EntityScript
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityScript",
              DisplayName="[Ck][EntityScript] Get_ScriptClass")
    static TSubclassOf<UCk_EntityScript_UE>
    Get_ScriptClass(
        const FCk_Handle_EntityScript& InHandle);

public:
    // Re-establish every entity script's _AssociatedEntity back-pointer after a CkSnapshot restore: the field is
    // Transient and set only at spawn, so a restored script would read a tombstone handle at teardown and ensure.
    // Not Blueprint-exposed; internal restore plumbing. Returns the number of scripts re-linked.
    static int32
    Relink_AssociatedEntities_AfterRestore(
        UWorld* InWorld);

public:
    // Hidden in the editor through the DefaultCkFoundation.ini Config file (see: BlueprintEditor.Menu section)
    // The completion delegate reports that the spawn REQUEST was drained, not that construction finished —
    // a script returning ECk_EntityScript_ConstructionFlow::Continue still completes with Succeeded.
    // Promise_OnConstructed on the returned pending handle remains the construction-finished channel.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityScript|Private",
        DisplayName="[CK][EntityScript] Request SpawnEntity",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_PendingEntityScript
    Request_SpawnEntity(
        UPARAM(ref) FCk_Handle& InLifetimeOwner,
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass,
        FInstancedStruct InSpawnParams,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Hidden in the editor through the DefaultCkFoundation.ini Config file (see: BlueprintEditor.Menu section)
    // Completion semantics: see Request_SpawnEntity.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityScript|Private",
        DisplayName="[CK][EntityScript] Request SpawnEntity (Archetype)",
        meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_PendingEntityScript
    Request_SpawnEntity_Archetype(
        UPARAM(ref) FCk_Handle& InLifetimeOwner,
        UCk_EntityScript_UE* InEntityScriptClassArchetype,
        FInstancedStruct InSpawnParams,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|EntityScript",
              DisplayName="[Ck][EntityScript] Try Get Entity With EntityScript In Ownership Chain")
    static FCk_Handle
    TryGet_Entity_EntityScript_InOwnershipChain(
        const FCk_Handle& InHandle);

public:
    static auto
    Add(
        FCk_Handle& InScriptEntity,
        const TSubclassOf<UCk_EntityScript_UE>& InEntityScriptClass,
        const FInstancedStruct& InSpawnParams,
        const FCk_EntityScript_PostConstruction_Func& InOptionalFunc = nullptr,
        const FCk_Delegate_Request_OnCompleted& InDelegate = {}) -> FCk_Handle_PendingEntityScript;

    static auto
    Add(
        FCk_Handle& InScriptEntity,
        const TWeakObjectPtr<UCk_EntityScript_UE>& InEntityScriptClassArchetype,
        const FInstancedStruct& InSpawnParams,
        const FCk_EntityScript_PostConstruction_Func& InOptionalFunc = nullptr,
        const FCk_Delegate_Request_OnCompleted& InDelegate = {}) -> FCk_Handle_PendingEntityScript;

    static auto
    TryInjectEntityScriptSpawnParams(
        UCk_EntityScript_UE* InEntityScript,
        const FInstancedStruct& InSpawnParams) -> void;

    // Establishes entity-script replication for an already-constructed, driver-bearing entity on the authority:
    // resolves the owning driver, accounts for a just-created owner's dependent count, calls Request_Replicate and
    // marks it to fire OnDependentsReplicationComplete. Caller must ensure InHandle already has a driver fragment.
    static auto
    Request_ReplicateEntityScript(
        FCk_Handle& InHandle,
        const FCk_Handle& InReplicatedOwner,
        UCk_EntityScript_UE* InEntityScript,
        const FInstancedStruct& InSpawnParams,
        const TOptional<FCk_Handle>& InContextOwnerOverride = {}) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_PendingEntityScript"))
class CKECS_API UCk_Utils_PendingEntityScript_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_PendingEntityScript_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|EntityScript",
              DisplayName = "[Ck][EntityScript] Promise/Future OnConstructed",
              meta = (Keywords = "bind, wait, when, finish"))
    static void
    Promise_OnConstructed(
        UPARAM(ref) FCk_Handle_PendingEntityScript& InPendingEntityScript,
        const FCk_Delegate_EntityScript_Constructed& InDelegate);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck][EntityScript] Get Is Valid (Pending Entity Script)",
              Category = "Ck|Utils|EntityScript",
              meta = (CompactNodeTitle = "IsValid"))
    static bool
    Get_IsValid(
        const FCk_Handle_PendingEntityScript& InHandle);

    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck][EntityScript] Request Destroy Pending Entity Script",
              Category = "Ck|Utils|EntityScript")
    static void
    Request_DestroyEntity(
        UPARAM(ref) FCk_Handle_PendingEntityScript& InHandle,
        ECk_EntityLifetime_DestructionBehavior InDestructionBehavior = ECk_EntityLifetime_DestructionBehavior::ForceDestroy);
};

// --------------------------------------------------------------------------------------------------------------------