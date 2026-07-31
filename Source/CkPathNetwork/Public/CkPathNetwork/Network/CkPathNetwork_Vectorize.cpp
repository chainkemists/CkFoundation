#include "CkPathNetwork_Vectorize.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkPathNetwork/CkPathNetwork_Log.h"

#include <Templates/Function.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pathnetwork_vectorize
{
    // Chamfer 3-4 distances are scaled by the orthogonal weight: cells = DT / ChamferOrtho.
    constexpr auto ChamferOrtho = 3;
    constexpr auto ChamferDiag = 4;
    constexpr auto ChamferInfinity = TNumericLimits<int32>::Max() / 2;

    // A cell center can differ from the represented surface by half a cell diagonal (~0.707).
    // Slightly exceed that quantization error so positive simplification removes raster stairs.
    constexpr auto GridQuantizationToleranceCells = 0.75f;
    constexpr auto HeightSimplificationToleranceCells = 0.5f;

    // Zhang-Suen P2..P9 order (clockwise from north); Y grows south in mask space.
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

            // Out-of-bounds counts as empty so paint touching the mask border measures a small width.
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

    auto
    Get_IsReducedNeighbor(
        const TArray<uint8>& InGrid,
        const TArray<uint8>& InSupportGrid,
        int32 InSizeX,
        int32 InSizeY,
        int32 InX,
        int32 InY,
        int32 InDX,
        int32 InDY)
        -> bool
    {
        if (Get_At(InGrid, InSizeX, InSizeY, InX + InDX, InY + InDY) == 0)
        { return false; }

        const auto IsDiagonal = InDX != 0 && InDY != 0;
        if (NOT IsDiagonal)
        { return true; }

        const auto HasRedundantSkeletonConnection =
            Get_At(InGrid, InSizeX, InSizeY, InX + InDX, InY) != 0 ||
            Get_At(InGrid, InSizeX, InSizeY, InX, InY + InDY) != 0;
        if (HasRedundantSkeletonConnection)
        { return false; }

        // A corner-only diagonal has no traversable area. The original, pre-thinning mask must
        // cover both cells beside the corner before the reduced skeleton may cross it.
        return
            Get_At(
                InSupportGrid,
                InSizeX,
                InSizeY,
                InX + InDX,
                InY) != 0 &&
            Get_At(
                InSupportGrid,
                InSizeX,
                InSizeY,
                InX,
                InY + InDY) != 0;
    }

    auto
    Compute_SkeletonDegrees(
        const TArray<uint8>& InGrid,
        const TArray<uint8>& InSupportGrid,
        int32 InSizeX,
        int32 InSizeY)
        -> TArray<uint8>
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
                    if (Get_IsReducedNeighbor(
                        InGrid,
                        InSupportGrid,
                        InSizeX,
                        InSizeY,
                        X,
                        Y,
                        NeighborDX[N],
                        NeighborDY[N]))
                    { ++Degree; }
                }

                Degrees[Index] = static_cast<uint8>(Degree);
            }
        }

        return Degrees;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Trace_Chains(
        const TArray<uint8>& InGrid,
        const TArray<uint8>& InSupportGrid,
        const TArray<uint8>& InDegrees,
        int32 InSizeX,
        int32 InSizeY)
        -> TArray<FTracedChain>
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

                if (Get_IsReducedNeighbor(
                    InGrid,
                    InSupportGrid,
                    InSizeX,
                    InSizeY,
                    X,
                    Y,
                    NeighborDX[N],
                    NeighborDY[N]))
                { InFunc(NY * InSizeX + NX); }
            }
        };

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

            const auto WalkClosedTheLoop = Cur == Index;

            if (WalkClosedTheLoop)
            {
                Chain._PixelIndices.Add(Index);
            }
            else
            {
                Chain._IsRing = false;
                Chain._IsDeadEnd = true;
            }

            Chains.Add(MoveTemp(Chain));
        }

        return Chains;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Compute_ChordDeviation(
        const FVector& InPoint,
        const FVector& InSegA,
        const FVector& InSegB)
        -> FVector2D
    {
        const auto A = FVector2D{InSegA};
        const auto B = FVector2D{InSegB};
        const auto P = FVector2D{InPoint};

        const auto AB = B - A;
        const auto LengthSq = AB.SquaredLength();

        if (LengthSq < UE_KINDA_SMALL_NUMBER)
        {
            return FVector2D{
                FVector2D::Distance(P, A),
                FMath::Abs(InPoint.Z - InSegA.Z)};
        }

        const auto T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / LengthSq, 0.0, 1.0);
        return FVector2D{
            FVector2D::Distance(P, A + AB * T),
            FMath::Abs(InPoint.Z - FMath::Lerp(InSegA.Z, InSegB.Z, T))};
    }

    auto
    Simplify_DouglasPeucker_Span(
        const TArray<FVector>& InPoints,
        int32 InFirst,
        int32 InLast,
        float InPlanarTolerance,
        float InHeightTolerance,
        TFunctionRef<bool(int32, int32)> InIsChordSupported,
        TSet<int32>& OutKept)
        -> void
    {
        struct FSpan { int32 _First; int32 _Last; };
        auto Stack = TArray<FSpan>{};
        Stack.Push(FSpan{InFirst, InLast});

        while (Stack.Num() > 0)
        {
            const auto Span = Stack.Pop();

            if (Span._Last - Span._First < 2)
            { continue; }

            auto MaxExcess = 0.0;
            auto MaxIndex = int32{INDEX_NONE};

            for (auto Index = Span._First + 1; Index < Span._Last; ++Index)
            {
                const auto Deviation = Compute_ChordDeviation(
                    InPoints[Index],
                    InPoints[Span._First],
                    InPoints[Span._Last]);
                const auto Excess = FMath::Max(
                    Deviation.X - InPlanarTolerance,
                    Deviation.Y - InHeightTolerance);
                if (Excess > MaxExcess)
                {
                    MaxExcess = Excess;
                    MaxIndex = Index;
                }
            }

            if (MaxIndex == INDEX_NONE &&
                NOT InIsChordSupported(Span._First, Span._Last))
            {
                MaxIndex = (Span._First + Span._Last) / 2;
            }

            if (MaxIndex != INDEX_NONE)
            {
                OutKept.Add(MaxIndex);
                Stack.Push(FSpan{Span._First, MaxIndex});
                Stack.Push(FSpan{MaxIndex, Span._Last});
            }
        }
    }

    // Rings (first == last) are split at the midpoint so the coincident anchors can't collapse the loop.
    auto
    Simplify_DouglasPeucker(
        const TArray<FVector>& InPoints,
        float InPlanarTolerance,
        float InHeightTolerance,
        TFunctionRef<bool(int32, int32)> InIsChordSupported,
        bool InIsRing)
        -> TArray<int32>
    {
        if (InPlanarTolerance <= 0.0f)
        {
            auto AllIndices = TArray<int32>{};
            AllIndices.Reserve(InPoints.Num());
            for (auto Index = 0; Index < InPoints.Num(); ++Index)
            { AllIndices.Add(Index); }
            return AllIndices;
        }

        auto Kept = TSet<int32>{};
        Kept.Add(0);
        Kept.Add(InPoints.Num() - 1);

        if (InIsRing && InPoints.Num() >= 4)
        {
            const auto Mid = InPoints.Num() / 2;
            Kept.Add(Mid);
            Simplify_DouglasPeucker_Span(
                InPoints,
                0,
                Mid,
                InPlanarTolerance,
                InHeightTolerance,
                InIsChordSupported,
                Kept);
            Simplify_DouglasPeucker_Span(
                InPoints,
                Mid,
                InPoints.Num() - 1,
                InPlanarTolerance,
                InHeightTolerance,
                InIsChordSupported,
                Kept);
        }
        else
        {
            Simplify_DouglasPeucker_Span(
                InPoints,
                0,
                InPoints.Num() - 1,
                InPlanarTolerance,
                InHeightTolerance,
                InIsChordSupported,
                Kept);
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
    Try_VectorizeMaskToRibbons(
        const FCk_PathNetwork_DetectionMask& InMask,
        const FCk_PathNetwork_VectorizeParams& InParams,
        ICk_PathNetwork_VectorizationSegmentEvaluator* InSegmentEvaluator)
        -> FCk_PathNetwork_VectorizeResult
    {
        using namespace ck_pathnetwork_vectorize;

        auto Result = FCk_PathNetwork_VectorizeResult{};
        auto Ribbons = TArray<FCk_PathNetwork_Ribbon>{};

        const auto MaskIsValid = InMask.Get_IsValidMask();
        CK_ENSURE_IF_NOT(MaskIsValid,
            TEXT("Vectorize_MaskToRibbons: invalid mask (SizeX [{}], SizeY [{}], Occupancy [{}], Heights [{}])"),
            InMask.Get_SizeX(), InMask.Get_SizeY(), InMask.Get_Occupancy().Num(), InMask.Get_Heights().Num())
        { }
        if (NOT MaskIsValid)
        {
            Result._Succeeded = false;
            Result._FailureReason = TEXT("Cannot vectorize an invalid detection mask");
            return Result;
        }

        const auto SizeX = InMask.Get_SizeX();
        const auto SizeY = InMask.Get_SizeY();
        const auto CellSize = InMask.Get_CellSize();
        const auto RequestedSimplifyTolerance =
            InParams.Get_SimplifyTolerance();
        const auto EffectiveSimplifyTolerance =
            RequestedSimplifyTolerance > 0.0f
            ? FMath::Max(
                RequestedSimplifyTolerance,
                GridQuantizationToleranceCells * CellSize)
            : 0.0f;
        const auto HeightSimplificationTolerance =
            FMath::Min(
                EffectiveSimplifyTolerance,
                HeightSimplificationToleranceCells * CellSize);

        auto Grid = TArray<uint8>{};
        Grid.SetNumUninitialized(SizeX * SizeY);
        for (auto Index = 0; Index < Grid.Num(); ++Index)
        { Grid[Index] = InMask.Get_Occupancy()[Index] != 0 ? 1 : 0; }
        const auto SupportGrid = Grid;

        const auto DistanceTransform = Compute_ChamferDistanceTransform(Grid, SizeX, SizeY);

        Apply_ZhangSuenThinning(Grid, SizeX, SizeY);

        const auto Degrees = Compute_SkeletonDegrees(
            Grid,
            SupportGrid,
            SizeX,
            SizeY);
        const auto Chains = Trace_Chains(
            Grid,
            SupportGrid,
            Degrees,
            SizeX,
            SizeY);

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

            const auto IsThinningWhiskerOrSpeck = (Chain._IsDeadEnd || Chain._IsRing) && ChainLength < InParams.Get_MinRibbonLength();
            if (IsThinningWhiskerOrSpeck)
            { continue; }

            // Junction-to-junction chains are structural, EXCEPT few-pixel stubs inside a junction
            // cluster (thinning can leave 2-3 adjacent node pixels): the builder fuses their endpoints
            // into one node, so such a stub could only survive as a degenerate self-loop.
            constexpr auto JunctionClusterStubCells = 3.0f;
            const auto IsJunctionClusterStub = NOT Chain._IsRing && NOT Chain._IsDeadEnd && ChainLength < JunctionClusterStubCells * CellSize;
            if (IsJunctionClusterStub)
            { continue; }

            auto EvaluationFailure = FString{};
            const auto EvaluateDetectorSegment =
                [&](const int32 InFirst, const int32 InLast)
                {
                    if (InSegmentEvaluator == nullptr)
                    { return true; }

                    const auto Evaluation = InSegmentEvaluator->Evaluate_Segment(
                        WorldPoints[InFirst],
                        WorldPoints[InLast]);
                    switch (Evaluation._Decision)
                    {
                        case ECk_PathNetwork_VectorizationSegmentDecision::Supported:
                            return true;
                        case ECk_PathNetwork_VectorizationSegmentDecision::Unsupported:
                            return false;
                        case ECk_PathNetwork_VectorizationSegmentDecision::EvaluationFailed:
                            EvaluationFailure = Evaluation._FailureReason.IsEmpty()
                                ? TEXT("Detector failed to evaluate vectorization segment support")
                                : Evaluation._FailureReason;
                            return false;
                        default:
                            EvaluationFailure =
                                TEXT("Detector returned an invalid vectorization segment decision");
                            return false;
                    }
                };

            // Exact detector support is a topology contract, not only a simplification hint.
            // Evaluate every raw edge first so an unsupported terminal span becomes a real split.
            auto UnsupportedEdgePrefix = TArray<int32>{};
            UnsupportedEdgePrefix.SetNumZeroed(WorldPoints.Num());
            for (auto EdgeIndex = 0;
                 EdgeIndex < WorldPoints.Num() - 1;
                 ++EdgeIndex)
            {
                const auto EdgeIsSupported =
                    EvaluateDetectorSegment(EdgeIndex, EdgeIndex + 1);
                if (NOT EvaluationFailure.IsEmpty())
                {
                    Result._Succeeded = false;
                    Result._FailureReason = MoveTemp(EvaluationFailure);
                    return Result;
                }
                UnsupportedEdgePrefix[EdgeIndex + 1] =
                    UnsupportedEdgePrefix[EdgeIndex] +
                    (EdgeIsSupported ? 0 : 1);
            }

            const auto HasUnsupportedRawEdge =
                [&UnsupportedEdgePrefix](
                    const int32 InFirst,
                    const int32 InLast)
                {
                    return
                        UnsupportedEdgePrefix[InLast] -
                        UnsupportedEdgePrefix[InFirst] > 0;
                };

            const auto IsMaskChordSupported =
                [&](const int32 InFirst, const int32 InLast)
                {
                    const auto& Start = WorldPoints[InFirst];
                    const auto& End = WorldPoints[InLast];
                    const auto StartPixel =
                        Chain._PixelIndices[InFirst];
                    const auto EndPixel =
                        Chain._PixelIndices[InLast];
                    auto X = StartPixel % SizeX;
                    auto Y = StartPixel / SizeX;
                    const auto EndX = EndPixel % SizeX;
                    const auto EndY = EndPixel / SizeX;
                    const auto StepX =
                        EndX > X ? 1 : EndX < X ? -1 : 0;
                    const auto StepY =
                        EndY > Y ? 1 : EndY < Y ? -1 : 0;
                    const auto CellCountX = FMath::Abs(EndX - X);
                    const auto CellCountY = FMath::Abs(EndY - Y);
                    auto TraversedX = 0;
                    auto TraversedY = 0;

                    const auto& Heights = InMask.Get_Heights();
                    const auto HasHeights =
                        Heights.Num() == SizeX * SizeY;
                    const auto StartXY = FVector2D{Start};
                    const auto EndXY = FVector2D{End};
                    const auto ChordXY = EndXY - StartXY;
                    const auto ChordLengthSquared =
                        ChordXY.SquaredLength();
                    const auto IsCellSupported =
                        [&](const int32 InX, const int32 InY)
                        {
                            if (NOT InMask.Get_IsOccupied(InX, InY))
                            { return false; }

                            if (NOT HasHeights)
                            { return true; }

                            const auto CellLocation =
                                InMask.Get_CellWorldLocation(InX, InY);
                            const auto T =
                                ChordLengthSquared >
                                    UE_KINDA_SMALL_NUMBER
                                ? FMath::Clamp(
                                    FVector2D::DotProduct(
                                        FVector2D{CellLocation} -
                                            StartXY,
                                        ChordXY) /
                                        ChordLengthSquared,
                                    0.0,
                                    1.0)
                                : 0.0;
                            const auto ChordZ =
                                FMath::Lerp(Start.Z, End.Z, T);
                            return FMath::Abs(
                                CellLocation.Z - ChordZ) <=
                                HeightSimplificationTolerance;
                        };

                    if (NOT IsCellSupported(X, Y))
                    { return false; }

                    while (TraversedX < CellCountX ||
                        TraversedY < CellCountY)
                    {
                        const auto Decision =
                            (1 + 2 * static_cast<int64>(TraversedX)) *
                                CellCountY -
                            (1 + 2 * static_cast<int64>(TraversedY)) *
                                CellCountX;
                        if (Decision == 0)
                        {
                            if (NOT IsCellSupported(X + StepX, Y) ||
                                NOT IsCellSupported(X, Y + StepY))
                            { return false; }

                            X += StepX;
                            Y += StepY;
                            ++TraversedX;
                            ++TraversedY;
                        }
                        else if (Decision < 0)
                        {
                            X += StepX;
                            ++TraversedX;
                        }
                        else
                        {
                            Y += StepY;
                            ++TraversedY;
                        }

                        if (NOT IsCellSupported(X, Y))
                        { return false; }
                    }

                    return true;
                };
            const auto IsChordSupported =
                [&](const int32 InFirst, const int32 InLast)
                {
                    if (HasUnsupportedRawEdge(InFirst, InLast) ||
                        NOT IsMaskChordSupported(InFirst, InLast))
                    { return false; }

                    if (NOT EvaluationFailure.IsEmpty())
                    { return true; }
                    return EvaluateDetectorSegment(InFirst, InLast);
                };
            const auto KeptIndices = Simplify_DouglasPeucker(
                WorldPoints,
                EffectiveSimplifyTolerance,
                HeightSimplificationTolerance,
                IsChordSupported,
                Chain._IsRing);
            if (NOT EvaluationFailure.IsEmpty())
            {
                Result._Succeeded = false;
                Result._FailureReason = MoveTemp(EvaluationFailure);
                return Result;
            }

            auto Points = TArray<FCk_PathNetwork_RibbonPoint>{};
            Points.Reserve(KeptIndices.Num());
            const auto FlushPoints =
                [&Points, &Ribbons]()
                {
                    if (Points.Num() >= 2)
                    {
                        auto Ribbon = FCk_PathNetwork_Ribbon{Points};
                        Ribbon.Set_Source(
                            ECk_PathNetwork_RibbonSource::Generated);
                        Ribbon.Set_RibbonId(FGuid::NewGuid());
                        Ribbons.Add(MoveTemp(Ribbon));
                    }
                    Points.Reset();
                };

            for (auto KeptPosition = 0;
                 KeptPosition < KeptIndices.Num();
                 ++KeptPosition)
            {
                const auto KeptIndex = KeptIndices[KeptPosition];
                if (KeptPosition > 0 &&
                    HasUnsupportedRawEdge(
                        KeptIndices[KeptPosition - 1],
                        KeptIndex))
                { FlushPoints(); }

                const auto PixelIndex = Chain._PixelIndices[KeptIndex];

                // DT measures to the CENTER of the nearest empty cell; the paint boundary is half a cell closer.
                const auto DistanceCells = DistanceTransform[PixelIndex] / static_cast<float>(ChamferOrtho);
                const auto HalfWidth = FMath::Max(
                    InParams.Get_MinHalfWidth(),
                    (DistanceCells - 0.5f) * CellSize);

                Points.Add(FCk_PathNetwork_RibbonPoint{WorldPoints[KeptIndex], HalfWidth});
            }
            FlushPoints();
        }

        pathnetwork::Verbose(TEXT("Vectorize_MaskToRibbons: [{}]x[{}] mask -> [{}] chains -> [{}] ribbons"),
            SizeX, SizeY, Chains.Num(), Ribbons.Num());

        Result._Ribbons = MoveTemp(Ribbons);
        return Result;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
    Vectorize_MaskToRibbons(
        const FCk_PathNetwork_DetectionMask& InMask,
        const FCk_PathNetwork_VectorizeParams& InParams)
        -> TArray<FCk_PathNetwork_Ribbon>
    {
        auto Result = Try_VectorizeMaskToRibbons(
            InMask,
            InParams,
            nullptr);
        return MoveTemp(Result._Ribbons);
    }
}

// --------------------------------------------------------------------------------------------------------------------
