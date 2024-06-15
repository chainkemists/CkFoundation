#include "CkEditorToolbar_Subsystem.h"

#include "CkEditorToolbar/Settings/CkEditorToolbar_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEditorToolbar/Settings/CkEditorToolbar_Settings.h"
#include "CkEditorToolbar/CkEditorToolbar_Log.h"

#include "CkUI/CkUI_Utils.h"

#include <Blueprint/WidgetTree.h>
#include <EditorUtilityWidget.h>
#include <LevelEditor.h>
#include <Components/Widget.h>
#include "Blueprint/UserWidget.h"

// --------------------------------------------------------------------------------------------------------------------

FName UCk_EditorToolbar_Toolbar_Subsystem_UE::_ToolbarExtensionSectionName = FName{TEXT("CkEditorToolbar")};

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EditorToolbar_Toolbar_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    if (UCk_Utils_EditorOnly_UE::Get_IsCommandletOrCooking())
    { return; }

    // When changing the map, clean up and refresh the toolbar appropriately
    auto& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
    LevelEditorModule.OnMapChanged().AddUObject(this, &ThisType::OnMapChanged);

    // Clean up and refresh the toolbar when Blueprints are recompiled
    GEditor->OnBlueprintReinstanced().AddUObject(this, &ThisType::OnBlueprintReinstanced);

    // Clean up and refresh the toolbar when the config is changed
    GetMutableDefault<UCk_EditorToolbar_UserSettings_UE>()->OnSettingChanged().AddUObject(this, &ThisType::OnSettingChanged);
}

auto
    UCk_EditorToolbar_Toolbar_Subsystem_UE::
    Deinitialize() -> void
{
    UToolMenus::UnregisterOwner(this);

    Super::Deinitialize();
}

auto
    UCk_EditorToolbar_Toolbar_Subsystem_UE::
    Request_CreateToolbarWidget(
        TSoftClassPtr<UEditorUtilityWidget> InWidgetClass)
    -> TSharedRef<SWidget>
{
    const auto& WidgetClassName = InWidgetClass.ToString();

    if (const auto& ExistingUMGWidget = _CreatedWidgets.Find(WidgetClassName);
        ck::IsValid(ExistingUMGWidget, ck::IsValid_Policy_NullptrOnly{}))
    {
        UCk_Utils_Object_UE::Request_TrySetOuter
        (
            FCk_Utils_Object_SetOuter_Params{*ExistingUMGWidget}
                .Set_Outer(GetTransientPackage())
                .Set_RenameFlags(ECk_Utils_Object_RenameFlags::DoNotDirty)
        );

        _CreatedWidgets.Remove(WidgetClassName);
    }

    const auto& CreatedToolbarWidget = [&]() -> TSharedRef<SWidget>
    {
        const auto& EditorWorld = GEditor->GetEditorWorldContext().World();
        if (ck::Is_NOT_Valid(EditorWorld))
        { return SNullWidget::NullWidget; }

        const auto& CreatedUmgWidget = CreateWidget<UEditorUtilityWidget>(EditorWorld, InWidgetClass.LoadSynchronous());
        if (ck::Is_NOT_Valid(CreatedUmgWidget))
        { return SNullWidget::NullWidget; }

        // Editor Utility is flagged as transient to prevent from dirty the World it's created in when a property added to the Utility Widget is changed
        CreatedUmgWidget->SetFlags(RF_Transient);

        // Mark nested utility widgets as transient to prevent them from dirtying the world (since they'll be created via CreateWidget and not CreateUtilityWidget)
        UCk_Utils_UI_UE::ForEachWidgetAndChildren_IncludingUserWidgets(CreatedUmgWidget, [&](UWidget* InWidget) -> bool
        {
            if (ck::Is_NOT_Valid(InWidget))
            { return true; }

            if (NOT InWidget->IsA(UEditorUtilityWidget::StaticClass()))
            { return true; }

            InWidget->SetFlags(RF_Transient);

            return false;
        });

        _CreatedWidgets.Add(WidgetClassName, CreatedUmgWidget);

        return SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .VAlign(VAlign_Fill)
                [
                    CreatedUmgWidget->TakeWidget()
                ];
    }();

    return CreatedToolbarWidget;
}

auto
    UCk_EditorToolbar_Toolbar_Subsystem_UE::
    Request_CleanupToolbarWidgets()
    -> void
{
    for (const auto& Kvp : _CreatedWidgets)
    {
        if (const auto& Widget = Kvp.Value;
            ck::IsValid(Widget))
        {
            // Make the widgets no longer owned by the world so they can get GC'd
            UCk_Utils_Object_UE::Request_TrySetOuter
            (
                FCk_Utils_Object_SetOuter_Params{Widget}
                    .Set_Outer(GetTransientPackage())
                    .Set_RenameFlags(ECk_Utils_Object_RenameFlags::DoNotDirty)
            );
        }
    }

    _CreatedWidgets.Empty();
}

auto
    UCk_EditorToolbar_Toolbar_Subsystem_UE::
    Request_ExtendMenuAtToolbarExtensionPoint(
        ECk_EditorToolbar_ExtensionPoint InExtensionPoint,
        TFunction<void(UToolMenu*)> InLamda)
    -> void
{
    const auto& ExtensionPointName = [&]() -> FName
    {
        switch (InExtensionPoint)
        {
            case ECk_EditorToolbar_ExtensionPoint::Play: { return TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar"); }
            case ECk_EditorToolbar_ExtensionPoint::Modes: { return TEXT("LevelEditor.LevelEditorToolBar.ModesToolBar"); }
            case ECk_EditorToolbar_ExtensionPoint::Assets: { return TEXT("LevelEditor.LevelEditorToolBar.AssetsToolBar"); }
            case ECk_EditorToolbar_ExtensionPoint::User: { return TEXT("LevelEditor.LevelEditorToolBar.User"); }
            case ECk_EditorToolbar_ExtensionPoint::Settings: { return TEXT("LevelEditor.LevelEditorToolBar.SettingsToolbar"); }
            default:
            {
                CK_INVALID_ENUM(InExtensionPoint);
                return {};
            }
        }
    }();

    // Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
    FToolMenuOwnerScoped OwnerScoped(this);

    const auto& ToolbarMenu = UToolMenus::Get()->ExtendMenu(ExtensionPointName);

    CK_ENSURE_IF_NOT(ck::IsValid(ToolbarMenu, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Failed to find Toolbar Menu [{}]"), ExtensionPointName)
    { return; }

    InLamda(ToolbarMenu);
}

auto
    UCk_EditorToolbar_Toolbar_Subsystem_UE::
    OnSettingChanged(
        UObject* InObject,
        FPropertyChangedEvent& InPropertyChangedEvent)
    -> void
{
    Request_CleanupToolbarExtensions();
    Request_RefreshToolbarExtensions();
}

auto
    UCk_EditorToolbar_Toolbar_Subsystem_UE::
    OnBlueprintReinstanced()
    -> void
{
    Request_CleanupToolbarExtensions();
    Request_RefreshToolbarExtensions();
}

auto
    UCk_EditorToolbar_Toolbar_Subsystem_UE::
    OnMapChanged(
        UWorld* InInWorld,
        EMapChangeType InMapChangeType)
    -> void
{
    switch (InMapChangeType)
    {
        case EMapChangeType::NewMap:
        case EMapChangeType::LoadMap:
        {
            ck::editor_toolbar::Verbose(TEXT("Refreshing Editor Toolbar Widgets during [{}]"), InMapChangeType);

            Request_RefreshToolbarExtensions();
            break;
        }
        case EMapChangeType::TearDownWorld:
        {
            ck::editor_toolbar::Verbose(TEXT("Cleaning up Editor Toolbar Widgets during [{}]"), InMapChangeType);

            Request_CleanupToolbarExtensions();

            break;
        }
        case EMapChangeType::SaveMap:
        default:
        {
            break;
        }
    }
}

auto
    UCk_EditorToolbar_Toolbar_Subsystem_UE::
    Request_RefreshToolbarExtensions()
    -> void
{
    const auto& EditorToolbarSubsystem = GEditor->GetEditorSubsystem<UCk_EditorToolbar_Toolbar_Subsystem_UE>();
    const auto& Settings = GetDefault<UCk_EditorToolbar_UserSettings_UE>();

    if (ck::Is_NOT_Valid(EditorToolbarSubsystem) || ck::Is_NOT_Valid(Settings))
    { return; }

    for (const auto& ToolbarExtensionWidgets = UCk_Utils_EditorToolbar_Settings_UE::Get_ToolbarExtensionWidgets();
         const auto& Kvp : ToolbarExtensionWidgets)
    {
        Request_ExtendMenuAtToolbarExtensionPoint(Kvp.Key, [&](UToolMenu* InMenu)
        {
            auto& Section = InMenu->FindOrAddSection(_ToolbarExtensionSectionName);
            Section.Blocks.Empty();

            for (const auto& ToolbarUtilityWidget : Kvp.Value.Get_UtilityWidgets())
            {
                if (ck::Is_NOT_Valid(ToolbarUtilityWidget))
                { continue; }

                const auto& Widget = EditorToolbarSubsystem->Request_CreateToolbarWidget(ToolbarUtilityWidget);

                const auto& GeneratedWidgetName = ck::Format_UE(TEXT("CkEditorToolbar_{}"), ToolbarUtilityWidget.ToString());

                Section.AddEntry(FToolMenuEntry::InitWidget(*GeneratedWidgetName, Widget, FText::AsCultureInvariant(GeneratedWidgetName)));
            }
        });
    }
}

auto
    UCk_EditorToolbar_Toolbar_Subsystem_UE::
    Request_CleanupToolbarExtensions()
    -> void
{
    for (auto i = 0; i < static_cast<int32>(ECk_EditorToolbar_ExtensionPoint::Count); ++i)
    {
        Request_ExtendMenuAtToolbarExtensionPoint(static_cast<ECk_EditorToolbar_ExtensionPoint>(i), [&](UToolMenu* InMenu)
        {
            auto& Section = InMenu->FindOrAddSection(_ToolbarExtensionSectionName);
            Section.Blocks.Empty();

            if (const auto& EditorToolbarSubsystem = GEditor->GetEditorSubsystem<UCk_EditorToolbar_Toolbar_Subsystem_UE>();
                ck::IsValid(EditorToolbarSubsystem))
            {
                EditorToolbarSubsystem->Request_CleanupToolbarWidgets();
            }
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------

