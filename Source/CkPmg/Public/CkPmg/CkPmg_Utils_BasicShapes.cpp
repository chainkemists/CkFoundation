#include "CkPmg_Utils_BasicShapes.h"
#include "CkPmg_Utils.h"
#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_BasicShapes.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkPmg/CkPmg_Fragment_FlatShapes.h"

// --------------------------------------------------------------------------------------------------------------------
// Add_ functions
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Sphere(
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
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Sphere);
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

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Box(
        FCk_Handle& InHandle,
        FTransform InTransform,
        FVector InExtent,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Box);
    Params.Set_Size(InExtent.X);
    Params.Set_SecondarySize(InExtent.Y);
    Params.Set_TertiarySize(InExtent.Z);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Circle(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        bool InDrawDirectionLine,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    // Add common fragment
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    // Add shape-specific fragment
    auto Params = ck::FFragment_Pmg_Circle_Params{};
    Params.Set_Radius(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_DrawDirectionLine(InDrawDirectionLine);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Circle_Params>(Params);

    // Add current and setup tag
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    // Add transform
    UCk_Utils_Transform_UE::Add(InHandle, InTransform);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Cylinder(
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
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cylinder);
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
    UCk_Utils_Pmg_BasicShapes::
    Add_Capsule(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InHalfHeight,
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
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Capsule);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHalfHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Plane(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InWidth,
        float InHeight,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Plane);
    Params.Set_Size(InWidth);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Torus(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InMajorRadius,
        float InMinorRadius,
        int32 InMajorSegments,
        int32 InMinorSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Torus);
    Params.Set_Size(InMajorRadius);
    Params.Set_SecondarySize(InMinorRadius);
    Params.Set_Segments(InMajorSegments);
    Params.Set_Rings(InMinorSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Ring(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InOuterRadius,
        float InInnerRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Ring);
    Params.Set_Size(InOuterRadius);
    Params.Set_InnerRadius(InInnerRadius);
    Params.Set_Segments(InSegments);
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
    UCk_Utils_Pmg_BasicShapes::
    Create_Sphere(
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
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Sphere);
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

auto
    UCk_Utils_Pmg_BasicShapes::
    Create_Box(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        FVector InExtent,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Box);
    Params.Set_Size(InExtent.X);
    Params.Set_SecondarySize(InExtent.Y);
    Params.Set_TertiarySize(InExtent.Z);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Create_Circle(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        bool InDrawDirectionLine,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Circle(NewEntity, InTransform, InRadius, InSegments, InColor, InDrawLines, InLineThickness, InDrawDirectionLine, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Create_Cylinder(
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
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cylinder);
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
    UCk_Utils_Pmg_BasicShapes::
    Create_Capsule(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        float InHalfHeight,
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
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Capsule);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHalfHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Create_Plane(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InWidth,
        float InHeight,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Plane);
    Params.Set_Size(InWidth);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Create_Torus(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InMajorRadius,
        float InMinorRadius,
        int32 InMajorSegments,
        int32 InMinorSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Torus);
    Params.Set_Size(InMajorRadius);
    Params.Set_SecondarySize(InMinorRadius);
    Params.Set_Segments(InMajorSegments);
    Params.Set_Rings(InMinorSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return UCk_Utils_Pmg_DebugShape_UE::Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Create_Ring(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InOuterRadius,
        float InInnerRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Ring);
    Params.Set_Size(InOuterRadius);
    Params.Set_InnerRadius(InInnerRadius);
    Params.Set_Segments(InSegments);
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
    UCk_Utils_Pmg_BasicShapes::
    DrawFilledSphere(
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
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Sphere);
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

auto
    UCk_Utils_Pmg_BasicShapes::
    DrawFilledBox(
        const UObject* InWorldContextObject,
        FVector InCenter,
        FVector InExtent,
        FRotator InRotation,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Box);
    Params.Set_Size(InExtent.X);
    Params.Set_SecondarySize(InExtent.Y);
    Params.Set_TertiarySize(InExtent.Z);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(InRotation, InCenter, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    DrawFilledCircle(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        bool InDrawDirectionLine,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Circle(NewEntity, Transform, InRadius, InSegments, InColor, InDrawLines, InLineThickness, InDrawDirectionLine, InDefaultAxis, InDuration);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    DrawFilledCylinder(
        const UObject* InWorldContextObject,
        FVector InCenter,
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
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cylinder);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHeight);
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
    UCk_Utils_Pmg_BasicShapes::
    DrawFilledCapsule(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        float InHalfHeight,
        FRotator InRotation,
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
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Capsule);
    Params.Set_Size(InRadius);
    Params.Set_SecondarySize(InHalfHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(InRotation, InCenter, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    DrawFilledPlane(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InWidth,
        float InHeight,
        FRotator InRotation,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Plane);
    Params.Set_Size(InWidth);
    Params.Set_SecondarySize(InHeight);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(InRotation, InCenter, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    DrawFilledTorus(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InMajorRadius,
        float InMinorRadius,
        int32 InMajorSegments,
        int32 InMinorSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Torus);
    Params.Set_Size(InMajorRadius);
    Params.Set_SecondarySize(InMinorRadius);
    Params.Set_Segments(InMajorSegments);
    Params.Set_Rings(InMinorSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    DrawFilledRing(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InOuterRadius,
        float InInnerRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Ring);
    Params.Set_Size(InOuterRadius);
    Params.Set_InnerRadius(InInnerRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return UCk_Utils_Pmg_DebugShape_UE::Create_TransientOwner(InWorldContextObject, Params, Transform);
}

// --------------------------------------------------------------------------------------------------------------------
