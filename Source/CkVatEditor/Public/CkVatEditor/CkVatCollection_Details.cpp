#include "CkVatCollection_Details.h"

#include "CkVatEditor/CkVatBaker.h"

#include "CkVat/Collection/CkVatCollection_Data.h"

#include "CkCore/Validation/CkIsValid.h"

#include <DetailCategoryBuilder.h>
#include <DetailLayoutBuilder.h>
#include <DetailWidgetRow.h>
#include <IPropertyUtilities.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Text/STextBlock.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "CkVatCollection_Details"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    auto
        FCkVatCollection_Details::
        MakeInstance()
        -> TSharedRef<IDetailCustomization>
    {
        return MakeShareable(new FCkVatCollection_Details);
    }

    auto
        FCkVatCollection_Details::
        CustomizeDetails(
            IDetailLayoutBuilder& DetailBuilder)
        -> void
    {
        TArray<TWeakObjectPtr<UObject>> CustomizedObjects;
        DetailBuilder.GetObjectsBeingCustomized(CustomizedObjects);

        TArray<TWeakObjectPtr<UCk_VatCollection_Data>> Collections;
        for (const auto& Object : CustomizedObjects)
        {
            if (auto* Collection = Cast<UCk_VatCollection_Data>(Object.Get());
                ck::IsValid(Collection))
            { Collections.Add(Collection); }
        }

        if (Collections.IsEmpty())
        { return; }

        const auto ButtonLabel = [Collections]() -> FText
        {
            for (const auto& Collection : Collections)
            {
                if (ck::IsValid(Collection.Get()) && Collection->Get_IsBakeStale())
                { return LOCTEXT("RebakeStale", "Rebake (inputs changed)"); }
            }
            for (const auto& Collection : Collections)
            {
                if (ck::IsValid(Collection.Get()) && Collection->Get_IsBaked())
                { return LOCTEXT("Rebake", "Rebake"); }
            }
            return LOCTEXT("Bake", "Bake");
        };

        const TWeakPtr<IPropertyUtilities> Utilities = DetailBuilder.GetPropertyUtilities().ToWeakPtr();

        auto& BakeCategory = DetailBuilder.EditCategory("Bake");
        BakeCategory.AddCustomRow(LOCTEXT("BakeRow", "Bake"))
            .ValueContent()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .Padding(5)
                .AutoWidth()
                [
                    SNew(SButton)
                    .Text_Lambda(ButtonLabel)
                    .ToolTipText(LOCTEXT("Bake_Tooltip",
                        "Bake this collection: generates the lookup-UV static mesh + VAT textures as sibling "
                        "assets (overwrite-in-place) and writes the serialized clip table back onto the collection."))
                    .OnClicked_Lambda([Collections, Utilities]() -> FReply
                    {
                        for (const auto& Collection : Collections)
                        {
                            if (auto* Resolved = Collection.Get();
                                ck::IsValid(Resolved))
                            { ck::vat_editor::Bake_VatCollection(*Resolved); }
                        }

                        // The Baked category is VisibleAnywhere data the bake just rewrote — refresh so
                        // the panel shows the new clip table/hash without reselecting the asset.
                        if (const auto PinnedUtilities = Utilities.Pin();
                            PinnedUtilities.IsValid())
                        { PinnedUtilities->ForceRefresh(); }

                        return FReply::Handled();
                    })
                ]
            ];
    }
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
