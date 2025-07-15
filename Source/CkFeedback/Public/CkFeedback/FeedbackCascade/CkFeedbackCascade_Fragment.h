#pragma once

#include "CkFeedbackCascade_Fragment_Data.h"

#include "CkEcs/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_FeedbackCascade_Params = FCk_Fragment_FeedbackCascade_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfFeedbackCascade, FCk_Handle_FeedbackCascade);
}

// --------------------------------------------------------------------------------------------------------------------