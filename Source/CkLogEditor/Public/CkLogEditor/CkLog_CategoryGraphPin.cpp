#include "CkLog_CategoryGraphPin.h"

#include "CkLog_CategoryWidget.h"

#include "CkLog/CkLog_Category.h"

#include <ScopedTransaction.h>
#include <UObject/UObjectIterator.h>
#include <Widgets/SBoxPanel.h>

#define LOCTEXT_NAMESPACE "SLogCategoryGraphPin"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    auto
        SLogCategory_GraphPin::
        Construct(
            const FArguments& InArgs,
            UEdGraphPin* InGraphPinObj)
        -> void
    {
        SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
        _LastSelectedName = FName{};
    }

    auto
        SLogCategory_GraphPin::
        GetDefaultValueWidget()
        -> TSharedRef<SWidget>
    {
        // Parse out current default value
        const auto& DefaultString = GraphPinObj->GetDefaultAsString();
        FCk_LogCategory DefaultLogCategory;

        UScriptStruct* PinLiteralStructType = FCk_LogCategory::StaticStruct();
        if (DefaultString.IsEmpty() == false)
        {
            PinLiteralStructType->ImportText
            (
                *DefaultString,
                &DefaultLogCategory,
                nullptr,
                PPF_SerializedAsImportText,
                GError,
                PinLiteralStructType->GetName(),
                true
            );
        }

        return SNew(SVerticalBox)
            +SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(ck::layout::SLogCategory_Widget)
                .OnLogCategoryChanged(this, &SLogCategory_GraphPin::OnLogCategoryChanged)
                .DefaultName(DefaultLogCategory.Get_Name())
                .Visibility(this, &SGraphPin::GetDefaultValueVisibility)
                .IsEnabled(this, &SLogCategory_GraphPin::Get_DefaultValueIsEnabled)
            ];
    }

    //--------------------------------------------------------------------------------------------------------------------------
    auto
        SLogCategory_GraphPin::
        OnLogCategoryChanged(
            FName SelectedName)
        -> void
    {
        FString FinalValue;
        const auto NewLogCategory = FCk_LogCategory{SelectedName};

        FCk_LogCategory::StaticStruct()->ExportText
        (
            FinalValue,
            &NewLogCategory,
            &NewLogCategory,
            nullptr,
            PPF_SerializedAsImportText,
            nullptr
        );

        if (FinalValue != GraphPinObj->GetDefaultAsString())
        {
            const FScopedTransaction Transaction(NSLOCTEXT("GraphEditor", "ChangePinValue", "Change Pin Value"));
            GraphPinObj->Modify();
            GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, FinalValue);
        }

        _LastSelectedName = SelectedName;
    }

    auto
        SLogCategory_GraphPin::
        Get_DefaultValueIsEnabled() const
        -> bool
    {
        return GraphPinObj->bDefaultValueIsReadOnly == false;
    }
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
