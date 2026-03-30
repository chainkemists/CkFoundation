#pragma once

#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SComboButton.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    class SCVarRef_Widget : public SCompoundWidget
    {
    public:
        DECLARE_DELEGATE_OneParam(FOnCVarRefChanged, FName)

        SLATE_BEGIN_ARGS(SCVarRef_Widget)
        : _FilterMetaData()
        , _DefaultName()
        {}

        SLATE_ARGUMENT(FString, FilterMetaData)
        SLATE_ARGUMENT(FName, DefaultName)
        SLATE_EVENT(FOnCVarRefChanged, OnCVarRefChanged)
        SLATE_END_ARGS()

    public:
        auto Construct(
            const FArguments& InArgs) -> void;

    private:
        auto GenerateCVarPicker() -> TSharedRef<SWidget>;
        auto GetSelectedValueAsText() const -> FText;
        auto OnItemPicked(
            const FString& InName) -> void;

    private:
        FOnCVarRefChanged OnCVarRefChanged;
        FString FilterMetaData;
        FName SelectedName;
        TSharedPtr<class SComboButton> ComboButton;
    };
}

// --------------------------------------------------------------------------------------------------------------------
