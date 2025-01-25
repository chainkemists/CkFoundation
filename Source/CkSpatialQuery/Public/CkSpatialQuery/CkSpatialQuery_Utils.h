#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "Jolt/Jolt.h"
#include <Jolt/Math/Vec3.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    using JoltFloat3 = JPH::Float3;
    using JoltVec3 = JPH::Vec3;
    using JoltQuat = JPH::Quat;

    auto
    Conv(
        FVector InVector) -> JoltVec3;

    auto
    Conv(
        Chaos::TVector<float, 3> InVector) -> JoltVec3;

    auto
    Conv(
        JoltVec3 InVector) -> FVector;

    auto
    Conv(
        JoltFloat3 InVector) -> FVector;

    auto
    Conv(
        FRotator InRotator) -> JoltQuat;

    auto
    Conv(
        FQuat InQuat) -> JoltQuat;

    auto
    Conv(
        JoltQuat InQuad) -> FQuat;
};

// --------------------------------------------------------------------------------------------------------------------
