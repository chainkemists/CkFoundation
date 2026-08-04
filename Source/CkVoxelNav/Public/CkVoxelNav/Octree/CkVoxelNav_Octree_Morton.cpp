#include "CkVoxelNav_Octree_Morton.h"

// The vendored libmorton header opens a file-scope `using namespace std;`, so it is included last and
// only here - nothing else in the module includes it.
#include "morton.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voxelnav
{
    auto
        Get_MortonFromCoords(
            const FIntVector& InCoords)
        -> MortonCode
    {
        const auto X = static_cast<uint_fast32_t>(FMath::Clamp(InCoords.X, 0, GMaxMortonCoord));
        const auto Y = static_cast<uint_fast32_t>(FMath::Clamp(InCoords.Y, 0, GMaxMortonCoord));
        const auto Z = static_cast<uint_fast32_t>(FMath::Clamp(InCoords.Z, 0, GMaxMortonCoord));

        return static_cast<MortonCode>(morton3D_64_encode(X, Y, Z));
    }

    auto
        Get_CoordsFromMorton(
            MortonCode InMortonCode)
        -> FIntVector
    {
        auto X = uint_fast32_t{0};
        auto Y = uint_fast32_t{0};
        auto Z = uint_fast32_t{0};

        morton3D_64_decode(InMortonCode, X, Y, Z);

        return FIntVector{static_cast<int32>(X), static_cast<int32>(Y), static_cast<int32>(Z)};
    }

    auto
        Get_VectorFromMorton(
            MortonCode InMortonCode)
        -> FVector
    {
        const auto Coords = Get_CoordsFromMorton(InMortonCode);

        return FVector{static_cast<double>(Coords.X), static_cast<double>(Coords.Y), static_cast<double>(Coords.Z)};
    }

    auto
        Get_ParentMorton(
            MortonCode InChildMortonCode)
        -> MortonCode
    {
        return InChildMortonCode >> 3;
    }

    auto
        Get_FirstChildMorton(
            MortonCode InParentMortonCode)
        -> MortonCode
    {
        return InParentMortonCode << 3;
    }

    auto
        Get_ParentMortonAtLayer(
            MortonCode InChildMortonCode,
            LayerIndex InTargetLayerIndex,
            LayerIndex InChildLayerIndex)
        -> MortonCode
    {
        if (InTargetLayerIndex <= InChildLayerIndex)
        { return InChildMortonCode; }

        auto Code = InChildMortonCode;

        for (auto CurrentLayer = static_cast<int32>(InChildLayerIndex);
             CurrentLayer < static_cast<int32>(InTargetLayerIndex);
             ++CurrentLayer)
        {
            Code = Get_ParentMorton(Code);
        }

        return Code;
    }
}

// --------------------------------------------------------------------------------------------------------------------
