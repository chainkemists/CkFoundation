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

UENUM(BlueprintType)
enum class ECk_EntityScript_ConstructionFlow : uint8
{
    // Construction is finished. BeginPlay can be called
    Finished UMETA(DisplayName = "Finish Construction"),

    // Construction is still ongoing. ContinueConstruction event will be called and the user will need
    // to manually invoke FinishConstruction in order for BeginPlay to be called
    Continue UMETA(DisplayName = "Continue Construction")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EntityScript_ConstructionFlow);

// -----------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKECS_API UCk_EntityScript_UE : public UCk_GameWorldContextObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityScript_UE);

public:
    friend class ck::FProcessor_EntityScript_SpawnEntity_HandleRequests;
    friend class UCk_Utils_EntityScript_UE;

public:
    [[nodiscard]]
    auto
    Construct(
        FCk_Handle& InHandle) -> ECk_EntityScript_ConstructionFlow;

    auto
    ContinueConstruction(
        FCk_Handle InHandle) -> void;

    auto
    BeginPlay() -> void;

    auto
    EndPlay() -> void;

protected:
    UFUNCTION(BlueprintInternalUseOnly, BlueprintImplementableEvent,
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

private:
    UFUNCTION(BlueprintPure,
        Category = "Ck|EntityScript",
        DisplayName = "[Ck][EntityScript] Get Script Entity",
        meta = (CompactNodeTitle="ScriptEntity", Keywords="this, associated", HideSelfPin = true))
    FCk_Handle
    DoGet_ScriptEntity() const;

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][EntityScript]Finish Construction",
              meta = (CompactNodeTitle="✅FinishConstruction", HideSelfPin = true, Keywords = "ongoing"))
    void
    DoFinishConstruction();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Ability|Script",
              DisplayName = "[Ck][EntityScript] Add Task To Deactivate On EndPlay",
              meta = (CompactNodeTitle="🛑", HideSelfPin = true, Keywords = "register, track, stop"))
    void
    DoRequest_AddTaskToDeactivateOnEndPlay(
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