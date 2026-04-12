#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkSmTask_EntityScript.generated.h"

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKSTATEMACHINE_API UCk_SmTask_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmTask_EntityScript);

    UCk_SmTask_EntityScript()
    {
        _Replication = ECk_Replication::DoesNotReplicate;
    }

    // ================================================================================================================
    // LIFECYCLE (EntityScript overrides)
    // ================================================================================================================

protected:
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;

    auto
    BeginPlay() -> void override;

    auto
    EndPlay() -> void override;

    // ================================================================================================================
    // VIRTUAL METHODS (user overrides)
    // ================================================================================================================

public:
    virtual auto
    Tick(
        float InDeltaSeconds) -> ECk_SmTaskResult;

    // ================================================================================================================
    // BLUEPRINT IMPLEMENTABLE EVENTS
    // ================================================================================================================

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|Task",
        DisplayName = "Tick")
    ECk_SmTaskResult
    DoTick(
        FCk_Handle InHandle,
        float InDeltaSeconds);

    // ================================================================================================================
    // HELPERS
    // ================================================================================================================

public:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|SM|Task",
        DisplayName = "[Ck][SM] Mark Task Result")
    void
    Mark_Result(
        ECk_SmTaskResult InResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|Task",
        DisplayName = "[Ck][SM] Get Owning StateMachine",
        meta = (CompactNodeTitle = "OwningSM", HideSelfPin = true))
    FCk_Handle_StateMachine
    Get_OwningStateMachine() const;

protected:
    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|Task",
        DisplayName = "[Ck][SM] Get Game Entity",
        meta = (CompactNodeTitle = "GameEntity", HideSelfPin = true))
    FCk_Handle
    DoGet_GameEntity() const;

    // ================================================================================================================
    // TAG
    // ================================================================================================================

public:
    auto
    Get_TaskTag() const -> FGameplayTag;

    static auto
    Get_TaskTagForClass(
        TSubclassOf<UCk_SmTask_EntityScript> InClass) -> FGameplayTag;

    // ================================================================================================================
    // MEMBERS
    // ================================================================================================================

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Task",
        meta = (AllowPrivateAccess = true))
    ECk_SmTaskMode _TaskMode = ECk_SmTaskMode::EnterExitOnly;

public:
    CK_PROPERTY_GET(_TaskMode);
};

// --------------------------------------------------------------------------------------------------------------------
