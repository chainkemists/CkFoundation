#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Per-agent episode state for the ck.Crowd.DiagNavClip diagnostic. Added lazily by
    // FProcessor_CrowdAgent_DiagNavClip the first time the CVar is on, so an agent pays nothing
    // while the diagnostic is off. Diagnostic-only: no processor reads it for behaviour.
    struct CKCROWD_API FFragment_CrowdAgent_DiagNavClip
    {
    public:
        CK_GENERATED_BODY(FFragment_CrowdAgent_DiagNavClip);

        friend class FProcessor_CrowdAgent_DiagNavClip;

    private:
        // Consecutive clipped seconds. An episode opens once this reaches the open threshold, and
        // any unclipped frame resets it.
        float _ClippedSeconds = 0.0f;
        float _EpisodeSeconds = 0.0f;
        float _SecondsSinceHoldLog = 0.0f;
        int32 _EpisodeCount = 0;
        bool _EpisodeOpen = false;

        // The agent's position at the previous sample, so the diagnostic can compare the
        // displacement STAGED this frame against the one that ACTUALLY landed since the last one.
        // It is therefore one frame stale relative to this frame's staging — irrelevant at the
        // half-second granularity an episode needs.
        FVector _LastPosition = FVector::ZeroVector;
        bool _HasLastPosition = false;

    public:
        CK_PROPERTY_GET(_EpisodeCount);
        CK_PROPERTY_GET(_EpisodeOpen);
    };
}

// --------------------------------------------------------------------------------------------------------------------
