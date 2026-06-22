#include "CkUIEditor_Module.h"

#include "CkGraphics/CkGraphics_Common.h"
#include "CkUIEditor/MaterialParameter/CkMaterialParameter_Customization.h"

#include <PropertyEditorModule.h>

#define LOCTEXT_NAMESPACE "FCkUIEditorModule"

// ----------------------------------------------------------------------------------------------------------------

auto
    FCkUIEditorModule::
    StartupModule()
    -> void
{
    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    // FCk_Material_Parameter renders as a single [override] Name [value] row; the value editor is chosen
    // from the active-type child property (numeric / color / asset picker).
    PropertyModule.RegisterCustomPropertyTypeLayout(
        FCk_Material_Parameter::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCk_MaterialParameter_Customization::MakeInstance));
}

auto
    FCkUIEditorModule::
    ShutdownModule()
    -> void
{
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

        PropertyModule.UnregisterCustomPropertyTypeLayout(FCk_Material_Parameter::StaticStruct()->GetFName());
    }
}

// ----------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkUIEditorModule, CkUIEditor)
