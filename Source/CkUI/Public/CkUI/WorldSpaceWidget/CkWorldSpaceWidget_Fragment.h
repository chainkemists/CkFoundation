#pragma once

#include "CkUI/WorldSpaceWidget/CkWorldSpaceWidget_Fragment_Data.h"

#include "Components/WidgetComponent.h"

#include <GameFramework/PlayerController.h>

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
        TStrongObjectPtr<UUserWidget> _ContentWidgetHardRef;
        TStrongObjectPtr<UCk_WorldSpaceWidget_Wrapper_UE> _WrapperWidget;
        TWeakObjectPtr<APlayerController> _WidgetOwningPlayer;
        TStrongObjectPtr<UWidgetComponent> _WidgetComponent;
        bool _Enabled = true;

    public:
        CK_PROPERTY_GET(_WidgetOwningPlayer);
        CK_PROPERTY_GET(_WrapperWidget);
        CK_PROPERTY_GET(_ContentWidgetHardRef);
        CK_PROPERTY_GET(_WidgetComponent);
        CK_PROPERTY(_Enabled);
    };
}
