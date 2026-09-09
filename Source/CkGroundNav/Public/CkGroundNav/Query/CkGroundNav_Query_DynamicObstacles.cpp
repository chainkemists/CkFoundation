#include "CkGroundNav_Query_DynamicObstacles.h"

#include <Algo/Sort.h>
#include <cmath>
#include <functional>
#include <limits>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace dynamicobstacles_private
    {
        constexpr auto kBroadPhaseMaximumMagnitude = 1.0e18;
        constexpr auto kBroadPhaseMinimumDelta = 1.0e-6;
        constexpr auto kBroadPhaseLeafSize = 8;

        auto Get_IsFinite(const FVector& InValue) -> bool
        {
            return FMath::IsFinite(InValue.X) && FMath::IsFinite(InValue.Y) && FMath::IsFinite(InValue.Z);
        }

        auto Get_IsFinite(const FVector2D& InValue) -> bool
        {
            return FMath::IsFinite(InValue.X) && FMath::IsFinite(InValue.Y);
        }

        auto Get_HasFiniteBounds(const FVector& InCentre, const FVector& InHalfExtents) -> bool
        {
            return Get_IsFinite(InCentre + InHalfExtents) && Get_IsFinite(InCentre - InHalfExtents);
        }

        auto Get_IsValid(const FCk_GroundNav_DynamicObstacleDisc& InDisc) -> bool
        {
            return Get_IsFinite(InDisc._Centre) && FMath::IsFinite(InDisc._RadiusUu) &&
                FMath::IsFinite(InDisc._VerticalHalfExtentUu) && InDisc._RadiusUu > 0.0f &&
                InDisc._VerticalHalfExtentUu > 0.0f && Get_HasFiniteBounds(
                    InDisc._Centre, FVector{InDisc._RadiusUu, InDisc._RadiusUu, InDisc._VerticalHalfExtentUu});
        }

        auto Get_IsValid(const FCk_GroundNav_DynamicObstacleObb& InObb) -> bool
        {
            const auto Rotation = InObb._YawTransform.GetRotation();
            const auto Scale = InObb._YawTransform.GetScale3D();
            const auto RotationIsFinite = FMath::IsFinite(Rotation.X) && FMath::IsFinite(Rotation.Y) &&
                FMath::IsFinite(Rotation.Z) && FMath::IsFinite(Rotation.W);
            const auto IsYawOnly = Rotation.X == 0.0 && Rotation.Y == 0.0;
            const auto IsUnitRotation = FMath::IsNearlyEqual(Rotation.SizeSquared(), 1.0f);
            const auto IsUnitScale = Scale.X == 1.0 && Scale.Y == 1.0 && Scale.Z == 1.0;

            return InObb._YawTransform.IsValid() && RotationIsFinite && IsYawOnly && IsUnitRotation &&
                IsUnitScale && Get_IsFinite(InObb._YawTransform.GetLocation()) &&
                Get_IsFinite(InObb._WorldHalfExtents) && InObb._WorldHalfExtents.X > 0.0f &&
                InObb._WorldHalfExtents.Y > 0.0f && InObb._WorldHalfExtents.Z > 0.0f &&
                Get_HasFiniteBounds(InObb._YawTransform.GetLocation(), InObb._WorldHalfExtents);
        }

        auto Get_IsSafeBroadPhaseValue(double InValue) -> bool
        {
            return std::isfinite(InValue) && FMath::Abs(InValue) <= kBroadPhaseMaximumMagnitude;
        }

        template <typename NodeType>
        auto Try_BuildDiscBroadPhase(
            TConstArrayView<FCk_GroundNav_DynamicObstacleDisc> InDiscs,
            TArray<NodeType>& OutNodes,
            TArray<int32>& OutIndices) -> bool
        {
            if (InDiscs.IsEmpty()) { return true; }

            struct FBounds
            {
                double _MinimumX;
                double _MinimumY;
                double _MinimumZ;
                double _MaximumX;
                double _MaximumY;
                double _MaximumZ;
            };
            auto Bounds = TArray<FBounds>{};
            Bounds.Reserve(InDiscs.Num());
            for (const auto& Disc : InDiscs)
            {
                const auto CentreX = static_cast<double>(Disc._Centre.X);
                const auto CentreY = static_cast<double>(Disc._Centre.Y);
                const auto CentreZ = static_cast<double>(Disc._Centre.Z);
                const auto Radius = static_cast<double>(Disc._RadiusUu);
                const auto HalfHeight = static_cast<double>(Disc._VerticalHalfExtentUu);
                if (NOT Get_IsSafeBroadPhaseValue(CentreX) || NOT Get_IsSafeBroadPhaseValue(CentreY) ||
                    NOT Get_IsSafeBroadPhaseValue(CentreZ) || NOT Get_IsSafeBroadPhaseValue(Radius) ||
                    NOT Get_IsSafeBroadPhaseValue(HalfHeight))
                { return false; }
                const auto MinimumX = std::nextafter(CentreX - Radius, -std::numeric_limits<double>::infinity());
                const auto MinimumY = std::nextafter(CentreY - Radius, -std::numeric_limits<double>::infinity());
                const auto MinimumZ = std::nextafter(CentreZ - HalfHeight, -std::numeric_limits<double>::infinity());
                const auto MaximumX = std::nextafter(CentreX + Radius, std::numeric_limits<double>::infinity());
                const auto MaximumY = std::nextafter(CentreY + Radius, std::numeric_limits<double>::infinity());
                const auto MaximumZ = std::nextafter(CentreZ + HalfHeight, std::numeric_limits<double>::infinity());
                if (NOT Get_IsSafeBroadPhaseValue(MinimumX) || NOT Get_IsSafeBroadPhaseValue(MinimumY) ||
                    NOT Get_IsSafeBroadPhaseValue(MinimumZ) || NOT Get_IsSafeBroadPhaseValue(MaximumX) ||
                    NOT Get_IsSafeBroadPhaseValue(MaximumY) || NOT Get_IsSafeBroadPhaseValue(MaximumZ))
                { return false; }
                Bounds.Add(FBounds{MinimumX, MinimumY, MinimumZ, MaximumX, MaximumY, MaximumZ});
            }

            OutIndices.Reserve(InDiscs.Num());
            for (auto Index = 0; Index < InDiscs.Num(); ++Index) { OutIndices.Add(Index); }
            auto Build = TFunction<int32(int32, int32)>{};
            Build = [&](const int32 InFirst, const int32 InCount) -> int32
            {
                const auto NodeIndex = OutNodes.AddDefaulted();
                auto& Node = OutNodes[NodeIndex];
                Node._MinimumX = Node._MinimumY = Node._MinimumZ = std::numeric_limits<double>::infinity();
                Node._MaximumX = Node._MaximumY = Node._MaximumZ = -std::numeric_limits<double>::infinity();
                for (auto Offset = 0; Offset < InCount; ++Offset)
                {
                    const auto& Bound = Bounds[OutIndices[InFirst + Offset]];
                    Node._MinimumX = FMath::Min(Node._MinimumX, Bound._MinimumX);
                    Node._MinimumY = FMath::Min(Node._MinimumY, Bound._MinimumY);
                    Node._MinimumZ = FMath::Min(Node._MinimumZ, Bound._MinimumZ);
                    Node._MaximumX = FMath::Max(Node._MaximumX, Bound._MaximumX);
                    Node._MaximumY = FMath::Max(Node._MaximumY, Bound._MaximumY);
                    Node._MaximumZ = FMath::Max(Node._MaximumZ, Bound._MaximumZ);
                }
                if (InCount <= kBroadPhaseLeafSize)
                {
                    Node._First = InFirst;
                    Node._Count = InCount;
                    return NodeIndex;
                }
                const auto ExtentX = Node._MaximumX - Node._MinimumX;
                const auto ExtentY = Node._MaximumY - Node._MinimumY;
                const auto ExtentZ = Node._MaximumZ - Node._MinimumZ;
                const auto Axis = ExtentX >= ExtentY && ExtentX >= ExtentZ ? 0 : ExtentY >= ExtentZ ? 1 : 2;
                Algo::Sort(MakeArrayView(OutIndices.GetData() + InFirst, InCount), [&](const int32 InLeft, const int32 InRight)
                {
                    const auto& Left = Bounds[InLeft];
                    const auto& Right = Bounds[InRight];
                    const auto LeftCentre = Axis == 0 ? Left._MinimumX + Left._MaximumX :
                        Axis == 1 ? Left._MinimumY + Left._MaximumY : Left._MinimumZ + Left._MaximumZ;
                    const auto RightCentre = Axis == 0 ? Right._MinimumX + Right._MaximumX :
                        Axis == 1 ? Right._MinimumY + Right._MaximumY : Right._MinimumZ + Right._MaximumZ;
                    return LeftCentre == RightCentre ? InLeft < InRight : LeftCentre < RightCentre;
                });
                const auto LeftCount = InCount / 2;
                const auto Left = Build(InFirst, LeftCount);
                const auto Right = Build(InFirst + LeftCount, InCount - LeftCount);
                OutNodes[NodeIndex]._Left = Left;
                OutNodes[NodeIndex]._Right = Right;
                return NodeIndex;
            };
            Build(0, OutIndices.Num());
            return true;
        }

        template <typename NodeType>
        auto Get_DiscBroadPhaseCandidates(
            TConstArrayView<NodeType> InNodes,
            TConstArrayView<int32> InIndices,
            double InMinimumX,
            double InMinimumY,
            double InMinimumZ,
            double InMaximumX,
            double InMaximumY,
            double InMaximumZ) -> TArray<int32, TInlineAllocator<16>>
        {
            auto Candidates = TArray<int32, TInlineAllocator<16>>{};
            if (InNodes.IsEmpty()) { return Candidates; }
            auto Pending = TArray<int32, TInlineAllocator<16>>{0};
            while (NOT Pending.IsEmpty())
            {
                const auto& Node = InNodes[Pending.Pop(EAllowShrinking::No)];
                const auto IsOverlapping = InMinimumX <= Node._MaximumX && InMaximumX >= Node._MinimumX &&
                    InMinimumY <= Node._MaximumY && InMaximumY >= Node._MinimumY &&
                    InMinimumZ <= Node._MaximumZ && InMaximumZ >= Node._MinimumZ;
                if (NOT IsOverlapping) { continue; }
                if (Node._Count > 0)
                {
                    for (auto Offset = 0; Offset < Node._Count; ++Offset)
                    { Candidates.Add(InIndices[Node._First + Offset]); }
                }
                else
                {
                    Pending.Add(Node._Left);
                    Pending.Add(Node._Right);
                }
            }
            Candidates.Sort();
            return Candidates;
        }

        auto Get_IsSafeBroadPhaseQueryBounds(
            const double InMinimumX,
            const double InMinimumY,
            const double InMinimumZ,
            const double InMaximumX,
            const double InMaximumY,
            const double InMaximumZ) -> bool
        {
            return Get_IsSafeBroadPhaseValue(InMinimumX) && Get_IsSafeBroadPhaseValue(InMinimumY) &&
                Get_IsSafeBroadPhaseValue(InMinimumZ) && Get_IsSafeBroadPhaseValue(InMaximumX) &&
                Get_IsSafeBroadPhaseValue(InMaximumY) && Get_IsSafeBroadPhaseValue(InMaximumZ) &&
                InMinimumX <= InMaximumX && InMinimumY <= InMaximumY && InMinimumZ <= InMaximumZ;
        }

        auto Get_IsSafeBroadPhaseDelta(double InDelta) -> bool
        {
            return InDelta == 0.0 || (Get_IsSafeBroadPhaseValue(InDelta) && FMath::Abs(InDelta) >= kBroadPhaseMinimumDelta);
        }

        struct FIntervalResult
        {
            bool _IsValid = false;
            bool _IsIntersecting = false;
            double _Enter = 0.0;
            double _Exit = 0.0;
        };

        auto Get_IntervalOnAxis(double InStart, double InDelta, double InMinimum, double InMaximum)
            -> FIntervalResult
        {
            if (NOT FMath::IsFinite(InStart) || NOT FMath::IsFinite(InDelta) ||
                NOT FMath::IsFinite(InMinimum) || NOT FMath::IsFinite(InMaximum) || InMinimum > InMaximum)
            { return {}; }
            if (InDelta == 0.0f)
            {
                if (InStart < InMinimum || InStart > InMaximum)
                { return FIntervalResult{true}; }
                return FIntervalResult{true, true, 0.0, 1.0};
            }

            auto First = (InMinimum - InStart) / InDelta;
            auto Last = (InMaximum - InStart) / InDelta;
            if (NOT FMath::IsFinite(First) || NOT FMath::IsFinite(Last))
            { return {}; }
            if (First > Last)
            { Swap(First, Last); }
            const auto Enter = FMath::Max(0.0, First);
            const auto Exit = FMath::Min(1.0, Last);
            return Enter <= Exit ? FIntervalResult{true, true, Enter, Exit} : FIntervalResult{true};
        }

        auto Get_DiscInterval(
            const FCk_GroundNav_DynamicObstacleDisc& InDisc,
            const FVector&                            InStart,
            const FVector&                            InEnd) -> FIntervalResult
        {
            const auto DeltaX = InEnd.X - InStart.X;
            const auto DeltaY = InEnd.Y - InStart.Y;
            const auto DeltaZ = InEnd.Z - InStart.Z;
            const auto FromX = InStart.X - InDisc._Centre.X;
            const auto FromY = InStart.Y - InDisc._Centre.Y;
            if (NOT FMath::IsFinite(DeltaX) || NOT FMath::IsFinite(DeltaY) || NOT FMath::IsFinite(DeltaZ) ||
                NOT FMath::IsFinite(FromX) || NOT FMath::IsFinite(FromY))
            { return {}; }
            const auto Vertical = Get_IntervalOnAxis(
                InStart.Z, DeltaZ, InDisc._Centre.Z - InDisc._VerticalHalfExtentUu,
                InDisc._Centre.Z + InDisc._VerticalHalfExtentUu);
            if (NOT Vertical._IsValid)
            { return {}; }
            if (NOT Vertical._IsIntersecting)
            { return FIntervalResult{true}; }
            const auto LengthSq = (DeltaX * DeltaX) + (DeltaY * DeltaY);
            if (NOT FMath::IsFinite(LengthSq))
            { return {}; }
            auto Horizontal = FIntervalResult{true, true, 0.0, 1.0};

            if (LengthSq == 0.0)
            {
                const auto Radius = static_cast<double>(InDisc._RadiusUu);
                const auto Distance = std::hypot(FromX, FromY);
                if (NOT FMath::IsFinite(Distance))
                { return {}; }
                if (Distance > Radius)
                { return FIntervalResult{true}; }
            }
            else
            {
                const auto Radius = static_cast<double>(InDisc._RadiusUu);
                const auto B = 2.0 * ((FromX * DeltaX) + (FromY * DeltaY));
                const auto C = (FromX * FromX) + (FromY * FromY) - (Radius * Radius);
                const auto Discriminant = (B * B) - (4.0 * LengthSq * C);
                if (NOT FMath::IsFinite(B) || NOT FMath::IsFinite(C) || NOT FMath::IsFinite(Discriminant))
                { return {}; }
                if (Discriminant < 0.0)
                { return FIntervalResult{true}; }

                const auto Root = FMath::Sqrt(FMath::Max(0.0, Discriminant));
                const auto Denominator = 2.0 * LengthSq;
                if (NOT FMath::IsFinite(Root) || NOT FMath::IsFinite(Denominator) || Denominator <= 0.0)
                { return {}; }
                const auto First = (-B - Root) / Denominator;
                const auto Last = (-B + Root) / Denominator;
                if (NOT FMath::IsFinite(First) || NOT FMath::IsFinite(Last))
                { return {}; }
                Horizontal = FIntervalResult{true, true, FMath::Max(0.0, First), FMath::Min(1.0, Last)};
                if (Horizontal._Enter > Horizontal._Exit)
                { return FIntervalResult{true}; }
            }

            const auto Enter = FMath::Max(Vertical._Enter, Horizontal._Enter);
            const auto Exit = FMath::Min(Vertical._Exit, Horizontal._Exit);
            return Enter <= Exit ? FIntervalResult{true, true, Enter, Exit} : FIntervalResult{true};
        }

        auto Get_ObbInterval(
            const FCk_GroundNav_DynamicObstacleObb& InObb,
            const FVector&                           InStart,
            const FVector&                           InEnd) -> FIntervalResult
        {
            const auto LocalStart = InObb._YawTransform.InverseTransformPositionNoScale(InStart);
            const auto LocalDelta = InObb._YawTransform.InverseTransformVectorNoScale(InEnd - InStart);
            if (NOT Get_IsFinite(LocalStart) || NOT Get_IsFinite(LocalDelta))
            { return {}; }
            auto Enter = 0.0;
            auto Exit = 1.0;

            for (auto Axis = 0; Axis < 3; ++Axis)
            {
                const auto Interval = Get_IntervalOnAxis(
                    LocalStart[Axis], LocalDelta[Axis], -InObb._WorldHalfExtents[Axis], InObb._WorldHalfExtents[Axis]);
                if (NOT Interval._IsValid)
                { return {}; }
                if (NOT Interval._IsIntersecting)
                { return FIntervalResult{true}; }
                Enter = FMath::Max(Enter, Interval._Enter);
                Exit = FMath::Min(Exit, Interval._Exit);
                if (Enter > Exit)
                { return FIntervalResult{true}; }
            }

            return FIntervalResult{true, true, Enter, Exit};
        }

        auto Get_IsDiscCoveringCell(
            const FCk_GroundNav_DynamicObstacleDisc& InDisc,
            const FVector2D&                          InCellMinXY,
            float                                     InCellSizeUu,
            float                                     InSurfaceZUu) -> TOptional<bool>
        {
            if (InSurfaceZUu < InDisc._Centre.Z - InDisc._VerticalHalfExtentUu ||
                InSurfaceZUu > InDisc._Centre.Z + InDisc._VerticalHalfExtentUu)
            { return false; }

            const auto Maximum = InCellMinXY + FVector2D{InCellSizeUu, InCellSizeUu};
            if (NOT Get_IsFinite(Maximum))
            { return {}; }
            const auto Closest = FVector2D{
                FMath::Clamp(InDisc._Centre.X, InCellMinXY.X, Maximum.X),
                FMath::Clamp(InDisc._Centre.Y, InCellMinXY.Y, Maximum.Y)};
            const auto DeltaX = static_cast<double>(Closest.X) - static_cast<double>(InDisc._Centre.X);
            const auto DeltaY = static_cast<double>(Closest.Y) - static_cast<double>(InDisc._Centre.Y);
            const auto Radius = static_cast<double>(InDisc._RadiusUu);
            const auto Distance = std::hypot(DeltaX, DeltaY);
            if (NOT FMath::IsFinite(Distance))
            { return {}; }
            return Distance <= Radius;
        }

        auto Get_IsPointInClosedConvexPolygon(const TArray<FVector2D, TInlineAllocator<4>>& InPolygon, const FVector2D& InPoint) -> TOptional<bool>
        {
            auto Positive = false;
            auto Negative = false;
            for (auto Index = 0; Index < InPolygon.Num(); ++Index)
            {
                const auto& A = InPolygon[Index];
                const auto& B = InPolygon[(Index + 1) % InPolygon.Num()];
                const auto EdgeX = static_cast<double>(B.X) - static_cast<double>(A.X);
                const auto EdgeY = static_cast<double>(B.Y) - static_cast<double>(A.Y);
                const auto PointX = static_cast<double>(InPoint.X) - static_cast<double>(A.X);
                const auto PointY = static_cast<double>(InPoint.Y) - static_cast<double>(A.Y);
                const auto Cross = (EdgeX * PointY) - (EdgeY * PointX);
                if (NOT FMath::IsFinite(Cross))
                { return {}; }
                Positive |= Cross > 0.0;
                Negative |= Cross < 0.0;
            }
            return NOT (Positive && Negative);
        }

        auto Get_IsObbCoveringCell(
            const FCk_GroundNav_DynamicObstacleObb& InObb,
            const FVector2D&                         InCellMinXY,
            float                                    InCellSizeUu,
            float                                    InSurfaceZUu) -> TOptional<bool>
        {
            auto Quad = TArray<FVector2D, TInlineAllocator<4>>{};
            const FVector Corners[] = {
                FVector{InCellMinXY.X, InCellMinXY.Y, InSurfaceZUu},
                FVector{InCellMinXY.X + InCellSizeUu, InCellMinXY.Y, InSurfaceZUu},
                FVector{InCellMinXY.X + InCellSizeUu, InCellMinXY.Y + InCellSizeUu, InSurfaceZUu},
                FVector{InCellMinXY.X, InCellMinXY.Y + InCellSizeUu, InSurfaceZUu}};

            auto MinimumZ = TNumericLimits<double>::Max();
            auto MaximumZ = TNumericLimits<double>::Lowest();
            for (const auto& Corner : Corners)
            {
                const auto Local = InObb._YawTransform.InverseTransformPositionNoScale(Corner);
                if (NOT Get_IsFinite(Local))
                { return {}; }
                Quad.Add(FVector2D{Local});
                MinimumZ = FMath::Min(MinimumZ, Local.Z);
                MaximumZ = FMath::Max(MaximumZ, Local.Z);
            }
            if (MinimumZ > InObb._WorldHalfExtents.Z || MaximumZ < -InObb._WorldHalfExtents.Z)
            { return false; }

            const auto Half = FVector2D{InObb._WorldHalfExtents};
            const FVector2D BoxCorners[] = {
                FVector2D{-Half.X, -Half.Y}, FVector2D{Half.X, -Half.Y},
                FVector2D{Half.X, Half.Y}, FVector2D{-Half.X, Half.Y}};

            for (const auto& Point : Quad)
            {
                if (FMath::Abs(Point.X) <= Half.X && FMath::Abs(Point.Y) <= Half.Y)
                { return true; }
            }
            for (const auto& Point : BoxCorners)
            {
                const auto IsInQuad = Get_IsPointInClosedConvexPolygon(Quad, Point);
                if (NOT IsInQuad.IsSet())
                { return {}; }
                if (IsInQuad.GetValue())
                { return true; }
            }

            struct FAxis
            {
                double _X;
                double _Y;
            };
            const auto IsSeparated = [&](const FAxis& InAxis) -> TOptional<bool>
            {
                auto PolygonMinimum = TNumericLimits<double>::Max();
                auto PolygonMaximum = TNumericLimits<double>::Lowest();
                for (const auto& Point : Quad)
                {
                    const auto Projection = (static_cast<double>(Point.X) * InAxis._X) +
                        (static_cast<double>(Point.Y) * InAxis._Y);
                    if (NOT FMath::IsFinite(Projection))
                    { return {}; }
                    PolygonMinimum = FMath::Min(PolygonMinimum, Projection);
                    PolygonMaximum = FMath::Max(PolygonMaximum, Projection);
                }
                const auto BoxRadius = (static_cast<double>(Half.X) * FMath::Abs(InAxis._X)) +
                    (static_cast<double>(Half.Y) * FMath::Abs(InAxis._Y));
                if (NOT FMath::IsFinite(BoxRadius))
                { return {}; }
                return PolygonMinimum > BoxRadius || PolygonMaximum < -BoxRadius;
            };

            const FAxis Axes[] = {
                FAxis{1.0, 0.0}, FAxis{0.0, 1.0},
                FAxis{static_cast<double>(Quad[1].X) - static_cast<double>(Quad[0].X),
                    static_cast<double>(Quad[1].Y) - static_cast<double>(Quad[0].Y)},
                FAxis{static_cast<double>(Quad[2].X) - static_cast<double>(Quad[1].X),
                    static_cast<double>(Quad[2].Y) - static_cast<double>(Quad[1].Y)}};
            for (const auto& Axis : Axes)
            {
                const auto IsAxisSeparated = IsSeparated(Axis);
                if (NOT IsAxisSeparated.IsSet())
                { return {}; }
                if (IsAxisSeparated.GetValue())
                { return false; }
            }
            return true;
        }
    }

    auto Try_MakeDynamicObstacleSnapshot(
        TConstArrayView<FCk_GroundNav_DynamicObstacleDisc> InDiscs,
        TConstArrayView<FCk_GroundNav_DynamicObstacleObb>  InObbs)
        -> TOptional<FCk_GroundNav_DynamicObstacleSnapshot>
    {
        using namespace dynamicobstacles_private;
        for (const auto& Disc : InDiscs)
        {
            if (NOT Get_IsValid(Disc))
            { return {}; }
        }
        for (const auto& Obb : InObbs)
        {
            if (NOT Get_IsValid(Obb))
            { return {}; }
        }

        auto Discs = TArray<FCk_GroundNav_DynamicObstacleDisc>{};
        auto Obbs = TArray<FCk_GroundNav_DynamicObstacleObb>{};
        Discs.Append(InDiscs);
        Obbs.Append(InObbs);
        auto Snapshot = FCk_GroundNav_DynamicObstacleSnapshot{MoveTemp(Discs), MoveTemp(Obbs)};
        Snapshot._HasDiscBroadPhase = NOT Snapshot._Discs.IsEmpty() && Try_BuildDiscBroadPhase(
            Snapshot._Discs, Snapshot._DiscBroadPhaseNodes, Snapshot._DiscBroadPhaseIndices);
        if (NOT Snapshot._HasDiscBroadPhase)
        {
            Snapshot._DiscBroadPhaseNodes.Reset();
            Snapshot._DiscBroadPhaseIndices.Reset();
        }
        return Snapshot;
    }

    auto Get_IsDynamicObstacleCoveringCell(
        const FCk_GroundNav_DynamicObstacleSnapshot& InSnapshot,
        const FVector2D&                             InCellMinXY,
        float                                        InCellSizeUu,
        float                                        InSurfaceZUu) -> TOptional<bool>
    {
        using namespace dynamicobstacles_private;
        if (NOT Get_IsFinite(InCellMinXY) || NOT FMath::IsFinite(InCellSizeUu) ||
            NOT FMath::IsFinite(InSurfaceZUu) || InCellSizeUu <= 0.0f)
        { return {}; }
        const auto CellMaximum = InCellMinXY + FVector2D{InCellSizeUu, InCellSizeUu};
        if (NOT Get_IsFinite(CellMaximum) || CellMaximum.X <= InCellMinXY.X || CellMaximum.Y <= InCellMinXY.Y)
        { return {}; }

        auto Candidates = TArray<int32, TInlineAllocator<16>>{};
        const auto MinimumX = std::nextafter(static_cast<double>(InCellMinXY.X), -std::numeric_limits<double>::infinity());
        const auto MinimumY = std::nextafter(static_cast<double>(InCellMinXY.Y), -std::numeric_limits<double>::infinity());
        const auto MinimumZ = std::nextafter(static_cast<double>(InSurfaceZUu), -std::numeric_limits<double>::infinity());
        const auto MaximumX = std::nextafter(static_cast<double>(CellMaximum.X), std::numeric_limits<double>::infinity());
        const auto MaximumY = std::nextafter(static_cast<double>(CellMaximum.Y), std::numeric_limits<double>::infinity());
        const auto MaximumZ = std::nextafter(static_cast<double>(InSurfaceZUu), std::numeric_limits<double>::infinity());
        const auto UsesBroadPhase = InSnapshot._HasDiscBroadPhase && Get_IsSafeBroadPhaseQueryBounds(
            MinimumX, MinimumY, MinimumZ, MaximumX, MaximumY, MaximumZ);
        if (UsesBroadPhase)
        {
            Candidates = Get_DiscBroadPhaseCandidates(MakeArrayView(InSnapshot._DiscBroadPhaseNodes), MakeArrayView(InSnapshot._DiscBroadPhaseIndices),
                MinimumX, MinimumY, MinimumZ, MaximumX, MaximumY, MaximumZ);
        }

        const auto CheckDisc = [&](const FCk_GroundNav_DynamicObstacleDisc& InDisc) -> TOptional<bool>
        {
            const auto IsCovering = Get_IsDiscCoveringCell(InDisc, InCellMinXY, InCellSizeUu, InSurfaceZUu);
            if (NOT IsCovering.IsSet())
            { return {}; }
            if (IsCovering.GetValue())
            { return true; }
            return false;
        };
        if (UsesBroadPhase)
        {
            for (const auto Index : Candidates)
            {
                const auto IsCovering = CheckDisc(InSnapshot._Discs[Index]);
                if (NOT IsCovering.IsSet()) { return {}; }
                if (IsCovering.GetValue()) { return true; }
            }
        }
        else for (const auto& Disc : InSnapshot.Get_Discs())
        {
            const auto IsCovering = CheckDisc(Disc);
            if (NOT IsCovering.IsSet()) { return {}; }
            if (IsCovering.GetValue()) { return true; }
        }
        for (const auto& Obb : InSnapshot.Get_Obbs())
        {
            const auto IsCovering = Get_IsObbCoveringCell(Obb, InCellMinXY, InCellSizeUu, InSurfaceZUu);
            if (NOT IsCovering.IsSet())
            { return {}; }
            if (IsCovering.GetValue())
            { return true; }
        }
        return false;
    }

    auto Get_DynamicUnionEdge(
        const FCk_GroundNav_DynamicObstacleSnapshot& InSnapshot,
        const FVector&                                InStart,
        const FVector&                                InEnd) -> TOptional<ECk_GroundNav_DynamicUnionEdge>
    {
        using namespace dynamicobstacles_private;
        if (NOT Get_IsFinite(InStart) || NOT Get_IsFinite(InEnd) ||
            NOT Get_IsFinite(InEnd - InStart))
        { return {}; }

        auto Intervals = TArray<TPair<double, double>, TInlineAllocator<16>>{};
        const auto DeltaX = static_cast<double>(InEnd.X) - static_cast<double>(InStart.X);
        const auto DeltaY = static_cast<double>(InEnd.Y) - static_cast<double>(InStart.Y);
        const auto DeltaZ = static_cast<double>(InEnd.Z) - static_cast<double>(InStart.Z);
        const auto MinimumX = std::nextafter(FMath::Min(static_cast<double>(InStart.X), static_cast<double>(InEnd.X)),
            -std::numeric_limits<double>::infinity());
        const auto MinimumY = std::nextafter(FMath::Min(static_cast<double>(InStart.Y), static_cast<double>(InEnd.Y)),
            -std::numeric_limits<double>::infinity());
        const auto MinimumZ = std::nextafter(FMath::Min(static_cast<double>(InStart.Z), static_cast<double>(InEnd.Z)),
            -std::numeric_limits<double>::infinity());
        const auto MaximumX = std::nextafter(FMath::Max(static_cast<double>(InStart.X), static_cast<double>(InEnd.X)),
            std::numeric_limits<double>::infinity());
        const auto MaximumY = std::nextafter(FMath::Max(static_cast<double>(InStart.Y), static_cast<double>(InEnd.Y)),
            std::numeric_limits<double>::infinity());
        const auto MaximumZ = std::nextafter(FMath::Max(static_cast<double>(InStart.Z), static_cast<double>(InEnd.Z)),
            std::numeric_limits<double>::infinity());
        const auto UsesBroadPhase = InSnapshot._HasDiscBroadPhase &&
            Get_IsSafeBroadPhaseDelta(DeltaX) && Get_IsSafeBroadPhaseDelta(DeltaY) && Get_IsSafeBroadPhaseDelta(DeltaZ) &&
            Get_IsSafeBroadPhaseQueryBounds(MinimumX, MinimumY, MinimumZ, MaximumX, MaximumY, MaximumZ);
        auto Candidates = TArray<int32, TInlineAllocator<16>>{};
        if (UsesBroadPhase)
        {
            Candidates = Get_DiscBroadPhaseCandidates(MakeArrayView(InSnapshot._DiscBroadPhaseNodes), MakeArrayView(InSnapshot._DiscBroadPhaseIndices),
                MinimumX, MinimumY, MinimumZ, MaximumX, MaximumY, MaximumZ);
        }

        const auto AddDiscInterval = [&](const FCk_GroundNav_DynamicObstacleDisc& InDisc) -> bool
        {
            if (NOT Get_IsFinite(InStart - InDisc._Centre) || NOT Get_IsFinite(InEnd - InDisc._Centre))
            { return false; }
            const auto Interval = Get_DiscInterval(InDisc, InStart, InEnd);
            if (NOT Interval._IsValid)
            { return false; }
            if (Interval._IsIntersecting)
            { Intervals.Add(TPair<double, double>{Interval._Enter, Interval._Exit}); }
            return true;
        };
        if (UsesBroadPhase)
        {
            for (const auto Index : Candidates)
            {
                if (NOT AddDiscInterval(InSnapshot._Discs[Index])) { return {}; }
            }
        }
        else for (const auto& Disc : InSnapshot.Get_Discs())
        {
            if (NOT AddDiscInterval(Disc)) { return {}; }
        }
        for (const auto& Obb : InSnapshot.Get_Obbs())
        {
            const auto Centre = Obb._YawTransform.GetLocation();
            if (NOT Get_IsFinite(InStart - Centre) || NOT Get_IsFinite(InEnd - Centre))
            { return {}; }
            const auto Interval = Get_ObbInterval(Obb, InStart, InEnd);
            if (NOT Interval._IsValid)
            { return {}; }
            if (Interval._IsIntersecting)
            { Intervals.Add(TPair<double, double>{Interval._Enter, Interval._Exit}); }
        }
        if (Intervals.IsEmpty())
        { return ECk_GroundNav_DynamicUnionEdge::Clear; }

        Intervals.Sort([](const auto& InLeft, const auto& InRight) { return InLeft.Key < InRight.Key; });
        auto Merged = TArray<TPair<double, double>, TInlineAllocator<16>>{};
        for (const auto& Interval : Intervals)
        {
            if (Merged.IsEmpty() || Interval.Key > Merged.Last().Value)
            { Merged.Add(Interval); }
            else
            { Merged.Last().Value = FMath::Max(Merged.Last().Value, Interval.Value); }
        }

        if (Merged.Num() != 1 || Merged[0].Key != 0.0)
        { return ECk_GroundNav_DynamicUnionEdge::NonMonotonic; }
        return Merged[0].Value == 1.0
            ? ECk_GroundNav_DynamicUnionEdge::InsideAll
            : ECk_GroundNav_DynamicUnionEdge::ExitsOnce;
    }
}

// --------------------------------------------------------------------------------------------------------------------
