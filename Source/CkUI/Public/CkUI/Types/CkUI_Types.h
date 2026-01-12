// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"

#include <GameplayTagContainer.h>

#include "CkUI_Types.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCommonActivatableWidget;
class AActor;

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
// Context
// --------------------------------------------------------------------------------------------------------------------

/**
 * Context data that can be injected into widgets.
 * Provides a flexible way to pass entity, actor, or arbitrary object references.
 */
USTRUCT(BlueprintType)
struct CKUI_API FCk_UI_Context
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_UI_Context);

    FCk_UI_Context() = default;

    static auto MakeFromEntity(FCk_Handle InEntity) -> FCk_UI_Context;
    static auto MakeFromActor(AActor* InActor) -> FCk_UI_Context;
    static auto MakeFromObject(UObject* InObject) -> FCk_UI_Context;

    auto IsValid() const -> bool;
    auto HasEntity() const -> bool;
    auto HasActor() const -> bool;
    auto HasPayload() const -> bool;

    auto Reset() -> void;

    auto operator==(const ThisType& Other) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);


private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle _Entity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<AActor> _Actor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UObject> _Payload;

public:
    CK_PROPERTY_GET(_Entity);
    CK_PROPERTY_GET(_Actor);
    CK_PROPERTY_GET(_Payload);
};

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_UI_Context, [](const FCk_UI_Context& InContext)
{
    return ck::Format(TEXT("Entity:[{}] Actor:[{}] Payload:[{}]"),
        InContext.Get_Entity(),
        InContext.Get_Actor().Get(),
        InContext.Get_Payload().Get());
});

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_UI_Context, IsValid_Policy_Default,
[=](const FCk_UI_Context& InContext)
{
    return InContext.IsValid();
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