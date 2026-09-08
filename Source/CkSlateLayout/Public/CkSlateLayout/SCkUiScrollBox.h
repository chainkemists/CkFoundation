#pragma once

#include "CoreMinimal.h"
#include "Widgets/Layout/SScrollBox.h"

class FArrangedChildren;
class FCkUiView;
namespace ck_ui_scrollbox { class SCkUiMeasuredContent; }

/**
 * A retained vertical scroll box which constrains its authored child to the
 * native scroll panel's actual cross-axis width before that panel arranges it.
 */
class CKSLATELAYOUT_API SCkUiScrollBox final : public SScrollBox
{
public:
    SLATE_BEGIN_ARGS(SCkUiScrollBox)
    {
    }
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    virtual void Tick(const FGeometry& InAllottedGeometry, double InCurrentTime, float InDeltaTime) override;
    virtual void OnArrangeChildren(const FGeometry& InAllottedGeometry, FArrangedChildren& OutArrangedChildren) const override;

private:
    friend class FCkUiView;

    /** Commit-only replacement for an already validated staged child. */
    void SetAuthoredContent(TSharedRef<SWidget> InContent, FMargin InPadding);

    TSharedPtr<ck_ui_scrollbox::SCkUiMeasuredContent> _MeasuredContent;
    FMargin _ContentPadding;
    mutable bool _IsResolvingScrollPanelGeometry = false;
};
