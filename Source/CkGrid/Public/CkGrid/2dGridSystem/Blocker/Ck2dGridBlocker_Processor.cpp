#include "Ck2dGridBlocker_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Fragment.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_2dGridBlocker_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_2dGridBlocker_Requests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_2dGridBlocker_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_2dGridBlocker_Params& InParams,
            FFragment_2dGridBlocker_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        const auto Grid = InParams.Get_Grid();

        CK_ENSURE_IF_NOT(ck::IsValid(Grid),
            TEXT("GridBlocker [{}] has an invalid grid handle — nothing to stamp."), InHandle)
        { return; }

        const auto& RangeMin = InParams.Get_RangeMin();
        const auto& RangeMax = InParams.Get_RangeMax();

        for (auto Y = RangeMin.Y; Y <= RangeMax.Y; ++Y)
        {
            for (auto X = RangeMin.X; X <= RangeMax.X; ++X)
            {
                const auto Coord = FIntPoint(X, Y);
                auto Cell = UCk_Utils_2dGridSystem_UE::Get_CellAt(Grid, Coord);

                if (ck::Is_NOT_Valid(Cell))
                { continue; }

                // Counted tag: Add == increment. Composes with shape and other blockers.
                Cell.Add<FTag_2dGridCell_Disabled>();
                InCurrent._StampedCells.Add(Coord);
            }
        }

        InCurrent._IsActive = true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_2dGridBlocker_Requests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_2dGridBlocker_Requests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_2dGridBlocker_Params& InParams,
            FFragment_2dGridBlocker_Current& InCurrent,
            FFragment_2dGridBlocker_Requests& InRequestsComp) const
        -> void
    {
        const auto Grid = InParams.Get_Grid();

        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_2dGridBlocker_Requests& InRequests)
        {
            auto FinalActive = InCurrent._IsActive;
            auto AnyRequest = false;

            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                FinalActive = InRequest.Get_Active();
                AnyRequest = true;

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));

            if (NOT AnyRequest)
            { return; }

            if (FinalActive == InCurrent._IsActive)
            { return; }

            if (ck::Is_NOT_Valid(Grid))
            {
                InCurrent._IsActive = FinalActive;
                return;
            }

            if (FinalActive)
            {
                for (const auto& Coord : InCurrent._StampedCells)
                {
                    auto Cell = UCk_Utils_2dGridSystem_UE::Get_CellAt(Grid, Coord);
                    if (ck::IsValid(Cell))
                    { Cell.Add<FTag_2dGridCell_Disabled>(); }
                }
            }
            else
            {
                for (const auto& Coord : InCurrent._StampedCells)
                {
                    auto Cell = UCk_Utils_2dGridSystem_UE::Get_CellAt(Grid, Coord);
                    if (ck::IsValid(Cell))
                    { Cell.Remove<FTag_2dGridCell_Disabled>(); }
                }
            }

            InCurrent._IsActive = FinalActive;
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
