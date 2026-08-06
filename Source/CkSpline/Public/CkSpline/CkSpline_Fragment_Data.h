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

// Curves are stored in the owning transform entity's LOCAL space; query utilities compose its world transform.
USTRUCT(BlueprintType)
struct CKSPLINE_API FCk_Spline_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Spline_Spec);

public:
    FCk_Spline_Spec() = default;
    FCk_Spline_Spec(FSplineCurves InCurves, bool InClosedLoop)
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
    // FSplineCurves is not AngelScript-bindable, so its getter and ctor stay plain C++ (no CK_PROPERTY)
    const FSplineCurves& Get_Curves() const { return _Curves; }
    CK_PROPERTY_GET(_ClosedLoop);
    CK_PROPERTY(_DefaultUpVector);
};

// --------------------------------------------------------------------------------------------------------------------
