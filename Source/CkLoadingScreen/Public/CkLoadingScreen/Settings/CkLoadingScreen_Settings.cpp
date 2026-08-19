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

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_TransitionScreenEnabled()
    -> bool
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_TransitionScreenEnabled();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_TransitionLogoTexture()
    -> const FSoftObjectPath&
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_TransitionLogoTexture();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_TransitionLogoSize()
    -> FVector2D
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_TransitionLogoSize();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_TransitionLogoOffset()
    -> FVector2D
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_TransitionLogoOffset();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_TransitionLogoThrobPeriod()
    -> float
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_TransitionLogoThrobPeriod();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_TransitionLogoMinOpacity()
    -> float
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_TransitionLogoMinOpacity();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_TransitionBackgroundImages()
    -> const TArray<FSoftObjectPath>&
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_TransitionBackgroundImages();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_TransitionBackgroundTint()
    -> FLinearColor
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_TransitionBackgroundTint();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_TransitionTipTexts()
    -> const TArray<FText>&
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_TransitionTipTexts();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_StartupMoviePaths()
    -> const TArray<FString>&
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_StartupMoviePaths();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_TransitionWaitForGameThreadScreen()
    -> bool
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_TransitionWaitForGameThreadScreen();
}

auto
    UCk_Utils_LoadingScreen_Settings_UE::
    Get_TransitionModeA_UMGHandOff()
    -> bool
{
    return GetDefault<UCk_LoadingScreen_ProjectSettings_UE>()->Get_TransitionModeA_UMGHandOff();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_LoadingScreen_UE::
    Get_TransitionBackgroundImages()
    -> TArray<TSoftObjectPtr<UTexture2D>>
{
    const auto& Paths = UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionBackgroundImages();

    auto Result = TArray<TSoftObjectPtr<UTexture2D>>{};
    Result.Reserve(Paths.Num());

    for (const auto& Path : Paths)
    {
        if (Path.IsNull())
        { continue; }

        Result.Emplace(Path);
    }

    return Result;
}

auto
    UCk_Utils_LoadingScreen_UE::
    Get_TransitionTipTexts()
    -> TArray<FText>
{
    return UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionTipTexts();
}

auto
    UCk_Utils_LoadingScreen_UE::
    Get_TransitionLogoThrobPeriod()
    -> float
{
    return UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionLogoThrobPeriod();
}

auto
    UCk_Utils_LoadingScreen_UE::
    Get_TransitionLogoMinOpacity()
    -> float
{
    return UCk_Utils_LoadingScreen_Settings_UE::Get_TransitionLogoMinOpacity();
}

// --------------------------------------------------------------------------------------------------------------------
