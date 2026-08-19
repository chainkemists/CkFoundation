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

    using FFragment_WorldSpaceWidget_Params = FCk_Fragment_WorldSpaceWidget_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKWORLDSPACEWIDGET_API FFragment_WorldSpaceWidget_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_WorldSpaceWidget_Current);

    public:
        friend class FProcessor_WorldSpaceWidget_HandleRequests;
        friend class UCk_Utils_WorldSpaceWidget_UE;

    public:
        FFragment_WorldSpaceWidget_Current() = default;

        explicit
        FFragment_WorldSpaceWidget_Current(
            UCk_WorldSpaceWidget_Wrapper_UE* InWrapperWidget);

        FFragment_WorldSpaceWidget_Current(
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
