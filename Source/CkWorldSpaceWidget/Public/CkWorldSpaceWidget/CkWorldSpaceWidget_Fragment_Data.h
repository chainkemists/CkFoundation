#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkUICore/CkUI_Utils.h"

#include "Components/CanvasPanel.h"
#include "Components/SizeBox.h"
#include "Components/WidgetComponent.h"

#include "Engine/EngineTypes.h"

#include "CkWorldSpaceWidget_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKWORLDSPACEWIDGET_API FCk_Handle_WorldSpaceWidget : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_WorldSpaceWidget); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_WorldSpaceWidget);

// --------------------------------------------------------------------------------------------------------------------

// None hides the widget (opacity 0) when its anchor projects off-screen or behind the camera.
// ClampToViewport keeps the widget's pivot inside the viewport rect so it rides the screen edge.
// ClampToViewport_ByBounds shrinks that rect by the desired size so the WHOLE widget stays on.
UENUM(BlueprintType)
enum class ECk_WorldSpaceWidget_Clamping_Policy : uint8
{
    None,
    ClampToViewport,
    ClampToViewport_ByBounds
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_WorldSpaceWidget_Clamping_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_WorldSpaceWidget_Scaling_Policy : uint8
{
    None,
    ScaleWithDistance
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_WorldSpaceWidget_Scaling_Policy);

// --------------------------------------------------------------------------------------------------------------------

// ScreenOverlay (default): projected world->screen each frame and drawn as a 2D viewport overlay
// (always screen-facing, constant pixel size; depth-occlusion only via the OcclusionInfo trace).
// WorldComponent: a real 3D UWidgetComponent — fixed orientation, perspective scale, GPU occlusion.
UENUM(BlueprintType)
enum class ECk_WorldSpaceWidget_RenderMode : uint8
{
    ScreenOverlay,
    WorldComponent
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_WorldSpaceWidget_RenderMode);

// --------------------------------------------------------------------------------------------------------------------

// Consumed only when RenderMode == WorldComponent.
USTRUCT(BlueprintType)
struct CKWORLDSPACEWIDGET_API FCk_WorldSpaceWidget_WorldComponentInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WorldSpaceWidget_WorldComponentInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FIntPoint _DrawSize = FIntPoint{512, 512};

    // When true the render target tracks the content widget's DESIRED size each frame and
    // _DrawSize is only the pre-first-layout initial value (UWidgetComponent bDrawAtDesiredSize).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    bool _DrawAtDesiredSize = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FVector2D _Pivot = FVector2D{0.5f, 0.5f};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    EWidgetBlendMode _BlendMode = EWidgetBlendMode::Masked;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    EWidgetGeometryMode _GeometryMode = EWidgetGeometryMode::Plane;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    bool _TwoSided = false;

    // Soft by design: a hard ref force-loads with the owning package and roots nothing anyway (GC
    // never walks the EnTT registry). Resolved resident-or-fail at the synchronous creation site —
    // no deferred setup to queue a load behind.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    TSoftObjectPtr<UMaterialInterface> _OverrideMaterial;

public:
    CK_PROPERTY_GET(_DrawSize);
    CK_PROPERTY(_DrawAtDesiredSize);
    CK_PROPERTY(_Pivot);
    CK_PROPERTY(_BlendMode);
    CK_PROPERTY(_GeometryMode);
    CK_PROPERTY(_TwoSided);
    CK_PROPERTY(_OverrideMaterial);

    CK_DEFINE_CONSTRUCTORS(FCk_WorldSpaceWidget_WorldComponentInfo, _DrawSize);
};

// --------------------------------------------------------------------------------------------------------------------

// HideWhenOccluded: each frame, trace from the player camera to the widget's world anchor
// (player pawn ignored); a blocking hit fades the overlay to zero opacity. Per-anchor
// (whole-widget) occlusion, not per-pixel GPU masking — the render path stays a projection.
UENUM(BlueprintType)
enum class ECk_WorldSpaceWidget_Occlusion_Policy : uint8
{
    None,
    HideWhenOccluded
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_WorldSpaceWidget_Occlusion_Policy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWORLDSPACEWIDGET_API FCk_WorldSpaceWidget_OcclusionInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WorldSpaceWidget_OcclusionInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    ECk_WorldSpaceWidget_Occlusion_Policy _OcclusionPolicy = ECk_WorldSpaceWidget_Occlusion_Policy::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, EditCondition="_OcclusionPolicy == ECk_WorldSpaceWidget_Occlusion_Policy::HideWhenOccluded"))
    TEnumAsByte<ECollisionChannel> _TraceChannel = ECollisionChannel::ECC_Camera;

public:
    CK_PROPERTY_GET(_OcclusionPolicy);
    CK_PROPERTY(_TraceChannel);

    CK_DEFINE_CONSTRUCTORS(FCk_WorldSpaceWidget_OcclusionInfo, _OcclusionPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWORLDSPACEWIDGET_API FCk_WorldSpaceWidget_ScalingInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WorldSpaceWidget_ScalingInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    ECk_WorldSpaceWidget_Scaling_Policy _ScalingPolicy = ECk_WorldSpaceWidget_Scaling_Policy::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, EditCondition="_ScalingPolicy == ECk_WorldSpaceWidget_Scaling_Policy::ScaleWithDistance"))
    float _MaxScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, UIMin = "0.0", ClampMin = "0.0", EditCondition="_ScalingPolicy == ECk_WorldSpaceWidget_Scaling_Policy::ScaleWithDistance"))
    float _MinScale = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, EditCondition="_ScalingPolicy == ECk_WorldSpaceWidget_Scaling_Policy::ScaleWithDistance"))
    float _ScaleFalloff_StartDistance = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, EditCondition="_ScalingPolicy == ECk_WorldSpaceWidget_Scaling_Policy::ScaleWithDistance"))
    float _ScaleFalloff_EndDistance = 5000.0f;

public:
    CK_PROPERTY_GET(_ScalingPolicy);
    CK_PROPERTY(_MaxScale);
    CK_PROPERTY(_MinScale);
    CK_PROPERTY(_ScaleFalloff_StartDistance);
    CK_PROPERTY(_ScaleFalloff_EndDistance);

    CK_DEFINE_CONSTRUCTORS(FCk_WorldSpaceWidget_ScalingInfo, _ScalingPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_WorldSpaceWidget_Fading_Policy : uint8
{
    None,
    FadeWithDistance
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_WorldSpaceWidget_Fading_Policy);

// --------------------------------------------------------------------------------------------------------------------

// FadeWithDistance maps camera->anchor distance to an opacity: MaxOpacity at/below
// StartDistance, MinOpacity at/above EndDistance, lerped between. It is a multiplier on the
// enabled-state, so it never fights occlusion or the off-screen hide (which force opacity to 0).
USTRUCT(BlueprintType)
struct CKWORLDSPACEWIDGET_API FCk_WorldSpaceWidget_FadingInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WorldSpaceWidget_FadingInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    ECk_WorldSpaceWidget_Fading_Policy _FadingPolicy = ECk_WorldSpaceWidget_Fading_Policy::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0",
              EditCondition="_FadingPolicy == ECk_WorldSpaceWidget_Fading_Policy::FadeWithDistance"))
    float _MaxOpacity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0",
              EditCondition="_FadingPolicy == ECk_WorldSpaceWidget_Fading_Policy::FadeWithDistance"))
    float _MinOpacity = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, EditCondition="_FadingPolicy == ECk_WorldSpaceWidget_Fading_Policy::FadeWithDistance"))
    float _FadeFalloff_StartDistance = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, EditCondition="_FadingPolicy == ECk_WorldSpaceWidget_Fading_Policy::FadeWithDistance"))
    float _FadeFalloff_EndDistance = 5000.0f;

public:
    CK_PROPERTY_GET(_FadingPolicy);
    CK_PROPERTY(_MaxOpacity);
    CK_PROPERTY(_MinOpacity);
    CK_PROPERTY(_FadeFalloff_StartDistance);
    CK_PROPERTY(_FadeFalloff_EndDistance);

    CK_DEFINE_CONSTRUCTORS(FCk_WorldSpaceWidget_FadingInfo, _FadingPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

// Opt-in scaling of the SCREEN-SPACE offset, not of the widget itself. AspectScaling scales the
// X offset by the viewport aspect vs 16:9 (clamped) so horizontal callouts don't drift on
// ultrawide/narrow displays; DistanceScaling scales per-axis by camera distance. Both compose.
UENUM(BlueprintType)
enum class ECk_WorldSpaceWidget_AspectScaling_Policy : uint8
{
    None,
    ScaleXByAspectRatio
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_WorldSpaceWidget_AspectScaling_Policy);

// --------------------------------------------------------------------------------------------------------------------

// What unit ScreenSpaceOffset is authored in.
//
// ViewportPixels is the historical meaning and remains the DEFAULT: the offset is applied as raw
// viewport pixels, i.e. a fixed pixel distance at every resolution. That is almost always the
// wrong look — the widget it displaces is laid out in DESIGN space and so grows with the DPI
// curve, meaning the further the viewport is from the resolution the offset was tuned at, the
// further the widget drifts off its anchor. It stays the default anyway, because every existing
// caller's numbers were tuned against it and re-interpreting the field would silently move all
// of them.
//
// DesignSpace authors the offset in the SAME units as the widget's own size (the space
// DesignScreenSize / UIScaleCurve define), so the offset and the widget scale together and the
// composition holds at every resolution. Prefer it for new callers.
UENUM(BlueprintType)
enum class ECk_WorldSpaceWidget_ScreenOffset_Space : uint8
{
    ViewportPixels,
    DesignSpace
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_WorldSpaceWidget_ScreenOffset_Space);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_WorldSpaceWidget_OffsetDistanceScaling_Policy : uint8
{
    None,
    ScaleWithDistance
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_WorldSpaceWidget_OffsetDistanceScaling_Policy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWORLDSPACEWIDGET_API FCk_WorldSpaceWidget_ScreenOffsetScalingInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WorldSpaceWidget_ScreenOffsetScalingInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    ECk_WorldSpaceWidget_AspectScaling_Policy _AspectScalingPolicy = ECk_WorldSpaceWidget_AspectScaling_Policy::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, EditCondition="_AspectScalingPolicy == ECk_WorldSpaceWidget_AspectScaling_Policy::ScaleXByAspectRatio"))
    float _AspectRatio_MinScale = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, EditCondition="_AspectScalingPolicy == ECk_WorldSpaceWidget_AspectScaling_Policy::ScaleXByAspectRatio"))
    float _AspectRatio_MaxScale = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    ECk_WorldSpaceWidget_OffsetDistanceScaling_Policy _DistanceScalingPolicy = ECk_WorldSpaceWidget_OffsetDistanceScaling_Policy::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, EditCondition="_DistanceScalingPolicy == ECk_WorldSpaceWidget_OffsetDistanceScaling_Policy::ScaleWithDistance"))
    FVector2D _DistanceScale_Max = FVector2D{1.0, 1.0};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, EditCondition="_DistanceScalingPolicy == ECk_WorldSpaceWidget_OffsetDistanceScaling_Policy::ScaleWithDistance"))
    FVector2D _DistanceScale_Min = FVector2D{0.1, 0.1};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, EditCondition="_DistanceScalingPolicy == ECk_WorldSpaceWidget_OffsetDistanceScaling_Policy::ScaleWithDistance"))
    float _DistanceFalloff_StartDistance = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, EditCondition="_DistanceScalingPolicy == ECk_WorldSpaceWidget_OffsetDistanceScaling_Policy::ScaleWithDistance"))
    float _DistanceFalloff_EndDistance = 5000.0f;

public:
    CK_PROPERTY(_AspectScalingPolicy);
    CK_PROPERTY(_AspectRatio_MinScale);
    CK_PROPERTY(_AspectRatio_MaxScale);
    CK_PROPERTY(_DistanceScalingPolicy);
    CK_PROPERTY(_DistanceScale_Max);
    CK_PROPERTY(_DistanceScale_Min);
    CK_PROPERTY(_DistanceFalloff_StartDistance);
    CK_PROPERTY(_DistanceFalloff_EndDistance);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWORLDSPACEWIDGET_API FCk_WorldSpaceWidget_LocationInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WorldSpaceWidget_LocationInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FVector _WorldSpaceOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FVector2D _ScreenSpaceOffset = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    ECk_WorldSpaceWidget_Clamping_Policy _ClampingPolicy = ECk_WorldSpaceWidget_Clamping_Policy::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    ECk_WorldSpaceWidget_ScreenOffset_Space _ScreenSpaceOffset_Space = ECk_WorldSpaceWidget_ScreenOffset_Space::ViewportPixels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_WorldSpaceWidget_ScreenOffsetScalingInfo _ScreenOffsetScaling;

public:
    CK_PROPERTY_GET(_WorldSpaceOffset);
    CK_PROPERTY_GET(_ScreenSpaceOffset);
    CK_PROPERTY(_ClampingPolicy);
    CK_PROPERTY(_ScreenSpaceOffset_Space);
    CK_PROPERTY(_ScreenOffsetScaling);

    CK_DEFINE_CONSTRUCTORS(FCk_WorldSpaceWidget_LocationInfo, _WorldSpaceOffset, _ScreenSpaceOffset);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWORLDSPACEWIDGET_API FCk_Fragment_WorldSpaceWidget_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_WorldSpaceWidget_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    TWeakObjectPtr<UUserWidget> _Widget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    ECk_UI_Widget_ViewportOperation _InitialViewportOperation = ECk_UI_Widget_ViewportOperation::AddToViewport;

    // Ranks against the CommonUI layout's own _LayoutZOrder (default 10); exceed it
    // only for a widget that must cover open menus.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    int32 _ZOrder = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_WorldSpaceWidget_LocationInfo _LocationInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_WorldSpaceWidget_ScalingInfo _ScalingInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_WorldSpaceWidget_FadingInfo _FadingInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_WorldSpaceWidget_OcclusionInfo _OcclusionInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    ECk_WorldSpaceWidget_RenderMode _RenderMode = ECk_WorldSpaceWidget_RenderMode::ScreenOverlay;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true, EditCondition="_RenderMode == ECk_WorldSpaceWidget_RenderMode::WorldComponent"))
    FCk_WorldSpaceWidget_WorldComponentInfo _WorldComponentInfo;

public:
    CK_PROPERTY_GET(_Widget);
    CK_PROPERTY_GET(_InitialViewportOperation);
    CK_PROPERTY_GET(_ZOrder);
    CK_PROPERTY(_LocationInfo);
    CK_PROPERTY(_ScalingInfo);
    CK_PROPERTY(_FadingInfo);
    CK_PROPERTY(_OcclusionInfo);
    CK_PROPERTY(_RenderMode);
    CK_PROPERTY(_WorldComponentInfo);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_WorldSpaceWidget_ParamsData, _Widget, _InitialViewportOperation, _ZOrder);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWORLDSPACEWIDGET_API FCk_Request_WorldSpaceWidget_SetLocationInfo : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_WorldSpaceWidget_SetLocationInfo);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_WorldSpaceWidget_SetLocationInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_WorldSpaceWidget_LocationInfo _LocationInfo;

public:
    CK_PROPERTY_GET(_LocationInfo);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_WorldSpaceWidget_SetLocationInfo, _LocationInfo);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWORLDSPACEWIDGET_API FCk_Request_WorldSpaceWidget_SetScalingInfo : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_WorldSpaceWidget_SetScalingInfo);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_WorldSpaceWidget_SetScalingInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_WorldSpaceWidget_ScalingInfo _ScalingInfo;

public:
    CK_PROPERTY_GET(_ScalingInfo);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_WorldSpaceWidget_SetScalingInfo, _ScalingInfo);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWORLDSPACEWIDGET_API FCk_Request_WorldSpaceWidget_SetFadingInfo : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_WorldSpaceWidget_SetFadingInfo);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_WorldSpaceWidget_SetFadingInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_WorldSpaceWidget_FadingInfo _FadingInfo;

public:
    CK_PROPERTY_GET(_FadingInfo);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_WorldSpaceWidget_SetFadingInfo, _FadingInfo);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKWORLDSPACEWIDGET_API FCk_Request_WorldSpaceWidget_SetOcclusionInfo : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_WorldSpaceWidget_SetOcclusionInfo);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_WorldSpaceWidget_SetOcclusionInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_WorldSpaceWidget_OcclusionInfo _OcclusionInfo;

public:
    CK_PROPERTY_GET(_OcclusionInfo);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_WorldSpaceWidget_SetOcclusionInfo, _OcclusionInfo);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintType, NotBlueprintable)
class CKWORLDSPACEWIDGET_API UCk_WorldSpaceWidget_Wrapper_UE : public UUserWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_WorldSpaceWidget_Wrapper_UE);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|WorldSpaceWidget")
    static UCk_WorldSpaceWidget_Wrapper_UE*
    Request_WrapWidget(
        UUserWidget* InContentWidget,
        int32 InZOrder);

protected:
    auto
    BuildWidgetHierarchy() -> void;

    auto
    Initialize() -> bool override;

    auto
    NativePreConstruct() -> void override;

    auto
    NativeConstruct() -> void override;

    auto
    NativeOnInitialized() -> void override;

    auto
    RebuildWidget() -> TSharedRef<SWidget> override;

public:
    auto Request_SetWidgetScale(const FVector2D& InScale) const -> void;

private:
    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> _ContentWidget;

    UPROPERTY(Transient)
    TObjectPtr<USizeBox> _ScalingBox;

public:
    CK_PROPERTY_GET(_ContentWidget);
    CK_PROPERTY_GET(_ScalingBox);
};
