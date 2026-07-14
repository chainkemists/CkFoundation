#include "CkPathNetwork_Vectorize.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkPathNetwork/CkPathNetwork_Log.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pathnetwork_vectorize
{
    // Chamfer 3-4 weights: orthogonal step = 3, diagonal step = 4. Half-width = (DT / 3) * CellSize.
    constexpr auto ChamferOrtho = 3;
    constexpr auto ChamferDiag = 4;
    constexpr auto ChamferInfinity = TNumericLimits<int32>::Max() / 2;

    // Neighbor offsets in Zhang-Suen order P2..P9 (clockwise from north). Y grows south in mask space.
    constexpr int32 NeighborDX[] = { 0, +1, +1, +1,  0, -1, -1, -1 };
    constexpr int32 NeighborDY[] = { -1, -1,  0, +1, +1, +1,  0, -1 };

    struct FTracedChain
    {
        TArray<int32> _PixelIndices;
        bool _IsRing = false;
        bool _IsDeadEnd = false;
    };

    auto
    Get_At(const TArray<uint8>& InGrid, int32 InSizeX, int32 InSizeY, int32 InX, int32 InY) -> uint8
    {
        if (InX < 0 || InY < 0 || InX >= InSizeX || InY >= InSizeY)
        { return 0; }

        return InGrid[InY * InSizeX + InX];
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Compute_ChamferDistanceTransform(const TArray<uint8>& InGrid, int32 InSizeX, int32 InSizeY) -> TArray<int32>
    {
        auto Dist = TArray<int32>{};
        Dist.SetNumUninitialized(InGrid.Num());

        for (auto Index = 0; Index < InGrid.Num(); ++Index)
        { Dist[Index] = InGrid[Index] != 0 ? ChamferInfinity : 0; }

        const auto Relax = [&](int32 InIndex, int32 InX, int32 InY, int32 InDX, int32 InDY, int32 InWeight)
        {
            const auto NX = InX + InDX;
            const auto NY = InY + InDY;

            // Out-of-bounds counts as empty (distance 0) so painted regions touching the mask
            // border measure a small width there instead of blowing up.
            const auto NeighborDist = (NX < 0 || NY < 0 || NX >= InSizeX || NY >= InSizeY)
                ? 0
                : Dist[NY * InSizeX + NX];

            Dist[InIndex] = FMath::Min(Dist[InIndex], NeighborDist + InWeight);
        };

        for (auto Y = 0; Y < InSizeY; ++Y)
        {
            for (auto X = 0; X < InSizeX; ++X)
            {
                const auto Index = Y * InSizeX + X;
                if (Dist[Index] == 0)
                { continue; }

                Relax(Index, X, Y, -1,  0, ChamferOrtho);
                Relax(Index, X, Y,  0, -1, ChamferOrtho);
                Relax(Index, X, Y, -1, -1, ChamferDiag);
                Relax(Index, X, Y, +1, -1, ChamferDiag);
            }
        }

        for (auto Y = InSizeY - 1; Y >= 0; --Y)
        {
            for (auto X = InSizeX - 1; X >= 0; --X)
            {
                const auto Index = Y * InSizeX + X;
                if (Dist[Index] == 0)
                { continue; }

                Relax(Index, X, Y, +1,  0, ChamferOrtho);
                Relax(Index, X, Y,  0, +1, ChamferOrtho);
                Relax(Index, X, Y, +1, +1, ChamferDiag);
                Relax(Index, X, Y, -1, +1, ChamferDiag);
            }
        }

        return Dist;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Apply_ZhangSuenThinning(TArray<uint8>& InOutGrid, int32 InSizeX, int32 InSizeY) -> void
    {
        auto ToDelete = TArray<int32>{};

        const auto RunSubIteration = [&](bool InIsFirstSubIteration) -> bool
        {
            ToDelete.Reset();

            for (auto Y = 0; Y < InSizeY; ++Y)
            {
                for (auto X = 0; X < InSizeX; ++X)
                {
                    const auto Index = Y * InSizeX + X;
                    if (InOutGrid[Index] == 0)
                    { continue; }

                    uint8 Neighbors[8];
                    auto NeighborSum = 0;
                    for (auto N = 0; N < 8; ++N)
                    {
                        Neighbors[N] = Get_At(InOutGrid, InSizeX, InSizeY, X + NeighborDX[N], Y + NeighborDY[N]);
                        NeighborSum += Neighbors[N];
                    }

                    if (NeighborSum < 2 || NeighborSum > 6)
                    { continue; }

                    auto Transitions = 0;
                    for (auto N = 0; N < 8; ++N)
                    {
                        if (Neighbors[N] == 0 && Neighbors[(N + 1) % 8] != 0)
                        { ++Transitions; }
                    }

                    if (Transitions != 1)
                    { continue; }

                    // P2=Neighbors[0], P4=Neighbors[2], P6=Neighbors[4], P8=Neighbors[6]
                    const auto ConditionsHold = InIsFirstSubIteration
                        ? (Neighbors[0] * Neighbors[2] * Neighbors[4] == 0) && (Neighbors[2] * Neighbors[4] * Neighbors[6] == 0)
                        : (Neighbors[0] * Neighbors[2] * Neighbors[6] == 0) && (Neighbors[0] * Neighbors[4] * Neighbors[6] == 0);

                    if (ConditionsHold)
                    { ToDelete.Add(Index); }
                }
            }

            for (const auto Index : ToDelete)
            { InOutGrid[Index] = 0; }

            return ToDelete.Num() > 0;
        };

        constexpr auto IsFirst = true;
        auto Changed = true;
        while (Changed)
        {
            const auto ChangedA = RunSubIteration(IsFirst);
            const auto ChangedB = RunSubIteration(NOT IsFirst);
            Changed = ChangedA || ChangedB;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    // Thinned skeletons keep REDUNDANT diagonal adjacencies at corners and junctions: a diagonal
    // neighbor that is also reachable through an orthogonal neighbor. Counting those as real
    // connectivity manufactures spurious junction pixels (raw degree 3+ at every staircase elbow)
    // and makes the chain walker ambiguous. A diagonal adjacency is REAL only when both of its
    // orthogonal "between" pixels are empty.
    auto
    Get_IsReducedNeighbor(const TArray<uint8>& InGrid, int32 InSizeX, int32 InSizeY, int32 InX, int32 InY, int32 InDX, int32 InDY) -> bool
    {
        if (Get_At(InGrid, InSizeX, InSizeY, InX + InDX, InY + InDY) == 0)
        { return false; }

        const auto IsDiagonal = InDX != 0 && InDY != 0;
        if (NOT IsDiagonal)
        { return true; }

        return Get_At(InGrid, InSizeX, InSizeY, InX + InDX, InY) == 0 &&
               Get_At(InGrid, InSizeX, InSizeY, InX, InY + InDY) == 0;
    }

    auto
    Compute_SkeletonDegrees(const TArray<uint8>& InGrid, int32 InSizeX, int32 InSizeY) -> TArray<uint8>
    {
        auto Degrees = TArray<uint8>{};
        Degrees.SetNumZeroed(InGrid.Num());

        for (auto Y = 0; Y < InSizeY; ++Y)
        {
            for (auto X = 0; X < InSizeX; ++X)
            {
                const auto Index = Y * InSizeX + X;
                if (InGrid[Index] == 0)
                { continue; }

                auto Degree = 0;
                for (auto N = 0; N < 8; ++N)
                {
                    if (Get_IsReducedNeighbor(InGrid, InSizeX, InSizeY, X, Y, NeighborDX[N], NeighborDY[N]))
                    { ++Degree; }
                }

                Degrees[Index] = static_cast<uint8>(Degree);
            }
        }

        return Degrees;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Trace_Chains(const TArray<uint8>& InGrid, const TArray<uint8>& InDegrees, int32 InSizeX, int32 InSizeY) -> TArray<FTracedChain>
    {
        auto Chains = TArray<FTracedChain>{};

        const auto IsNodePixel = [&](int32 InIndex) -> bool
        { return InGrid[InIndex] != 0 && InDegrees[InIndex] != 2; };

        const auto EncodeStep = [](int32 InFrom, int32 InTo) -> int64
        { return (static_cast<int64>(InFrom) << 32) | static_cast<int64>(static_cast<uint32>(InTo)); };

        auto ConsumedSteps = TSet<int64>{};
        auto VisitedInterior = TArray<uint8>{};
        VisitedInterior.SetNumZeroed(InGrid.Num());

        const auto ForEachSkeletonNeighbor = [&](int32 InIndex, auto InFunc)
        {
            const auto X = InIndex % InSizeX;
            const auto Y = InIndex / InSizeX;
            for (auto N = 0; N < 8; ++N)
            {
                const auto NX = X + NeighborDX[N];
                const auto NY = Y + NeighborDY[N];
                if (NX < 0 || NY < 0 || NX >= InSizeX || NY >= InSizeY)
                { continue; }

                if (Get_IsReducedNeighbor(InGrid, InSizeX, InSizeY, X, Y, NeighborDX[N], NeighborDY[N]))
                { InFunc(NY * InSizeX + NX); }
            }
        };

        // Chains between node pixels (junctions / endpoints).
        for (auto Index = 0; Index < InGrid.Num(); ++Index)
        {
            if (NOT IsNodePixel(Index))
            { continue; }

            ForEachSkeletonNeighbor(Index, [&](int32 InStartNeighbor)
            {
                if (ConsumedSteps.Contains(EncodeStep(Index, InStartNeighbor)))
                { return; }

                ConsumedSteps.Add(EncodeStep(Index, InStartNeighbor));

                auto Chain = FTracedChain{};
                Chain._PixelIndices.Add(Index);

                auto Prev = Index;
                auto Cur = InStartNeighbor;

                // Safety bound: a chain can never exceed the pixel count.
                for (auto Guard = 0; Guard <= InGrid.Num(); ++Guard)
                {
                    Chain._PixelIndices.Add(Cur);

                    if (IsNodePixel(Cur))
                    {
                        ConsumedSteps.Add(EncodeStep(Cur, Prev));
                        break;
                    }

                    VisitedInterior[Cur] = 1;

                    auto Next = int32{INDEX_NONE};
                    ForEachSkeletonNeighbor(Cur, [&](int32 InNeighbor)
                    {
                        if (InNeighbor != Prev && Next == INDEX_NONE)
                        { Next = InNeighbor; }
                    });

                    if (Next == INDEX_NONE)
                    { break; }

                    Prev = Cur;
                    Cur = Next;
                }

                Chain._IsDeadEnd = InDegrees[Chain._PixelIndices[0]] <= 1 || InDegrees[Chain._PixelIndices.Last()] <= 1;
                Chains.Add(MoveTemp(Chain));
            });
        }

        // Rings: leftover unvisited degree-2 pixels form closed loops with no junctions.
        for (auto Index = 0; Index < InGrid.Num(); ++Index)
        {
            if (InGrid[Index] == 0 || InDegrees[Index] != 2 || VisitedInterior[Index] != 0 || IsNodePixel(Index))
            { continue; }

            auto Chain = FTracedChain{};
            Chain._IsRing = true;
            Chain._PixelIndices.Add(Index);
            VisitedInterior[Index] = 1;

            auto Prev = Index;
            auto Cur = int32{INDEX_NONE};
            ForEachSkeletonNeighbor(Index, [&](int32 InNeighbor)
            {
                if (Cur == INDEX_NONE)
                { Cur = InNeighbor; }
            });

            for (auto Guard = 0; Cur != INDEX_NONE && Cur != Index && Guard <= InGrid.Num(); ++Guard)
            {
                Chain._PixelIndices.Add(Cur);
                VisitedInterior[Cur] = 1;

                auto Next = int32{INDEX_NONE};
                ForEachSkeletonNeighbor(Cur, [&](int32 InNeighbor)
                {
                    if (InNeighbor != Prev && Next == INDEX_NONE)
                    { Next = InNeighbor; }
                });

                Prev = Cur;
                Cur = Next;
            }

            if (Cur == Index)
            {
                // Genuine closure.
                Chain._PixelIndices.Add(Index);
            }
            else
            {
                // The walk broke off — this was not a clean ring; treat it as open noise.
                Chain._IsRing = false;
                Chain._IsDeadEnd = true;
            }

            Chains.Add(MoveTemp(Chain));
        }

        return Chains;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Compute_PerpendicularDistanceXY(const FVector& InPoint, const FVector& InSegA, const FVector& InSegB) -> float
    {
        const auto A = FVector2D{InSegA};
        const auto B = FVector2D{InSegB};
        const auto P = FVector2D{InPoint};

        const auto AB = B - A;
        const auto LengthSq = AB.SquaredLength();

        if (LengthSq < UE_KINDA_SMALL_NUMBER)
        { return static_cast<float>(FVector2D::Distance(P, A)); }

        const auto T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / LengthSq, 0.0, 1.0);
        return static_cast<float>(FVector2D::Distance(P, A + AB * T));
    }

    auto
    Simplify_DouglasPeucker_Span(const TArray<FVector>& InPoints, int32 InFirst, int32 InLast, float InTolerance, TSet<int32>& OutKept) -> void
    {
        struct FSpan { int32 _First; int32 _Last; };
        auto Stack = TArray<FSpan>{};
        Stack.Push(FSpan{InFirst, InLast});

        while (Stack.Num() > 0)
        {
            const auto Span = Stack.Pop();

            if (Span._Last - Span._First < 2)
            { continue; }

            auto MaxDist = 0.0f;
            auto MaxIndex = int32{INDEX_NONE};

            for (auto Index = Span._First + 1; Index < Span._Last; ++Index)
            {
                const auto Dist = Compute_PerpendicularDistanceXY(InPoints[Index], InPoints[Span._First], InPoints[Span._Last]);
                if (Dist > MaxDist)
                {
                    MaxDist = Dist;
                    MaxIndex = Index;
                }
            }

            if (MaxIndex != INDEX_NONE && MaxDist > InTolerance)
            {
                OutKept.Add(MaxIndex);
                Stack.Push(FSpan{Span._First, MaxIndex});
                Stack.Push(FSpan{MaxIndex, Span._Last});
            }
        }
    }

    // Returns the kept indices, sorted ascending. Rings (first == last) are split at the midpoint
    // so the coincident anchors can't collapse the whole loop.
    auto
    Simplify_DouglasPeucker(const TArray<FVector>& InPoints, float InTolerance, bool InIsRing) -> TArray<int32>
    {
        auto Kept = TSet<int32>{};
        Kept.Add(0);
        Kept.Add(InPoints.Num() - 1);

        if (InIsRing && InPoints.Num() >= 4)
        {
            const auto Mid = InPoints.Num() / 2;
            Kept.Add(Mid);
            Simplify_DouglasPeucker_Span(InPoints, 0, Mid, InTolerance, Kept);
            Simplify_DouglasPeucker_Span(InPoints, Mid, InPoints.Num() - 1, InTolerance, Kept);
        }
        else
        {
            Simplify_DouglasPeucker_Span(InPoints, 0, InPoints.Num() - 1, InTolerance, Kept);
        }

        auto Result = Kept.Array();
        Result.Sort();
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork
{
    auto
    Vectorize_MaskToRibbons(
        const FCk_PathNetwork_DetectionMask& InMask,
        const FCk_PathNetwork_VectorizeParams& InParams)
        -> TArray<FCk_PathNetwork_Ribbon>
    {
        using namespace ck_pathnetwork_vectorize;

        auto Ribbons = TArray<FCk_PathNetwork_Ribbon>{};

        CK_ENSURE_IF_NOT(InMask.Get_IsValidMask(),
            TEXT("Vectorize_MaskToRibbons: invalid mask (SizeX [{}], SizeY [{}], Occupancy [{}], Heights [{}])"),
            InMask.Get_SizeX(), InMask.Get_SizeY(), InMask.Get_Occupancy().Num(), InMask.Get_Heights().Num())
        { return Ribbons; }

        const auto SizeX = InMask.Get_SizeX();
        const auto SizeY = InMask.Get_SizeY();
        const auto CellSize = InMask.Get_CellSize();

        auto Grid = TArray<uint8>{};
        Grid.SetNumUninitialized(SizeX * SizeY);
        for (auto Index = 0; Index < Grid.Num(); ++Index)
        { Grid[Index] = InMask.Get_Occupancy()[Index] != 0 ? 1 : 0; }

        const auto DistanceTransform = Compute_ChamferDistanceTransform(Grid, SizeX, SizeY);

        Apply_ZhangSuenThinning(Grid, SizeX, SizeY);

        const auto Degrees = Compute_SkeletonDegrees(Grid, SizeX, SizeY);
        const auto Chains = Trace_Chains(Grid, Degrees, SizeX, SizeY);

        for (const auto& Chain : Chains)
        {
            if (Chain._PixelIndices.Num() < 2)
            { continue; }

            auto WorldPoints = TArray<FVector>{};
            WorldPoints.Reserve(Chain._PixelIndices.Num());

            for (const auto PixelIndex : Chain._PixelIndices)
            {
                const auto X = PixelIndex % SizeX;
                const auto Y = PixelIndex / SizeX;
                WorldPoints.Add(InMask.Get_CellWorldLocation(X, Y));
            }

            auto ChainLength = 0.0f;
            for (auto Index = 0; Index < WorldPoints.Num() - 1; ++Index)
            { ChainLength += static_cast<float>(FVector::Dist(WorldPoints[Index], WorldPoints[Index + 1])); }

            // Noise filters: short dead-end whiskers and short rings are artifacts of thinning /
            // paint specks. Junction-to-junction chains are structural and kept — EXCEPT the
            // few-pixel stubs inside a junction cluster (thinning can leave 2-3 adjacent node
            // pixels); the builder fuses their endpoints into one node anyway, so the stub would
            // only survive as a degenerate self-loop.
            if ((Chain._IsDeadEnd || Chain._IsRing) && ChainLength < InParams.Get_MinRibbonLength())
            { continue; }

            const auto IsJunctionClusterStub = NOT Chain._IsRing && NOT Chain._IsDeadEnd && ChainLength < 3.0f * CellSize;
            if (IsJunctionClusterStub)
            { continue; }

            const auto KeptIndices = Simplify_DouglasPeucker(WorldPoints, InParams.Get_SimplifyTolerance(), Chain._IsRing);

            auto Points = TArray<FCk_PathNetwork_RibbonPoint>{};
            Points.Reserve(KeptIndices.Num());

            for (const auto KeptIndex : KeptIndices)
            {
                const auto PixelIndex = Chain._PixelIndices[KeptIndex];

                // DT measures to the CENTER of the nearest empty cell; the paint boundary sits
                // half a cell closer.
                const auto DistanceCells = DistanceTransform[PixelIndex] / static_cast<float>(ChamferOrtho);
                const auto HalfWidth = FMath::Max(
                    InParams.Get_MinHalfWidth(),
                    (DistanceCells - 0.5f) * CellSize);

                Points.Add(FCk_PathNetwork_RibbonPoint{WorldPoints[KeptIndex], HalfWidth});
            }

            if (Points.Num() < 2)
            { continue; }

            auto Ribbon = FCk_PathNetwork_Ribbon{Points};
            Ribbon.Set_Source(ECk_PathNetwork_RibbonSource::Generated);
            Ribbon.Set_RibbonId(FGuid::NewGuid());
            Ribbons.Add(MoveTemp(Ribbon));
        }

        pathnetwork::Verbose(TEXT("Vectorize_MaskToRibbons: [{}]x[{}] mask -> [{}] chains -> [{}] ribbons"),
            SizeX, SizeY, Chains.Num(), Ribbons.Num());

        return Ribbons;
    }
}

// --------------------------------------------------------------------------------------------------------------------
