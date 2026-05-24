#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkCore/Time/CkTime.h"
#include "CkStateMachine/Net/CkStateMachine_NetContext.h"
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
        // NOTE: until SM Setup propagates params._Replication to child Task entities (planned for a
        // later phase per spec §8), this default is required to prevent Task entities from defaulting
        // to Replicates and tripping the
        // "Get_Replication(ReplicatedOwner) == ECk_Replication::Replicates" ensure cascade. Remove
        // this once propagation is wired.
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
    // TASK LIFECYCLE (Enter/Exit)
    // ================================================================================================================
    //
    // EnterTask fires from BeginPlay() once the task entity is fully constructed.
    // ExitTask is invoked synchronously by the StateMachine processor (transition or SM EndPlay).

public:
    virtual auto
    EnterTask(
        FCk_Handle_SmTask InHandle,
        ECk_Sm_NetContext InNetContext) -> void;

    virtual auto
    ExitTask(
        FCk_Handle_SmTask InHandle,
        ECk_Sm_NetContext InNetContext) -> void;

    // ================================================================================================================
    // VIRTUAL METHODS (user overrides)
    // ================================================================================================================

public:
    virtual auto
    Tick(
        FCk_Handle_SmTask InHandle,
        FCk_Time InDeltaT,
        ECk_Sm_NetContext InNetContext) -> ECk_SmTaskResult;

    // ================================================================================================================
    // BLUEPRINT IMPLEMENTABLE EVENTS
    // ================================================================================================================

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|Task",
        DisplayName = "Tick")
    ECk_SmTaskResult
    DoTick(
        FCk_Handle_SmTask InHandle,
        FCk_Time InDeltaT,
        ECk_Sm_NetContext InNetContext);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|Task",
        DisplayName = "Enter Task")
    void
    DoEnterTask(
        FCk_Handle_SmTask InHandle,
        ECk_Sm_NetContext InNetContext);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|Task",
        DisplayName = "Exit Task")
    void
    DoExitTask(
        FCk_Handle_SmTask InHandle,
        ECk_Sm_NetContext InNetContext);

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
        DisplayName = "[Ck][SM] Get StateMachine Context",
        meta = (CompactNodeTitle = "Context", HideSelfPin = true))
    FCk_Handle
    Get_StateMachineContext() const;

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
