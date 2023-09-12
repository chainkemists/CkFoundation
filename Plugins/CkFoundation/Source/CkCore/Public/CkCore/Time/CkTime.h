#pragma once
#include "CkFormat/CkFormat.h"

#include "CkMacros/CkMacros.h"

#include "CkTime.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Time_WorldTimeType : uint8
{
    PausedAndDilatedAndClamped,
    DilatedAndClampedOnly,
    RealTime,
    AudioTime,
    DeltaTime,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Time_WorldTimeType);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Time
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Time);

public:
    FCk_Time() = default;
    explicit FCk_Time(float InSeconds);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    auto operator<(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATORS(ThisType);

    auto operator+(ThisType InOther) const -> ThisType;
    auto operator-(ThisType InOther) const -> ThisType;
    auto operator*(ThisType InOther) const -> ThisType;
    auto operator/(ThisType InOther) const -> ThisType;
    CK_DECL_AND_DEF_ADD_SUBTRACT_ASSIGNMENT_OPERATORS(ThisType);

public:
    auto Get_Milliseconds() const -> float;

public:
    static const FCk_Time Zero;

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true, Units = "Seconds"))
    float _Seconds = 0.0f;

public:
    CK_PROPERTY_GET(_Seconds);
};

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(FCk_Time, [&]()
{
    return ck::Format(TEXT("{.3f}s"), InObj.Get_Seconds());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Time_Unreal
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Time_Unreal);

    friend class UCk_Utils_Time_UE;

    using TimeType       = FCk_Time;
    using WorldTimeType = ECk_Time_WorldTimeType;

public:
    FCk_Time_Unreal() = default;

private:
    FCk_Time_Unreal(
        const TimeType& InTime,
        WorldTimeType  InTimeType);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    auto operator< (const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATORS(ThisType);

    auto operator-(const ThisType& InOther) const  -> ThisType;
    auto operator+(const ThisType& InOther) const  -> ThisType;
    auto operator*(const ThisType& InOther) const  -> ThisType;
    auto operator/(const ThisType& InOther) const  -> ThisType;
    CK_DECL_AND_DEF_ADD_SUBTRACT_ASSIGNMENT_OPERATORS(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _Time;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Time_WorldTimeType _TimeType = ECk_Time_WorldTimeType::PausedAndDilatedAndClamped;

public:
    CK_PROPERTY_GET(_Time)
    CK_PROPERTY_GET(_TimeType)
};

CK_DEFINE_CUSTOM_FORMATTER(FCk_Time_Unreal, [&]()
{
    return ck::Format
    (
        TEXT("Time: [{}], TimeType: [{}]"),
        InObj.Get_Time(),
        InObj.Get_TimeType()
    );
});

// --------------------------------------------------------------------------------------------------------------------

