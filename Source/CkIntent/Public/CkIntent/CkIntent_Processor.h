#pragma once

#include "CkIntent_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

namespace ck
{
    class CKINTENT_API FProcessor_Intent_Setup
        : public TProcessor<FProcessor_Intent_Setup, FTag_Intent_Setup>
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

    class CKINTENT_API FProcessor_Intent_HandleRequests
        : public TProcessor<FProcessor_Intent_HandleRequests, FFragment_Intent_Params, FFragment_Intent_Requests>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Intent_Params&,
            FFragment_Intent_Requests&);
    };
}
