#include "CkEditorToolbar_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EditorToolbar_Settings_UE::
    Get_ToolbarExtensionWidgets()
    -> TMap<ECk_EditorToolbar_ExtensionPoint, FCk_EditorToolbar_ExtensionWidgets>
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_EditorToolbar_UserSettings_UE>()->Get_ToolbarExtensionWidgets();
}

// --------------------------------------------------------------------------------------------------------------------

