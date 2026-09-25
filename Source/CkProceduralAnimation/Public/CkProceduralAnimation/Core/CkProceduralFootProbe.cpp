#include "CkProceduralAnimation/Core/CkProceduralFootProbe.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProceduralFootProbeState::
        Advance(
            bool InHit,
            FCk_Time InDeltaTime,
            FCk_Time InGraceDuration)
        -> bool
    {
        if (NOT FMath::IsFinite(InDeltaTime.Get_Seconds()) || InDeltaTime < FCk_Time{}
            || NOT FMath::IsFinite(InGraceDuration.Get_Seconds()) || InGraceDuration < FCk_Time{})
        { return false; }
        if (InDeltaTime == FCk_Time{})
        { return true; }
        if (InHit)
        {
            Reset();
            return true;
        }
        _MissingDuration = FMath::Min(_MissingDuration + InDeltaTime, InGraceDuration);
        _State = _MissingDuration >= InGraceDuration
            ? EProceduralFootProbeState::Lost : EProceduralFootProbeState::Guessing;
        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProceduralFootProbeState::
        Reset()
        -> void
    {
        _MissingDuration = FCk_Time{};
        _State = EProceduralFootProbeState::Grounded;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        MakeProceduralGroundProbeSpan(
            int32 InAttempt,
            float InUpDistance,
            float InDownDistance,
            float InOutwardLean)
        -> FProceduralGroundProbeSpan
    {
        constexpr float UpScale[ProceduralGroundProbeAttempts] = {1.0f, 4.0f, 12.0f};
        constexpr float DownScale[ProceduralGroundProbeAttempts] = {1.0f, 1.5f, 2.5f};
        const auto Attempt = FMath::Clamp(InAttempt, 0, ProceduralGroundProbeAttempts - 1);
        return FProceduralGroundProbeSpan{}
            .Set_UpDistance(InUpDistance * UpScale[Attempt])
            .Set_DownDistance(InDownDistance * DownScale[Attempt])
            .Set_OutwardLean(Attempt == 0 ? InOutwardLean : 0.0f);
    }
}

// --------------------------------------------------------------------------------------------------------------------
