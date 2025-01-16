#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "Jolt/Jolt.h"
#include <Jolt/Math/Vec3.h>

//#include "CkSpatialQuery_Utils.generated.h"

namespace ck::jolt
{
    // Differentiating between the two Conv functions is necessary.
    // Why explicitly specify Position? Non-positional vectors should not be negated, as doing so can cause Jolt to crash.
    struct Position {};

    using JoltVec3 = JPH::Vec3;

    static auto
        Conv(
            FVector InVector)
            -> JoltVec3;

    static auto
        Conv(
            FVector InVector,
            Position)
            -> JoltVec3;

    static auto
        Conv(
            Chaos::TVector<float, 3> InVector)
            -> JoltVec3;

    static auto
        Conv(
            Chaos::TVector<float, 3> InVector,
            Position)
            -> JoltVec3;

    static auto
        Conv(
            JoltVec3 InVector)
            -> FVector;

    static auto
        Conv(
            JoltVec3 InVector,
            Position)
            -> FVector;
};
