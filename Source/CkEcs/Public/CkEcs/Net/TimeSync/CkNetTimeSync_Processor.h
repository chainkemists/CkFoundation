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
    class CKNET_API FProcessor_NetTimeSync_OnNetworkClockSynchronized
        : public TProcessor<FProcessor_NetTimeSync_OnNetworkClockSynchronized, TObjectPtr<UCk_Fragment_NetTimeSync_Rep>>
    {
    public:
        using TProcessor::TProcessor;
        ~FProcessor_NetTimeSync_OnNetworkClockSynchronized();

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const TObjectPtr<UCk_Fragment_NetTimeSync_Rep>& InTimeSync_Rep) -> void;

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

    class CKNET_API FProcessor_NetTimeSync_HandleRequests
        : public TProcessor<FProcessor_NetTimeSync_HandleRequests, FFragment_NetTimeSync_Requests>
    {
    public:
        using MarkedDirtyBy = FFragment_NetTimeSync_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(TimeType InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_NetTimeSync_Requests& InRequests) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKNET_API FProcessor_NetTimeSync_FirstSync
        : public TProcessor<FProcessor_NetTimeSync_FirstSync, FFragment_NetTimeSync, TObjectPtr<UCk_Fragment_NetTimeSync_Rep>>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_NetTimeSync& InTimeToSyncFrom,
            const TObjectPtr<UCk_Fragment_NetTimeSync_Rep>&) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------