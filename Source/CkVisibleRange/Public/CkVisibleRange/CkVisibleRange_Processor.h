#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_CadenceBuckets.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkVisibleRange/CkVisibleRange_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <int32 T_BucketIndex>
    class FProcessor_VisibleRange_Update_Bucket : public TCadenceBucketProcessor<
        FProcessor_VisibleRange_Update_Bucket<T_BucketIndex>,
        FProcessor_VisibleRange_Update_Bucket,
        T_BucketIndex,
        FCk_Handle_VisibleRange,
        ck::TReadOnly<FFragment_VisibleRange_Params>,
        ck::TReadWrite<FFragment_VisibleRange_Current>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;

    public:
        // Dependent-base ctor inheritance: unqualified lookup never examines a dependent base, so the
        // base is re-derived through the current instantiation (the `typename Self::Base` idiom).
        using BucketBase = typename FProcessor_VisibleRange_Update_Bucket::TCadenceBucketProcessor;
        using BucketBase::BucketBase;

    public:
        // Concrete types on purpose: the dependent base's TimeType/HandleType aliases are not visible
        // unqualified inside a class template.
        static auto
        ForEachEntity(
            FCk_Time InDeltaT,
            FCk_Handle_VisibleRange InHandle,
            const FFragment_VisibleRange_Params& InParams,
            FFragment_VisibleRange_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKVISIBLERANGE_API FProcessor_VisibleRange_HandleRequests
        : public ck_exp::TProcessor<FProcessor_VisibleRange_HandleRequests, FCk_Handle_VisibleRange,
            ck::TReadWrite<FFragment_VisibleRange_Current>, ck::TReadWrite<FFragment_VisibleRange_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TCadenceBucketDepList<FProcessor_VisibleRange_Update_Bucket>;
        using MarkedDirtyBy = FFragment_VisibleRange_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_VisibleRange_Current& InCurrent,
            FFragment_VisibleRange_Requests& InRequests) const
            -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisibleRange_Current& InCurrent,
            const FCk_Request_VisibleRange_ApplyRangeState& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_VisibleRange_Current& InCurrent,
            const FCk_Request_VisibleRange_SetVisibility& InRequest) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
