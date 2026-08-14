#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkInput/CkKeyBinding_Utils.h"

#include "CkUI/UserWidget/CkUserWidget.h"

#include "CkGameSettingsUI_KeyBindingPageWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCommonButtonBase;
class UImage;
class UPanelWidget;
class UTextBlock;
class UWidget;

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_GameSettingsUI_RebindRequested,
    FName, InMappingName,
    EPlayerMappableKeySlot, InSlot);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_GameSettingsUI_ConflictResolution : uint8
{
    // Give the conflicting mapping this row's old key, take the new one.
    Swap,

    // Unbind the conflicting mapping, take the new key.
    Overwrite,

    // Keep everything as it was.
    Cancel
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GameSettingsUI_ConflictResolution);

// --------------------------------------------------------------------------------------------------------------------

/**
 * One remappable-key row: display name + current key (glyph when the platform controller data has
 * one, key name otherwise) + per-row reset. The WBP owns the tree — bind _NameText, _KeyButton
 * (with _KeyIconImage/_KeyText inside it), and _ResetButton. Refreshes live on remap via the
 * KeyBinding subsystem's per-mapping listener.
 */
UCLASS(BlueprintType, Blueprintable)
class CKGAMESETTINGS_API UCk_GameSettingsUI_KeyBindingRowWidget : public UCk_UserWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettingsUI_KeyBindingRowWidget);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|GameSettings|KeyBinding",
              DisplayName = "[Ck][GameSettings] Inject Mapping")
    void
    InjectMapping(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        const FText& InDisplayName);

public:
    UPROPERTY(BlueprintAssignable,
              Category = "Ck|UI|GameSettings|KeyBinding")
    FCk_Delegate_GameSettingsUI_RebindRequested OnRebindRequested;

protected:
    auto NativeDestruct() -> void override;

private:
    auto HandleKeyClicked() -> void;
    auto HandleResetClicked() -> void;

    UFUNCTION()
    void
    HandleMappingKeyChanged(
        FName InMappingName,
        FKey InOldKey,
        FKey InNewKey);

private:
    auto DoRefreshKeyDisplay() -> void;

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UTextBlock> _NameText;

    /** Clicking it starts key capture on the owning page — required: without it the row cannot
     *  initiate a rebind. (_KeyIconImage/_KeyText stay optional — either alone is a valid look.) */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidget, AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> _KeyButton;

    /** Receives the platform glyph brush when one exists; collapsed otherwise. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UImage> _KeyIconImage;

    /** Receives the key's display name when no glyph exists; collapsed otherwise. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UTextBlock> _KeyText;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> _ResetButton;

    UPROPERTY(Transient)
    TWeakObjectPtr<APlayerController> _PlayerController;

    UPROPERTY(Transient)
    FName _MappingName;

    EPlayerMappableKeySlot _Slot = EPlayerMappableKeySlot::First;

    FCk_Handle_KeybindListener _ChangeListener;
    bool _ListenerBound = false;

public:
    CK_PROPERTY_GET(_MappingName);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * The keybinding settings page: rows enumerated from the active key profile's remappable mappings
 * (in display-category order), modal key capture (keyboard + gamepad, Escape cancels), conflict
 * resolution (swap / overwrite / cancel), per-row and page-wide reset, save-on-close. A PURE
 * consumer of UCk_Utils_KeyBinding_UE / UCk_Utils_KeyIcon_UE — no bespoke input plumbing.
 *
 * The WBP owns the tree — bind _RowContainer (required) and the optional pieces; set
 * _RowWidgetClass to the game's row WBP. Category headers are the WBP's job: hook OnRowCreated
 * and insert a header widget whenever the category changes.
 */
UCLASS(BlueprintType, Blueprintable)
class CKGAMESETTINGS_API UCk_GameSettingsUI_KeyBindingPageWidget : public UCk_UserWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettingsUI_KeyBindingPageWidget);

public:
    /** Re-enumerates the remappable mappings into rows. Called automatically on construct. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|GameSettings|KeyBinding",
              DisplayName = "[Ck][GameSettings] Request Rebuild Rows")
    void
    Request_RebuildRows();

    UFUNCTION(BlueprintPure,
              Category = "Ck|UI|GameSettings|KeyBinding")
    int32
    Get_GeneratedRowCount() const;

    /** Resolves the pending conflict surfaced by OnConflictDetected (or the bound overlay).
     *  Rejected when no conflict is pending. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|GameSettings|KeyBinding",
              DisplayName = "[Ck][GameSettings] Request Resolve Conflict")
    void
    Request_ResolveConflict(
        ECk_GameSettingsUI_ConflictResolution InResolution);

protected:
    /** Fired for every row after it is injected, in category order — insert category headers here. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|GameSettings|KeyBinding")
    void
    OnRowCreated(
        UCk_GameSettingsUI_KeyBindingRowWidget* InRow,
        const FText& InCategory);

    /** Conflict presentation is the game's choice: bind _ConflictOverlay for the inline treatment,
     *  or leave it unbound and this fires instead — present your own modal (a Confirm dialog on a
     *  modal layer) and answer with Request_ResolveConflict. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|GameSettings|KeyBinding")
    void
    OnConflictDetected(
        const FText& InDescription);

protected:
    auto NativeConstruct() -> void override;
    auto NativeDestruct() -> void override;
    auto NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) -> FReply override;
    auto NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) -> FReply override;

private:
    UFUNCTION()
    void
    HandleRebindRequested(
        FName InMappingName,
        EPlayerMappableKeySlot InSlot);

private:
    auto HandleResetAllClicked() -> void;
    auto HandleSwapClicked() -> void;
    auto HandleOverwriteClicked() -> void;
    auto HandleConflictCancelClicked() -> void;

private:
    auto DoBeginCapture(FName InMappingName, EPlayerMappableKeySlot InSlot) -> void;
    auto DoCancelCapture() -> void;
    auto DoAttemptRebind(const FKey& InNewKey) -> void;
    auto DoFinishRebind(bool InSucceeded, const FGameplayTagContainer& InFailureReason) -> void;
    auto DoSetStatus(const FText& InStatus) -> void;
    auto DoSetConflictOverlayVisible(bool InVisible) -> void;

private:
    /** The panel key rows are injected into — required (rows have no injection point without it).
     *  WBP-compile-enforced; headless native instantiation still runs with it null. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidget, AllowPrivateAccess = true))
    TObjectPtr<UPanelWidget> _RowContainer;

    /** Receives rebind failure reasons and reset confirmations. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UTextBlock> _StatusText;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> _ResetAllButton;

    /** Shown while capturing a key; any styled widget works. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UWidget> _CaptureOverlay;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UTextBlock> _CapturePromptText;

    /** Shown while a conflict awaits resolution; hosts the three conflict buttons. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UWidget> _ConflictOverlay;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UTextBlock> _ConflictText;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> _SwapButton;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> _OverwriteButton;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> _ConflictCancelButton;

    /** The row widget class instantiated per remappable mapping — set it to the game's row WBP. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|GameSettings|KeyBinding",
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_GameSettingsUI_KeyBindingRowWidget> _RowWidgetClass = UCk_GameSettingsUI_KeyBindingRowWidget::StaticClass();

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_GameSettingsUI_KeyBindingRowWidget>> _Rows;

    bool _CaptureActive = false;
    bool _ConflictPending = false;
    FName _PendingMappingName;
    EPlayerMappableKeySlot _PendingSlot = EPlayerMappableKeySlot::First;
    FKey _PendingKey;
};

// --------------------------------------------------------------------------------------------------------------------
