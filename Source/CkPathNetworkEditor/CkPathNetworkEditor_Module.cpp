#include "CkPathNetworkEditor_Module.h"

#include "CkPathNetworkEditor/CkPathNetwork_Details.h"
#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerEdMode.h"
#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerLauncher.h"

#include "CkPathNetwork/Actor/CkPathNetwork_Actor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include <ToolMenus.h>

#define LOCTEXT_NAMESPACE "FCkPathNetworkEditorModule"

// --------------------------------------------------------------------------------------------------------------------

void FCkPathNetworkEditorModule::StartupModule()
{
    // UE discovers visible UEdMode CDOs after editor modules load. Referencing the class here keeps
    // the mode linked in unity/non-unity builds without a second registration mechanism.
    UCk_PathNetworkDesigner_EdMode::StaticClass();

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(
            this,
            &FCkPathNetworkEditorModule::DoRegisterToolsMenu));

	auto& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyEditor.RegisterCustomClassLayout(
		ACk_PathNetwork_UE::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(
			&ck::layout::FCk_PathNetwork_Details::MakeInstance));
}

void FCkPathNetworkEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		auto& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditor.UnregisterCustomClassLayout(
			ACk_PathNetwork_UE::StaticClass()->GetFName());
	}
}

auto
    FCkPathNetworkEditorModule::
    DoRegisterToolsMenu()
    -> void
{
    auto* ToolMenus = UToolMenus::Get();
    if (ck::Is_NOT_Valid(ToolMenus))
    { return; }

    const auto OwnerScoped = FToolMenuOwnerScoped(this);
    auto* ToolsMenu =
        ToolMenus->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
    if (ck::Is_NOT_Valid(ToolsMenu))
    { return; }

    auto& Section = ToolsMenu->FindOrAddSection(
        TEXT("CkPathNetwork"),
        LOCTEXT("CkPathNetworkSection", "CkFoundation Path Network"));
    Section.AddMenuEntry(
        TEXT("CkPathNetwork_OpenDesigner"),
        LOCTEXT("OpenDesignerLabel", "Ck Path Network Designer"),
        LOCTEXT(
            "OpenDesignerTooltip",
            "Open or focus the docked Ck Path Network designer for the current editor world."),
        FSlateIcon{},
        FUIAction{
            FExecuteAction::CreateStatic(
                &ck::pathnetwork_editor::designer::Open_Designer),
            FCanExecuteAction::CreateStatic(
                &ck::pathnetwork_editor::designer::Can_OpenDesigner)});
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkPathNetworkEditorModule, CkPathNetworkEditor)
