#pragma once

#include "CkEntityScript_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEntityScript_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;

namespace ck
{
    class FProcessor_EntityScript_SpawnEntity_HandleRequests;
    class FProcessor_EntityScript_BeginPlay;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_EntityScript_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EntityScript_UE);

public:
    friend class ck::FProcessor_EntityScript_SpawnEntity_HandleRequests;
    friend class ck::FProcessor_EntityScript_BeginPlay;

public:
    // returns the handle to the newly spawned Entity (not yet constructed)
    UFUNCTION(BlueprintCallable, BlueprintInternalUseOnly)
    static FCk_Handle
    DoRequest_SpawnEntity(
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
        FCk_EntityScript_PostConstruction_Func InOptionalFunc = nullptr) -> FCk_Handle;

    static auto
    Get_EntityScript(
        const FCk_Handle& InHandle) -> TWeakObjectPtr<UCk_EntityScript_UE>;
};
// --------------------------------------------------------------------------------------------------------------------
