// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>
#include <InputCoreTypes.h>

#include "CkUI_Types.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCommonActivatableWidget;
class AActor;
class ULocalPlayer;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::widget_palette_categories
{
    const FText Default = NSLOCTEXT("CkUI", "WidgetPaletteCategory", "CkFoundation Plugin");
}

// --------------------------------------------------------------------------------------------------------------------
// Enums
// --------------------------------------------------------------------------------------------------------------------

/**
 * Describes how input should be routed between game and UI.
 */
UENUM(BlueprintType)
enum class ECk_UI_InputMode : uint8
{
    GameOnly,    // Input goes to game only, no cursor
    GameAndUI,   // Input shared between game and UI, cursor visible
    UIOnly       // Input captured by UI, game input blocked
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_InputMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_Widget_ViewportOperation : uint8
{
    DoNothing,
    AddToViewport,
    RemoveFromViewport
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_Widget_ViewportOperation);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Mirrors the tunable public surface of Slate's FNavigationConfig.
 *
 * Defaults match a freshly-constructed FNavigationConfig, so passing a default-constructed instance
 * to Request_SetNavigationConfig restores stock Slate navigation.
 *
 * The digital rule maps (KeyEventRules / KeyActionRules) are deliberately NOT mirrored: the
 * FNavigationConfig constructor repopulates them, so arrow-key navigation and the Accept/Back
 * actions keep their engine defaults regardless of what is set here.
 *
 * Why this exists: a game that binds Tab (or an arrow key) as a UI/gameplay input has to stop Slate
 * claiming it first. FNavigationConfig::GetNavigationDirectionFromKey consumes those keys before the
 * event can bubble to the game viewport, so e.g. a CommonUI action-router binding on Tab never fires
 * while Escape and gamepad face buttons do (the latter route through FCommonAnalogCursor, an input
 * PREPROCESSOR that runs ahead of navigation).
 */
USTRUCT(BlueprintType)
struct CKUICORE_API FCk_UI_NavigationConfig
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_UI_NavigationConfig);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    bool _TabNavigation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    bool _KeyNavigation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    bool _AnalogNavigation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    bool _IgnoreModifiersForNavigationActions = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _AnalogNavigationHorizontalThreshold = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _AnalogNavigationVerticalThreshold = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FKey _AnalogHorizontalKey = EKeys::Gamepad_LeftX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FKey _AnalogVerticalKey = EKeys::Gamepad_LeftY;

public:
    CK_PROPERTY(_TabNavigation);
    CK_PROPERTY(_KeyNavigation);
    CK_PROPERTY(_AnalogNavigation);
    CK_PROPERTY(_IgnoreModifiersForNavigationActions);
    CK_PROPERTY(_AnalogNavigationHorizontalThreshold);
    CK_PROPERTY(_AnalogNavigationVerticalThreshold);
    CK_PROPERTY(_AnalogHorizontalKey);
    CK_PROPERTY(_AnalogVerticalKey);
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_NamedSlot_SearchRecursive : uint8
{
    NonRecursive,
    Recursive
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_NamedSlot_SearchRecursive);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_NamedSlot_EnsureSlotIsFound : uint8
{
    EnsureSlotIsFound,
    AllowSlotNotFound
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_NamedSlot_EnsureSlotIsFound);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_NewMouseVisibility : uint8
{
    DontChange,
    Show,
    Hide,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_NewMouseVisibility);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_NewInputMode : uint8
{
    DontChange,
    GameOnly,
    UIOnly,
    GameAndUI
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_NewInputMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_ForEachWidgetResult : uint8
{
    Continue,
    SkipSubtree
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_ForEachWidgetResult);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_UI_CursorLock_Result : uint8
{
    Success,
    InvalidWidget,
    NoCursorAvailable,   // Slate is absent (headless/server/commandlet) or the user drives no cursor
    WidgetNotOnScreen,   // Widget was never constructed, is collapsed/hidden, or belongs to no window
    WindowNotForeground  // The platform refuses to confine the cursor to a background window
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_UI_CursorLock_Result);

// --------------------------------------------------------------------------------------------------------------------
// Delegates - Widget Operations
// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_UI_OnWidgetReady,
    UCommonActivatableWidget*, InWidget);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_MulticastDelegate_UI_OnWidgetPushed,
    UCommonActivatableWidget*, InWidget);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_MulticastDelegate_UI_OnWidgetPopped,
    UCommonActivatableWidget*, InWidget);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(
    FCk_MulticastDelegate_UI_OnLayerCleared);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_MulticastDelegate_UI_OnInputModeChanged,
    ECk_UI_InputMode, InNewMode);

// Native delegates for C++ usage
DECLARE_MULTICAST_DELEGATE_OneParam(
    FCk_NativeDelegate_UI_OnWidgetPushed,
    UCommonActivatableWidget*);

DECLARE_MULTICAST_DELEGATE_OneParam(
    FCk_NativeDelegate_UI_OnWidgetPopped,
    UCommonActivatableWidget*);

// --------------------------------------------------------------------------------------------------------------------