#include "CkFloatAttribute_Customization.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "FloatAttributeCustomization"

// ----------------------------------------------------------------------------------------------------

namespace ck_float_attribute_customization
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

TSharedRef<IPropertyTypeCustomization> FCk_Fragment_FloatAttribute_ParamsDataCustomization::MakeInstance()
{
    return MakeShareable(new FCk_Fragment_FloatAttribute_ParamsDataCustomization);
}

void FCk_Fragment_FloatAttribute_ParamsDataCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
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
        .Text(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetNameTitleText)
        .Font(IDetailLayoutBuilder::GetDetailFont())
    ];
}

void FCk_Fragment_FloatAttribute_ParamsDataCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
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
            .IsChecked(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetMinCheckState)
            .OnCheckStateChanged(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::OnMinCheckStateChanged)
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
            .ColorAndOpacity(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetMinLabelColor)
        ]
        +SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
        [
            SNew(SBox)
            .MinDesiredWidth(60.0f)
            [
                SNew(SNumericEntryBox<float>)
                .Value(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetMinValue)
                .OnValueCommitted(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::OnMinValueCommitted)
                .Font(IDetailLayoutBuilder::GetDetailFont())
                .IsEnabled(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::IsMinValueEnabled)
                .AllowSpin(false)
                .MinSliderValue(TOptional<float>())
                .MaxSliderValue(TOptional<float>())
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
                SNew(SNumericEntryBox<float>)
                .Value(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetBaseValue)
                .OnValueCommitted(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::OnBaseValueCommitted)
                .Font(IDetailLayoutBuilder::GetDetailFont())
                .AllowSpin(true)
                .MinValue(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetBaseValueMin)
                .MaxValue(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetBaseValueMax)
                .MinSliderValue(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetBaseValueSliderMin)
                .MaxSliderValue(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetBaseValueSliderMax)
                .SliderExponent(1.0f)
            ]
        ]
        +SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(4.0f, 0.0f, 4.0f, 0.0f)
        [
            SNew(SCheckBox)
            .IsChecked(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetMaxCheckState)
            .OnCheckStateChanged(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::OnMaxCheckStateChanged)
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
            .ColorAndOpacity(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetMaxLabelColor)
        ]
        +SHorizontalBox::Slot()
        .FillWidth(1.0f)
        [
            SNew(SBox)
            .MinDesiredWidth(60.0f)
            [
                SNew(SNumericEntryBox<float>)
                .Value(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetMaxValue)
                .OnValueCommitted(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::OnMaxValueCommitted)
                .Font(IDetailLayoutBuilder::GetDetailFont())
                .IsEnabled(this, &FCk_Fragment_FloatAttribute_ParamsDataCustomization::IsMaxValueEnabled)
                .AllowSpin(false)
                .MinSliderValue(TOptional<float>())
                .MaxSliderValue(TOptional<float>())
            ]
        ]
    ];

    StructBuilder.AddProperty(RefillParamsHandle.ToSharedRef());
}

TOptional<float> FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetBaseValue() const
{
    float Value;
    if (BaseValueHandle->GetValue(Value) == FPropertyAccess::Success)
    {
        return Value;
    }
    return TOptional<float>();
}

TOptional<float> FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetMinValue() const
{
    float Value;
    if (MinValueHandle->GetValue(Value) == FPropertyAccess::Success)
    {
        return Value;
    }
    return TOptional<float>();
}

TOptional<float> FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetMaxValue() const
{
    float Value;
    if (MaxValueHandle->GetValue(Value) == FPropertyAccess::Success)
    {
        return Value;
    }
    return TOptional<float>();
}

void FCk_Fragment_FloatAttribute_ParamsDataCustomization::OnBaseValueCommitted(float NewValue, ETextCommit::Type CommitType)
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        float ClampedValue = NewValue;

        if (ck_float_attribute_customization::Get_HasMin(MinMaxMode))
        {
            float MinValue;
            if (MinValueHandle->GetValue(MinValue) == FPropertyAccess::Success)
            {
                ClampedValue = FMath::Max(ClampedValue, MinValue);
            }
        }

        if (ck_float_attribute_customization::Get_HasMax(MinMaxMode))
        {
            float MaxValue;
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

void FCk_Fragment_FloatAttribute_ParamsDataCustomization::OnMinValueCommitted(float NewValue, ETextCommit::Type CommitType)
{
    MinValueHandle->SetValue(NewValue);

    float BaseValue;
    if (BaseValueHandle->GetValue(BaseValue) == FPropertyAccess::Success)
    {
        if (BaseValue < NewValue)
        {
            BaseValueHandle->SetValue(NewValue);
        }
    }
}

void FCk_Fragment_FloatAttribute_ParamsDataCustomization::OnMaxValueCommitted(float NewValue, ETextCommit::Type CommitType)
{
    MaxValueHandle->SetValue(NewValue);

    float BaseValue;
    if (BaseValueHandle->GetValue(BaseValue) == FPropertyAccess::Success)
    {
        if (BaseValue > NewValue)
        {
            BaseValueHandle->SetValue(NewValue);
        }
    }
}

ECheckBoxState FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetMinCheckState() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        return ck_float_attribute_customization::Get_HasMin(MinMaxMode) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Unchecked;
}

ECheckBoxState FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetMaxCheckState() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        return ck_float_attribute_customization::Get_HasMax(MinMaxMode) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
    }
    return ECheckBoxState::Unchecked;
}

void FCk_Fragment_FloatAttribute_ParamsDataCustomization::OnMinCheckStateChanged(ECheckBoxState NewState)
{
    const auto MinChecked = (NewState == ECheckBoxState::Checked);
    const auto MaxChecked = (GetMaxCheckState() == ECheckBoxState::Checked);

    uint8 NewMode = ck_float_attribute_customization::MinMaxMode_None;
    if (MinChecked && MaxChecked)
    {
        NewMode = ck_float_attribute_customization::MinMaxMode_MinMax;
    }
    else if (MinChecked)
    {
        NewMode = ck_float_attribute_customization::MinMaxMode_Min;
    }
    else if (MaxChecked)
    {
        NewMode = ck_float_attribute_customization::MinMaxMode_Max;
    }

    MinMaxHandle->SetValue(NewMode);

    if (MinChecked)
    {
        float BaseValue, MinValue;
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

void FCk_Fragment_FloatAttribute_ParamsDataCustomization::OnMaxCheckStateChanged(ECheckBoxState NewState)
{
    const auto MinChecked = (GetMinCheckState() == ECheckBoxState::Checked);
    const auto MaxChecked = (NewState == ECheckBoxState::Checked);

    uint8 NewMode = ck_float_attribute_customization::MinMaxMode_None;
    if (MinChecked && MaxChecked)
    {
        NewMode = ck_float_attribute_customization::MinMaxMode_MinMax;
    }
    else if (MinChecked)
    {
        NewMode = ck_float_attribute_customization::MinMaxMode_Min;
    }
    else if (MaxChecked)
    {
        NewMode = ck_float_attribute_customization::MinMaxMode_Max;
    }

    MinMaxHandle->SetValue(NewMode);

    if (MaxChecked)
    {
        float BaseValue, MaxValue;
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

FSlateColor FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetMinLabelColor() const
{
    return IsMinValueEnabled() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground();
}

FSlateColor FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetMaxLabelColor() const
{
    return IsMaxValueEnabled() ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground();
}

TOptional<float> FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetBaseValueMin() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (ck_float_attribute_customization::Get_HasMin(MinMaxMode))
        {
            float MinValue;
            if (MinValueHandle->GetValue(MinValue) == FPropertyAccess::Success)
            {
                return MinValue;
            }
        }
    }
    return TOptional<float>();
}

TOptional<float> FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetBaseValueMax() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (ck_float_attribute_customization::Get_HasMax(MinMaxMode))
        {
            float MaxValue;
            if (MaxValueHandle->GetValue(MaxValue) == FPropertyAccess::Success)
            {
                return MaxValue;
            }
        }
    }
    return TOptional<float>();
}

TOptional<float> FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetBaseValueSliderMin() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (MinMaxMode == ck_float_attribute_customization::MinMaxMode_MinMax)
        {
            float MinValue;
            if (MinValueHandle->GetValue(MinValue) == FPropertyAccess::Success)
            {
                return MinValue;
            }
        }
    }
    return TOptional<float>();
}

TOptional<float> FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetBaseValueSliderMax() const
{
    uint8 MinMaxMode;
    if (MinMaxHandle->GetValue(MinMaxMode) == FPropertyAccess::Success)
    {
        if (MinMaxMode == ck_float_attribute_customization::MinMaxMode_MinMax)
        {
            float MaxValue;
            if (MaxValueHandle->GetValue(MaxValue) == FPropertyAccess::Success)
            {
                return MaxValue;
            }
        }
    }
    return TOptional<float>();
}

bool FCk_Fragment_FloatAttribute_ParamsDataCustomization::IsMinValueEnabled() const
{
    uint8 MinMaxValue;
    if (MinMaxHandle->GetValue(MinMaxValue) == FPropertyAccess::Success)
    {
        return ck_float_attribute_customization::Get_HasMin(MinMaxValue);
    }
    return false;
}

bool FCk_Fragment_FloatAttribute_ParamsDataCustomization::IsMaxValueEnabled() const
{
    uint8 MinMaxValue;
    if (MinMaxHandle->GetValue(MinMaxValue) == FPropertyAccess::Success)
    {
        return ck_float_attribute_customization::Get_HasMax(MinMaxValue);
    }
    return false;
}

FText FCk_Fragment_FloatAttribute_ParamsDataCustomization::GetNameTitleText() const
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

// ----------------------------------------------------------------------------------------------------

TSharedRef<IPropertyTypeCustomization> FCk_Fragment_FloatAttributeRefill_ParamsDataCustomization::MakeInstance()
{
    return MakeShareable(new FCk_Fragment_FloatAttributeRefill_ParamsDataCustomization);
}

void FCk_Fragment_FloatAttributeRefill_ParamsDataCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    HeaderRow.NameContent()
    [
        StructPropertyHandle->CreatePropertyNameWidget()
    ];
}

void FCk_Fragment_FloatAttributeRefill_ParamsDataCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    RefillAttributeNameHandle = StructPropertyHandle->GetChildHandle(TEXT("_RefillAttributeName"));
    RefillBehaviorHandle = StructPropertyHandle->GetChildHandle(TEXT("_RefillBehavior"));
    FillRateHandle = StructPropertyHandle->GetChildHandle(TEXT("_FillRate"));
    StartingStateHandle = StructPropertyHandle->GetChildHandle(TEXT("_StartingState"));

    check(RefillAttributeNameHandle.IsValid());
    check(RefillBehaviorHandle.IsValid());
    check(FillRateHandle.IsValid());
    check(StartingStateHandle.IsValid());

    StructBuilder.AddProperty(RefillAttributeNameHandle.ToSharedRef());

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
        +SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .Padding(0.0f, 0.0f, 4.0f, 0.0f)
        [
            RefillBehaviorHandle->CreatePropertyValueWidget()
        ]
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

    StructBuilder.AddProperty(StartingStateHandle.ToSharedRef());
}

#undef LOCTEXT_NAMESPACE