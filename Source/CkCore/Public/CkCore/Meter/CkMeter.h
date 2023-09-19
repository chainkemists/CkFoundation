#pragma once

#include "CkCore/Chrono/CkChrono.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include "CkMeter.generated.h"

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
    CK_PROPERTY_GET(_MinCapacity);
    CK_PROPERTY_GET(_MaxCapacity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Meter_Capacity, _MinCapacity, _MaxCapacity);
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
    FCk_FloatRange_0to1 _StartingValue = FCk_FloatRange_0to1{0.0f};

public:
    CK_PROPERTY_GET(_Capacity);
    CK_PROPERTY_GET(_StartingValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Meter_Params, _Capacity, _StartingValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake = "CkCore.Ck_Utils_Meter_UE.Make_Meter", HasNativeBreak = "CkCore.Ck_Utils_Meter_UE.Break_Meter"))
struct CKCORE_API FCk_Meter
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Meter);

public:
    using ChronoType = FCk_Chrono;

public:
    FCk_Meter() = default;
    explicit FCk_Meter(FCk_Meter_Params InParams);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

public:
    auto Consume(const FCk_Meter_Consume& InConsume) -> ThisType&;
    auto Replenish(const FCk_Meter_Replenish& InReplenish) -> ThisType&;

public:
    auto Get_IsFull() const -> bool;
    auto Get_IsEmpty() const -> bool;
    auto Get_Remaining() const -> FCk_Meter_Remaining;
    auto Get_Used() const -> FCk_Meter_Used;
    auto Get_Value() const -> FCk_Meter_Current;
    auto Get_PercentageUsed() const -> FCk_FloatRange_0to1;
    auto Get_PercentageRemaining() const -> FCk_FloatRange_0to1;
    auto Get_Size() const -> FCk_Meter_Size;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Meter_Params _Params;

private:
    ChronoType _InternalChrono;

public:
    CK_PROPERTY_GET(_Params);
};

// --------------------------------------------------------------------------------------------------------------------
