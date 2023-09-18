#include "CkValueRange.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_FloatRange_0to1::
    FCk_FloatRange_0to1(
        float InValue)
    : _Value(InValue)
{
    CK_ENSURE
    (
        InValue >= 0.0f && InValue <= 1.0f,
        TEXT("InValue [{}] is NOT between 0...1)"),
        InValue
    );
}

// --------------------------------------------------------------------------------------------------------------------

FCk_FloatRange_Minus1to1::
    FCk_FloatRange_Minus1to1(
        float InValue)
    : _Value(InValue)
{
    CK_ENSURE
    (
        InValue >= -1.0f && InValue <= 1.0f,
        TEXT("InValue [{}] is NOT between -1...+1)"),
        InValue
    );
}

// --------------------------------------------------------------------------------------------------------------------
