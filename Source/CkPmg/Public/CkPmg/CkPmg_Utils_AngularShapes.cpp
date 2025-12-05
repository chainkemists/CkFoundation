#include "CkPmg_Utils_AngularShapes.h"
#include "CkPmg_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Add_ functions
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_AngularShapes::
    Add_Cone(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cone);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Add_Wedge(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InStartAngle,
        float InEndAngle,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Wedge);
    Params.Set_Size(InRadius);
    Params.Set_StartAngle(InStartAngle);
    Params.Set_EndAngle(InEndAngle);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Add_Arc(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InStartAngle,
        float InEndAngle,
        float InThickness,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Arc);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InThickness);
    Params.Set_StartAngle(InStartAngle);
    Params.Set_EndAngle(InEndAngle);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Add_Frustum(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InLength,
        float InNearWidth,
        float InNearHeight,
        float InFarWidth,
        float InFarHeight,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Frustum);
    Params.Set_Size(InLength);
    Params.Set_NearWidth(InNearWidth);
    Params.Set_NearHeight(InNearHeight);
    Params.Set_FarWidth(InFarWidth);
    Params.Set_FarHeight(InFarHeight);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Add_WedgeCone(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        float InStartAngle,
        float InEndAngle,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::WedgeCone);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHeight);
    Params.Set_StartAngle(InStartAngle);
    Params.Set_EndAngle(InEndAngle);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Add_Pyramid(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InBaseSize,
        float InHeight,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Pyramid);
    Params.Set_Size(InBaseSize);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Add_Hemisphere(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        int32 InRings,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Hemisphere);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------
// Create_ functions
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_AngularShapes::
    Create_Cone(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cone);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Create_Wedge(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        float InStartAngle,
        float InEndAngle,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Wedge);
    Params.Set_Size(InRadius);
    Params.Set_StartAngle(InStartAngle);
    Params.Set_EndAngle(InEndAngle);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Create_Arc(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        float InStartAngle,
        float InEndAngle,
        float InThickness,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Arc);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InThickness);
    Params.Set_StartAngle(InStartAngle);
    Params.Set_EndAngle(InEndAngle);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Create_Frustum(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InLength,
        float InNearWidth,
        float InNearHeight,
        float InFarWidth,
        float InFarHeight,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Frustum);
    Params.Set_Size(InLength);
    Params.Set_NearWidth(InNearWidth);
    Params.Set_NearHeight(InNearHeight);
    Params.Set_FarWidth(InFarWidth);
    Params.Set_FarHeight(InFarHeight);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Create_WedgeCone(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        float InStartAngle,
        float InEndAngle,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::WedgeCone);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHeight);
    Params.Set_StartAngle(InStartAngle);
    Params.Set_EndAngle(InEndAngle);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Create_Pyramid(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InBaseSize,
        float InHeight,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Pyramid);
    Params.Set_Size(InBaseSize);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    Create_Hemisphere(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        int32 InRings,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Hemisphere);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------
// DrawFilled_ functions
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_AngularShapes::
    DrawFilledCone(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        float InRadius,
        float InHeight,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cone);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InOrigin, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    DrawFilledWedge(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        float InStartAngle,
        float InEndAngle,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Wedge);
    Params.Set_Size(InRadius);
    Params.Set_StartAngle(InStartAngle);
    Params.Set_EndAngle(InEndAngle);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    DrawFilledArc(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        float InStartAngle,
        float InEndAngle,
        float InThickness,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Arc);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InThickness);
    Params.Set_StartAngle(InStartAngle);
    Params.Set_EndAngle(InEndAngle);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    DrawFilledFrustum(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        FRotator InRotation,
        float InLength,
        float InNearWidth,
        float InNearHeight,
        float InFarWidth,
        float InFarHeight,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Frustum);
    Params.Set_Size(InLength);
    Params.Set_NearWidth(InNearWidth);
    Params.Set_NearHeight(InNearHeight);
    Params.Set_FarWidth(InFarWidth);
    Params.Set_FarHeight(InFarHeight);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(InRotation, InOrigin, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    DrawFilledWedgeCone(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        FRotator InRotation,
        float InRadius,
        float InHeight,
        float InStartAngle,
        float InEndAngle,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::WedgeCone);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHeight);
    Params.Set_StartAngle(InStartAngle);
    Params.Set_EndAngle(InEndAngle);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(InRotation, InOrigin, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    DrawFilledPyramid(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        float InBaseSize,
        float InHeight,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Pyramid);
    Params.Set_Size(InBaseSize);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InOrigin, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_AngularShapes::
    DrawFilledHemisphere(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        int32 InSegments,
        int32 InRings,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Hemisphere);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

// --------------------------------------------------------------------------------------------------------------------
