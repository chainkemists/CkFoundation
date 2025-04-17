#include "CkEditorStyle_Module.h"

#include "CkEditorStyle/CkEditorStyle_Utils.h"
#include "CkEditorStyle/AssetThumbnail/CkAssetThumbnailRenderer.h"
#include "CkEditorStyle/Settings/CkEditorStyle_Settings.h"

#include <ThumbnailRendering/ThumbnailManager.h>
#include <PropertyEditorModule.h>

#define LOCTEXT_NAMESPACE "FCkEditorStyleModule"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkEditorStyleModule::
    StartupModule()
    -> void
{
    UCk_Utils_EditorStyle_UE::DoRegister_SlateStyle();
    UCk_Utils_EditorStyle_UE::DoReloadTextures();

    auto& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomClassLayout
    (
        UCk_EditorStyle_ProjectSettings_UE::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&ck::layout::FEditorStyle_ProjectSettings_Details::MakeInstance)
    );

    UThumbnailManager::Get().UnregisterCustomRenderer(UBlueprint::StaticClass());
    UThumbnailManager::Get().RegisterCustomRenderer(UBlueprint::StaticClass(), UCk_CustomBlueprintThumbnailRenderer_UE::StaticClass());

    UThumbnailManager::Get().UnregisterCustomRenderer(UDataAsset::StaticClass());
    UThumbnailManager::Get().RegisterCustomRenderer(UDataAsset::StaticClass(), UCk_CustomDataAssetThumbnailRenderer_UE::StaticClass());
}

auto
    FCkEditorStyleModule::
    ShutdownModule()
    -> void
{
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.UnregisterCustomPropertyTypeLayout(UCk_EditorStyle_ProjectSettings_UE::StaticClass()->GetFName());
    }

    UCk_Utils_EditorStyle_UE::DoUnregister_SlateStyle();
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkEditorStyleModule, CkEditorStyle)
