#pragma once

#include "CkCore/Chrono/CkChrono.h"
#include "CkCore/Time/CkTime.h"

#include "CkChrono_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Chrono"))
class CKCORE_API UCk_Utils_Chrono_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Chrono_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Chrono",
              DisplayName = "[Ck] Chrono -> Text",
              meta = (CompactNodeTitle = "->", BlueprintAutocast))
    static FText
    Conv_ChronoToText(
        const FCk_Chrono& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Chrono",
              DisplayName = "Chrono -> String",
              meta = (CompactNodeTitle = "->", BlueprintAutocast))
    static FString
    Conv_ChronoToString(
        const FCk_Chrono& InHandle);

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
              Category = "Ck|Utils|Chrono",
              DisplayName = "[Ck] Get Is Chrono Done")
    static bool
    Get_IsDone(
        const FCk_Chrono& InChrono);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Chrono",
              DisplayName = "[Ck] Get Chrono Time Remaining")
    static FCk_Time
    Get_TimeRemaining(
        const FCk_Chrono& InChrono,
        ECk_NormalizationPolicy InNormalization = ECk_NormalizationPolicy::None);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Chrono",
              DisplayName = "[Ck] Get Chrono Time Elapsed")
    static FCk_Time
    Get_TimeElapsed(
        const FCk_Chrono& InChrono,
        ECk_NormalizationPolicy InNormalization = ECk_NormalizationPolicy::None);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chrono",
              DisplayName = "[Ck] Tick Chrono")
    static ECk_Chrono_TickState
    Tick(
        UPARAM(ref) FCk_Chrono& InChrono,
        const FCk_Time& InDeltaT);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chrono",
              DisplayName = "[Ck] Consume Chrono")
    static ECk_Chrono_ConsumeState
    Consume(
        UPARAM(ref) FCk_Chrono& InChrono,
        const FCk_Time& InDeltaT);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chrono",
              DisplayName = "[Ck] Reset Chrono")
    static void
    Reset(
        UPARAM(ref) FCk_Chrono& InChrono);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Chrono",
              DisplayName = "[Ck] Complete Chrono")
    static void
    Complete(
        UPARAM(ref) FCk_Chrono& InChrono);
};

// --------------------------------------------------------------------------------------------------------------------
