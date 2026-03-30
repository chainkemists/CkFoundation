#include "CkCVar_RefGraphPin.h"

#include "CkCVarEditor/CkCVar_RefWidget.h"
#include "CkCVar/CkCVar_Data.h"

#include <ScopedTransaction.h>
#include <Widgets/SBoxPanel.h>

#define LOCTEXT_NAMESPACE "SCVarRefGraphPin"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    auto
        SCVarRef_GraphPin::
        Construct(
            const FArguments& InArgs,
            UEdGraphPin* InGraphPinObj)
        -> void
    {
        SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
        _LastSelectedName = FName{};
    }

    auto
        SCVarRef_GraphPin::
        GetDefaultValueWidget()
        -> TSharedRef<SWidget>
    {
        const auto& DefaultString = GraphPinObj->GetDefaultAsString();
        auto DefaultRef = FCk_CVarRef{};

        auto* PinLiteralStructType = FCk_CVarRef::StaticStruct();
        if (NOT DefaultString.IsEmpty())
        {
            PinLiteralStructType->ImportText
            (
                *DefaultString,
                &DefaultRef,
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
                SNew(ck::layout::SCVarRef_Widget)
                .OnCVarRefChanged(this, &SCVarRef_GraphPin::OnCVarRefChanged)
                .DefaultName(DefaultRef.Get_Name())
                .Visibility(this, &SGraphPin::GetDefaultValueVisibility)
                .IsEnabled(this, &SCVarRef_GraphPin::Get_DefaultValueIsEnabled)
            ];
    }

    auto
        SCVarRef_GraphPin::
        OnCVarRefChanged(
            FName SelectedName)
        -> void
    {
        auto FinalValue = FString{};
        const auto NewRef = FCk_CVarRef{SelectedName};

        FCk_CVarRef::StaticStruct()->ExportText
        (
            FinalValue,
            &NewRef,
            &NewRef,
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
        SCVarRef_GraphPin::
        Get_DefaultValueIsEnabled() const
        -> bool
    {
        return GraphPinObj->bDefaultValueIsReadOnly == false;
    }
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
