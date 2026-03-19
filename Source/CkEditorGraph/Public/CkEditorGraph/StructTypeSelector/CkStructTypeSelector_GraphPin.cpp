#include "CkStructTypeSelector_GraphPin.h"

#include "CkEditorGraph/StructTypeSelector/CkStructTypeSelector_Widget.h"
#include "CkEditorGraph/StructTypeSelector/CkStructTypeSelector_K2NodeHelpers.h"

#include "CkCore/StructTypeSelector/CkStructTypeSelector.h"
#include "CkCore/Validation/CkIsValid.h"

#include <EdGraphSchema_K2.h>
#include <ScopedTransaction.h>

// --------------------------------------------------------------------------------------------------------------------

auto ck::layout::SStructTypeSelector_GraphPin::Construct(
    const FArguments& InArgs,
    UEdGraphPin* InPin) -> void
{
    SGraphPin::Construct(SGraphPin::FArguments(), InPin);
}

// --------------------------------------------------------------------------------------------------------------------

auto ck::layout::SStructTypeSelector_GraphPin::GetDefaultValueWidget() -> TSharedRef<SWidget>
{
    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SStructTypeSelector_Widget)
            .CurrentStruct(this, &SStructTypeSelector_GraphPin::Get_CurrentStruct)
            .OnStructSelected(this, &SStructTypeSelector_GraphPin::OnStructSelected)
            .BaseStructFilter(Get_BaseStructFilter())
        ];
}

// --------------------------------------------------------------------------------------------------------------------

auto ck::layout::SStructTypeSelector_GraphPin::OnStructSelected(
    UScriptStruct* InSelectedStruct) -> void
{
    if (ck::Is_NOT_Valid(GraphPinObj, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    const auto* StructType = FCk_StructTypeSelector::StaticStruct();

    auto SelectorValue = FCk_StructTypeSelector{};
    SelectorValue.Set_StructType(InSelectedStruct);

    FString ExportedText;
    StructType->ExportText(ExportedText, &SelectorValue, nullptr, nullptr, PPF_None, nullptr);

    const auto* K2Schema = GetDefault<UEdGraphSchema_K2>();

    {
        const FScopedTransaction Transaction(FText::FromString(TEXT("Change Struct Type Selection")));
        GraphPinObj->Modify();
        K2Schema->TrySetDefaultValue(*GraphPinObj, ExportedText);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto ck::layout::SStructTypeSelector_GraphPin::Get_CurrentStruct() const -> UScriptStruct*
{
    if (ck::Is_NOT_Valid(GraphPinObj, ck::IsValid_Policy_NullptrOnly{}))
    { return nullptr; }

    const auto& DefaultValue = GraphPinObj->GetDefaultAsString();
    if (DefaultValue.IsEmpty())
    { return nullptr; }

    const auto* StructType = FCk_StructTypeSelector::StaticStruct();

    auto SelectorValue = FCk_StructTypeSelector{};
    StructType->ImportText(*DefaultValue, &SelectorValue, nullptr, PPF_None, GLog, StructType->GetName());

    return SelectorValue.Get_StructType();
}

// --------------------------------------------------------------------------------------------------------------------

auto ck::layout::SStructTypeSelector_GraphPin::Get_BaseStructFilter() const -> UScriptStruct*
{
    if (ck::Is_NOT_Valid(GraphPinObj, ck::IsValid_Policy_NullptrOnly{}))
    { return nullptr; }

    return ck::FStructTypeSelectorHelpers::GetBaseStructFilter(*GraphPinObj);
}

// --------------------------------------------------------------------------------------------------------------------
