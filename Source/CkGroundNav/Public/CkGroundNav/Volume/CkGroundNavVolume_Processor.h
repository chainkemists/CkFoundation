#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkJolt/World/CkJoltWorld_Processor.h"

#include "CkGroundNav/Volume/CkGroundNavVolume_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
// Scheduling note for the two Jolt-touching processors below: FGroup_Transform, after
// FProcessor_JoltWorld_WaitForAsync and before FProcessor_JoltWorld_Step, is the only window provably
// outside the async physics step — WaitForAsync consumes the previous frame's step and Step kicks the
// next. A processor pulling triangles out of the static world anywhere after Step would be reading it
// concurrently with the task-graph update whenever async physics is on.
//
// The build processor deliberately declares NO MarkedDirtyBy: the scheduler then runs it exactly once
// per main tick and never pumps it, which is what makes a per-tick budget mean anything.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Validates the authored params and, unless the volume opted out, arms the first build.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_Setup : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_Setup,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_Params>,
        FTag_GroundNavVolume_NeedsSetup,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using MarkedDirtyBy = FTag_GroundNavVolume_NeedsSetup;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    class CKGROUNDNAV_API FProcessor_GroundNavVolume_HandleRequests : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_HandleRequests,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_Params>,
        ck::TReadWrite<FFragment_GroundNavVolume_BuildState>,
        ck::TReadWrite<FFragment_GroundNavVolume_Requests>,
        TExclude<FTag_GroundNavVolume_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_GroundNavVolume_Setup>;
        using MarkedDirtyBy = FFragment_GroundNavVolume_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            FFragment_GroundNavVolume_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            const FCk_Request_GroundNavVolume_Build& InRequest) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Opens a build: resolves the geometry backend and sizes the field. Split from the slice below so the
    // backend is created exactly once per build rather than tested for on every tick of one.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_StartBuild : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_StartBuild,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_Params>,
        ck::TReadOnly<FFragment_GroundNavVolume_BuiltField>,
        ck::TReadWrite<FFragment_GroundNavVolume_BuildState>,
        FTag_GroundNavVolume_NeedsBuild,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_WaitForAsync>;
        using RunBefore = TDepList<FProcessor_JoltWorld_Step>;
        using MarkedDirtyBy = FTag_GroundNavVolume_NeedsBuild;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_BuildState& InBuildState) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // One budgeted slice of baking per tick, per volume.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_Build : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_Build,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_Params>,
        ck::TReadWrite<FFragment_GroundNavVolume_BuildState>,
        ck::TReadWrite<FFragment_GroundNavVolume_BuiltField>,
        FTag_GroundNavVolume_BuildInProgress,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_WaitForAsync, FProcessor_GroundNavVolume_StartBuild>;
        using RunBefore = TDepList<FProcessor_JoltWorld_Step>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            FFragment_GroundNavVolume_BuiltField& InBuiltField) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // The three processors above exclude owners already tagged for destruction, so a destroyed volume's
    // queued requests are never drained AND its in-flight build never reports. This fires both as
    // Failed_Cancelled, so a caller awaiting completion terminates instead of hanging forever on a
    // delegate that no longer has a processor to fire it.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_CancelPendingRequests : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_CancelPendingRequests,
        FCk_Handle_GroundNavVolume,
        ck::TReadWrite<FFragment_GroundNavVolume_BuildState>,
        ck::TReadOnly<FFragment_GroundNavVolume_Requests>,
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
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            const FFragment_GroundNavVolume_Requests& InRequests) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
