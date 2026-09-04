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
        ck::TReadWrite<FFragment_GroundNavVolume_RepairState>,
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
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            FFragment_GroundNavVolume_BuildState& InBuildState,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            const FCk_Request_GroundNavVolume_Build& InRequest) -> void;

        // A build about to be armed supersedes any repair already sliced open: it re-bakes every tile
        // from live geometry, so the repair's remaining slices would spend probes on ground the build is
        // about to publish anyway - and would publish a field derived from the one the build replaces.
        // The callers riding it hear Failed_Cancelled rather than a verdict about ground this repair
        // will now never look at.
        static auto
        DoCancel_RepairInFlight(
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_RepairState& InRepairState) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Accumulates dirty world boxes onto the volume and arms a local repair for them. It opens nothing and
    // reads no geometry: a dirty box is a claim about ground, and StartRepair below is what turns an
    // accumulated claim into a repair.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_HandleRepairRequests : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_HandleRepairRequests,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_BuiltField>,
        ck::TReadWrite<FFragment_GroundNavVolume_RepairState>,
        ck::TReadWrite<FFragment_GroundNavVolume_RepairRequests>,
        TExclude<FTag_GroundNavVolume_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_GroundNavVolume_Setup>;
        using MarkedDirtyBy = FFragment_GroundNavVolume_RepairRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_RepairRequests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            const FCk_Request_GroundNavVolume_Repair& InRequest) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Admits area markup onto the volume and hands the change to the stage its kind owes - the cost
    // tag, or a dirty REGION for the local repair. It never bakes and never reads the field's cells:
    // admission is a decision about a RECORD.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_HandleMarkupRequests : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_HandleMarkupRequests,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_BuiltField>,
        ck::TReadWrite<FFragment_GroundNavVolume_RepairState>,
        ck::TReadWrite<FFragment_GroundNavVolume_Markup>,
        ck::TReadWrite<FFragment_GroundNavVolume_MarkupRequests>,
        TExclude<FTag_GroundNavVolume_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_GroundNavVolume_Setup>;
        using MarkedDirtyBy = FFragment_GroundNavVolume_MarkupRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_Markup& InMarkup,
            FFragment_GroundNavVolume_MarkupRequests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_Markup& InMarkup,
            const FCk_Request_GroundNavVolume_AreaMarkup& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_Markup& InMarkup,
            const FCk_Request_GroundNavVolume_ReleaseAreaMarkup& InRequest) -> void;

        // A COST change is a tag: the derive restamps every tile from the whole record list. A
        // WALKABILITY change is a BOX - it owes an actual re-bake of the ground the record covers, and
        // that ground is what the local repair takes. The caller passes the bounds of the record the
        // change is about, and both the old and the new bounds on a move: the footprint a record left
        // is ground nothing else will ever revisit.
        static auto
        DoMark_MarkupDirty(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            ECk_GroundNav_MarkupKind InKind,
            const FBox& InMarkupWorldBounds) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Admits authored links onto the volume and raises the one stage a link change owes. It never bakes
    // and never resolves an endpoint: admission is a decision about a RECORD, and where its two points
    // stand is the composition's separate answer.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_HandleLinkRequests : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_HandleLinkRequests,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_Params>,
        ck::TReadOnly<FFragment_GroundNavVolume_BuiltField>,
        ck::TReadWrite<FFragment_GroundNavVolume_Links>,
        ck::TReadWrite<FFragment_GroundNavVolume_LinkRequests>,
        TExclude<FTag_GroundNavVolume_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_TimeDelta;
        using RunAfter = TDepList<FProcessor_GroundNavVolume_Setup>;
        using MarkedDirtyBy = FFragment_GroundNavVolume_LinkRequests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_Links& InLinks,
            FFragment_GroundNavVolume_LinkRequests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_Links& InLinks,
            const FCk_Request_GroundNavVolume_Link& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_Links& InLinks,
            const FCk_Request_GroundNavVolume_ReleaseLink& InRequest) -> void;
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
        ck::TReadWrite<FFragment_GroundNavVolume_RepairState>,
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
            FFragment_GroundNavVolume_BuildState& InBuildState,
            FFragment_GroundNavVolume_RepairState& InRepairState) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // One budgeted slice of baking per tick, per volume.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_Build : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_Build,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_Params>,
        ck::TReadWrite<FFragment_GroundNavVolume_BuildState>,
        ck::TReadWrite<FFragment_GroundNavVolume_RepairState>,
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
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_BuiltField& InBuiltField) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Opens a local repair over the accumulated dirty region. Split from the repair slice for the same
    // reason StartBuild is split from Build: resolving the geometry backend and seeding from the published
    // field are the steps that can fail, and they must fail before any repair state exists to half-fill.
    //
    // A repair NEVER opens while a full build is armed or running. A build re-bakes every tile from live
    // geometry, so it answers every region pending when it STARTED - those ride it to its publish - and a
    // repair opened against a field the build is about to replace would publish over it. A region raised
    // after that snapshot opens its repair against the field the build publishes.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_StartRepair : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_StartRepair,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_BuiltField>,
        ck::TReadWrite<FFragment_GroundNavVolume_RepairState>,
        FTag_GroundNavVolume_NeedsRepair,
        TExclude<FTag_GroundNavVolume_NeedsBuild>,
        TExclude<FTag_GroundNavVolume_BuildInProgress>,
        TExclude<FTag_GroundNavVolume_RepairInProgress>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<
            FProcessor_JoltWorld_WaitForAsync,
            FProcessor_GroundNavVolume_StartBuild,
            FProcessor_GroundNavVolume_Build>;
        using RunBefore = TDepList<FProcessor_JoltWorld_Step>;
        using MarkedDirtyBy = FTag_GroundNavVolume_NeedsRepair;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_BuiltField& InBuiltField,
            FFragment_GroundNavVolume_RepairState& InRepairState) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // One budgeted slice of local repair per tick, per volume, and the publish that ends one. Declares no
    // MarkedDirtyBy for the same reason the build slice does not: the scheduler then runs it exactly once
    // per main tick and never pumps it, which is what makes a per-tick probe budget mean anything.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_Repair : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_Repair,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_Params>,
        ck::TReadWrite<FFragment_GroundNavVolume_RepairState>,
        ck::TReadWrite<FFragment_GroundNavVolume_BuiltField>,
        FTag_GroundNavVolume_RepairInProgress,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_WaitForAsync, FProcessor_GroundNavVolume_StartRepair>;
        using RunBefore = TDepList<FProcessor_JoltWorld_Step>;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Params& InParams,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            FFragment_GroundNavVolume_BuiltField& InBuiltField) const -> void;

    private:
        // Spends the repair state, drops the pinned physics session and reports the one outcome to every
        // caller riding it. Every path out of a slice that is not BudgetExhausted goes through here, so
        // there is exactly one place a repair can end and no path that ends one without reporting.
        static auto
        DoEnd(
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            ECk_Request_OperationResult InResult) -> void;

        // Puts a failed repair's region back so the ground it named is not lost, ONCE.
        static auto
        DoRetain_FailedRegion(
            HandleType InVolumeEntity,
            FFragment_GroundNavVolume_RepairState& InRepairState,
            const FBox& InFailedBounds) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Answers a COST markup change by deriving a new field from the published one and swapping the
    // pointer. It runs in the same window as the build's own publish so there is exactly one place per
    // tick where a volume's field changes hands, and it reads no geometry: cost is a plate label, and
    // restamping one spends no probes.
    //
    // A volume with nothing published yet has nothing to derive from and clears the tag regardless —
    // the records are already on the volume, so a build that STARTS after them bakes the price in
    // through its params. A build already in flight snapshotted its records before these arrived.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_MarkupCostDerive : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_MarkupCostDerive,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_Markup>,
        ck::TReadWrite<FFragment_GroundNavVolume_BuiltField>,
        FTag_GroundNavVolume_MarkupCostDirty,
        TExclude<FTag_GroundNavVolume_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_GroundNavVolume_Build, FProcessor_GroundNavVolume_Repair>;
        using MarkedDirtyBy = FTag_GroundNavVolume_MarkupCostDirty;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Markup& InMarkup,
            FFragment_GroundNavVolume_BuiltField& InBuiltField) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // Answers a LINK change by deriving a new field from the published one and swapping the pointer,
    // through the same publish the build and the cost derive use. It re-bakes nothing: a link is two
    // world points, and finding what they stand on reads cells that are already published, so no span,
    // no clearance and no plate of any tile can move under it. It is not the cost derive's pinned
    // ZERO-READ claim - it does read cells - but it spends no probe and never reaches the backend.
    //
    // Runs AFTER the cost derive by an explicit edge rather than by declaration order, because both are
    // owed in the same tick whenever one request changed a price and another changed a link: the cost
    // derive restamps every tile from the published field, and a link derive that ran first would hand
    // it a field whose links the restamp then re-resolved from the stale record list.
    //
    // A build or a repair that is RUNNING, and a repair that is armed, KEEP the tag: this runs again
    // in the tick their publish lands, after it. A volume with nothing published, or one a build is
    // armed for, clears the tag instead - that publish resolves the records from
    // FCk_GroundNav_FieldParams itself, which is the same wait the cost derive makes and the reason a
    // link authored before the first bake is never lost.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_LinkDerive : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_LinkDerive,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_Links>,
        ck::TReadWrite<FFragment_GroundNavVolume_BuiltField>,
        FTag_GroundNavVolume_LinksDirty,
        TExclude<FTag_GroundNavVolume_NeedsSetup>,
        TExclude<FTag_DestroyEntity_Initiate>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<
            FProcessor_GroundNavVolume_Build,
            FProcessor_GroundNavVolume_Repair,
            FProcessor_GroundNavVolume_MarkupCostDerive>;
        using MarkedDirtyBy = FTag_GroundNavVolume_LinksDirty;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InVolumeEntity,
            const FFragment_GroundNavVolume_Links& InLinks,
            FFragment_GroundNavVolume_BuiltField& InBuiltField) const -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // The build processors above exclude owners already tagged for destruction, so a destroyed volume's
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

    // ----------------------------------------------------------------------------------------------------------------

    // The repair processors above exclude owners already tagged for destruction, so a destroyed volume's
    // queued repair requests are never drained AND its in-flight repair never reports. This fires all
    // four populations as Failed_Cancelled, so a caller awaiting completion terminates instead of
    // hanging forever on a delegate that no longer has a processor to fire it.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_CancelPendingRepairRequests : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_CancelPendingRepairRequests,
        FCk_Handle_GroundNavVolume,
        ck::TReadWrite<FFragment_GroundNavVolume_RepairState>,
        ck::TReadOnly<FFragment_GroundNavVolume_RepairRequests>,
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
            FFragment_GroundNavVolume_RepairState& InRepairState,
            const FFragment_GroundNavVolume_RepairRequests& InRequests) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // The markup drain excludes owners already tagged for destruction, so a destroyed volume's queued
    // markup requests are never reached. This completes them Failed_Cancelled instead of leaving a
    // caller waiting on a delegate no processor will fire.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_CancelPendingMarkupRequests : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_CancelPendingMarkupRequests,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_MarkupRequests>,
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
            const FFragment_GroundNavVolume_MarkupRequests& InRequests) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // The link drain excludes owners already tagged for destruction, so a destroyed volume's queued link
    // requests are never reached. This completes them Failed_Cancelled instead of leaving a caller
    // waiting on a delegate no processor will fire.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_CancelPendingLinkRequests : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_CancelPendingLinkRequests,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_LinkRequests>,
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
            const FFragment_GroundNavVolume_LinkRequests& InRequests) -> void;
    };

    // ----------------------------------------------------------------------------------------------------------------

    // A volume enters the world-field registry at setup and is replaced there on every publish, so a
    // destroyed volume would keep answering the world's queries through its last field until the world
    // itself went away. This takes it out at end-play, the moment its field stops being anyone's ground.
    class CKGROUNDNAV_API FProcessor_GroundNavVolume_Unpublish : public ck_exp::TProcessor<
        FProcessor_GroundNavVolume_Unpublish,
        FCk_Handle_GroundNavVolume,
        ck::TReadOnly<FFragment_GroundNavVolume_BuiltField>,
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
            const FFragment_GroundNavVolume_BuiltField& InBuiltField) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
