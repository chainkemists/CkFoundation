#include "CkPmg_Utils_DebugLines.h"

#include "CkPmg_Utils_DebugShapes.h"

#include "CkCore/Math/Vector/CkVector_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Fragment.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pmg
{
    namespace ck_pmg_utils_debug_lines
    {
        auto
            GetOrAddLinesFragment(
                FCk_Handle& InHandle)
            -> ck::FFragment_Pmg_DebugShape_Lines&
        {
            // Line-only consumers never run a shape Setup, so the bake view needs Common defaulted here.
            InHandle.AddOrGet<ck::FFragment_Pmg_DebugShape_Common>();

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
        Create_DebugLineSet(
            FCk_Handle& InOwningEntity,
            const FTransform& InTransform,
            const FLinearColor& InColor,
            float InThickness,
            ECk_Pmg_RenderMode InRenderMode)
        -> FCk_Handle_Pmg_DebugShape
    {
        const auto OwnerIsValid = ck::IsValid(InOwningEntity);
        CK_ENSURE_IF_NOT(OwnerIsValid,
            TEXT("Cannot create a retained PMG line set under invalid owner [{}]"),
            InOwningEntity)
        {}
        if (NOT OwnerIsValid)
        { return {}; }

        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
        const auto EntityWasCreated = ck::IsValid(NewEntity);
        CK_ENSURE_IF_NOT(EntityWasCreated,
            TEXT("Cannot create a retained PMG line-set entity under owner [{}]"),
            InOwningEntity)
        {}
        if (NOT EntityWasCreated)
        { return {}; }

        auto Common = FFragment_Pmg_DebugShape_Common{};
        Common.Set_Color(InColor);
        Common.Set_DrawLines(true);
        Common.Set_LineThickness(InThickness);
        Common.Set_Duration(FCk_Time{-1.0f});
        Common.Set_RenderMode(InRenderMode);
        pmg_debug_shape::AddCommon(NewEntity, Common);

        NewEntity.Add<FFragment_Pmg_DebugShape_Current>();
        NewEntity.Add<FTag_Pmg_DebugShape_LineSet>();
        NewEntity.Add<FTag_Pmg_DebugShape_NeedsSetup>();
        UCk_Utils_Transform_UE::Add(NewEntity, InTransform, ECk_Replication::DoesNotReplicate);

        return UCk_Utils_Pmg_DebugShape_UE::CastChecked(NewEntity);
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
        auto& Fragment = ck_pmg_utils_debug_lines::GetOrAddLinesFragment(InHandle);

        Fragment._Lines.Add(ck::FCk_Pmg_DebugLine{
            ck_pmg_utils_debug_lines::WorldToLocal(InHandle, InWorldStart),
            ck_pmg_utils_debug_lines::WorldToLocal(InHandle, InWorldEnd),
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
        // Capsule axis is Z, mirroring UE's DrawDebugCapsule.
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

        AppendArc(AxisX, AxisY, 0.0f, 2.0f * PI,  CylHalf);
        AppendArc(AxisX, AxisY, 0.0f, 2.0f * PI, -CylHalf);

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
        // The entity rotation MUST be composed in here — see CkPmg/Claude.md.
        auto EntityRotation = FQuat::Identity;
        if (InHandle.Has<ck::FFragment_Transform>())
        { EntityRotation = InHandle.Get<ck::FFragment_Transform>().Get_Transform().GetRotation(); }

        const auto Quat = EntityRotation * UCk_Utils_Vector3_UE::Get_PlaneAxisRotation(InAxis);
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
