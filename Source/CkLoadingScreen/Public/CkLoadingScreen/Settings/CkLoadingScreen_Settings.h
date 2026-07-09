#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include <UObject/SoftObjectPath.h>

#include "CkLoadingScreen_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Loading Screen"))
class CKLOADINGSCREEN_API UCk_LoadingScreen_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_LoadingScreen_ProjectSettings_UE);

private:
    /** The UserWidget class to display as the loading screen. Falls back to a plain throbber when unset. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Display",
              meta = (AllowPrivateAccess = true, MetaClass = "/Script/UMG.UserWidget"))
    FSoftClassPath _LoadingScreenWidget;

    /** The z-order of the loading screen widget in the viewport stack. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Display",
              meta = (AllowPrivateAccess = true))
    int32 _LoadingScreenZOrder = 10000;

    /**
     * How long to hold the loading screen up after other loading finishes (in seconds), to give
     * texture streaming a chance to avoid blurriness. World rendering is re-enabled during this
     * window so streaming can actually progress behind the screen.
     *
     * Not applied in the editor unless _HoldLoadingScreenAdditionalSecsEvenInEditor is set.
     */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Configuration",
              meta = (AllowPrivateAccess = true, ForceUnits = s, ConsoleVariable = "ck.LoadingScreen.HoldAdditionalSecs"))
    float _HoldLoadingScreenAdditionalSecs = 2.0f;

    /** The interval in seconds beyond which the loading screen is considered permanently hung (if non-zero). */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Configuration",
              meta = (AllowPrivateAccess = true, ForceUnits = s))
    float _LoadingScreenHeartbeatHangDuration = 0.0f;

    /** The interval in seconds between each log of what is keeping the loading screen up (if non-zero). */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Configuration",
              meta = (AllowPrivateAccess = true, ForceUnits = s))
    float _LogLoadingScreenHeartbeatInterval = 5.0f;

    /** Tick Slate as soon as the screen is shown so it displays immediately, even in the editor. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Configuration",
              meta = (AllowPrivateAccess = true))
    bool _ForceTickLoadingScreenEvenInEditor = true;

    /** When true, the reason the loading screen is shown or hidden is printed to the log every frame. */
    UPROPERTY(Transient, EditDefaultsOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable = "ck.LoadingScreen.LogReasonEveryFrame"))
    bool _LogLoadingScreenReasonEveryFrame = false;

    /** Force the loading screen to be displayed (useful for debugging). */
    UPROPERTY(Transient, EditDefaultsOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable = "ck.LoadingScreen.AlwaysShow"))
    bool _ForceLoadingScreenVisible = false;

    /**
     * Escape hatch: fully suppress the loading screen presentation (widget, input block, render
     * suppression). Holder bookkeeping still runs so tests can assert on it. Presentation is also
     * suppressed automatically for commandlets, -unattended, and null-RHI runs.
     */
    UPROPERTY(Transient, EditDefaultsOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true, ConsoleVariable = "ck.LoadingScreen.Disable"))
    bool _DisableLoadingScreen = false;

    /** Apply the additional hold delay even in the editor (useful when iterating on loading screens). */
    UPROPERTY(Transient, EditDefaultsOnly, Category = "Debugging",
              meta = (AllowPrivateAccess = true))
    bool _HoldLoadingScreenAdditionalSecsEvenInEditor = false;

public:
    CK_PROPERTY_GET(_LoadingScreenWidget);
    CK_PROPERTY_GET(_LoadingScreenZOrder);
    CK_PROPERTY_GET(_HoldLoadingScreenAdditionalSecs);
    CK_PROPERTY_GET(_LoadingScreenHeartbeatHangDuration);
    CK_PROPERTY_GET(_LogLoadingScreenHeartbeatInterval);
    CK_PROPERTY_GET(_ForceTickLoadingScreenEvenInEditor);
    CK_PROPERTY_GET(_LogLoadingScreenReasonEveryFrame);
    CK_PROPERTY_GET(_ForceLoadingScreenVisible);
    CK_PROPERTY_GET(_DisableLoadingScreen);
    CK_PROPERTY_GET(_HoldLoadingScreenAdditionalSecsEvenInEditor);
};

// --------------------------------------------------------------------------------------------------------------------

class CKLOADINGSCREEN_API UCk_Utils_LoadingScreen_Settings_UE
{
public:
    static auto
    Get_LoadingScreenWidget() -> const FSoftClassPath&;

    static auto
    Get_LoadingScreenZOrder() -> int32;

    static auto
    Get_HoldLoadingScreenAdditionalSecs() -> float;

    static auto
    Get_LoadingScreenHeartbeatHangDuration() -> float;

    static auto
    Get_LogLoadingScreenHeartbeatInterval() -> float;

    static auto
    Get_ForceTickLoadingScreenEvenInEditor() -> bool;

    static auto
    Get_LogLoadingScreenReasonEveryFrame() -> bool;

    static auto
    Get_ForceLoadingScreenVisible() -> bool;

    static auto
    Get_DisableLoadingScreen() -> bool;

    static auto
    Get_HoldLoadingScreenAdditionalSecsEvenInEditor() -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
