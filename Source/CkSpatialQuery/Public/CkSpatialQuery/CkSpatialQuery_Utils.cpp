#include "CkSpatialQuery_Utils.h"

namespace ck::jolt
{
    auto
        Conv(
            FVector InVector)
            -> JoltVec3
    {
        return JoltVec3{static_cast<float>(InVector.X), static_cast<float>(InVector.Z), static_cast<float>(InVector.Y)};
    }

    auto
        Conv(
            FVector InVector,
            Position)
            -> JoltVec3
    {
        return JoltVec3{static_cast<float>(InVector.X), static_cast<float>(InVector.Y), static_cast<float>(InVector.Z)};
    }

    static auto
        Conv(
            Chaos::TVector<float, 3> InVector)
            -> JoltVec3
    {
        return JoltVec3{InVector.X, InVector.Y, InVector.Z};
    }

    static auto
        Conv(
            Chaos::TVector<float, 3> InVector,
            Position)
            -> JoltVec3
    {
        return JoltVec3{InVector.X, InVector.Y, InVector.Z};
    }

    static auto
        Conv(
            JoltVec3 InVector)
            -> FVector
    {
        return FVector{InVector.GetX(), InVector.GetY(), InVector.GetZ()};
    }

    static auto
        Conv(
            JoltVec3 InVector,
            Position)
            -> FVector
    {
        return FVector{InVector.GetX(), InVector.GetY(), InVector.GetZ()};
    }
};
