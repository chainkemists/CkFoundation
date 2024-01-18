#include "CkLog_CategoryDetails.h"

#include "CkLog_CategoryWidget.h"
#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkLog/CkLog_Category.h"
#include "CkLog/CkLog_Utils.h"

#include <DetailWidgetRow.h>
#include <IDetailChildrenBuilder.h>
#include <IDetailGroup.h>
#include <Misc/AssertionMacros.h>
#include <PropertyHandle.h>
#include <SlotBase.h>
#include <Widgets/DeclarativeSyntaxSupport.h>
#include <Widgets/SBoxPanel.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FLogCategoryCustomization"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    auto
        FLogCategory_Details::
        MakeInstance()
        -> TSharedRef<IPropertyTypeCustomization>
    {
        return MakeShareable(new FLogCategory_Details);
    }

    auto
        FLogCategory_Details::
        CustomizeHeader(
            TSharedRef<IPropertyHandle> InStructPropertyHandle,
            class FDetailWidgetRow& InHeaderRow,
            IPropertyTypeCustomizationUtils& InStructCustomizationUtils)
        -> void
    {
        _NameProperty = InStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCk_LogCategory, _Name));

        const auto& FilterMetaStr = InStructPropertyHandle->GetProperty()->GetMetaData(TEXT("FilterMetaTag"));

        FName Value;
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
                    SNew(ck::layout::SLogCategory_Widget)
                    .OnLogCategoryChanged(this, &FLogCategory_Details::OnLogCategoryChanged)
                    .DefaultName(Value)
                    .FilterMetaData(FilterMetaStr)
                ]
            ];
    }

    auto
        FLogCategory_Details::
        CustomizeChildren(
            TSharedRef<class IPropertyHandle> InStructPropertyHandle,
            class IDetailChildrenBuilder& InStructBuilder,
            IPropertyTypeCustomizationUtils& InStructCustomizationUtils)
        -> void
    {
    }

    auto
        FLogCategory_Details::
        OnLogCategoryChanged(
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
