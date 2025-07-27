#pragma once

#include "CkEntityScript_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEntityScript_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;

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
    // Hidden in the editor through the Config file (see: BlueprintEditor.Menu section)
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EntityScript|Private",
        DisplayName="[CK][EntityScript] Request SpawnEntity")
    static FCk_Handle_PendingEntityScript
    Request_SpawnEntity(
        UPARAM(ref) FCk_Handle& InLifetimeOwner,
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass,
        FInstancedStruct InSpawnParams);

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
        FCk_EntityScript_PostConstruction_Func InOptionalFunc = nullptr) -> FCk_Handle_PendingEntityScript;
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
};

// --------------------------------------------------------------------------------------------------------------------