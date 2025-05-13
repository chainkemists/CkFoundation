#include "CkSpatialQuery_Utils.h"

namespace ck::jolt
{
    auto
        Conv(
            const FTransform& InMatrix)
        -> JoltMatrix
    {
        FMatrix UnrealMatrix = InMatrix.ToMatrixWithScale();
        return Conv(UnrealMatrix);
    }

    auto
        Conv(
            const FMatrix& InMatrix)
        -> JoltMatrix
    {
        return JPH::Mat44(
            JPH::Vec4(InMatrix.M[0][0], InMatrix.M[0][1], InMatrix.M[0][2], InMatrix.M[0][3]),
            JPH::Vec4(InMatrix.M[1][0], InMatrix.M[1][1], InMatrix.M[1][2], InMatrix.M[1][3]),
            JPH::Vec4(InMatrix.M[2][0], InMatrix.M[2][1], InMatrix.M[2][2], InMatrix.M[2][3]),
            JPH::Vec4(InMatrix.M[3][0], InMatrix.M[3][1], InMatrix.M[3][2], InMatrix.M[3][3]));
    }

    auto
        Conv(
            const JoltMatrix& InMatrix)
        -> FMatrix
    {
        auto UnrealMatrix = FMatrix{};

        // Extract the columns from the Jolt matrix
        const auto& Column0 = InMatrix.GetColumn4(0);
        const auto& Column1 = InMatrix.GetColumn4(1);
        const auto& Column2 = InMatrix.GetColumn4(2);
        const auto& Column3 = InMatrix.GetColumn4(3);

        // Fill the Unreal matrix
        UnrealMatrix.M[0][0] = Column0.GetX();
        UnrealMatrix.M[0][1] = Column0.GetY();
        UnrealMatrix.M[0][2] = Column0.GetZ();
        UnrealMatrix.M[0][3] = Column0.GetW();

        UnrealMatrix.M[1][0] = Column1.GetX();
        UnrealMatrix.M[1][1] = Column1.GetY();
        UnrealMatrix.M[1][2] = Column1.GetZ();
        UnrealMatrix.M[1][3] = Column1.GetW();

        UnrealMatrix.M[2][0] = Column2.GetX();
        UnrealMatrix.M[2][1] = Column2.GetY();
        UnrealMatrix.M[2][2] = Column2.GetZ();
        UnrealMatrix.M[2][3] = Column2.GetW();

        UnrealMatrix.M[3][0] = Column3.GetX();
        UnrealMatrix.M[3][1] = Column3.GetY();
        UnrealMatrix.M[3][2] = Column3.GetZ();
        UnrealMatrix.M[3][3] = Column3.GetW();

        return UnrealMatrix;
    }

    auto
        Conv(
            const FVector& InVector)
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
            const FRotator& InRotator)
        -> JoltQuat
    {
        return Conv(InRotator.Quaternion());
    }

    auto
        Conv(
            const FQuat& InQuat)
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
