#include "CkPathNetworkEditor_Module.h"

#include "CkPathNetworkEditor/CkPathNetwork_Details.h"

#include "CkPathNetwork/Actor/CkPathNetwork_Actor.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

#define LOCTEXT_NAMESPACE "FCkPathNetworkEditorModule"

// --------------------------------------------------------------------------------------------------------------------

void FCkPathNetworkEditorModule::StartupModule()
{
	auto& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	PropertyEditor.RegisterCustomClassLayout(
		ACk_PathNetwork_UE::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(
			&ck::layout::FCk_PathNetwork_Details::MakeInstance));
}

void FCkPathNetworkEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		auto& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditor.UnregisterCustomClassLayout(
			ACk_PathNetwork_UE::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkPathNetworkEditorModule, CkPathNetworkEditor)
