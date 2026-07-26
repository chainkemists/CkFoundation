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

class FCk_Fragment_IntegerAttribute_ParamsDataCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
    FText GetNameTitleText() const;

    TOptional<int32> GetBaseValue() const;
    TOptional<int32> GetMinValue() const;
    TOptional<int32> GetMaxValue() const;

    void OnBaseValueCommitted(int32 NewValue, ETextCommit::Type CommitType);
    void OnMinValueCommitted(int32 NewValue, ETextCommit::Type CommitType);
    void OnMaxValueCommitted(int32 NewValue, ETextCommit::Type CommitType);

    ECheckBoxState GetMinCheckState() const;
    ECheckBoxState GetMaxCheckState() const;
    void OnMinCheckStateChanged(ECheckBoxState NewState);
    void OnMaxCheckStateChanged(ECheckBoxState NewState);

    FSlateColor GetMinLabelColor() const;
    FSlateColor GetMaxLabelColor() const;
    bool IsMinValueEnabled() const;
    bool IsMaxValueEnabled() const;

    TOptional<int32> GetBaseValueMin() const;
    TOptional<int32> GetBaseValueMax() const;
    TOptional<int32> GetBaseValueSliderMin() const;
    TOptional<int32> GetBaseValueSliderMax() const;

    TSharedPtr<IPropertyHandle> NameHandle;
    TSharedPtr<IPropertyHandle> BaseValueHandle;
    TSharedPtr<IPropertyHandle> MinMaxHandle;
    TSharedPtr<IPropertyHandle> MinValueHandle;
    TSharedPtr<IPropertyHandle> MaxValueHandle;
    TSharedPtr<IPropertyHandle> RefillParamsHandle;
};

class FCk_Fragment_IntegerAttributeRefill_ParamsDataCustomization : public IPropertyTypeCustomization
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
