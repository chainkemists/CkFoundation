#pragma once

#include "CkSmCondition_EventDriven.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkSmCondition_SubSmFinished.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Condition: Sub-SM Finished"))
class CKSTATEMACHINE_API UCk_SmCondition_SubSmFinished : public UCk_SmCondition_EventDriven
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_SubSmFinished);

    UCk_SmCondition_SubSmFinished() = default;

public:
    // Setup/teardown live in the condition lifecycle (not BeginPlay/EndPlay) so the base's
    // transition-authority gate suppresses them on non-authority machines.
    auto
    EnterCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext) -> void override;

    auto
    ExitCondition(
        FCk_Handle_SmCondition InHandle,
        ECk_Sm_NetContext InNetContext) -> void override;

protected:
    UFUNCTION()
    void
    OnSubSmStopped(
        FCk_Handle_StateMachine InHandle,
        FCk_Sm_Payload_OnStopped InPayload);

    UFUNCTION()
    void
    OnSubSmConstructed(
        FCk_Handle_SmTask InTaskHandle,
        FCk_Sm_Payload_OnSubSmConstructed InPayload);

private:
    auto
    DoBindToSubSm(
        FCk_Handle_StateMachine InSubSm) -> void;

private:
    TArray<FCk_Handle_StateMachine> _BoundSubSms;
    TArray<FCk_Handle_SmTask> _PendingTasks;
};

// --------------------------------------------------------------------------------------------------------------------
