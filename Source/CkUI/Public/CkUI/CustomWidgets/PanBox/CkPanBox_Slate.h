#pragma once

#include "CkUI/CustomWidgets/PanBox/CkPanBox_Types.h"

#include <CoreMinimal.h>
#include <Widgets/DeclarativeSyntaxSupport.h>
#include <Widgets/SCompoundWidget.h>

// --------------------------------------------------------------------------------------------------------------------

/**
 * Pan-and-zoom container: arranges its single child (the "surface") within the box's
 * own bounds (the "viewport") under a view transform.
 *
 * ViewportPos = SurfacePos * Zoom + Offset — the one formula every conversion derives from.
 *
 * Deliberately input-agnostic and focus-agnostic: no pointer/key/focus overrides live
 * here, so child content keeps full control over its own input and focus routing. All
 * view changes arrive through the View attribute owned by the UMG wrapper
 * (UCk_PanBoxWidget_UE), which is also where clamping happens.
 */
class CKUI_API SCk_PanBox : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCk_PanBox)
        : _View(FCk_PanBox_View{})
        , _ExplicitSurfaceSize(TOptional<FVector2D>{})
    {}
        SLATE_ATTRIBUTE(FCk_PanBox_View, View)

        // When unset, the surface is the child's desired size.
        SLATE_ATTRIBUTE(TOptional<FVector2D>, ExplicitSurfaceSize)

        SLATE_DEFAULT_SLOT(FArguments, Content)
    SLATE_END_ARGS()

public:
    auto Construct(const FArguments& InArgs) -> void;

    auto Set_Content(const TSharedRef<SWidget>& InContent) -> void;
    auto Get_SurfaceSize() const -> FVector2D;

public:
    auto ComputeDesiredSize(float InLayoutScaleMultiplier) const -> FVector2D override;

    auto OnArrangeChildren(
        const FGeometry& AllottedGeometry,
        FArrangedChildren& ArrangedChildren) const -> void override;

private:
    TAttribute<FCk_PanBox_View> _View;
    TAttribute<TOptional<FVector2D>> _ExplicitSurfaceSize;
};

// --------------------------------------------------------------------------------------------------------------------
