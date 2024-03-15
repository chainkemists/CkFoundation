#pragma once

#include "CkEcsTemplate_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECSTEMPLATE_API FProcessor_EcsTemplate_HandleRequests : public TProcessor<
            FProcessor_EcsTemplate_HandleRequests,
            FFragment_EcsTemplate_Current,
            FFragment_EcsTemplate_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_EcsTemplate_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(
            TimeType InDeltaT) -> void;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EcsTemplate_Current& InComp,
            FFragment_EcsTemplate_Requests& InRequestsComp) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKECSTEMPLATE_API FProcessor_EcsTemplate_Replicate : public TProcessor<
            FProcessor_EcsTemplate_Replicate,
            FFragment_EcsTemplate_Current,
            TObjectPtr<UCk_Fragment_EcsTemplate_Rep>,
            FTag_EcsTemplate_Updated,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_EcsTemplate_Updated;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_EcsTemplate_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_EcsTemplate_Rep>& InComp) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
