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

    // Row 1: Name tag
    StructBuilder.AddProperty(NameHandle.ToSharedRef());

    // Row 2: Min/Current/Max with checkboxes
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
        // Min checkbox
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
        // Min label
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
        // Min value
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
        // Current label
        +SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(4.0f, 0.0f, 2.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("CurrentLabel", "Current:"))
            .Font(IDetailLayoutBuilder::GetDetailFont())
        ]
        // Current/Base value
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
        // Max checkbox
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
        // Max label
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
        // Max value
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
    // Clamp based on active min/max settings
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        uint8 ClampedValue = NewValue;

        // Apply min constraint if enabled
        if (MinMaxMode == 1 || MinMaxMode == 3) // Min or Min&Max
        {
            uint8 MinValue;
            if (MinValueHandle->GetValue(MinValue) == FPropertyAccess::Success)
            {
                ClampedValue = FMath::Max(ClampedValue, MinValue);
            }
        }

        // Apply max constraint if enabled
        if (MinMaxMode == 2 || MinMaxMode == 3) // Max or Min&Max
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

    // If base value is now below min, update it
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

    // If base value is now above max, update it
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
        return (MinMaxMode == 1 || MinMaxMode == 3) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Unchecked;
}

ECheckBoxState FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetMaxCheckState() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        return (MinMaxMode == 2 || MinMaxMode == 3) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Unchecked;
}

void FCk_Fragment_ByteAttribute_ParamsDataCustomization::OnMinCheckStateChanged(ECheckBoxState NewState)
{
    const auto MinChecked = (NewState == ECheckBoxState::Checked);
    const auto MaxChecked = (GetMaxCheckState() == ECheckBoxState::Checked);

    // Update MinMaxMode enum based on checkbox states
    uint8 NewMode = 0; // None
    if (MinChecked && MaxChecked)
    {
        NewMode = 3; // Min and Max
    }
    else if (MinChecked)
    {
        NewMode = 1; // Min only
    }
    else if (MaxChecked)
    {
        NewMode = 2; // Max only
    }

    MinMaxHandle->SetValue(NewMode);

    // Clamp base value if needed
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

    // Update MinMaxMode enum based on checkbox states
    uint8 NewMode = 0; // None
    if (MinChecked && MaxChecked)
    {
        NewMode = 3; // Min and Max
    }
    else if (MinChecked)
    {
        NewMode = 1; // Min only
    }
    else if (MaxChecked)
    {
        NewMode = 2; // Max only
    }

    MinMaxHandle->SetValue(NewMode);

    // Clamp base value if needed
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
        if (MinMaxMode == 1 || MinMaxMode == 3) // Min or Min&Max
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
        if (MinMaxMode == 2 || MinMaxMode == 3) // Max or Min&Max
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
    // Only return slider min if BOTH min and max are enabled
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (MinMaxMode == 3) // Both Min and Max must be enabled for slider
        {
            uint8 MinValue;
            if (MinValueHandle->GetValue(MinValue) == FPropertyAccess::Success)
            {
                return MinValue;
            }
        }
    }
    return TOptional<uint8>(); // No slider
}

TOptional<uint8> FCk_Fragment_ByteAttribute_ParamsDataCustomization::GetBaseValueSliderMax() const
{
    // Only return slider max if BOTH min and max are enabled
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (MinMaxMode == 3) // Both Min and Max must be enabled for slider
        {
            uint8 MaxValue;
            if (MaxValueHandle->GetValue(MaxValue) == FPropertyAccess::Success)
            {
                return MaxValue;
            }
        }
    }
    return TOptional<uint8>(); // No slider
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