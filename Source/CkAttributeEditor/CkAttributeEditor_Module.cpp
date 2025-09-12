#include "CkAttributeEditor_Module.h"

#include "CkAttribute/ByteAttribute/CkByteAttribute_Fragment_Data.h"
#include "CkAttribute/Public/CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"

#include "CkAttributeEditor/ByteAttribute/CkByteAttribute_Customization.h"
#include "CkAttributeEditor/FloatAttribute/CkFloatAttribute_Customization.h"

#define LOCTEXT_NAMESPACE "FCkAttributeEditorModule"

// ----------------------------------------------------------------------------------------------------------------

auto
    FCkAttributeEditorModule::
    StartupModule()
    -> void
{
    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    PropertyModule.RegisterCustomPropertyTypeLayout(
        FCk_Fragment_ByteAttribute_ParamsData::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCk_Fragment_ByteAttribute_ParamsDataCustomization::MakeInstance));

    PropertyModule.RegisterCustomPropertyTypeLayout(
        FCk_Fragment_FloatAttribute_ParamsData::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCk_Fragment_FloatAttribute_ParamsDataCustomization::MakeInstance));

    PropertyModule.RegisterCustomPropertyTypeLayout(
        FCk_Fragment_FloatAttributeRefill_ParamsData::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCk_Fragment_FloatAttributeRefill_ParamsDataCustomization::MakeInstance));
}

auto
    FCkAttributeEditorModule::
    ShutdownModule()
    -> void
{
    // Unregister the customizations if the module is loaded
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

        PropertyModule.UnregisterCustomPropertyTypeLayout(FCk_Fragment_ByteAttribute_ParamsData::StaticStruct()->GetFName());
        PropertyModule.UnregisterCustomPropertyTypeLayout(FCk_Fragment_FloatAttribute_ParamsData::StaticStruct()->GetFName());
        PropertyModule.UnregisterCustomPropertyTypeLayout(FCk_Fragment_FloatAttributeRefill_ParamsData::StaticStruct()->GetFName());
    }
}

// ----------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkAttributeEditorModule, CkAttributeEditor)