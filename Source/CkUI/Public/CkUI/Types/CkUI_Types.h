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
class UCk_UI_Subsystem_UE;

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
struct CKUI_API FCk_UI_NavigationConfig
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

/**
 * Opaque handle representing an active input suspension.
 *
 * Created via UCk_Utils_UI_UE::SuspendInput() or UCk_UI_Subsystem_UE::SuspendInput().
 * Must be explicitly resumed via Resume() or UCk_Utils_UI_UE::ResumeInput().
 *
 * The handle tracks:
 * - Unique ID for this suspension instance
 * - The underlying CommonInputSubsystem token
 * - Weak reference to the owning LocalPlayer
 *
 * Invalid handles (default constructed or already resumed) are safe to use -
 * Resume() will simply no-op.
 */
USTRUCT(BlueprintType)
struct CKUI_API FCk_Handle_InputSuspension
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_InputSuspension);

    FCk_Handle_InputSuspension() = default;

    auto IsValid() const -> bool;
    auto Resume() -> void;
    auto Get_LocalPlayer() const -> const ULocalPlayer*;

    auto operator==(const ThisType& Other) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    friend class UCk_UI_Subsystem_UE;

    static auto Create(
        uint32 InId,
        FName InReason,
        FName InToken,
        const ULocalPlayer* InLocalPlayer) -> FCk_Handle_InputSuspension;


    auto DoMarkInvalid() -> void;

private:
    uint32 _Id = 0;

    UPROPERTY()
    FName _Reason = NAME_None;

    UPROPERTY()
    FName _Token = NAME_None;

    UPROPERTY()
    TWeakObjectPtr<const ULocalPlayer> _LocalPlayer;

public:
    CK_PROPERTY_GET(_Id);
    CK_PROPERTY_GET(_Reason);
    CK_PROPERTY_GET(_Token);
};

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_Handle_InputSuspension, [](const FCk_Handle_InputSuspension& InHandle)
{
    return ck::Format(TEXT("Id:[{}] | Reason:[{}]"), InHandle.Get_Id(), InHandle.Get_Reason());
});

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_Handle_InputSuspension, IsValid_Policy_Default,
[](const FCk_Handle_InputSuspension& InHandle)
{
    return InHandle.IsValid();
});

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