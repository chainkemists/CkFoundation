#include "CkPathNetwork_Types.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_PathNetwork_DetectionMask::
    Get_IsValidMask() const
    -> bool
{
    return _SizeX > 0 && _SizeY > 0 &&
           _Occupancy.Num() == _SizeX * _SizeY &&
           (_Heights.IsEmpty() || _Heights.Num() == _SizeX * _SizeY);
}

auto
    FCk_PathNetwork_DetectionMask::
    Get_IsOccupied(int32 InX, int32 InY) const
    -> bool
{
    if (InX < 0 || InY < 0 || InX >= _SizeX || InY >= _SizeY)
    { return false; }

    return _Occupancy[InY * _SizeX + InX] != 0;
}

auto
    FCk_PathNetwork_DetectionMask::
    Get_CellWorldLocation(int32 InX, int32 InY) const
    -> FVector
{
    const auto Z = _Heights.Num() == _SizeX * _SizeY
        ? _Heights[InY * _SizeX + InX]
        : static_cast<float>(_Origin.Z);

    return FVector{
        _Origin.X + (InX + 0.5f) * _CellSize,
        _Origin.Y + (InY + 0.5f) * _CellSize,
        static_cast<double>(Z)};
}

// --------------------------------------------------------------------------------------------------------------------
