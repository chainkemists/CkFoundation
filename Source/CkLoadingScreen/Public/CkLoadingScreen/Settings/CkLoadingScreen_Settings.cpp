#include "CkLoadingScreen_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_LoadingScreenWidget()
    -> const FSoftClassPath&
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_LoadingScreenWidget();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_LoadingScreenZOrder()
    -> int32
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_LoadingScreenZOrder();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_HoldLoadingScreenAdditionalSecs()
    -> float
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_HoldLoadingScreenAdditionalSecs();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_LoadingScreenHeartbeatHangDuration()
    -> float
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_LoadingScreenHeartbeatHangDuration();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_LogLoadingScreenHeartbeatInterval()
    -> float
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_LogLoadingScreenHeartbeatInterval();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_ForceTickLoadingScreenEvenInEditor()
    -> bool
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_ForceTickLoadingScreenEvenInEditor();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_WaitForStreamingLevels()
    -> bool
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_WaitForStreamingLevels();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_LogLoadingScreenReasonEveryFrame()
    -> bool
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_LogLoadingScreenReasonEveryFrame();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_ForceLoadingScreenVisible()
    -> bool
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_ForceLoadingScreenVisible();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_DisableLoadingScreen()
    -> bool
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_DisableLoadingScreen();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_HoldLoadingScreenAdditionalSecsEvenInEditor()
    -> bool
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_HoldLoadingScreenAdditionalSecsEvenInEditor();
}

// --------------------------------------------------------------------------------------------------------------------
