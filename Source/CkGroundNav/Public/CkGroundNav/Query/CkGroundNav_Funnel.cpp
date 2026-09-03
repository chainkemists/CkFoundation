#include "CkGroundNav_Funnel.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace funnel_private
    {
        // A signed twice-area in uu^2 over inputs that are multiples of a cell size: anything this small is
        // collinearity rather than a turn, and reading it as a turn is what makes a degenerate portal emit a
        // corner that is not one.
        constexpr auto kAreaEpsilon = 1e-9;

        // Two waypoints closer than this in XY are the same corner.
        constexpr auto kSamePointUu = 1e-6;

        // ------------------------------------------------------------------------------------------------------------

        /** The funnel as the scan carries it: the corner the string is pinned at, and the two ends bounding it. */
        struct FFunnelState
        {
            FVector _Apex = FVector::ZeroVector;
            FVector _LeftVertex = FVector::ZeroVector;
            FVector _RightVertex = FVector::ZeroVector;
        };

        // ------------------------------------------------------------------------------------------------------------

        auto Get_TriArea2(
            const FVector& InA,
            const FVector& InB,
            const FVector& InC) -> double
        {
            return ((InB.X - InA.X) * (InC.Y - InA.Y)) - ((InC.X - InA.X) * (InB.Y - InA.Y));
        }

        auto Get_InterpolatedZ(
            const FVector& InA,
            const FVector& InB,
            double         InT) -> double
        {
            return FMath::Lerp(InA.Z, InB.Z, InT);
        }

        auto Get_DistanceXY(
            const FVector& InA,
            const FVector& InB) -> double
        {
            const auto DeltaX = InB.X - InA.X;
            const auto DeltaY = InB.Y - InA.Y;

            return FMath::Sqrt((DeltaX * DeltaX) + (DeltaY * DeltaY));
        }

        auto Get_IsSamePointXY(
            const FVector& InA,
            const FVector& InB) -> bool
        {
            return FMath::Abs(InA.X - InB.X) <= kSamePointUu &&
                   FMath::Abs(InA.Y - InB.Y) <= kSamePointUu;
        }

        auto Get_PointOnPortal(
            const FCk_GroundNav_FunnelPortal& InPortal,
            double                            InT) -> FVector
        {
            return FVector
            {
                FMath::Lerp(InPortal._Left.X, InPortal._Right.X, InT),
                FMath::Lerp(InPortal._Left.Y, InPortal._Right.Y, InT),
                Get_InterpolatedZ(InPortal._Left, InPortal._Right, InT)
            };
        }

        auto Make_PointPortal(
            const FVector& InPoint) -> FCk_GroundNav_FunnelPortal
        {
            auto Portal = FCk_GroundNav_FunnelPortal{};
            Portal._Left = InPoint;
            Portal._Right = InPoint;

            return Portal;
        }

        /**
         * The interval a body of a given radius can actually aim at: both ends pulled toward the midpoint by
         * the radius, each carrying the height interpolated at the parameter it was pulled to. An interval no
         * wider than the body collapses to its midpoint rather than inverting through itself.
         */
        auto Get_Inset(
            const FCk_GroundNav_FunnelPortal& InPortal,
            double                            InRadiusUu) -> FCk_GroundNav_FunnelPortal
        {
            // A negative radius would push each end past the other and hand the funnel an inverted portal.
            const auto RadiusUu = FMath::Max(InRadiusUu, 0.0);
            const auto WidthUu = Get_DistanceXY(InPortal._Left, InPortal._Right);

            if (WidthUu <= RadiusUu * 2.0)
            { return Make_PointPortal(Get_PointOnPortal(InPortal, 0.5)); }

            const auto T = RadiusUu / WidthUu;

            auto Inset = FCk_GroundNav_FunnelPortal{};
            Inset._Left = Get_PointOnPortal(InPortal, T);
            Inset._Right = Get_PointOnPortal(InPortal, 1.0 - T);

            return Inset;
        }

        auto DoAdd_Waypoint(
            TArray<FVector>& OutWaypoints,
            const FVector&   InPoint) -> void
        {
            if (OutWaypoints.Num() > 0 && Get_IsSamePointXY(OutWaypoints.Last(), InPoint))
            { return; }

            OutWaypoints.Add(InPoint);
        }

        auto Get_PathLengthXY(
            TConstArrayView<FVector> InWaypoints) -> double
        {
            auto LengthUu = 0.0;

            for (auto Index = 1; Index < InWaypoints.Num(); ++Index)
            { LengthUu += Get_DistanceXY(InWaypoints[Index - 1], InWaypoints[Index]); }

            return LengthUu;
        }

        // ------------------------------------------------------------------------------------------------------------

        /**
         * The simple stupid funnel over a portal list whose FIRST entry is the start as a zero-width portal —
         * that is what makes the apex an index into the same list, so a collapsed funnel restarts its scan at
         * the corner it just emitted instead of at the beginning.
         *
         * Emits the start and every corner the string bends at; the terminal point is the caller's, since only
         * the caller knows whether it is an end or a point chosen on a target. Returns the funnel as it stands
         * after the last portal, which is the wedge that terminal point has to lie inside.
         */
        auto Do_Funnel(
            TConstArrayView<FCk_GroundNav_FunnelPortal> InPortals,
            TArray<FVector>&                            OutWaypoints) -> FFunnelState
        {
            auto State = FFunnelState{};

            if (InPortals.IsEmpty())
            { return State; }

            State._Apex = InPortals[0]._Left;
            State._LeftVertex = InPortals[0]._Left;
            State._RightVertex = InPortals[0]._Right;

            auto ApexIndex = 0;
            auto LeftIndex = 0;
            auto RightIndex = 0;

            DoAdd_Waypoint(OutWaypoints, State._Apex);

            // Only a restart can fail to make progress — every other path through the body advances the index
            // — so restarts are what the cap has to bound. Each one moves the apex forward onto a corner just
            // emitted, so a sound scan cannot reach this many.
            auto RestartCount = 0;
            const auto RestartCap = 4 * (InPortals.Num() + 2);

            for (auto Index = 1; Index < InPortals.Num(); ++Index)
            {
                const auto RestartsAreBounded = RestartCount <= RestartCap;
                CK_ENSURE_IF_NOT(RestartsAreBounded,
                    TEXT("GroundNav funnel failed to converge over [{}] portals after [{}] apex restarts"),
                    InPortals.Num(), RestartCount)
                { return State; }

                const auto& NewLeft = InPortals[Index]._Left;
                const auto& NewRight = InPortals[Index]._Right;

                if (Get_TriArea2(State._Apex, State._RightVertex, NewRight) <= kAreaEpsilon)
                {
                    const auto FunnelStaysOpen =
                        Get_IsSamePointXY(State._Apex, State._RightVertex) ||
                        Get_TriArea2(State._Apex, State._LeftVertex, NewRight) >= -kAreaEpsilon;

                    if (FunnelStaysOpen)
                    {
                        State._RightVertex = NewRight;
                        RightIndex = Index;
                    }
                    else
                    {
                        DoAdd_Waypoint(OutWaypoints, State._LeftVertex);

                        State._Apex = State._LeftVertex;
                        State._RightVertex = State._LeftVertex;
                        ApexIndex = LeftIndex;
                        RightIndex = LeftIndex;

                        ++RestartCount;

                        Index = ApexIndex;
                        continue;
                    }
                }

                if (Get_TriArea2(State._Apex, State._LeftVertex, NewLeft) >= -kAreaEpsilon)
                {
                    const auto FunnelStaysOpen =
                        Get_IsSamePointXY(State._Apex, State._LeftVertex) ||
                        Get_TriArea2(State._Apex, State._RightVertex, NewLeft) <= kAreaEpsilon;

                    if (FunnelStaysOpen)
                    {
                        State._LeftVertex = NewLeft;
                        LeftIndex = Index;
                    }
                    else
                    {
                        DoAdd_Waypoint(OutWaypoints, State._RightVertex);

                        State._Apex = State._RightVertex;
                        State._LeftVertex = State._RightVertex;
                        ApexIndex = RightIndex;
                        LeftIndex = RightIndex;

                        ++RestartCount;

                        Index = ApexIndex;
                        continue;
                    }
                }
            }

            return State;
        }

        // ------------------------------------------------------------------------------------------------------------

        /**
         * Narrows [OutTMin, OutTMax] to the parameters where a boundary value running affinely from InAtZero
         * to InAtOne stays non-positive. A boundary parallel to the interval has no crossing to solve for and
         * constrains nothing, which is also why it is never divided by.
         */
        auto DoClip_NonPositive(
            double  InAtZero,
            double  InAtOne,
            double& OutTMin,
            double& OutTMax) -> void
        {
            const auto Slope = InAtOne - InAtZero;

            if (FMath::Abs(Slope) <= kAreaEpsilon)
            { return; }

            const auto Crossing = -InAtZero / Slope;

            if (Slope > 0.0)
            { OutTMax = FMath::Min(OutTMax, Crossing); }
            else
            { OutTMin = FMath::Max(OutTMin, Crossing); }
        }

        /**
         * The point of the target the apex reaches in a straight line. Each funnel boundary is a half-plane
         * whose value along the target is affine in the target's own parameter, so the visible span is an
         * interval in that parameter and the answer is the apex's projection clamped into it.
         */
        auto Get_VisibleTargetPoint(
            const FFunnelState&               InFunnel,
            const FCk_GroundNav_FunnelPortal& InTarget) -> FVector
        {
            auto TMin = 0.0;
            auto TMax = 1.0;

            const auto RightAtZero = Get_TriArea2(InFunnel._Apex, InFunnel._RightVertex, InTarget._Left);
            const auto RightAtOne = Get_TriArea2(InFunnel._Apex, InFunnel._RightVertex, InTarget._Right);
            const auto LeftAtZero = Get_TriArea2(InFunnel._Apex, InFunnel._LeftVertex, InTarget._Left);
            const auto LeftAtOne = Get_TriArea2(InFunnel._Apex, InFunnel._LeftVertex, InTarget._Right);

            DoClip_NonPositive(RightAtZero, RightAtOne, TMin, TMax);
            DoClip_NonPositive(-LeftAtZero, -LeftAtOne, TMin, TMax);

            TMin = FMath::Clamp(TMin, 0.0, 1.0);
            TMax = FMath::Clamp(TMax, 0.0, 1.0);

            // The two boundaries can cross each other on the target when the wedge only grazes it; the
            // parameter they cross at is the single point that was visible.
            if (TMin > TMax)
            {
                const auto Grazed = (TMin + TMax) * 0.5;
                TMin = Grazed;
                TMax = Grazed;
            }

            const auto DeltaX = InTarget._Right.X - InTarget._Left.X;
            const auto DeltaY = InTarget._Right.Y - InTarget._Left.Y;
            const auto LengthSquared = (DeltaX * DeltaX) + (DeltaY * DeltaY);

            const auto Projected = LengthSquared > 0.0
                ? (((InFunnel._Apex.X - InTarget._Left.X) * DeltaX) +
                   ((InFunnel._Apex.Y - InTarget._Left.Y) * DeltaY)) / LengthSquared
                : 0.0;

            return Get_PointOnPortal(InTarget, FMath::Clamp(Projected, TMin, TMax));
        }

        // ------------------------------------------------------------------------------------------------------------

        using FWorkingPortals = TArray<FCk_GroundNav_FunnelPortal, TInlineAllocator<32>>;

        auto DoBuild_WorkingPortals(
            const FVector&                              InStart,
            TConstArrayView<FCk_GroundNav_FunnelPortal> InPortals,
            double                                      InRadiusUu,
            FWorkingPortals&                            OutWorking) -> void
        {
            OutWorking.Reserve(InPortals.Num() + 2);
            OutWorking.Add(Make_PointPortal(InStart));

            for (const auto& Portal : InPortals)
            { OutWorking.Add(Get_Inset(Portal, InRadiusUu)); }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_StringPull(
            const FVector&                              InStart,
            const FVector&                              InEnd,
            TConstArrayView<FCk_GroundNav_FunnelPortal> InPortals,
            float                                       InRadiusUu,
            TArray<FVector>&                            OutWaypoints)
        -> double
    {
        using namespace funnel_private;

        OutWaypoints.Reset();

        auto Working = FWorkingPortals{};
        DoBuild_WorkingPortals(InStart, InPortals, static_cast<double>(InRadiusUu), Working);
        Working.Add(Make_PointPortal(InEnd));

        Do_Funnel(Working, OutWaypoints);

        DoAdd_Waypoint(OutWaypoints, InEnd);

        return Get_PathLengthXY(OutWaypoints);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_StringPull_ToSegment(
            const FVector&                              InStart,
            TConstArrayView<FCk_GroundNav_FunnelPortal> InPortals,
            const FCk_GroundNav_FunnelPortal&           InTarget,
            float                                       InRadiusUu,
            FVector&                                    OutPoint,
            TArray<FVector>&                            OutWaypoints)
        -> double
    {
        using namespace funnel_private;

        OutWaypoints.Reset();

        const auto RadiusUu = static_cast<double>(InRadiusUu);
        const auto Target = Get_Inset(InTarget, RadiusUu);

        auto Working = FWorkingPortals{};
        DoBuild_WorkingPortals(InStart, InPortals, RadiusUu, Working);

        // The target is the last portal and NOT a terminal point: the funnel has to consume it before the
        // apex knows which part of it it ends up seeing.
        Working.Add(Target);

        const auto Funnel = Do_Funnel(Working, OutWaypoints);

        OutPoint = Get_VisibleTargetPoint(Funnel, Target);

        DoAdd_Waypoint(OutWaypoints, OutPoint);

        return Get_PathLengthXY(OutWaypoints);
    }
}

// --------------------------------------------------------------------------------------------------------------------
