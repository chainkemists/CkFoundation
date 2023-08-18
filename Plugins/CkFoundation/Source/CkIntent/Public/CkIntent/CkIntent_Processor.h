#pragma once

#include "CkIntent_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

namespace ck
{
    class CKINTENT_API FCk_Processor_Intent_Setup
        : public TProcessor<FCk_Processor_Intent_Setup, FTag_Intent_Setup>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto Tick(FTimeType InDeltaT) -> void;

        auto ForEachEntity(
            FTimeType InDeltaT,
            FHandleType InHandle,
            FTag_Intent_Setup&);

    private:
        FTimeType _Delay = FTimeType{5.0f};
    };

    class CKINTENT_API FCk_Processor_Intent_HandleRequests
        : public TProcessor<FCk_Processor_Intent_HandleRequests, FCk_Fragment_Intent_Params, FCk_Fragment_Intent_Requests>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            FTimeType InDeltaT,
            FHandleType InHandle,
            FCk_Fragment_Intent_Params&,
            FCk_Fragment_Intent_Requests&);
    };
}
