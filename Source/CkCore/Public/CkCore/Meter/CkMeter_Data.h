#pragma once

#include "CkCore/Chrono/CkChrono.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include "CkMeter_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Meter_Current
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Meter_Current);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _CurrentValue = 0.0f;

public:
    CK_PROPERTY_GET(_CurrentValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Meter_Current, _CurrentValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Meter_Size
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Meter_Size);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _Size = 0.0f;

public:
    CK_PROPERTY_GET(_Size);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Meter_Size, _Size);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Meter_Remaining
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Meter_Remaining);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _AmountRemaining = 0.0f;

public:
    CK_PROPERTY_GET(_AmountRemaining);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Meter_Remaining, _AmountRemaining);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Meter_Used
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Meter_Used);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _AmountUsed = 0.0f;

public:
    CK_PROPERTY_GET(_AmountUsed);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Meter_Used, _AmountUsed);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Meter_Replenish
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Meter_Replenish);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = 0.0f, UIMin = 0.0f))
    float _ReplenishAmount = 0.0f;

public:
    CK_PROPERTY_GET(_ReplenishAmount);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Meter_Replenish, _ReplenishAmount);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Meter_Consume
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Meter_Consume);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = 0.0f, UIMin = 0.0f))
    float _ConsumeAmount = 0.0f;

public:
    CK_PROPERTY_GET(_ConsumeAmount);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Meter_Consume, _ConsumeAmount);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Meter_Capacity
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Meter_Capacity);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _MinCapacity = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _MaxCapacity = 0.0f;

public:
    CK_PROPERTY(_MinCapacity);
    CK_PROPERTY_GET(_MaxCapacity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Meter_Capacity, _MaxCapacity);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Meter_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Meter_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Meter_Capacity _Capacity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_FloatRange_0to1 _StartingPercentage = FCk_FloatRange_0to1{0.0f};

public:
    CK_PROPERTY_GET(_Capacity);
    CK_PROPERTY(_StartingPercentage);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Meter_Params, _Capacity);
};

// --------------------------------------------------------------------------------------------------------------------
