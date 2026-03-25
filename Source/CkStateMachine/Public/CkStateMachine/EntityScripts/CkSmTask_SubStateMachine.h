#pragma once

#include "CkSmTask_EntityScript.h"

#include "CkStateMachine/CkStateMachine_Fragment_Data.h"

#include "CkSmState_EntityScript.h"

#include "CkSmTask_SubStateMachine.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Task: Sub-StateMachine"))
class CKSTATEMACHINE_API UCk_SmTask_SubStateMachine : public UCk_SmTask_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmTask_SubStateMachine);

    UCk_SmTask_SubStateMachine()
    {
        _TaskMode = ECk_SmTaskMode::Tick;
    }

    auto
    OnStateEnter() -> void override;

    auto
    OnStateExit() -> void override;

    auto
    Tick(
        float InDeltaSeconds) -> ECk_SmTaskResult override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Task|SubStateMachine",
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_SmState_EntityScript> _InitialStateClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Task|SubStateMachine",
        meta = (AllowPrivateAccess = true))
    ECk_SmTask_SubSm_CompletionBehavior _CompletionBehavior =
        ECk_SmTask_SubSm_CompletionBehavior::KeepRunning;

public:
    CK_PROPERTY_GET(_InitialStateClass);
    CK_PROPERTY_GET(_CompletionBehavior);

private:
    FCk_Handle_StateMachine _SubSmHandle;
};

// --------------------------------------------------------------------------------------------------------------------
