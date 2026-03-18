#include "CkIntegerAttribute_Customization.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "IntegerAttributeCustomization"

/* FCk_Fragment_IntegerAttribute_ParamsDataCustomization
 *****************************************************************************/

TSharedRef<IPropertyTypeCustomization> FCk_Fragment_IntegerAttribute_ParamsDataCustomization::MakeInstance()
{
    return MakeShareable(new FCk_Fragment_IntegerAttribute_ParamsDataCustomization);
}

void FCk_Fragment_IntegerAttribute_ParamsDataCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
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
        .Text(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetNameTitleText)
        .Font(IDetailLayoutBuilder::GetDetailFont())
    ];
}

void FCk_Fragment_IntegerAttribute_ParamsDataCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    // Get handles to all properties
    NameHandle = StructPropertyHandle->GetChildHandle(TEXT("_Name"));
    BaseValueHandle = StructPropertyHandle->GetChildHandle(TEXT("_BaseValue"));
    MinMaxHandle = StructPropertyHandle->GetChildHandle(TEXT("_MinMax"));
    MinValueHandle = StructPropertyHandle->GetChildHandle(TEXT("_MinValue"));
    MaxValueHandle = StructPropertyHandle->GetChildHandle(TEXT("_MaxValue"));
    RefillParamsHandle = StructPropertyHandle->GetChildHandle(TEXT("_RefillParams"));

    check(NameHandle.IsValid());
    check(BaseValueHandle.IsValid());
    check(MinMaxHandle.IsValid());
    check(MinValueHandle.IsValid());
    check(MaxValueHandle.IsValid());
    check(RefillParamsHandle.IsValid());

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
            .IsChecked(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetMinCheckState)
            .OnCheckStateChanged(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::OnMinCheckStateChanged)
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
            .ColorAndOpacity(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetMinLabelColor)
        ]
        // Min value
        +SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
        [
            SNew(SBox)
            .MinDesiredWidth(60.0f)
            [
                SNew(SNumericEntryBox<int32>)
                .Value(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetMinValue)
                .OnValueCommitted(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::OnMinValueCommitted)
                .Font(IDetailLayoutBuilder::GetDetailFont())
                .IsEnabled(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::IsMinValueEnabled)
                .AllowSpin(false)
                .MinSliderValue(TOptional<int32>())
                .MaxSliderValue(TOptional<int32>())
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
                SNew(SNumericEntryBox<int32>)
                .Value(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetBaseValue)
                .OnValueCommitted(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::OnBaseValueCommitted)
                .Font(IDetailLayoutBuilder::GetDetailFont())
                .AllowSpin(true)
                .MinValue(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetBaseValueMin)
                .MaxValue(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetBaseValueMax)
                .MinSliderValue(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetBaseValueSliderMin)
                .MaxSliderValue(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetBaseValueSliderMax)
                .SliderExponent(1.0f)
            ]
        ]
        // Max checkbox
        +SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(4.0f, 0.0f, 4.0f, 0.0f)
        [
            SNew(SCheckBox)
            .IsChecked(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetMaxCheckState)
            .OnCheckStateChanged(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::OnMaxCheckStateChanged)
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
            .ColorAndOpacity(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetMaxLabelColor)
        ]
        // Max value
        +SHorizontalBox::Slot()
        .FillWidth(1.0f)
        [
            SNew(SBox)
            .MinDesiredWidth(60.0f)
            [
                SNew(SNumericEntryBox<int32>)
                .Value(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetMaxValue)
                .OnValueCommitted(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::OnMaxValueCommitted)
                .Font(IDetailLayoutBuilder::GetDetailFont())
                .IsEnabled(this, &FCk_Fragment_IntegerAttribute_ParamsDataCustomization::IsMaxValueEnabled)
                .AllowSpin(false)
                .MinSliderValue(TOptional<int32>())
                .MaxSliderValue(TOptional<int32>())
            ]
        ]
    ];

    // Row 3: Refill Parameters (with inline checkbox due to InlineEditConditionToggle)
    StructBuilder.AddProperty(RefillParamsHandle.ToSharedRef());
}

TOptional<int32> FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetBaseValue() const
{
    int32 Value;
    if (BaseValueHandle->GetValue(Value) == FPropertyAccess::Success)
    {
        return Value;
    }
    return TOptional<int32>();
}

TOptional<int32> FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetMinValue() const
{
    int32 Value;
    if (MinValueHandle->GetValue(Value) == FPropertyAccess::Success)
    {
        return Value;
    }
    return TOptional<int32>();
}

TOptional<int32> FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetMaxValue() const
{
    int32 Value;
    if (MaxValueHandle->GetValue(Value) == FPropertyAccess::Success)
    {
        return Value;
    }
    return TOptional<int32>();
}

void FCk_Fragment_IntegerAttribute_ParamsDataCustomization::OnBaseValueCommitted(int32 NewValue, ETextCommit::Type CommitType)
{
    // Clamp based on active min/max settings
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        int32 ClampedValue = NewValue;

        // Apply min constraint if enabled
        if (MinMaxMode == 1 || MinMaxMode == 3) // Min or Min&Max
        {
            int32 MinValue;
            if (MinValueHandle->GetValue(MinValue) == FPropertyAccess::Success)
            {
                ClampedValue = FMath::Max(ClampedValue, MinValue);
            }
        }

        // Apply max constraint if enabled
        if (MinMaxMode == 2 || MinMaxMode == 3) // Max or Min&Max
        {
            int32 MaxValue;
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

void FCk_Fragment_IntegerAttribute_ParamsDataCustomization::OnMinValueCommitted(int32 NewValue, ETextCommit::Type CommitType)
{
    MinValueHandle->SetValue(NewValue);

    // If base value is now below min, update it
    int32 BaseValue;
    if (BaseValueHandle->GetValue(BaseValue) == FPropertyAccess::Success)
    {
        if (BaseValue < NewValue)
        {
            BaseValueHandle->SetValue(NewValue);
        }
    }
}

void FCk_Fragment_IntegerAttribute_ParamsDataCustomization::OnMaxValueCommitted(int32 NewValue, ETextCommit::Type CommitType)
{
    MaxValueHandle->SetValue(NewValue);

    // If base value is now above max, update it
    int32 BaseValue;
    if (BaseValueHandle->GetValue(BaseValue) == FPropertyAccess::Success)
    {
        if (BaseValue > NewValue)
        {
            BaseValueHandle->SetValue(NewValue);
        }
    }
}

ECheckBoxState FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetMinCheckState() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        return (MinMaxMode == 1 || MinMaxMode == 3) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Unchecked;
}

ECheckBoxState FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetMaxCheckState() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        return (MinMaxMode == 2 || MinMaxMode == 3) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Unchecked;
}

void FCk_Fragment_IntegerAttribute_ParamsDataCustomization::OnMinCheckStateChanged(ECheckBoxState NewState)
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
        int32 BaseValue, MinValue;
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

void FCk_Fragment_IntegerAttribute_ParamsDataCustomization::OnMaxCheckStateChanged(ECheckBoxState NewState)
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
        int32 BaseValue, MaxValue;
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

FSlateColor FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetMinLabelColor() const
{
    return IsMinValueEnabled() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground();
}

FSlateColor FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetMaxLabelColor() const
{
    return IsMaxValueEnabled() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground();
}

TOptional<int32> FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetBaseValueMin() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (MinMaxMode == 1 || MinMaxMode == 3) // Min or Min&Max
        {
            int32 MinValue;
            if (MinValueHandle->GetValue(MinValue) == FPropertyAccess::Success)
            {
                return MinValue;
            }
        }
    }
    return TOptional<int32>();
}

TOptional<int32> FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetBaseValueMax() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (MinMaxMode == 2 || MinMaxMode == 3) // Max or Min&Max
        {
            int32 MaxValue;
            if (MaxValueHandle->GetValue(MaxValue) == FPropertyAccess::Success)
            {
                return MaxValue;
            }
        }
    }
    return TOptional<int32>();
}

TOptional<int32> FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetBaseValueSliderMin() const
{
    // Only return slider min if BOTH min and max are enabled
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (MinMaxMode == 3) // Both Min and Max must be enabled for slider
        {
            int32 MinValue;
            if (MinValueHandle->GetValue(MinValue) == FPropertyAccess::Success)
            {
                return MinValue;
            }
        }
    }
    return TOptional<int32>(); // No slider
}

TOptional<int32> FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetBaseValueSliderMax() const
{
    // Only return slider max if BOTH min and max are enabled
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (MinMaxMode == 3) // Both Min and Max must be enabled for slider
        {
            int32 MaxValue;
            if (MaxValueHandle->GetValue(MaxValue) == FPropertyAccess::Success)
            {
                return MaxValue;
            }
        }
    }
    return TOptional<int32>(); // No slider
}

bool FCk_Fragment_IntegerAttribute_ParamsDataCustomization::IsMinValueEnabled() const
{
    uint8 MinMaxValue;
    if (MinMaxHandle->GetValue(MinMaxValue) == FPropertyAccess::Success)
    {
        return (MinMaxValue == 1 || MinMaxValue == 3);
    }
    return false;
}

bool FCk_Fragment_IntegerAttribute_ParamsDataCustomization::IsMaxValueEnabled() const
{
    uint8 MinMaxValue;
    if (MinMaxHandle->GetValue(MinMaxValue) == FPropertyAccess::Success)
    {
        return (MinMaxValue == 2 || MinMaxValue == 3);
    }
    return false;
}

FText FCk_Fragment_IntegerAttribute_ParamsDataCustomization::GetNameTitleText() const
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

/* FCk_Fragment_IntegerAttributeRefill_ParamsDataCustomization
 *****************************************************************************/

TSharedRef<IPropertyTypeCustomization> FCk_Fragment_IntegerAttributeRefill_ParamsDataCustomization::MakeInstance()
{
    return MakeShareable(new FCk_Fragment_IntegerAttributeRefill_ParamsDataCustomization);
}

void FCk_Fragment_IntegerAttributeRefill_ParamsDataCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    // Just show the struct name in the header
    HeaderRow.NameContent()
    [
        StructPropertyHandle->CreatePropertyNameWidget()
    ];
}

void FCk_Fragment_IntegerAttributeRefill_ParamsDataCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    // Get handles to all properties
    RefillAttributeNameHandle = StructPropertyHandle->GetChildHandle(TEXT("_RefillAttributeName"));
    RefillBehaviorHandle = StructPropertyHandle->GetChildHandle(TEXT("_RefillBehavior"));
    FillRateHandle = StructPropertyHandle->GetChildHandle(TEXT("_FillRate"));
    StartingStateHandle = StructPropertyHandle->GetChildHandle(TEXT("_StartingState"));

    check(RefillAttributeNameHandle.IsValid());
    check(RefillBehaviorHandle.IsValid());
    check(FillRateHandle.IsValid());
    check(StartingStateHandle.IsValid());

    // Row 1: Refill Attribute Name
    StructBuilder.AddProperty(RefillAttributeNameHandle.ToSharedRef());

    // Row 2: Refill Behavior and Fill Rate in one row
    StructBuilder.AddCustomRow(LOCTEXT("RefillSettingsRow", "Refill Settings"))
    .NameContent()
    [
        SNew(STextBlock)
        .Text(LOCTEXT("RefillSettingsLabel", "Refill Settings"))
        .Font(IDetailLayoutBuilder::GetDetailFont())
    ]
    .ValueContent()
    .MinDesiredWidth(250.0f)
    [
        SNew(SHorizontalBox)
        // Refill Behavior
        +SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 4.0f, 0.0f)
        [
            RefillBehaviorHandle->CreatePropertyValueWidget()
        ]
        // Fill Rate
        +SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(4.0f, 0.0f, 0.0f, 0.0f)
        [
            SNew(SHorizontalBox)
            +SHorizontalBox::Slot()
            .AutoWidth()
            .VAlign(VAlign_Center)
            .Padding(0.0f, 0.0f, 4.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("FillRateLabel", "Rate:"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
            ]
            +SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                FillRateHandle->CreatePropertyValueWidget()
            ]
        ]
    ];

    // Row 3: Starting State
    StructBuilder.AddProperty(StartingStateHandle.ToSharedRef());
}

#undef LOCTEXT_NAMESPACE
