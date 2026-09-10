#include "CkGroundNav/Streaming/CkGroundNav_StreamManifestBinding_UE.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_GroundNav_StreamManifestBinding_UE::
    TryGet_DataLayerSelector(
        ck::groundnav::FCk_GroundNav_DataLayerSelector& OutSelector) const
    -> bool
{
    auto Candidate = FCk_GroundNav_DataLayerSelector{};
    if (NOT ck::groundnav::TryMake_DataLayerSelector(_DataLayerNames, Candidate) ||
        Candidate.Get_LayerNames() != _DataLayerNames)
    { return false; }

    OutSelector = MoveTemp(Candidate);
    return true;
}

auto ACk_GroundNav_StreamManifestBinding_UE::Get_HasCanonicalTileCoords() const -> bool
{
    if (_TileCoords.IsEmpty()) { return false; }
    for (auto Index = 0; Index < _TileCoords.Num(); ++Index)
    {
        const auto& Coord = _TileCoords[Index];
        if (Coord.X < 0 || Coord.Y < 0 ||
            (Index > 0 && (Coord.Y < _TileCoords[Index - 1].Y ||
                           (Coord.Y == _TileCoords[Index - 1].Y && Coord.X <= _TileCoords[Index - 1].X))))
        { return false; }
    }
    return true;
}

auto
    ACk_GroundNav_StreamManifestBinding_UE::
    Get_HasValidIdentity() const
    -> bool
{
    return _StreamingVolumeId > 0 && _PartitionId > 0 && Get_HasCanonicalTileCoords();
}

// --------------------------------------------------------------------------------------------------------------------
