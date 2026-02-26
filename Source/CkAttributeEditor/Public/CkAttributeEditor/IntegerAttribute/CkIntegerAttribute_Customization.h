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
 * Customization for FCk_Fragment_IntegerAttribute_ParamsData
 */
class FCk_Fragment_IntegerAttribute_ParamsDataCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    // IPropertyTypeCustomization interface
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
    // Title display
    FText GetNameTitleText() const;

    // Value getters
    TOptional<int32> GetBaseValue() const;
    TOptional<int32> GetMinValue() const;
    TOptional<int32> GetMaxValue() const;

    // Value setters
    void OnBaseValueCommitted(int32 NewValue, ETextCommit::Type CommitType);
    void OnMinValueCommitted(int32 NewValue, ETextCommit::Type CommitType);
    void OnMaxValueCommitted(int32 NewValue, ETextCommit::Type CommitType);

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
    TOptional<int32> GetBaseValueMin() const;
    TOptional<int32> GetBaseValueMax() const;
    TOptional<int32> GetBaseValueSliderMin() const;
    TOptional<int32> GetBaseValueSliderMax() const;

    // Property handles
    TSharedPtr<IPropertyHandle> NameHandle;
    TSharedPtr<IPropertyHandle> BaseValueHandle;
    TSharedPtr<IPropertyHandle> MinMaxHandle;
    TSharedPtr<IPropertyHandle> MinValueHandle;
    TSharedPtr<IPropertyHandle> MaxValueHandle;
};
