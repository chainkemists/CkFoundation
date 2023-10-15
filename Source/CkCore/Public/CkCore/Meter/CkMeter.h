#pragma once

#include "CkCore/Chrono/CkChrono.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"
#include "CkCore/Meter/CkMeter_Data.h"

#include "CkMeter.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (
    HasNativeMake = "/Script/CkCore.Ck_Utils_Meter_UE:Make_Meter",
    HasNativeBreak = "/Script/CkCore.Ck_Utils_Meter_UE:Break_Meter"))
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

    auto operator+(const ThisType& InOther) const -> ThisType;
    auto operator-(const ThisType& InOther) const -> ThisType;
    CK_DECL_AND_DEF_ADD_SUBTRACT_ASSIGNMENT_OPERATORS(ThisType);

    auto operator*(const ThisType& InOther) const -> ThisType;
    auto operator/(const ThisType& InOther) const -> ThisType;
    CK_DECL_AND_DEF_MULTIPLY_DIVIDE_ASSIGNMENT_OPERATORS(ThisType);

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

CK_DEFINE_CUSTOM_FORMATTER(FCk_Meter, [&]()
{
    return ck::Format
    (
        TEXT("Min:[{}] Max:[{}] Used:[{}]"),
        InObj.Get_Params().Get_Capacity().Get_MinCapacity(),
        InObj.Get_Params().Get_Capacity().Get_MaxCapacity(),
        InObj.Get_Used().Get_AmountUsed()
    );
});

// --------------------------------------------------------------------------------------------------------------------
