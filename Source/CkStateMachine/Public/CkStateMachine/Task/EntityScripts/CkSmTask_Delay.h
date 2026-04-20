#pragma once

#include "CkSmTask_EntityScript.h"

#include "CkCore/Time/CkTime.h"
#include "CkTimer/CkTimer_Fragment_Data.h"

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
        _TaskMode = ECk_SmTaskMode::EnterExitOnly;
    }

public:
    auto
    EnterTask(
        FCk_Handle_SmTask InHandle) -> void override;

    auto
    ExitTask(
        FCk_Handle_SmTask InHandle) -> void override;

protected:
    UFUNCTION()
    void
    OnDelayTimerComplete(
        FCk_Handle_Timer InHandle,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Task|Delay",
        meta = (AllowPrivateAccess = true))
    FCk_Time _Duration = FCk_Time{1.0f};

public:
    CK_PROPERTY_GET(_Duration);

private:
    FCk_Handle_Timer _DelayTimerHandle;
};

// --------------------------------------------------------------------------------------------------------------------
