#include "CkLogEditor_Module.h"

#include "CkLog/CkLog_Category.h"

#include "CkLogEditor/CkLog_CategoryDetails.h"
#include "CkLogEditor/CkLog_CategoryGraphPanelPinFactory.h"
#include "CkLogEditor/CkLog_CategoryGraphPin.h"

#include <EdGraphUtilities.h>
#include <PropertyEditorModule.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "FCkLogEditorModule"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkLogEditorModule::
    StartupModule()
    -> void
{
    auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomPropertyTypeLayout
    (
        FCk_LogCategory::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&ck::layout::FLogCategory_Details::MakeInstance));

    _GraphPanelPinFactory = MakeShareable(new FCk_LogCategory_GraphPanelPinFactory());
    FEdGraphUtilities::RegisterVisualPinFactory(_GraphPanelPinFactory);
}

auto
    FCkLogEditorModule::
    ShutdownModule()
    -> void
{
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.UnregisterCustomPropertyTypeLayout(FCk_LogCategory::StaticStruct()->GetFName());
    }

    if (_GraphPanelPinFactory.IsValid())
    {
        FEdGraphUtilities::UnregisterVisualPinFactory(_GraphPanelPinFactory);
        _GraphPanelPinFactory.Reset();
    }
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_MODULE(FCkLogEditorModule, CkLogEditor)
