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
     * PANELS are the deliberately-open fixture: one quad, two triangles, no other faces — a fence plane,
     * or a wall whose underside was never modelled. They exist so the closure contract can be tested
     * against exactly the geometry it refuses. Bodies are numbered boxes first, then panels, each +1 so
     * index 0 is never a body.
     *
     * No thread affinity of its own — it reads nothing but its own box and panel lists.
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
            const auto AnyBox = _Boxes.ContainsByPredicate([&](const FBox& InBox) -> bool
            { return InBox.Intersect(InBounds); });

            const auto AnyPanel = _Panels.ContainsByPredicate([&](const FPanel& InPanel) -> bool
            { return InPanel.Get_Bounds().Intersect(InBounds); });

            return AnyBox || AnyPanel;
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

            for (auto Index = 0; Index < _Panels.Num(); ++Index)
            {
                if (NOT _Panels[Index].Get_Bounds().Intersect(InBounds))
                { continue; }

                OutBodies.Emplace(FCk_GroundNav_BodyRef{static_cast<uint64>(_Boxes.Num() + Index) + 1});
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

            for (const auto& Panel : _Panels)
            {
                if (NOT Panel.Get_Bounds().Intersect(InBounds))
                { continue; }

                Panel.Add_To(OutBatch);
            }

            return OutBatch.Get_TriangleCount() - TrianglesBefore;
        }

        auto
        Get_WorldRevision() const -> uint64 override
        {
            return _WorldRevision;
        }

        auto
        Get_BodyKind(
            const FCk_GroundNav_BodyRef& InBody) const -> ECk_GroundNav_BodyKind override
        {
            const auto* Panel = Get_Panel(InBody);

            return Panel != nullptr ? Panel->_Kind : ECk_GroundNav_BodyKind::Solid;
        }

        auto
        Get_BodyBounds(
            const FCk_GroundNav_BodyRef& InBody) const -> FBox override
        {
            if (const auto* Box = Get_Box(InBody); Box != nullptr)
            { return *Box; }

            if (const auto* Panel = Get_Panel(InBody); Panel != nullptr)
            { return Panel->Get_Bounds(); }

            return FBox{ForceInit};
        }

        auto
        Get_BodyTriangles(
            const FCk_GroundNav_BodyRef& InBody,
            FCk_GroundNav_GeometryBatch& OutBatch) const -> int32 override
        {
            const auto TrianglesBefore = OutBatch.Get_TriangleCount();

            if (const auto* Box = Get_Box(InBody); Box != nullptr)
            { OutBatch.Add_Box(*Box); }
            else if (const auto* Panel = Get_Panel(InBody); Panel != nullptr)
            { Panel->Add_To(OutBatch); }

            return OutBatch.Get_TriangleCount() - TrianglesBefore;
        }

        auto
        Get_BodyDescription(
            const FCk_GroundNav_BodyRef& InBody) const -> FString override
        {
            if (Get_Box(InBody) != nullptr)
            { return FString::Printf(TEXT("Stub box %llu"), InBody._Value); }

            if (Get_Panel(InBody) != nullptr)
            { return FString::Printf(TEXT("Stub panel %llu"), InBody._Value - static_cast<uint64>(_Boxes.Num())); }

            return FString::Printf(TEXT("Stub body %llu (not held)"), InBody._Value);
        }

    public:
        auto Get_Boxes() const -> const TArray<FBox>& { return _Boxes; }

        /**
         * Add one open quad: corners in order around the panel, two triangles, and nothing else. Solid
         * by default so it VIOLATES the closure contract, which is what a test of that contract needs;
         * pass Surface to author the exempt kind instead. Returns the new body's ref.
         */
        auto Add_Panel(
            const FVector& InA,
            const FVector& InB,
            const FVector& InC,
            const FVector& InD,
            ECk_GroundNav_BodyKind InKind = ECk_GroundNav_BodyKind::Solid) -> FCk_GroundNav_BodyRef
        {
            _Panels.Emplace(FPanel{InA, InB, InC, InD, InKind});

            return FCk_GroundNav_BodyRef{static_cast<uint64>(_Boxes.Num() + _Panels.Num())};
        }

        /**
         * Fake a world mutation WITHOUT touching the box list, so a test can pin what a build does about a
         * changed world separately from what it would bake out of the new one.
         */
        auto Request_BumpWorldRevision() -> void { ++_WorldRevision; }

    private:
        struct FPanel
        {
            FVector _A = FVector::ZeroVector;
            FVector _B = FVector::ZeroVector;
            FVector _C = FVector::ZeroVector;
            FVector _D = FVector::ZeroVector;
            ECk_GroundNav_BodyKind _Kind = ECk_GroundNav_BodyKind::Solid;

            auto Get_Bounds() const -> FBox { return FBox{TArray<FVector>{_A, _B, _C, _D}}; }

            auto Add_To(FCk_GroundNav_GeometryBatch& OutBatch) const -> void
            {
                OutBatch.Add_Triangle(_A, _B, _C);
                OutBatch.Add_Triangle(_A, _C, _D);
            }
        };

        auto Get_Box(const FCk_GroundNav_BodyRef& InBody) const -> const FBox*
        {
            const auto Index = static_cast<int64>(InBody._Value) - 1;

            return Index >= 0 && Index < _Boxes.Num() ? &_Boxes[Index] : nullptr;
        }

        auto Get_Panel(const FCk_GroundNav_BodyRef& InBody) const -> const FPanel*
        {
            const auto Index = static_cast<int64>(InBody._Value) - 1 - _Boxes.Num();

            return Index >= 0 && Index < _Panels.Num() ? &_Panels[Index] : nullptr;
        }

    private:
        TArray<FBox> _Boxes;
        TArray<FPanel> _Panels;

        // Starts at 1, not 0: a stub that read as 0 would be indistinguishable from a backend that cannot
        // answer at all, and the fail-closed comparison this feeds must never rest on that ambiguity.
        uint64 _WorldRevision = 1;
    };
}

// --------------------------------------------------------------------------------------------------------------------
