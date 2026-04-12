#pragma once

#include "CkSmCondition_EventDriven.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Request_Data.h"

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

protected:
    auto
    BeginPlay() -> void override;

    auto
    EndPlay() -> void override;

    UFUNCTION()
    void
    OnSubSmStopped(
        FCk_Handle_StateMachine InHandle,
        FCk_Sm_Payload_OnStopped InPayload);

private:
    TArray<FCk_Handle_StateMachine> _BoundSubSms;
};

// --------------------------------------------------------------------------------------------------------------------
