#include "CkTime.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Game/CkGame_Utils.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Time::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return FMath::IsNearlyEqual(_Seconds, InOther._Seconds);
}

auto
    FCk_Time::
    operator<(
        const ThisType& InOther) const
    -> bool
{
    return _Seconds < InOther._Seconds;
}

auto
    FCk_Time::
    operator+(
        const ThisType& InOther) const
    -> ThisType
{
    return ThisType{_Seconds + InOther._Seconds};
}

auto
    FCk_Time::
    operator-(
        const ThisType& InOther) const
    -> ThisType
{
    return ThisType{_Seconds - InOther._Seconds};
}

auto
    FCk_Time::
    operator*(
        const ThisType& InOther) const
    -> ThisType
{
    return ThisType{_Seconds * InOther._Seconds};
}

auto
    FCk_Time::
    operator/(
        const ThisType& InOther) const
    -> ThisType
{
    return ThisType{_Seconds / InOther._Seconds};
}

auto
    FCk_Time::
    Get_Milliseconds() const
    -> float
{
    return _Seconds * 1000.0f;
}

auto
    FCk_Time::
    ZeroSecond()
    -> const FCk_Time&
{
    static auto ZeroSecond = FCk_Time{ 0.0f };
    return ZeroSecond;
}

auto
    FCk_Time::
    OneMillisecond()
    -> const FCk_Time&
{
    static auto OneMillisecond = FCk_Time{ 0.001f };
    return OneMillisecond;
}

auto
    FCk_Time::
    TenMilliseconds()
    -> const FCk_Time&
{
    static auto TenMilliseconds = FCk_Time{ 0.01f };
    return TenMilliseconds;
}

auto
    FCk_Time::
    HundredMilliseconds()
    -> const FCk_Time&
{
    static auto HundredMilliseconds = FCk_Time{ 0.1f };
    return HundredMilliseconds;
}

auto
    FCk_Time::
    OneSecond()
    -> const FCk_Time&
{
    static auto OneSecond = FCk_Time{ 1.0f };
    return OneSecond;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_WorldTime::
    FCk_WorldTime(
        const UObject* InWorldContextObject)
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject), TEXT("Invalid World Context Object when trying to construct FCk_WorldTime!"))
    { return; }

    const auto& gameInstance = UCk_Utils_Game_UE::Get_GameInstance(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(gameInstance), TEXT("Could not get a valid GameInstance when trying to construct FCk_WorldTime!"))
    { return; }

    const auto& currentWorld = gameInstance->GetWorld();

    CK_ENSURE_IF_NOT(ck::IsValid(currentWorld), TEXT("Could not get a valid World when trying to construct FCk_WorldTime!"))
    { return; }

    _Time        = TimeType{static_cast<float>(currentWorld->TimeSeconds)};
    _RealTime    = TimeType{static_cast<float>(currentWorld->RealTimeSeconds)};
    _DeltaT      = TimeType{(currentWorld->GetDeltaSeconds())};
    _FrameNumber = GFrameCounter;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Time_Unreal::
    operator==(const ThisType& InOther) const
    -> bool
{
    const auto& IsSameWorldTimeType = Get_TimeType() == InOther.Get_TimeType();

    CK_ENSURE_IF_NOT(IsSameWorldTimeType,
        TEXT("WorldTimeTypes [{}] and [{}] do not match"),
        Get_TimeType(),
        InOther.Get_TimeType())
    { return false; }

    return Get_Time() == InOther.Get_Time();
}

auto
    FCk_Time_Unreal::
    operator<(const ThisType& InOther) const
    -> bool
{
    const auto& IsSameWorldTimeType = Get_TimeType() == InOther.Get_TimeType();

    CK_ENSURE_IF_NOT(IsSameWorldTimeType,
        TEXT("WorldTimeTypes [{}] and [{}] do not match"),
        Get_TimeType(),
        InOther.Get_TimeType())
    {return false; }

    return Get_Time() < InOther.Get_Time();
}

auto
    FCk_Time_Unreal::
    operator==(
        const TimeType& InOther) const
    -> bool
{
    return Get_Time() == InOther;
}

auto
    FCk_Time_Unreal::
    operator<(
        const TimeType& InOther) const
    -> bool
{
    return Get_Time() < InOther;
}

auto
    FCk_Time_Unreal::
    operator!=(
        const TimeType& InOther) const
    -> bool
{
    return Get_Time() != InOther;
}

auto
    FCk_Time_Unreal::
    operator>(
        const TimeType& InOther) const
    -> bool
{
    return Get_Time() > InOther;
}

auto
    FCk_Time_Unreal::
    operator<=(
        const TimeType& InOther) const
    -> bool
{
    return Get_Time() <= InOther;
}

auto
    FCk_Time_Unreal::
    operator>=(
        const TimeType& InOther) const
    -> bool
{
    return Get_Time() >= InOther;
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
