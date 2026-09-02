#pragma once

#include "CkGroundNav/Bake/CkGroundNav_BakeTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * A surface normal stored as three signed bytes.
     *
     * Quantized deliberately, and not as an afterthought: a span field holds one of these per span over
     * a whole world, and full precision buys nothing downstream — the merge criteria compare normals
     * against a cone measured in whole degrees. Quantizing also makes the bake bit-reproducible, since
     * two mathematically-equal normals computed by different triangle orderings collapse to the same
     * stored value instead of differing in their last bits.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_QuantizedNormal
    {
    public:
        int8 _X = 0;
        int8 _Y = 0;
        int8 _Z = 127;

    public:
        auto operator==(const FCk_GroundNav_QuantizedNormal&) const -> bool = default;

    public:
        static auto Make(const FVector& InNormal) -> FCk_GroundNav_QuantizedNormal;

        auto Get_Normal() const -> FVector;

        /** Cosine of the angle from straight up, read straight off the stored Z. */
        auto Get_UpDot() const -> double { return static_cast<double>(_Z) / 127.0; }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One solid interval in a column, and the surface on top of it.
     *
     * Spans in a column are ordered by height and never overlap — that invariant is what lets layer
     * extraction treat a column as a short ordered list rather than a search.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_Span
    {
    public:
        // Bottom and top of the solid, in world Z.
        float _MinZ = 0.0f;
        float _MaxZ = 0.0f;

        // The normal of the surface that formed _MaxZ — the face an agent would stand on.
        FCk_GroundNav_QuantizedNormal _Normal;

        // Set by rasterization from the slope test, and cleared by the walkability filters. A span that is not
        // walkable still EXISTS: it occupies its column so the span above it has correct headroom.
        bool _IsWalkable = false;

    public:
        auto Get_Height() const -> float { return _MaxZ - _MinZ; }
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The rasterized world: a regular XY lattice of columns, each holding its ordered spans.
     *
     * Addressed by integer index only. No world position is stored per column — the origin and cell
     * size reconstruct every coordinate, which is what keeps the field relocatable and serializable.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_SpanField
    {
    public:
        // Min corner of column (0,0), in world space.
        FVector _Origin = FVector::ZeroVector;

        int32 _SizeX = 0;
        int32 _SizeY = 0;

        float _CellSizeUu = 0.0f;

        // _SizeX * _SizeY entries, row-major in X.
        //
        // Every column is sorted ascending by _MinZ, spans are disjoint, and _MaxZ < next._MinZ.
        // Rasterization establishes it in DoInsert_Span and no later stage reorders. It is load-bearing:
        // the clearance filter reads a span's headroom as Column[Index + 1]._MinZ - Span._MaxZ, which is
        // the right number only under this invariant and silently the wrong one under any other.
        TArray<TArray<FCk_GroundNav_Span>> _Columns;

    public:
        auto Get_ColumnCount() const -> int32 { return _SizeX * _SizeY; }

        auto Get_IsValidColumn(int32 InX, int32 InY) const -> bool
        {
            return InX >= 0 && InY >= 0 && InX < _SizeX && InY < _SizeY;
        }

        auto Get_ColumnIndex(int32 InX, int32 InY) const -> int32 { return (InY * _SizeX) + InX; }

        auto Get_Column(int32 InX, int32 InY) const -> const TArray<FCk_GroundNav_Span>&
        {
            return _Columns[Get_ColumnIndex(InX, InY)];
        }

        auto Get_MutableColumn(int32 InX, int32 InY) -> TArray<FCk_GroundNav_Span>&
        {
            return _Columns[Get_ColumnIndex(InX, InY)];
        }

        /** World-space min corner of the given column's XY square. */
        auto Get_ColumnMinCorner(int32 InX, int32 InY) const -> FVector2D
        {
            return FVector2D{
                _Origin.X + (static_cast<double>(InX) * _CellSizeUu),
                _Origin.Y + (static_cast<double>(InY) * _CellSizeUu)};
        }

        auto Get_TotalSpanCount() const -> int32;
    };
}

// --------------------------------------------------------------------------------------------------------------------
