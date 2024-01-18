#pragma once

#include <CoreMinimal.h>
#include <Widgets/DeclarativeSyntaxSupport.h>
#include <Widgets/SWidget.h>
#include <SGraphPin.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    class SLogCategory_GraphPin : public SGraphPin
    {
    public:
        SLATE_BEGIN_ARGS(SLogCategory_GraphPin) {}
        SLATE_END_ARGS()

    public:
        auto Construct(
            const FArguments& InArgs,
            UEdGraphPin* InGraphPinObj) -> void;

        auto GetDefaultValueWidget() -> TSharedRef<SWidget> override;

        auto OnLogCategoryChanged(
            FName InSelectedName) -> void;

    private:
        FName _LastSelectedName;

    private:
        auto Get_DefaultValueIsEnabled() const -> bool;
    };
}

// --------------------------------------------------------------------------------------------------------------------
