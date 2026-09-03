#include "CkGroundNavPath_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkGroundNav/CkGroundNav_Log.h"
#include "CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h"
#include "CkGroundNav/Search/CkGroundNav_PathPostProcess.h"

#include <Engine/World.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavPath_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavPath_Slice);
CK_REGISTER_PROCESSOR(ck::FProcessor_GroundNavPath_CancelPendingRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_path_processor
{
    using namespace ck::groundnav;

    // ----------------------------------------------------------------------------------------------------------------

    /** How many agents one tick may touch. Clamped to at least one: a cap of zero would park every
     *  episode forever, and a search nobody ever slices is a stall, not a saving. */
    static TAutoConsoleVariable<int32> CVarMaxSearchesPerFrame(
        TEXT("ck.GroundNav.MaxSearchesPerFrame"),
        8,
        TEXT("Agents whose ground-path search may be advanced in one tick. The rest wait a tick.\n")
        TEXT("Clamped to at least 1. Default 8."),
        ECVF_Default);

    /** Wall-clock the whole tick may spend searching, shared across those agents. There is deliberately
     *  NO unbounded setting: a per-frame slice budget that never runs out is the failure this processor
     *  exists to prevent. */
    static TAutoConsoleVariable<float> CVarSliceBudgetMs(
        TEXT("ck.GroundNav.SliceBudgetMs"),
        1.0f,
        TEXT("Milliseconds one tick may spend across ALL ground-path searches. Each agent is handed\n")
        TEXT("what the agents before it left. Clamped to at least 0.01ms. Default 1.0."),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVarMaxIterationsPerSlice(
        TEXT("ck.GroundNav.MaxIterationsPerSlice"),
        0,
        TEXT("Expansions one agent may make in one tick. Zero means as many as the time budget allows.\n")
        TEXT("Slicing changes nothing but where the work stops, so this never changes a verdict."),
        ECVF_Default);

    /** Mirrors ck.Nav.MaxDeferralSeconds. An episode parked on ground nobody has baked is worth waiting
     *  for, but not forever - past this it is failed with Unbuilt so the caller leaves Pending. */
    static TAutoConsoleVariable<float> CVarMaxDeferralSeconds(
        TEXT("ck.GroundNav.MaxDeferralSeconds"),
        5.0f,
        TEXT("Hard timeout for an episode parked on unbuilt ground. After this many seconds without a\n")
        TEXT("field to plan over, the request is force-failed with Unbuilt so the caller transitions\n")
        TEXT("out of Pending. Default 5s."),
        ECVF_Default);

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_CostParams(
            const ck::FFragment_GroundNavPath_Params& InParams)
        -> FCk_GroundNav_PathCostParams
    {
        auto Cost = FCk_GroundNav_PathCostParams{};

        Cost._SlopePenaltyK = InParams.Get_SlopePenaltyK();
        Cost._ClearanceBiasK = InParams.Get_ClearanceBiasK();
        Cost._CornerOffsetK = InParams.Get_CornerOffsetK();

        // The per-plate multiplier table is deliberately left empty: it is markup the field carries,
        // and an empty table prices every plate at one, which is the unmarked field.
        return Cost;
    }

    auto
        Get_Query(
            const ck::FFragment_GroundNavPath_Params& InParams,
            const FCk_Request_GroundNavPath_FindPath& InRequest)
        -> FCk_GroundNav_PathQuery
    {
        auto Query = FCk_GroundNav_PathQuery{};

        Query._Start = InRequest.Get_From();
        Query._Goal = InRequest.Get_Goal();
        Query._VerticalToleranceUu = InParams.Get_VerticalToleranceUu();
        Query._Agent._RadiusUu = InParams.Get_AgentRadiusUu();
        Query._Cost = Get_CostParams(InParams);
        Query._GreedyWeightW = InParams.Get_GreedyWeightW();
        Query._MaxExpansions = InParams.Get_MaxExpansions();
        Query._MaxCorridorLength = InParams.Get_MaxCorridorLength();
        Query._AllowPartialPath = InParams.Get_AllowPartialPath();

        return Query;
    }

    /** The agent location the post-process drops the first waypoint against is the request's own From:
     *  the path entity carries no transform of its own, and From is where the caller said the body was
     *  when it asked. */
    auto
        Get_PostParams(
            const ck::FFragment_GroundNavPath_Params& InParams,
            const FVector&                            InAgentLocation)
        -> FCk_GroundNav_PathPostParams
    {
        auto PostParams = FCk_GroundNav_PathPostParams{};

        PostParams._Agent._RadiusUu = InParams.Get_AgentRadiusUu();
        PostParams._VerticalToleranceUu = InParams.Get_VerticalToleranceUu();
        PostParams._AgentLocation = InAgentLocation;
        PostParams._Cost = Get_CostParams(InParams);

        return PostParams;
    }

    /** The corridor keyed by the ONE durable identity a crossing has. Node ids are per-search pool ids
     *  and mean nothing to a second search, so a corridor is stored as keys or it is not stored. */
    auto
        Get_CorridorKeys(
            const FCk_GroundNav_PathResult& InResult)
        -> TArray<FCk_GroundNav_CrossingKey>
    {
        auto Keys = TArray<FCk_GroundNav_CrossingKey>{};
        Keys.Reserve(InResult._Crossings.Num());

        for (const auto& Crossing : InResult._Crossings)
        {
            auto Key = FCk_GroundNav_CrossingKey{};

            Key._FromFlatPlate = Crossing._FromFlatPlate;
            Key._ToFlatPlate = Crossing._ToFlatPlate;
            Key._Direction = Crossing._Direction;
            Key._Left = Crossing._Left;
            Key._Right = Crossing._Right;

            Keys.Emplace(Key);
        }

        return Keys;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Drops the episode without touching the corridor keys: those outlive the episode that found them,
    // because they are what a later repair re-canonicalises against.
    auto
        FGroundNavPath_Episode::
        DoClear(
            FFragment_GroundNavPath_Current& InCurrent)
        -> void
    {
        InCurrent._Search = groundnav::FCk_GroundNav_PathSearch{};
        InCurrent._Field.Reset();
        InCurrent._PendingRequest = FCk_Request_GroundNavPath_FindPath{};
        InCurrent._HasBegun = false;
        InCurrent._PendingSince = FCk_Time{};
    }

    // The one line that proves this provider is alive: every published verdict, terminal or timed out.
    auto
        FGroundNavPath_Episode::
        DoLog_Published(
            FCk_Handle_GroundNavPath              InPathEntity,
            const FCk_GroundNavPath_Result&       InPublished)
        -> void
    {
        groundnav::Display(
            TEXT("GroundNav Path [{}] published [{}] rev [{}] waypoints [{}] expansions [{}]"),
            InPathEntity, InPublished.Get_Status(), InPublished.Get_RequestRevision(),
            InPublished.Get_Waypoints().Num(), InPublished.Get_ExpansionCount());
    }

    auto
        FGroundNavPath_Episode::
        DoPublish_Failure(
            FCk_Handle_GroundNavPath         InPathEntity,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result&  InResult,
            ECk_GroundNav_PathStatus         InStatus,
            int32                            InExpansionCount,
            int64                            InPlannedAgainstEpoch)
        -> void
    {
        const auto Request = InCurrent._PendingRequest;

        // Waypoints are cleared here and NOT at the install boundary: this module answers what its own
        // search found, and a failed search found nothing. What a consumer does with the route it was
        // already walking is the consumer's decision, made where the plan is installed.
        InResult._Result
            .Set_Status(InStatus)
            .Set_Waypoints({})
            .Set_RequestRevision(Request.Get_RequestRevision())
            .Set_LengthUu(0.0)
            .Set_ExpansionCount(InExpansionCount)
            .Set_PlannedAgainstEpoch(InPlannedAgainstEpoch);

        InResult._HasFreshResult = true;

        DoLog_Published(InPathEntity, InResult._Result);

        DoClear(InCurrent);
        InPathEntity.Try_Remove<FTag_GroundNavPath_SearchInFlight>();

        Request.TryFireCompletion(InPathEntity, ECk_Request_OperationResult::Failed);

        UUtils_Signal_OnGroundNavPathFailed::Broadcast(
            InPathEntity, MakePayload(InPathEntity, InStatus));
    }

    auto
        FGroundNavPath_Episode::
        DoPublish_Success(
            FCk_Handle_GroundNavPath              InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current&      InCurrent,
            FFragment_GroundNavPath_Result&       InResult)
        -> void
    {
        using namespace ck_groundnav_path_processor;

        const auto& SearchResult = InCurrent._Search.Get_Result();
        const auto Request = InCurrent._PendingRequest;

        const auto Plan = groundnav::Get_PathPlan(
            SearchResult,
            *InCurrent._Field,
            Get_PostParams(InParams, Request.Get_From()));

        auto Locations = TArray<FVector>{};
        Locations.Reserve(Plan._Waypoints.Num());

        for (const auto& Waypoint : Plan._Waypoints)
        { Locations.Emplace(Waypoint._Location); }

        InResult._Result
            .Set_Status(Plan._Status)
            .Set_Waypoints(Locations)
            .Set_RequestRevision(Request.Get_RequestRevision())
            .Set_LengthUu(Plan._LengthUu)
            .Set_ExpansionCount(SearchResult._ExpansionCount)
            .Set_PlannedAgainstEpoch(Plan._PlannedAgainstEpoch._Value);

        InResult._HasFreshResult = true;

        DoLog_Published(InPathEntity, InResult._Result);

        InCurrent._LastCorridorKeys = Get_CorridorKeys(SearchResult);
        InCurrent._LastSourceFlatPlate = SearchResult._PlateCorridor.IsEmpty()
            ? INDEX_NONE
            : SearchResult._PlateCorridor[0];

        DoClear(InCurrent);
        InPathEntity.Try_Remove<FTag_GroundNavPath_SearchInFlight>();

        Request.TryFireCompletion(InPathEntity, ECk_Request_OperationResult::Succeeded);

        UUtils_Signal_OnGroundNavPathReady::Broadcast(
            InPathEntity, MakePayload(InPathEntity));
    }

    auto
        FGroundNavPath_Episode::
        DoPublish_Terminal(
            FCk_Handle_GroundNavPath              InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current&      InCurrent,
            FFragment_GroundNavPath_Result&       InResult)
        -> void
    {
        const auto& SearchResult = InCurrent._Search.Get_Result();
        const auto Status = SearchResult._Status;

        if (Status == ECk_GroundNav_PathStatus::Ready || Status == ECk_GroundNav_PathStatus::Partial)
        {
            DoPublish_Success(InPathEntity, InParams, InCurrent, InResult);
            return;
        }

        groundnav::Verbose(TEXT("GroundNav Path [{}] found no route from [{}] to [{}]: [{}]"),
            InPathEntity, SearchResult._StartPoint, SearchResult._GoalPoint, Status);

        DoPublish_Failure(
            InPathEntity, InCurrent, InResult,
            Status, SearchResult._ExpansionCount, SearchResult._PlannedAgainstEpoch._Value);
    }

    /**
     * Stands a search up over whichever field covers the start, or leaves the episode parked.
     *
     * A world with no field near the start is treated exactly as Unbuilt rather than given a status of
     * its own: both mean ground nothing is known about, both are worth waiting for, and a consumer that
     * told them apart would have nothing different to do about either.
     */
    auto
        FGroundNavPath_Episode::
        DoTry_Begin(
            FCk_Handle_GroundNavPath              InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current&      InCurrent)
        -> void
    {
        using namespace ck_groundnav_path_processor;

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InPathEntity);

        const auto Field = groundnav::world_fields::TryGet_Field(
            World, InCurrent._PendingRequest.Get_From());

        if (NOT Field.IsValid())
        { return; }

        auto Search = groundnav::FCk_GroundNav_PathSearch{};

        const auto Status = Search.Request_Begin(
            Field, Get_Query(InParams, InCurrent._PendingRequest));

        // Ground the field itself has not baked. The episode stays parked and re-probes next tick,
        // because the volume covering it may still publish.
        if (Status == ECk_GroundNav_PathStatus::Unbuilt)
        { return; }

        InCurrent._Field = Field;
        InCurrent._Search = MoveTemp(Search);
        InCurrent._HasBegun = true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavPath_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult,
            FFragment_GroundNavPath_Requests& InRequests) const
        -> void
    {
        // Copied and reset BEFORE the drain, not reset after it: publishing a terminal verdict
        // broadcasts from inside this call, and a FindPath enqueued by a signal handler must not be
        // wiped by a reset that happens afterwards.
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        ck::algo::ForEachRequest(RequestsCopy, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InPathEntity, InParams, InCurrent, InResult, InRequest);
            }), policy::DontResetContainer{});

        // LOAD-BEARING, not bookkeeping. CkCrowd's OnGroundNavPathResolved reads a surviving queue as
        // proof that whatever the slot holds predates the request in flight, so a fragment left on the
        // entity makes every published result look stale forever and the agent never leaves Pending.
        if (InRequests._Requests.IsEmpty())
        {
            InPathEntity.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_GroundNavPath_HandleRequests::
        DoHandleRequest(
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult,
            const FCk_Request_GroundNavPath_FindPath& InRequest)
        -> void
    {
        // No completion guard here, unlike CkVoxelNav's synchronous search: the outcome is frames away,
        // so the delegate rides _PendingRequest and is fired by whoever publishes the terminal result.
        // A replaced episode strands whoever was waiting on it, so that one completes as cancelled.
        InCurrent._PendingRequest.TryFireCompletion(
            InPathEntity, ECk_Request_OperationResult::Failed_Cancelled);

        FGroundNavPath_Episode::DoClear(InCurrent);

        InResult._HasFreshResult = false;

        InCurrent._PendingRequest = InRequest;
        InCurrent._PendingSince = FCk_Time{FPlatformTime::Seconds()};

        InPathEntity.AddOrGet<FTag_GroundNavPath_SearchInFlight>();

        FGroundNavPath_Episode::DoTry_Begin(InPathEntity, InParams, InCurrent);

        if (InCurrent._HasBegun && InCurrent._Search.Get_IsTerminal())
        { FGroundNavPath_Episode::DoPublish_Terminal(InPathEntity, InParams, InCurrent, InResult); }
    }

    auto
        FProcessor_GroundNavPath_HandleRequests::
        DoHandleRequest(
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult,
            const FCk_Request_GroundNavPath_AbandonPath& InRequest)
        -> void
    {
        auto RequestResult = ECk_Request_OperationResult::Succeeded;
        const auto Guard = MakeCompletionGuard(InRequest, InPathEntity, RequestResult);

        InCurrent._PendingRequest.TryFireCompletion(
            InPathEntity, ECk_Request_OperationResult::Failed_Cancelled);

        FGroundNavPath_Episode::DoClear(InCurrent);

        InPathEntity.Try_Remove<FTag_GroundNavPath_SearchInFlight>();

        // The slot is returned to nothing-planned, which is what a cleared _HasFreshResult means. There
        // is no None in ECk_GroundNav_PathStatus and inventing one here would put a status on the wire
        // that no search can produce; the post-abandon revision is carried so a consumer can still date
        // the slot it is reading.
        InResult._Result
            .Set_Waypoints({})
            .Set_RequestRevision(InRequest.Get_RequestRevision());

        InResult._HasFreshResult = false;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavPath_Slice::
        DoTick(
            FCk_Time InDeltaT)
        -> void
    {
        using namespace ck_groundnav_path_processor;

        constexpr auto MinSliceMs = 0.01;

        _SearchesRemainingThisTick = FMath::Max(1, CVarMaxSearchesPerFrame.GetValueOnGameThread());

        _SliceRemainingThisTick = FCk_Time{
            FMath::Max(MinSliceMs, static_cast<double>(CVarSliceBudgetMs.GetValueOnGameThread()))
            / 1000.0};

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_GroundNavPath_Slice::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPathEntity,
            const FFragment_GroundNavPath_Params& InParams,
            FFragment_GroundNavPath_Current& InCurrent,
            FFragment_GroundNavPath_Result& InResult) const
        -> void
    {
        using namespace ck_groundnav_path_processor;

        // Both ceilings are read on EVERY entity and an exhausted one stops the pass. A budget that is
        // seeded and never consulted bounds nothing, which is the state CkNavigation's own per-frame
        // query cap is in.
        if (_SearchesRemainingThisTick <= 0 || _SliceRemainingThisTick.Get_Seconds() <= 0.0)
        { return; }

        --_SearchesRemainingThisTick;

        if (NOT InCurrent._HasBegun)
        {
            FGroundNavPath_Episode::DoTry_Begin(InPathEntity, InParams, InCurrent);

            if (NOT InCurrent._HasBegun)
            {
                const auto DeferredForSeconds =
                    FPlatformTime::Seconds() - InCurrent._PendingSince.Get_Seconds();

                const auto MaxDeferralSeconds =
                    static_cast<double>(CVarMaxDeferralSeconds.GetValueOnGameThread());

                if (DeferredForSeconds >= MaxDeferralSeconds)
                {
                    groundnav::Display(
                        TEXT("GroundNav Path [{}] deferral timed out after [{}]s with no field to plan ")
                        TEXT("over at [{}] - failing rev [{}] as Unbuilt"),
                        InPathEntity, DeferredForSeconds, InCurrent._PendingRequest.Get_From(),
                        InCurrent._PendingRequest.Get_RequestRevision());

                    constexpr auto NoExpansions = 0;
                    constexpr auto NoEpoch = int64{0};

                    FGroundNavPath_Episode::DoPublish_Failure(InPathEntity, InCurrent, InResult,
                        ECk_GroundNav_PathStatus::Unbuilt, NoExpansions, NoEpoch);
                }

                return;
            }

            if (InCurrent._Search.Get_IsTerminal())
            {
                FGroundNavPath_Episode::DoPublish_Terminal(InPathEntity, InParams, InCurrent, InResult);
                return;
            }
        }

        auto Slice = groundnav::FCk_GroundNav_PathSliceParams{};

        Slice._MaxIterations = FMath::Max(0, CVarMaxIterationsPerSlice.GetValueOnGameThread());
        Slice._Budget = _SliceRemainingThisTick;

        const auto SliceBeganAt = FPlatformTime::Seconds();

        InCurrent._Search.ContinueSearch(Slice);

        const auto SpentSeconds = FPlatformTime::Seconds() - SliceBeganAt;

        _SliceRemainingThisTick = FCk_Time{
            FMath::Max(0.0, _SliceRemainingThisTick.Get_Seconds() - SpentSeconds)};

        if (InCurrent._Search.Get_IsTerminal())
        { FGroundNavPath_Episode::DoPublish_Terminal(InPathEntity, InParams, InCurrent, InResult); }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_GroundNavPath_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InPathEntity,
            FFragment_GroundNavPath_Current& InCurrent,
            const FFragment_GroundNavPath_Requests& InRequests)
        -> void
    {
        // Two separate populations, and missing either one strands a caller. The QUEUE holds requests
        // the drain never reached; _PendingRequest holds the delegate that has been riding the
        // multi-frame search and is the one a caller is most likely actually waiting on.
        request::FireCancelledForPending(InPathEntity, InRequests.Get_Requests());

        InCurrent._PendingRequest.TryFireCompletion(
            InPathEntity, ECk_Request_OperationResult::Failed_Cancelled);

        // Drops the field snapshot with the entity rather than leaving it to fragment teardown.
        InCurrent._Field.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------
