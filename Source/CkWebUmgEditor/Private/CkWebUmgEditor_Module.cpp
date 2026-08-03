#include "CkWebUmgEditor/CkWebUmgEditor_Module.h"

#include "CkWebUmgEditor/CkWebUmg_Importer.h"

#include <DesktopPlatformModule.h>
#include <Framework/Application/SlateApplication.h>
#include <ToolMenus.h>

#define LOCTEXT_NAMESPACE "FCkWebUmgEditorModule"

void FCkWebUmgEditorModule::StartupModule()
{
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
    {
        auto* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
        if (ToolsMenu == nullptr)
        { return; }

        auto& Section = ToolsMenu->FindOrAddSection("CkWebUmg", LOCTEXT("CkWebUmgSection", "CkWebUmg"));

        Section.AddMenuEntry(
            "CkWebUmg_ImportWebPage",
            LOCTEXT("ImportWebPage", "Import Web Page (CkWebUmg)..."),
            LOCTEXT("ImportWebPage_Tooltip",
                "Extracts an .html mockup (or reads a .ckui.json IR bundle) into a CkWebUmg page asset under /Game/WebUmg"),
            FSlateIcon{},
            FUIAction{FExecuteAction::CreateLambda([]()
            {
                auto* DesktopPlatform = FDesktopPlatformModule::Get();
                if (DesktopPlatform == nullptr)
                { return; }

                auto SelectedFiles = TArray<FString>{};
                const auto PickedAFile = DesktopPlatform->OpenFileDialog(
                    FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
                    TEXT("Import Web Page"),
                    TEXT(""),
                    TEXT(""),
                    TEXT("Web page or CkWebUmg IR (*.html;*.ckui.json)|*.html;*.ckui.json"),
                    EFileDialogFlags::None,
                    SelectedFiles);
                if (NOT PickedAFile || SelectedFiles.Num() == 0)
                { return; }

                constexpr auto SaveToDisk = true;
                const auto& File = SelectedFiles[0];
                if (File.EndsWith(TEXT(".html")))
                { ck::webumg::editor::ImportPageAssetFromHtml(File, TEXT("/Game/WebUmg"), SaveToDisk); }
                else
                { ck::webumg::editor::ImportPageAsset(File, TEXT("/Game/WebUmg"), SaveToDisk); }
            })});
    }));
}

void FCkWebUmgEditorModule::ShutdownModule()
{
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkWebUmgEditorModule, CkWebUmgEditor);
