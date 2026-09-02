#include "CkGroundNav_Rasterize.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace rasterize_private
    {
        // Sutherland-Hodgman clipping against one axis-aligned half-plane. Public algorithm: polygon
        // clipping against a convex window (Sutherland & Hodgman, 1974).
        //
        // InAxis 0 = X, 1 = Y. InKeepGreater selects which side of InPlane survives.
        auto DoClip_ToPlane(
            const TArray<FVector>& InPolygon,
            int32                  InAxis,
            double                 InPlane,
            bool                   InKeepGreater,
            TArray<FVector>&       OutPolygon) -> void
        {
            OutPolygon.Reset();

            if (InPolygon.Num() < 2)
            { return; }

            const auto Get_IsInside = [&](const FVector& InVertex) -> bool
            {
                const auto Value = InAxis == 0 ? InVertex.X : InVertex.Y;
                return InKeepGreater ? Value >= InPlane : Value <= InPlane;
            };

            for (auto Index = 0; Index < InPolygon.Num(); ++Index)
            {
                const auto& Current = InPolygon[Index];
                const auto& Previous = InPolygon[(Index + InPolygon.Num() - 1) % InPolygon.Num()];

                const auto CurrentInside = Get_IsInside(Current);
                const auto PreviousInside = Get_IsInside(Previous);

                if (CurrentInside != PreviousInside)
                {
                    const auto PreviousValue = InAxis == 0 ? Previous.X : Previous.Y;
                    const auto CurrentValue = InAxis == 0 ? Current.X : Current.Y;
                    const auto Denominator = CurrentValue - PreviousValue;

                    // Denominator is non-zero whenever the two endpoints straddle the plane, which is
                    // exactly the branch we are in.
                    const auto Alpha = (InPlane - PreviousValue) / Denominator;

                    OutPolygon.Emplace(Previous + ((Current - Previous) * Alpha));
                }

                if (CurrentInside)
                { OutPolygon.Emplace(Current); }
            }
        }

        /**
         * Twice the signed area the polygon projects onto XY.
         *
         * A vertical face lying on a cell boundary survives clipping into BOTH adjacent columns as a
         * sliver with three or more vertices and zero footprint. Admitting it would plant a phantom
         * surface in the column beyond the wall — and a cliff edge with a phantom floor beside it is
         * no longer a cliff edge, so the ledge filter would never fire.
         */
        auto Get_TwiceProjectedArea(
            const TArray<FVector>& InPolygon) -> double
        {
            auto Accumulated = 0.0;

            for (auto Index = 0; Index < InPolygon.Num(); ++Index)
            {
                const auto& Current = InPolygon[Index];
                const auto& Next = InPolygon[(Index + 1) % InPolygon.Num()];

                Accumulated += (Current.X * Next.Y) - (Next.X * Current.Y);
            }

            return FMath::Abs(Accumulated);
        }

        /**
         * Which of two merging spans keeps its surface — its _MaxZ, _Normal and _IsWalkable.
         *
         * Height decides it whenever the two heights differ. On an EXACT tie the order is CONTENT
         * ONLY: the NON-walkable face wins, then the greater quantized normal read lexicographically
         * as (_Z, _Y, _X). No epsilon anywhere, because the inputs are either bit-equal or not and a
         * tolerance would smuggle back the very ambiguity this resolves.
         *
         * Non-walkable wins because a face lying exactly flush on a floor is something SOLID standing
         * on that floor: the bottom of a wall, a crate, a pillar. Faces are all the rasterizer sees of
         * a body — its interior is never filled in — so that flush bottom face is the only evidence
         * the column holds that the floor there is covered. Letting the floor win would open every
         * wall footprint as standable ground with the wall's own top as its headroom.
         *
         * The rule exists because the alternative — keeping whichever span was submitted last — leaks
         * the geometry batch's triangle order into the baked field. A body resting on a floor reaches
         * this tie in every scene, so two batches holding the same triangles in a different order
         * would disagree about whether the floor under it is walkable. Being a total order over
         * content, this predicate cannot.
         */
        auto Get_ShouldAdoptExistingSurface(
            const FCk_GroundNav_Span& InExisting,
            const FCk_GroundNav_Span& InIncoming) -> bool
        {
            if (InExisting._MaxZ != InIncoming._MaxZ)
            { return InExisting._MaxZ > InIncoming._MaxZ; }

            if (InExisting._IsWalkable != InIncoming._IsWalkable)
            { return NOT InExisting._IsWalkable; }

            const auto& ExistingNormal = InExisting._Normal;
            const auto& IncomingNormal = InIncoming._Normal;

            if (ExistingNormal._Z != IncomingNormal._Z)
            { return ExistingNormal._Z > IncomingNormal._Z; }

            if (ExistingNormal._Y != IncomingNormal._Y)
            { return ExistingNormal._Y > IncomingNormal._Y; }

            return ExistingNormal._X > IncomingNormal._X;
        }

        /**
         * Insert one span into a column, keeping the column ordered by height, non-overlapping, and
         * merged across gaps no larger than InMergeThreshold.
         *
         * The merged span keeps the HIGHEST contributing surface's normal — the face an agent stands on.
         */
        auto DoInsert_Span(
            TArray<FCk_GroundNav_Span>& InOutColumn,
            FCk_GroundNav_Span          InSpan,
            float                       InMergeThreshold) -> void
        {
            auto InsertAt = 0;

            while (InsertAt < InOutColumn.Num())
            {
                auto& Existing = InOutColumn[InsertAt];

                // Entirely above the new span, with a gap wider than the merge threshold: stop here.
                if (Existing._MinZ > InSpan._MaxZ + InMergeThreshold)
                { break; }

                // Entirely below with a wide gap: keep scanning upward.
                if (Existing._MaxZ + InMergeThreshold < InSpan._MinZ)
                {
                    ++InsertAt;
                    continue;
                }

                // Overlapping or within the threshold: absorb and re-test from the same slot, because
                // absorbing one span can bring the next one within reach.
                InSpan._MinZ = FMath::Min(InSpan._MinZ, Existing._MinZ);

                if (Get_ShouldAdoptExistingSurface(Existing, InSpan))
                {
                    InSpan._MaxZ = Existing._MaxZ;
                    InSpan._Normal = Existing._Normal;
                    InSpan._IsWalkable = Existing._IsWalkable;
                }

                InOutColumn.RemoveAt(InsertAt);
            }

            InOutColumn.Insert(InSpan, InsertAt);

            // The ascending, disjoint column order three later stages read without re-deriving it
            // (see FCk_GroundNav_SpanField::_Columns). Only the two neighbours can have been
            // disturbed — the rest of the column was already ordered — so the check stays O(1).
            const auto BelowIsOrdered = NOT InOutColumn.IsValidIndex(InsertAt - 1) ||
                InOutColumn[InsertAt - 1]._MaxZ < InSpan._MinZ;
            const auto AboveIsOrdered = NOT InOutColumn.IsValidIndex(InsertAt + 1) ||
                InSpan._MaxZ < InOutColumn[InsertAt + 1]._MinZ;
            const auto ColumnStaysOrdered = BelowIsOrdered && AboveIsOrdered;

            CK_ENSURE_IF_NOT(ColumnStaysOrdered,
                TEXT("GroundNav column order broken inserting span [{}, {}] at index [{}] of [{}] spans"),
                InSpan._MinZ, InSpan._MaxZ, InsertAt, InOutColumn.Num())
            { return; }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoRasterizeSpans(
            const FCk_GroundNav_GeometryBatch& InGeometry,
            const FBox&                        InRegion,
            const FCk_GroundNav_BakeConfig&    InConfig,
            const FCk_GroundNav_AgentProfile&  InProfile,
            FCk_GroundNav_SpanField&           OutSpans)
        -> FCk_GroundNav_BakeStageResult
    {
        using namespace rasterize_private;

        auto Result = FCk_GroundNav_BakeStageResult{};

        if (NOT InConfig.Get_IsValid() || NOT InRegion.IsValid)
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        if (Get_ProfileRejection(InProfile) != EProfileRejection::None)
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::InvalidInput);
            return Result;
        }

        const auto CellSize = InConfig.Get_CellSizeUu();
        const auto Extent = InRegion.GetSize();

        const auto SizeX = FMath::Max(1, FMath::CeilToInt(Extent.X / CellSize));
        const auto SizeY = FMath::Max(1, FMath::CeilToInt(Extent.Y / CellSize));

        if (static_cast<int64>(SizeX) * static_cast<int64>(SizeY) > InConfig.Get_MaxColumnsPerTile())
        {
            Result.Set_Status(ECk_GroundNav_BakeStatus::LimitExceeded);
            return Result;
        }

        OutSpans = FCk_GroundNav_SpanField{};
        OutSpans._Origin = InRegion.Min;
        OutSpans._SizeX = SizeX;
        OutSpans._SizeY = SizeY;
        OutSpans._CellSizeUu = CellSize;
        OutSpans._Columns.SetNum(SizeX * SizeY);

        // Two surfaces within one step of each other are one climbable surface, so they merge. Below
        // that the column keeps them apart and layer extraction can see both.
        const auto MergeThreshold = InProfile.Get_StepHeightUu();
        const auto WalkableUpDot = FMath::Cos(FMath::DegreesToRadians(InProfile.Get_MaxSlopeDegrees()));

        auto DroppedCount = 0;

        // One probe is one innermost read: one triangle fetched, plus one triangle-to-cell clip visit.
        auto ProbesSpent = 0;

        auto Clipped = TArray<FVector>{};
        auto Scratch = TArray<FVector>{};
        auto RowPolygon = TArray<FVector>{};

        const auto TriangleCount = InGeometry.Get_TriangleCount();

        for (auto TriangleIndex = 0; TriangleIndex < TriangleCount; ++TriangleIndex)
        {
            auto A = FVector::ZeroVector;
            auto B = FVector::ZeroVector;
            auto C = FVector::ZeroVector;
            InGeometry.Get_Triangle(TriangleIndex, A, B, C);
            ++ProbesSpent;

            if (A.ContainsNaN() || B.ContainsNaN() || C.ContainsNaN())
            {
                ++DroppedCount;
                continue;
            }

            const auto Cross = FVector::CrossProduct(B - A, C - A);

            // A zero cross product means the three corners are collinear: the triangle has no area, no
            // normal, and nothing to stand on.
            if (Cross.IsNearlyZero())
            {
                ++DroppedCount;
                continue;
            }

            const auto Normal = Cross.GetSafeNormal();
            const auto Quantized = FCk_GroundNav_QuantizedNormal::Make(Normal);
            const auto IsWalkable = Quantized.Get_UpDot() >= WalkableUpDot;

            const auto TriangleBounds = FBox{TArray<FVector>{A, B, C}};

            if (NOT TriangleBounds.IntersectXY(InRegion))
            { continue; }

            // Column range this triangle can possibly touch.
            const auto MinX = FMath::Clamp(
                FMath::FloorToInt((TriangleBounds.Min.X - InRegion.Min.X) / CellSize), 0, SizeX - 1);
            const auto MaxX = FMath::Clamp(
                FMath::FloorToInt((TriangleBounds.Max.X - InRegion.Min.X) / CellSize), 0, SizeX - 1);
            const auto MinY = FMath::Clamp(
                FMath::FloorToInt((TriangleBounds.Min.Y - InRegion.Min.Y) / CellSize), 0, SizeY - 1);
            const auto MaxY = FMath::Clamp(
                FMath::FloorToInt((TriangleBounds.Max.Y - InRegion.Min.Y) / CellSize), 0, SizeY - 1);

            for (auto Y = MinY; Y <= MaxY; ++Y)
            {
                const auto RowMin = InRegion.Min.Y + (static_cast<double>(Y) * CellSize);
                const auto RowMax = RowMin + CellSize;

                // Clip the whole triangle to this row once, then walk the row's columns. Clipping per
                // column from the full triangle would repeat this work for every column in the row.
                Scratch = TArray<FVector>{A, B, C};
                DoClip_ToPlane(Scratch, 1, RowMin, true, Clipped);
                DoClip_ToPlane(Clipped, 1, RowMax, false, RowPolygon);

                if (RowPolygon.Num() < 3)
                { continue; }

                for (auto X = MinX; X <= MaxX; ++X)
                {
                    ++ProbesSpent;

                    const auto ColumnMin = InRegion.Min.X + (static_cast<double>(X) * CellSize);
                    const auto ColumnMax = ColumnMin + CellSize;

                    DoClip_ToPlane(RowPolygon, 0, ColumnMin, true, Clipped);
                    DoClip_ToPlane(Clipped, 0, ColumnMax, false, Scratch);

                    if (Scratch.Num() < 3)
                    { continue; }

                    // Scaled to the cell so the same threshold holds at every cell size. A genuinely
                    // thin triangle still clears it by orders of magnitude; only a zero-footprint
                    // sliver does not.
                    constexpr auto DegenerateAreaFraction = 1.0e-4;

                    if (Get_TwiceProjectedArea(Scratch) <
                        (static_cast<double>(CellSize) * CellSize * DegenerateAreaFraction))
                    { continue; }

                    auto SpanMinZ = TNumericLimits<double>::Max();
                    auto SpanMaxZ = TNumericLimits<double>::Lowest();

                    for (const auto& Vertex : Scratch)
                    {
                        SpanMinZ = FMath::Min(SpanMinZ, Vertex.Z);
                        SpanMaxZ = FMath::Max(SpanMaxZ, Vertex.Z);
                    }

                    auto Span = FCk_GroundNav_Span{};
                    Span._MinZ = static_cast<float>(SpanMinZ);
                    Span._MaxZ = static_cast<float>(SpanMaxZ);
                    Span._Normal = Quantized;
                    Span._IsWalkable = IsWalkable;

                    DoInsert_Span(OutSpans.Get_MutableColumn(X, Y), Span, MergeThreshold);
                }
            }
        }

        Result.Set_Status(ECk_GroundNav_BakeStatus::Completed);
        Result.Set_ProbesSpent(ProbesSpent);
        Result.Set_DroppedInputCount(DroppedCount);

        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
