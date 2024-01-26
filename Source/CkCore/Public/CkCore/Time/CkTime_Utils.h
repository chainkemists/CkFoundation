#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkTime_Utils.generated.h"
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Utils_Time_GetWorldTime_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Utils_Time_GetWorldTime_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<const UObject> _Object   = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TObjectPtr<UWorld> _World    = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Time_WorldTimeType _TimeType = ECk_Time_WorldTimeType::PausedAndDilatedAndClamped;

public:
    CK_PROPERTY_GET(_Object);
    CK_PROPERTY_GET(_World);
    CK_PROPERTY_GET(_TimeType);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Utils_Time_GetWorldTime_Params, _Object, _World, _TimeType);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Utils_Time_GetWorldTime_Result
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Utils_Time_GetWorldTime_Result);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time_Unreal _WorldTime;

public:
    CK_PROPERTY_GET(_WorldTime);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Utils_Time_GetWorldTime_Result, _WorldTime);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Time_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Time_UE);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Time To Text",
              Category = "Ck|Utils|Time",
              meta = (CompactNodeTitle = "->", BlueprintAutocast))
    static FText
    Conv_TimeToText(
        const FCk_Time& InTime);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Time To String",
              Category = "Ck|Utils|Chrono",
              meta = (CompactNodeTitle = "->", BlueprintAutocast))
    static FString
    Conv_TimeToString(
        const FCk_Time& InTime);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Make Time (From Seconds)",
              Category = "Ck|Utils|Time",
              meta=(CompactNodeTitle="Seconds"))
    static FCk_Time
    Make_FromSeconds(
        float InSeconds);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Make Time (From Milliseconds)",
              Category = "Ck|Utils|Time",
              meta=(CompactNodeTitle="Milliseconds"))
    static FCk_Time
    Make_FromMilliseconds(
        float InMilliSeconds);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Make World Time",
              Category = "Ck|Utils|Time",
              meta = (NativeMakeFunc, DefaultToSelf = "InWorldContextObject"))
    static FCk_WorldTime
    Make_WorldTime(
        const UObject* InWorldContextObject);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Frame Number",
              Category = "Ck|Utils|Time")
    static int64
    Get_FrameNumber();

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Frame Counter",
              Category = "Ck|Utils|Time")
    static int64
    Get_FrameCounter();

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get World Time",
              Category = "Ck|Utils|Time")
    static FCk_Utils_Time_GetWorldTime_Result
    Get_WorldTime(
        const FCk_Utils_Time_GetWorldTime_Params& InParams);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Milliseconds",
              Category = "Ck|Utils|Time")
    static float
    Get_Milliseconds(
        const FCk_Time& InTime);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Time Zero",
              Category = "Ck|Utils|Time",
              meta = (CompactNodeTitle = "Is Zero"))
    static bool
    Get_IsZero(
        const FCk_Time& InTime);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Time + Time",
              Category = "Ck|Utils|Time",
              meta = (CompactNodeTitle = "+", KeyWords = "add"))
    static FCk_Time
    Add(
        const FCk_Time& InA,
        const FCk_Time& InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Time - Time",
              Category = "Ck|Utils|Time",
              meta = (CompactNodeTitle = "-", KeyWords = "subtract"))
    static FCk_Time
    Subtract(const FCk_Time& InA,
        const FCk_Time& InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Time / Time",
              Category = "Ck|Utils|Time",
              meta = (CompactNodeTitle = "/", KeyWords = "divide"))
    static FCk_Time
    Divide(const FCk_Time& InA,
        const FCk_Time& InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Time * Time",
              Category = "Ck|Utils|Time",
              meta = (CompactNodeTitle = "*", KeyWords = "multiply"))
    static FCk_Time
    Multiply(
        const FCk_Time& InA,
        const FCk_Time& InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Time == Time",
              Category = "Ck|Utils|Time",
              meta = (CompactNodeTitle = "==", KeyWords = "equal"))
    static bool
    IsEqual(
        const FCk_Time& InA,
        const FCk_Time& InB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Time != Time",
              Category = "Ck|Utils|Time",
              meta = (CompactNodeTitle = "!=", KeyWords = "equal, not"))
    static bool
    IsNotEqual(
        const FCk_Time& InA,
        const FCk_Time& InB);
};

// --------------------------------------------------------------------------------------------------------------------
