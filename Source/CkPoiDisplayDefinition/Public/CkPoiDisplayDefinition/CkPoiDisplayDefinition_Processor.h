#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkPoiDisplayDefinition/CkPoiDisplayDefinition_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPOIDISPLAYDEFINITION_API FProcessor_PoiDisplayDefinition_HandleRequests
        : public ck_exp::TProcessor<FProcessor_PoiDisplayDefinition_HandleRequests, FCk_Handle_PoiDisplayDefinition,
            ck::TReadWrite<FFragment_PoiDisplayDefinition_Current>, ck::TReadWrite<FFragment_PoiDisplayDefinition_Requests>,
            TExclude<FTag_DestroyEntity_Initiate>, CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FFragment_PoiDisplayDefinition_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_PoiDisplayDefinition_Current& InCurrent,
            FFragment_PoiDisplayDefinition_Requests& InRequests) const
            -> void;

    private:
        // Each returns whether it actually mutated Current, so the drain can broadcast ONCE for a batch
        // (and not at all for a no-op re-assert of the current value).
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_PoiDisplayDefinition_Current& InCurrent,
            const FCk_Request_PoiDisplayDefinition_SetTint& InRequest) -> bool;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_PoiDisplayDefinition_Current& InCurrent,
            const FCk_Request_PoiDisplayDefinition_SetSizeHint& InRequest) -> bool;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPOIDISPLAYDEFINITION_API FProcessor_PoiDisplayDefinition_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_PoiDisplayDefinition_CancelPendingRequests,
        FCk_Handle_PoiDisplayDefinition,
        ck::TReadOnly<FFragment_PoiDisplayDefinition_Requests>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PoiDisplayDefinition_Requests& InRequestsComp)
            -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
