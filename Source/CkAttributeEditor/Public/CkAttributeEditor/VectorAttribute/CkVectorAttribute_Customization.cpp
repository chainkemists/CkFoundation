#include "CkVectorAttribute_Customization.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/STextBlock.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "VectorAttributeCustomization"

/* FCk_Fragment_VectorAttribute_ParamsDataCustomization
 *****************************************************************************/

TSharedRef<IPropertyTypeCustomization> FCk_Fragment_VectorAttribute_ParamsDataCustomization::MakeInstance()
{
    return MakeShareable(new FCk_Fragment_VectorAttribute_ParamsDataCustomization);
}

void FCk_Fragment_VectorAttribute_ParamsDataCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    NameHandle = StructPropertyHandle->GetChildHandle(TEXT("_Name"));

    HeaderRow.NameContent()
    [
        StructPropertyHandle->CreatePropertyNameWidget()
    ]
    .ValueContent()
    .MinDesiredWidth(250.0f)
    [
        SNew(STextBlock)
        .Text(this, &FCk_Fragment_VectorAttribute_ParamsDataCustomization::GetNameTitleText)
        .Font(IDetailLayoutBuilder::GetDetailFont())
    ];
}

void FCk_Fragment_VectorAttribute_ParamsDataCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    // Get handles to all properties
    NameHandle = StructPropertyHandle->GetChildHandle(TEXT("_Name"));
    TSharedPtr<IPropertyHandle> BaseValueHandle = StructPropertyHandle->GetChildHandle(TEXT("_BaseValue"));
    TSharedPtr<IPropertyHandle> MinMaxHandle = StructPropertyHandle->GetChildHandle(TEXT("_MinMax"));
    TSharedPtr<IPropertyHandle> MinValueHandle = StructPropertyHandle->GetChildHandle(TEXT("_MinValue"));
    TSharedPtr<IPropertyHandle> MaxValueHandle = StructPropertyHandle->GetChildHandle(TEXT("_MaxValue"));

    check(NameHandle.IsValid());
    check(BaseValueHandle.IsValid());
    check(MinMaxHandle.IsValid());
    check(MinValueHandle.IsValid());
    check(MaxValueHandle.IsValid());

    // FVector properties use default widgets since they already have good built-in editing
    StructBuilder.AddProperty(NameHandle.ToSharedRef());
    StructBuilder.AddProperty(BaseValueHandle.ToSharedRef());
    StructBuilder.AddProperty(MinMaxHandle.ToSharedRef());
    StructBuilder.AddProperty(MinValueHandle.ToSharedRef());
    StructBuilder.AddProperty(MaxValueHandle.ToSharedRef());
}

FText FCk_Fragment_VectorAttribute_ParamsDataCustomization::GetNameTitleText() const
{
    if (NameHandle.IsValid())
    {
        FString TagString;
        if (NameHandle->GetValueAsFormattedString(TagString) == FPropertyAccess::Success && !TagString.IsEmpty())
        {
            return FText::FromString(TagString);
        }
    }
    return LOCTEXT("NoName", "(None)");
}

#undef LOCTEXT_NAMESPACE
