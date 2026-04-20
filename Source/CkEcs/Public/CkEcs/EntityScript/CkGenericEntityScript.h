#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkGenericEntityScript.generated.h"

// -----------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_EntityScript_SpawnEntity_HandleRequests;
}

// -----------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKECS_API UCk_GenericEntityScript_UE : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GenericEntityScript_UE);

public:
    friend class ck::FProcessor_EntityScript_SpawnEntity_HandleRequests;

protected:
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;

    auto
    ContinueConstruction(
        FCk_Handle InHandle) -> void override;

    auto
    BeginPlay() -> void override;

    auto
    EndPlay() -> void override;

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript",
        DisplayName = "ConstructionScript")
    ECk_EntityScript_ConstructionFlow
    DoConstruct(
        UPARAM(ref) FCk_Handle& InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript",
        DisplayName = "ContinueConstruction")
    void
    DoContinueConstruction(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript",
        DisplayName = "BeginPlay")
    void
    DoBeginPlay(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript",
        DisplayName = "EndPlay")
    void
    DoEndPlay(
        FCk_Handle InHandle);
};

// -----------------------------------------------------------------------------------------------------------
