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
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InCenter,
        float InRadius,
        int32 InSegments,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugSphere(InWorldContextObject, InCenter, InRadius, InSegments, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugLine(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InLineStart,
        const FVector InLineEnd,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugLine(InWorldContextObject, InLineStart, InLineEnd, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugCircle(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugCircle(InWorldContextObject, InCenter, InRadius, InNumSegments, InLineColor, InDuration, InThickness, InYAxis, InZAxis, InDrawAxis);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugPoint(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InPosition,
        float InSize,
        FLinearColor InPointColor,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugPoint(InWorldContextObject, InPosition, InSize, InPointColor, InDuration);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugArrow(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InLineStart,
        const FVector InLineEnd,
        float InArrowSize,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugArrow(InWorldContextObject, InLineStart, InLineEnd, InArrowSize, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugBox(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InCenter,
        FVector InExtent,
        FLinearColor InLineColor,
        const FRotator InRotation,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugBox(InWorldContextObject, InCenter, InExtent, InLineColor, InRotation, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugCylinder(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugCylinder(InWorldContextObject, InStart, InEnd, InRadius, InSegments, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugCone(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugCone(InWorldContextObject, InOrigin, InDirection, InLength, InAngleWidth, InAngleHeight, InNumSides, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugCapsule(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugCapsule(InWorldContextObject, InCenter, InHalfHeight, InRadius, InRotation, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugString(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InTextLocation,
        const FString& InText,
        AActor* InTestBaseActor,
        FLinearColor InTextColor,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugString(InWorldContextObject, InTextLocation, InText, InTestBaseActor, InTextColor, InDuration);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugBigArrow(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    const auto ForwardVector = FRotationMatrix(InDirection).GetScaledAxis(EAxis::X);
    const auto RightVector = FRotationMatrix(InDirection).GetScaledAxis(EAxis::Y);

    const auto LeftLineStart = InLocation + RightVector * -InArrowLineWidth;
    const auto RightLineStart = InLocation + RightVector * InArrowLineWidth;
    const auto LeftLineEnd = LeftLineStart + ForwardVector * InArrowLineHeight;
    const auto RightLineEnd = RightLineStart + ForwardVector * InArrowLineHeight;

    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity, LeftLineStart, LeftLineEnd, InArrowColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity, RightLineStart, RightLineEnd, InArrowColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity, LeftLineStart, RightLineStart, InArrowColor, InDuration, InThickness);

    const auto LeftBaseArrowEnd = LeftLineEnd + RightVector * -InArrowLineWidth / 2.f;
    const auto RightBaseArrowEnd = RightLineEnd + RightVector * InArrowLineWidth / 2.f;

    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity, LeftLineEnd, LeftBaseArrowEnd, InArrowColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity, RightLineEnd, RightBaseArrowEnd, InArrowColor, InDuration, InThickness);

    const auto ArrowTip = InLocation + ForwardVector * (InArrowLineHeight + InArrowLineHeight / 3.f);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity, LeftBaseArrowEnd, ArrowTip, InArrowColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity, RightBaseArrowEnd, ArrowTip, InArrowColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugTransformGizmo(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FTransform& InTransform,
        float InAxisLength,
        float InAxisThickness,
        bool InDrawAxisCones,
        float InConeSize,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    const auto& Origin = InTransform.GetLocation();
    const auto& Rotation = InTransform.GetRotation();

    // Get axis vectors
    const auto XAxis = Rotation.GetAxisX();
    const auto YAxis = Rotation.GetAxisY();
    const auto ZAxis = Rotation.GetAxisZ();

    // Draw X axis (Red)
    const auto XEnd = Origin + XAxis * InAxisLength;
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity, Origin, XEnd, FLinearColor::Red, InDuration, InAxisThickness);

    if (InDrawAxisCones)
    {
        DrawDebugCone(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
            XEnd, XAxis, InConeSize, 15.0f, 15.0f, 8, FLinearColor::Red, InDuration, InAxisThickness * 0.5f);
    }

    // Draw Y axis (Green)
    const auto YEnd = Origin + YAxis * InAxisLength;
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity, Origin, YEnd, FLinearColor::Green, InDuration, InAxisThickness);

    if (InDrawAxisCones)
    {
        DrawDebugCone(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
            YEnd, YAxis, InConeSize, 15.0f, 15.0f, 8, FLinearColor::Green, InDuration, InAxisThickness * 0.5f);
    }

    // Draw Z axis (Blue)
    const auto ZEnd = Origin + ZAxis * InAxisLength;
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity, Origin, ZEnd, FLinearColor::Blue, InDuration, InAxisThickness);

    if (InDrawAxisCones)
    {
        DrawDebugCone(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
            ZEnd, ZAxis, InConeSize, 15.0f, 15.0f, 8, FLinearColor::Blue, InDuration, InAxisThickness * 0.5f);
    }

    // Draw small sphere at origin
    DrawDebugSphere(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        Origin, InAxisThickness * 2.0f, 8, FLinearColor::White, InDuration, InAxisThickness * 0.5f);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugCoordinateSystem(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InAxisLoc,
        const FRotator InAxisRot,
        float InScale,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugCoordinateSystem(InWorldContextObject, InAxisLoc, InAxisRot, InScale, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugPlane(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FPlane& InPlaneCoordinates,
        const FVector InLocation,
        float InSize,
        FLinearColor InPlaneColor,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugPlane(InWorldContextObject, InPlaneCoordinates, InLocation, InSize, InPlaneColor, InDuration);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugFrustum(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FTransform& InFrustumTransform,
        FLinearColor InFrustumColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugFrustum(InWorldContextObject, InFrustumTransform, InFrustumColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugFloatHistoryTransform(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FDebugFloatHistory& InFloatHistory,
        const FTransform& InDrawTransform,
        FVector2D InDrawSize,
        FLinearColor InDrawColor,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugFloatHistoryTransform(InWorldContextObject, InFloatHistory, InDrawTransform, InDrawSize, InDrawColor, InDuration);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugFloatHistoryLocation(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FDebugFloatHistory& InFloatHistory,
        FVector InDrawLocation,
        FVector2D InDrawSize,
        FLinearColor InDrawColor,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

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
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    const auto HalfGridSize = InGridSize * 0.5f;
    const auto LineSpacing = InGridSize / InGridLines;

    // Draw grid lines along forward direction
    for (int32 i = 0; i <= InGridLines; ++i)
    {
        const auto Offset = (i * LineSpacing) - HalfGridSize;
        const auto LineStart = InCenter + InRight * Offset - InForward * HalfGridSize;
        const auto LineEnd = InCenter + InRight * Offset + InForward * HalfGridSize;

        DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                     LineStart, LineEnd, InGridColor, InDuration, InThickness);
    }

    // Draw grid lines along right direction
    for (int32 i = 0; i <= InGridLines; ++i)
    {
        const auto Offset = (i * LineSpacing) - HalfGridSize;
        const auto LineStart = InCenter + InForward * Offset - InRight * HalfGridSize;
        const auto LineEnd = InCenter + InForward * Offset + InRight * HalfGridSize;

        DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                     LineStart, LineEnd, InGridColor, InDuration, InThickness);
    }
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugCross(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InCenter,
        float InSize,
        FLinearColor InColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    const auto HalfSize = InSize * 0.5f;

    // Draw X axis line
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 InCenter + FVector(-HalfSize, 0, 0), InCenter + FVector(HalfSize, 0, 0),
                 InColor, InDuration, InThickness);

    // Draw Y axis line
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 InCenter + FVector(0, -HalfSize, 0), InCenter + FVector(0, HalfSize, 0),
                 InColor, InDuration, InThickness);

    // Draw Z axis line
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 InCenter + FVector(0, 0, -HalfSize), InCenter + FVector(0, 0, HalfSize),
                 InColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugAxis(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    if (InDirection.IsNearlyZero())
    { return; }

    const auto NormalizedDirection = InDirection.GetSafeNormal();
    const auto AxisEnd = InCenter + NormalizedDirection * InLength;

    if (InDrawArrowHead)
    {
        DrawDebugArrow(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                      InCenter, AxisEnd, InArrowSize, InColor, InDuration, InThickness);
    }
    else
    {
        DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                     InCenter, AxisEnd, InColor, InDuration, InThickness);
    }
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugWireframeSphere(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    // Draw horizontal rings
    for (int32 Ring = 0; Ring <= InRings; ++Ring)
    {
        const auto Angle = PI * Ring / InRings - PI * 0.5f;
        const auto RingRadius = InRadius * FMath::Cos(Angle);
        const auto RingHeight = InRadius * FMath::Sin(Angle);
        const auto RingCenter = InCenter + FVector(0, 0, RingHeight);

        DrawDebugCircle(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                        RingCenter, RingRadius, InSegments, InLineColor, InDuration, InThickness,
                        FVector(1, 0, 0), FVector(0, 1, 0), false);
    }

    // Draw vertical meridians
    for (int32 Meridian = 0; Meridian < InSegments; ++Meridian)
    {
        const auto MeridianAngle = 2 * PI * Meridian / InSegments;
        const auto XAxis = FVector(FMath::Cos(MeridianAngle), FMath::Sin(MeridianAngle), 0);
        const auto YAxis = FVector(0, 0, 1);

        DrawDebugCircle(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                        InCenter, InRadius, InRings * 2, InLineColor, InDuration, InThickness,
                        XAxis, YAxis, false);
    }
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugTriangle(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InVertex1,
        const FVector InVertex2,
        const FVector InVertex3,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 InVertex1, InVertex2, InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 InVertex2, InVertex3, InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 InVertex3, InVertex1, InLineColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugPolygon(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const TArray<FVector>& InVertices,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness,
        bool InClosedLoop)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    if (InVertices.Num() < 2)
    { return; }

    // Draw lines between consecutive vertices
    for (int32 i = 0; i < InVertices.Num() - 1; ++i)
    {
        DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                     InVertices[i], InVertices[i + 1], InLineColor, InDuration, InThickness);
    }

    // Close the loop if requested
    if (InClosedLoop && InVertices.Num() > 2)
    {
        DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                     InVertices.Last(), InVertices[0], InLineColor, InDuration, InThickness);
    }
#endif
}

auto
    UCk_Utils_DebugDraw_UE::
    DrawDebugWireframeBox(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector InCenter,
        const FVector InExtent,
        const FQuat InRotation,
        FLinearColor InLineColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

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
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 WorldVertices[0], WorldVertices[1], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 WorldVertices[1], WorldVertices[2], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 WorldVertices[2], WorldVertices[3], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 WorldVertices[3], WorldVertices[0], InLineColor, InDuration, InThickness);

    // Draw top face
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 WorldVertices[4], WorldVertices[5], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 WorldVertices[5], WorldVertices[6], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 WorldVertices[6], WorldVertices[7], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 WorldVertices[7], WorldVertices[4], InLineColor, InDuration, InThickness);

    // Draw vertical edges
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 WorldVertices[0], WorldVertices[4], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 WorldVertices[1], WorldVertices[5], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 WorldVertices[2], WorldVertices[6], InLineColor, InDuration, InThickness);
    DrawDebugLine(InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                 WorldVertices[3], WorldVertices[7], InLineColor, InDuration, InThickness);
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DebugDraw_Screen_UE::
    DrawDebugRect_OnScreen(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FBox2D& InRect,
        FLinearColor InRectColor)
    -> void
{
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

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
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FBox2D& InRect,
        FLinearColor InRectColor,
        float InLineThickness)
    -> void
{
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

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
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        FVector2D InLineStart,
        FVector2D InLineEnd,
        FLinearColor InLineColor,
        float InLineThickness)
    -> void
{
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

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

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DebugDraw_Entity_UE::
    DrawEntityTransformGizmo(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FTransform& InTransform,
        const FString& InEntityName,
        float InAxisLength,
        float InAxisThickness,
        bool InDrawAxisCones,
        float InConeSize,
        bool InDrawEntityName,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    // Draw the transform gizmo
    UCk_Utils_DebugDraw_UE::DrawDebugTransformGizmo(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        InTransform, InAxisLength, InAxisThickness, InDrawAxisCones, InConeSize, InDuration);

    // Draw entity name if requested
    if (InDrawEntityName && !InEntityName.IsEmpty())
    {
        const auto& NameLocation = InTransform.GetLocation() + FVector(0, 0, InAxisLength * 0.5f);
        UCk_Utils_DebugDraw_UE::DrawDebugString(
            InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
            NameLocation, InEntityName, nullptr, FLinearColor::Yellow, InDuration);
    }
#endif
}

auto
    UCk_Utils_DebugDraw_Entity_UE::
    DrawEntityBounds(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector& InCenter,
        const FVector& InExtent,
        const FRotator& InRotation,
        FLinearColor InBoundsColor,
        float InThickness,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UCk_Utils_DebugDraw_UE::DrawDebugBox(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        InCenter, InExtent, InBoundsColor, InRotation, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_Entity_UE::
    DrawEntitySphereBounds(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector& InCenter,
        float InRadius,
        int32 InSegments,
        FLinearColor InBoundsColor,
        float InThickness,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UCk_Utils_DebugDraw_UE::DrawDebugSphere(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        InCenter, InRadius, InSegments, InBoundsColor, InDuration, InThickness);
#endif
}

auto
    UCk_Utils_DebugDraw_Entity_UE::
    DrawEntityMovementVector(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector& InLocation,
        const FVector& InVelocity,
        float InVelocityScale,
        FLinearColor InVelocityColor,
        float InArrowSize,
        float InThickness,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    if (InVelocity.IsNearlyZero())
    { return; }

    const auto VelocityEnd = InLocation + InVelocity * InVelocityScale;

    UCk_Utils_DebugDraw_UE::DrawDebugArrow(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        InLocation, VelocityEnd, InArrowSize, InVelocityColor, InDuration, InThickness);

    // Draw velocity magnitude as text
    const auto VelocityMagnitude = InVelocity.Size();
    const auto VelocityText = FString::Printf(TEXT("%.1f"), VelocityMagnitude);
    const auto TextLocation = FMath::Lerp(InLocation, VelocityEnd, 0.5f) + FVector(0, 0, 20);

    UCk_Utils_DebugDraw_UE::DrawDebugString(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        TextLocation, VelocityText, nullptr, InVelocityColor, InDuration);
#endif
}

auto
    UCk_Utils_DebugDraw_Entity_UE::
    DrawEntityHealthBar(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector& InLocation,
        float InHealthPercentage,
        float InBarWidth,
        float InBarHeight,
        FLinearColor InHealthColor,
        FLinearColor InBackgroundColor,
        float InOffsetZ,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    const auto HealthPercent = FMath::Clamp(InHealthPercentage, 0.0f, 1.0f);
    const auto BarLocation = InLocation + FVector(0, 0, InOffsetZ);

    // Draw background bar (full width)
    const auto BackgroundExtent = FVector(InBarWidth * 0.5f, InBarHeight * 0.5f, 1.0f);
    UCk_Utils_DebugDraw_UE::DrawDebugBox(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        BarLocation, BackgroundExtent, InBackgroundColor, FRotator::ZeroRotator, InDuration, 2.0f);

    // Draw health bar (proportional width)
    if (HealthPercent > 0.0f)
    {
        const auto HealthWidth = InBarWidth * HealthPercent;
        const auto HealthExtent = FVector(HealthWidth * 0.5f, InBarHeight * 0.5f, 1.1f);
        const auto HealthOffset = FVector((HealthWidth - InBarWidth) * 0.5f, 0, 0);

        UCk_Utils_DebugDraw_UE::DrawDebugBox(
            InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
            BarLocation + HealthOffset, HealthExtent, InHealthColor, FRotator::ZeroRotator, InDuration, 2.0f);
    }

    // Draw health percentage text
    const auto HealthText = FString::Printf(TEXT("%.0f%%"), HealthPercent * 100.0f);
    const auto TextLocation = BarLocation + FVector(0, 0, InBarHeight + 10.0f);

    UCk_Utils_DebugDraw_UE::DrawDebugString(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        TextLocation, HealthText, nullptr, FLinearColor::White, InDuration);
#endif
}

auto
    UCk_Utils_DebugDraw_Entity_UE::
    DrawEntityLineOfSight(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector& InEyeLocation,
        const FVector& InTargetLocation,
        bool InHasLineOfSight,
        FLinearColor InVisibleColor,
        FLinearColor InBlockedColor,
        float InThickness,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    const auto LineColor = InHasLineOfSight ? InVisibleColor : InBlockedColor;

    UCk_Utils_DebugDraw_UE::DrawDebugLine(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        InEyeLocation, InTargetLocation, LineColor, InDuration, InThickness);

    // Draw small spheres at endpoints
    UCk_Utils_DebugDraw_UE::DrawDebugSphere(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        InEyeLocation, 3.0f, 8, LineColor, InDuration, 1.0f);

    UCk_Utils_DebugDraw_UE::DrawDebugSphere(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        InTargetLocation, 3.0f, 8, LineColor, InDuration, 1.0f);

    // Draw status text
    const auto StatusText = InHasLineOfSight ? TEXT("VISIBLE") : TEXT("BLOCKED");
    const auto MidPoint = FMath::Lerp(InEyeLocation, InTargetLocation, 0.5f) + FVector(0, 0, 15);

    UCk_Utils_DebugDraw_UE::DrawDebugString(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        MidPoint, StatusText, nullptr, LineColor, InDuration);
#endif
}

auto
    UCk_Utils_DebugDraw_Entity_UE::
    DrawEntityVisionCone(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const FVector& InOrigin,
        const FVector& InDirection,
        float InRange,
        float InHalfAngleDegrees,
        int32 InNumSides,
        FLinearColor InVisionColor,
        float InDuration,
        float InThickness)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    // Draw the cone
    UCk_Utils_DebugDraw_UE::DrawDebugCone(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        InOrigin, InDirection, InRange, InHalfAngleDegrees, InHalfAngleDegrees,
        InNumSides, InVisionColor, InDuration, InThickness);

    // Draw center line
    const auto CenterEnd = InOrigin + InDirection * InRange;
    UCk_Utils_DebugDraw_UE::DrawDebugLine(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        InOrigin, CenterEnd, InVisionColor, InDuration, InThickness * 2.0f);

    // Draw range text
    const auto RangeText = FString::Printf(TEXT("Range: %.0f"), InRange);
    const auto TextLocation = CenterEnd + FVector(0, 0, 20);

    UCk_Utils_DebugDraw_UE::DrawDebugString(
        InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
        TextLocation, RangeText, nullptr, InVisionColor, InDuration);
#endif
}

auto
    UCk_Utils_DebugDraw_Entity_UE::
    DrawEntityPath(
        const UObject* InWorldContextObject,
        const FCk_LogCategory InOptionalLogCategory,
        ECk_LogVerbosity InOptionalLogVerbosity,
        const TArray<FVector>& InPathPoints,
        FLinearColor InPathColor,
        float InThickness,
        bool InDrawWaypoints,
        float InWaypointSize,
        float InDuration)
    -> void
{
#if ENABLE_DRAW_DEBUG
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    if (InPathPoints.Num() < 2)
    { return; }

    // Draw lines between consecutive points
    for (int32 i = 0; i < InPathPoints.Num() - 1; ++i)
    {
        UCk_Utils_DebugDraw_UE::DrawDebugLine(
            InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
            InPathPoints[i], InPathPoints[i + 1], InPathColor, InDuration, InThickness);

        // Draw arrow at midpoint to show direction
        const auto MidPoint = FMath::Lerp(InPathPoints[i], InPathPoints[i + 1], 0.5f);
        const auto Direction = (InPathPoints[i + 1] - InPathPoints[i]).GetSafeNormal();
        const auto ArrowEnd = MidPoint + Direction * (InWaypointSize * 2.0f);

        UCk_Utils_DebugDraw_UE::DrawDebugArrow(
            InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
            MidPoint, ArrowEnd, InWaypointSize * 0.5f, InPathColor, InDuration, InThickness * 0.5f);
    }

    // Draw waypoints
    if (InDrawWaypoints)
    {
        for (int32 i = 0; i < InPathPoints.Num(); ++i)
        {
            const auto WaypointColor = (i == 0) ? FLinearColor::Green :
                                      (i == InPathPoints.Num() - 1) ? FLinearColor::Red : InPathColor;

            UCk_Utils_DebugDraw_UE::DrawDebugSphere(
                InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                InPathPoints[i], InWaypointSize, 8, WaypointColor, InDuration, InThickness);

            // Draw waypoint number
            const auto WaypointText = FString::Printf(TEXT("%d"), i);
            const auto TextLocation = InPathPoints[i] + FVector(0, 0, InWaypointSize + 10);

            UCk_Utils_DebugDraw_UE::DrawDebugString(
                InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
                TextLocation, WaypointText, nullptr, WaypointColor, InDuration);
        }
    }

    // Draw total path distance
    float TotalDistance = 0.0f;
    for (int32 i = 0; i < InPathPoints.Num() - 1; ++i)
    {
        TotalDistance += FVector::Dist(InPathPoints[i], InPathPoints[i + 1]);
    }

    if (InPathPoints.Num() > 0)
    {
        const auto DistanceText = FString::Printf(TEXT("Path Length: %.1f"), TotalDistance);
        const auto TextLocation = InPathPoints[0] + FVector(0, 0, 50);

        UCk_Utils_DebugDraw_UE::DrawDebugString(
            InWorldContextObject, InOptionalLogCategory, InOptionalLogVerbosity,
            TextLocation, DistanceText, nullptr, InPathColor, InDuration);
    }
#endif
}

// --------------------------------------------------------------------------------------------------------------------
