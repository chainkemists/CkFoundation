#pragma once

#include "CkCore/Macros/CkMacros.h"

// -----------------------------------------------------------------------------------------

namespace ck::widgets
{
    class SEditorStyle_AssetClassPicker: public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SEditorStyle_AssetClassPicker){}
        SLATE_END_ARGS()

        CK_GENERATED_BODY(SEditorStyle_AssetClassPicker);

    public:
        auto Construct(
            const FArguments& InArgs) -> void;

        auto ConfigureProperties() -> bool;

        auto OnClassPicked(
            UClass* ChosenClass) -> void;

        auto MakeParentClassPicker() -> void;

        auto OnOkClicked() -> FReply;

        auto OnCancelClicked() -> FReply;

        auto CloseDialog(
            bool InWasPicked = false) -> void;

        auto OnKeyDown(
            const FGeometry& MyGeometry,
            const FKeyEvent& InKeyEvent) -> FReply override;

    private:
        auto DoGet_Brush(
            FName PropertyName) const -> const FSlateBrush*;

        auto DoGet_Margin(
            FName PropertyName) const -> const FMargin&;

        auto DoGet_Float(
            FName PropertyName) const -> float;

    private:
        TWeakPtr<SWindow> _PickerWindow;
        TSharedPtr<SVerticalBox> _ParentClassContainer;
        TWeakObjectPtr<UClass> _ClassPicked;

        bool _IsOkClicked = false;

    public:
        CK_PROPERTY_GET(_ClassPicked);
    };
}
// --------------------------------------------------------------------------------------------------------------------
