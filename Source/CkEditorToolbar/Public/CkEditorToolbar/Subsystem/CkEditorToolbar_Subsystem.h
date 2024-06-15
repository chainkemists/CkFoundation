#pragma once

#include "CkEditorToolbar/Settings/CkEditorToolbar_Settings.h"

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <EditorSubsystem.h>
#include <EditorUtilityWidget.h>
#include <Widgets/SWidget.h>

#include "CkEditorToolbar_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKEDITORTOOLBAR_API UCk_EditorToolbar_Toolbar_Subsystem_UE : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EditorToolbar_Toolbar_Subsystem_UE);

protected:
    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto
    Deinitialize() -> void override;

protected:
    auto
    OnSettingChanged(
        UObject* InObject,
        struct FPropertyChangedEvent& InPropertyChangedEvent) -> void;

    auto
    OnBlueprintReinstanced() -> void;

    auto
    OnMapChanged(
        UWorld* InInWorld,
        EMapChangeType InMapChangeType) -> void;

private:
    auto
    Request_RefreshToolbarExtensions() -> void;

    auto
    Request_CleanupToolbarExtensions() -> void;

    auto
    Request_CreateToolbarWidget(
        TSoftClassPtr<UEditorUtilityWidget> InWidgetClass) -> TSharedRef<SWidget>;

    auto
    Request_CleanupToolbarWidgets() -> void;

    auto
    Request_ExtendMenuAtToolbarExtensionPoint(
        ECk_EditorToolbar_ExtensionPoint InExtensionPoint,
        TFunction<void(UToolMenu*)> InLamda) -> void;

private:
    static FName _ToolbarExtensionSectionName;

private:
    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<UEditorUtilityWidget>> _CreatedWidgets;
};

// --------------------------------------------------------------------------------------------------------------------
