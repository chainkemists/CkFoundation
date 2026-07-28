#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkPathNetwork/Network/CkPathNetwork_Types.h"

#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkPathNetwork_EditorUtils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ACk_PathNetwork_UE;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPATHNETWORKEDITOR_API FCk_PathNetworkEditor_DetectorBakeResult
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetworkEditor_DetectorBakeResult);

private:
    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    bool _Succeeded = false;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FString _FailureReason;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _AuthoredRibbonCount = 0;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _GeneratedRibbonCount = 0;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _TotalRibbonCount = 0;

public:
    CK_PROPERTY_GET(_Succeeded);
    CK_PROPERTY_GET(_FailureReason);
    CK_PROPERTY_GET(_AuthoredRibbonCount);
    CK_PROPERTY_GET(_GeneratedRibbonCount);
    CK_PROPERTY_GET(_TotalRibbonCount);

private:
    friend class UCk_Utils_PathNetworkEditor_UE;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPATHNETWORKEDITOR_API FCk_PathNetworkEditor_NavmeshConformanceFailure
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetworkEditor_NavmeshConformanceFailure);

private:
    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FVector _SourcePoint = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FVector _ProjectedPoint = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    float _PlanarDelta = 0.0f;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    float _VerticalDelta = 0.0f;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    bool _Projected = false;

public:
    CK_PROPERTY_GET(_SourcePoint);
    CK_PROPERTY_GET(_ProjectedPoint);
    CK_PROPERTY_GET(_PlanarDelta);
    CK_PROPERTY_GET(_VerticalDelta);
    CK_PROPERTY_GET(_Projected);

private:
    friend class UCk_Utils_PathNetworkEditor_UE;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPATHNETWORKEDITOR_API FCk_PathNetworkEditor_NavmeshProjectabilityResult
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetworkEditor_NavmeshProjectabilityResult);

private:
    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    bool _Succeeded = false;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FString _FailureReason;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _TotalPointCount = 0;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FVector _ProjectionExtent = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    TArray<FVector> _UnprojectablePoints;

    // Generated points only. A successful broad projection still fails here when
    // it would move the detected sidewalk onto another surface.
    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    TArray<FCk_PathNetworkEditor_NavmeshConformanceFailure> _NonconformantPoints;

public:
    CK_PROPERTY_GET(_Succeeded);
    CK_PROPERTY_GET(_FailureReason);
    CK_PROPERTY_GET(_TotalPointCount);
    CK_PROPERTY_GET(_ProjectionExtent);
    CK_PROPERTY_GET(_UnprojectablePoints);
    CK_PROPERTY_GET(_NonconformantPoints);

private:
    friend class UCk_Utils_PathNetworkEditor_UE;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPATHNETWORKEDITOR_API FCk_PathNetworkEditor_NavmeshTrimResult
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_PathNetworkEditor_NavmeshTrimResult);

private:
    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    bool _Succeeded = false;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    FString _FailureReason;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _TrimmedPointCount = 0;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _RemovedRibbonCount = 0;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    int32 _RemainingRibbonCount = 0;

    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    TArray<FVector> _InternalUnprojectablePoints;

    // Includes every generated point rejected during endpoint trimming, with
    // broad-projection and delta detail for bake diagnostics.
    UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
    TArray<FCk_PathNetworkEditor_NavmeshConformanceFailure> _NonconformantPoints;

public:
    CK_PROPERTY_GET(_Succeeded);
    CK_PROPERTY_GET(_FailureReason);
    CK_PROPERTY_GET(_TrimmedPointCount);
    CK_PROPERTY_GET(_RemovedRibbonCount);
    CK_PROPERTY_GET(_RemainingRibbonCount);
    CK_PROPERTY_GET(_InternalUnprojectablePoints);
    CK_PROPERTY_GET(_NonconformantPoints);

private:
    friend class UCk_Utils_PathNetworkEditor_UE;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKPATHNETWORKEDITOR_API UCk_Utils_PathNetworkEditor_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_PathNetworkEditor_UE);

public:
    UFUNCTION(BlueprintPure,
              Category="Ck|Utils|PathNetworkEditor",
              DisplayName="[Ck][PathNetworkEditor] Get Ribbon Id String")
    static FString
    Get_RibbonIdString(
        const FCk_PathNetwork_Ribbon& InRibbon);

    UFUNCTION(BlueprintCallable,
              Category="Ck|Utils|PathNetworkEditor",
              DisplayName="[Ck][PathNetworkEditor] Bake Detector To Actor")
    static FCk_PathNetworkEditor_DetectorBakeResult
    Bake_DetectorToActor(
        ACk_PathNetwork_UE* InActor);

    UFUNCTION(BlueprintPure,
              Category="Ck|Utils|PathNetworkEditor",
              DisplayName="[Ck][PathNetworkEditor] Validate Ribbon Point Projectability")
    static FCk_PathNetworkEditor_NavmeshProjectabilityResult
    Validate_RibbonPointProjectability(
        const ACk_PathNetwork_UE* InActor,
        FVector InProjectionExtent,
        float InMaxPlanarProjectionDelta,
        float InMaxVerticalProjectionDelta);

    UFUNCTION(BlueprintCallable,
              Category="Ck|Utils|PathNetworkEditor",
              DisplayName="[Ck][PathNetworkEditor] Trim Unprojectable Generated Ribbon Endpoints")
    static FCk_PathNetworkEditor_NavmeshTrimResult
    Trim_UnprojectableGeneratedRibbonEndpoints(
        ACk_PathNetwork_UE* InActor,
        FVector InProjectionExtent,
        float InMaxPlanarProjectionDelta,
        float InMaxVerticalProjectionDelta);
};

// --------------------------------------------------------------------------------------------------------------------
