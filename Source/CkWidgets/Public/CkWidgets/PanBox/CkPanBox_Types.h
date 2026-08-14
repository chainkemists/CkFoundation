#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

#include "CkPanBox_Types.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PanBox_BoundsPolicy : uint8
{
    // The offset is unrestricted — the surface may be panned fully out of view.
    Unbounded,

    // Per-axis: the surface covers the viewport when larger than it, and stays fully visible when smaller.
    ClampViewToSurface
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PanBox_BoundsPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PanBox_SurfaceSizeMode : uint8
{
    ChildDesiredSize,
    Explicit
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PanBox_SurfaceSizeMode);

// --------------------------------------------------------------------------------------------------------------------

/**
 * View transform of a PanBox: ViewportPos = SurfacePos * Zoom + Offset.
 * Offset is the viewport-space position of the surface origin (top-left), in Slate units.
 */
USTRUCT(BlueprintType)
struct CKWIDGETS_API FCk_PanBox_View
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_PanBox_View);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PanBox",
              meta = (AllowPrivateAccess = true))
    FVector2D _Offset = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PanBox",
              meta = (AllowPrivateAccess = true, ClampMin = "0.0001"))
    float _Zoom = 1.0f;

public:
    CK_PROPERTY(_Offset);
    CK_PROPERTY(_Zoom);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_PanBox_View, _Offset, _Zoom);
};

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_PanBox_View, [](const FCk_PanBox_View& InObj)
{
    return ck::Format(TEXT("Offset: [{}, {}], Zoom: [{}]"),
        InObj.Get_Offset().X, InObj.Get_Offset().Y, InObj.Get_Zoom());
});

// --------------------------------------------------------------------------------------------------------------------
