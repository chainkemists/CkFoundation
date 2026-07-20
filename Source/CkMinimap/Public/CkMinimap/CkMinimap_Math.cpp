#include "CkMinimap_Math.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_minimap_math
{
    // Rotate a world-XY delta by -InYawDegrees (θ = view yaw): Rot.X = ΔX·cosθ + ΔY·sinθ;
    // Rot.Y = -ΔX·sinθ + ΔY·cosθ. Spelled out in scalars so no FRotator sign convention can creep in.
    auto Get_RotatedByMinusYaw(const FVector2D& InDelta, float InYawDegrees) -> FVector2D
    {
        const auto Theta = FMath::DegreesToRadians(InYawDegrees);
        const auto CosTheta = FMath::Cos(Theta);
        const auto SinTheta = FMath::Sin(Theta);

        return FVector2D
        {
            InDelta.X * CosTheta + InDelta.Y * SinTheta,
            -InDelta.X * SinTheta + InDelta.Y * CosTheta
        };
    }

    auto Get_RotatedByPlusYaw(const FVector2D& InDelta, float InYawDegrees) -> FVector2D
    {
        return Get_RotatedByMinusYaw(InDelta, -InYawDegrees);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::minimap
{
    auto
        Get_WorldToFrame(
            const FVector& InWorldPos,
            const FVector& InViewOrigin,
            float InViewYawDegrees,
            float InViewExtent,
            ECk_Minimap_RotationMode InRotationMode)
        -> FVector2D
    {
        if (InViewExtent <= 0.0f)
        { return FVector2D::ZeroVector; }

        auto Delta = FVector2D{InWorldPos.X - InViewOrigin.X, InWorldPos.Y - InViewOrigin.Y};

        if (InRotationMode == ECk_Minimap_RotationMode::RotateWithObserver)
        {
            Delta = ck_minimap_math::Get_RotatedByMinusYaw(Delta, InViewYawDegrees);
        }

        return FVector2D{Delta.Y / InViewExtent, -Delta.X / InViewExtent};
    }

    auto
        Get_FrameToWorld(
            const FVector2D& InFramePos,
            const FVector& InViewOrigin,
            float InViewYawDegrees,
            float InViewExtent,
            ECk_Minimap_RotationMode InRotationMode)
        -> FVector2D
    {
        // Invert the axis swap first (ΔX = -Frame.Y·E, ΔY = Frame.X·E), then undo the view rotation
        auto Delta = FVector2D{-InFramePos.Y * InViewExtent, InFramePos.X * InViewExtent};

        if (InRotationMode == ECk_Minimap_RotationMode::RotateWithObserver)
        {
            Delta = ck_minimap_math::Get_RotatedByPlusYaw(Delta, InViewYawDegrees);
        }

        return FVector2D{InViewOrigin.X + Delta.X, InViewOrigin.Y + Delta.Y};
    }

    auto
        Get_BoundsToFrame(
            const FVector& InWorldPos,
            const FCk_Minimap_WorldBounds& InBounds)
        -> FVector2D
    {
        if (ck::Is_NOT_Valid(InBounds))
        { return FVector2D::ZeroVector; }

        const auto& Center = InBounds.Get_Center();
        const auto& HalfExtents = InBounds.Get_HalfExtents();

        return FVector2D
        {
            static_cast<float>((InWorldPos.Y - Center.Y) / HalfExtents.Y),
            static_cast<float>(-(InWorldPos.X - Center.X) / HalfExtents.X)
        };
    }

    auto
        Get_FrameToWorld_Bounds(
            const FVector2D& InFramePos,
            const FCk_Minimap_WorldBounds& InBounds)
        -> FVector2D
    {
        if (ck::Is_NOT_Valid(InBounds))
        { return FVector2D::ZeroVector; }

        const auto& Center = InBounds.Get_Center();
        const auto& HalfExtents = InBounds.Get_HalfExtents();

        return FVector2D
        {
            Center.X - InFramePos.Y * HalfExtents.X,
            Center.Y + InFramePos.X * HalfExtents.Y
        };
    }

    auto
        Get_IsInsideFrame(
            const FVector2D& InFramePos,
            ECk_Minimap_FrameShape InFrameShape)
        -> bool
    {
        switch (InFrameShape)
        {
            case ECk_Minimap_FrameShape::Rectangle:
            {
                return FMath::Abs(InFramePos.X) <= 1.0 && FMath::Abs(InFramePos.Y) <= 1.0;
            }
            case ECk_Minimap_FrameShape::Circle:
            {
                return InFramePos.SizeSquared() <= 1.0;
            }
            default:
            {
                CK_INVALID_ENUM(InFrameShape);
                return true;
            }
        }
    }

    auto
        Get_ClampToFrame(
            const FVector2D& InFramePos,
            ECk_Minimap_FrameShape InFrameShape)
        -> FVector2D
    {
        if (Get_IsInsideFrame(InFramePos, InFrameShape))
        { return InFramePos; }

        switch (InFrameShape)
        {
            case ECk_Minimap_FrameShape::Rectangle:
            {
                const auto MaxComponent = FMath::Max(FMath::Abs(InFramePos.X), FMath::Abs(InFramePos.Y));

                if (MaxComponent <= 0.0)
                { return FVector2D::ZeroVector; }

                return InFramePos / MaxComponent;
            }
            case ECk_Minimap_FrameShape::Circle:
            {
                const auto Length = InFramePos.Size();

                if (Length <= 0.0)
                { return FVector2D::ZeroVector; }

                return InFramePos / Length;
            }
            default:
            {
                CK_INVALID_ENUM(InFrameShape);
                return InFramePos;
            }
        }
    }

    auto
        Get_CellCounts(
            const FCk_Minimap_WorldBounds& InBounds,
            float InCellSize)
        -> FIntPoint
    {
        if (ck::Is_NOT_Valid(InBounds) || InCellSize <= 0.0f)
        { return FIntPoint{1, 1}; }

        const auto& HalfExtents = InBounds.Get_HalfExtents();

        return FIntPoint
        {
            FMath::Max(1, FMath::CeilToInt32(2.0 * HalfExtents.X / InCellSize)),
            FMath::Max(1, FMath::CeilToInt32(2.0 * HalfExtents.Y / InCellSize))
        };
    }

    auto
        Get_CellIndex(
            const FVector& InWorldPos,
            const FCk_Minimap_WorldBounds& InBounds,
            float InCellSize,
            const FIntPoint& InCellCounts)
        -> int32
    {
        if (ck::Is_NOT_Valid(InBounds) || InCellSize <= 0.0f || InCellCounts.X < 1 || InCellCounts.Y < 1)
        { return INDEX_NONE; }

        const auto& Center = InBounds.Get_Center();
        const auto& HalfExtents = InBounds.Get_HalfExtents();

        const auto MinX = Center.X - HalfExtents.X;
        const auto MinY = Center.Y - HalfExtents.Y;

        // Inclusive on BOTH edges — max-edge positions belong to the last cell (clamped floor below)
        if (InWorldPos.X < MinX || InWorldPos.X > Center.X + HalfExtents.X ||
            InWorldPos.Y < MinY || InWorldPos.Y > Center.Y + HalfExtents.Y)
        { return INDEX_NONE; }

        const auto CellX = FMath::Clamp(FMath::FloorToInt32((InWorldPos.X - MinX) / InCellSize), 0, InCellCounts.X - 1);
        const auto CellY = FMath::Clamp(FMath::FloorToInt32((InWorldPos.Y - MinY) / InCellSize), 0, InCellCounts.Y - 1);

        return CellY * InCellCounts.X + CellX;
    }

    auto
        Get_CellCenter(
            int32 InCellX,
            int32 InCellY,
            const FCk_Minimap_WorldBounds& InBounds,
            float InCellSize)
        -> FVector2D
    {
        const auto& Center = InBounds.Get_Center();
        const auto& HalfExtents = InBounds.Get_HalfExtents();

        return FVector2D
        {
            Center.X - HalfExtents.X + (InCellX + 0.5) * InCellSize,
            Center.Y - HalfExtents.Y + (InCellY + 0.5) * InCellSize
        };
    }

    auto
        Get_CellUVRect(
            int32 InCellIndex,
            const FCk_Minimap_WorldBounds& InBounds,
            float InCellSize)
        -> FBox2D
    {
        const auto CellCounts = Get_CellCounts(InBounds, InCellSize);

        if (ck::Is_NOT_Valid(InBounds) || InCellSize <= 0.0f ||
            InCellIndex < 0 || InCellIndex >= CellCounts.X * CellCounts.Y)
        { return FBox2D{FVector2D::ZeroVector, FVector2D::ZeroVector}; }

        const auto CellX = InCellIndex % CellCounts.X;
        const auto CellY = InCellIndex / CellCounts.X;

        const auto& HalfExtents = InBounds.Get_HalfExtents();
        const auto ExtentX = 2.0 * HalfExtents.X;
        const auto ExtentY = 2.0 * HalfExtents.Y;

        // U spans world +Y (map right), V spans world -X (map down) — UV = (Frame + 1) / 2 of Get_BoundsToFrame.
        // Normalized over the BOUNDS extent and clamped so overshooting edge cells never seam past the texture
        const auto UMin = FMath::Clamp(CellY * InCellSize / ExtentY, 0.0, 1.0);
        const auto UMax = FMath::Clamp((CellY + 1) * InCellSize / ExtentY, 0.0, 1.0);
        const auto VMin = FMath::Clamp(1.0 - ((CellX + 1) * InCellSize / ExtentX), 0.0, 1.0);
        const auto VMax = FMath::Clamp(1.0 - (CellX * InCellSize / ExtentX), 0.0, 1.0);

        return FBox2D{FVector2D{UMin, VMin}, FVector2D{UMax, VMax}};
    }
}

// --------------------------------------------------------------------------------------------------------------------
