#include "CkPmg_Utils.h"

#include "CkPmg/CkPmg_Log.h"
#include "CkPmg_Fragment.h"

#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#if WITH_EDITOR
#include "AssetUtils/CreateStaticMeshUtil.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "ProceduralMeshConversion.h"
#endif

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledArrow(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        FRotator InDirection,
        float InLength,
        float InShaftWidth,
        float InArrowHeadRatio,
        float InArrowHeadWidthMultiplier,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Arrow);
    Params.Set_Size(InLength);
    Params.Set_SecondarySize(InShaftWidth);
    Params.Set_ArrowHeadRatio(InArrowHeadRatio);
    Params.Set_ArrowHeadWidthMultiplier(InArrowHeadWidthMultiplier);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(InDirection, InOrigin, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledCross(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cross);
    Params.Set_Size(InSize);
    Params.Set_SecondarySize(InThickness);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledStar(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InOuterRadius,
        int32 InPoints,
        float InInnerRadiusRatio,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Star);
    Params.Set_Size(InOuterRadius);
    Params.Set_Points(InPoints);
    Params.Set_InnerRadiusRatio(InInnerRadiusRatio);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawDashedLine(
        const UObject* InWorldContextObject,
        FVector InStart,
        FVector InEnd,
        float InDashLength,
        float InGapLength,
        float InThickness,
        FLinearColor InColor,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    const auto Direction = InEnd - InStart;
    const auto Distance = Direction.Size();
    const auto Rotation = Direction.Rotation();

    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::DashedLine);
    Params.Set_Size(Distance);
    Params.Set_SecondarySize(InThickness);
    Params.Set_DashLength(InDashLength);
    Params.Set_GapLength(InGapLength);
    Params.Set_Color(InColor);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(Rotation, InStart, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledCheckmark(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Checkmark);
    Params.Set_Size(InSize);
    Params.Set_SecondarySize(InThickness);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledDiamond(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Diamond);
    Params.Set_Size(InSize);
    Params.Set_SecondarySize(InThickness);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawPivot(
        const UObject* InWorldContextObject,
        FVector InOrigin,
        FRotator InRotation,
        float InAxisLength,
        float InArrowSize,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Pivot);
    Params.Set_Size(InAxisLength);
    Params.Set_SecondarySize(InArrowSize);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(InRotation, InOrigin, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

// --------------------------------------------------------------------------------------------------------------------
// Add_ and Create_ implementations for Star, Plane, Pyramid, Hemisphere, DashedLine, Checkmark, Diamond, Pivot
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Star(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InOuterRadius,
        int32 InPoints,
        float InInnerRadiusRatio,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Star);
    Params.Set_Size(InOuterRadius);
    Params.Set_Points(InPoints);
    Params.Set_InnerRadiusRatio(InInnerRadiusRatio);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_DashedLine(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InLength,
        float InDashLength,
        float InGapLength,
        float InThickness,
        FLinearColor InColor,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::DashedLine);
    Params.Set_Size(InLength);
    Params.Set_SecondarySize(InThickness);
    Params.Set_DashLength(InDashLength);
    Params.Set_GapLength(InGapLength);
    Params.Set_Color(InColor);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Checkmark(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Checkmark);
    Params.Set_Size(InSize);
    Params.Set_SecondarySize(InThickness);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Diamond(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Diamond);
    Params.Set_Size(InSize);
    Params.Set_SecondarySize(InThickness);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Pivot(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InAxisLength,
        float InArrowSize,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Pivot);
    Params.Set_Size(InAxisLength);
    Params.Set_SecondarySize(InArrowSize);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

// Create_ functions for the same shapes

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Star(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InOuterRadius,
        int32 InPoints,
        float InInnerRadiusRatio,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Star);
    Params.Set_Size(InOuterRadius);
    Params.Set_Points(InPoints);
    Params.Set_InnerRadiusRatio(InInnerRadiusRatio);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_DashedLine(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InLength,
        float InDashLength,
        float InGapLength,
        float InThickness,
        FLinearColor InColor,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::DashedLine);
    Params.Set_Size(InLength);
    Params.Set_SecondarySize(InThickness);
    Params.Set_DashLength(InDashLength);
    Params.Set_GapLength(InGapLength);
    Params.Set_Color(InColor);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Checkmark(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Checkmark);
    Params.Set_Size(InSize);
    Params.Set_SecondarySize(InThickness);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Diamond(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Diamond);
    Params.Set_Size(InSize);
    Params.Set_SecondarySize(InThickness);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Pivot(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InAxisLength,
        float InArrowSize,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Pivot);
    Params.Set_Size(InAxisLength);
    Params.Set_SecondarySize(InArrowSize);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------
// Icon and Symbol Shape Implementations
// --------------------------------------------------------------------------------------------------------------------

// Add_ functions

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Warning(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Warning);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Prohibition(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Prohibition);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_NoEntry(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::NoEntry);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_MagnifyingGlass(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::MagnifyingGlass);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_QuestionMark(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::QuestionMark);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_ExclamationMark(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::ExclamationMark);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Flag(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Flag);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_InfoCircle(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::InfoCircle);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Pin(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Pin);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------
// Create_ functions

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Warning(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Warning);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Prohibition(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Prohibition);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_NoEntry(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::NoEntry);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_MagnifyingGlass(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::MagnifyingGlass);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_QuestionMark(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::QuestionMark);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_ExclamationMark(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::ExclamationMark);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Flag(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Flag);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_InfoCircle(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::InfoCircle);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Pin(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Pin);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------
// DrawFilled_ functions

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledWarning(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Warning);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledProhibition(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Prohibition);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledNoEntry(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::NoEntry);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledMagnifyingGlass(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::MagnifyingGlass);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledQuestionMark(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::QuestionMark);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledExclamationMark(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::ExclamationMark);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledFlag(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Flag);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledInfoCircle(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InRadius,
        int32 InSegments,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::InfoCircle);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    DrawFilledPin(
        const UObject* InWorldContextObject,
        FVector InCenter,
        float InSize,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Pin);
    Params.Set_Size(InSize);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_Donut_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Pmg_Donut_ParamsData& InParams)
    -> FCk_Handle_Pmg_Donut
{
    ck::pmg::VeryVerbose(TEXT("Adding Pmg Donut feature to Entity [{}]"), InHandle);

    CK_ENSURE_IF_NOT(InParams.Get_InnerRadius() < InParams.Get_OuterRadius(),
        TEXT("Inner radius must be less than outer radius for Entity [{}]"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(InParams.Get_Segments() >= 3,
        TEXT("Segments must be at least 3 for Entity [{}]"), InHandle)
    { return {}; }

    auto& ParamsFragment = InHandle.Add<ck::FFragment_Pmg_Donut_Params>();
    ParamsFragment._Params = InParams;

    InHandle.Add<ck::FFragment_Pmg_Donut_Current>();
    InHandle.Add<ck::FTag_Pmg_Donut_NeedsSetup>();

    UCk_Utils_Handle_UE::Set_DebugName(InHandle, TEXT("Pmg: Donut"));

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Pmg_Donut_UE, FCk_Handle_Pmg_Donut,
    ck::FFragment_Pmg_Donut_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_UpdateParams(
        FCk_Handle_Pmg_Donut& InDonut,
        const FCk_Request_Pmg_Donut_UpdateParams& InRequest)
    -> FCk_Handle_Pmg_Donut
{
    ck::pmg::Verbose(TEXT("Requesting param update for Pmg Donut [{}]"), InDonut);

    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>() = InRequest;

    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetInnerRadius(
        FCk_Handle_Pmg_Donut& InDonut,
        float InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_InnerRadius(InValue);
    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetOuterRadius(
        FCk_Handle_Pmg_Donut& InDonut,
        float InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_OuterRadius(InValue);
    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetSegments(
        FCk_Handle_Pmg_Donut& InDonut,
        int32 InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_Segments(InValue);
    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetFillAngle(
        FCk_Handle_Pmg_Donut& InDonut,
        float InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_FillAngle(InValue);
    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetMaterial(
        FCk_Handle_Pmg_Donut& InDonut,
        UMaterialInterface* InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_Material(InValue);
    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetEnableCollision(
        FCk_Handle_Pmg_Donut& InDonut,
        bool InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_EnableCollision(InValue);
    return InDonut;
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Request_SetRenderMode(
        FCk_Handle_Pmg_Donut& InDonut,
        ECk_Pmg_RenderMode InValue)
    -> FCk_Handle_Pmg_Donut
{
    InDonut.AddOrGet<ck::FFragment_Pmg_Donut_UpdateParams>().Set_RenderMode(InValue);
    return InDonut;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_Donut_UE::
    Get_InnerRadius(
        const FCk_Handle_Pmg_Donut& InDonut)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDonut), TEXT("Invalid Pmg Donut handle"))
    { return 0.0f; }

    CK_ENSURE_IF_NOT(InDonut.Has<ck::FFragment_Pmg_Donut_Current>(),
        TEXT("Pmg Donut [{}] has no Current fragment"), InDonut)
    { return 0.0f; }

    const auto& Current = InDonut.Get<ck::FFragment_Pmg_Donut_Current>();
    return Current.Get_InnerRadius();
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Get_OuterRadius(
        const FCk_Handle_Pmg_Donut& InDonut)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDonut), TEXT("Invalid Pmg Donut handle"))
    { return 0.0f; }

    CK_ENSURE_IF_NOT(InDonut.Has<ck::FFragment_Pmg_Donut_Current>(),
        TEXT("Pmg Donut [{}] has no Current fragment"), InDonut)
    { return 0.0f; }

    const auto& Current = InDonut.Get<ck::FFragment_Pmg_Donut_Current>();
    return Current.Get_OuterRadius();
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Get_Segments(
        const FCk_Handle_Pmg_Donut& InDonut)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDonut), TEXT("Invalid Pmg Donut handle"))
    { return 0; }

    CK_ENSURE_IF_NOT(InDonut.Has<ck::FFragment_Pmg_Donut_Current>(),
        TEXT("Pmg Donut [{}] has no Current fragment"), InDonut)
    { return 0; }

    const auto& Current = InDonut.Get<ck::FFragment_Pmg_Donut_Current>();
    return Current.Get_Segments();
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Get_FillAngle(
        const FCk_Handle_Pmg_Donut& InDonut)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDonut), TEXT("Invalid Pmg Donut handle"))
    { return 0.0f; }

    CK_ENSURE_IF_NOT(InDonut.Has<ck::FFragment_Pmg_Donut_Current>(),
        TEXT("Pmg Donut [{}] has no Current fragment"), InDonut)
    { return 0.0f; }

    const auto& Current = InDonut.Get<ck::FFragment_Pmg_Donut_Current>();
    return Current.Get_FillAngle();
}

auto
    UCk_Utils_Pmg_Donut_UE::
    Get_RenderMode(
        const FCk_Handle_Pmg_Donut& InDonut)
    -> ECk_Pmg_RenderMode
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDonut), TEXT("Invalid Pmg Donut handle"))
    { return ECk_Pmg_RenderMode::DoubleSided; }

    CK_ENSURE_IF_NOT(InDonut.Has<ck::FFragment_Pmg_Donut_Current>(),
        TEXT("Pmg Donut [{}] has no Current fragment"), InDonut)
    { return ECk_Pmg_RenderMode::DoubleSided; }

    const auto& Current = InDonut.Get<ck::FFragment_Pmg_Donut_Current>();
    return Current.Get_RenderMode();
}

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Pmg_DebugShape_ParamsData& InParams,
        FTransform InTransform)
    -> FCk_Handle_Pmg_DebugShape
{
    ck::pmg::VeryVerbose(TEXT("Adding Pmg DebugShape feature to Entity [{}]"), InHandle);

    auto& ParamsFragment = InHandle.Add<ck::FFragment_Pmg_DebugShape_Params>();
    ParamsFragment._Params = InParams;

    InHandle.Add<ck::FFragment_Pmg_DebugShape_Current>();
    InHandle.Add<ck::FTag_Pmg_DebugShape_NeedsSetup>();

    UCk_Utils_Transform_UE::Add(InHandle, InTransform, ECk_Replication::DoesNotReplicate);
    UCk_Utils_Handle_UE::Set_DebugName(InHandle, TEXT("Pmg: DebugShape"));

    return Cast(InHandle);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create(
        FCk_Handle& InOwningEntity,
        const FCk_Fragment_Pmg_DebugShape_ParamsData& InParams,
        FTransform InTransform)
    -> FCk_Handle_Pmg_DebugShape
{
    ck::pmg::VeryVerbose(TEXT("Creating Pmg DebugShape entity from Owner [{}]"), InOwningEntity);

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwningEntity);
    CK_ENSURE_IF_NOT(ck::IsValid(NewEntity), TEXT("Failed to create entity from Owner [{}]"), InOwningEntity)
    { return {}; }

    return Add(NewEntity, InParams, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_TransientOwner(
        const UObject* InWorldContextObject,
        const FCk_Fragment_Pmg_DebugShape_ParamsData& InParams,
        FTransform InTransform)
    -> FCk_Handle_Pmg_DebugShape
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject), TEXT("Invalid WorldContextObject"))
    { return {}; }

    const auto World = InWorldContextObject->GetWorld();
    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Invalid World"))
    { return {}; }

    ck::pmg::VeryVerbose(TEXT("Creating Pmg DebugShape entity with transient owner"));

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(World);
    CK_ENSURE_IF_NOT(ck::IsValid(NewEntity), TEXT("Failed to create entity with transient owner"))
    { return {}; }

    return Add(NewEntity, InParams, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Pmg_DebugShape_UE, FCk_Handle_Pmg_DebugShape,
    ck::FFragment_Pmg_DebugShape_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Circle);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DrawDirectionLine(InDrawDirectionLine);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Arrow(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InLength,
        float InShaftWidth,
        float InArrowHeadRatio,
        float InArrowHeadWidthMultiplier,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Arrow);
    Params.Set_Size(InLength);
    Params.Set_SecondarySize(InShaftWidth);
    Params.Set_ArrowHeadRatio(InArrowHeadRatio);
    Params.Set_ArrowHeadWidthMultiplier(InArrowHeadWidthMultiplier);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Add_Cross(
        FCk_Handle& InHandle,
        FTransform InTransform,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cross);
    Params.Set_Size(InSize);
    Params.Set_SecondarySize(InThickness);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Add(InHandle, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Add(InHandle, Params, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Circle);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DrawDirectionLine(InDrawDirectionLine);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Arrow(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InLength,
        float InShaftWidth,
        float InArrowHeadRatio,
        float InArrowHeadWidthMultiplier,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Arrow);
    Params.Set_Size(InLength);
    Params.Set_SecondarySize(InShaftWidth);
    Params.Set_ArrowHeadRatio(InArrowHeadRatio);
    Params.Set_ArrowHeadWidthMultiplier(InArrowHeadWidthMultiplier);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
    Create_Cross(
        FCk_Handle& InOwningEntity,
        FTransform InTransform,
        float InSize,
        float InThickness,
        FLinearColor InColor,
        bool InDrawLines,
        float InLineThickness,
        ECk_Plane_Axis InDefaultAxis,
        float InDuration)
    -> FCk_Handle_Pmg_DebugShape
{
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Cross);
    Params.Set_Size(InSize);
    Params.Set_SecondarySize(InThickness);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    return Create(InOwningEntity, Params, InTransform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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

    return Create(InOwningEntity, Params, InTransform);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    auto Params = FCk_Fragment_Pmg_DebugShape_ParamsData{};
    Params.Set_ShapeType(ECk_Pmg_DebugShape_Type::Circle);
    Params.Set_Size(InRadius);
    Params.Set_Segments(InSegments);
    Params.Set_Color(InColor);
    Params.Set_DrawLines(InDrawLines);
    Params.Set_LineThickness(InLineThickness);
    Params.Set_DrawDirectionLine(InDrawDirectionLine);
    Params.Set_DefaultAxis(InDefaultAxis);
    Params.Set_Duration(FCk_Time(InDuration));

    const auto Transform = FTransform(FRotator::ZeroRotator, InCenter, FVector::OneVector);
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

auto
    UCk_Utils_Pmg_DebugShape_UE::
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
    return Create_TransientOwner(InWorldContextObject, Params, Transform);
}

// --------------------------------------------------------------------------------------------------------------------
