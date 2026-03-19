#pragma once

#include <SGraphPin.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    class CKEDITORGRAPH_API SStructTypeSelector_GraphPin : public SGraphPin
    {
    public:
        SLATE_BEGIN_ARGS(SStructTypeSelector_GraphPin) {}
        SLATE_END_ARGS()

        auto Construct(const FArguments& InArgs, UEdGraphPin* InPin) -> void;

    protected:
        auto GetDefaultValueWidget() -> TSharedRef<SWidget> override;

    private:
        auto OnStructSelected(UScriptStruct* InSelectedStruct) -> void;
        auto Get_CurrentStruct() const -> UScriptStruct*;
        auto Get_BaseStructFilter() const -> UScriptStruct*;
    };
}

// --------------------------------------------------------------------------------------------------------------------
