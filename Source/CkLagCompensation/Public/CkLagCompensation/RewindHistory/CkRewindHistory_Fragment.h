#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkLagCompensation/RewindHistory/CkRewindHistory_Fragment_Data.h"
#include "CkLagCompensation/RewindHistory/CkRewindHistory_Query.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_RewindHistory_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Records a frame on the next record pass regardless of the record interval.
    // Used by tests and by callers that need a guaranteed sample this frame
    CK_DEFINE_ECS_TAG(FTag_RewindHistory_ForceRecord);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_RewindHistory_Params = FCk_RewindHistory_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKLAGCOMPENSATION_API FFragment_RewindHistory_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_RewindHistory_Current);

    public:
        friend class FProcessor_RewindHistory_Record;
        friend class ::UCk_Utils_RewindHistory_UE;

    private:
        FCk_LagComp_FrameBuffer _Frames;
        FCk_Time _LastRecordTime;

    public:
        CK_PROPERTY_GET(_Frames);
        CK_PROPERTY_GET(_LastRecordTime);
    };
}

// --------------------------------------------------------------------------------------------------------------------
