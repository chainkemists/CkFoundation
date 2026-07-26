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

class FCk_Fragment_ByteAttribute_ParamsDataCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
    FText GetNameTitleText() const;

    uint8 GetBaseValue() const;
    uint8 GetMinValue() const;
    uint8 GetMaxValue() const;

    void OnBaseValueCommitted(uint8 NewValue, ETextCommit::Type CommitType);
    void OnMinValueCommitted(uint8 NewValue, ETextCommit::Type CommitType);
    void OnMaxValueCommitted(uint8 NewValue, ETextCommit::Type CommitType);

    ECheckBoxState GetMinCheckState() const;
    ECheckBoxState GetMaxCheckState() const;
    void OnMinCheckStateChanged(ECheckBoxState NewState);
    void OnMaxCheckStateChanged(ECheckBoxState NewState);

    FSlateColor GetMinLabelColor() const;
    FSlateColor GetMaxLabelColor() const;
    bool IsMinValueEnabled() const;
    bool IsMaxValueEnabled() const;

    TOptional<uint8> GetBaseValueMin() const;
    TOptional<uint8> GetBaseValueMax() const;
    TOptional<uint8> GetBaseValueSliderMin() const;
    TOptional<uint8> GetBaseValueSliderMax() const;

    TSharedPtr<IPropertyHandle> NameHandle;
    TSharedPtr<IPropertyHandle> BaseValueHandle;
    TSharedPtr<IPropertyHandle> MinMaxHandle;
    TSharedPtr<IPropertyHandle> MinValueHandle;
    TSharedPtr<IPropertyHandle> MaxValueHandle;
};