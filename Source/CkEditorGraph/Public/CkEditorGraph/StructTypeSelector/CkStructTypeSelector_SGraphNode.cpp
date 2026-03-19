#include "CkStructTypeSelector_SGraphNode.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Styling/AppStyle.h>
#include <UObject/Class.h>
#include <UObject/EnumProperty.h>

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto GetPayloadMode(const TWeakObjectPtr<UEdGraphNode>& InNode) -> ECk_CompactExpanded
    {
        if (NOT InNode.IsValid())
        { return ECk_CompactExpanded::Compact; }

        auto* Prop = CastField<FByteProperty>(InNode->GetClass()->FindPropertyByName(TEXT("_PayloadMode")));
        if (ck::Is_NOT_Valid(Prop, ck::IsValid_Policy_NullptrOnly{}))
        {
            auto* EnumProp = CastField<FEnumProperty>(InNode->GetClass()->FindPropertyByName(TEXT("_PayloadMode")));
            if (ck::Is_NOT_Valid(EnumProp, ck::IsValid_Policy_NullptrOnly{}))
            { return ECk_CompactExpanded::Compact; }

            const auto* ValuePtr = EnumProp->ContainerPtrToValuePtr<uint8>(InNode.Get());
            return static_cast<ECk_CompactExpanded>(*ValuePtr);
        }

        const auto* ValuePtr = Prop->ContainerPtrToValuePtr<uint8>(InNode.Get());
        return static_cast<ECk_CompactExpanded>(*ValuePtr);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto SCk_GraphNode_WithPayloadBanner::Construct(
    const FArguments& InArgs,
    UEdGraphNode* InNode) -> void
{
    _OwningNode = InNode;

    GraphNode = InNode;
    SetCursor(EMouseCursor::CardinalCross);
    UpdateGraphNode();
}

// --------------------------------------------------------------------------------------------------------------------

auto SCk_GraphNode_WithPayloadBanner::CreateBelowPinControls(
    TSharedPtr<SVerticalBox> MainBox) -> void
{
    SGraphNodeK2Default::CreateBelowPinControls(MainBox);

    if (NOT _OwningNode.IsValid())
    { return; }

    MainBox->AddSlot()
    .AutoHeight()
    .Padding(4.0f, 2.0f)
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
        .BorderBackgroundColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
        .Padding(6.0f, 4.0f)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0.0f, 0.0f, 4.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text_Lambda([this]()
                {
                    return GetPayloadMode(_OwningNode) == ECk_CompactExpanded::Compact
                        ? FText::FromString(TEXT("📦"))
                        : FText::FromString(TEXT("📋"));
                })
                .ColorAndOpacity_Lambda([this]()
                {
                    return GetPayloadMode(_OwningNode) == ECk_CompactExpanded::Compact
                        ? FSlateColor(FLinearColor(0.4f, 0.8f, 1.0f))
                        : FSlateColor(FLinearColor(1.0f, 0.8f, 0.4f));
                })
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text_Lambda([this]()
                {
                    return GetPayloadMode(_OwningNode) == ECk_CompactExpanded::Compact
                        ? FText::FromString(TEXT("Compact"))
                        : FText::FromString(TEXT("Expanded"));
                })
                .Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
                .ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
            ]
        ]
    ];
}

// --------------------------------------------------------------------------------------------------------------------
