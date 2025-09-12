#pragma once

#include "IPropertyTypeCustomization.h"
#include "Containers/Array.h"
#include "Templates/SharedPointer.h"
#include "Misc/Optional.h"

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

    // Min/Max dropdown
    TSharedRef<SWidget> OnGenerateMinMaxComboWidget(TSharedPtr<FString> InComboString);
    void OnMinMaxSelectionChanged(TSharedPtr<FString> InSelectedItem, ESelectInfo::Type SelectInfo);

    // Visibility helpers
    EVisibility GetMinValueVisibility() const;
    EVisibility GetMaxValueVisibility() const;
    bool IsMinValueEnabled() const;
    bool IsMaxValueEnabled() const;

    // Property handles
    TSharedPtr<IPropertyHandle> NameHandle;
    TSharedPtr<IPropertyHandle> BaseValueHandle;
    TSharedPtr<IPropertyHandle> MinMaxHandle;
    TSharedPtr<IPropertyHandle> MinValueHandle;
    TSharedPtr<IPropertyHandle> MaxValueHandle;
    TSharedPtr<IPropertyHandle> RefillParamsHandle;

    // Combo box data
    TArray<TSharedPtr<FString>> MinMaxComboList;
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