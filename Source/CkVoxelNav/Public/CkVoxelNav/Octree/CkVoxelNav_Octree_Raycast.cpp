#include "CkVoxelNav_Octree_Raycast.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Morton.h"
#include "CkVoxelNav/Octree/CkVoxelNav_Octree_Query.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_octree_raycast
{
    using namespace ck::voxelnav;

    // A normalized direction component below this reads as axis-parallel. Dividing by it yields parametric
    // values of ~1e7 at volume scale, which sit outside every node's span in both directions and therefore
    // never select a midplane - exactly the behaviour a parallel axis should have.
    constexpr auto MinDirectionComponent = 1.0e-4;

    constexpr auto ChildSlotCount = 8;

    // The transition table's "the segment has left this node" answer.
    constexpr auto ChildWalkComplete = 8;

    // ----------------------------------------------------------------------------------------------------------------

    /** The segment's parametric span across one node, per axis, in the MIRRORED space where every
     *  direction component is positive. A child's span is the parent's halved at the midplanes, which is
     *  what lets the walk descend without touching a single node's geometry. */
    struct FOctreeRay
    {
        FOctreeRay() = default;

        FOctreeRay(
            double InTx0, double InTx1,
            double InTy0, double InTy1,
            double InTz0, double InTz1)
            : _Tx0(InTx0), _Tx1(InTx1), _Txm(0.5 * (InTx0 + InTx1))
            , _Ty0(InTy0), _Ty1(InTy1), _Tym(0.5 * (InTy0 + InTy1))
            , _Tz0(InTz0), _Tz1(InTz1), _Tzm(0.5 * (InTz0 + InTz1))
        {}

        auto Get_Intersects() const -> bool
        {
            return FMath::Max3(_Tx0, _Ty0, _Tz0) < FMath::Min3(_Tx1, _Ty1, _Tz1);
        }

        /** False once the node lies wholly behind the segment's start or wholly past its end. This is the
         *  only thing bounding the walk to a SEGMENT rather than an infinite ray. */
        auto Get_OverlapsSegment(
            double InSegmentLength) const -> bool
        {
            return _Tx1 >= 0.0 && _Ty1 >= 0.0 && _Tz1 >= 0.0 &&
                   _Tx0 <= InSegmentLength && _Ty0 <= InSegmentLength && _Tz0 <= InSegmentLength;
        }

        double _Tx0 = 0.0, _Tx1 = 0.0, _Txm = 0.0;
        double _Ty0 = 0.0, _Ty1 = 0.0, _Tym = 0.0;
        double _Tz0 = 0.0, _Tz1 = 0.0, _Tzm = 0.0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** The segment in WORLD space, plus the per-axis mirror flags that let one positive-direction walk
     *  serve all eight direction octants. The flags un-mirror a child SLOT back to a real child index;
     *  every geometric test uses the world-space fields, so no result ever needs transforming back. */
    struct FSegment
    {
        FVector _Origin = FVector::ZeroVector;
        FVector _Direction = FVector::ZeroVector;
        double _Length = 0.0;
        uint8 _MirroredAxes = 0;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /** Which of the eight child slots the segment enters first, per Revelles: take the axis it enters the
     *  node through (the largest entry parameter), then ask, at that parameter, whether it is already past
     *  each of the other two MIDPLANES. */
    auto
        Get_FirstChildSlot(
            const FOctreeRay& InRay)
        -> int32
    {
        auto Slot = 0;

        if (InRay._Tx0 > InRay._Ty0)
        {
            if (InRay._Tx0 > InRay._Tz0)
            {
                if (InRay._Tym < InRay._Tx0)
                { Slot |= 2; }

                if (InRay._Tzm < InRay._Tx0)
                { Slot |= 4; }

                return Slot;
            }
        }
        else
        {
            if (InRay._Ty0 > InRay._Tz0)
            {
                if (InRay._Txm < InRay._Ty0)
                { Slot |= 1; }

                if (InRay._Tzm < InRay._Ty0)
                { Slot |= 4; }

                return Slot;
            }
        }

        if (InRay._Txm < InRay._Tz0)
        { Slot |= 1; }

        if (InRay._Tym < InRay._Tz0)
        { Slot |= 2; }

        return Slot;
    }

    /** Which slot the segment enters next: whichever exit plane of the current child it reaches first. */
    auto
        Get_NextChildSlot(
            double InTx, int32 InSlotOnX,
            double InTy, int32 InSlotOnY,
            double InTz, int32 InSlotOnZ)
        -> int32
    {
        if (InTx < InTy)
        {
            if (InTx < InTz)
            { return InSlotOnX; }
        }
        else
        {
            if (InTy < InTz)
            { return InSlotOnY; }
        }

        return InSlotOnZ;
    }

    // ----------------------------------------------------------------------------------------------------------------

    /** The outward normal of the cell face the segment entered through. A point on an axis-aligned box's
     *  surface is furthest from the centre along the axis of the face it sits on, so the largest component
     *  names the face and its sign names the direction. */
    auto
        Get_FaceNormal(
            const FVector& InImpactPoint,
            const FVector& InCellCenter)
        -> FVector
    {
        const auto ToCenter = (InCellCenter - InImpactPoint).GetAbs();

        if (ToCenter.X >= ToCenter.Y && ToCenter.X >= ToCenter.Z)
        { return FVector{InImpactPoint.X > InCellCenter.X ? 1.0 : -1.0, 0.0, 0.0}; }

        if (ToCenter.Y >= ToCenter.X && ToCenter.Y >= ToCenter.Z)
        { return FVector{0.0, InImpactPoint.Y > InCellCenter.Y ? 1.0 : -1.0, 0.0}; }

        return FVector{0.0, 0.0, InImpactPoint.Z > InCellCenter.Z ? 1.0 : -1.0};
    }

    /** Slab test against one cell, in world space and in DISTANCE along the segment. The parametric walk
     *  already knows the segment reaches this cell; what it does not carry is where, and the impact point
     *  is the whole reason a caller asks for a result rather than a bool. */
    auto
        Get_SegmentEntersBox(
            const FBox& InBox,
            const FSegment& InSegment,
            double& OutEntryDistance)
        -> bool
    {
        auto TEnter = -TNumericLimits<double>::Max();
        auto TExit = TNumericLimits<double>::Max();

        for (auto Axis = 0; Axis < 3; ++Axis)
        {
            const auto Origin = InSegment._Origin[Axis];
            const auto Direction = InSegment._Direction[Axis];

            if (FMath::Abs(Direction) < MinDirectionComponent)
            {
                if (Origin < InBox.Min[Axis] || Origin > InBox.Max[Axis])
                { return false; }

                continue;
            }

            const auto InverseDirection = 1.0 / Direction;

            auto TNear = (InBox.Min[Axis] - Origin) * InverseDirection;
            auto TFar = (InBox.Max[Axis] - Origin) * InverseDirection;

            if (TNear > TFar)
            { Swap(TNear, TFar); }

            TEnter = FMath::Max(TNear, TEnter);
            TExit = FMath::Min(TFar, TExit);

            if (TEnter > TExit)
            { return false; }
        }

        if (TExit < 0.0 || TEnter > InSegment._Length)
        { return false; }

        // A segment that STARTS inside the cell enters it at zero distance, which is what a caller
        // standing in geometry must be told.
        OutEntryDistance = FMath::Max(TEnter, 0.0);

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoRecordHit(
            const FSegment& InSegment,
            const FNodeAddress& InCell,
            const FVector& InCellCenter,
            double InEntryDistance,
            FCk_VoxelNav_RaycastResult& InOutResult)
        -> void
    {
        if (InOutResult._IsBlocked && InOutResult._Distance <= InEntryDistance)
        { return; }

        InOutResult._IsBlocked = true;
        InOutResult._Distance = InEntryDistance;
        InOutResult._ImpactPoint = InSegment._Origin + InSegment._Direction * InEntryDistance;
        InOutResult._ImpactNormal = Get_FaceNormal(InOutResult._ImpactPoint, InCellCenter);
        InOutResult._BlockingCell = InCell;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoTraverseCell(
            const FOctreeRay& InRay,
            const FNodeAddress& InCell,
            const FOctree& InOctree,
            const FSegment& InSegment,
            FCk_VoxelNav_RaycastResult& InOutResult) -> bool;

    /** A layer-0 cell. The leaf store is parallel to layer 0, so the node index IS the leaf index. */
    auto
        DoTraverseLeaf(
            const FNodeAddress& InCell,
            const FOctree& InOctree,
            const FSegment& InSegment,
            FCk_VoxelNav_RaycastResult& InOutResult)
        -> bool
    {
        const auto LeafIdx = InCell.Get_NodeIndex();
        const auto& LeafNodes = InOctree.Get_LeafNodes();

        if (NOT LeafNodes.Get_LeafNodes().IsValidIndex(LeafIdx) ||
            NOT InOctree.Get_Layer(0).Get_Nodes().IsValidIndex(LeafIdx))
        { return false; }

        const auto& LeafNode = LeafNodes.Get_LeafNode(LeafIdx);

        if (LeafNode.Get_IsFullyFree())
        { return false; }

        const auto LeafCenter =
            Get_LeafNodePositionFromMorton(InOctree, InOctree.Get_Layer(0).Get_Node(LeafIdx).Get_MortonCode());
        const auto LeafExtent = static_cast<double>(LeafNodes.Get_LeafNodeExtent());

        auto LeafEntryDistance = 0.0;

        if (NOT Get_SegmentEntersBox(
            FBox::BuildAABB(LeafCenter, FVector{LeafExtent}), InSegment, LeafEntryDistance))
        { return false; }

        if (LeafNode.Get_IsFullyOccluded())
        {
            DoRecordHit(InSegment, InCell, LeafCenter, LeafEntryDistance, InOutResult);
            return true;
        }

        // Sub-node centres are computed exactly as the rasterizer placed them, so a hit reports the same
        // cell the bake wrote. Sub-nodes are disjoint, so the smallest entry distance among the ones the
        // segment enters IS the closest hit inside this leaf.
        const auto SubNodeSize = static_cast<double>(LeafNodes.Get_LeafSubNodeSize());
        const auto SubNodeExtent = static_cast<double>(LeafNodes.Get_LeafSubNodeExtent());
        const auto LeafOrigin = LeafCenter - FVector{LeafExtent};

        auto ClosestDistance = TNumericLimits<double>::Max();
        auto ClosestCell = FNodeAddress{};
        auto ClosestCenter = FVector::ZeroVector;

        for (auto Candidate = 0; Candidate <= FNodeAddress::MaxSubNodeIndex; ++Candidate)
        {
            const auto SubNodeIdx = static_cast<SubNodeIndex>(Candidate);

            if (NOT LeafNode.Get_IsSubNodeOccluded(SubNodeIdx))
            { continue; }

            const auto SubNodeCenter =
                LeafOrigin + Get_VectorFromMorton(SubNodeIdx) * SubNodeSize + FVector{SubNodeExtent};

            auto SubNodeEntryDistance = 0.0;

            if (NOT Get_SegmentEntersBox(
                FBox::BuildAABB(SubNodeCenter, FVector{SubNodeExtent}), InSegment, SubNodeEntryDistance))
            { continue; }

            if (SubNodeEntryDistance >= ClosestDistance)
            { continue; }

            ClosestDistance = SubNodeEntryDistance;
            ClosestCell = FNodeAddress::Make(0, LeafIdx, SubNodeIdx);
            ClosestCenter = SubNodeCenter;
        }

        if (NOT ClosestCell.Get_IsValid())
        { return false; }

        DoRecordHit(InSegment, ClosestCell, ClosestCenter, ClosestDistance, InOutResult);

        return true;
    }

    /** A cell above layer 0. Childless means the whole subtree is navigable free space, which is the
     *  hierarchy's entire pruning power - most of a volume is answered here without a geometric test. */
    auto
        DoTraverseBranch(
            const FOctreeRay& InRay,
            const FNodeAddress& InCell,
            const FOctree& InOctree,
            const FSegment& InSegment,
            FCk_VoxelNav_RaycastResult& InOutResult)
        -> bool
    {
        const auto LayerIdx = InCell.Get_LayerIndex();

        if (LayerIdx >= InOctree.Get_LayerCount())
        { return false; }

        const auto& Layer = InOctree.Get_Layer(LayerIdx);

        if (NOT Layer.Get_Nodes().IsValidIndex(InCell.Get_NodeIndex()))
        { return false; }

        const auto FirstChild = Layer.Get_Node(InCell.Get_NodeIndex()).Get_FirstChild();

        if (NOT FirstChild.Get_IsValid())
        { return false; }

        auto ChildSlot = Get_FirstChildSlot(InRay);

        while (ChildSlot < ChildSlotCount)
        {
            auto ChildRay = FOctreeRay{};
            auto NextSlot = ChildWalkComplete;

            switch (ChildSlot)
            {
                case 0:
                    ChildRay = FOctreeRay{InRay._Tx0, InRay._Txm, InRay._Ty0, InRay._Tym, InRay._Tz0, InRay._Tzm};
                    NextSlot = Get_NextChildSlot(InRay._Txm, 1, InRay._Tym, 2, InRay._Tzm, 4);
                    break;

                case 1:
                    ChildRay = FOctreeRay{InRay._Txm, InRay._Tx1, InRay._Ty0, InRay._Tym, InRay._Tz0, InRay._Tzm};
                    NextSlot = Get_NextChildSlot(InRay._Tx1, ChildWalkComplete, InRay._Tym, 3, InRay._Tzm, 5);
                    break;

                case 2:
                    ChildRay = FOctreeRay{InRay._Tx0, InRay._Txm, InRay._Tym, InRay._Ty1, InRay._Tz0, InRay._Tzm};
                    NextSlot = Get_NextChildSlot(InRay._Txm, 3, InRay._Ty1, ChildWalkComplete, InRay._Tzm, 6);
                    break;

                case 3:
                    ChildRay = FOctreeRay{InRay._Txm, InRay._Tx1, InRay._Tym, InRay._Ty1, InRay._Tz0, InRay._Tzm};
                    NextSlot = Get_NextChildSlot(
                        InRay._Tx1, ChildWalkComplete, InRay._Ty1, ChildWalkComplete, InRay._Tzm, 7);
                    break;

                case 4:
                    ChildRay = FOctreeRay{InRay._Tx0, InRay._Txm, InRay._Ty0, InRay._Tym, InRay._Tzm, InRay._Tz1};
                    NextSlot = Get_NextChildSlot(InRay._Txm, 5, InRay._Tym, 6, InRay._Tz1, ChildWalkComplete);
                    break;

                case 5:
                    ChildRay = FOctreeRay{InRay._Txm, InRay._Tx1, InRay._Ty0, InRay._Tym, InRay._Tzm, InRay._Tz1};
                    NextSlot = Get_NextChildSlot(
                        InRay._Tx1, ChildWalkComplete, InRay._Tym, 7, InRay._Tz1, ChildWalkComplete);
                    break;

                case 6:
                    ChildRay = FOctreeRay{InRay._Tx0, InRay._Txm, InRay._Tym, InRay._Ty1, InRay._Tzm, InRay._Tz1};
                    NextSlot = Get_NextChildSlot(
                        InRay._Txm, 7, InRay._Ty1, ChildWalkComplete, InRay._Tz1, ChildWalkComplete);
                    break;

                case 7:
                    ChildRay = FOctreeRay{InRay._Txm, InRay._Tx1, InRay._Tym, InRay._Ty1, InRay._Tzm, InRay._Tz1};
                    NextSlot = ChildWalkComplete;
                    break;

                default:
                    return false;
            }

            // The eight children of a node are contiguous in Morton order, and the slot is expressed in the
            // mirrored space, so un-mirroring it recovers the real child.
            const auto ChildNodeIndex =
                FirstChild.Get_NodeIndex() +
                static_cast<NodeIndex>(ChildSlot ^ static_cast<int32>(InSegment._MirroredAxes));

            if (DoTraverseCell(ChildRay, FNodeAddress::Make(FirstChild.Get_LayerIndex(), ChildNodeIndex),
                InOctree, InSegment, InOutResult))
            { return true; }

            ChildSlot = NextSlot;
        }

        return false;
    }

    auto
        DoTraverseCell(
            const FOctreeRay& InRay,
            const FNodeAddress& InCell,
            const FOctree& InOctree,
            const FSegment& InSegment,
            FCk_VoxelNav_RaycastResult& InOutResult)
        -> bool
    {
        if (NOT InRay.Get_OverlapsSegment(InSegment._Length))
        { return false; }

        if (InCell.Get_LayerIndex() == 0)
        { return DoTraverseLeaf(InCell, InOctree, InSegment, InOutResult); }

        return DoTraverseBranch(InRay, InCell, InOctree, InSegment, InOutResult);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    auto
        Raycast(
            const FOctree& InOctree,
            const FVector& InFrom,
            const FVector& InTo)
        -> FCk_VoxelNav_RaycastResult
    {
        using namespace ck_voxelnav_octree_raycast;

        QUICK_SCOPE_CYCLE_COUNTER(VoxelNav_Raycast);

        auto Result = FCk_VoxelNav_RaycastResult{};

        const auto OctreeIsBuilt = InOctree.Get_IsValid();

        CK_ENSURE_IF_NOT(OctreeIsBuilt,
            TEXT("Cannot raycast against a VoxelNav octree that has not been built"))
        {}

        if (NOT OctreeIsBuilt)
        { return Result; }

        const auto Delta = InTo - InFrom;
        const auto SegmentLength = Delta.Size();

        if (FMath::IsNearlyZero(SegmentLength))
        { return Result; }

        auto Segment = FSegment{};
        Segment._Origin = InFrom;
        Segment._Direction = Delta / SegmentLength;
        Segment._Length = SegmentLength;

        // The walk only handles positive direction components, so a negative axis is mirrored about the
        // volume's centre - which maps the navigation bounds onto themselves, since the centre is their
        // midpoint - and the mirror is undone per child slot during descent.
        const auto& NavigationBounds = InOctree.Get_NavigationBounds();
        const auto VolumeCenter = NavigationBounds.GetCenter();

        auto MirroredOrigin = InFrom;
        auto MirroredDirection = Segment._Direction;

        for (auto Axis = 0; Axis < 3; ++Axis)
        {
            if (FMath::Abs(MirroredDirection[Axis]) < MinDirectionComponent)
            {
                MirroredDirection[Axis] = MinDirectionComponent;
                continue;
            }

            if (MirroredDirection[Axis] >= 0.0)
            { continue; }

            MirroredOrigin[Axis] = VolumeCenter[Axis] * 2.0 - MirroredOrigin[Axis];
            MirroredDirection[Axis] = -MirroredDirection[Axis];
            Segment._MirroredAxes = static_cast<uint8>(Segment._MirroredAxes | (1 << Axis));
        }

        const auto RootRay = FOctreeRay
        {
            (NavigationBounds.Min.X - MirroredOrigin.X) / MirroredDirection.X,
            (NavigationBounds.Max.X - MirroredOrigin.X) / MirroredDirection.X,
            (NavigationBounds.Min.Y - MirroredOrigin.Y) / MirroredDirection.Y,
            (NavigationBounds.Max.Y - MirroredOrigin.Y) / MirroredDirection.Y,
            (NavigationBounds.Min.Z - MirroredOrigin.Z) / MirroredDirection.Z,
            (NavigationBounds.Max.Z - MirroredOrigin.Z) / MirroredDirection.Z
        };

        if (NOT RootRay.Get_Intersects())
        { return Result; }

        const auto RootLayer = static_cast<LayerIndex>(InOctree.Get_LayerCount() - 1);

        DoTraverseCell(RootRay, FNodeAddress::Make(RootLayer, 0), InOctree, Segment, Result);

        return Result;
    }

    auto
        Get_IsSegmentBlocked(
            const FOctree& InOctree,
            const FVector& InFrom,
            const FVector& InTo)
        -> bool
    {
        return Raycast(InOctree, InFrom, InTo)._IsBlocked;
    }
}

// --------------------------------------------------------------------------------------------------------------------
