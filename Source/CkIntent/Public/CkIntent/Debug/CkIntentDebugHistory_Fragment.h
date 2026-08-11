#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkIntent/Debug/CkIntentDebugHistory_Fragment_Data.h"
#include "CkIntent/CkIntentSampler_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_IntentDebugHistory_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_IntentDebugHistory_Params = FCk_Fragment_IntentDebugHistory_ParamsData;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The retained rows, oldest first, plus the recording cursor.
     *
     * A flat append-and-trim array rather than a second ring: consumers read this as a straight timeline (oldest
     * to newest), the trim runs once per pass, and a debug-depth buffer has no business optimizing eviction at
     * the cost of every reader doing modular arithmetic.
     *
     * Capacity lives HERE rather than only in Params because it is runtime-mutable (the debugger's history
     * field); Params keeps the declared initial value.
     */
    struct CKINTENT_API FFragment_IntentDebugHistory_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_IntentDebugHistory_Current);

    public:
        friend class FProcessor_IntentDebugHistory_Record;
        friend class ::UCk_Utils_IntentDebugHistory_UE;

    private:
        TArray<FCk_Intent_FrameRecord> _Rows;

        int32 _Capacity = 0;

        // The newest sampler frame index already recorded — the dedup cursor that makes an unrated recorder
        // lose nothing and duplicate nothing at any frame rate.
        int32 _LastRecordedFrame = INDEX_NONE;

    public:
        CK_PROPERTY_GET(_Rows);
        CK_PROPERTY_GET(_Capacity);
    };
}

// --------------------------------------------------------------------------------------------------------------------
