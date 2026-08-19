#include "CkWorldSpaceWidget_Fragment.h"

#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_WorldSpaceWidget_Current::
        FFragment_WorldSpaceWidget_Current(
            UCk_WorldSpaceWidget_Wrapper_UE* InWrapperWidget)
        : _ContentWidgetHardRef(InWrapperWidget->Get_ContentWidget())
        , _WrapperWidget(InWrapperWidget)
        , _WidgetOwningLocalPlayer(InWrapperWidget->GetOwningLocalPlayer())
    {
    }

    FFragment_WorldSpaceWidget_Current::
        FFragment_WorldSpaceWidget_Current(
            UWidgetComponent* InWidgetComponent,
            UUserWidget* InContentWidget)
        : _ContentWidgetHardRef(InContentWidget)
        , _WidgetComponent(InWidgetComponent)
    {
    }

    auto
        FFragment_WorldSpaceWidget_Current::
        Get_ResolvedOwningPlayer() const
        -> APlayerController*
    {
        const auto OwningLocalPlayer = _WidgetOwningLocalPlayer.Get();

        if (ck::Is_NOT_Valid(OwningLocalPlayer))
        { return {}; }

        return OwningLocalPlayer->PlayerController;
    }

    auto
        FFragment_WorldSpaceWidget_Current::
        Get_IsRenderViewEjected() const
        -> bool
    {
        const auto OwningLocalPlayer = _WidgetOwningLocalPlayer.Get();

        if (ck::Is_NOT_Valid(OwningLocalPlayer))
        { return false; }

        const auto ViewportClient = OwningLocalPlayer->ViewportClient.Get();

        if (ck::Is_NOT_Valid(ViewportClient))
        { return false; }

        return ViewportClient->IsSimulateInEditorViewport();
    }
}

// --------------------------------------------------------------------------------------------------------------------
