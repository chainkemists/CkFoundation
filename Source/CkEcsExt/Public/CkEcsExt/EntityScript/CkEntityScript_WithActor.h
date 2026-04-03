#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkEntityScript_WithActor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
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
    UPROPERTY(Transient)
    TObjectPtr<AActor> _OwningActor;

public:
    CK_PROPERTY_GET(_OwningActor);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType)
class CKECSEXT_API UCk_EntityScript_WithActor_Default_UE : public UCk_EntityScript_WithActor_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityScript_WithActor_Default_UE);
};

// --------------------------------------------------------------------------------------------------------------------
