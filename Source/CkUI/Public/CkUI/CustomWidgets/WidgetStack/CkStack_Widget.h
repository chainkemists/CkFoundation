// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include <CommonActivatableWidget.h>
#include <Widgets/CommonActivatableWidgetContainer.h>

#include "CkStack_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Base widget stack with common functionality.
 * Works with any UCommonActivatableWidget type.
 *
 * Extends UCommonActivatableWidgetStack with:
 * - Virtual hooks for push/pop operations (OnPreWidgetPush, etc.)
 * - Native delegates for widget events
 * - Convenience query methods
 *
 * Note: Use PushWidgetClass/PushWidgetInstance/PopWidget/PopSpecificWidget/ClearAllWidgets
 * instead of base class methods to ensure hooks are called.
 */
UCLASS(BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_Stack_UserWidget_UE : public UCommonActivatableWidgetStack
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Stack_UserWidget_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Operations (with hooks)
    // ----------------------------------------------------------------------------------------------------------------

public:
    /**
     * Creates and pushes a widget of the given class.
     * Calls OnPreWidgetPush before adding, OnPostWidgetPush after.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|StackWidget",
        meta = (DeterminesOutputType = "InWidgetClass", DynamicOutputParam = "ReturnValue"))
    UCommonActivatableWidget* PushWidgetClass(TSubclassOf<UCommonActivatableWidget> InWidgetClass);

    /**
     * Pushes an existing widget instance.
     * Calls OnPreWidgetPush before adding, OnPostWidgetPush after.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|StackWidget")
    UCommonActivatableWidget* PushWidgetInstance(UCommonActivatableWidget* InWidgetInstance);

    /**
     * Removes and returns the top widget.
     * Calls OnPreWidgetPop before removing, OnPostWidgetPop after.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|StackWidget")
    UCommonActivatableWidget* PopWidget();

    /**
     * Removes a specific widget from the stack.
     * Calls OnPreWidgetPop before removing, OnPostWidgetPop after.
     * @return true if the widget was found and removed.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|StackWidget")
    bool PopSpecificWidget(UCommonActivatableWidget* InWidget);

    // ----------------------------------------------------------------------------------------------------------------
    // Query Operations
    // ----------------------------------------------------------------------------------------------------------------

public:
    /** Returns the topmost widget, or nullptr if empty. */
    UFUNCTION(BlueprintPure, Category = "Ck|UI|StackWidget")
    UCommonActivatableWidget* Get_TopWidget() const;

    /** Returns true if the stack has any widgets (including deactivated ones in transition). */
    UFUNCTION(BlueprintPure, Category = "Ck|UI|StackWidget")
    bool HasWidgets() const;

    /**
     * Returns true if the stack has any activated widgets.
     * Unlike HasWidgets(), this ignores widgets that are deactivated (e.g., mid-pop transition).
     * Use this for input routing decisions.
     */
    UFUNCTION(BlueprintPure, Category = "Ck|UI|StackWidget")
    bool HasActiveWidgets() const;

    /** Returns true if the given widget is in this stack. */
    UFUNCTION(BlueprintPure, Category = "Ck|UI|StackWidget")
    bool ContainsWidget(UCommonActivatableWidget* InWidget) const;

    // ----------------------------------------------------------------------------------------------------------------
    // Configuration
    // ----------------------------------------------------------------------------------------------------------------

public:
    /** Sets all transition properties at once. */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|StackWidget")
    void SetTransitionSettings(
        ECommonSwitcherTransition InTransitionType,
        ETransitionCurve InTransitionCurve,
        FCk_Time InTransitionDuration,
        ECommonSwitcherTransitionFallbackStrategy InFallbackStrategy = ECommonSwitcherTransitionFallbackStrategy::None);

    // ----------------------------------------------------------------------------------------------------------------
    // Events - Native
    // ----------------------------------------------------------------------------------------------------------------

public:
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnWidgetPushed, UCommonActivatableWidget*);
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnWidgetPopped, UCommonActivatableWidget*);

    FOnWidgetPushed OnWidgetPushed;
    FOnWidgetPopped OnWidgetPopped;

    // ----------------------------------------------------------------------------------------------------------------
    // UWidget Overrides
    // ----------------------------------------------------------------------------------------------------------------

protected:
#if WITH_EDITOR
    virtual auto GetPaletteCategory() -> const FText override;
#endif

    // ----------------------------------------------------------------------------------------------------------------
    // Extensibility Hooks
    // ----------------------------------------------------------------------------------------------------------------

protected:
    /** Called before a widget is added to the stack. */
    virtual auto OnPreWidgetPush(UCommonActivatableWidget* InWidget) -> void {}

    /** Called after a widget is added to the stack. */
    virtual auto OnPostWidgetPush(UCommonActivatableWidget* InWidget) -> void {}

    /** Called before a widget is removed from the stack. */
    virtual auto OnPreWidgetPop(UCommonActivatableWidget* InWidget) -> void {}

    /** Called after a widget is removed from the stack. */
    virtual auto OnPostWidgetPop(UCommonActivatableWidget* InWidget) -> void {}
};

// --------------------------------------------------------------------------------------------------------------------