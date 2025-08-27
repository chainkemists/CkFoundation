#include "CkWorldSpaceWidget_Fragment_Data.h"

#include <Components/CanvasPanelSlot.h>
#include <Blueprint/WidgetTree.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_WorldSpaceWidget_Wrapper_UE::
    Request_WrapWidget(
        UUserWidget* InContentWidget)
    -> UCk_WorldSpaceWidget_Wrapper_UE*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InContentWidget), TEXT("Invalid InContentWidget"))
    { return nullptr; }

    CK_ENSURE_IF_NOT(ck::IsValid(InContentWidget->GetOwningPlayer()), TEXT("Invalid OwningPlayer"))
    { return nullptr; }

    auto NewWrapperWidget = NewObject<UCk_WorldSpaceWidget_Wrapper_UE>(
        InContentWidget->GetOwningPlayer(),
        UCk_WorldSpaceWidget_Wrapper_UE::StaticClass()
    );

    CK_ENSURE_IF_NOT(ck::IsValid(NewWrapperWidget), TEXT("Failed to create wrapper widget"))
    { return nullptr; }

    NewWrapperWidget->_ContentWidget = InContentWidget;
    NewWrapperWidget->Initialize();

    if (ck::Is_NOT_Valid(NewWrapperWidget->WidgetTree))
    {
        NewWrapperWidget->WidgetTree = NewObject<UWidgetTree>(NewWrapperWidget, TEXT("WidgetTree"));
    }

    NewWrapperWidget->BuildWidgetHierarchy();
    NewWrapperWidget->AddToViewport(9999);

    return NewWrapperWidget;
}

auto
    UCk_WorldSpaceWidget_Wrapper_UE::
    BuildWidgetHierarchy()
    -> void
{
    if (ck::Is_NOT_Valid(WidgetTree))
    {
        WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    }

    auto RootPanelWidget = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Panel Root"));
    CK_ENSURE_IF_NOT(ck::IsValid(RootPanelWidget), TEXT("Failed to create RootPanelWidget"))
    { return; }

    _ScalingBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Scaling Root"));
    CK_ENSURE_IF_NOT(ck::IsValid(_ScalingBox), TEXT("Failed to create ScalingBox"))
    { return; }

    _ScalingBox->ClearHeightOverride();
    _ScalingBox->ClearWidthOverride();

    auto RootPanelSlot = Cast<UCanvasPanelSlot>(RootPanelWidget->AddChild(_ScalingBox));
    if (ck::IsValid(RootPanelSlot))
    {
        RootPanelSlot->SetAutoSize(true);
        RootPanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    }

    if (ck::IsValid(_ContentWidget))
    {
        _ScalingBox->AddChild(_ContentWidget);
    }

    WidgetTree->RootWidget = RootPanelWidget;
}

auto
    UCk_WorldSpaceWidget_Wrapper_UE::
    NativePreConstruct()
    -> void
{
    Super::NativePreConstruct();
}

auto
    UCk_WorldSpaceWidget_Wrapper_UE::
    NativeConstruct()
    -> void
{
    Super::NativeConstruct();

    CK_ENSURE_IF_NOT(ck::IsValid(_ScalingBox), TEXT("ScalingBox is NULL in NativeConstruct - hierarchy was not built properly"))
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(_ContentWidget), TEXT("ContentWidget is NULL in NativeConstruct"))
    { return; }

    ForceLayoutPrepass();
}

auto
    UCk_WorldSpaceWidget_Wrapper_UE::
    NativeOnInitialized()
    -> void
{
    Super::NativeOnInitialized();
}

auto
    UCk_WorldSpaceWidget_Wrapper_UE::
    RebuildWidget()
    -> TSharedRef<SWidget>
{
    if (ck::Is_NOT_Valid(WidgetTree) || ck::Is_NOT_Valid(WidgetTree->RootWidget))
    {
        BuildWidgetHierarchy();
    }

    return Super::RebuildWidget();
}

auto
    UCk_WorldSpaceWidget_Wrapper_UE::
    Initialize()
    -> bool
{
    Super::Initialize();

    if (ck::Is_NOT_Valid(GetOwningPlayer()) && ck::IsValid(_ContentWidget))
    {
        SetOwningPlayer(_ContentWidget->GetOwningPlayer());
    }

    return true;
}

auto
    UCk_WorldSpaceWidget_Wrapper_UE::
    Request_SetWidgetScale(
        const FVector2D& InScale) const
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_ScalingBox), TEXT("Request_SetWidgetScale called but ScalingBox is invalid"))
    { return; }

    _ScalingBox->SetRenderScale(InScale);
}
