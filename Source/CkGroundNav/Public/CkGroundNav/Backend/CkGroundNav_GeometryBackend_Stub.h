#pragma once

#include "CkGroundNav/Backend/CkGroundNav_GeometryBackend.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * Test double: the geometry surface over a hand-authored list of world-space boxes, so a bake can be
     * driven to completion with no physics world, no level and no subsystems — pure math, and the
     * expected output of every stage is computable by hand.
     *
     * Each box contributes its 12 triangles. Overlap is exact AABB-vs-AABB, and FBox::Intersect counts a
     * shared face as an intersection, so a fixture placed flush against a bounds edge reads as
     * overlapping. Author layouts that clearly straddle or clear the boundary; one that touches it
     * exactly is asserting a tie-break rather than the behavior under test.
     *
     * No thread affinity of its own — it reads nothing but its own box list.
     */
    class FCk_GroundNav_GeometryBackend_Stub final : public ICk_GroundNav_GeometryBackend
    {
    public:
        FCk_GroundNav_GeometryBackend_Stub() = default;

        explicit FCk_GroundNav_GeometryBackend_Stub(
            TArray<FBox> InBoxes)
            : _Boxes(MoveTemp(InBoxes))
        { }

    public:
        auto
        Get_IsValid() const -> bool override
        {
            return true;
        }

        auto
        Get_HasGeometryInBounds(
            const FBox& InBounds) const -> bool override
        {
            return _Boxes.ContainsByPredicate([&](const FBox& InBox) -> bool
            { return InBox.Intersect(InBounds); });
        }

        auto
        Get_StaticBodiesInBounds(
            const FBox& InBounds,
            TArray<FCk_GroundNav_BodyRef>& OutBodies) const -> int32 override
        {
            OutBodies.Reset();

            for (auto Index = 0; Index < _Boxes.Num(); ++Index)
            {
                if (NOT _Boxes[Index].Intersect(InBounds))
                { continue; }

                // +1 so index 0 is not the never-a-body sentinel.
                OutBodies.Emplace(FCk_GroundNav_BodyRef{static_cast<uint64>(Index) + 1});
            }

            return OutBodies.Num();
        }

        auto
        Get_TrianglesInBounds(
            const FBox& InBounds,
            FCk_GroundNav_GeometryBatch& OutBatch) const -> int32 override
        {
            const auto TrianglesBefore = OutBatch.Get_TriangleCount();

            for (const auto& Box : _Boxes)
            {
                if (NOT Box.Intersect(InBounds))
                { continue; }

                OutBatch.Add_Box(Box);
            }

            return OutBatch.Get_TriangleCount() - TrianglesBefore;
        }

    public:
        auto Get_Boxes() const -> const TArray<FBox>& { return _Boxes; }

    private:
        TArray<FBox> _Boxes;
    };
}

// --------------------------------------------------------------------------------------------------------------------
