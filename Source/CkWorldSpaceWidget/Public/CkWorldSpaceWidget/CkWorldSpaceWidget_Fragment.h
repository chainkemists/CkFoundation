#pragma once

#include "CkWorldSpaceWidget/CkWorldSpaceWidget_Fragment_Data.h"

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

    private:
        // STRONG: pins the caller-supplied content widget, which the pooling subsystem never handed out
        TStrongObjectPtr<UUserWidget> _ContentWidgetHardRef;
        // WEAK — lifetimes owned by the CkCore ObjectPooling subsystem (DestroyOnRelease)
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
