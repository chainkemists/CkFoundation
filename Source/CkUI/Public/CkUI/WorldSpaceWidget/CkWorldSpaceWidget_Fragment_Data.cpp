// CkWorldSpaceWidget_Fragment_Data.cpp
#include "CkWorldSpaceWidget_Fragment_Data.h"

#include <Components/CanvasPanelSlot.h>
#include <Blueprint/WidgetTree.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_WorldSpaceWidget_Wrapper_UE* UCk_WorldSpaceWidget_Wrapper_UE::Request_WrapWidget(UUserWidget* InContentWidget)
{
    if (!InContentWidget || !InContentWidget->GetOwningPlayer())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid InContentWidget or OwningPlayer"));
        return nullptr;
    }

    auto* NewWrapperWidget = NewObject<UCk_WorldSpaceWidget_Wrapper_UE>(
        InContentWidget->GetOwningPlayer(),
        UCk_WorldSpaceWidget_Wrapper_UE::StaticClass()
    );

    if (!NewWrapperWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create wrapper widget!"));
        return nullptr;
    }

    // Set the content widget BEFORE initializing
    NewWrapperWidget->_ContentWidget = InContentWidget;

    // Now initialize the widget tree
    NewWrapperWidget->Initialize();

    // Important: Call WidgetTree ConstructWidget if needed
    if (!NewWrapperWidget->WidgetTree)
    {
        NewWrapperWidget->WidgetTree = NewObject<UWidgetTree>(NewWrapperWidget, TEXT("WidgetTree"));
    }

    // Build the widget hierarchy
    NewWrapperWidget->BuildWidgetHierarchy();

    UE_LOG(LogTemp, Warning, TEXT("Wrapper Widget Created: %s"), *NewWrapperWidget->GetName());

    // Add to viewport AFTER everything is set up
    NewWrapperWidget->AddToViewport(9999);

    return NewWrapperWidget;
}

void UCk_WorldSpaceWidget_Wrapper_UE::BuildWidgetHierarchy()
{
    if (!WidgetTree)
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
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
    _ScalingBox->ClearHeightOverride();
    _ScalingBox->ClearWidthOverride();

    UE_LOG(LogTemp, Warning, TEXT("Scaling Box Created: %s"), *_ScalingBox->GetName());

    // Add ScalingBox to RootPanel
    UCanvasPanelSlot* RootPanelSlot = Cast<UCanvasPanelSlot>(RootPanelWidget->AddChild(_ScalingBox));
    if (RootPanelSlot)
    {
        RootPanelSlot->SetAutoSize(true);
        RootPanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    }

    // Add Content Widget to ScalingBox
    if (_ContentWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("Adding Content Widget: %s to ScalingBox"), *_ContentWidget->GetName());
        _ScalingBox->AddChild(_ContentWidget);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ContentWidget is NULL when building hierarchy"));
    }

    // Set the root widget
    WidgetTree->RootWidget = RootPanelWidget;
}

void UCk_WorldSpaceWidget_Wrapper_UE::NativePreConstruct()
{
    Super::NativePreConstruct();

    // If we're in the designer/editor, we might want to set up a preview
    // But for runtime, the hierarchy should already be built via BuildWidgetHierarchy
}

void UCk_WorldSpaceWidget_Wrapper_UE::NativeConstruct()
{
    Super::NativeConstruct();

    // At this point, the widget hierarchy should already be set up
    // Just verify everything is in place
    if (!_ScalingBox)
    {
        UE_LOG(LogTemp, Error, TEXT("ScalingBox is NULL in NativeConstruct - hierarchy was not built properly"));
        return;
    }

    if (!_ContentWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("ContentWidget is NULL in NativeConstruct"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("NativeConstruct completed successfully"));
    ForceLayoutPrepass();
}

void UCk_WorldSpaceWidget_Wrapper_UE::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    UE_LOG(LogTemp, Warning, TEXT("NativeOnInitialized called"));

    // The widget hierarchy should be built before this point
    if (_ScalingBox && _ContentWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("Both ScalingBox and ContentWidget exist in NativeOnInitialized"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Widget hierarchy not yet built in NativeOnInitialized (this is expected)"));
    }
}

auto UCk_WorldSpaceWidget_Wrapper_UE::RebuildWidget() -> TSharedRef<SWidget>
{
    // Make sure our widget tree is properly set up when rebuilding
    if (!WidgetTree || !WidgetTree->RootWidget)
    {
        BuildWidgetHierarchy();
    }

    return Super::RebuildWidget();
}

auto UCk_WorldSpaceWidget_Wrapper_UE::Initialize() -> bool
{
    // Call parent initialize
    Super::Initialize();

    // Ensure we have an owning player
    if (!GetOwningPlayer() && _ContentWidget)
    {
        SetOwningPlayer(_ContentWidget->GetOwningPlayer());
    }

    return true;
}

auto UCk_WorldSpaceWidget_Wrapper_UE::Request_SetWidgetScale(const FVector2D& InScale) const -> void
{
    if (ck::IsValid(_ScalingBox))
    {
        _ScalingBox->SetRenderScale(InScale);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Request_SetWidgetScale called but ScalingBox is invalid"));
    }
}

// --------------------------------------------------------------------------------------------------------------------
