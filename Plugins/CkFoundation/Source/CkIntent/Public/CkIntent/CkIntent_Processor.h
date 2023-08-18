#pragma once

#include "CkIntent_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

namespace ck
{
    class CKINTENT_API FCk_Processor_Intent_Setup
        : public TProcessor<FCk_Processor_Intent_Setup, FCk_Fragment_Intent_Params, FTag_Intent_Setup>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(FTimeType InDeltaT,
            FHandleType InHandle,
            FCk_Fragment_Intent_Params& InIntentParams);
    };
}
