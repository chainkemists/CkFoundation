#pragma once

#include "CkCore/Public/CkCore/Macros/CkMacros.h"

#include <IPropertyTypeCustomization.h>
#include <Widgets/Input/SComboBox.h>

// --------------------------------------------------------------------------------------------------------------------

class IDetailGroup;
class IPropertyHandle;
class SWidget;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    class FLogCategory_Details : public IPropertyTypeCustomization
    {
    public:
        CK_GENERATED_BODY(FLogCategory_Details);

    public:
        static auto MakeInstance() -> TSharedRef<IPropertyTypeCustomization>;

    public:
        auto CustomizeHeader(
            TSharedRef<IPropertyHandle> InStructPropertyHandle,
            class FDetailWidgetRow& InHeaderRow,
            IPropertyTypeCustomizationUtils& InStructCustomizationUtils) -> void override;

        auto CustomizeChildren(
            TSharedRef<class IPropertyHandle> InStructPropertyHandle,
            class IDetailChildrenBuilder& InStructBuilder,
            IPropertyTypeCustomizationUtils& InStructCustomizationUtils) -> void override;

    private:
        auto OnLogCategoryChanged(
            FName InSelectedName) const -> void;

    private:
        TSharedPtr<IPropertyHandle> _NameProperty;
    };
}

// --------------------------------------------------------------------------------------------------------------------
