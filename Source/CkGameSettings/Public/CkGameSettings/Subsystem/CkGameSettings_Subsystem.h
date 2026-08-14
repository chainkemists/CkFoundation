#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGameSettings/CkGameSettings_Common.h"

#include <Containers/Ticker.h>
#include <Subsystems/GameInstanceSubsystem.h>

#include "CkGameSettings_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_GameSettings_Collection_PDA;
class UCk_GameSettings_StorageProvider_UE;
struct FWorldContext;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Headless registry for user-facing game settings: declare -> typed get/set -> change notification.
 *
 * Settings are data, never widgets: every registered setting is enumerable, resettable, and
 * testable with zero UI. Values are stored canonically as strings and converted at the typed
 * access boundary. Registration is atomic per batch/collection: one invalid definition rejects
 * the whole batch and registers nothing.
 */
UCLASS(NotBlueprintable, BlueprintType, DisplayName="CkSubsystem_GameSettings")
class CKGAMESETTINGS_API UCk_GameSettings_Subsystem_UE : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettings_Subsystem_UE);

public:
    auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    auto Deinitialize() -> void override;
    auto ShouldCreateSubsystem(UObject* InOuter) const -> bool override;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Setting")
    bool
    Request_RegisterSetting(
        const FCk_GameSettings_SettingDefinition& InDefinition);

    /** Atomic: every definition is validated first; ONE invalid definition rejects the whole batch and registers nothing. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Settings")
    bool
    Request_RegisterSettings(
        const TArray<FCk_GameSettings_SettingDefinition>& InDefinitions);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Collection")
    bool
    Request_RegisterCollection(
        const UCk_GameSettings_Collection_PDA* InCollection);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Setting Value (Bool)")
    bool
    Get_SettingValue_Bool(
        FName InKey,
        bool InFallback = false) const;

    UFUNCTION(BlueprintPure,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Setting Value (Int32)")
    int32
    Get_SettingValue_Int32(
        FName InKey,
        int32 InFallback = 0) const;

    UFUNCTION(BlueprintPure,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Setting Value (Float)")
    float
    Get_SettingValue_Float(
        FName InKey,
        float InFallback = 0.0f) const;

    UFUNCTION(BlueprintPure,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Setting Value (String)")
    FString
    Get_SettingValue_String(
        FName InKey,
        const FString& InFallback = FString(TEXT(""))) const;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Is Setting Registered")
    bool
    Get_IsSettingRegistered(
        FName InKey) const;

    UFUNCTION(BlueprintPure,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Setting Definition")
    bool
    Get_SettingDefinition(
        FName InKey,
        FCk_GameSettings_SettingDefinition& OutDefinition) const;

    /** Keys in registration order. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Get All Setting Keys")
    TArray<FName>
    Get_AllSettingKeys() const;

    /** Keys (in registration order) whose CategoryTags match the query. An empty query matches every registered setting. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Setting Keys By Category")
    TArray<FName>
    Get_SettingKeysByCategory(
        const FGameplayTagQuery& InCategoryQuery) const;

public:
    /**
     * Validates (registered, type match, range/options), stores the value, and fires the change
     * delegate iff the value actually changed. Returns whether the value now holds — an
     * idempotent same-value set returns true without firing.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Set Setting Value (Bool)")
    bool
    Request_SetSettingValue_Bool(
        const FCk_Request_GameSettings_SetValue_Bool& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Set Setting Value (Int32)")
    bool
    Request_SetSettingValue_Int32(
        const FCk_Request_GameSettings_SetValue_Int32& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Set Setting Value (Float)")
    bool
    Request_SetSettingValue_Float(
        const FCk_Request_GameSettings_SetValue_Float& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Set Setting Value (String)")
    bool
    Request_SetSettingValue_String(
        const FCk_Request_GameSettings_SetValue_String& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Reset To Default")
    bool
    Request_ResetToDefault(
        const FCk_Request_GameSettings_ResetToDefault& InRequest);

    /** Returns how many settings actually changed back to their default. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Reset All To Defaults")
    int32
    Request_ResetAllToDefaults();

public:
    /** InKey == None binds to changes of EVERY setting (wildcard). Binding before the key is registered is legitimate. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Bind To OnSettingChanged")
    void
    BindTo_OnSettingChanged(
        FName InKey,
        const FCk_Delegate_GameSettings_OnSettingChanged& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Unbind From OnSettingChanged")
    void
    UnbindFrom_OnSettingChanged(
        FName InKey,
        const FCk_Delegate_GameSettings_OnSettingChanged& InDelegate);

public:
    /**
     * Registers the apply target for a Handler-bound setting. Registering before the setting's
     * definition exists is legitimate; a handler arriving for a key with a pending stored value
     * applies that value immediately. Returns false (with ensure) only on a known type mismatch.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Apply Handler (Bool)")
    bool
    Request_RegisterApplyHandler_Bool(
        FName InKey,
        const FCk_Delegate_GameSettings_ApplyHandler_Bool& InHandler);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Apply Handler (Int32)")
    bool
    Request_RegisterApplyHandler_Int32(
        FName InKey,
        const FCk_Delegate_GameSettings_ApplyHandler_Int32& InHandler);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Apply Handler (Float)")
    bool
    Request_RegisterApplyHandler_Float(
        FName InKey,
        const FCk_Delegate_GameSettings_ApplyHandler_Float& InHandler);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Apply Handler (String)")
    bool
    Request_RegisterApplyHandler_String(
        FName InKey,
        const FCk_Delegate_GameSettings_ApplyHandler_String& InHandler);

public:
    /**
     * Registers the read/write-through pair for an External-policy setting. External values are
     * NEVER stored in the provider: reads route through InGetter, writes through InSetter.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register External Accessors (Bool)")
    bool
    Request_RegisterExternalAccessors_Bool(
        FName InKey,
        const FCk_Delegate_GameSettings_ExternalGetter_Bool& InGetter,
        const FCk_Delegate_GameSettings_ApplyHandler_Bool& InSetter);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register External Accessors (Int32)")
    bool
    Request_RegisterExternalAccessors_Int32(
        FName InKey,
        const FCk_Delegate_GameSettings_ExternalGetter_Int32& InGetter,
        const FCk_Delegate_GameSettings_ApplyHandler_Int32& InSetter);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register External Accessors (Float)")
    bool
    Request_RegisterExternalAccessors_Float(
        FName InKey,
        const FCk_Delegate_GameSettings_ExternalGetter_Float& InGetter,
        const FCk_Delegate_GameSettings_ApplyHandler_Float& InSetter);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register External Accessors (String)")
    bool
    Request_RegisterExternalAccessors_String(
        FName InKey,
        const FCk_Delegate_GameSettings_ExternalGetter_String& InGetter,
        const FCk_Delegate_GameSettings_ApplyHandler_String& InSetter);

public:
    /** Begins the single staged-changes session. While active, sets apply LIVE (preview) and record priors. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Begin Pending Changes")
    bool
    Request_BeginPendingChanges();

    /** Commits the session: previewed values stay, priors are discarded, storage is flushed. Returns how many settings were committed. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Apply Pending Changes")
    int32
    Request_ApplyPendingChanges();

    /** Reverts the session: every recorded prior is re-applied live. Returns how many settings changed back. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Revert Pending Changes")
    int32
    Request_RevertPendingChanges();

    UFUNCTION(BlueprintPure,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Has Pending Changes")
    bool
    Get_HasPendingChanges() const;

    UFUNCTION(BlueprintPure,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Has Unapplied Change")
    bool
    Get_HasUnappliedChange(
        FName InKey) const;

public:
    /** Registers the Audio pack from the project-settings config (SoundMix + categories). Idempotent per key. Returns how many categories were newly registered. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Audio Pack")
    int32
    Request_RegisterAudioPack();

    /** Registers the Video pack (External-policy bridge over GEngine->GetGameUserSettings()). Idempotent per key. Returns how many settings were newly registered. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Register Video Pack")
    int32
    Request_RegisterVideoPack();

    /** Runs the engine hardware benchmark, applies + saves the result, and fires change delegates for every registered video.* key. No-op (with a Display log) under headless presentation. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Run Hardware Benchmark")
    bool
    Request_RunHardwareBenchmark();

    /**
     * Applies InNewResolution ("WIDTHxHEIGHT") and starts a revert countdown: unless
     * Request_ConfirmResolution arrives within InWindowSeconds, the prior resolution is restored.
     * Nothing is saved to GameUserSettings.ini until confirm or revert.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Set Resolution With Confirm Window")
    bool
    Request_SetResolutionWithConfirmWindow(
        const FString& InNewResolution,
        float InWindowSeconds);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Confirm Resolution")
    bool
    Request_ConfirmResolution();

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Flush Storage")
    void
    Request_FlushStorage();

    /**
     * Re-runs the boot merge against the storage provider: each stored value applies onto its
     * registered definition, or is retained as an orphan for a definition registered later.
     * Returns how many stored values were read.
     */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Request Reload From Storage")
    int32
    Request_ReloadFromStorage();

    UFUNCTION(BlueprintPure,
              Category = "Ck|GameSettings",
              DisplayName = "[Ck][GameSettings] Get Storage Provider")
    UCk_GameSettings_StorageProvider_UE*
    Get_StorageProvider() const;

private:
    struct FDeferredCVarApply
    {
        FName _Key;
        FCk_CVarRef _CVar;
        ECk_GameSettings_ValueType _ValueType = ECk_GameSettings_ValueType::Bool;
        FString _Value;
        double _EnqueuedAtSeconds = 0.0;
    };

private:
    auto DoFindValidationFailure(const FCk_GameSettings_SettingDefinition& InDefinition) const -> TOptional<FString>;
    auto DoResetToDefault(FName InKey) -> bool;
    auto DoFireChanged(FName InKey, const FString& InNewValue) -> void;

    auto DoGet_CurrentValueString(FName InKey, const FCk_GameSettings_SettingDefinition& InDefinition) const -> FString;
    auto DoCommitValue(FName InKey, const FCk_GameSettings_SettingDefinition& InDefinition, const FString& InNewValueString) -> bool;
    auto DoRouteApply(FName InKey, const FCk_GameSettings_SettingDefinition& InDefinition, const FString& InValueString) -> void;
    auto DoConsumeOrphanOrDefault(FName InKey, const FCk_GameSettings_SettingDefinition& InDefinition) -> void;
    auto DoAbsorbStoredValues(ECk_GameSettings_Scope InScope) -> int32;
    auto DoHandleTick() -> void;
    auto DoFlush() -> void;
    auto DoHandlePreLoadMap(const FWorldContext& InWorldContext, const FString& InMapName) -> void;

    static auto DoGet_IsPieWorldContext(const UGameInstance* InGameInstance) -> bool;

private:
    TMap<FName, FCk_GameSettings_SettingDefinition> _Definitions;
    TMap<FName, FString> _CurrentValues;
    TArray<FName> _RegistrationOrder;
    TMap<FName, FCk_Delegate_GameSettings_OnSettingChanged_MC> _ChangeBindings;

    UPROPERTY(Transient)
    TObjectPtr<UCk_GameSettings_StorageProvider_UE> _StorageProvider;

    TMap<FName, FCk_Delegate_GameSettings_ApplyHandler_Bool> _ApplyHandlers_Bool;
    TMap<FName, FCk_Delegate_GameSettings_ApplyHandler_Int32> _ApplyHandlers_Int32;
    TMap<FName, FCk_Delegate_GameSettings_ApplyHandler_Float> _ApplyHandlers_Float;
    TMap<FName, FCk_Delegate_GameSettings_ApplyHandler_String> _ApplyHandlers_String;

    TMap<FName, FCk_Delegate_GameSettings_ExternalGetter_Bool> _ExternalGetters_Bool;
    TMap<FName, FCk_Delegate_GameSettings_ExternalGetter_Int32> _ExternalGetters_Int32;
    TMap<FName, FCk_Delegate_GameSettings_ExternalGetter_Float> _ExternalGetters_Float;
    TMap<FName, FCk_Delegate_GameSettings_ExternalGetter_String> _ExternalGetters_String;

    TMap<FName, FString> _OrphanValues_Machine;
    TMap<FName, FString> _OrphanValues_Player0;

    TArray<FDeferredCVarApply> _DeferredCVarApplies;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UObject>> _PackHandlerObjects;

    bool _PendingSessionActive = false;
    TMap<FName, FString> _PendingPriorValues;

    bool _ResolutionConfirmActive = false;
    double _ResolutionConfirmDeadlineSeconds = 0.0;
    FIntPoint _ResolutionConfirmPriorResolution = FIntPoint::ZeroValue;

    bool _IsPieInstance = false;
    bool _FlushPending = false;
    double _LastDirtyTimeSeconds = 0.0;

    FTSTicker::FDelegateHandle _TickerHandle;
    FDelegateHandle _EnginePreExitHandle;
    FDelegateHandle _AppDeactivateHandle;
};

// --------------------------------------------------------------------------------------------------------------------
