#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkEntityScript.generated.h"

// -----------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_EntityScript_SpawnEntity_HandleRequests;
}

// -----------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKECS_API UCk_EntityScript_UE : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityScript_UE);

public:
    friend class ck::FProcessor_EntityScript_SpawnEntity_HandleRequests;

public:
    auto
    Construct(FCk_Handle& InHandle) const -> void;

    auto
    BeginPlay() -> void;

    auto
    EndPlay() -> void;

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript",
        DisplayName = "ConstructionScript")
    void
    DoConstruct(
        UPARAM(ref) FCk_Handle& InHandle) const;

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript",
        DisplayName = "BeginPlay")
    void
    DoBeginPlay();

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript",
        DisplayName = "EndPlay")
    void
    DoEndPlay();

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Ck|EntityScript",
        meta=(AllowPrivateAccess))
    ECk_Replication _Replication = ECk_Replication::Replicates;

    UPROPERTY(BlueprintReadOnly,
        Category = "Ck|EntityScript",
        DisplayName = "Associated Entity",
        meta=(AllowPrivateAccess))
    FCk_Handle _AssociatedEntity;

public:
    CK_PROPERTY_GET(_Replication);
};

// -----------------------------------------------------------------------------------------------------------
