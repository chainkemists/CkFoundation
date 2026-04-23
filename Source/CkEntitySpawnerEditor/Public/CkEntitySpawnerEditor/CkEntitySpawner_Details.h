#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <IDetailCustomization.h>

// --------------------------------------------------------------------------------------------------------------------

class IDetailLayoutBuilder;
class IPropertyHandle;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    // Disables the bound script property's row on the inline _EntityScript view so it's visually
    // obvious the value is spawner-driven.
    class FCk_EntitySpawner_Details : public IDetailCustomization
    {
    public:
        CK_GENERATED_BODY(FCk_EntitySpawner_Details);

    public:
        static auto MakeInstance() -> TSharedRef<IDetailCustomization>;

    public:
        auto CustomizeDetails(
            IDetailLayoutBuilder& DetailBuilder) -> void override;
    };
}

// --------------------------------------------------------------------------------------------------------------------
