#pragma once

#include "CkWorldSpaceWidget/CkWorldSpaceWidget_Fragment_Data.h"

#include "Components/SlateWrapperTypes.h"
#include "Components/WidgetComponent.h"

#include <GameFramework/PlayerController.h>

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_WorldSpaceWidget_UE;
class ULocalPlayer;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_WorldSpaceWidget_NeedsUpdateScaling);
    CK_DEFINE_ECS_TAG(FTag_WorldSpaceWidget_Disabled);

    // --------------------------------------------------------------------------------------------------------------------

    /** The retained immutable residue of FCk_WorldSpaceWidget_Spec: the authored fields nothing
     *  replaces at runtime. The four *Info fields are NOT here -- each has its own Request_Set*, so
     *  they live in FFragment_WorldSpaceWidget_Tunables. */
    struct CKWORLDSPACEWIDGET_API FFragment_WorldSpaceWidget_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_WorldSpaceWidget_Params);

    private:
        TWeakObjectPtr<UUserWidget> _Widget;

        ECk_UI_Widget_ViewportOperation _InitialViewportOperation = ECk_UI_Widget_ViewportOperation::AddToViewport;

        int32 _ZOrder = 0;

        ECk_WorldSpaceWidget_RenderMode _RenderMode = ECk_WorldSpaceWidget_RenderMode::ScreenOverlay;

        FCk_WorldSpaceWidget_WorldComponentInfo _WorldComponentInfo;

    public:
        CK_PROPERTY_GET(_Widget);
        CK_PROPERTY_GET(_InitialViewportOperation);
        CK_PROPERTY_GET(_ZOrder);
        CK_PROPERTY_GET(_RenderMode);
        CK_PROPERTY_GET(_WorldComponentInfo);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_WorldSpaceWidget_Params, _Widget, _InitialViewportOperation,
            _ZOrder, _RenderMode, _WorldComponentInfo);
    };

    // --------------------------------------------------------------------------------------------------------------------

    /** Presentation config a request REPLACES at runtime. Not _Params: that name promises
     *  immutability, and the promise is what lets the update processors take their view TReadOnly. */
    struct CKWORLDSPACEWIDGET_API FFragment_WorldSpaceWidget_Tunables
    {
    public:
        CK_GENERATED_BODY(FFragment_WorldSpaceWidget_Tunables);

    public:
        friend class FProcessor_WorldSpaceWidget_HandleRequests;

    private:
        FCk_WorldSpaceWidget_LocationInfo _LocationInfo;
        FCk_WorldSpaceWidget_ScalingInfo _ScalingInfo;
        FCk_WorldSpaceWidget_FadingInfo _FadingInfo;
        FCk_WorldSpaceWidget_OcclusionInfo _OcclusionInfo;

    public:
        CK_PROPERTY_GET(_LocationInfo);
        CK_PROPERTY_GET(_ScalingInfo);
        CK_PROPERTY_GET(_FadingInfo);
        CK_PROPERTY_GET(_OcclusionInfo);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_WorldSpaceWidget_Tunables, _LocationInfo, _ScalingInfo,
            _FadingInfo, _OcclusionInfo);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKWORLDSPACEWIDGET_API FFragment_WorldSpaceWidget
    {
    public:
        CK_GENERATED_BODY(FFragment_WorldSpaceWidget);

    public:
        friend class FProcessor_WorldSpaceWidget_HandleRequests;
        friend class UCk_Utils_WorldSpaceWidget_UE;

    public:
        FFragment_WorldSpaceWidget() = default;

        explicit
        FFragment_WorldSpaceWidget(
            UCk_WorldSpaceWidget_Wrapper_UE* InWrapperWidget);

        FFragment_WorldSpaceWidget(
            UWidgetComponent* InWidgetComponent,
            UUserWidget* InContentWidget);

    public:
        // The controller CURRENTLY driving the owning local player — resolved on every read because
        // controller identity churns (DebugCamera swap, travel, possession) while the local player does not.
        auto Get_ResolvedOwningPlayer() const -> APlayerController*;

        // True while the owning player's viewport renders from the editor camera (PIE ejected / SIE) —
        // no PlayerController represents that view, so a projection through the player is undefined.
        auto Get_IsRenderViewEjected() const -> bool;

    private:
        // STRONG: pins the caller-supplied content widget, which the pooling subsystem never handed out
        TStrongObjectPtr<UUserWidget> _ContentWidgetHardRef;
        // WEAK — lifetimes owned by the CkCore ObjectPooling subsystem (DestroyOnRelease)
        TWeakObjectPtr<UCk_WorldSpaceWidget_Wrapper_UE> _WrapperWidget;
        TWeakObjectPtr<ULocalPlayer> _WidgetOwningLocalPlayer;
        TWeakObjectPtr<UWidgetComponent> _WidgetComponent;
        // ScreenOverlay only: the wrapper's visibility as it was before the disable that
        // Collapsed it, so re-enabling restores what the widget actually had.
        TOptional<ESlateVisibility> _PreDisableVisibility;

    public:
        CK_PROPERTY_GET(_WidgetOwningLocalPlayer);
        CK_PROPERTY_GET(_WrapperWidget);
        CK_PROPERTY_GET(_ContentWidgetHardRef);
        CK_PROPERTY_GET(_WidgetComponent);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKWORLDSPACEWIDGET_API FFragment_WorldSpaceWidget_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_WorldSpaceWidget_Requests);

    public:
        friend class FProcessor_WorldSpaceWidget_HandleRequests;
        friend class UCk_Utils_WorldSpaceWidget_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_WorldSpaceWidget_SetLocationInfo,
            FCk_Request_WorldSpaceWidget_SetScalingInfo,
            FCk_Request_WorldSpaceWidget_SetFadingInfo,
            FCk_Request_WorldSpaceWidget_SetOcclusionInfo>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}
