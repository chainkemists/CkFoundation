#include "CkPieLayoutEditor_Module.h"

#include "CkPieLayoutEditor/Settings/CkPieLayoutEditor_Settings.h"
#include "CkPieLayoutEditor/Subsystem/CkPieLayoutEditor_Subsystem.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Editor.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FCkPieLayoutEditorModule"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkPieLayoutEditorModule::
    StartupModule()
    -> void
{
    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FCkPieLayoutEditorModule::DoRegisterToolsMenu));
}

auto
    FCkPieLayoutEditorModule::
    ShutdownModule()
    -> void
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkPieLayoutEditorModule::
    DoRegisterToolsMenu()
    -> void
{
    auto* ToolMenus = UToolMenus::Get();
    if (ck::Is_NOT_Valid(ToolMenus))
    { return; }

    const auto OwnerScoped = FToolMenuOwnerScoped(this);

    auto* ToolsMenu = ToolMenus->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"));
    if (ck::Is_NOT_Valid(ToolsMenu))
    { return; }

    auto& Section = ToolsMenu->FindOrAddSection(
        TEXT("CkPieLayout"),
        LOCTEXT("CkPieLayout_SectionLabel", "CkFoundation PIE Layout"));

    Section.AddMenuEntry(
        TEXT("CkPieLayout_OpenPreferences"),
        LOCTEXT("CkPieLayout_OpenPreferences_Label", "Open Preferences"),
        LOCTEXT("CkPieLayout_OpenPreferences_Tooltip", "Open the PIE Layout settings in Editor Preferences."),
        FSlateIcon(),
        FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& /*InContext*/)
        {
            auto* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>(TEXT("Settings"));
            if (ck::Is_NOT_Valid(SettingsModule, ck::IsValid_Policy_NullptrOnly{}))
            { return; }

            const auto* SettingsCDO = GetDefault<UCk_PieLayout_Settings_UE>();
            if (ck::Is_NOT_Valid(SettingsCDO))
            { return; }

            SettingsModule->ShowViewer(
                SettingsCDO->GetContainerName(),
                SettingsCDO->GetCategoryName(),
                SettingsCDO->GetSectionName());
        }));

    Section.AddMenuEntry(
        TEXT("CkPieLayout_ArrangeNow"),
        LOCTEXT("CkPieLayout_ArrangeNow_Label", "Arrange PIE Windows Now"),
        LOCTEXT("CkPieLayout_ArrangeNow_Tooltip", "Arrange the currently-open Play-In-Editor windows into the grid immediately."),
        FSlateIcon(),
        FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext& /*InContext*/)
        {
            if (ck::Is_NOT_Valid(GEditor))
            { return; }

            if (auto* Subsystem = GEditor->GetEditorSubsystem<UCk_PieLayout_Subsystem_UE>();
                ck::IsValid(Subsystem))
            { Subsystem->Request_ArrangeNow(); }
        }));
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkPieLayoutEditorModule, CkPieLayoutEditor)
