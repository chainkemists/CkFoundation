#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkStateMachine/CkStateMachine_Fragment_Data.h"

#include "CkSmTask_EntityScript.generated.h"

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKSTATEMACHINE_API UCk_SmTask_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmTask_EntityScript);

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
    OnStateEnter() -> void;

    virtual auto
    OnStateExit() -> void;

    virtual auto
    Tick(
        float InDeltaSeconds) -> ECk_SmTaskResult;

    // ================================================================================================================
    // BLUEPRINT IMPLEMENTABLE EVENTS
    // ================================================================================================================

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|Task",
        DisplayName = "On State Enter")
    void
    DoOnStateEnter(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|SM|Task",
        DisplayName = "On State Exit")
    void
    DoOnStateExit(
        FCk_Handle InHandle);

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

protected:
    UFUNCTION(BlueprintPure,
        Category = "Ck|SM|Task",
        DisplayName = "[Ck][SM] Get Game Entity",
        meta = (CompactNodeTitle = "GameEntity", HideSelfPin = true))
    FCk_Handle
    DoGet_GameEntity() const;

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
