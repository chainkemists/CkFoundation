#include "CkGroundNav_MarkupMask.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace markupmask_private
    {
        // A cell square carries four corners; the slab clip a cylinder needs can cut two of its edges
        // and add two more.
        using FLocalPolygon = TArray<FVector, TInlineAllocator<8>>;

        auto Get_IsFinite(const FVector& InValue) -> bool
        {
            return FMath::IsFinite(InValue.X) && FMath::IsFinite(InValue.Y) && FMath::IsFinite(InValue.Z);
        }

        auto Get_ShapeExtentsAreUsable(const FCk_AnyShape& InShape) -> bool
        {
            switch (InShape.Get_ShapeType())
            {
                case ECk_Shape_Type::Box:
                {
                    const auto HalfExtents = InShape.Get_Box().Get_HalfExtents();

                    return Get_IsFinite(HalfExtents) &&
                        HalfExtents.X > 0.0 && HalfExtents.Y > 0.0 && HalfExtents.Z > 0.0;
                }

                case ECk_Shape_Type::Sphere:
                {
                    const auto Radius = InShape.Get_Sphere().Get_Radius();

                    return FMath::IsFinite(Radius) && Radius > 0.0f;
                }

                case ECk_Shape_Type::Capsule:
                {
                    const auto HalfHeight = InShape.Get_Capsule().Get_HalfHeight();
                    const auto Radius = InShape.Get_Capsule().Get_Radius();

                    // A zero half-height capsule is a ball, which is a volume; a zero radius one is a
                    // line segment, which is not.
                    return FMath::IsFinite(HalfHeight) && FMath::IsFinite(Radius) &&
                        HalfHeight >= 0.0f && Radius > 0.0f;
                }

                case ECk_Shape_Type::Cylinder:
                {
                    const auto HalfHeight = InShape.Get_Cylinder().Get_HalfHeight();
                    const auto Radius = InShape.Get_Cylinder().Get_Radius();

                    return FMath::IsFinite(HalfHeight) && FMath::IsFinite(Radius) &&
                        HalfHeight > 0.0f && Radius > 0.0f;
                }

                case ECk_Shape_Type::None:
                default:
                    return false;
            }
        }

        // Invertibility is the whole requirement: every covering test is evaluated in the shape's own
        // frame, and a transform with a collapsed axis has no frame to evaluate anything in.
        auto Get_TransformIsUsable(const FTransform& InTransform) -> bool
        {
            if (NOT InTransform.IsValid())
            { return false; }

            const auto Scale = InTransform.GetScale3D();

            return Get_IsFinite(InTransform.GetLocation()) && Get_IsFinite(Scale) &&
                Scale.X != 0.0 && Scale.Y != 0.0 && Scale.Z != 0.0;
        }

        auto Get_MarkupIsUsable(const FCk_GroundNav_MarkupRecord& InMarkup) -> bool
        {
            return Get_ShapeExtentsAreUsable(InMarkup.Get_Shape()) &&
                Get_TransformIsUsable(InMarkup.Get_WorldTransform());
        }

        // The cell's closed square at one surface height, carried into the shape's frame. Corners are
        // in perimeter order, so consecutive entries are edges and the affine map keeps them one.
        auto Get_LocalCellQuad(
            const FTransform& InTransform,
            const FVector2D&  InCellMinXY,
            double            InCellSizeUu,
            double            InSurfaceZUu) -> FLocalPolygon
        {
            const auto MaxX = InCellMinXY.X + InCellSizeUu;
            const auto MaxY = InCellMinXY.Y + InCellSizeUu;

            auto Quad = FLocalPolygon{};
            Quad.Add(InTransform.InverseTransformPosition(FVector{InCellMinXY.X, InCellMinXY.Y, InSurfaceZUu}));
            Quad.Add(InTransform.InverseTransformPosition(FVector{MaxX, InCellMinXY.Y, InSurfaceZUu}));
            Quad.Add(InTransform.InverseTransformPosition(FVector{MaxX, MaxY, InSurfaceZUu}));
            Quad.Add(InTransform.InverseTransformPosition(FVector{InCellMinXY.X, MaxY, InSurfaceZUu}));

            return Quad;
        }

        auto Get_DistanceSqToPolygon(const FLocalPolygon& InPolygon, const FVector& InPoint) -> double
        {
            auto Best = TNumericLimits<double>::Max();

            for (auto Index = 1; Index < InPolygon.Num() - 1; ++Index)
            {
                const auto Closest = FMath::ClosestPointOnTriangleToPoint(
                    InPoint, InPolygon[0], InPolygon[Index], InPolygon[Index + 1]);

                Best = FMath::Min(Best, FVector::DistSquared(Closest, InPoint));
            }

            return Best;
        }

        auto Get_PolygonIntersectsBall(
            const FLocalPolygon& InPolygon,
            const FVector&       InCentre,
            double               InRadius) -> bool
        {
            return Get_DistanceSqToPolygon(InPolygon, InCentre) <= (InRadius * InRadius);
        }

        // Separating-axis projection of the polygon against the canonical box. The axis need not be
        // normalized: both sides project onto the same vector, so a common scale cancels.
        auto Get_IsSeparatedAlong(
            const FLocalPolygon& InPolygon,
            const FVector&       InHalfExtents,
            const FVector&       InAxis) -> bool
        {
            if (InAxis.SizeSquared() <= 0.0)
            { return false; }

            const auto BoxRadius =
                (InHalfExtents.X * FMath::Abs(InAxis.X)) +
                (InHalfExtents.Y * FMath::Abs(InAxis.Y)) +
                (InHalfExtents.Z * FMath::Abs(InAxis.Z));

            auto PolygonMin = TNumericLimits<double>::Max();
            auto PolygonMax = TNumericLimits<double>::Lowest();

            for (const auto& Corner : InPolygon)
            {
                const auto Projection = FVector::DotProduct(Corner, InAxis);

                PolygonMin = FMath::Min(PolygonMin, Projection);
                PolygonMax = FMath::Max(PolygonMax, Projection);
            }

            // Touching is covering, so the intervals must be strictly apart to separate.
            return PolygonMin > BoxRadius || PolygonMax < -BoxRadius;
        }

        auto Get_PolygonIntersectsBox(const FLocalPolygon& InPolygon, const FVector& InHalfExtents) -> bool
        {
            const auto EdgeA = InPolygon[1] - InPolygon[0];
            const auto EdgeB = InPolygon[3] - InPolygon[0];

            const FVector Axes[] = {
                FVector::XAxisVector,
                FVector::YAxisVector,
                FVector::ZAxisVector,
                FVector::CrossProduct(EdgeA, EdgeB),
                FVector::CrossProduct(FVector::XAxisVector, EdgeA),
                FVector::CrossProduct(FVector::XAxisVector, EdgeB),
                FVector::CrossProduct(FVector::YAxisVector, EdgeA),
                FVector::CrossProduct(FVector::YAxisVector, EdgeB),
                FVector::CrossProduct(FVector::ZAxisVector, EdgeA),
                FVector::CrossProduct(FVector::ZAxisVector, EdgeB)};

            for (const auto& Axis : Axes)
            {
                if (Get_IsSeparatedAlong(InPolygon, InHalfExtents, Axis))
                { return false; }
            }

            return true;
        }

        // Keeps the part of the polygon where (InSign * Z) <= InLimit.
        auto Get_PolygonClippedToHalfSpace(
            const FLocalPolygon& InPolygon,
            double               InSign,
            double               InLimit) -> FLocalPolygon
        {
            auto Clipped = FLocalPolygon{};
            const auto Count = InPolygon.Num();

            for (auto Index = 0; Index < Count; ++Index)
            {
                const auto& Current = InPolygon[Index];
                const auto& Next = InPolygon[(Index + 1) % Count];

                const auto CurrentDistance = (InSign * Current.Z) - InLimit;
                const auto NextDistance = (InSign * Next.Z) - InLimit;

                if (CurrentDistance <= 0.0)
                { Clipped.Add(Current); }

                if ((CurrentDistance < 0.0 && NextDistance > 0.0) ||
                    (CurrentDistance > 0.0 && NextDistance < 0.0))
                {
                    const auto Alpha = CurrentDistance / (CurrentDistance - NextDistance);
                    Clipped.Add(Current + ((Next - Current) * Alpha));
                }
            }

            return Clipped;
        }

        auto Get_PolygonContainsOriginXy(const FLocalPolygon& InPolygon) -> bool
        {
            const auto Count = InPolygon.Num();

            if (Count < 3)
            { return false; }

            auto SawPositive = false;
            auto SawNegative = false;

            for (auto Index = 0; Index < Count; ++Index)
            {
                const auto& Current = InPolygon[Index];
                const auto& Next = InPolygon[(Index + 1) % Count];

                const auto Cross =
                    ((Next.X - Current.X) * -Current.Y) -
                    ((Next.Y - Current.Y) * -Current.X);

                SawPositive |= Cross > 0.0;
                SawNegative |= Cross < 0.0;
            }

            return NOT (SawPositive && SawNegative);
        }

        auto Get_MinXyDistanceSqToOrigin(const FLocalPolygon& InPolygon) -> double
        {
            auto Best = TNumericLimits<double>::Max();
            const auto Count = InPolygon.Num();

            for (auto Index = 0; Index < Count; ++Index)
            {
                const auto Current = FVector2D{InPolygon[Index].X, InPolygon[Index].Y};
                const auto Next = FVector2D{
                    InPolygon[(Index + 1) % Count].X,
                    InPolygon[(Index + 1) % Count].Y};

                const auto Edge = Next - Current;
                const auto EdgeLengthSq = Edge.SizeSquared();

                const auto Alpha = EdgeLengthSq > 0.0
                    ? FMath::Clamp(FVector2D::DotProduct(-Current, Edge) / EdgeLengthSq, 0.0, 1.0)
                    : 0.0;

                Best = FMath::Min(Best, (Current + (Edge * Alpha)).SizeSquared());
            }

            return Best;
        }

        // A cylinder is the intersection of a slab and an infinite round column, so the polygon is cut
        // to the slab first and the survivor's XY silhouette measured against the axis. Measuring the
        // whole polygon against the axis and testing its Z separately would admit a square that passes
        // beside the cylinder above one cap and below the other.
        auto Get_PolygonIntersectsCylinder(
            const FLocalPolygon& InPolygon,
            double               InHalfHeight,
            double               InRadius) -> bool
        {
            const auto Clipped = Get_PolygonClippedToHalfSpace(
                Get_PolygonClippedToHalfSpace(InPolygon, 1.0, InHalfHeight), -1.0, InHalfHeight);

            if (Clipped.IsEmpty())
            { return false; }

            if (Get_PolygonContainsOriginXy(Clipped))
            { return true; }

            return Get_MinXyDistanceSqToOrigin(Clipped) <= (InRadius * InRadius);
        }

        // A capsule is exactly its cylinder plus its two end balls: a point inside the slab is within
        // the radius of the axis, and one outside it within the radius of the nearer cap centre.
        auto Get_PolygonIntersectsCapsule(
            const FLocalPolygon& InPolygon,
            double               InHalfHeight,
            double               InRadius) -> bool
        {
            return Get_PolygonIntersectsCylinder(InPolygon, InHalfHeight, InRadius) ||
                Get_PolygonIntersectsBall(InPolygon, FVector{0.0, 0.0, -InHalfHeight}, InRadius) ||
                Get_PolygonIntersectsBall(InPolygon, FVector{0.0, 0.0, InHalfHeight}, InRadius);
        }

        // Half-size of the shape's world bounds along one world axis, from the shape's support in that
        // direction. InAxisComponents holds that world axis' component of each transformed local axis,
        // which IS the direction the support is taken along in the shape's own frame.
        auto Get_BoundsHalfSizeAlong(
            const FCk_AnyShape& InShape,
            const FVector&      InAxisComponents) -> double
        {
            const auto AbsX = FMath::Abs(InAxisComponents.X);
            const auto AbsY = FMath::Abs(InAxisComponents.Y);
            const auto AbsZ = FMath::Abs(InAxisComponents.Z);

            switch (InShape.Get_ShapeType())
            {
                case ECk_Shape_Type::Box:
                {
                    const auto HalfExtents = InShape.Get_Box().Get_HalfExtents();

                    return (HalfExtents.X * AbsX) + (HalfExtents.Y * AbsY) + (HalfExtents.Z * AbsZ);
                }

                case ECk_Shape_Type::Sphere:
                    return InShape.Get_Sphere().Get_Radius() * InAxisComponents.Size();

                case ECk_Shape_Type::Capsule:
                {
                    const auto& Dimensions = InShape.Get_Capsule();

                    return (Dimensions.Get_HalfHeight() * AbsZ) +
                        (Dimensions.Get_Radius() * InAxisComponents.Size());
                }

                case ECk_Shape_Type::Cylinder:
                {
                    const auto& Dimensions = InShape.Get_Cylinder();

                    return (Dimensions.Get_HalfHeight() * AbsZ) +
                        (Dimensions.Get_Radius() * FMath::Sqrt((AbsX * AbsX) + (AbsY * AbsY)));
                }

                case ECk_Shape_Type::None:
                default:
                    return 0.0;
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_MarkupWorldBounds(
            const FCk_GroundNav_MarkupRecord& InMarkup)
        -> FBox
    {
        using namespace markupmask_private;

        if (NOT Get_MarkupIsUsable(InMarkup))
        { return FBox{ForceInit}; }

        const auto& Transform = InMarkup.Get_WorldTransform();

        const auto LocalX = Transform.TransformVector(FVector::XAxisVector);
        const auto LocalY = Transform.TransformVector(FVector::YAxisVector);
        const auto LocalZ = Transform.TransformVector(FVector::ZAxisVector);

        const auto HalfSize = FVector{
            Get_BoundsHalfSizeAlong(InMarkup.Get_Shape(), FVector{LocalX.X, LocalY.X, LocalZ.X}),
            Get_BoundsHalfSizeAlong(InMarkup.Get_Shape(), FVector{LocalX.Y, LocalY.Y, LocalZ.Y}),
            Get_BoundsHalfSizeAlong(InMarkup.Get_Shape(), FVector{LocalX.Z, LocalY.Z, LocalZ.Z})};

        if (NOT Get_IsFinite(HalfSize) || HalfSize.X <= 0.0 || HalfSize.Y <= 0.0 || HalfSize.Z <= 0.0)
        { return FBox{ForceInit}; }

        const auto Centre = Transform.GetLocation();

        return FBox{Centre - HalfSize, Centre + HalfSize};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_MarkupCellRect(
            const FCk_GroundNav_MarkupRecord& InMarkup,
            const FVector2D&                  InLatticeOriginXY,
            float                             InCellSizeUu,
            int32                             InSizeX,
            int32                             InSizeY)
        -> TOptional<FCk_GroundNav_CellRect>
    {
        const auto Bounds = Get_MarkupWorldBounds(InMarkup);

        if (NOT Bounds.IsValid)
        { return {}; }

        if (NOT FMath::IsFinite(InCellSizeUu) || InCellSizeUu <= 0.0f || InSizeX <= 0 || InSizeY <= 0)
        { return {}; }

        const auto CellSize = static_cast<double>(InCellSizeUu);

        // A cell's square is CLOSED, so it counts when its far edge merely reaches the bound. The low
        // index is therefore the first cell whose MAX edge is at or past the minimum, and the high
        // index the last cell whose MIN edge is at or before the maximum. Exact comparisons: a bound
        // sitting on a cell line claims the cells on both sides of it.
        const auto MinX = FMath::CeilToInt32((Bounds.Min.X - InLatticeOriginXY.X) / CellSize) - 1;
        const auto MinY = FMath::CeilToInt32((Bounds.Min.Y - InLatticeOriginXY.Y) / CellSize) - 1;
        const auto MaxX = FMath::FloorToInt32((Bounds.Max.X - InLatticeOriginXY.X) / CellSize);
        const auto MaxY = FMath::FloorToInt32((Bounds.Max.Y - InLatticeOriginXY.Y) / CellSize);

        if (MaxX < 0 || MaxY < 0 || MinX > (InSizeX - 1) || MinY > (InSizeY - 1))
        { return {}; }

        auto Rect = FCk_GroundNav_CellRect{};
        Rect._MinX = FMath::Max(MinX, 0);
        Rect._MinY = FMath::Max(MinY, 0);
        Rect._MaxX = FMath::Min(MaxX, InSizeX - 1);
        Rect._MaxY = FMath::Min(MaxY, InSizeY - 1);

        return Rect;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsMarkupCoveringCell(
            const FCk_GroundNav_MarkupRecord& InMarkup,
            const FVector2D&                  InCellMinXY,
            float                             InCellSizeUu,
            float                             InSurfaceZUu)
        -> bool
    {
        using namespace markupmask_private;

        if (NOT Get_MarkupIsUsable(InMarkup))
        { return false; }

        const auto InputsAreFinite =
            FMath::IsFinite(InCellSizeUu) && FMath::IsFinite(InSurfaceZUu) &&
            FMath::IsFinite(InCellMinXY.X) && FMath::IsFinite(InCellMinXY.Y);

        if (NOT InputsAreFinite || InCellSizeUu <= 0.0f)
        { return false; }

        const auto Quad = Get_LocalCellQuad(
            InMarkup.Get_WorldTransform(),
            InCellMinXY,
            static_cast<double>(InCellSizeUu),
            static_cast<double>(InSurfaceZUu));

        const auto& Shape = InMarkup.Get_Shape();

        switch (Shape.Get_ShapeType())
        {
            case ECk_Shape_Type::Box:
                return Get_PolygonIntersectsBox(Quad, Shape.Get_Box().Get_HalfExtents());

            case ECk_Shape_Type::Sphere:
                return Get_PolygonIntersectsBall(
                    Quad, FVector::ZeroVector, Shape.Get_Sphere().Get_Radius());

            case ECk_Shape_Type::Capsule:
                return Get_PolygonIntersectsCapsule(
                    Quad,
                    Shape.Get_Capsule().Get_HalfHeight(),
                    Shape.Get_Capsule().Get_Radius());

            case ECk_Shape_Type::Cylinder:
                return Get_PolygonIntersectsCylinder(
                    Quad,
                    Shape.Get_Cylinder().Get_HalfHeight(),
                    Shape.Get_Cylinder().Get_Radius());

            case ECk_Shape_Type::None:
            default:
                return false;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
