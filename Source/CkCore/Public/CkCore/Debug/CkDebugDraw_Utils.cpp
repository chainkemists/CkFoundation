#include "CkDebugDraw_Utils.h"

#include "CkCore/Debug/CkDebugDraw_Subsystem.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Math/Geometry/CkGeometry_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include <GameFramework/HUD.h>

#include <Kismet/KismetSystemLibrary.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DebugDraw_UE::
    Create_ASCII_ProgressBar(
        const FCk_FloatRange_0to1& InProgressValue,
        const int32 InProgressBarCharacterLength,
        ECk_ForwardReverse InForwardOrReverse,
        ECk_ASCII_ProgressBar_Style InStyle)
    -> FString
{
    CK_ENSURE_IF_NOT(InProgressBarCharacterLength > 0, TEXT("ASCII ProgressBar character length needs to be > 0"))
    { return {}; }

    FStringBuilderBase StringBuilder;

    const auto& ProgressValue = InProgressValue.Get_Value();
    const auto& NumberOfCharacters = FMath::RoundToInt(ProgressValue * static_cast<float>(InProgressBarCharacterLength));

    const auto& ProgressCharacter = [&]() -> FString
    {
        switch (InStyle)
        {
            case ECk_ASCII_ProgressBar_Style::Equal_Symbol:
            {
                return TEXT("=");
            }
            case ECk_ASCII_ProgressBar_Style::HashTag_Symbol:
            {
                return TEXT("#");
            }
            case ECk_ASCII_ProgressBar_Style::FilledBlock_Symbol:
            {
                return TEXT("█");
            }
            default:
            {
                CK_INVALID_ENUM(InStyle);
                return TEXT("█");
            }
        }
    }();

    switch (InForwardOrReverse)
    {
        case ECk_ForwardReverse::Forward:
        {
            for (auto Index = 0; Index < InProgressBarCharacterLength; ++Index)
            {
                if (Index < NumberOfCharacters)
                {
                    StringBuilder.Append(ProgressCharacter);
                }
                else
                {
                    StringBuilder.Append(TEXT(" "));
                }
            }
            break;
        }
        case ECk_ForwardReverse::Reverse:
        {
            for (auto Index = InProgressBarCharacterLength - 1; Index >= 0; --Index)
            {
                if (Index < NumberOfCharacters)
                {
                    StringBuilder.Append(ProgressCharacter);
                }
                else
                {
                    StringBuilder.Append(TEXT(" "));
                }
            }
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InForwardOrReverse);
            break;
        }
    }

    return StringBuilder.ToString();
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugSphere(
        const UObject* InWorldContextObject,
        const FVector InCenter,
        float InRadius,
        int32 InSegments,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugSphere(InWorldContextObject, InCenter, InRadius, InSegments, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugLine(
        const UObject* InWorldContextObject,
        const FVector InLineStart,
        const FVector InLineEnd,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugLine(InWorldContextObject, InLineStart, InLineEnd, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugCircle(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        int32 InNumSegments,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness,
        FVector InYAxis,
        FVector InZAxis,
        bool InDrawAxis)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugCircle(InWorldContextObject, InCenter, InRadius, InNumSegments, InLineColor, InDuration, InThickness, InYAxis, InZAxis, InDrawAxis);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugPoint(
        const UObject* InWorldContextObject,
        const FVector InPosition,
        float InSize,
        FLinearColor InPointColor,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugPoint(InWorldContextObject, InPosition, InSize, InPointColor, InDuration);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugArrow(
        const UObject* InWorldContextObject,
        const FVector InLineStart,
        const FVector InLineEnd,
        float InArrowSize,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugArrow(InWorldContextObject, InLineStart, InLineEnd, InArrowSize, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugBox(
        const UObject* InWorldContextObject,
        const FVector InCenter,
        FVector InExtent,
        FLinearColor InLineColor,
        const FRotator InRotation,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugBox(InWorldContextObject, InCenter, InExtent, InLineColor, InRotation, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugCylinder(
        const UObject* InWorldContextObject,
        const FVector InStart,
        const FVector InEnd,
        float InRadius,
        int32 InSegments,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugCylinder(InWorldContextObject, InStart, InEnd, InRadius, InSegments, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugCone(
        const UObject* InWorldContextObject,
        const FVector InOrigin,
        const FVector InDirection,
        float InLength,
        float InAngleWidth,
        float InAngleHeight,
        int32 InNumSides,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugCone(InWorldContextObject, InOrigin, InDirection, InLength, InAngleWidth, InAngleHeight, InNumSides, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugCapsule(
        const UObject* InWorldContextObject,
        const FVector InCenter,
        float InHalfHeight,
        float InRadius,
        const FRotator InRotation,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugCapsule(InWorldContextObject, InCenter, InHalfHeight, InRadius, InRotation, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugString(
        const UObject* InWorldContextObject,
        const FVector InTextLocation,
        const FString& InText,
        AActor* InTestBaseActor,
        FLinearColor InTextColor,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugString(InWorldContextObject, InTextLocation, InText, InTestBaseActor, InTextColor, InDuration);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugBigArrow(
        const UObject* InWorldContextObject,
        const FVector InLocation,
        const FRotator InDirection,
        const FLinearColor InArrowColor,
        float InArrowLineHeight,
        float InArrowLineWidth,
        float InThickness,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    const auto ForwardVector = FRotationMatrix(InDirection).GetScaledAxis(EAxis::X);
    const auto RightVector = FRotationMatrix(InDirection).GetScaledAxis(EAxis::Y);

    const auto LeftLineStart = InLocation + RightVector * -InArrowLineWidth;
    const auto RightLineStart = InLocation + RightVector * InArrowLineWidth;
    const auto LeftLineEnd = LeftLineStart + ForwardVector * InArrowLineHeight;
    const auto RightLineEnd = RightLineStart + ForwardVector * InArrowLineHeight;

    DrawDebugLine(InWorldContextObject, LeftLineStart, LeftLineEnd, InArrowColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, RightLineStart, RightLineEnd, InArrowColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, LeftLineStart, RightLineStart, InArrowColor, InDuration, InThickness);

    const auto LeftBaseArrowEnd = LeftLineEnd + RightVector * -InArrowLineWidth / 2.f;
    const auto RightBaseArrowEnd = RightLineEnd + RightVector * InArrowLineWidth / 2.f;

    DrawDebugLine(InWorldContextObject, LeftLineEnd, LeftBaseArrowEnd, InArrowColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, RightLineEnd, RightBaseArrowEnd, InArrowColor, InDuration, InThickness);

    const auto ArrowTip = InLocation + ForwardVector * (InArrowLineHeight + InArrowLineHeight / 3.f);
    DrawDebugLine(InWorldContextObject, LeftBaseArrowEnd, ArrowTip, InArrowColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, RightBaseArrowEnd, ArrowTip, InArrowColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugTransformGizmo(
        const UObject* InWorldContextObject,
        const FTransform& InTransform,
        float InAxisLength,
        float InAxisThickness,
        bool InDrawAxisCones,
        float InConeSize,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    const auto& Origin = InTransform.GetLocation();
    const auto& Rotation = InTransform.GetRotation();

    // Get axis vectors
    const auto XAxis = Rotation.GetAxisX();
    const auto YAxis = Rotation.GetAxisY();
    const auto ZAxis = Rotation.GetAxisZ();

    // Draw X axis (Red)
    const auto XEnd = Origin + XAxis * InAxisLength;
    DrawDebugLine(InWorldContextObject, Origin, XEnd, FLinearColor::Red, InDuration, InAxisThickness);

    constexpr auto DebugConeAngleDegrees = 15.0f;
    constexpr auto DebugConeAngleRadians = FMath::DegreesToRadians(DebugConeAngleDegrees);

    if (InDrawAxisCones)
    {
        DrawDebugCone(InWorldContextObject,
            XEnd, -XAxis, InConeSize, DebugConeAngleRadians, DebugConeAngleRadians, 8, FLinearColor::Red, InDuration, InAxisThickness * 0.5f);
    }

    // Draw Y axis (Green)
    const auto YEnd = Origin + YAxis * InAxisLength;
    DrawDebugLine(InWorldContextObject, Origin, YEnd, FLinearColor::Green, InDuration, InAxisThickness);

    if (InDrawAxisCones)
    {
        DrawDebugCone(InWorldContextObject,
            YEnd, -YAxis, InConeSize, DebugConeAngleRadians, DebugConeAngleRadians, 8, FLinearColor::Green, InDuration, InAxisThickness * 0.5f);
    }

    // Draw Z axis (Blue)
    const auto ZEnd = Origin + ZAxis * InAxisLength;
    DrawDebugLine(InWorldContextObject, Origin, ZEnd, FLinearColor::Blue, InDuration, InAxisThickness);

    if (InDrawAxisCones)
    {
        DrawDebugCone(InWorldContextObject,
            ZEnd, -ZAxis, InConeSize, DebugConeAngleRadians, DebugConeAngleRadians, 8, FLinearColor::Blue, InDuration, InAxisThickness * 0.5f);
    }
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugCoordinateSystem(
        const UObject* InWorldContextObject,
        const FVector InAxisLoc,
        const FRotator InAxisRot,
        float InScale,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugCoordinateSystem(InWorldContextObject, InAxisLoc, InAxisRot, InScale, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugPlane(
        const UObject* InWorldContextObject,
        const FPlane& InPlaneCoordinates,
        const FVector InLocation,
        float InSize,
        FLinearColor InPlaneColor,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugPlane(InWorldContextObject, InPlaneCoordinates, InLocation, InSize, InPlaneColor, InDuration);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugFrustum(
        const UObject* InWorldContextObject,
        const FTransform& InFrustumTransform,
        FLinearColor InFrustumColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugFrustum(InWorldContextObject, InFrustumTransform, InFrustumColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugFloatHistoryTransform(
        const UObject* InWorldContextObject,
        const FDebugFloatHistory& InFloatHistory,
        const FTransform& InDrawTransform,
        FVector2D InDrawSize,
        FLinearColor InDrawColor,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugFloatHistoryTransform(InWorldContextObject, InFloatHistory, InDrawTransform, InDrawSize, InDrawColor, InDuration);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugFloatHistoryLocation(
        const UObject* InWorldContextObject,
        const FDebugFloatHistory& InFloatHistory,
        FVector InDrawLocation,
        FVector2D InDrawSize,
        FLinearColor InDrawColor,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::DrawDebugFloatHistoryLocation(InWorldContextObject, InFloatHistory, InDrawLocation, InDrawSize, InDrawColor, InDuration);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    FlushPersistentDebugLines(
        const UObject* InWorldContextObject)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::FlushPersistentDebugLines(InWorldContextObject);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    FlushDebugStrings(
        const UObject* InWorldContextObject)
    -> void
{
#if ENABLE_DRAW_DEBUG
    UKismetSystemLibrary::FlushDebugStrings(InWorldContextObject);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugGrid(
        const UObject* InWorldContextObject,
        const FVector InCenter,
        const FVector InForward,
        const FVector InRight,
        float InGridSize,
        int32 InGridLines,
        FLinearColor InGridColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    const auto HalfGridSize = InGridSize * 0.5f;
    const auto LineSpacing = InGridSize / InGridLines;

    // Draw grid lines along forward direction
    for (int32 I = 0; I <= InGridLines; ++I)
    {
        const auto Offset = (I * LineSpacing) - HalfGridSize;
        const auto LineStart = InCenter + InRight * Offset - InForward * HalfGridSize;
        const auto LineEnd = InCenter + InRight * Offset + InForward * HalfGridSize;

        DrawDebugLine(InWorldContextObject, LineStart, LineEnd, InGridColor, InDuration, InThickness);
    }

    // Draw grid lines along right direction
    for (int32 i = 0; i <= InGridLines; ++i)
    {
        const auto Offset = (i * LineSpacing) - HalfGridSize;
        const auto LineStart = InCenter + InForward * Offset - InRight * HalfGridSize;
        const auto LineEnd = InCenter + InForward * Offset + InRight * HalfGridSize;

        DrawDebugLine(InWorldContextObject, LineStart, LineEnd, InGridColor, InDuration, InThickness);
    }
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugCross(
        const UObject* InWorldContextObject,
        const FVector InCenter,
        float InSize,
        FLinearColor InColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    const auto HalfSize = InSize * 0.5f;

    // Draw X axis line
    DrawDebugLine(InWorldContextObject,
                 InCenter + FVector(-HalfSize, 0, 0), InCenter + FVector(HalfSize, 0, 0),
                 InColor, InDuration, InThickness);

    // Draw Y axis line
    DrawDebugLine(InWorldContextObject,
                 InCenter + FVector(0, -HalfSize, 0), InCenter + FVector(0, HalfSize, 0),
                 InColor, InDuration, InThickness);

    // Draw Z axis line
    DrawDebugLine(InWorldContextObject,
                 InCenter + FVector(0, 0, -HalfSize), InCenter + FVector(0, 0, HalfSize),
                 InColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugAxis(
        const UObject* InWorldContextObject,
        const FVector InCenter,
        const FVector InDirection,
        float InLength,
        FLinearColor InColor,
        float InDuration,
        float InThickness,
        bool InDrawArrowHead,
        float InArrowSize)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (InDirection.IsNearlyZero())
    { return; }

    const auto NormalizedDirection = InDirection.GetSafeNormal();
    const auto AxisEnd = InCenter + NormalizedDirection * InLength;

    if (InDrawArrowHead)
    {
        DrawDebugArrow(InWorldContextObject,
                      InCenter, AxisEnd, InArrowSize, InColor, InDuration, InThickness);
    }
    else
    {
        DrawDebugLine(InWorldContextObject,
                     InCenter, AxisEnd, InColor, InDuration, InThickness);
    }
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugWireframeSphere(
        const UObject* InWorldContextObject,
        const FVector InCenter,
        float InRadius,
        int32 InRings,
        int32 InSegments,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    // Draw horizontal rings
    for (int32 Ring = 0; Ring <= InRings; ++Ring)
    {
        const auto Angle = PI * Ring / InRings - PI * 0.5f;
        const auto RingRadius = InRadius * FMath::Cos(Angle);
        const auto RingHeight = InRadius * FMath::Sin(Angle);
        const auto RingCenter = InCenter + FVector(0, 0, RingHeight);

        DrawDebugCircle(InWorldContextObject,
                        RingCenter, RingRadius, InSegments, InLineColor, InDuration, InThickness,
                        FVector(1, 0, 0), FVector(0, 1, 0), false);
    }

    // Draw vertical meridians
    for (int32 Meridian = 0; Meridian < InSegments; ++Meridian)
    {
        const auto MeridianAngle = 2 * PI * Meridian / InSegments;
        const auto XAxis = FVector(FMath::Cos(MeridianAngle), FMath::Sin(MeridianAngle), 0);
        const auto YAxis = FVector(0, 0, 1);

        DrawDebugCircle(InWorldContextObject,
                        InCenter, InRadius, InRings * 2, InLineColor, InDuration, InThickness,
                        XAxis, YAxis, false);
    }
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugTriangle(
        const UObject* InWorldContextObject,
        const FVector InVertex1,
        const FVector InVertex2,
        const FVector InVertex3,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    DrawDebugLine(InWorldContextObject,
                 InVertex1, InVertex2, InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject,
                 InVertex2, InVertex3, InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject,
                 InVertex3, InVertex1, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugPolygon(
        const UObject* InWorldContextObject,
        const TArray<FVector>& InVertices,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness,
        bool InClosedLoop)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (InVertices.Num() < 2)
    { return; }

    // Draw lines between consecutive vertices
    for (int32 I = 0; I < InVertices.Num() - 1; ++I)
    {
        DrawDebugLine(InWorldContextObject,
                     InVertices[I], InVertices[I + 1], InLineColor, InDuration, InThickness);
    }

    // Close the loop if requested
    if (InClosedLoop && InVertices.Num() > 2)
    {
        DrawDebugLine(InWorldContextObject,
                     InVertices.Last(), InVertices[0], InLineColor, InDuration, InThickness);
    }
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugWireframeBox(
        const UObject* InWorldContextObject,
        const FVector InCenter,
        const FVector InExtent,
        const FQuat InRotation,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    // Calculate box vertices
    const auto Transform = FTransform(InRotation, InCenter);
    const auto HalfExtent = InExtent;

    // Define the 8 corners of a box relative to center
    const TArray LocalVertices = {
        FVector(-HalfExtent.X, -HalfExtent.Y, -HalfExtent.Z), // 0: Front-Bottom-Left
        FVector( HalfExtent.X, -HalfExtent.Y, -HalfExtent.Z), // 1: Front-Bottom-Right
        FVector( HalfExtent.X,  HalfExtent.Y, -HalfExtent.Z), // 2: Back-Bottom-Right
        FVector(-HalfExtent.X,  HalfExtent.Y, -HalfExtent.Z), // 3: Back-Bottom-Left
        FVector(-HalfExtent.X, -HalfExtent.Y,  HalfExtent.Z), // 4: Front-Top-Left
        FVector( HalfExtent.X, -HalfExtent.Y,  HalfExtent.Z), // 5: Front-Top-Right
        FVector( HalfExtent.X,  HalfExtent.Y,  HalfExtent.Z), // 6: Back-Top-Right
        FVector(-HalfExtent.X,  HalfExtent.Y,  HalfExtent.Z)  // 7: Back-Top-Left
    };

    // Transform vertices to world space
    auto WorldVertices = TArray<FVector>{};
    WorldVertices.Reserve(8);
    for (const auto& LocalVertex : LocalVertices)
    {
        WorldVertices.Add(Transform.TransformPosition(LocalVertex));
    }

    // Draw bottom face
    DrawDebugLine(InWorldContextObject,
                 WorldVertices[0], WorldVertices[1], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject,
                 WorldVertices[1], WorldVertices[2], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject,
                 WorldVertices[2], WorldVertices[3], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject,
                 WorldVertices[3], WorldVertices[0], InLineColor, InDuration, InThickness);

    // Draw top face
    DrawDebugLine(InWorldContextObject,
                 WorldVertices[4], WorldVertices[5], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject,
                 WorldVertices[5], WorldVertices[6], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject,
                 WorldVertices[6], WorldVertices[7], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject,
                 WorldVertices[7], WorldVertices[4], InLineColor, InDuration, InThickness);

    // Draw vertical edges
    DrawDebugLine(InWorldContextObject,
                 WorldVertices[0], WorldVertices[4], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject,
                 WorldVertices[1], WorldVertices[5], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject,
                 WorldVertices[2], WorldVertices[6], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject,
                 WorldVertices[3], WorldVertices[7], InLineColor, InDuration, InThickness);
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DebugDraw_Screen_UE::
    DrawDebugRect_OnScreen(
        const UObject* InWorldContextObject,
        const FBox2D& InRect,
        FLinearColor InRectColor)
    -> void
{
    if (ck::Is_NOT_Valid(InWorldContextObject))
    { return; }

    const auto& World = InWorldContextObject->GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return; }

    const auto& DebugDrawSubsystem = World->GetSubsystem<UCk_DebugDraw_Subsystem_UE>();

    if (ck::Is_NOT_Valid(DebugDrawSubsystem))
    { return; }

    DebugDrawSubsystem->Request_DrawRect_OnScreen(
        FCk_Request_DebugDrawOnScreen_Rect{InRect}
        .Set_RectColor(InRectColor));
}

auto
    UCk_Utils_DebugDraw_Screen_UE::
    DrawDebugHollowRect_OnScreen(
        const UObject* InWorldContextObject,
        const FBox2D& InRect,
        FLinearColor InRectColor,
        float InLineThickness)
    -> void
{
    if (ck::Is_NOT_Valid(InWorldContextObject))
    { return; }

    const auto& World = InWorldContextObject->GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return; }

    const auto& DebugDrawSubsystem = World->GetSubsystem<UCk_DebugDraw_Subsystem_UE>();

    if (ck::Is_NOT_Valid(DebugDrawSubsystem))
    { return; }

    UCk_Utils_Geometry2D_UE::ForEach_BoxEdges(InRect, [&](const FCk_LineSegment2D& InLineSegment)
    {
        DebugDrawSubsystem->Request_DrawLine_OnScreen(
            FCk_Request_DebugDrawOnScreen_Line{InLineSegment.Get_LineStart(), InLineSegment.Get_LineEnd()}
            .Set_LineColor(InRectColor)
            .Set_LineThickness(InLineThickness));
    });
}

auto
    UCk_Utils_DebugDraw_Screen_UE::
    DrawDebugLine_OnScreen(
        const UObject* InWorldContextObject,
        FVector2D InLineStart,
        FVector2D InLineEnd,
        FLinearColor InLineColor,
        float InLineThickness)
    -> void
{
    if (ck::Is_NOT_Valid(InWorldContextObject))
    { return; }

    const auto& World = InWorldContextObject->GetWorld();

    if (ck::Is_NOT_Valid(World))
    { return; }

    const auto& DebugDrawSubsystem = World->GetSubsystem<UCk_DebugDraw_Subsystem_UE>();

    if (ck::Is_NOT_Valid(DebugDrawSubsystem))
    { return; }

    DebugDrawSubsystem->Request_DrawLine_OnScreen(
        FCk_Request_DebugDrawOnScreen_Line{InLineStart, InLineEnd}
        .Set_LineColor(InLineColor)
        .Set_LineThickness(InLineThickness));
}

auto
    UCk_Utils_DebugDraw_Screen_UE::
    DrawDebugText_OnScreen(
        const UObject* InWorldContextObject,
        FVector2D InTextPosition,
        const FString& InText,
        FLinearColor InTextColor,
        float InTextScale)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject), TEXT("WorldContextObject is not valid"))
    { return; }

    const auto World = InWorldContextObject->GetWorld();
    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("World is not valid"))
    { return; }

    auto DebugSubsystem = World->GetSubsystem<UCk_DebugDraw_Subsystem_UE>();
    CK_ENSURE_IF_NOT(ck::IsValid(DebugSubsystem), TEXT("DebugDraw subsystem not available"))
    { return; }

    DebugSubsystem->Request_DrawText_OnScreen(
        FCk_Request_DebugDrawOnScreen_Text{InTextPosition, InText}
        .Set_TextColor(InTextColor)
        .Set_TextScale(InTextScale)
    );
}

// --------------------------------------------------------------------------------------------------------------------
