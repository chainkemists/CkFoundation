#include "CkResourceLoaderEditor.h"

#include "PropertyEditorModule.h"

#include "CkResourceLoaderEditor/Settings/CkResourceLoaderEditor_Settings.h"

#define LOCTEXT_NAMESPACE "FCkResourceLoaderEditorModule"

auto
    FCkResourceLoaderEditorModule::
    StartupModule()
    -> void
{
    auto& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomClassLayout
    (
        UCk_ResourceLoaderEditor_ProjectSettings_UE::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&ck::details::FResourceLoader_ProjectSettings_Details::MakeInstance)
    );
}

auto
    FCkResourceLoaderEditorModule::
    ShutdownModule()
    -> void
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkResourceLoaderEditorModule, CkResourceLoaderEditor)
