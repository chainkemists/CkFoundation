#pragma once

#include "CkCore/Chrono/CkChrono.h"
#include "CkCore/Time/CkTime.h"

#include "CkChrono_Utils.generated.h"


// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Chrono_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Chrono_UE);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Chrono",
              meta = (NativeBreakFunc))
    static void
    Break_Chrono(
        const FCk_Chrono& InChrono,
        FCk_Time& OutGoal,
        FCk_Time& OutTimeElapsed,
        FCk_Time& OutTimeRemaining);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Chrono")
    static bool
    Get_IsDone(
        const FCk_Chrono& InChrono);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Chrono")
    static FCk_Time
    Get_TimeRemaining(
        const FCk_Chrono&       InChrono,
        ECk_NormalizationPolicy InNormalization = ECk_NormalizationPolicy::None);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Chrono")
    static FCk_Time
    Get_TimeElapsed(
        const FCk_Chrono&       InChrono,
        ECk_NormalizationPolicy InNormalization = ECk_NormalizationPolicy::None);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chrono")
    static ECk_Chrono_TickState
    Tick_Chrono(
        UPARAM(ref) FCk_Chrono& InChrono,
        const FCk_Time&         InDeltaT);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chrono")
    static ECk_Chrono_ConsumeState
    Consume_Chrono(
        UPARAM(ref) FCk_Chrono& InChrono,
        const FCk_Time&         InDeltaT);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chrono")
    static void
    Reset_Chrono(
        UPARAM(ref) FCk_Chrono& InChrono);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chrono")
    static void
    Complete_Chrono(
        UPARAM(ref) FCk_Chrono& InChrono);
};

// --------------------------------------------------------------------------------------------------------------------
