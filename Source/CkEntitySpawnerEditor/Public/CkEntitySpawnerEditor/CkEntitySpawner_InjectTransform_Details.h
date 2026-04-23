#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <IPropertyTypeCustomization.h>

// --------------------------------------------------------------------------------------------------------------------

class IPropertyHandle;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    // Dropdown picker for FCk_EntitySpawner_ScriptPropertyBinding. Lists FTransform UPROPERTY
    // fields discovered on the sibling _EntityScript via reflection.
    class FCk_EntitySpawner_InjectTransform_Details : public IPropertyTypeCustomization
    {
    public:
        CK_GENERATED_BODY(FCk_EntitySpawner_InjectTransform_Details);

    public:
        static auto MakeInstance() -> TSharedRef<IPropertyTypeCustomization>;

    public:
        auto CustomizeHeader(
            TSharedRef<IPropertyHandle> InStructPropertyHandle,
            class FDetailWidgetRow& InHeaderRow,
            IPropertyTypeCustomizationUtils& InStructCustomizationUtils) -> void override;

        auto CustomizeChildren(
            TSharedRef<IPropertyHandle> InStructPropertyHandle,
            class IDetailChildrenBuilder& InStructBuilder,
            IPropertyTypeCustomizationUtils& InStructCustomizationUtils) -> void override;

    private:
        auto MakeMenu() const -> TSharedRef<class SWidget>;
        auto GetButtonLabel() const -> FText;
        auto CollectCandidateProperties() const -> TArray<TTuple<FName, FText>>;

        auto ResolveOwningEntityScriptClass() const -> const UClass*;

    private:
        TSharedPtr<IPropertyHandle> _PropertyNameHandle;
        TSharedPtr<IPropertyHandle> _OuterBindingHandle;
    };
}

// --------------------------------------------------------------------------------------------------------------------
