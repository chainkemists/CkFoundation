#include "CkPmg_Utils_DebugLines.h"

#include "CkCore/Math/Vector/CkVector_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pmg
{
    namespace
    {
        auto
            GetOrAddLinesFragment(
                FCk_Handle& InHandle)
            -> ck::FFragment_Pmg_DebugShape_Lines&
        {
            // FProcessor_Pmg_DebugShape_BakeLines requires Common + Lines +
            // Current (mesh component). Shape Setup processors already
            // provide Common via their params; for debug-only consumers
            // (no procedural mesh, just wireframe overlays) we ensure Common
            // exists with defaults so the bake processor matches.
            InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Common>();

            // Stamp the one-shot bake gate. Idempotent — each Append_* call
            // re-stamps it, but the bake processor only consumes it once
            // and runs once per "lines changed" event.
            InHandle.AddOrGet<ck::FTag_Pmg_DebugShape_LinesNeedBaking>();

            return InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Lines>();
        }

        auto
            WorldToLocal(
                const FCk_Handle& InHandle,
                const FVector& InWorld)
            -> FVector
        {
            if (NOT InHandle.Has<ck::FFragment_Transform>())
            { return InWorld; }

            const auto& Transform = InHandle.Get<ck::FFragment_Transform>().Get_Transform();
            return Transform.InverseTransformPosition(InWorld);
        }
    }

    auto
        Append_DebugLine_World(
            FCk_Handle InHandle,
            const FVector& InWorldStart,
            const FVector& InWorldEnd,
            const FLinearColor& InColor,
            float InThickness)
        -> void
    {
        auto& Fragment = GetOrAddLinesFragment(InHandle);

        Fragment._Lines.Add(ck::FCk_Pmg_DebugLine{
            WorldToLocal(InHandle, InWorldStart),
            WorldToLocal(InHandle, InWorldEnd),
            InColor,
            InThickness});
    }

    auto
        Append_DebugBox_World(
            FCk_Handle InHandle,
            const FVector& InCenter,
            const FVector& InExtent,
            const FRotator& InRotation,
            const FLinearColor& InColor,
            float InThickness)
        -> void
    {
        // Generate the 8 corners of the oriented box, then push the 12 edges.
        const auto Quat = InRotation.Quaternion();
        const FVector Corners[8] = {
            InCenter + Quat.RotateVector(FVector(-InExtent.X, -InExtent.Y, -InExtent.Z)),
            InCenter + Quat.RotateVector(FVector( InExtent.X, -InExtent.Y, -InExtent.Z)),
            InCenter + Quat.RotateVector(FVector( InExtent.X,  InExtent.Y, -InExtent.Z)),
            InCenter + Quat.RotateVector(FVector(-InExtent.X,  InExtent.Y, -InExtent.Z)),
            InCenter + Quat.RotateVector(FVector(-InExtent.X, -InExtent.Y,  InExtent.Z)),
            InCenter + Quat.RotateVector(FVector( InExtent.X, -InExtent.Y,  InExtent.Z)),
            InCenter + Quat.RotateVector(FVector( InExtent.X,  InExtent.Y,  InExtent.Z)),
            InCenter + Quat.RotateVector(FVector(-InExtent.X,  InExtent.Y,  InExtent.Z))};

        // Bottom face (0-1-2-3), top face (4-5-6-7), 4 vertical edges.
        const int32 Edges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}};

        for (const auto& Edge : Edges)
        {
            Append_DebugLine_World(InHandle, Corners[Edge[0]], Corners[Edge[1]], InColor, InThickness);
        }
    }

    auto
        Append_DebugCapsule_World(
            FCk_Handle InHandle,
            const FVector& InCenter,
            float InHalfHeight,
            float InRadius,
            const FRotator& InRotation,
            const FLinearColor& InColor,
            float InThickness)
        -> void
    {
        // UE's DrawDebugCapsule draws the capsule along its Z axis. We mirror
        // that: 4 longitudinal lines down the cylinder body, plus a top and
        // bottom ring, plus 2 vertical half-rings (one along X, one along Y)
        // tying the hemispherical caps together.
        const auto Quat = InRotation.Quaternion();
        const auto CylHalf = FMath::Max(0.0f, InHalfHeight - InRadius);

        const FVector AxisOffset[4] = {
            FVector( InRadius, 0.0f, 0.0f),
            FVector(-InRadius, 0.0f, 0.0f),
            FVector(0.0f,  InRadius, 0.0f),
            FVector(0.0f, -InRadius, 0.0f)};

        for (const auto& Off : AxisOffset)
        {
            const auto Top    = InCenter + Quat.RotateVector(Off + FVector(0.0f, 0.0f,  CylHalf));
            const auto Bottom = InCenter + Quat.RotateVector(Off + FVector(0.0f, 0.0f, -CylHalf));
            Append_DebugLine_World(InHandle, Top, Bottom, InColor, InThickness);
        }

        constexpr auto Segments = 24;

        const auto AppendArc = [&](const FVector& InAxisU, const FVector& InAxisV,
                                   float InStartAngle, float InEndAngle, float InZ)
        {
            for (auto i = 0; i < Segments; ++i)
            {
                const auto T1 = InStartAngle + (InEndAngle - InStartAngle) * (float(i)     / Segments);
                const auto T2 = InStartAngle + (InEndAngle - InStartAngle) * (float(i + 1) / Segments);

                const auto P1Local = InAxisU * (InRadius * FMath::Cos(T1)) + InAxisV * (InRadius * FMath::Sin(T1)) + FVector(0.0f, 0.0f, InZ);
                const auto P2Local = InAxisU * (InRadius * FMath::Cos(T2)) + InAxisV * (InRadius * FMath::Sin(T2)) + FVector(0.0f, 0.0f, InZ);

                const auto P1World = InCenter + Quat.RotateVector(P1Local);
                const auto P2World = InCenter + Quat.RotateVector(P2Local);
                Append_DebugLine_World(InHandle, P1World, P2World, InColor, InThickness);
            }
        };

        const auto AxisX = FVector::ForwardVector;
        const auto AxisY = FVector::RightVector;
        const auto AxisZ = FVector::UpVector;

        // Top + bottom equator rings.
        AppendArc(AxisX, AxisY, 0.0f, 2.0f * PI,  CylHalf);
        AppendArc(AxisX, AxisY, 0.0f, 2.0f * PI, -CylHalf);

        // Top hemisphere half-rings: XZ goes from +X up to -X, YZ from +Y up to -Y.
        // Implemented by rotating an upward arc around X axis with U=Z, V=X then U=Z, V=Y.
        for (auto i = 0; i < Segments; ++i)
        {
            const auto T1 = 0.0f + PI * (float(i)     / Segments);
            const auto T2 = 0.0f + PI * (float(i + 1) / Segments);

            const auto P1XZ_Top = FVector(InRadius * FMath::Cos(T1), 0.0f, CylHalf + InRadius * FMath::Sin(T1));
            const auto P2XZ_Top = FVector(InRadius * FMath::Cos(T2), 0.0f, CylHalf + InRadius * FMath::Sin(T2));
            Append_DebugLine_World(InHandle,
                InCenter + Quat.RotateVector(P1XZ_Top),
                InCenter + Quat.RotateVector(P2XZ_Top), InColor, InThickness);

            const auto P1YZ_Top = FVector(0.0f, InRadius * FMath::Cos(T1), CylHalf + InRadius * FMath::Sin(T1));
            const auto P2YZ_Top = FVector(0.0f, InRadius * FMath::Cos(T2), CylHalf + InRadius * FMath::Sin(T2));
            Append_DebugLine_World(InHandle,
                InCenter + Quat.RotateVector(P1YZ_Top),
                InCenter + Quat.RotateVector(P2YZ_Top), InColor, InThickness);

            const auto P1XZ_Bot = FVector(InRadius * FMath::Cos(T1), 0.0f, -CylHalf - InRadius * FMath::Sin(T1));
            const auto P2XZ_Bot = FVector(InRadius * FMath::Cos(T2), 0.0f, -CylHalf - InRadius * FMath::Sin(T2));
            Append_DebugLine_World(InHandle,
                InCenter + Quat.RotateVector(P1XZ_Bot),
                InCenter + Quat.RotateVector(P2XZ_Bot), InColor, InThickness);

            const auto P1YZ_Bot = FVector(0.0f, InRadius * FMath::Cos(T1), -CylHalf - InRadius * FMath::Sin(T1));
            const auto P2YZ_Bot = FVector(0.0f, InRadius * FMath::Cos(T2), -CylHalf - InRadius * FMath::Sin(T2));
            Append_DebugLine_World(InHandle,
                InCenter + Quat.RotateVector(P1YZ_Bot),
                InCenter + Quat.RotateVector(P2YZ_Bot), InColor, InThickness);
        }
    }

    auto
        Append_DebugCircle_PlaneAxis_World(
            FCk_Handle InHandle,
            const FVector& InCenter,
            float InRadius,
            ECk_Plane_Axis InAxis,
            int32 InSegments,
            const FLinearColor& InColor,
            float InThickness)
        -> void
    {
        const auto Quat = UCk_Utils_Vector3_UE::Get_PlaneAxisRotation(InAxis);
        const auto Steps = FMath::Max(3, InSegments);

        for (auto i = 0; i < Steps; ++i)
        {
            const auto T1 = 2.0f * PI * (float(i)     / Steps);
            const auto T2 = 2.0f * PI * (float(i + 1) / Steps);

            const auto P1 = InCenter + Quat.RotateVector(FVector(InRadius * FMath::Cos(T1), InRadius * FMath::Sin(T1), 0.0f));
            const auto P2 = InCenter + Quat.RotateVector(FVector(InRadius * FMath::Cos(T2), InRadius * FMath::Sin(T2), 0.0f));
            Append_DebugLine_World(InHandle, P1, P2, InColor, InThickness);
        }
    }

    auto
        Append_DebugTriangle_World(
            FCk_Handle InHandle,
            const FVector& InV1,
            const FVector& InV2,
            const FVector& InV3,
            const FLinearColor& InColor,
            float InThickness)
        -> void
    {
        Append_DebugLine_World(InHandle, InV1, InV2, InColor, InThickness);
        Append_DebugLine_World(InHandle, InV2, InV3, InColor, InThickness);
        Append_DebugLine_World(InHandle, InV3, InV1, InColor, InThickness);
    }

    auto
        Append_DebugPolygon_World(
            FCk_Handle InHandle,
            const TArray<FVector>& InVertices,
            const FLinearColor& InColor,
            float InThickness)
        -> void
    {
        const auto Count = InVertices.Num();
        if (Count < 2)
        { return; }

        for (auto i = 0; i < Count; ++i)
        {
            const auto& A = InVertices[i];
            const auto& B = InVertices[(i + 1) % Count];
            Append_DebugLine_World(InHandle, A, B, InColor, InThickness);
        }
    }
}
