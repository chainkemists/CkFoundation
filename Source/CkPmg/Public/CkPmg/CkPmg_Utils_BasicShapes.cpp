#include "CkPmg_Utils_BasicShapes.h"
#include "CkPmg_Utils.h"
#include "CkPmg_Fragment.h"
#include "CkPmg_Fragment_BasicShapes.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------
// Sphere
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Sphere(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        int32 InRings,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_Sphere_Params{};
    Params.Set_Radius(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Sphere_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Create_Sphere(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        int32 InRings,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Sphere(NewEntity, InTransform, InRadius, InSegments, InRings, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Sphere(NewEntity, Transform, InRadius, InSegments, InRings, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Box
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Box(
        FCk_Handle& InHandle,
        FTransform InTransform,
        FVector InExtent,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_Box_Params{};
    Params.Set_Extent(InExtent);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Box_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Create_Box(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        FVector InExtent,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Box(NewEntity, InTransform, InExtent, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    DrawFilledBox(
        const UObject* InWorldContextObject,
        FVector InCenter,
        FVector InExtent,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Box(NewEntity, Transform, InExtent, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Cone
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Cone(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        int32 InSegments,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_Cone_Params{};
    Params.Set_Radius(InRadius);
    Params.Set_Height(InHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Cone_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Create_Cone(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        int32 InSegments,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Cone(NewEntity, InTransform, InRadius, InHeight, InSegments, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    DrawFilledCone(
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Cone(NewEntity, Transform, InRadius, InHeight, InSegments, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Cylinder
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Cylinder(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        int32 InSegments,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_Cylinder_Params{};
    Params.Set_Radius(InRadius);
    Params.Set_Height(InHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Cylinder_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Create_Cylinder(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        float InHeight,
        int32 InSegments,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Cylinder(NewEntity, InTransform, InRadius, InHeight, InSegments, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Cylinder(NewEntity, Transform, InRadius, InHeight, InSegments, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Capsule
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Capsule(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        float InHalfHeight,
        int32 InSegments,
        int32 InRings,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_Capsule_Params{};
    Params.Set_Radius(InRadius);
    Params.Set_HalfHeight(InHalfHeight);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Capsule_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
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
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Capsule(NewEntity, InTransform, InRadius, InHalfHeight, InSegments, InRings, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    DrawFilledCapsule(
        const UObject* InWorldContextObject,
        FVector InCenter,
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Capsule(NewEntity, Transform, InRadius, InHalfHeight, InSegments, InRings, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Pyramid
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Pyramid(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InBaseSize,
        float InHeight,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_Pyramid_Params{};
    Params.Set_BaseSize(InBaseSize);
    Params.Set_Height(InHeight);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Pyramid_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Create_Pyramid(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InBaseSize,
        float InHeight,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Pyramid(NewEntity, InTransform, InBaseSize, InHeight, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    DrawFilledPyramid(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InBaseSize,
        float InHeight,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Pyramid(NewEntity, Transform, InBaseSize, InHeight, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Hemisphere
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Hemisphere(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        int32 InRings,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_Hemisphere_Params{};
    Params.Set_Radius(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Rings(InRings);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Hemisphere_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
}

auto
    UCk_Utils_Pmg_BasicShapes::
    Create_Hemisphere(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        int32 InRings,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Hemisphere(NewEntity, InTransform, InRadius, InSegments, InRings, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

auto
    UCk_Utils_Pmg_BasicShapes::
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Hemisphere(NewEntity, Transform, InRadius, InSegments, InRings, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
// Torus
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_BasicShapes::
    Add_Torus(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InMajorRadius,
        float InMinorRadius,
        int32 InMajorSegments,
        int32 InMinorSegments,
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Common = ck::FFragment_Pmg_DebugShape_Common{};
    Common.Set_Color(InColor);
    Common.Set_DrawLines(InDrawLines);
    Common.Set_LineThickness(InLineThickness);
    Common.Set_Duration(FCk_Time{InDuration});
    InHandle.Add<ck::FFragment_Pmg_DebugShape_Common>(Common);

    auto Params = ck::FFragment_Pmg_Torus_Params{};
    Params.Set_MajorRadius(InMajorRadius);
    Params.Set_MinorRadius(InMinorRadius);
    Params.Set_MajorSegments(InMajorSegments);
    Params.Set_MinorSegments(InMinorSegments);
    Params.Set_Axis(InDefaultAxis);
    InHandle.Add<ck::FFragment_Pmg_Torus_Params>(Params);

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);

    return UCk_Utils_Pmg_DebugShape_UE::Cast(InHandle);
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
        ECk_Plane_Axis InDefaultAxis,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    return Add_Torus(NewEntity, InTransform, InMajorRadius, InMinorRadius, InMajorSegments, InMinorSegments, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InWorldContextObject);
    const auto Transform = FTransform{FRotator::ZeroRotator, InCenter, FVector::OneVector};
    return Add_Torus(NewEntity, Transform, InMajorRadius, InMinorRadius, InMajorSegments, InMinorSegments, InDefaultAxis, InColor, InDrawLines, InLineThickness, InDuration);
}

// --------------------------------------------------------------------------------------------------------------------
