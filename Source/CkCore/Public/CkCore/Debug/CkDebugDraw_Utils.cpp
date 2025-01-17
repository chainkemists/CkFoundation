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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugSphere(InWorldContextObject, InCenter, InRadius, InSegments, InLineColor, InDuration, InThickness);
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugLine(InWorldContextObject, InLineStart, InLineEnd, InLineColor, InDuration, InThickness);
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugCircle(InWorldContextObject, InCenter, InRadius, InNumSegments, InLineColor, InDuration, InThickness, InYAxis, InZAxis, InDrawAxis);
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugPoint(InWorldContextObject, InPosition, InSize, InPointColor, InDuration);
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugArrow(InWorldContextObject, InLineStart, InLineEnd, InArrowSize, InLineColor, InDuration, InThickness);
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugBox(InWorldContextObject, InCenter, InExtent, InLineColor, InRotation, InDuration, InThickness);
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugCylinder(InWorldContextObject, InStart, InEnd, InRadius, InSegments, InLineColor, InDuration, InThickness);
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugCone(InWorldContextObject, InOrigin, InDirection, InLength, InAngleWidth, InAngleHeight, InNumSides, InLineColor, InDuration, InThickness);
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugCapsule(InWorldContextObject, InCenter, InHalfHeight, InRadius, InRotation, InLineColor, InDuration, InThickness);
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
    if (const auto& LogCategoryName = InOptionalLogCategory.Get_Name();
        ck::IsValid(LogCategoryName) && NOT UCk_Utils_Log_UE::Get_IsLogActive_ForVerbosity(InOptionalLogCategory, InOptionalLogVerbosity))
    { return; }

    UKismetSystemLibrary::DrawDebugString(InWorldContextObject, InTextLocation, InText, InTestBaseActor, InTextColor, InDuration);
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
