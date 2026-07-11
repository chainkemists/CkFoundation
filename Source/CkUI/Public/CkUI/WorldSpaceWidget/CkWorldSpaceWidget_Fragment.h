#pragma once

#include "CkUI/WorldSpaceWidget/CkWorldSpaceWidget_Fragment_Data.h"

#include "Components/WidgetComponent.h"

#include <GameFramework/PlayerController.h>

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_WorldSpaceWidget_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ECS_TAG(FTag_WorldSpaceWidget_NeedsUpdateScaling);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_WorldSpaceWidget_Params = FCk_Fragment_WorldSpaceWidget_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKUI_API FFragment_WorldSpaceWidget_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_WorldSpaceWidget_Current);

    public:
        friend class FProcessor_WorldSpaceWidget_HandleRequests;
        friend class UCk_Utils_WorldSpaceWidget_UE;

    public:
        FFragment_WorldSpaceWidget_Current() = default;

        // ScreenOverlay mode: wraps the content widget and adds it to the viewport.
        explicit
        FFragment_WorldSpaceWidget_Current(
            UCk_WorldSpaceWidget_Wrapper_UE* InWrapperWidget);

        // WorldComponent mode: a real 3D UWidgetComponent displays the explicit
        // content-widget instance (assigned via SetWidget), depth-occluded
        // per-pixel by world geometry.
        FFragment_WorldSpaceWidget_Current(
            UWidgetComponent* InWidgetComponent,
            UUserWidget* InContentWidget);

    private:
        // STRONG: pins the caller-supplied content widget, which the pooling subsystem never vended
        TStrongObjectPtr<UUserWidget> _ContentWidgetHardRef;
        // WEAK (wrapper + component): the CkCore ObjectPooling subsystem owns their lifetime (vended
        // DestroyOnRelease through the pooling-aware Request_CreateNewObject; EndPlay releases them)
        TWeakObjectPtr<UCk_WorldSpaceWidget_Wrapper_UE> _WrapperWidget;
        TWeakObjectPtr<APlayerController> _WidgetOwningPlayer;
        TWeakObjectPtr<UWidgetComponent> _WidgetComponent;
        bool _Enabled = true;

    public:
        CK_PROPERTY_GET(_WidgetOwningPlayer);
        CK_PROPERTY_GET(_WrapperWidget);
        CK_PROPERTY_GET(_ContentWidgetHardRef);
        CK_PROPERTY_GET(_WidgetComponent);
        CK_PROPERTY(_Enabled);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKUI_API FFragment_WorldSpaceWidget_Requests
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
