#include "CkPathNetwork_EditorUtils.h"

#include "CkPathNetworkEditor/Authoring/CkPathNetwork_AuthoringService.h"

#include "CkPathNetwork/Actor/CkPathNetwork_Actor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkNavigation/NavSurface/CkNavSurface_Utils.h"

#include <ScopedTransaction.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pathnetwork_editor
{
    struct FNavmeshConformance
    {
        bool _Projected = false;
        FVector _ProjectedPoint = FVector::ZeroVector;
        float _PlanarDelta = 0.0f;
        float _VerticalDelta = 0.0f;
    };

    auto
    Evaluate_NavmeshConformance(
        const UObject* InWorldContext,
        const FVector& InSourcePoint,
        const FVector& InProjectionExtent)
        -> FNavmeshConformance
    {
        const auto Query = FCk_NavSurface_ProjectionQuery{InSourcePoint}
            .Set_SearchHalfExtents(InProjectionExtent);

        const auto Projected = UCk_Utils_NavSurface_UE::Try_ProjectPoint(InWorldContext, Query);

        auto Result = FNavmeshConformance{};
        Result._Projected = Projected.Get_Status() == ECk_NavSurface_QueryStatus::Success;
        if (NOT Result._Projected)
        { return Result; }

        Result._ProjectedPoint = Projected.Get_Location();
        Result._PlanarDelta = FVector::Dist2D(InSourcePoint, Projected.Get_Location());
        Result._VerticalDelta = FMath::Abs(InSourcePoint.Z - Projected.Get_Location().Z);
        return Result;
    }

    auto
    Is_Conformant(
        const FNavmeshConformance& InConformance,
        const float InMaxPlanarProjectionDelta,
        const float InMaxVerticalProjectionDelta)
        -> bool
    {
        return InConformance._Projected
            && InConformance._PlanarDelta <= InMaxPlanarProjectionDelta
            && InConformance._VerticalDelta <= InMaxVerticalProjectionDelta;
    }

}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkEditor_UE::
    Get_RibbonIdString(
        const FCk_PathNetwork_Ribbon& InRibbon)
    -> FString
{
    return InRibbon.Get_RibbonId().ToString(EGuidFormats::Digits);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkEditor_UE::
    Bake_DetectorToActor(
        ACk_PathNetwork_UE* InActor)
    -> FCk_PathNetworkEditor_DetectorBakeResult
{
    auto Result = FCk_PathNetworkEditor_DetectorBakeResult{};

    const bool ActorIsValid = ck::IsValid(InActor);
    CK_ENSURE_IF_NOT(ActorIsValid,
        TEXT("Bake_DetectorToActor requires a valid PathNetwork actor"))
    {
        Result._FailureReason = TEXT("PathNetwork actor is invalid");
        return Result;
    }

    const auto& Detector = InActor->Get_Detector();
    const bool DetectorIsValid = ck::IsValid(Detector);
    CK_ENSURE_IF_NOT(DetectorIsValid,
        TEXT("Bake_DetectorToActor on [{}] requires an assigned valid detector"), InActor)
    {
        Result._FailureReason = TEXT("No valid detector is assigned");
        return Result;
    }

    const auto Preview = ck::pathnetwork_editor::authoring::Preview(
        ck::pathnetwork_editor::authoring::FPreviewRequest{
            ._World = InActor->GetWorld(),
            ._DetectorTemplate = Detector,
            ._SourceActor = InActor,
            ._DetectionBounds = InActor->Get_DetectionBounds(),
            ._VectorizeParams = InActor->Get_VectorizeParams()});
    if (NOT Preview._Succeeded)
    {
        Result._FailureReason = Preview._FailureReason;
        return Result;
    }

    const auto Applied =
        ck::pathnetwork_editor::authoring::ApplyPreview_ToExistingActor(InActor, Preview);
    Result._Succeeded = Applied._Succeeded;
    Result._FailureReason = Applied._FailureReason;
    Result._AuthoredRibbonCount = Applied._AuthoredRibbonCount;
    Result._GeneratedRibbonCount = Applied._GeneratedRibbonCount;
    Result._TotalRibbonCount = Applied._TotalRibbonCount;
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkEditor_UE::
    Trim_UnprojectableGeneratedRibbonEndpoints(
        ACk_PathNetwork_UE* InActor,
        FVector InProjectionExtent,
        float InMaxPlanarProjectionDelta,
        float InMaxVerticalProjectionDelta)
    -> FCk_PathNetworkEditor_NavmeshTrimResult
{
    auto Result = FCk_PathNetworkEditor_NavmeshTrimResult{};

    const bool ProjectionExtentIsValid =
        NOT InProjectionExtent.ContainsNaN()
        && InProjectionExtent.X > 0.0
        && InProjectionExtent.Y > 0.0
        && InProjectionExtent.Z > 0.0;
    CK_ENSURE_IF_NOT(ProjectionExtentIsValid,
        TEXT("Trim_UnprojectableGeneratedRibbonEndpoints requires a finite positive projection extent, received [{}]"),
        InProjectionExtent)
    {
        Result._FailureReason = TEXT("Projection extent must be finite and positive");
        return Result;
    }

    const bool TolerancesAreValid = FMath::IsFinite(InMaxPlanarProjectionDelta)
        && FMath::IsFinite(InMaxVerticalProjectionDelta)
        && InMaxPlanarProjectionDelta >= 0.0f
        && InMaxVerticalProjectionDelta >= 0.0f;
    CK_ENSURE_IF_NOT(TolerancesAreValid,
        TEXT("Trim_UnprojectableGeneratedRibbonEndpoints requires finite non-negative planar/vertical tolerances, received [{}, {}]"),
        InMaxPlanarProjectionDelta, InMaxVerticalProjectionDelta)
    {
        Result._FailureReason = TEXT("Planar and vertical conformance tolerances must be finite and non-negative");
        return Result;
    }

    const bool ActorIsValid = ck::IsValid(InActor);
    CK_ENSURE_IF_NOT(ActorIsValid,
        TEXT("Trim_UnprojectableGeneratedRibbonEndpoints requires a valid PathNetwork actor"))
    {
        Result._FailureReason = TEXT("PathNetwork actor is invalid");
        return Result;
    }

    auto* World = InActor->GetWorld();
    const bool WorldIsValid = ck::IsValid(World);
    CK_ENSURE_IF_NOT(WorldIsValid,
        TEXT("Trim_UnprojectableGeneratedRibbonEndpoints on [{}] requires a valid world"), InActor)
    {
        Result._FailureReason = TEXT("PathNetwork actor has no valid world");
        return Result;
    }

    const auto ProviderHealth = UCk_Utils_NavSurface_UE::Get_ProviderHealth(World);
    const bool NavSurfaceIsAvailable = ProviderHealth != ECk_NavSurface_ProviderHealth::NoData
        && ProviderHealth != ECk_NavSurface_ProviderHealth::Error;
    CK_ENSURE_IF_NOT(NavSurfaceIsAvailable,
        TEXT("Trim_UnprojectableGeneratedRibbonEndpoints on [{}] found no navigation surface, health [{}]"),
        InActor, ProviderHealth)
    {
        Result._FailureReason = TEXT("World has no navigation surface");
        return Result;
    }

    const auto EvaluateGeneratedPoint = [&](const FCk_PathNetwork_RibbonPoint& InPoint)
    {
        return ck_pathnetwork_editor::Evaluate_NavmeshConformance(
            World, InPoint.Get_Location(), InProjectionExtent);
    };
    const auto RecordNonconformantPoint = [&](const FCk_PathNetwork_RibbonPoint& InPoint,
                                               const ck_pathnetwork_editor::FNavmeshConformance& InConformance)
    {
        auto Failure = FCk_PathNetworkEditor_NavmeshConformanceFailure{};
        Failure._SourcePoint = InPoint.Get_Location();
        Failure._ProjectedPoint = InConformance._ProjectedPoint;
        Failure._PlanarDelta = InConformance._PlanarDelta;
        Failure._VerticalDelta = InConformance._VerticalDelta;
        Failure._Projected = InConformance._Projected;
        Result._NonconformantPoints.Add(MoveTemp(Failure));
    };

    const auto& RelativeRibbons = InActor->Get_Ribbons();
    const auto WorldRibbons = InActor->Get_WorldRibbons();
    const bool RibbonCountsMatch = RelativeRibbons.Num() == WorldRibbons.Num();
    CK_ENSURE_IF_NOT(RibbonCountsMatch,
        TEXT("Trim_UnprojectableGeneratedRibbonEndpoints on [{}] requires matching relative/world ribbon counts; "
             "relative [{}], world [{}]"),
        InActor, RelativeRibbons.Num(), WorldRibbons.Num())
    {
        Result._FailureReason = TEXT("Relative and world ribbon counts do not match");
        return Result;
    }

    auto NewRibbons = TArray<FCk_PathNetwork_Ribbon>{};
    for (auto RibbonIndex = 0; RibbonIndex < WorldRibbons.Num(); ++RibbonIndex)
    {
        const auto& Ribbon = WorldRibbons[RibbonIndex];
        if (Ribbon.Get_Source() == ECk_PathNetwork_RibbonSource::Authored)
        {
            NewRibbons.Add(RelativeRibbons[RibbonIndex]);
            continue;
        }

        auto Points = Ribbon.Get_Points();
        while (NOT Points.IsEmpty())
        {
            const auto Conformance = EvaluateGeneratedPoint(Points[0]);
            if (ck_pathnetwork_editor::Is_Conformant(
                    Conformance, InMaxPlanarProjectionDelta, InMaxVerticalProjectionDelta))
            { break; }
            RecordNonconformantPoint(Points[0], Conformance);
            Points.RemoveAt(0, 1, EAllowShrinking::No);
            ++Result._TrimmedPointCount;
        }
        while (NOT Points.IsEmpty())
        {
            const auto Conformance = EvaluateGeneratedPoint(Points.Last());
            if (ck_pathnetwork_editor::Is_Conformant(
                    Conformance, InMaxPlanarProjectionDelta, InMaxVerticalProjectionDelta))
            { break; }
            RecordNonconformantPoint(Points.Last(), Conformance);
            Points.RemoveAt(Points.Num() - 1, 1, EAllowShrinking::No);
            ++Result._TrimmedPointCount;
        }

        if (Points.Num() < 2)
        {
            ++Result._RemovedRibbonCount;
            continue;
        }

        for (const auto& Point : Points)
        {
            const auto Conformance = EvaluateGeneratedPoint(Point);
            if (NOT ck_pathnetwork_editor::Is_Conformant(
                    Conformance, InMaxPlanarProjectionDelta, InMaxVerticalProjectionDelta))
            {
                Result._InternalUnprojectablePoints.Add(Point.Get_Location());
                RecordNonconformantPoint(Point, Conformance);
            }
        }

        auto TrimmedWorldRibbon = Ribbon;
        TrimmedWorldRibbon.Set_Points(Points);
        NewRibbons.Add(InActor->Convert_WorldRibbonToRelative(TrimmedWorldRibbon));
    }

    const bool HasNoInternalUnprojectablePoints = Result._InternalUnprojectablePoints.IsEmpty();
    CK_ENSURE_IF_NOT(HasNoInternalUnprojectablePoints,
        TEXT("Trim_UnprojectableGeneratedRibbonEndpoints on [{}] found [{}] internal nonconformant point(s); "
             "no ribbons were mutated"),
        InActor, Result._InternalUnprojectablePoints.Num())
    {
        Result._FailureReason = TEXT("Generated ribbons contain internal nonconformant points");
        return Result;
    }

    const FScopedTransaction Transaction{
        NSLOCTEXT("CkPathNetworkEditor", "TrimGeneratedRibbonEndpointsTransaction",
                  "Path Network: Trim Unprojectable Generated Ribbon Endpoints")};
    InActor->Modify();
    InActor->Set_Ribbons(NewRibbons);
    InActor->PostEditChange();

    Result._RemainingRibbonCount = NewRibbons.Num();
    Result._Succeeded = true;
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PathNetworkEditor_UE::
    Validate_RibbonPointProjectability(
        const ACk_PathNetwork_UE* InActor,
        FVector InProjectionExtent,
        float InMaxPlanarProjectionDelta,
        float InMaxVerticalProjectionDelta)
    -> FCk_PathNetworkEditor_NavmeshProjectabilityResult
{
    auto Result = FCk_PathNetworkEditor_NavmeshProjectabilityResult{};
    Result._ProjectionExtent = InProjectionExtent;

    const bool ProjectionExtentIsValid =
        NOT InProjectionExtent.ContainsNaN()
        && InProjectionExtent.X > 0.0
        && InProjectionExtent.Y > 0.0
        && InProjectionExtent.Z > 0.0;
    CK_ENSURE_IF_NOT(ProjectionExtentIsValid,
        TEXT("Validate_RibbonPointProjectability requires a finite positive projection extent, received [{}]"),
        InProjectionExtent)
    {
        Result._FailureReason = TEXT("Projection extent must be finite and positive");
        return Result;
    }

    const bool TolerancesAreValid = FMath::IsFinite(InMaxPlanarProjectionDelta)
        && FMath::IsFinite(InMaxVerticalProjectionDelta)
        && InMaxPlanarProjectionDelta >= 0.0f
        && InMaxVerticalProjectionDelta >= 0.0f;
    CK_ENSURE_IF_NOT(TolerancesAreValid,
        TEXT("Validate_RibbonPointProjectability requires finite non-negative planar/vertical tolerances, received [{}, {}]"),
        InMaxPlanarProjectionDelta, InMaxVerticalProjectionDelta)
    {
        Result._FailureReason = TEXT("Planar and vertical conformance tolerances must be finite and non-negative");
        return Result;
    }

    const bool ActorIsValid = ck::IsValid(InActor);
    CK_ENSURE_IF_NOT(ActorIsValid,
        TEXT("Validate_RibbonPointProjectability requires a valid PathNetwork actor"))
    {
        Result._FailureReason = TEXT("PathNetwork actor is invalid");
        return Result;
    }

    auto* World = InActor->GetWorld();
    const bool WorldIsValid = ck::IsValid(World);
    CK_ENSURE_IF_NOT(WorldIsValid,
        TEXT("Validate_RibbonPointProjectability on [{}] requires a valid world"), InActor)
    {
        Result._FailureReason = TEXT("PathNetwork actor has no valid world");
        return Result;
    }

    const auto ProviderHealth = UCk_Utils_NavSurface_UE::Get_ProviderHealth(World);
    const bool NavSurfaceIsAvailable = ProviderHealth != ECk_NavSurface_ProviderHealth::NoData
        && ProviderHealth != ECk_NavSurface_ProviderHealth::Error;
    CK_ENSURE_IF_NOT(NavSurfaceIsAvailable,
        TEXT("Validate_RibbonPointProjectability on [{}] found no navigation surface, health [{}]"),
        InActor, ProviderHealth)
    {
        Result._FailureReason = TEXT("World has no navigation surface");
        return Result;
    }

    for (const auto& WorldRibbon : InActor->Get_WorldRibbons())
    {
        if (WorldRibbon.Get_Source() == ECk_PathNetwork_RibbonSource::Authored)
        { continue; }
        for (const auto& Point : WorldRibbon.Get_Points())
        {
            ++Result._TotalPointCount;

            const auto Conformance = ck_pathnetwork_editor::Evaluate_NavmeshConformance(
                World, Point.Get_Location(), Result._ProjectionExtent);
            if (NOT ck_pathnetwork_editor::Is_Conformant(
                    Conformance, InMaxPlanarProjectionDelta, InMaxVerticalProjectionDelta))
            {
                Result._UnprojectablePoints.Add(Point.Get_Location());
                auto Failure = FCk_PathNetworkEditor_NavmeshConformanceFailure{};
                Failure._SourcePoint = Point.Get_Location();
                Failure._ProjectedPoint = Conformance._ProjectedPoint;
                Failure._PlanarDelta = Conformance._PlanarDelta;
                Failure._VerticalDelta = Conformance._VerticalDelta;
                Failure._Projected = Conformance._Projected;
                Result._NonconformantPoints.Add(MoveTemp(Failure));
            }
        }
    }

    Result._Succeeded = true;
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
