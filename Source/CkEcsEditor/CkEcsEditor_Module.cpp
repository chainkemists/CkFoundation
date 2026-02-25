#include "CkEcsEditor_Module.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkEcsEditor/CkEcsEditor_Log.h"
#include "CkEcsEditor/EntityScript/CkEntityScript_K2Node.h"
#include "CkEcsEditor/SpawnParamsToolbox/CkEntityScriptSpawnParamsToolbox.h"

#include "CkEditorStyle/CkEditorStyle_Utils.h"

#include <WorkspaceMenuStructure.h>
#include <WorkspaceMenuStructureModule.h>

#define LOCTEXT_NAMESPACE "FCkEcsEditorModule"

auto
    FCkEcsEditorModule::
    StartupModule()
    -> void
{
    DoRegisterTabSpawner();

    // EntityScript
    const auto& EntityScript_Name = UCk_EntityScript_UE::StaticClass()->GetFName();

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetIcon, EntityScript_Name}
        .Set_IconFileName(TEXT("Icon_EntityScript_16px"))
        .Set_IconBrushType(ECk_CustomIconBrushType::PNG)
        .Set_IconSize(ECk_IconSize::IconSize_16x16));

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetThumbnail, EntityScript_Name}
        .Set_IconFileName(TEXT("Icon_EntityScript_64px"))
        .Set_IconBrushType(ECk_CustomIconBrushType::PNG)
        .Set_IconSize(ECk_IconSize::IconSize_64x64));

    // EntityConstructionScript
    const auto& EntityConstructionScript_Name = UCk_Entity_ConstructionScript_PDA::StaticClass()->GetFName();

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetIcon, EntityConstructionScript_Name}
        .Set_IconFileName(TEXT("Icon_EntityConstructionScript_16px"))
        .Set_IconBrushType(ECk_CustomIconBrushType::PNG)
        .Set_IconSize(ECk_IconSize::IconSize_16x16));

    UCk_Utils_EditorStyle_UE::Register_CustomSlateStyle(FCk_CustomAssetStyle_Params{
        ECk_AssetStyleType::AssetThumbnail, EntityConstructionScript_Name}
        .Set_IconFileName(TEXT("Icon_EntityConstructionScript_64px"))
        .Set_IconBrushType(ECk_CustomIconBrushType::PNG)
        .Set_IconSize(ECk_IconSize::IconSize_64x64));

    // Register details customization
    auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    // Try both possible class name formats
    PropertyModule.RegisterCustomClassLayout(
        UCk_K2Node_EntityScript::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FCk_EntityScriptNode_DetailsCustomization::MakeInstance)
    );

    PropertyModule.RegisterCustomClassLayout(
        TEXT("Ck_K2Node_EntityScript"),
        FOnGetDetailCustomizationInstance::CreateStatic(&FCk_EntityScriptNode_DetailsCustomization::MakeInstance)
    );
}

auto
    FCkEcsEditorModule::
    ShutdownModule()
    -> void
{
    DoUnregisterTabSpawner();

    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        auto& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.UnregisterCustomClassLayout(UCk_K2Node_EntityScript::StaticClass()->GetFName());
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkEcsEditorModule::
    DoRegisterTabSpawner()
    -> void
{
    auto& GlobalTabManager = FGlobalTabmanager::Get();

    GlobalTabManager->RegisterNomadTabSpawner(
        SpawnParamsToolbox_TabName,
        FOnSpawnTab::CreateRaw(this, &FCkEcsEditorModule::DoOnSpawnTab)
    )
    .SetDisplayName(FText::FromString(SpawnParamsToolbox_TabDisplayName))
    .SetTooltipText(FText::FromString(TEXT("Discover, validate, and repair EntityScript SpawnParams structs")))
    .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsDebugCategory());
}

auto
    FCkEcsEditorModule::
    DoUnregisterTabSpawner()
    -> void
{
    if (FSlateApplication::IsInitialized())
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(SpawnParamsToolbox_TabName);
    }
}

auto
    FCkEcsEditorModule::
    DoOnSpawnTab(const FSpawnTabArgs& Args)
    -> TSharedRef<SDockTab>
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        .Label(FText::FromString(SpawnParamsToolbox_TabDisplayName))
        .ToolTipText(FText::FromString(TEXT("EntityScript SpawnParams Toolbox - Discover, validate, and repair")))
        [
            SNew(SCkEntityScriptSpawnParamsToolbox)
        ];
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkEcsEditorModule, CkEcsEditor)
