#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGameSettings/CkGameSettings_Common.h"

#include "CkUI/UserWidget/CkUserWidget.h"

#include <CommonButtonBase.h>

#include "CkGameSettingsUI_RowWidgets.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCheckBox;
class USlider;
class UTextBlock;

// --------------------------------------------------------------------------------------------------------------------

/**
 * One settings-screen row bound to ONE registered setting by key. The row owns the MECHANISM
 * (subsystem change-subscription lifecycle, typed read/commit, disabled state from edit
 * conditions); the WBP owns the tree — author it and bind the subclass's optional widgets.
 *
 * Rows never cache the definition: it is re-queried by key on every refresh (the registry may
 * re-register a key with a different shape).
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class CKGAMESETTINGS_API UCk_GameSettingsUI_RowWidgetBase : public UCk_UserWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettingsUI_RowWidgetBase);

public:
    /** Bind this row to a registered setting — displays the current value, tracks changes.
     *  Rebinding to a different key unsubscribes the previous one. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|GameSettings|Row",
              DisplayName = "[Ck][GameSettings] Inject Setting")
    void
    InjectSetting(
        const UObject* InWorldContextObject,
        FName InKey);

    /** Design-time/preview path: display a definition + value with NO subsystem subscription. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|GameSettings|Row",
              DisplayName = "[Ck][GameSettings] Inject Preview")
    void
    InjectPreview(
        const FCk_GameSettings_SettingDefinition& InDefinition,
        const FString& InValueString);

    UFUNCTION(BlueprintPure,
              Category = "Ck|UI|GameSettings|Row")
    FName
    Get_SettingKey() const;

protected:
    auto NativeDestruct() -> void override;

protected:
    /** Push the definition + current value into the subclass's value controls. */
    virtual auto DoRefreshControl(const FCk_GameSettings_SettingDefinition& InDefinition, const FString& InValueString) -> void CK_PURE_VIRTUAL(DoRefreshControl);

protected:
    auto DoRefreshFromRegistry() -> void;
    auto DoRefreshCommon(const FCk_GameSettings_SettingDefinition& InDefinition) -> void;
    auto Get_WorldContext() const -> const UObject*;
    auto Get_CurrentValueString(const FCk_GameSettings_SettingDefinition& InDefinition) const -> FString;

private:
    UFUNCTION()
    void
    HandleSettingChanged(
        FName InKey,
        const FString& InNewValue);

protected:
    /** Bind a TextBlock named _DisplayNameText in the WBP to receive the setting's display name. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UTextBlock> _DisplayNameText;

    UPROPERTY(Transient, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FName _Key;

    UPROPERTY(Transient)
    TWeakObjectPtr<const UObject> _WorldContextObject;

    bool _RefreshingControl = false;

public:
    CK_PROPERTY_GET(_Key);
};

// --------------------------------------------------------------------------------------------------------------------

/** Bool setting → checkbox. Designer slot: _ValueCheckBox. */
UCLASS(BlueprintType, Blueprintable)
class CKGAMESETTINGS_API UCk_GameSettingsUI_RowWidget_Toggle : public UCk_GameSettingsUI_RowWidgetBase
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettingsUI_RowWidget_Toggle);

protected:
    auto DoRefreshControl(const FCk_GameSettings_SettingDefinition& InDefinition, const FString& InValueString) -> void override;

private:
    UFUNCTION()
    void
    HandleCheckStateChanged(
        bool InIsChecked);

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCheckBox> _ValueCheckBox;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Int32/Float setting WITH a full [min, max] range → slider + numeric readout. Designer slots:
 * _ValueSlider, _ValueText. Dragging previews the readout only; the persistence-affecting
 * Request_SetSettingValue_* fires on handle release.
 */
UCLASS(BlueprintType, Blueprintable)
class CKGAMESETTINGS_API UCk_GameSettingsUI_RowWidget_Slider : public UCk_GameSettingsUI_RowWidgetBase
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettingsUI_RowWidget_Slider);

protected:
    auto DoRefreshControl(const FCk_GameSettings_SettingDefinition& InDefinition, const FString& InValueString) -> void override;

private:
    UFUNCTION()
    void
    HandleValueChanged(
        float InNewValue);

    UFUNCTION()
    void
    HandleCaptureBegin();

    UFUNCTION()
    void
    HandleCaptureEnd();

private:
    auto DoUpdateReadout(float InValue, ECk_GameSettings_ValueType InValueType) -> void;
    auto DoCommitValue(float InValue) -> void;

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<USlider> _ValueSlider;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UTextBlock> _ValueText;

    bool _DragInProgress = false;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Spinner-style prev/next + label (gamepad-friendly). Resolved for: any setting with _Options,
 * and numeric settings without a full range (steps by 1). A String setting without options is
 * display-only (nothing to cycle) — supply a row-class override for free-text entry.
 * Designer slots: _PrevButton, _NextButton, _ValueText.
 */
UCLASS(BlueprintType, Blueprintable)
class CKGAMESETTINGS_API UCk_GameSettingsUI_RowWidget_Select : public UCk_GameSettingsUI_RowWidgetBase
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettingsUI_RowWidget_Select);

protected:
    auto DoRefreshControl(const FCk_GameSettings_SettingDefinition& InDefinition, const FString& InValueString) -> void override;

private:
    auto HandlePrevClicked() -> void;
    auto HandleNextClicked() -> void;

    auto DoStep(int32 InDirection) -> void;
    auto DoCommitOptionValue(const FCk_GameSettings_SettingDefinition& InDefinition, const FString& InOptionValue) -> void;
    auto Get_CurrentOptionIndex(const FCk_GameSettings_SettingDefinition& InDefinition, const FString& InValueString) const -> int32;

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> _PrevButton;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> _NextButton;

    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UTextBlock> _ValueText;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::game_settings_ui
{
    /** Idempotent click binding for an OPTIONAL bound button — unbound slots are simply skipped. */
    template <typename T_Listener>
    auto
        BindClick(
            UCommonButtonBase* InButton,
            T_Listener* InListener,
            void (T_Listener::*InHandler)()) -> void
    {
        if (ck::Is_NOT_Valid(InButton))
        { return; }

        InButton->OnClicked().RemoveAll(InListener);
        InButton->OnClicked().AddUObject(InListener, InHandler);
    }

    /** How a setting's edit conditions dispose it on a settings screen. */
    enum class EEditDisposition
    {
        Editable,

        // Required platform traits missing AND a player-facing _DisabledReason is authored:
        // render the row disabled with the reason as tooltip.
        Disabled,

        // Required platform traits missing with NO authored reason: omit the row entirely.
        Hidden
    };

    CKGAMESETTINGS_API auto
    Get_EditDisposition(
        const FCk_GameSettings_EditConditions& InEditConditions,
        const FGameplayTagContainer& InPlatformTraits) -> EEditDisposition;

    /** Reads the platform traits from CommonUI's settings. */
    CKGAMESETTINGS_API auto
    Get_EditDisposition(
        const FCk_GameSettings_EditConditions& InEditConditions) -> EEditDisposition;

    /**
     * Type→row-class mapping: options-present → _SelectRowClassOverride else built-in Select;
     * otherwise per-type override else built-in default (Bool → Toggle; numeric with a FULL
     * [min, max] range → Slider; everything else → Select).
     */
    CKGAMESETTINGS_API auto
    Get_ResolvedRowClass(
        const FCk_GameSettings_SettingDefinition& InDefinition,
        const TMap<ECk_GameSettings_ValueType, TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>>& InRowClassOverrides,
        const TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>& InSelectRowClassOverride) -> TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>;

    /** Project-settings-driven overload of the above. */
    CKGAMESETTINGS_API auto
    Get_ResolvedRowClass(
        const FCk_GameSettings_SettingDefinition& InDefinition) -> TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>;
}

// --------------------------------------------------------------------------------------------------------------------
