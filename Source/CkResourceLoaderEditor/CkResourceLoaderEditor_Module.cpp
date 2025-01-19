#include "CkResourceLoaderEditor_Module.h"

#include "PropertyEditorModule.h"

#include "CkResourceLoaderEditor/Settings/CkResourceLoaderEditor_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkResourceLoaderEditorModule"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkResourceLoaderEditorModule::
    StartupModule()
    -> void
{
    auto& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomClassLayout
    (
        UCk_ResourceLoaderEditor_ProjectSettings_UE::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&ck::layout::FResourceLoaderEditor_ProjectSettings_Details::MakeInstance)
    );
}

auto
    FCkResourceLoaderEditorModule::
    ShutdownModule()
    -> void
{
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.UnregisterCustomPropertyTypeLayout(UCk_ResourceLoaderEditor_ProjectSettings_UE::StaticClass()->GetFName());
    }
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_MODULE(FCkResourceLoaderEditorModule, CkResourceLoaderEditor)

// --------------------------------------------------------------------------------------------------------------------
