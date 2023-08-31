#pragma once

#include "CkNetTimeSync_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

class UNetworkTimeSubsystem;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    // TODO: improve this system to avoid Ticking after getting the first TimeSync_Rep Fragment
    class CKNET_API FCk_Processor_TimeSync_OnNetworkClockSynchronized
        : public TProcessor<FCk_Processor_TimeSync_OnNetworkClockSynchronized, TObjectPtr<UCk_Fragment_TimeSync_Rep>>
    {
    public:
        using TProcessor::TProcessor;
        ~FCk_Processor_TimeSync_OnNetworkClockSynchronized();

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const TObjectPtr<UCk_Fragment_TimeSync_Rep>& InTimeSync_Rep);

    private:
        auto OnNetworkClockSynchronized(
            float OldServerDelta,
            float NewServerDelta,
            float RoundTripTime) -> void;

    private:
        FDelegateHandle _DelegateHandle;
        TWeakObjectPtr<UNetworkTimeSubsystem> _NetworkTimeSubsystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKNET_API FCk_Processor_TimeSync_HandleRequests
        : public TProcessor<FCk_Processor_TimeSync_HandleRequests, FFragment_TimeSync_Requests>
    {
    public:
        using MarkedDirtyBy = FFragment_TimeSync_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto Tick(TimeType InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_TimeSync_Requests& InRequests);
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------