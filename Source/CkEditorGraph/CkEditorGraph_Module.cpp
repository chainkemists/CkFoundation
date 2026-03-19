#include "CkEditorGraph_Module.h"

#include "CkEditorGraph/StructTypeSelector/CkStructTypeSelector_PinFactory.h"

#define LOCTEXT_NAMESPACE "FCkEditorGraphModule"

// ----------------------------------------------------------------------------------------------------------------

void FCkEditorGraphModule::StartupModule()
{
    _StructTypeSelectorPinFactory = MakeShareable(new FCk_StructTypeSelector_PinFactory());
    FEdGraphUtilities::RegisterVisualPinFactory(_StructTypeSelectorPinFactory);
}

void FCkEditorGraphModule::ShutdownModule()
{
    if (_StructTypeSelectorPinFactory.IsValid())
    {
        FEdGraphUtilities::UnregisterVisualPinFactory(_StructTypeSelectorPinFactory);
        _StructTypeSelectorPinFactory.Reset();
    }
}

// ----------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkEditorGraphModule, CkEditorGraph)
