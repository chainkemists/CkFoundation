#include "CkMaterialParameter_Customization.h"

#include "CkGraphics/CkGraphics_Common.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"

#include <Widgets/SBoxPanel.h>
#include <Widgets/Input/SCheckBox.h>
#include <Widgets/Text/STextBlock.h>

#define LOCTEXT_NAMESPACE "CkMaterialParameterCustomization"

// ----------------------------------------------------------------------------------------------------------------

TSharedRef<IPropertyTypeCustomization>
    FCk_MaterialParameter_Customization::
    MakeInstance()
{
    return MakeShareable(new FCk_MaterialParameter_Customization);
}

void
    FCk_MaterialParameter_Customization::
    CustomizeHeader(
        TSharedRef<IPropertyHandle> StructPropertyHandle,
        FDetailWidgetRow& HeaderRow,
        IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    const auto OverrideHandle = StructPropertyHandle->GetChildHandle(TEXT("_Override"));
    const auto NameHandle     = StructPropertyHandle->GetChildHandle(TEXT("_ParameterName"));
    const auto TypeHandle     = StructPropertyHandle->GetChildHandle(TEXT("_Type"));

    // Select which value child to show based on the parameter's type (set by discovery).
    auto ValueHandle = TSharedPtr<IPropertyHandle>{};
    if (TypeHandle.IsValid())
    {
        auto TypeRaw = uint8{0};
        if (TypeHandle->GetValue(TypeRaw) == FPropertyAccess::Success)
        {
            switch (static_cast<ECk_Material_ParameterType>(TypeRaw))
            {
                case ECk_Material_ParameterType::Color:   { ValueHandle = StructPropertyHandle->GetChildHandle(TEXT("_ColorValue"));   break; }
                case ECk_Material_ParameterType::Texture: { ValueHandle = StructPropertyHandle->GetChildHandle(TEXT("_TextureValue")); break; }
                default:                                  { ValueHandle = StructPropertyHandle->GetChildHandle(TEXT("_ScalarValue"));  break; }
            }
        }
    }

    if (OverrideHandle.IsValid() == false || NameHandle.IsValid() == false || ValueHandle.IsValid() == false)
    {
        // Defensive fallback — should never happen unless the struct layout changes.
        HeaderRow.NameContent()
        [
            StructPropertyHandle->CreatePropertyNameWidget()
        ];
        return;
    }

    HeaderRow
    .NameContent()
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0.0f, 0.0f, 6.0f, 0.0f)
        [
            SNew(SCheckBox)
            .IsChecked_Lambda([OverrideHandle]() -> ECheckBoxState
            {
                auto IsOverridden = false;
                return OverrideHandle->GetValue(IsOverridden) == FPropertyAccess::Success && IsOverridden
                    ? ECheckBoxState::Checked
                    : ECheckBoxState::Unchecked;
            })
            .OnCheckStateChanged_Lambda([OverrideHandle](ECheckBoxState InNewState)
            {
                OverrideHandle->SetValue(InNewState == ECheckBoxState::Checked);
            })
            .ToolTipText(LOCTEXT("OverrideTooltip",
                "When checked, this parameter overrides the material default on the per-widget dynamic material instance."))
        ]
        + SHorizontalBox::Slot()
        .FillWidth(1.0f)
        .VAlign(VAlign_Center)
        [
            SNew(STextBlock)
            .Font(IDetailLayoutBuilder::GetDetailFont())
            .Text_Lambda([NameHandle]() -> FText
            {
                auto ParameterName = FString{};
                return NameHandle->GetValueAsFormattedString(ParameterName) == FPropertyAccess::Success && ParameterName.IsEmpty() == false
                    ? FText::FromString(ParameterName)
                    : LOCTEXT("UnnamedParameter", "(None)");
            })
        ]
    ]
    .ValueContent()
    .MinDesiredWidth(250.0f)
    [
        ValueHandle->CreatePropertyValueWidget()
    ];
}

void
    FCk_MaterialParameter_Customization::
    CustomizeChildren(
        TSharedRef<IPropertyHandle> StructPropertyHandle,
        IDetailChildrenBuilder& StructBuilder,
        IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    // Intentionally empty — the parameter is shown entirely on the header row as a one-liner.
}

// ----------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
