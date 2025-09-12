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
 * Customization for FCk_Fragment_ByteAttribute_ParamsData
 */
class FCk_Fragment_ByteAttribute_ParamsDataCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    // IPropertyTypeCustomization interface
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
    // Value getters
    uint8 GetBaseValue() const;
    uint8 GetMinValue() const;
    uint8 GetMaxValue() const;

    // Value setters
    void OnBaseValueCommitted(uint8 NewValue, ETextCommit::Type CommitType);
    void OnMinValueCommitted(uint8 NewValue, ETextCommit::Type CommitType);
    void OnMaxValueCommitted(uint8 NewValue, ETextCommit::Type CommitType);

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

    // Combo box data
    TArray<TSharedPtr<FString>> MinMaxComboList;
};