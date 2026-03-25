#pragma once

#include "CkSmTask_EntityScript.h"

#include "CkSmTask_Delay.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Task: Delay"))
class CKSTATEMACHINE_API UCk_SmTask_Delay : public UCk_SmTask_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmTask_Delay);

    UCk_SmTask_Delay()
    {
        _TaskMode = ECk_SmTaskMode::Tick;
    }

    auto
    OnStateEnter() -> void override;

    auto
    Tick(
        float InDeltaSeconds) -> ECk_SmTaskResult override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Task|Delay",
        meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _DelaySeconds = 1.0f;

public:
    CK_PROPERTY_GET(_DelaySeconds);

private:
    float _ElapsedSeconds = 0.0f;
};

// --------------------------------------------------------------------------------------------------------------------
