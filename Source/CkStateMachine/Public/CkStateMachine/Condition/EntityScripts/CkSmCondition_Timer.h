#pragma once

#include "CkSmCondition_EntityScript.h"

#include "CkCore/Time/CkTime.h"
#include "CkTimer/CkTimer_Fragment_Data.h"

#include "CkSmCondition_Timer.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable, BlueprintType,
    meta = (DisplayName = "SM Condition: Timer"))
class CKSTATEMACHINE_API UCk_SmCondition_Timer : public UCk_SmCondition_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_SmCondition_Timer);

    UCk_SmCondition_Timer()
    {
        _ConditionMode = ECk_SmConditionMode::EventDriven;
        _ResetBehavior = ECk_SmConditionResetBehavior::Manual;
    }

protected:
    auto
    BeginPlay() -> void override;

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
};

// --------------------------------------------------------------------------------------------------------------------
