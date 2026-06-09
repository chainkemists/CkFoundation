#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <Components/SplineComponent.h>

#include "CkSpline_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKSPLINE_API FCk_Handle_Spline : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Spline); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Spline);

// --------------------------------------------------------------------------------------------------------------------

// Spline path data. Holds an FSplineCurves blob (cubic position/rotation/scale curves plus
// the arc-length reparam table) and orientation metadata. Curves are stored in the owning
// transform entity's local space; query utilities compose the entity's world transform.
USTRUCT(BlueprintType)
struct CKSPLINE_API FCk_Fragment_Spline_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Spline_ParamsData);

public:
    FCk_Fragment_Spline_ParamsData() = default;
    FCk_Fragment_Spline_ParamsData(FSplineCurves InCurves, bool InClosedLoop)
        : _Curves(MoveTemp(InCurves)), _ClosedLoop(InClosedLoop) {}

private:
    UPROPERTY()
    FSplineCurves _Curves;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector _DefaultUpVector = FVector::UpVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    bool _ClosedLoop = false;

public:
    // _Curves is FSplineCurves, which is not an AngelScript-bindable type. Expose the getter and the
    // (FSplineCurves, bool) constructor as plain C++
    const FSplineCurves& Get_Curves() const { return _Curves; }
    CK_PROPERTY_GET(_ClosedLoop);
    CK_PROPERTY(_DefaultUpVector);
};

// --------------------------------------------------------------------------------------------------------------------
