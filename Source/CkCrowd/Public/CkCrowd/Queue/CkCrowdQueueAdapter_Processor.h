#pragma once

#include "CkCrowd/Agent/CkCrowdAgent_Fragment.h"
#include "CkCrowd/Agent/CkCrowdAgent_FaceAngle3D_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_FaceAngle_Processor.h"
#include "CkCrowd/Agent/CkCrowdAgent_HandleRequests_Processor.h"
#include "CkCrowd/Queue/CkCrowdQueueAdapter_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkQueue/Queue/CkQueue_Formation_Processor.h"

namespace ck
{
    class CKCROWD_API FProcessor_CrowdQueueAdapter_Dispatch : public ck_exp::TProcessor<
        FProcessor_CrowdQueueAdapter_Dispatch,
        FCk_Handle_CrowdAgent,
        ck::TReadWrite<FFragment_CrowdQueueAdapter>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_Queue_Formation>;
        using RunBefore = TDepList<FProcessor_CrowdAgent_HandleRequests>;
        using TProcessor::TProcessor;

        static auto ForEachEntity(TimeType InDeltaT, HandleType InAgent, FFragment_CrowdQueueAdapter& InAdapter) -> void;
    };

    class CKCROWD_API FProcessor_CrowdQueueAdapter_ObserveOutcome : public ck_exp::TProcessor<
        FProcessor_CrowdQueueAdapter_ObserveOutcome,
        FCk_Handle_CrowdAgent,
        ck::TReadWrite<FFragment_CrowdQueueAdapter>,
        TExclude<FTag_CrowdQueueAdapter_LeaveRequested>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<FProcessor_CrowdAgent_HandleRequests>;
        using TProcessor::TProcessor;

        static auto ForEachEntity(TimeType InDeltaT, HandleType InAgent, FFragment_CrowdQueueAdapter& InAdapter) -> void;
    };

    class CKCROWD_API FProcessor_CrowdQueueAdapter_MaintainFacing : public ck_exp::TProcessor<
        FProcessor_CrowdQueueAdapter_MaintainFacing,
        FCk_Handle_CrowdAgent,
        ck::TReadOnly<FFragment_CrowdQueueAdapter>,
        ck::TReadOnly<FFragment_CrowdAgent_FaceAngle>,
        TExclude<FTag_CrowdQueueAdapter_LeaveRequested>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform_SyncFrom;
        using RunAfter = TDepList<FProcessor_CrowdAgent_FaceAngle, FProcessor_CrowdAgent_FaceAngle3D>;
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;
        using TProcessor::TProcessor;

        static auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InAgent,
            const FFragment_CrowdQueueAdapter& InAdapter,
            const FFragment_CrowdAgent_FaceAngle& InFaceAngle) -> void;
    };

    class CKCROWD_API FProcessor_CrowdQueueAdapter_EndPlay : public ck_exp::TProcessor<
        FProcessor_CrowdQueueAdapter_EndPlay,
        FCk_Handle_CrowdAgent,
        ck::TReadWrite<FFragment_CrowdQueueAdapter>,
        CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using TProcessor::TProcessor;

        static auto ForEachEntity(TimeType InDeltaT, HandleType InAgent, FFragment_CrowdQueueAdapter& InAdapter) -> void;
    };
}
