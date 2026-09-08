#pragma once

#include "CkSlateLayout/CkUiDocument.h"

enum class ECkUiBuiltinBindingKind : uint8
{
    None,
    Native,
    OptionalText,
    Image,
    SearchText,
    Collection,
    TreeCollection,
};

struct FCkUiBuiltinWidgetSchema
{
    ECkUiNodeKind Kind = ECkUiNodeKind::Column;
    const TCHAR* Tag = TEXT("");
    ECkUiBuiltinBindingKind BindingKind = ECkUiBuiltinBindingKind::None;
    bool bAllowsAction = false;
    bool bRequiresAction = false;
    bool bAllowsPlaceholder = false;
    bool bAllowsVisibility = true;
    bool bAllowsLiteralText = false;
    int32 MinChildren = 0;
    int32 MaxChildren = 0;
    bool bAllowsGap = false;
    bool bRequiresZeroGap = false;
    bool bAllowsPadding = false;
    bool bAllowsBackground = false;
    bool bAllowsTextStyle = false;
};

// Internal built-in-only registry. Public custom widget registration is deliberately deferred to Gate 02d later slices.
namespace ck_ui_widget_registry
{
    auto FindByTag(const FString& InTag) -> const FCkUiBuiltinWidgetSchema*;
    auto FindByKind(ECkUiNodeKind InKind) -> const FCkUiBuiltinWidgetSchema*;
}
