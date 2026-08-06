#include "CkAttributeEditor_Module.h"

#include "CkAttribute/ByteAttribute/CkByteAttribute_Fragment_Data.h"
#include "CkAttribute/Public/CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Fragment_Data.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Fragment_Data.h"

#include "CkAttributeEditor/ByteAttribute/CkByteAttribute_Customization.h"
#include "CkAttributeEditor/FloatAttribute/CkFloatAttribute_Customization.h"
#include "CkAttributeEditor/IntegerAttribute/CkIntegerAttribute_Customization.h"
#include "CkAttributeEditor/VectorAttribute/CkVectorAttribute_Customization.h"

#define LOCTEXT_NAMESPACE "FCkAttributeEditorModule"

// ----------------------------------------------------------------------------------------------------

auto
    FCkAttributeEditorModule::
    StartupModule()
    -> void
{
    FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    PropertyModule.RegisterCustomPropertyTypeLayout(
        FCk_ByteAttribute_Spec::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCk_Fragment_ByteAttribute_ParamsDataCustomization::MakeInstance));

    PropertyModule.RegisterCustomPropertyTypeLayout(
        FCk_FloatAttribute_Spec::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCk_Fragment_FloatAttribute_ParamsDataCustomization::MakeInstance));

    PropertyModule.RegisterCustomPropertyTypeLayout(
        FCk_FloatAttributeRefill_Spec::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCk_Fragment_FloatAttributeRefill_ParamsDataCustomization::MakeInstance));

    PropertyModule.RegisterCustomPropertyTypeLayout(
        FCk_IntegerAttribute_Spec::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCk_Fragment_IntegerAttribute_ParamsDataCustomization::MakeInstance));

    PropertyModule.RegisterCustomPropertyTypeLayout(
        FCk_IntegerAttributeRefill_Spec::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCk_Fragment_IntegerAttributeRefill_ParamsDataCustomization::MakeInstance));

    PropertyModule.RegisterCustomPropertyTypeLayout(
        FCk_VectorAttribute_Spec::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FCk_Fragment_VectorAttribute_ParamsDataCustomization::MakeInstance));
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

        PropertyModule.UnregisterCustomPropertyTypeLayout(FCk_ByteAttribute_Spec::StaticStruct()->GetFName());
        PropertyModule.UnregisterCustomPropertyTypeLayout(FCk_FloatAttribute_Spec::StaticStruct()->GetFName());
        PropertyModule.UnregisterCustomPropertyTypeLayout(FCk_FloatAttributeRefill_Spec::StaticStruct()->GetFName());
        PropertyModule.UnregisterCustomPropertyTypeLayout(FCk_IntegerAttribute_Spec::StaticStruct()->GetFName());
        PropertyModule.UnregisterCustomPropertyTypeLayout(FCk_IntegerAttributeRefill_Spec::StaticStruct()->GetFName());
        PropertyModule.UnregisterCustomPropertyTypeLayout(FCk_VectorAttribute_Spec::StaticStruct()->GetFName());
    }
}

// ----------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkAttributeEditorModule, CkAttributeEditor)