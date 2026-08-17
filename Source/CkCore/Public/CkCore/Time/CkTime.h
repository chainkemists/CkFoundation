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

    auto operator-() const -> ThisType;

    auto operator+(const ThisType& InOther) const -> ThisType;
    auto operator-(const ThisType& InOther) const -> ThisType;
    auto operator*(double InOther) const -> ThisType;
    auto operator*(int32 InOther) const -> ThisType;
    auto operator/(const ThisType& InOther) const -> double;
    auto operator/(double InOther) const -> ThisType;
    auto operator/(int32 InOther) const -> ThisType;
    CK_DECL_AND_DEF_ADD_SUBTRACT_ASSIGNMENT_OPERATORS(ThisType);

    // Required for angelscript operator bindings
    auto OpCmp(const ThisType& InOther) const -> int;

public:
    auto Get_Milliseconds() const -> double;

public:
    static auto
    ZeroSecond() -> const FCk_Time&;

    static auto
    OneMillisecond() -> const FCk_Time&;

    static auto
    TenMilliseconds() -> const FCk_Time&;

    static auto
    HundredMilliseconds() -> const FCk_Time&;

    static auto
    OneSecond() -> const FCk_Time&;

private:
    // SaveGame: FCk_Time is a plain UHT struct (no native serializer), so CkSnapshot's SaveGame-gated
    // tagged-property recursion needs the inner field tagged or any snapshotted FCk_Time restores as
    // 0s even when the OUTER field carries SaveGame (e.g. RenderTarget Params' _SyncInterval).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame,
        meta = (AllowPrivateAccess = true, Units = "Seconds", UIMin = "0.0", ClampMin = "0.0"))
    double _Seconds = 0.0;

public:
    CK_PROPERTY_GET(_Seconds);

public:
    // Hand-written (not CK_DEFINE_CONSTRUCTORS) so they are constexpr — FCk_Time is then a literal type usable
    // in compile-time contexts (e.g. a processor's `static constexpr FCk_Time TickRate`).
    constexpr FCk_Time() = default;
    constexpr explicit FCk_Time(double InSeconds) : _Seconds(InSeconds) {}
    CK_ANGELSCRIPT_CTOR_REGISTRATION(FCk_Time, _Seconds);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::time
{
    namespace detail
    {
        // Declared, never defined, not constexpr: naming it in a factory's invalid branch makes a non-positive
        // interval a COMPILE error (the diagnostic names this function). Never reached at runtime — the
        // factories are consteval, so they are always constant-evaluated.
        auto Interval_MustBePositive() -> void;
    }

    // Compile-time FCk_Time interval literals, e.g. `static constexpr FCk_Time TickRate = ck::time::Hz(4);`
    // (== ck::time::Seconds(0.25)). For an every-tick processor declare no TickRate at all rather than reaching
    // for a zero interval. Runtime construction stays FCk_Time{Seconds}.

    consteval auto
    Seconds(
        double InSeconds)
        -> FCk_Time
    {
        if (NOT (InSeconds > 0.0))
        { detail::Interval_MustBePositive(); }

        return FCk_Time{InSeconds};
    }

    consteval auto
    Milliseconds(
        double InMilliseconds)
        -> FCk_Time
    {
        if (NOT (InMilliseconds > 0.0))
        { detail::Interval_MustBePositive(); }

        return FCk_Time{InMilliseconds / 1000.0};
    }

    consteval auto
    Minutes(
        double InMinutes)
        -> FCk_Time
    {
        if (NOT (InMinutes > 0.0))
        { detail::Interval_MustBePositive(); }

        return FCk_Time{InMinutes * 60.0};
    }

    consteval auto
    Hz(
        double InCyclesPerSecond)
        -> FCk_Time
    {
        if (NOT (InCyclesPerSecond > 0.0))
        { detail::Interval_MustBePositive(); }

        return FCk_Time{1.0 / InCyclesPerSecond};
    }
}

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
    auto operator*(double InOther) const -> ThisType;
    auto operator*(int32 InOther) const -> ThisType;
    auto operator/(const ThisType& InOther) const -> double;
    auto operator/(double InOther) const -> ThisType;
    auto operator/(int32 InOther) const -> ThisType;
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

// Accumulates its own lifetime into InOutTotal. Nesting a scope inside another reports both, so a stage
// containing a sub-stage needs no subtraction at the call site.
class CKCORE_API FCk_ScopedStopwatch
{
public:
    explicit FCk_ScopedStopwatch(FCk_Time& InOutTotal);
    ~FCk_ScopedStopwatch();

    FCk_ScopedStopwatch(const FCk_ScopedStopwatch&) = delete;
    auto operator=(const FCk_ScopedStopwatch&) -> FCk_ScopedStopwatch& = delete;

private:
    FCk_Time& _Total;
    double    _StartSeconds = 0.0;
};

// --------------------------------------------------------------------------------------------------------------------
// IsValid and Formatters

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_Time, [](const FCk_Time& InObj)
{
    return ck::Format(TEXT("{:.3f}s"), InObj.Get_Seconds());
});

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_WorldTime, [&]()
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

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_Time_Unreal, [&]()
{
    return ck::Format
    (
        TEXT("Time: [{}], TimeType: [{}]"),
        InObj.Get_Time(),
        InObj.Get_TimeType()
    );
});

// --------------------------------------------------------------------------------------------------------------------

