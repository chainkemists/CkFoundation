#include "CkTime.h"

#include "CkEnsure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

const FCk_Time FCk_Time::Zero = FCk_Time{0.0f};

// --------------------------------------------------------------------------------------------------------------------

FCk_Time::
FCk_Time(float InSeconds)
    : _Seconds(InSeconds)
{
}

auto FCk_Time::
operator==(const ThisType& InOther) const -> bool
{
    return _Seconds == InOther._Seconds;
}

auto FCk_Time::
operator<(const ThisType& InOther) const -> bool
{
    return _Seconds < InOther._Seconds;
}

auto FCk_Time::
operator+(ThisType InOther) const -> ThisType
{
    return ThisType{_Seconds + InOther._Seconds};
}

auto FCk_Time::
operator-(ThisType InOther) const -> ThisType
{
    return ThisType{_Seconds - InOther._Seconds};
}

auto FCk_Time::
operator*(ThisType InOther) const -> ThisType
{
    return ThisType{_Seconds * InOther._Seconds};
}

auto FCk_Time::
operator/(ThisType InOther) const -> ThisType
{
    return ThisType{_Seconds / InOther._Seconds};
}

auto FCk_Time::
Get_Milliseconds() const -> float
{
    return _Seconds * 1000.0f;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Time_Unreal::
    FCk_Time_Unreal(
        const TimeType& InTime,
        WorldTimeType  InTimeType)
    : _Time(InTime)
    , _TimeType(InTimeType)
{
}

auto
    FCk_Time_Unreal::
    operator==(const ThisType& InOther) const
    -> bool
{
    const auto& IsSameWorldTimeType = Get_TimeType() == InOther.Get_TimeType();

    if (NOT CK_ENSURE(IsSameWorldTimeType,
        TEXT("WorldTimeTypes [{}] and [{}] do not match"),
        Get_TimeType(),
        InOther.Get_TimeType()))
    {
        return false;
    }

    return Get_Time() == InOther.Get_Time();
}

auto
    FCk_Time_Unreal::
    operator<(const ThisType& InOther) const
    -> bool
{
    const auto& IsSameWorldTimeType = Get_TimeType() == InOther.Get_TimeType();

    if (NOT CK_ENSURE(IsSameWorldTimeType,
        TEXT("WorldTimeTypes [{}] and [{}] do not match"),
        Get_TimeType(),
        InOther.Get_TimeType()))
    {
        return false;
    }

    return Get_Time() < InOther.Get_Time();
}

auto
    FCk_Time_Unreal::
    operator-(const ThisType& InOther) const
    -> ThisType
{
    const auto& IsSameWorldTimeType = Get_TimeType() == InOther.Get_TimeType();

    CK_ENSURE
    (
        IsSameWorldTimeType,
        TEXT("WorldTimeTypes [{}] and [{}] do not match"),
        Get_TimeType(),
        InOther.Get_TimeType()
    );

    return ThisType{ Get_Time() - InOther.Get_Time(), Get_TimeType() };
}

auto
    FCk_Time_Unreal::
    operator+(const ThisType& InOther) const
    -> ThisType
{
    const auto& IsSameWorldTimeType = Get_TimeType() == InOther.Get_TimeType();

    CK_ENSURE
    (
        IsSameWorldTimeType,
        TEXT("WorldTimeTypes [{}] and [{}] do not match"),
        Get_TimeType(),
        InOther.Get_TimeType()
    );

    return ThisType{ Get_Time() + InOther.Get_Time(), Get_TimeType() };
}

auto
    FCk_Time_Unreal::
    operator*(const ThisType& InOther) const
    -> ThisType
{
    const auto& IsSameWorldTimeType = Get_TimeType() == InOther.Get_TimeType();

    CK_ENSURE
    (
        IsSameWorldTimeType,
        TEXT("WorldTimeTypes [{}] and [{}] do not match"),
        Get_TimeType(),
        InOther.Get_TimeType()
    );

    return ThisType{ Get_Time() * InOther.Get_Time(), Get_TimeType() };
}

auto
    FCk_Time_Unreal::
    operator/(const ThisType& InOther) const
    -> ThisType
{
    const auto& IsSameWorldTimeType = Get_TimeType() == InOther.Get_TimeType();

    CK_ENSURE
    (
        IsSameWorldTimeType,
        TEXT("WorldTimeTypes [{}] and [{}] do not match"),
        Get_TimeType(),
        InOther.Get_TimeType()
    );

    return ThisType{ Get_Time() / InOther.Get_Time(), Get_TimeType() };
}

// --------------------------------------------------------------------------------------------------------------------
