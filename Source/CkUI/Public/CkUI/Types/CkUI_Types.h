// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>

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
struct CKUI_API FCk_UI_InputSuspensionToken
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_UI_InputSuspensionToken);

    FCk_UI_InputSuspensionToken() = default;

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
        const ULocalPlayer* InLocalPlayer) -> FCk_UI_InputSuspensionToken;


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

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_UI_InputSuspensionToken, [](const FCk_UI_InputSuspensionToken& InHandle)
{
    return ck::Format(TEXT("Id:[{}] | Reason:[{}]"), InHandle.Get_Id(), InHandle.Get_Reason());
});

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_UI_InputSuspensionToken, IsValid_Policy_Default,
[](const FCk_UI_InputSuspensionToken& InHandle)
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