#include "CkFogOfWar_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkMinimap/CkMinimap_Log.h"
#include "CkMinimap/CkFogOfWar_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("CkFogOfWar"), STATGROUP_CkFogOfWar, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("FogOfWar::Reveal"), STAT_CkFogOfWar_Reveal, STATGROUP_CkFogOfWar);

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_FogOfWar_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_FogOfWar_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_FogOfWar_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_FogOfWar_Update);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_fog_of_war_processor
{
    constexpr int64 MaxTotalCells = 1048576;

    auto
        DoStampCircle(
            const FCk_Minimap_WorldBounds& InBounds,
            float InCellSize,
            const FIntPoint& InCellCounts,
            TBitArray<>& InOutExplored,
            const FVector& InCenter,
            float InRadius,
            TArray<int32>& InOutNewlyRevealed)
        -> void
    {
        if (InCellCounts.X <= 0 || InCellCounts.Y <= 0)
        { return; }

        const auto ContainingCell = UCk_Utils_FogOfWar_UE::Get_CellIndex(InCenter, InBounds, InCellSize, InCellCounts);

        if (ContainingCell != INDEX_NONE && NOT InOutExplored[ContainingCell])
        {
            InOutExplored[ContainingCell] = true;
            InOutNewlyRevealed.Add(ContainingCell);
        }

        const auto BoundsMin = InBounds.Get_Center() - InBounds.Get_HalfExtents();
        const auto CenterXY = FVector2D{InCenter.X, InCenter.Y};

        const auto CoordMinX = FMath::Clamp(FMath::FloorToInt32((CenterXY.X - InRadius - BoundsMin.X) / InCellSize), 0, InCellCounts.X - 1);
        const auto CoordMaxX = FMath::Clamp(FMath::FloorToInt32((CenterXY.X + InRadius - BoundsMin.X) / InCellSize), 0, InCellCounts.X - 1);
        const auto CoordMinY = FMath::Clamp(FMath::FloorToInt32((CenterXY.Y - InRadius - BoundsMin.Y) / InCellSize), 0, InCellCounts.Y - 1);
        const auto CoordMaxY = FMath::Clamp(FMath::FloorToInt32((CenterXY.Y + InRadius - BoundsMin.Y) / InCellSize), 0, InCellCounts.Y - 1);

        const auto RadiusSquared = InRadius * InRadius;

        for (auto CellY = CoordMinY; CellY <= CoordMaxY; ++CellY)
        {
            for (auto CellX = CoordMinX; CellX <= CoordMaxX; ++CellX)
            {
                const auto CellCenter = UCk_Utils_FogOfWar_UE::Get_CellCenter(CellX, CellY, InBounds, InCellSize);

                if (FVector2D::DistSquared(CellCenter, CenterXY) > RadiusSquared)
                { continue; }

                const auto CellIndex = CellY * InCellCounts.X + CellX;

                if (InOutExplored[CellIndex])
                { continue; }

                InOutExplored[CellIndex] = true;
                InOutNewlyRevealed.Add(CellIndex);
            }
        }
    }

    // Takes the scratch array, not the fragment: this file-local helper is not a friend of it — its callers are.
    auto
        DoFlushRevealedBatch(
            FCk_Handle_FogOfWar& InFogEntity,
            TArray<int32>& InOutNewlyRevealed)
        -> void
    {
        if (InOutNewlyRevealed.IsEmpty())
        { return; }

        ck::UUtils_Signal_OnFogOfWarCellsRevealed::Broadcast(InFogEntity,
            ck::MakePayload(InFogEntity, FCk_FogOfWar_RevealedCells{InOutNewlyRevealed}));

        InOutNewlyRevealed.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_FogOfWar_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InFogEntity,
            const FFragment_FogOfWar_Params& InParams,
            FFragment_FogOfWar& InFogOfWar)
        -> void
    {
        InFogEntity.Remove<MarkedDirtyBy>();

        CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Bounds()),
            TEXT("FogOfWar [{}] has INVALID Bounds (both half-extents must be > 0). The grid stays unallocated — "
                 "every location will query as explored (unfogged)"), InFogEntity)
        { return; }

        const auto CellCounts = UCk_Utils_FogOfWar_UE::Get_CellCounts(InParams.Get_Bounds(), InParams.Get_CellSize());
        const auto TotalCells = static_cast<int64>(CellCounts.X) * static_cast<int64>(CellCounts.Y);

        CK_ENSURE_IF_NOT(TotalCells <= ck_fog_of_war_processor::MaxTotalCells,
            TEXT("FogOfWar [{}] grid of [{}x{}] cells exceeds the [{}] cell budget — increase CellSize or shrink "
                 "the Bounds. The grid stays unallocated (unfogged)"),
            InFogEntity, CellCounts.X, CellCounts.Y, ck_fog_of_war_processor::MaxTotalCells)
        { return; }

        InFogOfWar._Explored.Init(false, static_cast<int32>(TotalCells));
        InFogOfWar._CellCounts = CellCounts;

        const auto RunUpdateImmediately = FCk_Time{TNumericLimits<double>::Max()};
        InFogOfWar._TimeSinceUpdate = RunUpdateImmediately;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_FogOfWar_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InFogEntity,
            FFragment_FogOfWar& InFogOfWar,
            const FFragment_FogOfWar_Params& InParams,
            FFragment_FogOfWar_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            using T = std::decay_t<decltype(InRequest)>;

            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InFogEntity, Result);

            // AddRevealer / SetExplored have a genuine ensure-gated failure path; every other
            // DoHandleRequest overload is void with no rejection, so reaching the line after the
            // call IS the success condition.
            if constexpr (std::is_same_v<T, FCk_Request_FogOfWar_AddRevealer> ||
                std::is_same_v<T, FCk_Request_FogOfWar_SetExplored>)
            {
                if (DoHandleRequest(InFogEntity, InFogOfWar, InParams, InRequest))
                { Result = ECk_Request_OperationResult::Succeeded; }
            }
            else
            {
                DoHandleRequest(InFogEntity, InFogOfWar, InParams, InRequest);
                Result = ECk_Request_OperationResult::Succeeded;
            }

            if (InRequest.Get_IsRequestHandleValid())
            {
                InRequest.GetAndDestroyRequestHandle();
            }
        }), policy::DontResetContainer{});

        ck_fog_of_war_processor::DoFlushRevealedBatch(InFogEntity, InFogOfWar._NewlyRevealedScratch);

        if (InRequests._Requests.IsEmpty())
        {
            InFogEntity.Remove<MarkedDirtyBy>();
        }
    }

    auto
        FProcessor_FogOfWar_HandleRequests::
        DoHandleRequest(
            HandleType InFogEntity,
            FFragment_FogOfWar& InFogOfWar,
            const FFragment_FogOfWar_Params& InParams,
            const FCk_Request_FogOfWar_AddRevealer& InRequest)
        -> bool
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InRequest.Get_Revealer()),
            TEXT("AddRevealer on FogOfWar [{}] received an INVALID Revealer handle"), InFogEntity)
        { return false; }

        minimap::VeryVerbose(TEXT("Handling AddRevealer [{}] Request for FogOfWar with Entity [{}]"),
            InRequest.Get_Revealer(), InFogEntity);

        InFogOfWar._Revealers.AddUnique(InRequest.Get_Revealer());

        return true;
    }

    auto
        FProcessor_FogOfWar_HandleRequests::
        DoHandleRequest(
            HandleType InFogEntity,
            FFragment_FogOfWar& InFogOfWar,
            const FFragment_FogOfWar_Params& InParams,
            const FCk_Request_FogOfWar_RemoveRevealer& InRequest)
        -> void
    {
        minimap::VeryVerbose(TEXT("Handling RemoveRevealer [{}] Request for FogOfWar with Entity [{}]"),
            InRequest.Get_Revealer(), InFogEntity);

        InFogOfWar._Revealers.Remove(InRequest.Get_Revealer());
    }

    auto
        FProcessor_FogOfWar_HandleRequests::
        DoHandleRequest(
            HandleType InFogEntity,
            FFragment_FogOfWar& InFogOfWar,
            const FFragment_FogOfWar_Params& InParams,
            const FCk_Request_FogOfWar_RevealLocation& InRequest)
        -> void
    {
        minimap::VeryVerbose(TEXT("Handling RevealLocation Request for FogOfWar with Entity [{}]"), InFogEntity);

        const auto Radius = InRequest.Get_Radius() > 0.0f
            ? InRequest.Get_Radius()
            : InParams.Get_RevealRadius();

        ck_fog_of_war_processor::DoStampCircle(InParams.Get_Bounds(), InParams.Get_CellSize(),
            InFogOfWar._CellCounts, InFogOfWar._Explored, InRequest.Get_Location(), Radius,
            InFogOfWar._NewlyRevealedScratch);
    }

    auto
        FProcessor_FogOfWar_HandleRequests::
        DoHandleRequest(
            HandleType InFogEntity,
            FFragment_FogOfWar& InFogOfWar,
            const FFragment_FogOfWar_Params& InParams,
            const FCk_Request_FogOfWar_RevealAll& InRequest)
        -> void
    {
        if (InFogOfWar._Explored.IsEmpty())
        { return; }

        minimap::VeryVerbose(TEXT("Handling RevealAll Request for FogOfWar with Entity [{}]"), InFogEntity);

        for (auto CellIndex = 0; CellIndex < InFogOfWar._Explored.Num(); ++CellIndex)
        {
            if (InFogOfWar._Explored[CellIndex])
            { continue; }

            InFogOfWar._NewlyRevealedScratch.Add(CellIndex);
        }

        InFogOfWar._Explored.SetRange(0, InFogOfWar._Explored.Num(), true);
    }

    auto
        FProcessor_FogOfWar_HandleRequests::
        DoHandleRequest(
            HandleType InFogEntity,
            FFragment_FogOfWar& InFogOfWar,
            const FFragment_FogOfWar_Params& InParams,
            const FCk_Request_FogOfWar_Reset& InRequest)
        -> void
    {
        const auto NothingToReset = InFogOfWar._Explored.IsEmpty() || InFogOfWar._Explored.CountSetBits() == 0;

        if (NothingToReset)
        { return; }

        minimap::VeryVerbose(TEXT("Handling Reset Request for FogOfWar with Entity [{}]"), InFogEntity);

        InFogOfWar._Explored.Init(false, InFogOfWar._Explored.Num());

        InFogOfWar._NewlyRevealedScratch.Reset();

        UUtils_Signal_OnFogOfWarReset::Broadcast(InFogEntity, MakePayload(InFogEntity));
    }

    auto
        FProcessor_FogOfWar_HandleRequests::
        DoHandleRequest(
            HandleType InFogEntity,
            FFragment_FogOfWar& InFogOfWar,
            const FFragment_FogOfWar_Params& InParams,
            const FCk_Request_FogOfWar_SetExplored& InRequest)
        -> bool
    {
        const auto& Payload = InRequest.Get_ExploredData();

        CK_ENSURE_IF_NOT(Payload.Get_CellCountX() == InFogOfWar._CellCounts.X &&
            Payload.Get_CellCountY() == InFogOfWar._CellCounts.Y,
            TEXT("SetExplored on FogOfWar [{}] carries a [{}x{}] grid but the live grid is [{}x{}] — the map or "
                 "its Bounds/CellSize changed since this data was captured. Dropping the restore, keeping the "
                 "fresh grid"),
            InFogEntity, Payload.Get_CellCountX(), Payload.Get_CellCountY(),
            InFogOfWar._CellCounts.X, InFogOfWar._CellCounts.Y)
        { return false; }

        minimap::VeryVerbose(TEXT("Handling SetExplored Request for FogOfWar with Entity [{}]"), InFogEntity);

        const auto& PackedCells = Payload.Get_PackedCells();

        for (auto CellIndex = 0; CellIndex < InFogOfWar._Explored.Num(); ++CellIndex)
        {
            const auto ByteIndex = CellIndex / 8;

            if (NOT PackedCells.IsValidIndex(ByteIndex))
            { break; }

            const auto IsSetInPayload = (PackedCells[ByteIndex] & (1 << (CellIndex % 8))) != 0;

            if (NOT IsSetInPayload || InFogOfWar._Explored[CellIndex])
            { continue; }

            InFogOfWar._Explored[CellIndex] = true;
            InFogOfWar._NewlyRevealedScratch.Add(CellIndex);
        }

        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_FogOfWar_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InFogEntity,
            const FFragment_FogOfWar_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InFogEntity, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_FogOfWar_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InFogEntity,
            const FFragment_FogOfWar_Params& InParams,
            FFragment_FogOfWar& InFogOfWar) const
        -> void
    {
        if (InFogOfWar._Explored.IsEmpty())
        { return; }

        InFogOfWar._TimeSinceUpdate += InDeltaT;

        const auto UpdateInterval = InParams.Get_UpdateInterval();

        if (UpdateInterval > FCk_Time::ZeroSecond() && InFogOfWar._TimeSinceUpdate < UpdateInterval)
        { return; }

        InFogOfWar._TimeSinceUpdate = FCk_Time::ZeroSecond();

        {
            SCOPE_CYCLE_COUNTER(STAT_CkFogOfWar_Reveal);

            // Revealers die (pawn destroyed, possession changed) as part of normal play — prune silently
            InFogOfWar._Revealers.RemoveAll([](const FCk_Handle& InRevealer)
            {
                return ck::Is_NOT_Valid(InRevealer);
            });

            for (const auto& Revealer : InFogOfWar._Revealers)
            {
                const auto RevealerTransform = UCk_Utils_Transform_UE::Cast(Revealer);

                if (ck::Is_NOT_Valid(RevealerTransform))
                { continue; }

                ck_fog_of_war_processor::DoStampCircle(InParams.Get_Bounds(), InParams.Get_CellSize(),
                    InFogOfWar._CellCounts, InFogOfWar._Explored,
                    UCk_Utils_Transform_UE::Get_EntityCurrentLocation(RevealerTransform),
                    InParams.Get_RevealRadius(), InFogOfWar._NewlyRevealedScratch);
            }
        }

        ck_fog_of_war_processor::DoFlushRevealedBatch(InFogEntity, InFogOfWar._NewlyRevealedScratch);
    }
}

// --------------------------------------------------------------------------------------------------------------------
