#pragma once

#include "CkCore/Object/CkWorldContextObject.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CoreMinimal.h"

#include "CkEntityScript.generated.h"

// -----------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_EntityScript_SpawnEntity_HandleRequests;
}

// -----------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType)
class CKECS_API UCk_EntityScript_UE : public UCk_GameWorldContextObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityScript_UE);

public:
    friend class ck::FProcessor_EntityScript_SpawnEntity_HandleRequests;
    friend class UCk_Utils_EntityScript_UE;

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
    DoBeginPlay(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|EntityScript",
        DisplayName = "EndPlay")
    void
    DoEndPlay(
        FCk_Handle InHandle);

private:
    UFUNCTION(BlueprintPure,
        Category = "Ck|EntityScript",
        DisplayName = "[Ck][EntityScript] Get Script Entity",
        meta = (CompactNodeTitle="ScriptEntity", Keywords="this, associated", HideSelfPin = true))
    FCk_Handle
    DoGet_ScriptEntity() const;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][EntityScript] Add Task To Deactivate On EndPlay",
              meta = (CompactNodeTitle="🛑", HideSelfPin = true, Keywords = "Register, Track"))
    void
    DoRequest_AddTaskToDeactivateOnDeactivate(
        class UBlueprintTaskTemplate* InTask);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Ck|EntityScript",
        meta=(AllowPrivateAccess))
    ECk_Replication _Replication = ECk_Replication::Replicates;

    UPROPERTY(Transient)
    FCk_Handle _AssociatedEntity;

private:
    TArray<TWeakObjectPtr<class UBlueprintTaskTemplate>> _TasksToDeactivate;

public:
    CK_PROPERTY_GET(_Replication);
};

// ----------------------------------------------------------------------------------------------------------