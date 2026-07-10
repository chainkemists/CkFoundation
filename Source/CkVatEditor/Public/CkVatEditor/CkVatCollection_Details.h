#pragma once

#include "CoreMinimal.h"

#include <IDetailCustomization.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    // Details customization for UCk_VatCollection_Data: a Bake/Rebake button at the top of the Bake
    // category (the button relabels itself when Get_IsBakeStale flags the serialized bake as stale).
    // The bake itself is ck::vat_editor::Bake_VatCollection — this is UX only.
    class FCkVatCollection_Details : public IDetailCustomization
    {
    public:
        static auto
        MakeInstance() -> TSharedRef<IDetailCustomization>;

        auto
        CustomizeDetails(
            IDetailLayoutBuilder& DetailBuilder) -> void override;
    };
}

// --------------------------------------------------------------------------------------------------------------------
