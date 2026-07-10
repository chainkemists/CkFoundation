#include "CkVatEditor_Module.h"

#include "CkVatEditor/CkVatCollection_Details.h"

#include "CkVat/Collection/CkVatCollection_Data.h"

#include <PropertyEditorModule.h>

#define LOCTEXT_NAMESPACE "FCkVatEditorModule"

void FCkVatEditorModule::StartupModule()
{
    auto& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    PropertyModule.RegisterCustomClassLayout
    (
        UCk_VatCollection_Data::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&ck::layout::FCkVatCollection_Details::MakeInstance)
    );
}

void FCkVatEditorModule::ShutdownModule()
{
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.UnregisterCustomClassLayout(UCk_VatCollection_Data::StaticClass()->GetFName());
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkVatEditorModule, CkVatEditor)
