#include "CkValueRange_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntRange_UE::
    Get_IsWithinRange(
        const int32 InValue,
        const FCk_IntRange& InRange,
        const ECk_Inclusiveness InInclusiveness)
    -> bool
{
    const auto& Range = FCk_FloatRange(InRange.Get_Min(), InRange.Get_Max(InInclusiveness));
    return UCk_Utils_FloatRange_UE::Get_IsWithinRange(InValue, Range, InInclusiveness);
}

auto
    UCk_Utils_IntRange_UE::
    Conv_IntRangeToVector2D(
        const FCk_IntRange& InRange)
    -> FVector2D
{
    return InRange.ToVector2D();
}

auto
    UCk_Utils_IntRange_UE::
    Get_RandomValueInRange(
        const FCk_IntRange& InRange)
    -> int32
{
    return InRange.Get_RandomValueInRange();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FloatRange_UE::
    Get_IsWithinRange(
        const float InValue,
        const FCk_FloatRange& InRange,
        ECk_Inclusiveness InInclusiveness)
    -> bool
{
    switch(InInclusiveness)
    {
        case ECk_Inclusiveness::Inclusive:
        {
            return InRange.Get_IsWithinInclusive(InValue);
        }
        case ECk_Inclusiveness::Exclusive:
        {
            return InRange.Get_IsWithinExclusive(InValue);
        }
        default:
        {
            CK_INVALID_ENUM(InInclusiveness);
            return false;
        }
    }
}

auto
    UCk_Utils_FloatRange_UE::
    Conv_FloatRangeToVector2D(
        const FCk_FloatRange& InRange)
    -> FVector2D
{
    return InRange.ToVector2D();
}

auto
    UCk_Utils_FloatRange_UE::
    Get_RandomValueInRange(
        const FCk_FloatRange& InRange)
    -> float
{
    return InRange.Get_RandomValueInRange();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FloatRange_0to1_UE::
    // ReSharper disable once CppInconsistentNaming
    Make_FloatRange_0to1(
        float In0to1)
    -> FCk_FloatRange_0to1
{
    return FCk_FloatRange_0to1{In0to1};
}

// --------------------------------------------------------------------------------------------------------------------
