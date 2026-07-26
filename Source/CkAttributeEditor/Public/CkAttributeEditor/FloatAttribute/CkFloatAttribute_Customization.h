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

class FCk_Fragment_FloatAttribute_ParamsDataCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
    FText GetNameTitleText() const;

    TOptional<float> GetBaseValue() const;
    TOptional<float> GetMinValue() const;
    TOptional<float> GetMaxValue() const;

    void OnBaseValueCommitted(float NewValue, ETextCommit::Type CommitType);
    void OnMinValueCommitted(float NewValue, ETextCommit::Type CommitType);
    void OnMaxValueCommitted(float NewValue, ETextCommit::Type CommitType);

    ECheckBoxState GetMinCheckState() const;
    ECheckBoxState GetMaxCheckState() const;
    void OnMinCheckStateChanged(ECheckBoxState NewState);
    void OnMaxCheckStateChanged(ECheckBoxState NewState);

    FSlateColor GetMinLabelColor() const;
    FSlateColor GetMaxLabelColor() const;
    bool IsMinValueEnabled() const;
    bool IsMaxValueEnabled() const;

    TOptional<float> GetBaseValueMin() const;
    TOptional<float> GetBaseValueMax() const;
    TOptional<float> GetBaseValueSliderMin() const;
    TOptional<float> GetBaseValueSliderMax() const;

    TSharedPtr<IPropertyHandle> NameHandle;
    TSharedPtr<IPropertyHandle> BaseValueHandle;
    TSharedPtr<IPropertyHandle> MinMaxHandle;
    TSharedPtr<IPropertyHandle> MinValueHandle;
    TSharedPtr<IPropertyHandle> MaxValueHandle;
    TSharedPtr<IPropertyHandle> RefillParamsHandle;
};

class FCk_Fragment_FloatAttributeRefill_ParamsDataCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
    TSharedPtr<IPropertyHandle> RefillAttributeNameHandle;
    TSharedPtr<IPropertyHandle> RefillBehaviorHandle;
    TSharedPtr<IPropertyHandle> FillRateHandle;
    TSharedPtr<IPropertyHandle> StartingStateHandle;
};