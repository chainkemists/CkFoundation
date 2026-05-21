#include "CkSpline/CkSpline_Utils.h"

#include "CkSpline/CkSpline_Fragment.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include <Components/SplineComponent.h>

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Spline_UE, FCk_Handle_Spline, ck::FFragment_Spline_Params)

// --------------------------------------------------------------------------------------------------------------------

namespace ck_spline_detail
{
    auto DoMakeParamsFromComponent(
        const USplineComponent* InSourceSpline)
        -> FCk_Fragment_Spline_ParamsData
    {
        auto Params = FCk_Fragment_Spline_ParamsData{InSourceSpline->GetSplineCurves(), InSourceSpline->IsClosedLoop()};
        Params.Set_DefaultUpVector(InSourceSpline->GetDefaultUpVector(ESplineCoordinateSpace::Local));
        return Params;
    }

    auto DoEntityTransform(
        const FCk_Handle_Spline& InSpline)
        -> FTransform
    {
        return UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentTransform(InSpline);
    }

    auto DoInputKeyAtDistance(
        const ck::FFragment_Spline_Params& InData,
        float InDistance)
        -> float
    {
        return InData.Get_Curves().ReparamTable.Eval(InDistance, 0.0f);
    }

    auto DoWorldLocationAtKey(
        const ck::FFragment_Spline_Params& InData,
        const FTransform& InEntityTransform,
        float InKey)
        -> FVector
    {
        const auto Local = InData.Get_Curves().Position.Eval(InKey, FVector::ZeroVector);
        return InEntityTransform.TransformPosition(Local);
    }

    auto DoWorldTangentAtKey(
        const ck::FFragment_Spline_Params& InData,
        const FTransform& InEntityTransform,
        float InKey)
        -> FVector
    {
        const auto Local = InData.Get_Curves().Position.EvalDerivative(InKey, FVector::ZeroVector);
        return InEntityTransform.TransformVector(Local);
    }

    auto DoWorldQuatAtKey(
        const ck::FFragment_Spline_Params& InData,
        const FTransform& InEntityTransform,
        float InKey)
        -> FQuat
    {
        const auto& Curves = InData.Get_Curves();

        auto LocalQuat = Curves.Rotation.Eval(InKey, FQuat::Identity);
        LocalQuat.Normalize();

        const auto LocalDirection = Curves.Position.EvalDerivative(InKey, FVector::ZeroVector).GetSafeNormal();
        const auto UpVector = LocalQuat.RotateVector(InData.Get_DefaultUpVector());
        const auto LocalRotation = FRotationMatrix::MakeFromXZ(LocalDirection, UpVector).ToQuat();

        return InEntityTransform.GetRotation() * LocalRotation;
    }

    auto DoWorldScaleAtKey(
        const ck::FFragment_Spline_Params& InData,
        const FTransform& InEntityTransform,
        float InKey)
        -> FVector
    {
        const auto Local = InData.Get_Curves().Scale.Eval(InKey, FVector::OneVector);
        return InEntityTransform.GetScale3D() * Local;
    }

    // Mirrors USplineComponent::FindInputKeyClosestToWorldLocation: pull the query point
    // into the spline's local space, then let FInterpCurve solve for the nearest key.
    auto DoFindInputKeyAtWorldLocation(
        const ck::FFragment_Spline_Params& InData,
        const FTransform& InEntityTransform,
        const FVector& InWorldLocation)
        -> float
    {
        const auto LocalLocation = InEntityTransform.InverseTransformPosition(InWorldLocation);
        auto DistanceSquared = 0.0f;
        return InData.Get_Curves().Position.FindNearest(LocalLocation, DistanceSquared);
    }

    auto DoConvertPointType(
        EInterpCurveMode InMode)
        -> ESplinePointType::Type
    {
        switch (InMode)
        {
            case CIM_Linear:           return ESplinePointType::Linear;
            case CIM_Constant:         return ESplinePointType::Constant;
            case CIM_CurveAuto:        return ESplinePointType::Curve;
            case CIM_CurveAutoClamped: return ESplinePointType::CurveClamped;
            case CIM_CurveUser:
            case CIM_CurveBreak:
            default:                   return ESplinePointType::CurveCustomTangent;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spline_UE::
    Add(
        FCk_Handle_Transform& InEntity,
        const FCk_Fragment_Spline_ParamsData& InParams)
    -> FCk_Handle_Spline
{
    CK_ENSURE_IF_NOT(InParams.Get_Curves().GetSplineLength() > 0.0f,
        TEXT("Cannot Add a Spline feature: the supplied params describe a spline with zero length."))
    { return {}; }

    InEntity.AddOrGet<ck::FFragment_Spline_Params>() = InParams;

    return Cast(InEntity);
}

auto
    UCk_Utils_Spline_UE::
    Add_FromSplineComponent(
        FCk_Handle_Transform& InEntity,
        USplineComponent* InSourceSpline)
    -> FCk_Handle_Spline
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSourceSpline, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Cannot Add a Spline feature with an invalid USplineComponent."))
    { return {}; }

    return Add(InEntity, ck_spline_detail::DoMakeParamsFromComponent(InSourceSpline));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spline_UE::
    Create(
        const FCk_Handle_Transform& InOwner,
        const FCk_Fragment_Spline_ParamsData& InParams)
    -> FCk_Handle_Spline
{
    auto SplineEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);
    UCk_Utils_Handle_UE::Set_DebugName(SplineEntity, TEXT("SPLINE"));

    auto TransformEntity = UCk_Utils_Transform_UE::Add(
        SplineEntity, UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InOwner), ECk_Replication::DoesNotReplicate);

    return Add(TransformEntity, InParams);
}

auto
    UCk_Utils_Spline_UE::
    Create_FromSplineComponent(
        const FCk_Handle_Transform& InOwner,
        USplineComponent* InSourceSpline)
    -> FCk_Handle_Spline
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSourceSpline, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Cannot Create a Spline with an invalid USplineComponent."))
    { return {}; }

    return Create(
        InOwner,
        ck_spline_detail::DoMakeParamsFromComponent(InSourceSpline));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spline_UE::
    Make_Params_FromPoints(
        const TArray<FVector>& InLocalPoints,
        bool InClosedLoop)
    -> FCk_Fragment_Spline_ParamsData
{
    CK_ENSURE_IF_NOT(InLocalPoints.Num() > 1,
        TEXT("Cannot make Spline params from [{}] point(s) - at least 2 are required."), InLocalPoints.Num())
    { return {}; }

    auto* TempSpline = NewObject<USplineComponent>(GetTransientPackage());

    constexpr auto DontUpdateYet = false;
    TempSpline->SetSplinePoints(InLocalPoints, ESplineCoordinateSpace::Local, DontUpdateYet);
    TempSpline->SetClosedLoop(InClosedLoop, DontUpdateYet);
    TempSpline->UpdateSpline();

    return ck_spline_detail::DoMakeParamsFromComponent(TempSpline);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spline_UE::
    Get_Length(
        const FCk_Handle_Spline& InSpline)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return 0.0f; }

    return InSpline.Get<ck::FFragment_Spline_Params>().Get_Curves().GetSplineLength();
}

auto
    UCk_Utils_Spline_UE::
    Get_NumberOfPoints(
        const FCk_Handle_Spline& InSpline)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return 0; }

    return InSpline.Get<ck::FFragment_Spline_Params>().Get_Curves().Position.Points.Num();
}

auto
    UCk_Utils_Spline_UE::
    Get_IsClosedLoop(
        const FCk_Handle_Spline& InSpline)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return false; }

    return InSpline.Get<ck::FFragment_Spline_Params>().Get_ClosedLoop();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spline_UE::
    Get_LocationAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::ZeroVector; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    const auto Key = ck_spline_detail::DoInputKeyAtDistance(Data, InDistance);
    return ck_spline_detail::DoWorldLocationAtKey(Data, ck_spline_detail::DoEntityTransform(InSpline), Key);
}

auto
    UCk_Utils_Spline_UE::
    Get_DirectionAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance)
    -> FVector
{
    return Get_TangentAtDistance(InSpline, InDistance).GetSafeNormal();
}

auto
    UCk_Utils_Spline_UE::
    Get_TangentAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::ZeroVector; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    const auto Key = ck_spline_detail::DoInputKeyAtDistance(Data, InDistance);
    return ck_spline_detail::DoWorldTangentAtKey(Data, ck_spline_detail::DoEntityTransform(InSpline), Key);
}

auto
    UCk_Utils_Spline_UE::
    Get_RotationAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance)
    -> FRotator
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FRotator::ZeroRotator; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    const auto Key = ck_spline_detail::DoInputKeyAtDistance(Data, InDistance);
    return ck_spline_detail::DoWorldQuatAtKey(Data, ck_spline_detail::DoEntityTransform(InSpline), Key).Rotator();
}

auto
    UCk_Utils_Spline_UE::
    Get_UpVectorAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::UpVector; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    const auto Key = ck_spline_detail::DoInputKeyAtDistance(Data, InDistance);
    return ck_spline_detail::DoWorldQuatAtKey(Data, ck_spline_detail::DoEntityTransform(InSpline), Key).RotateVector(FVector::UpVector);
}

auto
    UCk_Utils_Spline_UE::
    Get_RightVectorAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::RightVector; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    const auto Key = ck_spline_detail::DoInputKeyAtDistance(Data, InDistance);
    return ck_spline_detail::DoWorldQuatAtKey(Data, ck_spline_detail::DoEntityTransform(InSpline), Key).RotateVector(FVector::RightVector);
}

auto
    UCk_Utils_Spline_UE::
    Get_ScaleAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::OneVector; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    const auto Key = ck_spline_detail::DoInputKeyAtDistance(Data, InDistance);
    return ck_spline_detail::DoWorldScaleAtKey(Data, ck_spline_detail::DoEntityTransform(InSpline), Key);
}

auto
    UCk_Utils_Spline_UE::
    Get_TransformAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance)
    -> FTransform
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FTransform::Identity; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    const auto Key = ck_spline_detail::DoInputKeyAtDistance(Data, InDistance);
    const auto EntityTransform = ck_spline_detail::DoEntityTransform(InSpline);

    return FTransform
    {
        ck_spline_detail::DoWorldQuatAtKey(Data, EntityTransform, Key),
        ck_spline_detail::DoWorldLocationAtKey(Data, EntityTransform, Key),
        ck_spline_detail::DoWorldScaleAtKey(Data, EntityTransform, Key)
    };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spline_UE::
    Get_LocationAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::ZeroVector; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    return ck_spline_detail::DoWorldLocationAtKey(Data, ck_spline_detail::DoEntityTransform(InSpline), InInputKey);
}

auto
    UCk_Utils_Spline_UE::
    Get_DirectionAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::ZeroVector; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    return ck_spline_detail::DoWorldTangentAtKey(Data, ck_spline_detail::DoEntityTransform(InSpline), InInputKey).GetSafeNormal();
}

auto
    UCk_Utils_Spline_UE::
    Get_RotationAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey)
    -> FRotator
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FRotator::ZeroRotator; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    return ck_spline_detail::DoWorldQuatAtKey(Data, ck_spline_detail::DoEntityTransform(InSpline), InInputKey).Rotator();
}

auto
    UCk_Utils_Spline_UE::
    Get_TransformAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey)
    -> FTransform
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FTransform::Identity; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    const auto EntityTransform = ck_spline_detail::DoEntityTransform(InSpline);

    return FTransform
    {
        ck_spline_detail::DoWorldQuatAtKey(Data, EntityTransform, InInputKey),
        ck_spline_detail::DoWorldLocationAtKey(Data, EntityTransform, InInputKey),
        ck_spline_detail::DoWorldScaleAtKey(Data, EntityTransform, InInputKey)
    };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spline_UE::
    Get_InputKeyAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return 0.0f; }

    const auto& Points = InSpline.Get<ck::FFragment_Spline_Params>().Get_Curves().Position.Points;

    CK_ENSURE_IF_NOT(Points.IsValidIndex(InPointIndex),
        TEXT("Spline point index [{}] is out of range [0, {})"), InPointIndex, Points.Num())
    { return 0.0f; }

    return Points[InPointIndex].InVal;
}

auto
    UCk_Utils_Spline_UE::
    Get_LocationAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> FVector
{
    return Get_LocationAtInputKey(InSpline, Get_InputKeyAtPoint(InSpline, InPointIndex));
}

auto
    UCk_Utils_Spline_UE::
    Get_DirectionAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> FVector
{
    return Get_DirectionAtInputKey(InSpline, Get_InputKeyAtPoint(InSpline, InPointIndex));
}

auto
    UCk_Utils_Spline_UE::
    Get_RotationAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> FRotator
{
    return Get_RotationAtInputKey(InSpline, Get_InputKeyAtPoint(InSpline, InPointIndex));
}

auto
    UCk_Utils_Spline_UE::
    Get_TransformAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> FTransform
{
    return Get_TransformAtInputKey(InSpline, Get_InputKeyAtPoint(InSpline, InPointIndex));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spline_UE::
    Get_InputKeyAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return 0.0f; }

    return ck_spline_detail::DoInputKeyAtDistance(InSpline.Get<ck::FFragment_Spline_Params>(), InDistance);
}

auto
    UCk_Utils_Spline_UE::
    Get_DistanceAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return 0.0f; }

    // The ReparamTable maps distance -> input key; invert it by searching its points.
    const auto& ReparamPoints = InSpline.Get<ck::FFragment_Spline_Params>().Get_Curves().ReparamTable.Points;

    if (ReparamPoints.Num() == 0)
    { return 0.0f; }

    if (InInputKey <= ReparamPoints[0].OutVal)
    { return ReparamPoints[0].InVal; }

    for (auto Index = 1; Index < ReparamPoints.Num(); ++Index)
    {
        const auto& Prev = ReparamPoints[Index - 1];
        const auto& Curr = ReparamPoints[Index];

        if (InInputKey <= Curr.OutVal)
        {
            const auto KeySpan = Curr.OutVal - Prev.OutVal;
            const auto Alpha = KeySpan > KINDA_SMALL_NUMBER ? (InInputKey - Prev.OutVal) / KeySpan : 0.0f;
            return FMath::Lerp(Prev.InVal, Curr.InVal, Alpha);
        }
    }

    return ReparamPoints.Last().InVal;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spline_UE::
    Get_NumberOfSegments(
        const FCk_Handle_Spline& InSpline)
    -> int32
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return 0; }

    const auto& Curves = InSpline.Get<ck::FFragment_Spline_Params>().Get_Curves();
    const auto NumPoints = Curves.Position.Points.Num();
    return Curves.Position.bIsLooped ? NumPoints : FMath::Max(0, NumPoints - 1);
}

auto
    UCk_Utils_Spline_UE::
    Get_DefaultUpVector(
        const FCk_Handle_Spline& InSpline)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::UpVector; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    return ck_spline_detail::DoEntityTransform(InSpline).GetRotation().RotateVector(Data.Get_DefaultUpVector());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spline_UE::
    Get_RollAtDistance(
        const FCk_Handle_Spline& InSpline,
        float InDistance)
    -> float
{
    return static_cast<float>(Get_RotationAtDistance(InSpline, InDistance).Roll);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spline_UE::
    Get_TangentAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::ZeroVector; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    return ck_spline_detail::DoWorldTangentAtKey(Data, ck_spline_detail::DoEntityTransform(InSpline), InInputKey);
}

auto
    UCk_Utils_Spline_UE::
    Get_UpVectorAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::UpVector; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    return ck_spline_detail::DoWorldQuatAtKey(Data, ck_spline_detail::DoEntityTransform(InSpline), InInputKey).RotateVector(FVector::UpVector);
}

auto
    UCk_Utils_Spline_UE::
    Get_RightVectorAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::RightVector; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    return ck_spline_detail::DoWorldQuatAtKey(Data, ck_spline_detail::DoEntityTransform(InSpline), InInputKey).RotateVector(FVector::RightVector);
}

auto
    UCk_Utils_Spline_UE::
    Get_ScaleAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::OneVector; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    return ck_spline_detail::DoWorldScaleAtKey(Data, ck_spline_detail::DoEntityTransform(InSpline), InInputKey);
}

auto
    UCk_Utils_Spline_UE::
    Get_RollAtInputKey(
        const FCk_Handle_Spline& InSpline,
        float InInputKey)
    -> float
{
    return static_cast<float>(Get_RotationAtInputKey(InSpline, InInputKey).Roll);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spline_UE::
    Get_TangentAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> FVector
{
    return Get_TangentAtInputKey(InSpline, Get_InputKeyAtPoint(InSpline, InPointIndex));
}

auto
    UCk_Utils_Spline_UE::
    Get_ArriveTangentAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::ZeroVector; }

    const auto& Points = InSpline.Get<ck::FFragment_Spline_Params>().Get_Curves().Position.Points;

    CK_ENSURE_IF_NOT(Points.IsValidIndex(InPointIndex),
        TEXT("Spline point index [{}] is out of range [0, {})"), InPointIndex, Points.Num())
    { return FVector::ZeroVector; }

    return ck_spline_detail::DoEntityTransform(InSpline).TransformVector(Points[InPointIndex].ArriveTangent);
}

auto
    UCk_Utils_Spline_UE::
    Get_LeaveTangentAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return FVector::ZeroVector; }

    const auto& Points = InSpline.Get<ck::FFragment_Spline_Params>().Get_Curves().Position.Points;

    CK_ENSURE_IF_NOT(Points.IsValidIndex(InPointIndex),
        TEXT("Spline point index [{}] is out of range [0, {})"), InPointIndex, Points.Num())
    { return FVector::ZeroVector; }

    return ck_spline_detail::DoEntityTransform(InSpline).TransformVector(Points[InPointIndex].LeaveTangent);
}

auto
    UCk_Utils_Spline_UE::
    Get_UpVectorAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> FVector
{
    return Get_UpVectorAtInputKey(InSpline, Get_InputKeyAtPoint(InSpline, InPointIndex));
}

auto
    UCk_Utils_Spline_UE::
    Get_RightVectorAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> FVector
{
    return Get_RightVectorAtInputKey(InSpline, Get_InputKeyAtPoint(InSpline, InPointIndex));
}

auto
    UCk_Utils_Spline_UE::
    Get_ScaleAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> FVector
{
    return Get_ScaleAtInputKey(InSpline, Get_InputKeyAtPoint(InSpline, InPointIndex));
}

auto
    UCk_Utils_Spline_UE::
    Get_RollAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> float
{
    return Get_RollAtInputKey(InSpline, Get_InputKeyAtPoint(InSpline, InPointIndex));
}

auto
    UCk_Utils_Spline_UE::
    Get_DistanceAtPoint(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> float
{
    return Get_DistanceAtInputKey(InSpline, Get_InputKeyAtPoint(InSpline, InPointIndex));
}

auto
    UCk_Utils_Spline_UE::
    Get_PointType(
        const FCk_Handle_Spline& InSpline,
        int32 InPointIndex)
    -> TEnumAsByte<ESplinePointType::Type>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return ESplinePointType::Constant; }

    const auto& Points = InSpline.Get<ck::FFragment_Spline_Params>().Get_Curves().Position.Points;

    CK_ENSURE_IF_NOT(Points.IsValidIndex(InPointIndex),
        TEXT("Spline point index [{}] is out of range [0, {})"), InPointIndex, Points.Num())
    { return ESplinePointType::Constant; }

    return ck_spline_detail::DoConvertPointType(Points[InPointIndex].InterpMode.GetValue());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Spline_UE::
    Find_InputKeyClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation)
    -> float
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSpline), TEXT("Invalid Spline handle [{}]"), InSpline)
    { return 0.0f; }

    const auto& Data = InSpline.Get<ck::FFragment_Spline_Params>();
    return ck_spline_detail::DoFindInputKeyAtWorldLocation(Data, ck_spline_detail::DoEntityTransform(InSpline), InWorldLocation);
}

auto
    UCk_Utils_Spline_UE::
    Find_LocationClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation)
    -> FVector
{
    return Get_LocationAtInputKey(InSpline, Find_InputKeyClosestToWorldLocation(InSpline, InWorldLocation));
}

auto
    UCk_Utils_Spline_UE::
    Find_DirectionClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation)
    -> FVector
{
    return Get_DirectionAtInputKey(InSpline, Find_InputKeyClosestToWorldLocation(InSpline, InWorldLocation));
}

auto
    UCk_Utils_Spline_UE::
    Find_TangentClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation)
    -> FVector
{
    return Get_TangentAtInputKey(InSpline, Find_InputKeyClosestToWorldLocation(InSpline, InWorldLocation));
}

auto
    UCk_Utils_Spline_UE::
    Find_RotationClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation)
    -> FRotator
{
    return Get_RotationAtInputKey(InSpline, Find_InputKeyClosestToWorldLocation(InSpline, InWorldLocation));
}

auto
    UCk_Utils_Spline_UE::
    Find_UpVectorClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation)
    -> FVector
{
    return Get_UpVectorAtInputKey(InSpline, Find_InputKeyClosestToWorldLocation(InSpline, InWorldLocation));
}

auto
    UCk_Utils_Spline_UE::
    Find_RightVectorClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation)
    -> FVector
{
    return Get_RightVectorAtInputKey(InSpline, Find_InputKeyClosestToWorldLocation(InSpline, InWorldLocation));
}

auto
    UCk_Utils_Spline_UE::
    Find_ScaleClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation)
    -> FVector
{
    return Get_ScaleAtInputKey(InSpline, Find_InputKeyClosestToWorldLocation(InSpline, InWorldLocation));
}

auto
    UCk_Utils_Spline_UE::
    Find_TransformClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation)
    -> FTransform
{
    return Get_TransformAtInputKey(InSpline, Find_InputKeyClosestToWorldLocation(InSpline, InWorldLocation));
}

auto
    UCk_Utils_Spline_UE::
    Get_DistanceClosestToWorldLocation(
        const FCk_Handle_Spline& InSpline,
        FVector InWorldLocation)
    -> float
{
    return Get_DistanceAtInputKey(InSpline, Find_InputKeyClosestToWorldLocation(InSpline, InWorldLocation));
}

// --------------------------------------------------------------------------------------------------------------------
