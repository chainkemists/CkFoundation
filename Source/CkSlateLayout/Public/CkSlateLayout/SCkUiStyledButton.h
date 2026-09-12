#pragma once

#include "CkSlateLayout/CkUiDocument.h"

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Input/SButton.h"

/**
 * Foundation-owned authored button whose FButtonStyle and state brushes remain
 * alive for the complete native button lifetime.
 */
class CKSLATELAYOUT_API SCkUiStyledButton final : public SButton
{
public:
    SLATE_BEGIN_ARGS(SCkUiStyledButton)
        : _VisualStyle()
        , _IsEnabled(true)
        , _ToolTipText(FText::GetEmpty())
        , _Tag(NAME_None)
    {}
        SLATE_ARGUMENT(FCkUiButtonVisualStyle, VisualStyle)
        SLATE_ATTRIBUTE(bool, IsEnabled)
        SLATE_ATTRIBUTE(FText, ToolTipText)
        SLATE_EVENT(FOnClicked, OnClicked)
        SLATE_ARGUMENT(FName, Tag)
        SLATE_DEFAULT_SLOT(FArguments, Content)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    static auto MakeButtonStyle(const FCkUiButtonVisualStyle& InVisualStyle) -> FButtonStyle;

    FButtonStyle _ButtonStyle;
};
