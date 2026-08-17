#pragma once

#include <CoreMinimal.h>
#include <UObject/StrongObjectPtr.h>

class UStaticMesh;

struct FCk_DebugScene_Triangle
{
    FVector _A = FVector::ZeroVector;
    FVector _B = FVector::ZeroVector;
    FVector _C = FVector::ZeroVector;
};

class CKDEBUGSCENE_API FCk_DebugScene_Mesh final
{
  public:
    static auto
    Create_FromTriangles(TArray<FCk_DebugScene_Triangle> InTriangles) -> TSharedPtr<FCk_DebugScene_Mesh>;

  public:
    auto
    Get_StaticMesh() const -> UStaticMesh*;
    auto
    Get_LocalBounds() const -> const FBox&;

    /// Ray direction may be unnormalized. OutDistance is world-ray parametric distance, matching the caller's ray.
    auto
    TryIntersect_Ray(const FVector& InLocalOrigin, const FVector& InLocalDirection, double InMaxDistance,
                          double& OutDistance) const -> bool;

  private:
    struct FBvhNode
    {
        FBox _Bounds = FBox{ForceInit};
        int32 _Left = INDEX_NONE;
        int32 _Right = INDEX_NONE;
        int32 _FirstTriangle = 0;
        int32 _TriangleCount = 0;
    };

    explicit FCk_DebugScene_Mesh(TArray<FCk_DebugScene_Triangle> InTriangles);

    auto
    Ensure_Bvh() const -> void;
    auto
    Build_Bvh(TArray<int32>& InOutTriangleIndices, int32 InBegin, int32 InEnd) const -> int32;
    auto
    TryIntersect_Triangle(const FCk_DebugScene_Triangle& InTriangle, const FVector& InOrigin,
                               const FVector& InDirection, double InMaxDistance, double& OutDistance) const -> bool;

  private:
    TArray<FCk_DebugScene_Triangle> _Triangles;
    TStrongObjectPtr<UStaticMesh> _StaticMesh;
    FBox _LocalBounds = FBox{ForceInit};

    mutable bool _BvhBuilt = false;
    mutable TArray<int32> _BvhTriangleIndices;
    mutable TArray<FBvhNode> _BvhNodes;
};
