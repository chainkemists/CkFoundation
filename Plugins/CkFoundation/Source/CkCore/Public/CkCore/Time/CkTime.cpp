#include "CkTime.h"

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
