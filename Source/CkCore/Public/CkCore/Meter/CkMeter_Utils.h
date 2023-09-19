#pragma once

#include "CkCore/Meter/CkMeter.h"

#include "CkMeter_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Meter_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Meter_UE);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Meter",
              meta = (NativeMakeFunc))
    static FCk_Meter
    Make_Meter(
        const FCk_Meter_Params& InMeterParams);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Meter",
              meta = (NativeBreakFunc))
    static void
    Break_Meter(
        const FCk_Meter& InMeter,
        FCk_Meter_Capacity& OutMeterCapacity,
        FCk_Meter_Current& OutMeterCurrentValue,
        FCk_Meter_Remaining& OutRemaining,
        FCk_Meter_Used& OutUsed);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Meter")
    static FCk_Meter
    Consume_Meter(
        UPARAM(ref) FCk_Meter& InMeter,
        const FCk_Meter_Consume& InConsume);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Meter")
    static FCk_Meter
    Replenish_Meter(
        UPARAM(ref) FCk_Meter& InMeter,
        const FCk_Meter_Replenish& InReplenish);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Meter")
    static FCk_Meter_Size
    Get_MeterSize(
        const FCk_Meter& InMeter);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Meter")
    static FCk_FloatRange_0to1
    Get_MeterPercentageUsed(
        const FCk_Meter& InMeter);

        UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Meter")
    static FCk_FloatRange_0to1
    Get_MeterPercentageRemaining(
        const FCk_Meter& InMeter);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Meter")
    static bool
    Get_IsMeterFull(
        const FCk_Meter& InMeter);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Meter")
    static bool
    Get_IsMeterEmpty(
        const FCk_Meter& InMeter);
};

// --------------------------------------------------------------------------------------------------------------------