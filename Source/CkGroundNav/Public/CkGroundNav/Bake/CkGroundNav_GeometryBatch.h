#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

/**
 * The bake's only view of world geometry: an indexed triangle list in world space, unreal units.
 *
 * A plain value type on purpose. It routinely carries hundreds of thousands of vertices, so nothing
 * about it belongs in a details panel or on the wire, and every bake stage downstream of collection
 * reads it as pure data with no knowledge of where it came from.
 *
 * Vertices are NOT welded. Rasterization visits triangles independently, so welding would cost a
 * hash pass that nothing reads.
 */
struct CKGROUNDNAV_API FCk_GroundNav_GeometryBatch
{
public:
    TArray<FVector> _Vertices;
    TArray<int32>   _Indices;

public:
    auto Get_TriangleCount() const -> int32 { return _Indices.Num() / 3; }
    auto Get_IsEmpty() const -> bool { return _Indices.IsEmpty(); }

    auto Reset() -> void
    {
        _Vertices.Reset();
        _Indices.Reset();
    }

    /**
     * The three world-space corners of one triangle. The caller is responsible for the index being in
     * range; every bake stage iterates [0, Get_TriangleCount()) and none of them can exceed it.
     */
    auto Get_Triangle(
        int32 InTriangleIndex,
        FVector& OutA,
        FVector& OutB,
        FVector& OutC) const -> void
    {
        const auto Base = InTriangleIndex * 3;

        OutA = _Vertices[_Indices[Base + 0]];
        OutB = _Vertices[_Indices[Base + 1]];
        OutC = _Vertices[_Indices[Base + 2]];
    }

    /** Append one world-space triangle. Used by the stub backend and by tests to author fixtures. */
    auto Add_Triangle(
        const FVector& InA,
        const FVector& InB,
        const FVector& InC) -> void
    {
        const auto Base = _Vertices.Num();

        _Vertices.Emplace(InA);
        _Vertices.Emplace(InB);
        _Vertices.Emplace(InC);

        _Indices.Emplace(Base + 0);
        _Indices.Emplace(Base + 1);
        _Indices.Emplace(Base + 2);
    }

    /** Append the 12 triangles of an axis-aligned box. The fixture primitive for hermetic bake tests. */
    auto Add_Box(const FBox& InBox) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
