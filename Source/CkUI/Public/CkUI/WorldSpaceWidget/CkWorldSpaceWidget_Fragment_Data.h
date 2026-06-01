#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkUI/CkUI_Utils.h"

#include "Components/CanvasPanel.h"
#include "Components/SizeBox.h"
#include "Components/WidgetComponent.h"

#include "Engine/EngineTypes.h"

#include "CkWorldSpaceWidget_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKUI_API FCk_Handle_WorldSpaceWidget : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_WorldSpaceWidget);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_WorldSpaceWidget);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_WorldSpaceWidget_Clamping_Policy : uint8
{
    None,
    ClampToViewport
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

// ScreenOverlay (default): the widget is projected world->screen each frame and
// drawn as a 2D viewport overlay (always screen-facing/billboard, constant pixel
// size; depth-occlusion only via the OcclusionInfo anchor trace).
// WorldComponent: the widget is a real 3D UWidgetComponent (Space=World) — fixed
// world orientation, true perspective scaling, free per-pixel GPU occlusion.
// Matches the legacy /Script/UMG.WidgetComponent callout.
UENUM(BlueprintType)
enum class ECk_WorldSpaceWidget_RenderMode : uint8
{
    ScreenOverlay,
    WorldComponent
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_WorldSpaceWidget_RenderMode);

// --------------------------------------------------------------------------------------------------------------------

// Consumed only when RenderMode == WorldComponent. The content widget instance
// (Params._Widget) is handed to the UWidgetComponent via SetWidget — we do NOT
// rely on the component's own InitWidget/SetWidgetClass instantiation, which is
// unreliable for runtime-created components.
USTRUCT(BlueprintType)
struct CKUI_API FCk_WorldSpaceWidget_WorldComponentInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WorldSpaceWidget_WorldComponentInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FIntPoint _DrawSize = FIntPoint{512, 512};

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    TObjectPtr<UMaterialInterface> _OverrideMaterial;

public:
    CK_PROPERTY(_DrawSize);
    CK_PROPERTY(_Pivot);
    CK_PROPERTY(_BlendMode);
    CK_PROPERTY(_GeometryMode);
    CK_PROPERTY(_TwoSided);
    CK_PROPERTY(_OverrideMaterial);

    CK_DEFINE_CONSTRUCTORS(FCk_WorldSpaceWidget_WorldComponentInfo, _DrawSize);
};

// --------------------------------------------------------------------------------------------------------------------

// HideWhenOccluded: each frame, trace from the player camera to the widget's
// world anchor (player pawn ignored); on a blocking hit the overlay is faded to
// zero opacity. Per-anchor occlusion (whole-widget), not per-pixel GPU masking —
// the render path stays a screen-space projection. Mirrors the legacy
// WorldSpaceWidgets plugin's bShouldBeOccluded behaviour.
UENUM(BlueprintType)
enum class ECk_WorldSpaceWidget_Occlusion_Policy : uint8
{
    None,
    HideWhenOccluded
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_WorldSpaceWidget_Occlusion_Policy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_WorldSpaceWidget_OcclusionInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WorldSpaceWidget_OcclusionInfo);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    ECk_WorldSpaceWidget_Occlusion_Policy _OcclusionPolicy = ECk_WorldSpaceWidget_Occlusion_Policy::None;

    // Legacy WorldSpaceWidgets traced on ECC_Camera; kept as the default.
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
struct CKUI_API FCk_WorldSpaceWidget_ScalingInfo
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

USTRUCT(BlueprintType)
struct CKUI_API FCk_WorldSpaceWidget_FadingInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WorldSpaceWidget_FadingInfo);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_WorldSpaceWidget_LocationInfo
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

public:
    CK_PROPERTY_GET(_WorldSpaceOffset);
    CK_PROPERTY_GET(_ScreenSpaceOffset);
    CK_PROPERTY_GET(_ClampingPolicy);

    CK_DEFINE_CONSTRUCTORS(FCk_WorldSpaceWidget_LocationInfo, _WorldSpaceOffset, _ScreenSpaceOffset, _ClampingPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_Fragment_WorldSpaceWidget_ParamsData
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

UCLASS(NotBlueprintType, NotBlueprintable)
class CKUI_API UCk_WorldSpaceWidget_Wrapper_UE : public UUserWidget
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
