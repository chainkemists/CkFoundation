#include "CkByteAttribute_Customization.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "ByteAttributeCustomization"

// ----------------------------------------------------------------------------------------------------

namespace ck_byte_attribute_customization
{
    constexpr uint8 MinMaxMode_None   = 0;
    constexpr uint8 MinMaxMode_Min    = 1;
    constexpr uint8 MinMaxMode_Max    = 2;
    constexpr uint8 MinMaxMode_MinMax = 3;

    auto Get_HasMin(uint8 InMinMaxMode) -> bool
    {
        return InMinMaxMode == MinMaxMode_Min || InMinMaxMode == MinMaxMode_MinMax;
    }

    auto Get_HasMax(uint8 InMinMaxMode) -> bool
    {
        return InMinMaxMode == MinMaxMode_Max || InMinMaxMode == MinMaxMode_MinMax;
    }
}

// ----------------------------------------------------------------------------------------------------

TSharedRef<IPropertyTypeCustomization> FCk_Fragment_ByteAttribute_ParamsDataCustomization::MakeInstance()
{
    return MakeShareable(new FCk_Fragment_ByteAttribute_ParamsDataCustomization);
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    NameHandle = StructPropertyHandle->GetChildHandle(TEXT("_Name"));

    HeaderRow.NameContent()
    [
        StructPropertyHandle->CreatePropertyNameWidget()
    ]
    .ValueContent()
    .MinDesiredWidth(250.0f)
    [
        SNew(STextBlock)
        .Text(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetNameTitleText)
        .Font(IDetailLayoutBuilder::GetDetailFont())
    ];
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
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

    StructBuilder.AddProperty(NameHandle.ToSharedRef());

    StructBuilder.AddCustomRow(LOCTEXT("ValueRow", "Value"))
    .NameContent()
    [
        SNew(STextBlock)
        .Text(LOCTEXT("ValueLabel", "Value"))
        .Font(IDetailLayoutBuilder::GetDetailFont())
    ]
    .ValueContent()
    .MinDesiredWidth(350.0f)
    .MaxDesiredWidth(500.0f)
    [
        SNew(SHorizontalBox)
        +SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0.0f, 0.0f, 4.0f, 0.0f)
        [
            SNew(SCheckBox)
            .IsChecked(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMinCheckState)
            .OnCheckStateChanged(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMinCheckStateChanged)
            .ToolTipText(LOCTEXT("MinCheckTooltip", "Enable minimum value constraint"))
        ]
        +SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0.0f, 0.0f, 2.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("MinLabel", "Min:"))
            .Font(IDetailLayoutBuilder::GetDetailFont())
            .ColorAndOpacity(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMinLabelColor)
        ]
        +SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
        [
            SNew(SBox)
            .MinDesiredWidth(60.0f)
            [
                SNew(SSpinBox<uint8>)
                .Value(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMinValue)
                .OnValueCommitted(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMinValueCommitted)
                .Font(IDetailLayoutBuilder::GetDetailFont())
                .IsEnabled(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::IsMinValueEnabled)
                .MinValue(0)
                .MaxValue(255)
            ]
        ]
        +SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(4.0f, 0.0f, 2.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("CurrentLabel", "Current:"))
            .Font(IDetailLayoutBuilder::GetDetailFont())
        ]
        +SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
        [
            SNew(SBox)
            .MinDesiredWidth(60.0f)
            [
                SNew(SSpinBox<uint8>)
                .Value(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetBaseValue)
                .OnValueCommitted(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnBaseValueCommitted)
                .Font(IDetailLayoutBuilder::GetDetailFont())
                .MinValue(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetBaseValueMin)
                .MaxValue(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetBaseValueMax)
                .MinSliderValue(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetBaseValueSliderMin)
                .MaxSliderValue(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetBaseValueSliderMax)
            ]
        ]
        +SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(4.0f, 0.0f, 4.0f, 0.0f)
        [
            SNew(SCheckBox)
            .IsChecked(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMaxCheckState)
            .OnCheckStateChanged(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMaxCheckStateChanged)
            .ToolTipText(LOCTEXT("MaxCheckTooltip", "Enable maximum value constraint"))
        ]
        +SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0.0f, 0.0f, 2.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("MaxLabel", "Max:"))
            .Font(IDetailLayoutBuilder::GetDetailFont())
            .ColorAndOpacity(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMaxLabelColor)
        ]
        +SHorizontalBox::Slot()
        .FillWidth(1.0f)
        [
            SNew(SBox)
            .MinDesiredWidth(60.0f)
            [
                SNew(SSpinBox<uint8>)
                .Value(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMaxValue)
                .OnValueCommitted(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMaxValueCommitted)
                .Font(IDetailLayoutBuilder::GetDetailFont())
                .IsEnabled(this, &FCk_Fragment_ByteAttribute_ParamsDataCustomization::IsMaxValueEnabled)
                .MinValue(0)
                .MaxValue(255)
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
    return 0;
}

uint8 FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMinValue() const
{
    uint8 Value;
    if (MinValueHandle->GetValue(Value) == FPropertyAccess::Success)
    {
        return Value;
    }
    return 0;
}

uint8 FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMaxValue() const
{
    uint8 Value;
    if (MaxValueHandle->GetValue(Value) == FPropertyAccess::Success)
    {
        return Value;
    }
    return 255;
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnBaseValueCommitted(uint8 NewValue, ETextCommit::Type CommitType)
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        uint8 ClampedValue = NewValue;

        if (ck_byte_attribute_customization::Get_HasMin(MinMaxMode))
        {
            uint8 MinValue;
            if (MinValueHandle->GetValue(MinValue) == FPropertyAccess::Success)
            {
                ClampedValue = FMath::Max(ClampedValue, MinValue);
            }
        }

        if (ck_byte_attribute_customization::Get_HasMax(MinMaxMode))
        {
            uint8 MaxValue;
            if (MaxValueHandle->GetValue(MaxValue) == FPropertyAccess::Success)
            {
                ClampedValue = FMath::Min(ClampedValue, MaxValue);
            }
        }

        BaseValueHandle->SetValue(ClampedValue);
    }
    else
    {
        BaseValueHandle->SetValue(NewValue);
    }
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMinValueCommitted(uint8 NewValue, ETextCommit::Type CommitType)
{
    MinValueHandle->SetValue(NewValue);

    uint8 BaseValue;
    if (BaseValueHandle->GetValue(BaseValue) == FPropertyAccess::Success)
    {
        if (BaseValue < NewValue)
        {
            BaseValueHandle->SetValue(NewValue);
        }
    }
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMaxValueCommitted(uint8 NewValue, ETextCommit::Type CommitType)
{
    MaxValueHandle->SetValue(NewValue);

    uint8 BaseValue;
    if (BaseValueHandle->GetValue(BaseValue) == FPropertyAccess::Success)
    {
        if (BaseValue > NewValue)
        {
            BaseValueHandle->SetValue(NewValue);
        }
    }
}

ECheckBoxState FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMinCheckState() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        return ck_byte_attribute_customization::Get_HasMin(MinMaxMode) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Unchecked;
}

ECheckBoxState FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMaxCheckState() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        return ck_byte_attribute_customization::Get_HasMax(MinMaxMode) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Unchecked;
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMinCheckStateChanged(ECheckBoxState NewState)
{
    const auto MinChecked = (NewState == ECheckBoxState::Checked);
    const auto MaxChecked = (GetMaxCheckState() == ECheckBoxState::Checked);

    uint8 NewMode = ck_byte_attribute_customization::MinMaxMode_None;
    if (MinChecked && MaxChecked)
    {
        NewMode = ck_byte_attribute_customization::MinMaxMode_MinMax;
    }
    else if (MinChecked)
    {
        NewMode = ck_byte_attribute_customization::MinMaxMode_Min;
    }
    else if (MaxChecked)
    {
        NewMode = ck_byte_attribute_customization::MinMaxMode_Max;
    }

    MinMaxHandle->SetValue(NewMode);

    if (MinChecked)
    {
        uint8 BaseValue, MinValue;
        if (BaseValueHandle->GetValue(BaseValue) == FPropertyAccess::Success &&
            MinValueHandle->GetValue(MinValue) == FPropertyAccess::Success)
        {
            if (BaseValue < MinValue)
            {
                BaseValueHandle->SetValue(MinValue);
            }
        }
    }
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMaxCheckStateChanged(ECheckBoxState NewState)
{
    const auto MinChecked = (GetMinCheckState() == ECheckBoxState::Checked);
    const auto MaxChecked = (NewState == ECheckBoxState::Checked);

    uint8 NewMode = ck_byte_attribute_customization::MinMaxMode_None;
    if (MinChecked && MaxChecked)
    {
        NewMode = ck_byte_attribute_customization::MinMaxMode_MinMax;
    }
    else if (MinChecked)
    {
        NewMode = ck_byte_attribute_customization::MinMaxMode_Min;
    }
    else if (MaxChecked)
    {
        NewMode = ck_byte_attribute_customization::MinMaxMode_Max;
    }

    MinMaxHandle->SetValue(NewMode);

    if (MaxChecked)
    {
        uint8 BaseValue, MaxValue;
        if (BaseValueHandle->GetValue(BaseValue) == FPropertyAccess::Success &&
            MaxValueHandle->GetValue(MaxValue) == FPropertyAccess::Success)
        {
            if (BaseValue > MaxValue)
            {
                BaseValueHandle->SetValue(MaxValue);
            }
        }
    }
}

FSlateColor FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMinLabelColor() const
{
    return IsMinValueEnabled() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground();
}

FSlateColor FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMaxLabelColor() const
{
    return IsMaxValueEnabled() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground();
}

TOptional<uint8> FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetBaseValueMin() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (ck_byte_attribute_customization::Get_HasMin(MinMaxMode))
        {
            uint8 MinValue;
            if (MinValueHandle->GetValue(MinValue) == FPropertyAccess::Success)
            {
                return MinValue;
            }
        }
    }
    return TOptional<uint8>();
}

TOptional<uint8> FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetBaseValueMax() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (ck_byte_attribute_customization::Get_HasMax(MinMaxMode))
        {
            uint8 MaxValue;
            if (MaxValueHandle->GetValue(MaxValue) == FPropertyAccess::Success)
            {
                return MaxValue;
            }
        }
    }
    return TOptional<uint8>();
}

TOptional<uint8> FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetBaseValueSliderMin() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (MinMaxMode == ck_byte_attribute_customization::MinMaxMode_MinMax)
        {
            uint8 MinValue;
            if (MinValueHandle->GetValue(MinValue) == FPropertyAccess::Success)
            {
                return MinValue;
            }
        }
    }
    return TOptional<uint8>();
}

TOptional<uint8> FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetBaseValueSliderMax() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (MinMaxMode == ck_byte_attribute_customization::MinMaxMode_MinMax)
        {
            uint8 MaxValue;
            if (MaxValueHandle->GetValue(MaxValue) == FPropertyAccess::Success)
            {
                return MaxValue;
            }
        }
    }
    return TOptional<uint8>();
}

bool FCk_Fragment_ByteAttribute_ParamsDataCustomization::IsMinValueEnabled() const
{
    uint8 MinMaxValue;
    if (MinMaxHandle->GetValue(MinMaxValue) == FPropertyAccess::Success)
    {
        return ck_byte_attribute_customization::Get_HasMin(MinMaxValue);
    }
    return false;
}

bool FCk_Fragment_ByteAttribute_ParamsDataCustomization::IsMaxValueEnabled() const
{
    uint8 MinMaxValue;
    if (MinMaxHandle->GetValue(MinMaxValue) == FPropertyAccess::Success)
    {
        return ck_byte_attribute_customization::Get_HasMax(MinMaxValue);
    }
    return false;
}

FText FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetNameTitleText() const
{
    if (NameHandle.IsValid())
    {
        FString TagString;
        if (NameHandle->GetValueAsFormattedString(TagString) == FPropertyAccess::Success && !TagString.IsEmpty())
        {
            return FText::FromString(TagString);
        }
    }
    return LOCTEXT("NoName", "(None)");
}

#undef LOCTEXT_NAMESPACE