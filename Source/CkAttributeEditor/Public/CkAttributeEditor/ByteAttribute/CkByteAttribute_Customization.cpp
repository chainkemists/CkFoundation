#include "CkByteAttribute_Customization.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "ByteAttributeCustomization"

/* FCk_Fragment_ByteAttribute_ParamsDataCustomization
 *****************************************************************************/

TSharedRef<IPropertyTypeCustomization> FCk_Fragment_ByteAttribute_ParamsDataCustomization::MakeInstance()
{
    return MakeShareable(new FCk_Fragment_ByteAttribute_ParamsDataCustomization);
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    // Just show the struct name in the header
    HeaderRow.NameContent()
    [
        StructPropertyHandle->CreatePropertyNameWidget()
    ];
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    // Get handles to all properties
    NameHandle = StructPropertyHandle->GetChildHandle(TEXT("_Name"));
    BaseValueHandle = StructPropertyHandle->GetChildHandle(TEXT("_BaseValue"));
    MinMaxHandle = StructPropertyHandle->GetChildHandle(TEXT("_MinMax"));
    MinValueHandle = StructPropertyHandle->GetChildHandle(TEXT("_MinValue"));
    MaxValueHandle = StructPropertyHandle->GetChildHandle(TEXT("_MaxValue"));

    check(NameHandle.IsValid());
    check(BaseValueHandle.IsValid());
    check(MinMaxHandle.IsValid());
    check(MinValueHandle.IsValid());
    check(MaxValueHandle.IsValid());

    // Build the MinMax combo list
    MinMaxComboList.Empty();
    MinMaxComboList.Add(MakeShareable(new FString(TEXT("None"))));
    MinMaxComboList.Add(MakeShareable(new FString(TEXT("Min"))));
    MinMaxComboList.Add(MakeShareable(new FString(TEXT("Max"))));
    MinMaxComboList.Add(MakeShareable(new FString(TEXT("Min and Max"))));

    // Get initial selection for MinMax combo
    uint8 MinMaxValue;
    TSharedPtr<FString> SelectedMinMax;
    if (MinMaxHandle->GetValue(MinMaxValue) == FPropertyAccess::Success)
    {
        SelectedMinMax = MinMaxComboList[MinMaxValue];
    }

    // Row 1: Name tag
    StructBuilder.AddProperty(NameHandle.ToSharedRef());

    // Row 2: Base value with Min/Max dropdown and conditional min/max fields
    StructBuilder.AddCustomRow(LOCTEXT("BaseValueRow", "Base Value"))
    .NameContent()
    [
        SNew(STextBlock)
        .Text(LOCTEXT("BaseValueLabel", "Base Value"))
        .Font(IDetailLayoutBuilder::GetDetailFont())
    ]
    .ValueContent()
    .MinDesiredWidth(250.0f)
    .MaxDesiredWidth(400.0f)
    [
        SNew(SHorizontalBox)
        // Min value (conditional)
        +SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0.0f, 0.0f, 4.0f, 0.0f)
        [
            SNew(SBox)
            .WidthOverride(60.0f)
            .Visibility(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMinValueVisibility)
            [
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(0.0f, 0.0f, 2.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("MinLabel", "Min:"))
                    .Font(IDetailLayoutBuilder::GetDetailFontItalic())
                ]
                +SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(SSpinBox<uint8>)
                    .Value(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMinValue)
                    .OnValueCommitted(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMinValueCommitted)
                    .Font(IDetailLayoutBuilder::GetDetailFont())
                    .MinValue(0)
                    .MaxValue(255)
                ]
            ]
        ]
        // Base value
        +SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 4.0f, 0.0f)
        [
            SNew(SSpinBox<uint8>)
            .Value(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetBaseValue)
            .OnValueCommitted(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnBaseValueCommitted)
            .Font(IDetailLayoutBuilder::GetDetailFont())
            .MinValue(0)
            .MaxValue(255)
        ]
        // Max value (conditional)
        +SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(0.0f, 0.0f, 4.0f, 0.0f)
        [
            SNew(SBox)
            .WidthOverride(60.0f)
            .Visibility(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMaxValueVisibility)
            [
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(0.0f, 0.0f, 2.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("MaxLabel", "Max:"))
                    .Font(IDetailLayoutBuilder::GetDetailFontItalic())
                ]
                +SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(SSpinBox<uint8>)
                    .Value(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMaxValue)
                    .OnValueCommitted(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMaxValueCommitted)
                    .Font(IDetailLayoutBuilder::GetDetailFont())
                    .MinValue(0)
                    .MaxValue(255)
                ]
            ]
        ]
        // MinMax dropdown
        +SHorizontalBox::Slot()
        .AutoWidth()
        [
            SNew(SComboBox<TSharedPtr<FString>>)
            .OptionsSource(&MinMaxComboList)
            .OnGenerateWidget(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnGenerateMinMaxComboWidget)
            .OnSelectionChanged(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMinMaxSelectionChanged)
            .InitiallySelectedItem(SelectedMinMax)
            [
                SNew(STextBlock)
                .Text_Lambda([this]() -> FText
                {
                    uint8 Value;
                    if (MinMaxHandle->GetValue(Value) == FPropertyAccess::Success)
                    {
                        switch(Value)
                        {
                            case 0: return LOCTEXT("None", "None");
                            case 1: return LOCTEXT("Min", "Min");
                            case 2: return LOCTEXT("Max", "Max");
                            case 3: return LOCTEXT("MinMax", "Min and Max");
                        }
                    }
                    return LOCTEXT("None", "None");
                })
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
        ]
    ];
}

uint8 FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetBaseValue() const
{
    uint8 Value;
    if (BaseValueHandle->GetValue(Value) == FPropertyAccess::Success)
    {
        return Value;
    }
    return {};
}

uint8 FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMinValue() const
{
    uint8 Value;
    if (MinValueHandle->GetValue(Value) == FPropertyAccess::Success)
    {
        return Value;
    }
    return {};
}

uint8 FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMaxValue() const
{
    uint8 Value;
    if (MaxValueHandle->GetValue(Value) == FPropertyAccess::Success)
    {
        return Value;
    }
    return {};
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnBaseValueCommitted(uint8 NewValue, ETextCommit::Type CommitType)
{
    BaseValueHandle->SetValue(NewValue);
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMinValueCommitted(uint8 NewValue, ETextCommit::Type CommitType)
{
    MinValueHandle->SetValue(NewValue);
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMaxValueCommitted(uint8 NewValue, ETextCommit::Type CommitType)
{
    MaxValueHandle->SetValue(NewValue);
}

TSharedRef<SWidget> FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnGenerateMinMaxComboWidget(TSharedPtr<FString> InComboString)
{
    return SNew(SBox)
        .WidthOverride(100.0f)
        [
            SNew(STextBlock)
            .Text(FText::FromString(*InComboString))
            .Font(IDetailLayoutBuilder::GetDetailFont())
        ];
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMinMaxSelectionChanged(TSharedPtr<FString> InSelectedItem, ESelectInfo::Type SelectInfo)
{
    int32 Index = MinMaxComboList.IndexOfByKey(InSelectedItem);
    if (Index >= 0)
    {
        MinMaxHandle->SetValue(static_cast<uint8>(Index));
    }
}

EVisibility FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMinValueVisibility() const
{
    uint8 MinMaxValue;
    if (MinMaxHandle->GetValue(MinMaxValue) == FPropertyAccess::Success)
    {
        // Show if Min (1) or MinMax (3)
        if (MinMaxValue == 1 || MinMaxValue == 3)
        {
            return EVisibility::Visible;
        }
    }
    return EVisibility::Collapsed;
}

EVisibility FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMaxValueVisibility() const
{
    uint8 MinMaxValue;
    if (MinMaxHandle->GetValue(MinMaxValue) == FPropertyAccess::Success)
    {
        // Show if Max (2) or MinMax (3)
        if (MinMaxValue == 2 || MinMaxValue == 3)
        {
            return EVisibility::Visible;
        }
    }
    return EVisibility::Collapsed;
}

bool FCk_Fragment_ByteAttribute_ParamsDataCustomization::IsMinValueEnabled() const
{
    uint8 MinMaxValue;
    if (MinMaxHandle->GetValue(MinMaxValue) == FPropertyAccess::Success)
    {
        return (MinMaxValue == 1 || MinMaxValue == 3);
    }
    return false;
}

bool FCk_Fragment_ByteAttribute_ParamsDataCustomization::IsMaxValueEnabled() const
{
    uint8 MinMaxValue;
    if (MinMaxHandle->GetValue(MinMaxValue) == FPropertyAccess::Success)
    {
        return (MinMaxValue == 2 || MinMaxValue == 3);
    }
    return false;
}

#undef LOCTEXT_NAMESPACE