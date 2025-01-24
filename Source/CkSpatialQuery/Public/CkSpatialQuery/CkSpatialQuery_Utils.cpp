#include "CkSpatialQuery_Utils.h"

namespace ck::jolt
{
    auto
        Conv(
            FVector InVector)
            -> JoltVec3
    {
        return JoltVec3{static_cast<float>(InVector.X), static_cast<float>(InVector.Y), static_cast<float>(InVector.Z)};
    }

    auto
        Conv(
            Chaos::TVector<float, 3> InVector)
            -> JoltVec3
    {
        return JoltVec3{InVector.X, InVector.Y, InVector.Z};
    }

    auto
        Conv(
            JoltVec3 InVector)
            -> FVector
    {
        return FVector{InVector.GetX(), InVector.GetY(), InVector.GetZ()};
    }

    auto
        Conv(
            JoltFloat3 InVector)
            -> FVector
    {
        return FVector{InVector.x, InVector.y, InVector.z};
    }

    auto
        Conv(
            FRotator InRotator)
            -> JoltQuat
    {
        return Conv(InRotator.Quaternion());
    }

    auto
        Conv(
            FQuat InQuat)
            -> JoltQuat
    {
        return JoltQuat{
            static_cast<float>(InQuat.X),
            static_cast<float>(InQuat.Y),
            static_cast<float>(InQuat.Z),
            static_cast<float>(InQuat.W)
        };
    }

    auto
        Conv(
            JoltQuat InQuad)
            -> FQuat
    {
        return FQuat{InQuad.GetX(), InQuad.GetY(), InQuad.GetZ(), InQuad.GetW()};
    }
};
