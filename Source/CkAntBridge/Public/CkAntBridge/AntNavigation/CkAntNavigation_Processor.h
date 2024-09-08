#pragma once

#include "CkAntNavigation_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKANTBRIDGE_API FProcessor_AntNavigation_HandleRequests : public TProcessor<
            FProcessor_AntNavigation_HandleRequests,
            FFragment_AntNavigation_Current,
            FFragment_AntNavigation_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_AntNavigation_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AntNavigation_Current& InComp,
            FFragment_AntNavigation_Requests& InRequestsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKANTBRIDGE_API FProcessor_AntNavigation_Replicate : public TProcessor<
            FProcessor_AntNavigation_Replicate,
            FFragment_AntNavigation_Current,
            TObjectPtr<UCk_Fragment_AntNavigation_Rep>,
            FTag_AntNavigation_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_AntNavigation_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AntNavigation_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_AntNavigation_Rep>& InComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
