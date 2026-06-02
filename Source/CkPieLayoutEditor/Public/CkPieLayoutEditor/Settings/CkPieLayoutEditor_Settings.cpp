#include "CkPieLayoutEditor_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_AutoArrangeOnPIE()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return true; }
    return Settings->Get_AutoArrangeOnPIE();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_ArrangeSingleWindow()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return true; }
    return Settings->Get_ArrangeSingleWindow();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_RestoreWindowPositionsOnStop()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return true; }
    return Settings->Get_RestoreWindowPositionsOnStop();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_InitialArrangeDelay()
    -> FCk_Time
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return FCk_Time{0.15}; }
    return Settings->Get_InitialArrangeDelay();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_RetryDuration()
    -> FCk_Time
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return FCk_Time{6.0}; }
    return Settings->Get_RetryDuration();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_RetryInterval()
    -> FCk_Time
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return FCk_Time{0.1}; }
    return Settings->Get_RetryInterval();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_TargetMonitor()
    -> ECk_PieLayout_TargetMonitor
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return ECk_PieLayout_TargetMonitor::PrimaryMonitor; }
    return Settings->Get_TargetMonitor();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_SpecificMonitorIndex()
    -> int32
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 0; }
    return Settings->Get_SpecificMonitorIndex();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_LayoutMode()
    -> ECk_PieLayout_Mode
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return ECk_PieLayout_Mode::AutoBalanced; }
    return Settings->Get_LayoutMode();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_OuterMargin()
    -> int32
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 8; }
    return Settings->Get_OuterMargin();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_WindowGap()
    -> int32
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 8; }
    return Settings->Get_WindowGap();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_MinimumWindowWidth()
    -> int32
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 420; }
    return Settings->Get_MinimumWindowWidth();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_MinimumWindowHeight()
    -> int32
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 280; }
    return Settings->Get_MinimumWindowHeight();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_MinimumLandscapeRatio()
    -> float
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 1.0f; }
    return Settings->Get_MinimumLandscapeRatio();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_PreferredAspectRatio()
    -> float
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return 1.777778f; }
    return Settings->Get_PreferredAspectRatio();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_IncludeListenServer()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return true; }
    return Settings->Get_IncludeListenServer();
}

auto
    UCk_Utils_PieLayout_Settings_UE::
    Get_ServerFirst()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_PieLayout_Settings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return true; }
    return Settings->Get_ServerFirst();
}

// --------------------------------------------------------------------------------------------------------------------
