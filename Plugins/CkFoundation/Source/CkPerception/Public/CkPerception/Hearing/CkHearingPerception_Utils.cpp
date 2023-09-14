#include "CkHearingPerception_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_HearingPerception_UE::
Get_NoiseEvent_IsEqual(
    const FCk_HearingPerception_NoiseEvent& InNoiseEventA,
    const FCk_HearingPerception_NoiseEvent& InNoiseEventB)
-> bool
{
    return InNoiseEventA == InNoiseEventB;
}

auto UCk_Utils_HearingPerception_UE::Get_NoiseEvent_IsNotEqual(
    const FCk_HearingPerception_NoiseEvent& InNoiseEventA,
    const FCk_HearingPerception_NoiseEvent& InNoiseEventB)
-> bool
{
    return InNoiseEventA != InNoiseEventB;
}

// --------------------------------------------------------------------------------------------------------------------
