#include "CkPanBox_Slate.h"

#include <Layout/ArrangedChildren.h>
#include <Widgets/SNullWidget.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    SCk_PanBox::
    Construct(
        const FArguments& InArgs)
    -> void
{
    _View = InArgs._View;
    _ExplicitSurfaceSize = InArgs._ExplicitSurfaceSize;

    ChildSlot
    [
        InArgs._Content.Widget
    ];
}

auto
    SCk_PanBox::
    Set_Content(
        const TSharedRef<SWidget>& InContent)
    -> void
{
    ChildSlot.AttachWidget(InContent);
    Invalidate(EInvalidateWidgetReason::Layout);
}

auto
    SCk_PanBox::
    Get_SurfaceSize() const
    -> FVector2D
{
    const auto& ExplicitSurfaceSize = _ExplicitSurfaceSize.Get();

    if (ExplicitSurfaceSize.IsSet())
    { return *ExplicitSurfaceSize; }

    return FVector2D{ChildSlot.GetWidget()->GetDesiredSize()};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCk_PanBox::
    ComputeDesiredSize(
        float InLayoutScaleMultiplier) const
    -> FVector2D
{
    // The surface at zoom 1 — never the zoomed size, so zooming can't reflow the outer layout.
    return Get_SurfaceSize();
}

auto
    SCk_PanBox::
    OnArrangeChildren(
        const FGeometry& AllottedGeometry,
        FArrangedChildren& ArrangedChildren) const
    -> void
{
    const auto& ChildWidget = ChildSlot.GetWidget();

    if (NOT ArrangedChildren.Accepts(ChildWidget->GetVisibility()))
    { return; }

    const auto& View = _View.Get();

    ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(
        ChildWidget,
        Get_SurfaceSize(),
        FSlateLayoutTransform{View.Get_Zoom(), View.Get_Offset()}));
}

// --------------------------------------------------------------------------------------------------------------------
