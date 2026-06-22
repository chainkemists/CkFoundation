#pragma once

#include "IPropertyTypeCustomization.h"

#include <Templates/SharedPointer.h>

// ----------------------------------------------------------------------------------------------------------------

class FDetailWidgetRow;
class IDetailChildrenBuilder;
class IPropertyHandle;
class IPropertyTypeCustomizationUtils;

// ----------------------------------------------------------------------------------------------------------------

/**
 * One-line details row for FCk_Material_Parameter:
 *
 *     [override checkbox]  ParameterName            [value editor]
 *
 * The value editor is produced by the active-type child handle's CreatePropertyValueWidget() — a numeric
 * box, color block, or asset picker selected by `_Type`. The `_Type` dropdown itself is hidden (discovery
 * owns it), and the parameter name is read-only.
 */
class FCk_MaterialParameter_Customization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    // IPropertyTypeCustomization interface
    virtual void CustomizeHeader(
        TSharedRef<IPropertyHandle> StructPropertyHandle,
        FDetailWidgetRow& HeaderRow,
        IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

    virtual void CustomizeChildren(
        TSharedRef<IPropertyHandle> StructPropertyHandle,
        IDetailChildrenBuilder& StructBuilder,
        IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
};

// ----------------------------------------------------------------------------------------------------------------
