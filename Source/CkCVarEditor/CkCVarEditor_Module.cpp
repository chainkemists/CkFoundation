#include "CkCVarEditor_Module.h"

#include "CkCVarEditor/CkCVar_RefGraphPanelPinFactory.h"
#include "CkCVarEditor/CkCVar_RefDetails.h"

#include "CkCVar/CkCVar_Data.h"

#include <PropertyEditorModule.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkCVarEditorModule"

void FCkCVarEditorModule::StartupModule()
{
	auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout
	(
		FCk_CVarRef::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&ck::layout::FCVarRef_Details::MakeInstance)
	);

	_CVarRefPinFactory = MakeShareable(new FCk_CVarRef_GraphPanelPinFactory());
	FEdGraphUtilities::RegisterVisualPinFactory(_CVarRefPinFactory);
}

void FCkCVarEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(FCk_CVarRef::StaticStruct()->GetFName());
	}

	if (_CVarRefPinFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualPinFactory(_CVarRefPinFactory);
		_CVarRefPinFactory.Reset();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkCVarEditorModule, CkCVarEditor)

// --------------------------------------------------------------------------------------------------------------------
