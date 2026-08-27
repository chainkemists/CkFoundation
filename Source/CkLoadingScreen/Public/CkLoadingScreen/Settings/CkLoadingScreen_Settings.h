#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include <Framework/Text/TextLayout.h>
#include <Kismet/BlueprintFunctionLibrary.h>
#include <Layout/Margin.h>
#include <Math/Color.h>
#include <Math/Vector2D.h>
#include <Types/SlateEnums.h>
#include <UObject/SoftObjectPath.h>
#include <UObject/SoftObjectPtr.h>
#include <Widgets/Layout/SScaleBox.h>

#include "CkLoadingScreen_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UTexture2D;

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

    /**
     * Keep the loading screen up while any streaming sublevel that should be loaded/visible is
     * still pending. This is the "player falls through the floor on slow machines" guard.
     */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Configuration",
              meta = (AllowPrivateAccess = true))
    bool _WaitForStreamingLevels = true;

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

private:
    /**
     * Master switch for the MoviePlayer transition layer - the screen that covers the BLOCKING part
     * of a map load, where the game thread is stalled and the UMG screen cannot animate (or even
     * exist yet). Off by default; a project opts in from its own ini.
     */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    bool _TransitionScreenEnabled = false;

    /** Corner logo drawn on the transition screen. Empty = no logo. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true, AllowedClasses = "/Script/Engine.Texture2D"))
    FSoftObjectPath _TransitionLogoTexture;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    FVector2D _TransitionLogoSize = FVector2D{192.0, 192.0};

    /**
     * Inset from the corner/edge the logo is anchored to (see the alignment settings below).
     * Negative values move the logo inward. Ignored on a centered axis.
     */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    FVector2D _TransitionLogoOffset = FVector2D{-40.0, -40.0};

    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    TEnumAsByte<EHorizontalAlignment> _TransitionLogoHAlign = HAlign_Right;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    TEnumAsByte<EVerticalAlignment> _TransitionLogoVAlign = VAlign_Bottom;

    /** Full 1 -> min -> 1 opacity cycle. Zero or negative leaves the logo static. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true, ForceUnits = s))
    float _TransitionLogoThrobPeriod = 1.5f;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true, ClampMin = "0.0", ClampMax = "1.0"))
    float _TransitionLogoMinOpacity = 0.35f;

    /**
     * One is picked at random per show. Empty = the tint IS the background, which is the look a
     * project inherits if it never authors any.
     */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true, AllowedClasses = "/Script/Engine.Texture2D"))
    TArray<FSoftObjectPath> _TransitionBackgroundImages;

    /**
     * Must be OPAQUE. The transition screen covers a blocking load, so a translucent plate leaves
     * whatever was last rendered showing through underneath it.
     */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    FLinearColor _TransitionBackgroundTint = FLinearColor::Black;

    /** One is picked per show and stays for the whole load - nothing can tick a rotation out there. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    TArray<FText> _TransitionTipTexts;

    /**
     * Text style for the tip line. Point this at the SAME CommonTextStyle asset the game-thread
     * UMG loading widget's tip block uses and the two halves of a load render tips identically -
     * one asset, zero drift. Resolved on the game thread before the load blocks; empty falls back
     * to the engine default font (Roboto Regular 20, white), the pre-setting look.
     */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true, MetaClass = "/Script/CommonUI.CommonTextStyle"))
    FSoftClassPath _TransitionTipStyle;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    TEnumAsByte<EHorizontalAlignment> _TransitionTipHAlign = HAlign_Center;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    TEnumAsByte<EVerticalAlignment> _TransitionTipVAlign = VAlign_Bottom;

    /** Inset of the tip block from the screen edges its alignment anchors it to. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    FMargin _TransitionTipPadding = FMargin{0.0f, 0.0f, 0.0f, 64.0f};

    /** Wrap width in Slate units. Zero = never wrap, so a long tip runs as a single line. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _TransitionTipWrapTextAt = 0.0f;

    /** Only observable on tips that wrap to more than one line. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    TEnumAsByte<ETextJustify::Type> _TransitionTipJustification = ETextJustify::Center;

    /** How the backdrop image fills the screen. */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    TEnumAsByte<EStretch::Type> _TransitionBackdropStretch = EStretch::ScaleToFill;

    /**
     * Boot logo movies, relative to Content/Movies.
     *
     * NOTE: this module loads at ELoadingPhase::Default, which is AFTER LaunchEngineLoop has already
     * called PlayMovie() for the startup screen (LaunchEngineLoop.cpp: PreLoadingScreen modules 3748,
     * PlayMovie 3810, Default modules 4617). So this only fires if the module is ever moved to an
     * earlier phase. The engine's own [/Script/MoviePlayer.MoviePlayerSettings] StartupMovies, read
     * at 3507, is the route that actually plays logos at boot.
     */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    TArray<FString> _StartupMoviePaths;

    /**
     * Hold the movie until the game-thread loading screen is mounted, then stop it, so the handoff
     * has no frame of raw world in it. False falls back to retired-plugin parity (the movie tears
     * itself down the instant loading completes).
     */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    bool _TransitionWaitForGameThreadScreen = true;

    /**
     * EXPERIMENTAL, ships dark. Hands the configured UMG widget to the loading thread instead of the
     * Slate transition widget. UMG ticked on the Slate loading thread is the Ultra-plugin pattern: it
     * works in the common case and is UNVERIFIED with the AngelScript VM. Packaged-build experiment.
     */
    UPROPERTY(Config, EditDefaultsOnly, Category = "Transition Screen",
              meta = (AllowPrivateAccess = true))
    bool _TransitionModeA_UMGHandOff = false;

public:
    CK_PROPERTY_GET(_LoadingScreenWidget);
    CK_PROPERTY_GET(_LoadingScreenZOrder);
    CK_PROPERTY_GET(_HoldLoadingScreenAdditionalSecs);
    CK_PROPERTY_GET(_LoadingScreenHeartbeatHangDuration);
    CK_PROPERTY_GET(_LogLoadingScreenHeartbeatInterval);
    CK_PROPERTY_GET(_ForceTickLoadingScreenEvenInEditor);
    CK_PROPERTY_GET(_WaitForStreamingLevels);
    CK_PROPERTY_GET(_LogLoadingScreenReasonEveryFrame);
    CK_PROPERTY_GET(_ForceLoadingScreenVisible);
    CK_PROPERTY_GET(_DisableLoadingScreen);
    CK_PROPERTY_GET(_HoldLoadingScreenAdditionalSecsEvenInEditor);

    CK_PROPERTY_GET(_TransitionScreenEnabled);
    CK_PROPERTY_GET(_TransitionLogoTexture);
    CK_PROPERTY_GET(_TransitionLogoSize);
    CK_PROPERTY_GET(_TransitionLogoOffset);
    CK_PROPERTY_GET(_TransitionLogoHAlign);
    CK_PROPERTY_GET(_TransitionLogoVAlign);
    CK_PROPERTY_GET(_TransitionLogoThrobPeriod);
    CK_PROPERTY_GET(_TransitionLogoMinOpacity);
    CK_PROPERTY_GET(_TransitionBackgroundImages);
    CK_PROPERTY_GET(_TransitionBackgroundTint);
    CK_PROPERTY_GET(_TransitionBackdropStretch);
    CK_PROPERTY_GET(_TransitionTipTexts);
    CK_PROPERTY_GET(_TransitionTipStyle);
    CK_PROPERTY_GET(_TransitionTipHAlign);
    CK_PROPERTY_GET(_TransitionTipVAlign);
    CK_PROPERTY_GET(_TransitionTipPadding);
    CK_PROPERTY_GET(_TransitionTipWrapTextAt);
    CK_PROPERTY_GET(_TransitionTipJustification);
    CK_PROPERTY_GET(_StartupMoviePaths);
    CK_PROPERTY_GET(_TransitionWaitForGameThreadScreen);
    CK_PROPERTY_GET(_TransitionModeA_UMGHandOff);
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
    Get_WaitForStreamingLevels() -> bool;

    static auto
    Get_LogLoadingScreenReasonEveryFrame() -> bool;

    static auto
    Get_ForceLoadingScreenVisible() -> bool;

    static auto
    Get_DisableLoadingScreen() -> bool;

    static auto
    Get_HoldLoadingScreenAdditionalSecsEvenInEditor() -> bool;

    static auto
    Get_TransitionScreenEnabled() -> bool;

    static auto
    Get_TransitionLogoTexture() -> const FSoftObjectPath&;

    static auto
    Get_TransitionLogoSize() -> FVector2D;

    static auto
    Get_TransitionLogoOffset() -> FVector2D;

    static auto
    Get_TransitionLogoHAlign() -> EHorizontalAlignment;

    static auto
    Get_TransitionLogoVAlign() -> EVerticalAlignment;

    static auto
    Get_TransitionLogoThrobPeriod() -> float;

    static auto
    Get_TransitionLogoMinOpacity() -> float;

    static auto
    Get_TransitionBackgroundImages() -> const TArray<FSoftObjectPath>&;

    static auto
    Get_TransitionBackgroundTint() -> FLinearColor;

    static auto
    Get_TransitionBackdropStretch() -> EStretch::Type;

    static auto
    Get_TransitionTipTexts() -> const TArray<FText>&;

    static auto
    Get_TransitionTipStyle() -> const FSoftClassPath&;

    static auto
    Get_TransitionTipHAlign() -> EHorizontalAlignment;

    static auto
    Get_TransitionTipVAlign() -> EVerticalAlignment;

    static auto
    Get_TransitionTipPadding() -> FMargin;

    static auto
    Get_TransitionTipWrapTextAt() -> float;

    static auto
    Get_TransitionTipJustification() -> ETextJustify::Type;

    static auto
    Get_StartupMoviePaths() -> const TArray<FString>&;

    static auto
    Get_TransitionWaitForGameThreadScreen() -> bool;

    static auto
    Get_TransitionModeA_UMGHandOff() -> bool;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Script-visible half of the settings surface.
 *
 * UCk_Utils_LoadingScreen_Settings_UE above is plain statics, so Blueprint and AngelScript cannot
 * see it. These exist so the GAME-THREAD loading screen widget can fall back to the same look
 * tokens the MoviePlayer transition screen uses — one source of defaults for both renderers, which
 * is what keeps the two halves of a travel looking like one screen.
 */
UCLASS(NotBlueprintable)
class CKLOADINGSCREEN_API UCk_Utils_LoadingScreen_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|LoadingScreen",
              DisplayName = "[Ck][LoadingScreen] Get Transition Background Images")
    static TArray<TSoftObjectPtr<UTexture2D>>
    Get_TransitionBackgroundImages();

    UFUNCTION(BlueprintPure,
              Category = "Ck|LoadingScreen",
              DisplayName = "[Ck][LoadingScreen] Get Transition Tip Texts")
    static TArray<FText>
    Get_TransitionTipTexts();

    UFUNCTION(BlueprintPure,
              Category = "Ck|LoadingScreen",
              DisplayName = "[Ck][LoadingScreen] Get Transition Logo Throb Period")
    static float
    Get_TransitionLogoThrobPeriod();

    UFUNCTION(BlueprintPure,
              Category = "Ck|LoadingScreen",
              DisplayName = "[Ck][LoadingScreen] Get Transition Logo Min Opacity")
    static float
    Get_TransitionLogoMinOpacity();
};

// --------------------------------------------------------------------------------------------------------------------
