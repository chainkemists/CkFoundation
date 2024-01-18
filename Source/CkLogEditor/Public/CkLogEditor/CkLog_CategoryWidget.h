#pragma once

#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SComboButton.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    class SLogCategory_Widget : public SCompoundWidget
    {
    public:
        DECLARE_DELEGATE_OneParam(FOnLogCategoryChanged, FName)

        SLATE_BEGIN_ARGS(SLogCategory_Widget)
        : _FilterMetaData()
        , _DefaultName()
        {}

        SLATE_ARGUMENT(FString, FilterMetaData)
        SLATE_ARGUMENT(FName, DefaultName)
        SLATE_EVENT(FOnLogCategoryChanged, OnLogCategoryChanged)
        SLATE_END_ARGS()

    public:
        auto Construct(
            const FArguments& InArgs) -> void;

    private:
        auto GenerateLogCategoryPicker() -> TSharedRef<SWidget>;
        auto GetSelectedValueAsText() const -> FText;
        auto OnItemPicked(
            FName InName) -> void;

    private:
        FOnLogCategoryChanged OnLogCategoryChanged;
        FString FilterMetaData;
        FName SelectedName;
        TSharedPtr<class SComboButton> ComboButton;
    };
}

// --------------------------------------------------------------------------------------------------------------------
