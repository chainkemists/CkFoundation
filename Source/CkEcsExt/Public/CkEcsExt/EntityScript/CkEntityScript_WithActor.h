#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"
#include "CkEcs/EntityScript/CkGenericEntityScript.h"

#include "CkEntityScript_WithActor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType, meta=(CkSpawnParams="FCk_EntityScript_WithActor_SpawnParams"))
class CKECSEXT_API UCk_EntityScript_WithActor_UE : public UCk_GenericEntityScript_UE
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
    Get_EffectiveReplication() const -> ECk_Replication override;

    auto
    EndPlay() -> void override;

protected:
    UFUNCTION(BlueprintPure,
        Category = "Ck|EntityScript|WithActor",
        DisplayName = "[Ck][EntityScript] Get Owning Actor",
        meta = (CompactNodeTitle="OwningActor", HideSelfPin = true))
    AActor*
    DoGet_OwningActor() const;

protected:
    auto
    ShowReplicationInEditor() const -> bool override;

    // Snapshot respawn opt-in. Only gameplay bridged actors that return true are stamped with
    // FFragment_ActorSpawnIntent and re-spawned by the snapshot on load. Framework infrastructure actors
    // (ActorRelay / CueRelay / StateMachineRelay, etc.) return false (the default) so they are NOT entity-
    // respawned — the framework re-acquires them on demand; respawning them during a load crashes. A gameplay
    // actor opts in by overriding this in its WithActor entity-script subclass (or, later, via data).
    virtual auto
    Get_IsSnapshotRespawnable() const -> bool;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (ExposeOnSpawn = true, AllowPrivateAccess = true))
    TObjectPtr<AActor> _OwningActor;

public:
    CK_PROPERTY_GET(_OwningActor);
};

// --------------------------------------------------------------------------------------------------------------------
