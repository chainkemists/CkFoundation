#include "CkWorldSpaceWidget_Fragment_Data.h"

#include <Components/CanvasPanelSlot.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_WorldSpaceWidget_Wrapper_UE* UCk_WorldSpaceWidget_Wrapper_UE::Request_WrapWidget(UUserWidget* InContentWidget)
{
    if (!InContentWidget || !InContentWidget->GetOwningPlayer())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid InContentWidget or OwningPlayer"));
        return nullptr;
    }

    auto* NewWrapperWidget = CreateWidget<UCk_WorldSpaceWidget_Wrapper_UE>(
        InContentWidget->GetOwningPlayer(), UCk_WorldSpaceWidget_Wrapper_UE::StaticClass()
    );

    if (!NewWrapperWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create wrapper widget!"));
        return nullptr;
    }

    UE_LOG(LogTemp, Warning, TEXT("Wrapper Widget Created: %s"), *NewWrapperWidget->GetName());

    NewWrapperWidget->_ContentWidget = InContentWidget;
    NewWrapperWidget->AddToViewport(9999);

    return NewWrapperWidget;
}

void UCk_WorldSpaceWidget_Wrapper_UE::NativeConstruct()
{
    Super::NativeConstruct();

    if (!WidgetTree)
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetTree is NULL in NativeConstruct"));
        return;
    }

    // Create root panel
    UCanvasPanel* RootPanelWidget = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Panel Root"));
    if (!RootPanelWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create RootPanelWidget"));
        return;
    }

    // Create SizeBox
    _ScalingBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Scaling Root"));
    if (!_ScalingBox)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create ScalingBox"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Scaling Box Created: %s"), *_ScalingBox->GetName());

    UCanvasPanelSlot* RootPanelSlot = Cast<UCanvasPanelSlot>(RootPanelWidget->AddChild(_ScalingBox));
    if (RootPanelSlot)
    {
        RootPanelSlot->SetAutoSize(true);
        RootPanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    }

    WidgetTree->RootWidget = RootPanelWidget;

    // Add Content Widget
    if (_ContentWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("Adding Content Widget: %s"), *_ContentWidget->GetName());
        _ScalingBox->AddChild(_ContentWidget);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ContentWidget is NULL in NativeConstruct"));
    }
}

void UCk_WorldSpaceWidget_Wrapper_UE::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    UE_LOG(LogTemp, Warning, TEXT("NativeOnInitialized called"));

    if (_ScalingBox && _ContentWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("Both ScalingBox and ContentWidget exist"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ScalingBox or ContentWidget is NULL in NativeOnInitialized"));
    }
}

auto
    UCk_WorldSpaceWidget_Wrapper_UE::
    RebuildWidget()
    -> TSharedRef<SWidget>
{
    return Super::RebuildWidget();
}

auto
    UCk_WorldSpaceWidget_Wrapper_UE::
    Request_SetWidgetScale(
        const FVector2D& InScale) const
    -> void
{
    if (ck::IsValid(_ScalingBox))
    {
        _ScalingBox->SetRenderScale(InScale);
    }
}

// --------------------------------------------------------------------------------------------------------------------
