#pragma once

#include "CkSmCondition_EventDriven.h"

#include "CkCore/Time/CkTime.h"
#include "CkTimer/CkTimer_Fragment_Data.h"

#include "CkSmCondition_Timer.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Condition: Timer"))
class CKSTATEMACHINE_API UCk_SmCondition_Timer : public UCk_SmCondition_EventDriven
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_Timer);

    UCk_SmCondition_Timer() = default;

public:
    // Setup/teardown live in the condition lifecycle (not BeginPlay/EndPlay) so the base's
    // transition-authority gate suppresses them on non-authority machines — a machine that
    // follows the relay must not run this condition's timer.
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
    OnTimerComplete(
        FCk_Handle_Timer InHandle,
        FCk_Chrono InChrono,
        FCk_Time InDeltaT);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "SM|Condition|Timer",
        meta = (AllowPrivateAccess = true))
    FCk_Time _Duration = FCk_Time{1.0f};

public:
    CK_PROPERTY_GET(_Duration);

private:
    FCk_Handle_Timer _TimerHandle;
};

// --------------------------------------------------------------------------------------------------------------------
