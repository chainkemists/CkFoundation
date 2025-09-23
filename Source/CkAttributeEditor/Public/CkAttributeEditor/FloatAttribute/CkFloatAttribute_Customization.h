#pragma once

#include "IPropertyTypeCustomization.h"
#include "Containers/Array.h"
#include "Templates/SharedPointer.h"
#include "Misc/Optional.h"
#include "Widgets/Input/SCheckBox.h"
#include "Styling/SlateColor.h"

class FDetailWidgetRow;
class IDetailChildrenBuilder;
class IPropertyHandle;
class SWidget;
class FString;

/**
 * Customization for FCk_Fragment_FloatAttribute_ParamsData
 */
class FCk_Fragment_FloatAttribute_ParamsDataCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    // IPropertyTypeCustomization interface
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
    // Value getters
    TOptional<float> GetBaseValue() const;
    TOptional<float> GetMinValue() const;
    TOptional<float> GetMaxValue() const;

    // Value setters
    void OnBaseValueCommitted(float NewValue, ETextCommit::Type CommitType);
    void OnMinValueCommitted(float NewValue, ETextCommit::Type CommitType);
    void OnMaxValueCommitted(float NewValue, ETextCommit::Type CommitType);

    // Checkbox state handlers
    ECheckBoxState GetMinCheckState() const;
    ECheckBoxState GetMaxCheckState() const;
    void OnMinCheckStateChanged(ECheckBoxState NewState);
    void OnMaxCheckStateChanged(ECheckBoxState NewState);

    // UI state helpers
    FSlateColor GetMinLabelColor() const;
    FSlateColor GetMaxLabelColor() const;
    bool IsMinValueEnabled() const;
    bool IsMaxValueEnabled() const;

    // Base value constraints
    TOptional<float> GetBaseValueMin() const;
    TOptional<float> GetBaseValueMax() const;
    TOptional<float> GetBaseValueSliderMin() const;
    TOptional<float> GetBaseValueSliderMax() const;

    // Property handles
    TSharedPtr<IPropertyHandle> NameHandle;
    TSharedPtr<IPropertyHandle> BaseValueHandle;
    TSharedPtr<IPropertyHandle> MinMaxHandle;
    TSharedPtr<IPropertyHandle> MinValueHandle;
    TSharedPtr<IPropertyHandle> MaxValueHandle;
    TSharedPtr<IPropertyHandle> RefillParamsHandle;
};

/**
 * Customization for FCk_Fragment_FloatAttributeRefill_ParamsData
 */
class FCk_Fragment_FloatAttributeRefill_ParamsDataCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    // IPropertyTypeCustomization interface
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
    // Property handles
    TSharedPtr<IPropertyHandle> RefillAttributeNameHandle;
    TSharedPtr<IPropertyHandle> RefillBehaviorHandle;
    TSharedPtr<IPropertyHandle> FillRateHandle;
    TSharedPtr<IPropertyHandle> StartingStateHandle;
};