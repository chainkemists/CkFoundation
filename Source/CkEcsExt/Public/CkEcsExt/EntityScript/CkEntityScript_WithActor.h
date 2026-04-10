#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkEntityScript_WithActor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType, meta=(CkSpawnParams="FCk_EntityScript_WithActor_SpawnParams"))
class CKECSEXT_API UCk_EntityScript_WithActor_UE : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityScript_WithActor_UE);

public:
    [[nodiscard]]
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;

    [[nodiscard]]
    auto
    Get_AreSpawnParamsMatching(
        const FInstancedStruct& InClientSpawnParams,
        const UCk_EntityScript_UE* InConstructedScript) const -> bool override;

    [[nodiscard]]
    auto
    Get_EffectiveReplication(
        const FInstancedStruct& InSpawnParams) const -> ECk_Replication override;

    auto
    EndPlay() -> void override;

protected:
    [[nodiscard]]
    virtual auto
    ConstructWithActor(
        FCk_Handle& InHandle,
        AActor* InOwningActor) -> ECk_EntityScript_ConstructionFlow;

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript|WithActor",
        DisplayName = "ConstructionScript (WithActor)")
    ECk_EntityScript_ConstructionFlow
    DoConstructWithActor(
        UPARAM(ref) FCk_Handle& InHandle,
        AActor* InOwningActor);

    UFUNCTION(BlueprintPure,
        Category = "Ck|EntityScript|WithActor",
        DisplayName = "[Ck][EntityScript] Get Owning Actor",
        meta = (CompactNodeTitle="OwningActor", HideSelfPin = true))
    AActor*
    DoGet_OwningActor() const;

protected:
    auto
    ShowReplicationInEditor() const -> bool override;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (ExposeOnSpawn = true, AllowPrivateAccess = true))
    TObjectPtr<AActor> _OwningActor;

public:
    CK_PROPERTY_GET(_OwningActor);
};

// --------------------------------------------------------------------------------------------------------------------
