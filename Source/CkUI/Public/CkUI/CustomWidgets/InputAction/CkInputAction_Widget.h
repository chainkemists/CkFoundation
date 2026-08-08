#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <CommonActionWidget.h>
#include <CoreMinimal.h>
#include <Styling/SlateBrush.h>
#include <UserSettings/EnhancedInputUserSettings.h>

#include "CkInputAction_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCk_InputAction_ResolvedKeyChangedEvent, FKey, NewKey);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_InputPrompt_Resolution : uint8
{
    // Reads the player's key profile first, then falls back to the parent's applied-context query.
    // Resolves while the action's Mapping Context is NOT applied, and honors Slot.
    KeyProfileThenAppliedContexts,

    // The parent's behavior verbatim: first key from the APPLIED Mapping Contexts that suits the
    // current device. Yields nothing whenever the owning Mapping Context is not applied, and has
    // no concept of a Slot.
    AppliedContextsOnly
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InputPrompt_Resolution);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_InputPrompt_UnboundPolicy : uint8
{
    // The parent's behavior: the widget collapses when no icon resolves.
    Collapse,

    // Draws UnboundBrush instead, so the prompt keeps its footprint and reads as 'not bound'.
    ShowUnboundBrush,

    // Stays laid out with no icon. For a rebinding row that draws its own 'Unbound' label from
    // Get_ResolvedKey / the OnResolvedKeyChanged event.
    KeepVisible
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InputPrompt_UnboundPolicy);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Input action prompt that resolves its glyph through the player's mappable key profile.
 *
 * UCommonActionWidget already refreshes on device swap AND on rebind (it listens to
 * UEnhancedInputLocalPlayerSubsystem::ControlMappingsRebuiltDelegate). What it cannot do is
 * resolve a key that no APPLIED Mapping Context provides: its icon lookup runs through
 * QueryKeysMappedToAction, so an action whose IMC is inactive — every row of a rebinding
 * screen, and any context-sensitive prompt shown ahead of its context — silently collapses.
 * It also takes the first key suiting the current device, so a secondary binding is
 * unreachable.
 *
 * This subclass resolves through UEnhancedPlayerMappableKeyProfile instead, which is
 * independent of which contexts are applied and is addressed by Slot, then falls back to the
 * parent lookup. It adds an unbound policy because the parent hard-collapses on a missing
 * glyph, and it re-resolves on key-profile changes that leave the applied contexts untouched.
 *
 * Requires the Input Action to carry Player Mappable Key Settings — that is what names the
 * mapping. Without them, resolution falls through to the parent's behavior.
 */
UCLASS(meta = (DisplayName = "Ck Input Action Prompt"))
class CKUI_API UCk_InputActionWidget_UE : public UCommonActionWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InputActionWidget_UE);

public:
    // Fires whenever the resolved key changes, including to an invalid key when the action
    // becomes unbound. Drive a key-name label from this.
    UPROPERTY(BlueprintAssignable, Category = "Ck|UI|InputAction|Events")
    FCk_InputAction_ResolvedKeyChangedEvent OnResolvedKeyChanged;

public:
    // The key this prompt currently displays. Invalid when the action is unbound.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|InputAction")
    FKey
    Get_ResolvedKey() const;

    // Localized display name of the resolved key ("Left Shift"), for a text fallback when no
    // brush is registered for it. Empty when the action is unbound.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|InputAction")
    FText
    Get_ResolvedKeyDisplayName() const;

    // Whether the action resolved to a key at all. False means unbound, not 'no artwork'.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|InputAction")
    bool
    Get_IsBound() const;

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|InputAction")
    void
    Request_SetSlot(
        EPlayerMappableKeySlot InSlot);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|InputAction")
    void
    Request_SetUnboundPolicy(
        ECk_InputPrompt_UnboundPolicy InUnboundPolicy);

public:
    auto GetIcon() const -> FSlateBrush override;
    auto ReleaseSlateResources(bool bReleaseChildren) -> void override;

#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif

protected:
    auto UpdateActionWidget() -> void override;
    auto OnWidgetRebuilt() -> void override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InputAction",
              meta = (AllowPrivateAccess = true))
    ECk_InputPrompt_Resolution _Resolution = ECk_InputPrompt_Resolution::KeyProfileThenAppliedContexts;

    // Which binding slot to display. Only consulted by the key-profile path.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InputAction",
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_Resolution == ECk_InputPrompt_Resolution::KeyProfileThenAppliedContexts",
                      EditConditionHides))
    EPlayerMappableKeySlot _Slot = EPlayerMappableKeySlot::First;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InputAction",
              meta = (AllowPrivateAccess = true))
    ECk_InputPrompt_UnboundPolicy _UnboundPolicy = ECk_InputPrompt_UnboundPolicy::Collapse;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InputAction",
              meta = (AllowPrivateAccess = true,
                      EditCondition = "_UnboundPolicy == ECk_InputPrompt_UnboundPolicy::ShowUnboundBrush",
                      EditConditionHides))
    FSlateBrush _UnboundBrush;

public:
    CK_PROPERTY_GET(_Resolution);
    CK_PROPERTY_GET(_Slot);
    CK_PROPERTY_GET(_UnboundPolicy);
    CK_PROPERTY_GET(_UnboundBrush);

private:
    auto DoResolveKey() const -> FKey;
    auto DoResolveKey_FromKeyProfile() const -> FKey;
    auto DoRefreshResolvedKey() -> void;

    auto DoListenToKeyProfileChanged(bool InListen) -> void;

    UFUNCTION()
    void
    HandleInputUserSettingsChanged(
        UEnhancedInputUserSettings* InSettings);

private:
    // Last key broadcast through OnResolvedKeyChanged. Its unset state is what makes the first
    // resolve broadcast even when the action is already unbound.
    TOptional<FKey> _ResolvedKey;

    // Unsubscribing re-queries nothing: teardown can run after the PlayerController is gone, and
    // the CkInput accessors ensure on a missing controller/subsystem.
    TWeakObjectPtr<UEnhancedInputUserSettings> _BoundUserSettings;
};

// --------------------------------------------------------------------------------------------------------------------
