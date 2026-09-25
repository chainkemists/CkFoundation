#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    enum class EProceduralFootProbeState : uint8
    {
        Grounded,
        Guessing,
        Lost
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralFootProbeState
    {
        CK_GENERATED_BODY(FProceduralFootProbeState);

    public:
        // Grace is elapsed simulation time, independent of the number of probe updates.
        // Zero delta preserves the state. Invalid durations return false without mutation.
        auto Advance(bool InHit, FCk_Time InDeltaTime, FCk_Time InGraceDuration) -> bool;
        auto Reset() -> void;

    private:
        FCk_Time _MissingDuration;
        EProceduralFootProbeState _State = EProceduralFootProbeState::Grounded;

    public:
        CK_PROPERTY_GET(_MissingDuration);
        CK_PROPERTY_GET(_State);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralGroundProbeSpan
    {
        CK_GENERATED_BODY(FProceduralGroundProbeSpan);

    private:
        float _UpDistance = 0.0f;
        float _DownDistance = 0.0f;
        float _OutwardLean = 0.0f;

    public:
        CK_PROPERTY(_UpDistance);
        CK_PROPERTY(_DownDistance);
        CK_PROPERTY(_OutwardLean);
    };

    // --------------------------------------------------------------------------------------------------------------------

    inline constexpr auto ProceduralGroundProbeAttempts = int32{3};

    CKPROCEDURALANIMATION_API auto
        MakeProceduralGroundProbeSpan(
            int32 InAttempt,
            float InUpDistance,
            float InDownDistance,
            float InOutwardLean)
        -> FProceduralGroundProbeSpan;
}

// --------------------------------------------------------------------------------------------------------------------
