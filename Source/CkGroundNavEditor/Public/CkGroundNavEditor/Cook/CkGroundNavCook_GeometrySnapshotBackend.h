#pragma once

#include "CkGroundNav/Backend/CkGroundNav_GeometryBackend.h"
#include "CkGroundNav/Bake/CkGroundNav_DataLayerSelector.h"
#include "CkJoltEditor/Cook/CkJoltCook_GeometrySnapshot.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav::cook
{
    /** Value backend over a descriptor-complete Jolt cook snapshot; no UObject survives collection. */
    class CKGROUNDNAVEDITOR_API FCk_GroundNav_CookGeometrySnapshotBackend final
        : public ICk_GroundNav_GeometryBackend
    {
    public:
        FCk_GroundNav_CookGeometrySnapshotBackend(
            const ck::jolt::cook::FCk_Jolt_CookGeometrySnapshot& InSnapshot,
            FCk_GroundNav_DataLayerSelector InSelector);

    public:
        auto Get_IsValid() const -> bool override;

        auto Get_HasGeometryInBounds(
            const FBox& InBounds) const -> bool override;

        auto Get_StaticBodiesInBounds(
            const FBox& InBounds,
            TArray<FCk_GroundNav_BodyRef>& OutBodies) const -> int32 override;

        auto Get_TrianglesInBounds(
            const FBox& InBounds,
            FCk_GroundNav_GeometryBatch& OutBatch) const -> int32 override;

        auto Get_WorldRevision() const -> uint64 override { return 1; }

        auto Get_BodyKind(
            const FCk_GroundNav_BodyRef& InBody) const -> ECk_GroundNav_BodyKind override;

        auto Get_BodyBounds(
            const FCk_GroundNav_BodyRef& InBody) const -> FBox override;

        auto Get_BodyTriangles(
            const FCk_GroundNav_BodyRef& InBody,
            FCk_GroundNav_GeometryBatch& OutBatch) const -> int32 override;

        auto Get_BodyDescription(
            const FCk_GroundNav_BodyRef& InBody) const -> FString override;

    private:
        auto Get_Body(
            const FCk_GroundNav_BodyRef& InBody) const
            -> const ck::jolt::cook::FCk_Jolt_CookGeometryBody*;

        auto Get_IsSelected(
            const ck::jolt::cook::FCk_Jolt_CookGeometryBody& InBody) const -> bool;

        static auto Append(
            const ck::jolt::FCk_Jolt_TriangleSoup& InSoup,
            FCk_GroundNav_GeometryBatch& OutBatch) -> int32;

    private:
        const ck::jolt::cook::FCk_Jolt_CookGeometrySnapshot* _Snapshot = nullptr;
        FCk_GroundNav_DataLayerSelector _Selector;
    };
}

// --------------------------------------------------------------------------------------------------------------------
