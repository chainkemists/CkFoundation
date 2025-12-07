#include "CkPmg_Processor_IconShapes.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkPmg_Utils_FlatShapes.h"

// --------------------------------------------------------------------------------------------------------------------
// Warning: Triangle with exclamation mark (triangle + bar + dot)
// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessor_Pmg_Warning_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_Warning_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
    -> void
{
    InHandle.Remove<FTag_Pmg_DebugShape_NeedsSetup>();

    CK_ENSURE_IF_NOT(InHandle.Has<FFragment_Transform>(), TEXT("Warning shape requires transform"))
    { return; }

    const auto& ParentTransform = InHandle.Get<FFragment_Transform>().Get_Transform();

    const auto Size = InParams.Get_Size();
    const auto Axis = InParams.Get_Axis();
    const auto Color = InCommon.Get_Color();
    const auto DrawLines = InCommon.Get_DrawLines();
    const auto LineThickness = InCommon.Get_LineThickness();
    const auto Duration = InCommon.Get_Duration().Get_Seconds();

    const auto OutlineColor = FLinearColor(Color.R, Color.G, Color.B, 1.0f);
    const auto FillColor = Color;
    
    auto HousingColor = OutlineColor;
    HousingColor.A = Color.A * 0.5f;

    auto AxisRotation = FQuat::Identity;
    switch (Axis)
    {
        case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
        case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
        case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
    }

    const auto TriangleSize = Size * 1.3f;
    const auto TriangleTransform = FTransform(AxisRotation, FVector::ZeroVector, FVector::OneVector) * ParentTransform;
    
    UCk_Utils_Pmg_FlatShapes::Create_Triangle(
        InHandle,
        TriangleTransform,
        TriangleSize,
        HousingColor,
        DrawLines,
        LineThickness,
        ECk_Plane_Axis::XY,
        Duration);

    // Calculate actual triangle bounds (Triangle shape centers vertices properly)
    const auto ActualTriangleTop = 0.211f * TriangleSize;
    const auto ActualTriangleBottom = -0.539f * TriangleSize;
    const auto ActualTriangleHeight = ActualTriangleTop - ActualTriangleBottom;
    
    // Add margins from triangle edges
    const auto TopMargin = ActualTriangleHeight * 0.15f;
    const auto BottomMargin = ActualTriangleHeight * 0.15f;
    const auto ExclamationTop = ActualTriangleTop - TopMargin;
    const auto ExclamationBottom = ActualTriangleBottom + BottomMargin;
    
    const auto BarHalfWidth = Size * 0.08f;
    const auto DotRadius = Size * 0.07f;
    const auto BarDotGap = Size * 0.08f;
    
    const auto DotCenterZ = ExclamationBottom + DotRadius;
    const auto BarBottom = DotCenterZ + DotRadius + BarDotGap;
    const auto BarTop = ExclamationTop;
    const auto BarHeight = BarTop - BarBottom;
    const auto BarCenterZ = (BarTop + BarBottom) * 0.5f;

    const auto BarOffsetLocal = FVector(0.0f, BarCenterZ, 0.0f);
    const auto BarOffsetWorld = AxisRotation.RotateVector(BarOffsetLocal);
    const auto BarTransform = FTransform(AxisRotation, BarOffsetWorld, FVector::OneVector) * ParentTransform;
    UCk_Utils_Pmg_FlatShapes::Create_Plane(
        InHandle,
        BarTransform,
        BarHalfWidth * 2.0f,
        BarHeight,
        FillColor,
        DrawLines,
        LineThickness,
        ECk_Plane_Axis::XY,
        Duration);

    const auto DotOffsetLocal = FVector(0.0f, DotCenterZ, 0.0f);
    const auto DotOffsetWorld = AxisRotation.RotateVector(DotOffsetLocal);
    const auto DotTransform = FTransform(AxisRotation, DotOffsetWorld, FVector::OneVector) * ParentTransform;
    UCk_Utils_Pmg_FlatShapes::Create_Circle(
        InHandle,
        DotTransform,
        DotRadius,
        16,
        FillColor,
        DrawLines,
        LineThickness,
        false,
        ECk_Plane_Axis::XY,
        Duration);
}

// --------------------------------------------------------------------------------------------------------------------
// Prohibition: Circle with diagonal X bars
// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessor_Pmg_Prohibition_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_Prohibition_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
    -> void
{
    InHandle.Remove<FTag_Pmg_DebugShape_NeedsSetup>();

    CK_ENSURE_IF_NOT(InHandle.Has<FFragment_Transform>(), TEXT("Prohibition shape requires transform"))
    { return; }

    const auto& ParentTransform = InHandle.Get<FFragment_Transform>().Get_Transform();

    const auto Radius = InParams.Get_Radius();
    const auto Segments = InParams.Get_Segments();
    const auto Axis = InParams.Get_Axis();
    const auto Color = InCommon.Get_Color();
    const auto DrawLines = InCommon.Get_DrawLines();
    const auto LineThickness = InCommon.Get_LineThickness();
    const auto Duration = InCommon.Get_Duration().Get_Seconds();

    const auto OutlineColor = FLinearColor(Color.R, Color.G, Color.B, 1.0f);
    const auto FillColor = Color;
    
    auto HousingColor = OutlineColor;
    HousingColor.A = Color.A * 0.5f;

    auto AxisRotation = FQuat::Identity;
    switch (Axis)
    {
        case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
        case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
        case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
    }

    const auto CircleTransform = FTransform(AxisRotation, FVector::ZeroVector, FVector::OneVector) * ParentTransform;
    UCk_Utils_Pmg_FlatShapes::Create_Circle(
        InHandle,
        CircleTransform,
        Radius,
        Segments,
        HousingColor,
        DrawLines,
        LineThickness,
        false,
        ECk_Plane_Axis::XY,
        Duration);

    const auto BarWidth = Radius * 0.15f;
    const auto BarLength = Radius * 1.3f;

    const auto Bar1Rotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(45.0f));
    const auto Bar1CombinedRotation = AxisRotation * Bar1Rotation;
    const auto Bar1Transform = FTransform(Bar1CombinedRotation, FVector::ZeroVector, FVector::OneVector) * ParentTransform;
    UCk_Utils_Pmg_FlatShapes::Create_Plane(
        InHandle,
        Bar1Transform,
        BarWidth,
        BarLength,
        FillColor,
        DrawLines,
        LineThickness,
        ECk_Plane_Axis::XY,
        Duration);

    const auto Bar2Rotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(-45.0f));
    const auto Bar2CombinedRotation = AxisRotation * Bar2Rotation;
    const auto Bar2Transform = FTransform(Bar2CombinedRotation, FVector::ZeroVector, FVector::OneVector) * ParentTransform;
    UCk_Utils_Pmg_FlatShapes::Create_Plane(
        InHandle,
        Bar2Transform,
        BarWidth,
        BarLength,
        FillColor,
        DrawLines,
        LineThickness,
        ECk_Plane_Axis::XY,
        Duration);
}

// --------------------------------------------------------------------------------------------------------------------
// NoEntry: Circle with horizontal bar
// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessor_Pmg_NoEntry_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_NoEntry_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
    -> void
{
    InHandle.Remove<FTag_Pmg_DebugShape_NeedsSetup>();

    CK_ENSURE_IF_NOT(InHandle.Has<FFragment_Transform>(), TEXT("NoEntry shape requires transform"))
    { return; }

    const auto& ParentTransform = InHandle.Get<FFragment_Transform>().Get_Transform();

    const auto Radius = InParams.Get_Radius();
    const auto Segments = InParams.Get_Segments();
    const auto Axis = InParams.Get_Axis();
    const auto Color = InCommon.Get_Color();
    const auto DrawLines = InCommon.Get_DrawLines();
    const auto LineThickness = InCommon.Get_LineThickness();
    const auto Duration = InCommon.Get_Duration().Get_Seconds();

    const auto OutlineColor = FLinearColor(Color.R, Color.G, Color.B, 1.0f);
    const auto FillColor = Color;
    
    auto HousingColor = OutlineColor;
    HousingColor.A = Color.A * 0.5f;

    auto AxisRotation = FQuat::Identity;
    switch (Axis)
    {
        case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
        case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
        case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
    }

    const auto CircleTransform = FTransform(AxisRotation, FVector::ZeroVector, FVector::OneVector) * ParentTransform;
    UCk_Utils_Pmg_FlatShapes::Create_Circle(
        InHandle,
        CircleTransform,
        Radius,
        Segments,
        HousingColor,
        DrawLines,
        LineThickness,
        false,
        ECk_Plane_Axis::XY,
        Duration);

    const auto BarWidth = Radius * 1.3f;
    const auto BarHeight = Radius * 0.25f;

    const auto BarTransform = FTransform(AxisRotation, FVector::ZeroVector, FVector::OneVector) * ParentTransform;
    UCk_Utils_Pmg_FlatShapes::Create_Plane(
        InHandle,
        BarTransform,
        BarWidth,
        BarHeight,
        FillColor,
        DrawLines,
        LineThickness,
        ECk_Plane_Axis::XY,
        Duration);
}

// --------------------------------------------------------------------------------------------------------------------
// InfoCircle: Circle with "i" (dot on top, bar on bottom)
// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FProcessor_Pmg_InfoCircle_Setup::
    ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_InfoCircle_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
    -> void
{
    InHandle.Remove<FTag_Pmg_DebugShape_NeedsSetup>();

    CK_ENSURE_IF_NOT(InHandle.Has<FFragment_Transform>(), TEXT("InfoCircle shape requires transform"))
    { return; }

    const auto& ParentTransform = InHandle.Get<FFragment_Transform>().Get_Transform();

    const auto Radius = InParams.Get_Radius();
    const auto Segments = InParams.Get_Segments();
    const auto Axis = InParams.Get_Axis();
    const auto Color = InCommon.Get_Color();
    const auto DrawLines = InCommon.Get_DrawLines();
    const auto LineThickness = InCommon.Get_LineThickness();
    const auto Duration = InCommon.Get_Duration().Get_Seconds();

    const auto OutlineColor = FLinearColor(Color.R, Color.G, Color.B, 1.0f);
    const auto FillColor = Color;
    
    auto HousingColor = OutlineColor;
    HousingColor.A = Color.A * 0.5f;

    auto AxisRotation = FQuat::Identity;
    switch (Axis)
    {
        case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
        case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
        case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
    }

    const auto CircleTransform = FTransform(AxisRotation, FVector::ZeroVector, FVector::OneVector) * ParentTransform;
    UCk_Utils_Pmg_FlatShapes::Create_Circle(
        InHandle,
        CircleTransform,
        Radius,
        Segments,
        HousingColor,
        DrawLines,
        LineThickness,
        false,
        ECk_Plane_Axis::XY,
        Duration);

    const auto DotRadius = Radius * 0.12f;
    const auto DotY = Radius * 0.3f;
    const auto BarWidth = Radius * 0.12f;
    const auto BarHeight = Radius * 0.5f;
    const auto BarTop = -Radius * 0.05f;
    const auto BarCenterY = BarTop - (BarHeight * 0.5f);

    const auto DotOffsetLocal = FVector(0.0f, DotY, 0.0f);
    const auto DotOffsetWorld = AxisRotation.RotateVector(DotOffsetLocal);
    const auto DotTransform = FTransform(AxisRotation, DotOffsetWorld, FVector::OneVector) * ParentTransform;
    UCk_Utils_Pmg_FlatShapes::Create_Circle(
        InHandle,
        DotTransform,
        DotRadius,
        8,
        FillColor,
        DrawLines,
        LineThickness,
        false,
        ECk_Plane_Axis::XY,
        Duration);

    const auto BarOffsetLocal = FVector(0.0f, BarCenterY, 0.0f);
    const auto BarOffsetWorld = AxisRotation.RotateVector(BarOffsetLocal);
    const auto BarTransform = FTransform(AxisRotation, BarOffsetWorld, FVector::OneVector) * ParentTransform;
    UCk_Utils_Pmg_FlatShapes::Create_Plane(
        InHandle,
        BarTransform,
        BarWidth * 2.0f,
        BarHeight,
        FillColor,
        DrawLines,
        LineThickness,
        ECk_Plane_Axis::XY,
        Duration);
}

// --------------------------------------------------------------------------------------------------------------------
