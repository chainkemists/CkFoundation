#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Color.h>
#include <Jolt/Math/Vec3.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    using JoltMatrix = JPH::Mat44;
    using JoltFloat3 = JPH::Float3;
    using JoltVec3 = JPH::Vec3;
    using JoltQuat = JPH::Quat;
    using JoltColor = JPH::Color;

    auto
    Conv(
        const FTransform& InMatrix) -> JoltMatrix;

    auto
    Conv(
        const FMatrix& InMatrix) -> JoltMatrix;

    auto
    Conv(
        const JoltMatrix& InMatrix) -> FMatrix;

    auto
    Conv(
        const FVector& InVector) -> JoltVec3;

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
        const FRotator& InRotator) -> JoltQuat;

    auto
    Conv(
        const FQuat& InQuat) -> JoltQuat;

    auto
    Conv(
        JoltQuat InQuad) -> FQuat;

    auto
    Conv(
        JoltColor InColor) -> FLinearColor;
};

// --------------------------------------------------------------------------------------------------------------------
