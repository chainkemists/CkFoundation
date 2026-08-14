#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkIntent/CkIntentSampler_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::intent_sampler
{
    // The geometry ECk_Intent_Octant encodes. Named rather than spelled at each site because the validation
    // boundary and the derivation have to agree on them exactly: a margin is rejected against the half-octant,
    // and the same half-octant is what an octant's hold band is built from.
    inline constexpr auto OctantCount = 8;
    inline constexpr auto OctantWidthDegrees = 45.0f;
    inline constexpr auto HalfOctantDegrees = OctantWidthDegrees / 2.0f;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_IntentSampler_Params = FCk_Fragment_IntentSampler_ParamsData;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * Routed events waiting for a logic frame to claim them.
     *
     * This buffer is what makes the record independent of the frame rate. The router's retention is cleared at
     * the top of every RENDER frame, while the sampler runs on a fixed 60 Hz cadence — so without a buffer the
     * two disagree in both directions: above 60 fps the sampler skips render frames and their events are gone
     * before it looks, and below 60 fps its replayed catch-up ticks re-read the same retention and record the
     * same press several times. An unrated collector appends every render frame's rows here; the rated sampler
     * takes ALL of them and clears.
     *
     * Consume-clears is the whole contract. It is what makes the second and later ticks of a catch-up burst find
     * an empty buffer, which is why a hitch cannot duplicate an edge.
     */
    struct CKINTENT_API FFragment_IntentSampler_PendingEvents
    {
    public:
        CK_GENERATED_BODY(FFragment_IntentSampler_PendingEvents);

    public:
        friend class FProcessor_IntentSampler_Collect;
        friend class FProcessor_IntentSampler_Sample;

    private:
        TArray<FCk_InputLayer_RoutedEvent> _Pending;

    public:
        CK_PROPERTY_GET(_Pending);
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The ring and the state the next row is derived from.
     *
     * Rows live in one flat array written circularly rather than in a queue that shifts: a row's storage slot is
     * meaningless to consumers (they address by OFFSET FROM THE LATEST), and overwrite-oldest is the only
     * eviction policy that keeps a fixed-cost record honest under a player who never stops pressing things.
     *
     * The held set is kept as KEYS, not ButtonIds. Held-ness is a physical fact about a device, and resolving it
     * to identities at row-write time means a rebind that lands mid-hold is reflected in the very next row
     * instead of leaving the record naming a button the key no longer produces.
     *
     * The last axis values are carried across frames because an axis at rest sends no events: a stick held at
     * full deflection produces one analog row and then silence, and a record that reset to zero on the silent
     * frames would describe a stick being flicked sixty times a second.
     *
     * The last octant and the cardinal press order are the two pieces of state the derived readings need and the
     * row cannot hold. Hysteresis is a statement about the PREVIOUS reading, and SOCD's priority policies are
     * statements about which of two held buttons went down first — neither is recoverable from a row in
     * isolation, which is precisely why they are derived here once rather than by every consumer.
     */
    struct CKINTENT_API FFragment_IntentSampler_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_IntentSampler_Current);

    public:
        friend class FProcessor_IntentSampler_Sample;

    private:
        TArray<FCk_Intent_FrameRecord> _Rows;

        int32 _NextWriteIndex = 0;

        int32 _RowCount = 0;

        int32 _NextFrameIndex = 0;

        TArray<FKey> _HeldKeys;

        float _LastAxisX = 0.0f;

        float _LastAxisY = 0.0f;

        ECk_Intent_Octant _LastOctant = ECk_Intent_Octant::Neutral;

        // The cardinal buttons currently down, oldest press first. An ordered list rather than per-button
        // timestamps because both priority policies only ever ask which of two entries came first, and a
        // re-press is expressed by moving the entry to the end instead of by rewriting a number.
        TArray<FCk_Input_ButtonId> _SocdPressOrder;

    public:
        CK_PROPERTY_GET(_Rows);
        CK_PROPERTY_GET(_NextWriteIndex);
        CK_PROPERTY_GET(_RowCount);
        CK_PROPERTY_GET(_HeldKeys);
    };
}

// --------------------------------------------------------------------------------------------------------------------
