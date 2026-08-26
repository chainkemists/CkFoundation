// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include <CommonActivatableWidget.h>
#include <Widgets/CommonActivatableWidgetContainer.h>
#include <UObject/ObjectKey.h>

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
class CKUI_API UCk_WidgetStack_UE : public UCommonActivatableWidgetStack
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_WidgetStack_UE);

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

    /**
     * Removes all widgets from the stack.
     * Calls OnPreWidgetPop/OnPostWidgetPop for each widget removed.
     * Broadcasts OnAllWidgetsCleared once after all widgets are removed.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|StackWidget")
    void ClearAllWidgets();

    /**
     * Builds the widget AND its full Slate tree for the given class through the container's own
     * pool, then returns it to the pool as an inactive instance while this stack keeps the Slate
     * tree alive — so the first real push (and every later one) reuses both the UObject and the
     * Slate with no RebuildWidget. Only classes opted in via
     * UCk_ActivatableWidget_UE::_KeepSlateAliveWhenPooled are warmed; anything else is a no-op
     * (warming a class whose Slate the release would drop again buys nothing).
     * Construct fires here, once per Slate build — the class must tolerate that (see the flag's
     * contract note).
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|StackWidget")
    void PreWarmWidgetClass(TSubclassOf<UCommonActivatableWidget> InWidgetClass);

    // ----------------------------------------------------------------------------------------------------------------

public:
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

public:
    /** Sets all transition properties at once. */
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|StackWidget")
    void SetTransitionSettings(
        ECommonSwitcherTransition InTransitionType,
        ETransitionCurve InTransitionCurve,
        FCk_Time InTransitionDuration,
        ECommonSwitcherTransitionFallbackStrategy InFallbackStrategy = ECommonSwitcherTransitionFallbackStrategy::None);

    // ----------------------------------------------------------------------------------------------------------------

public:
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnWidgetPushed, UCommonActivatableWidget*);
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnWidgetPopped, UCommonActivatableWidget*);
    DECLARE_MULTICAST_DELEGATE(FOnAllWidgetsCleared);

    FOnWidgetPushed OnWidgetPushed;
    FOnWidgetPopped OnWidgetPopped;
    FOnAllWidgetsCleared OnAllWidgetsCleared;

    // ----------------------------------------------------------------------------------------------------------------

protected:
#if WITH_EDITOR
    virtual auto GetPaletteCategory() -> const FText override;
#endif

    virtual auto ReleaseSlateResources(bool bReleaseChildren) -> void override;

    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoTryKeepSlateAlive(UCommonActivatableWidget* InWidget) -> void;
    auto DoForgetKeptSlate(UCommonActivatableWidget* InWidget) -> void;

    /**
     * Slate trees deliberately kept alive across release-to-pool for opted-in widget classes
     * (UCk_ActivatableWidget_UE::_KeepSlateAliveWhenPooled). The CommonUI pool retains the
     * UObject on release but drops its Slate ref (bReleaseSlate=true), so without this every
     * reopen pays a full RebuildWidget of the authored tree — 25+ ms for a large panel. The held
     * SObjectWidget also roots its UUserWidget for GC. Entries are dropped when their widget is
     * pushed (the container owns the tree while displayed) and re-added on pop; the whole map is
     * cleared with this stack's own Slate.
     */
    TMap<FObjectKey, TSharedPtr<SWidget>> _KeptAliveSlate;

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