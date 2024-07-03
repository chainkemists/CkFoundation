#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/UserSettings/CkUserSettings.h"

#include <EditorUtilityWidget.h>

#include "CkEditorToolbar_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM()
enum class ECk_EditorToolbar_ExtensionPoint : uint8
{
    Play,
    Modes,
    Assets,
    User,
    Settings,

    Count UMETA(Hidden)
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EditorToolbar_ExtensionPoint);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_EditorToolbar_ExtensionWidgets
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EditorToolbar_ExtensionWidgets);

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true, MetaClass="EditorUtilityWidget"))
    TArray<FSoftClassPath> _UtilityWidgets;

public:
    CK_PROPERTY_GET(_UtilityWidgets);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Editor Toolbar"))
class CKEDITORTOOLBAR_API UCk_EditorToolbar_UserSettings_UE : public UCk_Plugin_UserSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EditorToolbar_UserSettings_UE);

private:
    UPROPERTY(EditAnywhere, Config, Category = "Toolbar Extension Widgets",
        meta = (AllowPrivateAccess = true, ForceInlineRow))
    TMap<ECk_EditorToolbar_ExtensionPoint, FCk_EditorToolbar_ExtensionWidgets> _ToolbarExtensionWidgets;

public:
    CK_PROPERTY_GET(_ToolbarExtensionWidgets);
};

// --------------------------------------------------------------------------------------------------------------------

class CKEDITORTOOLBAR_API UCk_Utils_EditorToolbar_Settings_UE
{
public:
    static auto Get_ToolbarExtensionWidgets() -> TMap<ECk_EditorToolbar_ExtensionPoint, FCk_EditorToolbar_ExtensionWidgets>;
};

// --------------------------------------------------------------------------------------------------------------------
