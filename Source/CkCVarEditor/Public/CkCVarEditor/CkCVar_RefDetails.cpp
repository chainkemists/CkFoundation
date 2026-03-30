#include "CkCVar_RefDetails.h"

#include "CkCVarEditor/CkCVar_RefWidget.h"
#include "CkCVar/CkCVar_Data.h"

#include <DetailWidgetRow.h>
#include <IDetailChildrenBuilder.h>
#include <PropertyHandle.h>
#include <Widgets/DeclarativeSyntaxSupport.h>
#include <Widgets/SBoxPanel.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCVarRefCustomization"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    auto
        FCVarRef_Details::
        MakeInstance()
        -> TSharedRef<IPropertyTypeCustomization>
    {
        return MakeShareable(new FCVarRef_Details);
    }

    auto
        FCVarRef_Details::
        CustomizeHeader(
            TSharedRef<IPropertyHandle> InStructPropertyHandle,
            class FDetailWidgetRow& InHeaderRow,
            IPropertyTypeCustomizationUtils& InStructCustomizationUtils)
        -> void
    {
        _NameProperty = InStructPropertyHandle->GetChildHandle(
            GET_MEMBER_NAME_CHECKED(FCk_CVarRef, _Name));

        const auto& FilterMetaStr = InStructPropertyHandle->GetProperty()->GetMetaData(TEXT("FilterMetaTag"));

        auto Value = FName{};
        if (_NameProperty.IsValid())
        {
            _NameProperty->GetValue(Value);
        }

        InHeaderRow.
            NameContent()
            [
                InStructPropertyHandle->CreatePropertyNameWidget()
            ]
        .ValueContent()
            .MinDesiredWidth(500)
            .MaxDesiredWidth(4096)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .HAlign(HAlign_Fill)
                .Padding(0.f, 0.f, 2.f, 0.f)
                [
                    SNew(ck::layout::SCVarRef_Widget)
                    .OnCVarRefChanged(this, &FCVarRef_Details::OnCVarRefChanged)
                    .DefaultName(Value)
                    .FilterMetaData(FilterMetaStr)
                ]
            ];
    }

    auto
        FCVarRef_Details::
        CustomizeChildren(
            TSharedRef<class IPropertyHandle> InStructPropertyHandle,
            class IDetailChildrenBuilder& InStructBuilder,
            IPropertyTypeCustomizationUtils& InStructCustomizationUtils)
        -> void
    {
    }

    auto
        FCVarRef_Details::
        OnCVarRefChanged(
            FName InSelectedName) const
        -> void
    {
        if (_NameProperty.IsValid())
        {
            _NameProperty->SetValue(InSelectedName);
        }
    }
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
