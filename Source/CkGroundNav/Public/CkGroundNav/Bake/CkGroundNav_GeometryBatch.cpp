#include "CkGroundNav_GeometryBatch.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_GroundNav_GeometryBatch::
    Add_Box(
        const FBox& InBox)
    -> void
{
    const auto& Min = InBox.Min;
    const auto& Max = InBox.Max;

    // Corner naming: L/H per axis, so LHL is (Min.X, Max.Y, Min.Z).
    const auto LLL = FVector{Min.X, Min.Y, Min.Z};
    const auto HLL = FVector{Max.X, Min.Y, Min.Z};
    const auto HHL = FVector{Max.X, Max.Y, Min.Z};
    const auto LHL = FVector{Min.X, Max.Y, Min.Z};
    const auto LLH = FVector{Min.X, Min.Y, Max.Z};
    const auto HLH = FVector{Max.X, Min.Y, Max.Z};
    const auto HHH = FVector{Max.X, Max.Y, Max.Z};
    const auto LHH = FVector{Min.X, Max.Y, Max.Z};

    // Wound counter-clockwise seen from OUTSIDE the box, so the top face's normal points +Z and a
    // walkable-surface test on a floor box agrees with intuition without a winding fix-up later.
    const auto AddQuad = [&](const FVector& InA, const FVector& InB, const FVector& InC, const FVector& InD) -> void
    {
        Add_Triangle(InA, InB, InC);
        Add_Triangle(InA, InC, InD);
    };

    AddQuad(LLH, HLH, HHH, LHH); // +Z (top)
    AddQuad(LHL, HHL, HLL, LLL); // -Z (bottom)
    AddQuad(LLL, HLL, HLH, LLH); // -Y
    AddQuad(HHL, LHL, LHH, HHH); // +Y
    AddQuad(HLL, HHL, HHH, HLH); // +X
    AddQuad(LHL, LLL, LLH, LHH); // -X
}

// --------------------------------------------------------------------------------------------------------------------
