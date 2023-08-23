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
        auto Tick(TimeType InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FTag_Intent_Setup&);

    private:
        TimeType _Delay = TimeType{5.0f};
    };

    class CKINTENT_API FCk_Processor_Intent_HandleRequests
        : public TProcessor<FCk_Processor_Intent_HandleRequests, FCk_Fragment_Intent_Params, FCk_Fragment_Intent_Requests>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FCk_Fragment_Intent_Params&,
            FCk_Fragment_Intent_Requests&);
    };
}
