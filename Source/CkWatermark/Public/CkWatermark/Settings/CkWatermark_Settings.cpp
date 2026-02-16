#include "CkWatermark_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_FPS_ColorBands()
    -> const FCk_Watermark_ColorBands_HigherIsBetter&
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_FPS_ColorBands();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_ServerFPS_ColorBands()
    -> const FCk_Watermark_ColorBands_HigherIsBetter&
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_ServerFPS_ColorBands();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Ping_ColorBands()
    -> const FCk_Watermark_ColorBands_LowerIsBetter&
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Ping_ColorBands();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_EnsureCount_ColorBands()
    -> const FCk_Watermark_ColorBands_LowerIsBetter&
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_EnsureCount_ColorBands();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_ValueFontSize()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_ValueFontSize();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_LabelFontSize()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_LabelFontSize();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BracketFontSize()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BracketFontSize();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildType_FontSize()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildType_FontSize();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_TextOutlineSize()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_TextOutlineSize();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_TextOutlineColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_TextOutlineColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_StatCell_HorizontalPadding()
    -> float
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_StatCell_HorizontalPadding();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_VerticalPadding()
    -> float
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_VerticalPadding();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Bracket_InnerPadding()
    -> float
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Bracket_InnerPadding();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BracketOpen()
    -> FString
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BracketOpen();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BracketClose()
    -> FString
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BracketClose();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_LabelColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_LabelColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_UncoloredStat_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_UncoloredStat_Color();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildConfig_Debug_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildConfig_Debug_Color();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildConfig_Dev_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildConfig_Dev_Color();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildConfig_Test_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildConfig_Test_Color();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildConfig_Shipping_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildConfig_Shipping_Color();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Show_BuildId()
    -> bool
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Show_BuildId();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Show_MemoryPressure()
    -> bool
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Show_MemoryPressure();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Show_CpuBrand()
    -> bool
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Show_CpuBrand();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Show_OsVersion()
    -> bool
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Show_OsVersion();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Show_CpuCores()
    -> bool
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Show_CpuCores();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Show_NetworkType()
    -> bool
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Show_NetworkType();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Show_EcsDebugger()
    -> bool
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Show_EcsDebugger();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Show_EcsEntityMap()
    -> bool
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Show_EcsEntityMap();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Show_EcsCallstacks()
    -> bool
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Show_EcsCallstacks();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_InfoBar_Separator()
    -> FString
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_InfoBar_Separator();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_InfoBar_KeyValueSeparator()
    -> FString
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_InfoBar_KeyValueSeparator();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_InfoBar_FontSize()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_InfoBar_FontSize();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_InfoBar_KeyColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_InfoBar_KeyColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_InfoBar_ValueColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_InfoBar_ValueColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_InfoBar_SeparatorColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_InfoBar_SeparatorColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildId_Active_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildId_Active_Color();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildId_Inactive_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildId_Inactive_Color();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildId_EnabledBranches()
    -> const TArray<FString>&
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildId_EnabledBranches();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Ensures()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Ensures();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_UniqueEnsures()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_UniqueEnsures();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Ram()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Ram();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Vram()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Vram();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_MemPressure()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_MemPressure();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Ping()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Ping();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_ServerFps()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_ServerFps();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Fps()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Fps();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Time()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Time();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Frame()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Frame();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Widget_ZOrder()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Widget_ZOrder();
}

// --------------------------------------------------------------------------------------------------------------------
