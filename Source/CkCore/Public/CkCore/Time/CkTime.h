#pragma once
#include "CkCore/Format/CkFormat.h"

#include "CkCore/Macros/CkMacros.h"

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

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKCORE_API FCk_Time
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Time);

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
    static auto ZeroSecond() -> FCk_Time;
    static auto OneMillisecond() -> FCk_Time;
    static auto TenMilliseconds() -> FCk_Time;
    static auto HundredMilliseconds() -> FCk_Time;
    static auto OneSecond() -> FCk_Time;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, Units = "Seconds", UIMin = "0.0", ClampMin = "0.0"))
    float _Seconds = 0.0f;

public:
    CK_PROPERTY_GET(_Seconds);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Time, _Seconds);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake = "/Script/CkCore.Ck_Utils_Time_UE:Make_WorldTime"))
struct CKCORE_API FCk_WorldTime
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WorldTime);

public:
    using TimeType = FCk_Time;

public:
    FCk_WorldTime() = default;
    explicit FCk_WorldTime(const UObject* InWorldContextObject);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Time _Time;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Time _RealTime;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Time _DeltaT;

private:
    uint64 _FrameNumber = 0;

public:
    CK_PROPERTY_GET(_Time);
    CK_PROPERTY_GET(_RealTime);
    CK_PROPERTY_GET(_DeltaT);
    CK_PROPERTY_GET(_FrameNumber);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKCORE_API FCk_Time_Unreal
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Time_Unreal);

    friend class UCk_Utils_Time_UE;

    using TimeType      = FCk_Time;
    using WorldTimeType = ECk_Time_WorldTimeType;

public:
    auto operator==(const ThisType& InOther) const -> bool;
    auto operator< (const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATORS(ThisType);

    auto operator-(const ThisType& InOther) const  -> ThisType;
    auto operator+(const ThisType& InOther) const  -> ThisType;
    auto operator*(const ThisType& InOther) const  -> ThisType;
    auto operator/(const ThisType& InOther) const  -> ThisType;
    CK_DECL_AND_DEF_ADD_SUBTRACT_ASSIGNMENT_OPERATORS(ThisType);

    auto operator==(const TimeType& InOther) const -> bool;
    auto operator< (const TimeType& InOther) const -> bool;
    auto operator!=(const TimeType& InOther) const -> bool;
    auto operator> (const TimeType& InOther) const -> bool;
    auto operator<=(const TimeType& InOther) const -> bool;
    auto operator>=(const TimeType& InOther) const -> bool;

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

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Time_Unreal, _Time, _TimeType);
};

// --------------------------------------------------------------------------------------------------------------------
// IsValid and Formatters

CK_DEFINE_CUSTOM_FORMATTER(FCk_Time, [&]()
{
    return ck::Format(TEXT("{:.3f}s"), InObj.Get_Seconds());
});

CK_DEFINE_CUSTOM_FORMATTER(FCk_WorldTime, [&]()
{
    return ck::Format
    (
        TEXT("Time: [{}], RealTime: [{}], DeltaT: [{}], FrameNumber: [{}]"),
        InObj.Get_Time(),
        InObj.Get_RealTime(),
        InObj.Get_DeltaT(),
        InObj.Get_FrameNumber()
    );
});

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

