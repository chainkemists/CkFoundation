#include "CkUI_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkUI/CustomWidgets/Watermark/CkWatermark_Widget.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_Settings_UE::
    Get_WatermarkWidgetClass()
    -> TSubclassOf<UCk_Watermark_UserWidget_UE>
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_UI_ProjectSettings_UE>()->Get_WatermarkWidgetClass().LoadSynchronous();
}

auto
    UCk_Utils_UI_Settings_UE::
    Get_WatermarkWidget_ZOrder()
    -> int32
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_UI_ProjectSettings_UE>()->Get_WatermarkWidget_ZOrder();
}

// --------------------------------------------------------------------------------------------------------------------
