#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkUI/CkUI_Utils.h"

#include "Components/CanvasPanel.h"
#include "Components/SizeBox.h"

#include "CkWorldSpaceWidget_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKUI_API FCk_Handle_WorldSpaceWidget : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_WorldSpaceWidget); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_WorldSpaceWidget);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_WorldSpaceWidget_Clamping_Policy : uint8
{
    None,

    // The widget should always remain visible in the viewport and clamped to the edges
    ClampToViewport
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_WorldSpaceWidget_Clamping_Policy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_WorldSpaceWidget_Scaling_Policy : uint8
{
    None,

    // The widget should appear larger/smaller based on the distance from the camera
    ScaleWithDistance
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_WorldSpaceWidget_Scaling_Policy);

//--------------------------------------------------------------------------------------------------------------------

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
    CK_PROPERTY_GET(_MaxScale);
    CK_PROPERTY_GET(_MinScale);
    CK_PROPERTY_GET(_ScaleFalloff_StartDistance);
    CK_PROPERTY_GET(_ScaleFalloff_EndDistance);
};

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKUI_API FCk_WorldSpaceWidget_FadingInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_WorldSpaceWidget_FadingInfo);
};

//--------------------------------------------------------------------------------------------------------------------

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
    ECk_WorldSpaceWidget_Clamping_Policy _ClampingPolicy= ECk_WorldSpaceWidget_Clamping_Policy::None;

public:
    CK_PROPERTY_GET(_WorldSpaceOffset);
    CK_PROPERTY_GET(_ScreenSpaceOffset);
    CK_PROPERTY_GET(_ClampingPolicy);
};

//--------------------------------------------------------------------------------------------------------------------

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
        meta=(AllowPrivateAccess = true))
    FCk_WorldSpaceWidget_LocationInfo _LocationInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_WorldSpaceWidget_ScalingInfo _ScalingInfo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess = true))
    FCk_WorldSpaceWidget_FadingInfo _FadingInfo;

public:
    CK_PROPERTY_GET(_Widget);
    CK_PROPERTY_GET(_InitialViewportOperation);
    CK_PROPERTY_GET(_LocationInfo);
    CK_PROPERTY_GET(_ScalingInfo);
    CK_PROPERTY_GET(_FadingInfo);
};

// --------------------------------------------------------------------------------------------------------------------

// Utility widget that wraps any content widget to provide world space positioning and center-based scaling.
UCLASS(NotBlueprintType, NotBlueprintable)
class CKUI_API UCk_WorldSpaceWidget_Wrapper_UE : public UUserWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_WorldSpaceWidget_Wrapper_UE);

public:
    static auto
    Request_WrapWidget(
        UUserWidget* InContentWidget) -> UCk_WorldSpaceWidget_Wrapper_UE*;

protected:
    // Build the widget hierarchy programmatically
    void BuildWidgetHierarchy();

    // Override initialization to set up owning player
    virtual bool Initialize() override;

    // Widget lifecycle overrides
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;
    virtual void NativeOnInitialized() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;

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

// --------------------------------------------------------------------------------------------------------------------
